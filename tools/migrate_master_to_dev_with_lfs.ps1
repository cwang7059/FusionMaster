param(
  [string]$GitLabRepo = 'git@10.11.12.249:03-01/2622013.git',
  [string]$RemoteName = 'intranet',
  [string]$SourceBranch = 'master',
  [string]$TargetBranch = 'dev',
  [string]$LfsObjectsDir = '',
  [switch]$Force,
  [switch]$PushTags
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
    return @{
      ExitCode = $LASTEXITCODE
      Output = @($output)
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
  $top = Invoke-Capture -Command @('git', 'rev-parse', '--show-toplevel')
  if ($top.ExitCode -ne 0 -or -not $top.Output -or $top.Output.Count -eq 0) {
    throw 'Current path is not a Git repository.'
  }
  return $top.Output[0].Trim()
}

function Ensure-Remote {
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

function Copy-LfsObjectsIfProvided {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
  )

  if (-not $LfsObjectsDir -or $LfsObjectsDir.Trim() -eq '') {
    return
  }

  if (-not (Test-Path $LfsObjectsDir)) {
    throw "LfsObjectsDir not found: $LfsObjectsDir"
  }

  $dst = Join-Path $RepoRoot '.git\lfs\objects'
  if (-not (Test-Path $dst)) {
    New-Item -ItemType Directory -Path $dst -Force | Out-Null
  }

  Write-Host "[lfs] import local LFS objects: $LfsObjectsDir -> $dst"
  $null = & robocopy $LfsObjectsDir $dst /E
  if ($LASTEXITCODE -ge 8) {
    throw "robocopy failed with exit code $LASTEXITCODE"
  }
}

try {
  $repoRoot = Get-RepoRoot
  Write-Host "Repo: $repoRoot"
  Write-Host "Target: $GitLabRepo"
  Write-Host "Mode: full-history-with-lfs"

  $lfsCheck = Invoke-Capture -Command @('git', 'lfs', 'version') -WorkingDirectory $repoRoot
  if ($lfsCheck.ExitCode -ne 0) {
    throw 'git-lfs is not installed or not available in PATH.'
  }

  Invoke-Checked -Command @('git', 'lfs', 'install', '--local') -WorkingDirectory $repoRoot
  Copy-LfsObjectsIfProvided -RepoRoot $repoRoot
  Ensure-Remote -RepoRoot $repoRoot

  $verifySource = Invoke-Capture -Command @('git', 'rev-parse', '--verify', "$SourceBranch^{commit}") -WorkingDirectory $repoRoot
  if ($verifySource.ExitCode -ne 0) {
    throw "Source branch not found: $SourceBranch"
  }

  Write-Host "[lfs] push all local LFS objects to remote..."
  $lfsPush = Invoke-Capture -Command @('git', 'lfs', 'push', '--all', $RemoteName) -WorkingDirectory $repoRoot
  if ($lfsPush.ExitCode -ne 0) {
    $text = ($lfsPush.Output -join [Environment]::NewLine)
    throw "git lfs push failed.`n$text`nHint: if objects are missing, copy '.git\\lfs\\objects' from the source machine first."
  }

  $refspec = "refs/heads/${SourceBranch}:refs/heads/${TargetBranch}"
  if ($Force) {
    $refspec = "+$refspec"
  }

  Write-Host "[git] push branch $SourceBranch -> $TargetBranch (force=$($Force.IsPresent))"
  Invoke-Checked -Command @('git', 'push', $RemoteName, $refspec) -WorkingDirectory $repoRoot

  if ($PushTags) {
    Write-Host '[git] push tags...'
    if ($Force) {
      Invoke-Checked -Command @('git', 'push', $RemoteName, '--tags', '--force') -WorkingDirectory $repoRoot
    }
    else {
      Invoke-Checked -Command @('git', 'push', $RemoteName, '--tags') -WorkingDirectory $repoRoot
    }
  }

  Write-Host 'SUCCESS: full history + LFS push completed.'
  exit 0
}
catch {
  Write-Error $_.Exception.Message
  exit 1
}
