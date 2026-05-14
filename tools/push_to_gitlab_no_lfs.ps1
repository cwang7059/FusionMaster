param(
  [Parameter(Mandatory = $true)]
  [string]$GitLabRepo,

  [ValidateSet('auto', 'mirror', 'snapshot', 'branch', 'rewrite')]
  [string]$Mode = 'auto',

  [string]$RemoteName = 'intranet',

  [string]$SnapshotBranch = 'main',

  [string]$SourceBranch = 'master',

  [string]$TargetBranch = 'dev',

  [switch]$Force
)

$ErrorActionPreference = 'Stop'

function Invoke-Checked {
  param(
    [Parameter(Mandatory = $true)]
    [string[]]$Command,
    [string]$WorkingDirectory
  )

  if ($WorkingDirectory -and $WorkingDirectory.Trim() -ne '') {
    Push-Location $WorkingDirectory
  }

  $oldErrorPref = $ErrorActionPreference
  $ErrorActionPreference = 'Continue'

  try {
    $exe = $Command[0]
    $args = @()
    if ($Command.Count -gt 1) {
      $args = $Command[1..($Command.Count - 1)]
    }

    & $exe @args
    if ($LASTEXITCODE -ne 0) {
      throw "Command failed: $($Command -join ' ')"
    }
  }
  finally {
    $ErrorActionPreference = $oldErrorPref
    if ($WorkingDirectory -and $WorkingDirectory.Trim() -ne '') {
      Pop-Location
    }
  }
}

function Invoke-Capture {
  param(
    [Parameter(Mandatory = $true)]
    [string[]]$Command,
    [string]$WorkingDirectory
  )

  if ($WorkingDirectory -and $WorkingDirectory.Trim() -ne '') {
    Push-Location $WorkingDirectory
  }

  $oldErrorPref = $ErrorActionPreference
  $ErrorActionPreference = 'Continue'

  try {
    $exe = $Command[0]
    $args = @()
    if ($Command.Count -gt 1) {
      $args = $Command[1..($Command.Count - 1)]
    }

    $output = & $exe @args 2>&1
    $exitCode = $LASTEXITCODE
    return @{
      Output = @($output)
      ExitCode = $exitCode
    }
  }
  finally {
    $ErrorActionPreference = $oldErrorPref
    if ($WorkingDirectory -and $WorkingDirectory.Trim() -ne '') {
      Pop-Location
    }
  }
}

function Get-RepoRoot {
  $result = Invoke-Capture -Command @('git', 'rev-parse', '--show-toplevel')
  if ($result.ExitCode -ne 0 -or -not $result.Output -or $result.Output.Count -eq 0) {
    throw 'Current path is not a Git repository. Please run in a Git repo.'
  }
  return $result.Output[0].Trim()
}

function Ensure-RemoteUrl {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
  )

  $exists = Invoke-Capture -Command @('git', 'remote', 'get-url', $RemoteName) -WorkingDirectory $RepoRoot
  if ($exists.ExitCode -eq 0) {
    Invoke-Checked -Command @('git', 'remote', 'set-url', $RemoteName, $GitLabRepo) -WorkingDirectory $RepoRoot
  }
  else {
    Invoke-Checked -Command @('git', 'remote', 'add', $RemoteName, $GitLabRepo) -WorkingDirectory $RepoRoot
  }
}

function Push-Mirror {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
  )

  Write-Host "[mirror] configure remote '$RemoteName' => $GitLabRepo"
  Ensure-RemoteUrl -RepoRoot $RepoRoot

  $forceArgs = @()
  if ($Force) {
    $forceArgs = @('--force')
  }

  Write-Host "[mirror] push all branches..."
  Invoke-Checked -Command (@('git', 'push', $RemoteName, '--all') + $forceArgs) -WorkingDirectory $RepoRoot

  Write-Host "[mirror] push all tags..."
  Invoke-Checked -Command (@('git', 'push', $RemoteName, '--tags') + $forceArgs) -WorkingDirectory $RepoRoot
}

function Push-Branch {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
  )

  Write-Host "[branch] configure remote '$RemoteName' => $GitLabRepo"
  Ensure-RemoteUrl -RepoRoot $RepoRoot

  $verifySource = Invoke-Capture -Command @('git', 'rev-parse', '--verify', "$SourceBranch^{commit}") -WorkingDirectory $RepoRoot
  if ($verifySource.ExitCode -ne 0) {
    throw "Source branch not found: $SourceBranch"
  }

  $refspec = "refs/heads/${SourceBranch}:refs/heads/${TargetBranch}"
  if ($Force) {
    $refspec = "+$refspec"
  }

  Write-Host "[branch] push $SourceBranch -> $TargetBranch (force=$($Force.IsPresent))"
  Invoke-Checked -Command @('git', 'push', $RemoteName, $refspec) -WorkingDirectory $RepoRoot
}

function ConvertTo-ShSingleQuoted {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Value
  )

  return "'" + $Value + "'"
}

function Get-LfsPatternsFromAttributes {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
  )

  $attrFile = Join-Path $RepoRoot '.gitattributes'
  $patterns = @()

  if (Test-Path $attrFile) {
    foreach ($raw in Get-Content $attrFile) {
      if ($null -eq $raw) { continue }
      $line = $raw.Trim()
      if ($line -eq '' -or $line.StartsWith('#')) { continue }
      if ($line -notmatch '(^|\s)filter=lfs(\s|$)') { continue }

      $parts = $line -split '\s+'
      if ($parts.Count -gt 0) {
        $pathSpec = $parts[0].Trim()
        if ($pathSpec -ne '') {
          $patterns += $pathSpec
        }
      }
    }
  }

  if ($patterns.Count -eq 0) {
    $patterns = @('*.dll', '*.exe', '*.so', '*.a', '*.zip', '*.7z', '*.lib', '*.pdb', '*.dylib')
  }

  return @($patterns | Select-Object -Unique)
}

function Rewrite-And-PushBranchWithoutLfsHistory {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
  )

  $patterns = Get-LfsPatternsFromAttributes -RepoRoot $RepoRoot
  if (-not $patterns -or $patterns.Count -eq 0) {
    throw 'No LFS patterns found for rewrite.'
  }

  $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
  $tempMirror = Join-Path $env:TEMP ("gitlab-rewrite-$stamp-$([guid]::NewGuid().ToString('N').Substring(0, 8)).git")

  try {
    Write-Host "[rewrite] clone --mirror to temp: $tempMirror"
    Invoke-Checked -Command @('git', 'clone', '--mirror', $RepoRoot, $tempMirror)

    Disable-LfsFilters -RepoRoot $tempMirror

    $verifySource = Invoke-Capture -Command @('git', 'rev-parse', '--verify', "refs/heads/$SourceBranch^{commit}") -WorkingDirectory $tempMirror
    if ($verifySource.ExitCode -ne 0) {
      throw "Source branch not found in mirror repo: $SourceBranch"
    }

    $quotedPathSpecs = @()
    foreach ($pattern in $patterns) {
      $p = $pattern.Trim()
      if ($p -ne '') {
        $quotedPathSpecs += (ConvertTo-ShSingleQuoted -Value $p)
      }
    }

    if ($quotedPathSpecs.Count -eq 0) {
      throw 'No valid path specs generated for rewrite.'
    }

    $indexFilter = 'git rm -r --cached --ignore-unmatch -- ' + ($quotedPathSpecs -join ' ')
    Write-Host "[rewrite] rewrite branch history to drop LFS paths (count=$($quotedPathSpecs.Count))..."

    $oldFilterWarn = $env:FILTER_BRANCH_SQUELCH_WARNING
    $env:FILTER_BRANCH_SQUELCH_WARNING = '1'
    try {
      Invoke-Checked -Command @(
        'git', 'filter-branch',
        '--force',
        '--index-filter', $indexFilter,
        '--prune-empty',
        '--', "refs/heads/$SourceBranch"
      ) -WorkingDirectory $tempMirror
    }
    finally {
      if ($null -ne $oldFilterWarn) {
        $env:FILTER_BRANCH_SQUELCH_WARNING = $oldFilterWarn
      }
      else {
        Remove-Item Env:FILTER_BRANCH_SQUELCH_WARNING -ErrorAction SilentlyContinue
      }
    }

    $originalRefs = Invoke-Capture -Command @('git', 'for-each-ref', '--format=%(refname)', 'refs/original/') -WorkingDirectory $tempMirror
    if ($originalRefs.ExitCode -eq 0 -and $originalRefs.Output.Count -gt 0) {
      foreach ($item in $originalRefs.Output) {
        $ref = $item.Trim()
        if ($ref -ne '') {
          Invoke-Checked -Command @('git', 'update-ref', '-d', $ref) -WorkingDirectory $tempMirror
        }
      }
    }

    Invoke-Checked -Command @('git', 'reflog', 'expire', '--expire=now', '--all') -WorkingDirectory $tempMirror
    Invoke-Checked -Command @('git', 'gc', '--prune=now', '--aggressive') -WorkingDirectory $tempMirror

    Write-Host "[rewrite] configure remote '$RemoteName' => $GitLabRepo"
    Ensure-RemoteUrl -RepoRoot $tempMirror

    $refspec = "refs/heads/${SourceBranch}:refs/heads/${TargetBranch}"
    if ($Force) {
      $refspec = "+$refspec"
    }
    else {
      Write-Warning 'rewrite mode usually needs -Force when target branch already exists.'
    }

    Write-Host "[rewrite] push rewritten history $SourceBranch -> $TargetBranch (force=$($Force.IsPresent))"
    Invoke-Checked -Command @('git', 'push', $RemoteName, $refspec) -WorkingDirectory $tempMirror
  }
  finally {
    if (Test-Path $tempMirror) {
      Remove-Item -LiteralPath $tempMirror -Recurse -Force -ErrorAction SilentlyContinue
    }
  }
}

function Disable-LfsFilters {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
  )

  # Make this temporary repo independent from git-lfs availability.
  Invoke-Checked -Command @('git', 'config', 'filter.lfs.required', 'false') -WorkingDirectory $RepoRoot
  Invoke-Checked -Command @('git', 'config', 'filter.lfs.clean', 'cat') -WorkingDirectory $RepoRoot
  Invoke-Checked -Command @('git', 'config', 'filter.lfs.smudge', 'cat') -WorkingDirectory $RepoRoot
  Invoke-Checked -Command @('git', 'config', 'core.autocrlf', 'false') -WorkingDirectory $RepoRoot
  Invoke-Checked -Command @('git', 'config', 'core.safecrlf', 'false') -WorkingDirectory $RepoRoot
  $unsetProcess = Invoke-Capture -Command @('git', 'config', '--unset-all', 'filter.lfs.process') -WorkingDirectory $RepoRoot
  if ($unsetProcess.ExitCode -ne 0) {
    # Key can be absent; that's fine.
  }
}

function Warn-IfLfsPointerExists {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
  )

  $probe = Invoke-Capture -Command @('git', 'grep', '-I', '-n', '-m', '1', 'version https://git-lfs.github.com/spec/v1', '--', '.') -WorkingDirectory $RepoRoot
  if ($probe.ExitCode -eq 0 -and $probe.Output.Count -gt 0) {
    Write-Warning 'Detected LFS pointer files in current content. If large files were not copied in, snapshot push still contains pointer text.'
  }
}

function Copy-RepoWithoutGit {
  param(
    [Parameter(Mandatory = $true)]
    [string]$SourceRoot,
    [Parameter(Mandatory = $true)]
    [string]$DestinationRoot
  )

  if (-not (Test-Path $DestinationRoot)) {
    New-Item -ItemType Directory -Path $DestinationRoot | Out-Null
  }

  $null = & robocopy $SourceRoot $DestinationRoot /E /XD .git
  $code = $LASTEXITCODE
  if ($code -ge 8) {
    throw "robocopy failed with exit code $code"
  }
}

function Push-Snapshot {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
  )

  Warn-IfLfsPointerExists -RepoRoot $RepoRoot

  $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
  $tempRoot = Join-Path $env:TEMP ("gitlab-snapshot-$stamp-$([guid]::NewGuid().ToString('N').Substring(0, 8))")

  try {
    Write-Host "[snapshot] copying files to: $tempRoot"
    Copy-RepoWithoutGit -SourceRoot $RepoRoot -DestinationRoot $tempRoot

    Write-Host "[snapshot] init new repo..."
    Invoke-Checked -Command @('git', 'init', '-b', $SnapshotBranch) -WorkingDirectory $tempRoot
    Disable-LfsFilters -RepoRoot $tempRoot

    $sourceName = (Invoke-Capture -Command @('git', 'config', '--get', 'user.name') -WorkingDirectory $RepoRoot)
    $sourceMail = (Invoke-Capture -Command @('git', 'config', '--get', 'user.email') -WorkingDirectory $RepoRoot)
    if ($sourceName.ExitCode -eq 0 -and $sourceName.Output.Count -gt 0 -and $sourceName.Output[0].Trim() -ne '') {
      Invoke-Checked -Command @('git', 'config', 'user.name', $sourceName.Output[0].Trim()) -WorkingDirectory $tempRoot
    }
    if ($sourceMail.ExitCode -eq 0 -and $sourceMail.Output.Count -gt 0 -and $sourceMail.Output[0].Trim() -ne '') {
      Invoke-Checked -Command @('git', 'config', 'user.email', $sourceMail.Output[0].Trim()) -WorkingDirectory $tempRoot
    }

    Write-Host "[snapshot] create import commit..."
    Invoke-Checked -Command @('git', 'add', '-A') -WorkingDirectory $tempRoot
    $hasChanges = Invoke-Capture -Command @('git', 'diff', '--cached', '--quiet') -WorkingDirectory $tempRoot
    if ($hasChanges.ExitCode -eq 0) {
      throw 'No files to commit in snapshot repo.'
    }

    $repoName = Split-Path $RepoRoot -Leaf
    $commitMsg = "snapshot import from $repoName ($stamp)"
    Invoke-Checked -Command @('git', 'commit', '--quiet', '-m', $commitMsg) -WorkingDirectory $tempRoot

    Write-Host "[snapshot] push to $GitLabRepo (branch: $SnapshotBranch)..."
    Invoke-Checked -Command @('git', 'remote', 'add', 'origin', $GitLabRepo) -WorkingDirectory $tempRoot

    $pushArgs = @('git', 'push', '-u', 'origin', $SnapshotBranch)
    if ($Force) {
      $pushArgs += '--force'
    }
    Invoke-Checked -Command $pushArgs -WorkingDirectory $tempRoot
  }
  finally {
    if (Test-Path $tempRoot) {
      Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
  }
}

try {
  $repoRoot = Get-RepoRoot
  Write-Host "Repo: $repoRoot"
  Write-Host "Target: $GitLabRepo"
  Write-Host "Mode: $Mode"

  switch ($Mode) {
    'rewrite' {
      Rewrite-And-PushBranchWithoutLfsHistory -RepoRoot $repoRoot
      Write-Host 'SUCCESS: rewrite push completed.'
      break
    }
    'branch' {
      Push-Branch -RepoRoot $repoRoot
      Write-Host 'SUCCESS: branch push completed.'
      break
    }
    'mirror' {
      Push-Mirror -RepoRoot $repoRoot
      Write-Host 'SUCCESS: mirror push completed.'
      break
    }
    'snapshot' {
      Push-Snapshot -RepoRoot $repoRoot
      Write-Host 'SUCCESS: snapshot push completed.'
      break
    }
    'auto' {
      try {
        Push-Mirror -RepoRoot $repoRoot
        Write-Host 'SUCCESS: mirror push completed.'
      }
      catch {
        Write-Warning "mirror push failed: $($_.Exception.Message)"
        Write-Host 'auto fallback to snapshot push...'
        Push-Snapshot -RepoRoot $repoRoot
        Write-Host 'SUCCESS: snapshot push completed.'
      }
      break
    }
  }

  exit 0
}
catch {
  Write-Error $_.Exception.Message
  exit 1
}
