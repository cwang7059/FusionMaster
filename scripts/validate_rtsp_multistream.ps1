param(
    [string]$RepoRoot = "",
    [string]$BuildDir = "build_vs16_qt5",
    [string]$Config = "Release",
    [string]$RtspUrls = "cam1=rtsp://127.0.0.1:8554/live1;cam2=rtsp://127.0.0.1:8554/live2;cam3=rtsp://127.0.0.1:8554/live3",
    [int]$RunSeconds = 12
)

$ErrorActionPreference = "Stop"

function Get-LatestRuntimeLog {
    param([string]$LogRoot)

    if (-not (Test-Path $LogRoot)) {
        return $null
    }

    $dayDir = Get-ChildItem $LogRoot -Directory | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($null -eq $dayDir) {
        return $null
    }

    return Get-ChildItem $dayDir.FullName -File -Filter "runtime*.log" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
}

function Parse-ExpectedRtspCount {
    param([string]$Urls)

    if ([string]::IsNullOrWhiteSpace($Urls)) {
        return 0
    }

    $tokens = $Urls.Split(';', [System.StringSplitOptions]::RemoveEmptyEntries)
    $count = 0
    foreach ($token in $tokens) {
        $trimmed = $token.Trim()
        if ($trimmed -match "(?i)rtsp[s]?://") {
            $count++
        }
    }
    return $count
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
} else {
    $RepoRoot = (Resolve-Path $RepoRoot).Path
}

if ($RunSeconds -lt 3) {
    $RunSeconds = 3
}

$appExe = Join-Path $RepoRoot "$BuildDir\bin\$Config\app.exe"
if (-not (Test-Path $appExe)) {
    Write-Host "[validate_rtsp_multistream] FAIL: app.exe not found: $appExe"
    exit 1
}

$expectedCount = Parse-ExpectedRtspCount -Urls $RtspUrls
if ($expectedCount -lt 2) {
    Write-Host "[validate_rtsp_multistream] FAIL: require at least 2 RTSP URLs"
    Write-Host "[validate_rtsp_multistream] OSGI_RTSP_URLS=$RtspUrls"
    exit 1
}

$logRoot = Join-Path $RepoRoot "$BuildDir\bin\$Config\logs"

$env:OSGI_RTSP_URLS = $RtspUrls
$env:OSGI_RTSP_PACKET_LOG = "0"

Write-Host "[validate_rtsp_multistream] app=$appExe"
Write-Host "[validate_rtsp_multistream] urls=$RtspUrls"
Write-Host "[validate_rtsp_multistream] run=$RunSeconds sec"

$proc = Start-Process -FilePath $appExe -PassThru
Start-Sleep -Seconds $RunSeconds
if (-not $proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
}

$runtimeLog = Get-LatestRuntimeLog -LogRoot $logRoot
if ($null -eq $runtimeLog) {
    Write-Host "[validate_rtsp_multistream] FAIL: runtime log not found"
    exit 1
}

Write-Host "[validate_rtsp_multistream] log=$($runtimeLog.FullName)"

$lines = Get-Content $runtimeLog.FullName -Encoding UTF8

$pluginStarted = $false
$streamIds = New-Object System.Collections.Generic.HashSet[string]

foreach ($line in $lines) {
    if ($line -match "start via ctk:\s+org_common_rtsp_replay") {
        $pluginStarted = $true
    }

    # Match startup line format:
    # [RTSP_REPLAY] [流xxx] 启动: rtsp://...
    # We only depend on [RTSP_REPLAY], [id], and rtsp://
    if ($line -match "\[RTSP_REPLAY\]\s+\[([^\]]+)\].*rtsp[s]?://") {
        [void]$streamIds.Add($matches[1])
    }
}

$startedCount = $streamIds.Count
$required = [Math]::Min($expectedCount, 3)

Write-Host "[validate_rtsp_multistream] expected_streams=$expectedCount"
Write-Host "[validate_rtsp_multistream] started_streams=$startedCount"

if ($pluginStarted -and $startedCount -ge $required) {
    Write-Host "[validate_rtsp_multistream] PASS"
    exit 0
}

Write-Host "[validate_rtsp_multistream] FAIL"
Write-Host "[validate_rtsp_multistream] recent RTSP lines:"
$lines | Select-String -Pattern "RTSP_REPLAY|org_common_rtsp_replay" | Select-Object -Last 20 | ForEach-Object { Write-Host $_.Line }
exit 1

