param(
    [string]$ProfilePath = "profiles/default.json",
    [string]$BuildDir = "build_vs18",
    [string]$BuildConfig = "Release",
    [string]$ReportPath = "",
    [switch]$Strict
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-AbsolutePath {
    param(
        [string]$BaseDir,
        [string]$InputPath
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return $null
    }

    if ([System.IO.Path]::IsPathRooted($InputPath)) {
        return [System.IO.Path]::GetFullPath($InputPath)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $BaseDir $InputPath))
}

function Find-RuntimeBin {
    param(
        [string]$RepoRoot,
        [string]$BuildDir,
        [string]$BuildConfig
    )

    $candidates = @(
        (Join-Path $RepoRoot "$BuildDir/bin/$BuildConfig"),
        (Join-Path $RepoRoot "$BuildDir/bin")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path (Join-Path $candidate "app.exe")) {
            return $candidate
        }
    }

    throw "未找到 app.exe，请先编译。检查路径：$($candidates -join '; ')"
}

function Load-JsonFile {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        throw "文件不存在: $Path"
    }

    $raw = Get-Content -Path $Path -Raw -Encoding UTF8
    return $raw | ConvertFrom-Json
}

function Scan-Plugins {
    param([string]$RepoRoot)

    $map = @{}
    $roots = @("common", "plugins_business")

    foreach ($rootName in $roots) {
        $rootPath = Join-Path $RepoRoot $rootName
        if (-not (Test-Path $rootPath)) {
            continue
        }

        Get-ChildItem $rootPath -Directory | ForEach-Object {
            $manifestPath = Join-Path $_.FullName "plugin.json"
            if (-not (Test-Path $manifestPath)) {
                return
            }

            $manifest = Load-JsonFile -Path $manifestPath
            $pluginName = ([string]$manifest.name).Trim()
            if ([string]::IsNullOrWhiteSpace($pluginName)) {
                return
            }

            $dependencies = @()
            if ($manifest.PSObject.Properties.Name -contains "dependencies") {
                foreach ($dep in $manifest.dependencies) {
                    $depName = ([string]$dep).Trim()
                    if (-not [string]::IsNullOrWhiteSpace($depName)) {
                        $dependencies += $depName
                    }
                }
            }

            $map[$pluginName] = [PSCustomObject]@{
                Name = $pluginName
                Root = $rootName
                DirName = $_.Name
                SourcePath = $_.FullName
                ManifestPath = $manifestPath
                Dependencies = $dependencies
            }
        }
    }

    return $map
}

function Resolve-PluginSelection {
    param(
        [object]$ProfileJson,
        [hashtable]$PluginMap,
        [ref]$Warnings,
        [ref]$Errors
    )

    $warningList = @()
    $errorList = @()

    $selected = New-Object System.Collections.Generic.HashSet[string]
    $queue = New-Object System.Collections.Generic.Queue[string]

    $requested = @()
    if ($ProfileJson.PSObject.Properties.Name -contains "plugins") {
        foreach ($pluginName in $ProfileJson.plugins) {
            $name = ([string]$pluginName).Trim()
            if (-not [string]::IsNullOrWhiteSpace($name)) {
                $requested += $name
            }
        }
    }

    $loadAll = ($requested.Count -eq 0) -or ($requested -contains "*")
    if ($loadAll) {
        foreach ($name in $PluginMap.Keys) {
            $queue.Enqueue($name)
        }
    } else {
        foreach ($name in $requested) {
            $queue.Enqueue($name)
        }
    }

    while ($queue.Count -gt 0) {
        $name = $queue.Dequeue()
        if ([string]::IsNullOrWhiteSpace($name) -or $name -eq "*") {
            continue
        }
        if ($selected.Contains($name)) {
            continue
        }

        if (-not $PluginMap.ContainsKey($name)) {
            $errorList += ("Profile 指定插件不存在: {0}" -f $name)
            continue
        }

        [void]$selected.Add($name)
        $descriptor = $PluginMap[$name]

        foreach ($dep in $descriptor.Dependencies) {
            $depName = ([string]$dep).Trim()
            if ([string]::IsNullOrWhiteSpace($depName) -or $depName -eq "plugin_api") {
                continue
            }

            if (-not $PluginMap.ContainsKey($depName)) {
                $errorList += ("插件 [{0}] 依赖缺失: {1}" -f $name, $depName)
                continue
            }

            if (-not $selected.Contains($depName)) {
                $warningList += ("按依赖自动加入: {0} (被 {1} 依赖)" -f $depName, $name)
                $queue.Enqueue($depName)
            }
        }
    }

    $Warnings.Value = $warningList
    $Errors.Value = $errorList
    return @($selected)
}

function Find-PluginRuntimeLibrary {
    param([pscustomobject]$Descriptor)

    if ($null -eq $Descriptor -or [string]::IsNullOrWhiteSpace($Descriptor.SourcePath)) {
        return $null
    }

    if ($env:OS -eq "Windows_NT") {
        $ext = ".dll"
    } else {
        $ext = ".so"
    }

    $base = $Descriptor.SourcePath
    $name = $Descriptor.Name
    $candidates = @(
        (Join-Path $base ("{0}{1}" -f $name, $ext)),
        (Join-Path $base ("bin/{0}{1}" -f $name, $ext)),
        (Join-Path $base ("win-x64/bin/{0}.dll" -f $name)),
        (Join-Path $base ("linux-x64/bin/{0}.so" -f $name))
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return [System.IO.Path]::GetFullPath($candidate)
        }
    }

    $fallback = Get-ChildItem -Path $base -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Extension -in @(".dll", ".so", ".dylib") } |
        Select-Object -First 1

    if ($null -ne $fallback) {
        return $fallback.FullName
    }

    return $null
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Get-AbsolutePath -BaseDir $scriptDir -InputPath ".."
$profileFile = Get-AbsolutePath -BaseDir $repoRoot -InputPath $ProfilePath
$runtimeBin = Find-RuntimeBin -RepoRoot $repoRoot -BuildDir $BuildDir -BuildConfig $BuildConfig
$appPath = Join-Path $runtimeBin "app.exe"

if ([string]::IsNullOrWhiteSpace($ReportPath)) {
    $profileName = [System.IO.Path]::GetFileNameWithoutExtension($profileFile)
    $ReportPath = "dist/validate_{0}.txt" -f $profileName
}
$reportFile = Get-AbsolutePath -BaseDir $repoRoot -InputPath $ReportPath

$profileJson = Load-JsonFile -Path $profileFile
$pluginMap = Scan-Plugins -RepoRoot $repoRoot
if ($pluginMap.Count -eq 0) {
    throw "未扫描到任何插件（common/plugins_business）"
}

$warnings = @()
$errors = @()
$selectedNames = @(Resolve-PluginSelection -ProfileJson $profileJson -PluginMap $pluginMap -Warnings ([ref]$warnings) -Errors ([ref]$errors))

if ($selectedNames.Count -eq 0) {
    $errors += "按 profile 解析后没有可加载插件"
}

if (-not (Test-Path $appPath)) {
    $errors += ("app.exe 不存在: {0}" -f $appPath)
}

$rows = @()
foreach ($pluginName in ($selectedNames | Sort-Object)) {
    $descriptor = $pluginMap[$pluginName]
    $runtimeLib = Find-PluginRuntimeLibrary -Descriptor $descriptor
    $runtimeStatus = if ($null -eq $runtimeLib) { "缺失" } else { "OK" }

    if ($null -eq $runtimeLib) {
        $errors += ("插件运行库缺失: {0} ({1}/{2})" -f $descriptor.Name, $descriptor.Root, $descriptor.DirName)
    }
    $rows += ("- {0} | {1}/{2} | 运行库={3} | {4}" -f
        $descriptor.Name,
        $descriptor.Root,
        $descriptor.DirName,
        $runtimeStatus,
        ($(if ($null -eq $runtimeLib) { "-" } else { $runtimeLib })))
}

$reportLines = @()
$reportLines += ("验证时间: {0}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"))
$reportLines += ("Profile: {0}" -f $profileFile)
$reportLines += ("BuildDir: {0}" -f $BuildDir)
$reportLines += ("BuildConfig: {0}" -f $BuildConfig)
$reportLines += ("RuntimeBin: {0}" -f $runtimeBin)
$reportLines += ("AppPath: {0}" -f $appPath)
$reportLines += ""
$reportLines += ("插件总数: {0}" -f $pluginMap.Count)
$reportLines += ("选中插件: {0}" -f $selectedNames.Count)
$reportLines += ""
$reportLines += "选中插件明细:"
$reportLines += $rows

if ($warnings.Count -gt 0) {
    $reportLines += ""
    $reportLines += "警告:"
    $reportLines += ($warnings | ForEach-Object { "- $_" })
}

if ($errors.Count -gt 0) {
    $reportLines += ""
    $reportLines += "错误:"
    $reportLines += ($errors | ForEach-Object { "- $_" })
}

$reportDir = Split-Path -Parent $reportFile
if (-not (Test-Path $reportDir)) {
    New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
}
$reportLines | Set-Content -Path $reportFile -Encoding UTF8

Write-Host "[validate_profile] profile=$profileFile"
Write-Host "[validate_profile] report=$reportFile"
Write-Host "[validate_profile] selected=$($selectedNames.Count) warnings=$($warnings.Count) errors=$($errors.Count)"

if ($errors.Count -gt 0) {
    Write-Error "验收失败：存在错误，请查看报告。"
    exit 2
}

if ($Strict.IsPresent -and $warnings.Count -gt 0) {
    Write-Error "严格模式失败：存在警告，请查看报告。"
    exit 3
}

Write-Host "[validate_profile] pass"
exit 0

