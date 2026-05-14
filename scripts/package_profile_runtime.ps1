param(
    [string]$ProfilePath = "profiles/default.json",
    [string]$OutputDir = "dist/profile_package",
    [string]$BuildDir = "build_vs18",
    [string]$BuildConfig = "Release",
    [switch]$IncludeSource
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

    throw "app.exe not found. Build first. Checked: $($candidates -join '; ')"
}

function Load-JsonFile {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        throw "File not found: $Path"
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
            if ([string]::IsNullOrWhiteSpace([string]$manifest.name)) {
                return
            }

            $dependencies = @()
            if ($manifest.PSObject.Properties.Name -contains "dependencies") {
                foreach ($dep in $manifest.dependencies) {
                    $depName = [string]$dep
                    if (-not [string]::IsNullOrWhiteSpace($depName)) {
                        $dependencies += $depName
                    }
                }
            }

            $map[[string]$manifest.name] = [PSCustomObject]@{
                Name = [string]$manifest.name
                Root = $rootName
                DirName = $_.Name
                SourcePath = $_.FullName
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
        [ref]$Warnings
    )

    $warnings = @()
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
            $warnings += ("Profile plugin not found: {0}" -f $name)
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
                $warnings += ("Dependency missing for [{0}]: {1}" -f $name, $depName)
                continue
            }

            if (-not $selected.Contains($depName)) {
                $queue.Enqueue($depName)
            }
        }
    }

    $Warnings.Value = $warnings
    return @($selected)
}

function Copy-DirectoryContent {
    param(
        [string]$Source,
        [string]$Destination
    )

    if (-not (Test-Path $Source)) {
        return
    }

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    Copy-Item -Path (Join-Path $Source "*") -Destination $Destination -Recurse -Force
}

function Copy-PluginRuntimeContent {
    param(
        [pscustomobject]$Descriptor,
        [string]$TargetPath,
        [string]$BuildConfig,
        [ref]$Warnings
    )

    New-Item -ItemType Directory -Path $TargetPath -Force | Out-Null

    $pluginRoot = $Descriptor.SourcePath
    $manifestPath = Join-Path $pluginRoot "plugin.json"
    if (Test-Path $manifestPath) {
        Copy-Item -Path $manifestPath -Destination (Join-Path $TargetPath "plugin.json") -Force
    }

    $isDebugConfig = $BuildConfig -match 'Debug'
    $runtimeExt = @('.dll', '.so', '.dylib', '.pdb')
    $binaryCount = 0

    Get-ChildItem -Path $pluginRoot -File | ForEach-Object {
        $ext = $_.Extension.ToLowerInvariant()
        if (-not ($runtimeExt -contains $ext)) {
            return
        }

        $nameLower = $_.Name.ToLowerInvariant()
        if (-not $isDebugConfig) {
            if ($nameLower -match 'd\.(dll|pdb)$') {
                return
            }
        }

        Copy-Item -Path $_.FullName -Destination (Join-Path $TargetPath $_.Name) -Force
        $binaryCount++
    }

    $runtimeDirs = @('qml', 'resources', 'assets', 'config', 'configs', 'data', 'translations')
    foreach ($dirName in $runtimeDirs) {
        $srcDir = Join-Path $pluginRoot $dirName
        if (Test-Path $srcDir) {
            Copy-DirectoryContent -Source $srcDir -Destination (Join-Path $TargetPath $dirName)
        }
    }

    if ($binaryCount -eq 0) {
        $Warnings.Value += ('No runtime library found for plugin after config filter: {0}' -f $Descriptor.Name)
    }
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Get-AbsolutePath -BaseDir $scriptDir -InputPath ".."
$profileFile = Get-AbsolutePath -BaseDir $repoRoot -InputPath $ProfilePath
$outputRoot = Get-AbsolutePath -BaseDir $repoRoot -InputPath $OutputDir
$runtimeBin = Find-RuntimeBin -RepoRoot $repoRoot -BuildDir $BuildDir -BuildConfig $BuildConfig

Write-Host "[package_profile] repo=$repoRoot"
Write-Host "[package_profile] runtime_bin=$runtimeBin"
Write-Host "[package_profile] profile=$profileFile"
Write-Host "[package_profile] output=$outputRoot"
$modeName = if ($IncludeSource.IsPresent) { 'with_source' } else { 'runtime_only' }
Write-Host "[package_profile] mode=$modeName"

$profileJson = Load-JsonFile -Path $profileFile
$pluginMap = Scan-Plugins -RepoRoot $repoRoot
if ($pluginMap.Count -eq 0) {
    throw "No plugins found in common/ or plugins_business/."
}

$warnings = @()
$selectedNames = @(Resolve-PluginSelection -ProfileJson $profileJson -PluginMap $pluginMap -Warnings ([ref]$warnings))
if ($selectedNames.Count -eq 0) {
    throw "No plugins selected after profile resolution."
}

if (Test-Path $outputRoot) {
    Remove-Item -Path $outputRoot -Recurse -Force
}

New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $outputRoot "common") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $outputRoot "plugins_business") -Force | Out-Null

Copy-DirectoryContent -Source $runtimeBin -Destination (Join-Path $outputRoot "bin")
Copy-DirectoryContent -Source (Join-Path $repoRoot "profiles") -Destination (Join-Path $outputRoot "profiles")

$selectedRows = @()
foreach ($pluginName in ($selectedNames | Sort-Object)) {
    $descriptor = $pluginMap[$pluginName]
    $targetPath = Join-Path $outputRoot (Join-Path $descriptor.Root $descriptor.DirName)

    if ($IncludeSource.IsPresent) {
        Copy-DirectoryContent -Source $descriptor.SourcePath -Destination $targetPath
        $selectedRows += ("{0} -> {1}/{2} (with_source)" -f $pluginName, $descriptor.Root, $descriptor.DirName)
    } else {
        Copy-PluginRuntimeContent -Descriptor $descriptor -TargetPath $targetPath -BuildConfig $BuildConfig -Warnings ([ref]$warnings)
        $selectedRows += ("{0} -> {1}/{2} (runtime_only)" -f $pluginName, $descriptor.Root, $descriptor.DirName)
    }
}

$summary = @()
$summary += ("Pack time: {0}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"))
$summary += ("Profile: {0}" -f $profileFile)
$summary += ("BuildDir: {0}" -f $BuildDir)
$summary += ("BuildConfig: {0}" -f $BuildConfig)
$summary += ("Mode: {0}" -f $modeName)
$summary += ""
$summary += "Selected plugins:"
$summary += $selectedRows

if ($warnings.Count -gt 0) {
    $summary += ""
    $summary += "Warnings:"
    $summary += $warnings
}

$summaryPath = Join-Path $outputRoot "package_summary.txt"
$summary | Set-Content -Path $summaryPath -Encoding UTF8

$runScript = @(
    '@echo off',
    'setlocal',
    'set ROOT=%~dp0',
    'pushd %ROOT%',
    'if "%~1"=="" (',
    '  set PROFILE=profiles\default.json',
    ') else (',
    '  set PROFILE=%~1',
    ')',
    'bin\app.exe --profile %PROFILE%',
    'popd',
    'endlocal'
)
$runScript | Set-Content -Path (Join-Path $outputRoot "run_app.bat") -Encoding ASCII

Write-Host "[package_profile] done=$outputRoot"
Write-Host "[package_profile] plugin_count=$($selectedNames.Count)"
if ($warnings.Count -gt 0) {
    Write-Host "[package_profile] warning_count=$($warnings.Count)"
}

