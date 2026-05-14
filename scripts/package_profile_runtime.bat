@echo off
setlocal

set SCRIPT_DIR=%~dp0

set PROFILE_PATH=%~1
if "%PROFILE_PATH%"=="" set PROFILE_PATH=profiles/default.json

set OUTPUT_DIR=%~2
if "%OUTPUT_DIR%"=="" set OUTPUT_DIR=dist/profile_package

set BUILD_DIR=%~3
if "%BUILD_DIR%"=="" set BUILD_DIR=build_vs18

set BUILD_CONFIG=%~4
if "%BUILD_CONFIG%"=="" set BUILD_CONFIG=Release

set INCLUDE_SOURCE_SWITCH=
if /I "%~5"=="with_source" set INCLUDE_SOURCE_SWITCH=-IncludeSource

echo [package_profile] profile=%PROFILE_PATH%
echo [package_profile] output=%OUTPUT_DIR%
echo [package_profile] build=%BUILD_DIR% config=%BUILD_CONFIG%
if not "%INCLUDE_SOURCE_SWITCH%"=="" (
    echo [package_profile] mode=with_source
) else (
    echo [package_profile] mode=runtime_only
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%package_profile_runtime.ps1" ^
    -ProfilePath "%PROFILE_PATH%" ^
    -OutputDir "%OUTPUT_DIR%" ^
    -BuildDir "%BUILD_DIR%" ^
    -BuildConfig "%BUILD_CONFIG%" %INCLUDE_SOURCE_SWITCH%

if errorlevel 1 (
    echo [package_profile] failed
    exit /b 1
)

echo [package_profile] success
exit /b 0
