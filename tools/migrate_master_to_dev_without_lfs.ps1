param(
  [string]$GitLabRepo = 'git@10.11.12.249:03-01/2622013.git',
  [string]$SourceBranch = 'master',
  [string]$TargetBranch = 'dev',
  [string]$RemoteName = 'intranet',
  [switch]$NoForce
)

$ErrorActionPreference = 'Stop'

$coreScript = Join-Path $PSScriptRoot 'push_to_gitlab_no_lfs.ps1'
if (-not (Test-Path $coreScript)) {
  throw "Required script not found: $coreScript"
}

$callParams = @{
  GitLabRepo = $GitLabRepo
  Mode = 'rewrite'
  SourceBranch = $SourceBranch
  TargetBranch = $TargetBranch
  RemoteName = $RemoteName
}

if (-not $NoForce) {
  $callParams['Force'] = $true
}

Write-Host "Start rewrite migration: $SourceBranch -> $TargetBranch"
Write-Host "Remote: $GitLabRepo"
if (-not $NoForce) {
  Write-Host "Force push: ON"
}
else {
  Write-Host "Force push: OFF"
}

& $coreScript @callParams
exit $LASTEXITCODE
