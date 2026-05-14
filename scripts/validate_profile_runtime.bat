@echo off
setlocal

set SCRIPT_DIR=%~dp0

set PROFILE_PATH=%~1
if "%PROFILE_PATH%"=="" set PROFILE_PATH=profiles/default.json

set BUILD_DIR=%~2
if "%BUILD_DIR%"=="" set BUILD_DIR=build_vs18

set BUILD_CONFIG=%~3
if "%BUILD_CONFIG%"=="" set BUILD_CONFIG=Release

set REPORT_PATH=%~4
set STRICT_SWITCH=
if /I "%~4"=="strict" (
    set REPORT_PATH=
    set STRICT_SWITCH=-Strict
) else (
    if /I "%~5"=="strict" set STRICT_SWITCH=-Strict
)

echo [validate_profile] profile=%PROFILE_PATH%
echo [validate_profile] build=%BUILD_DIR% config=%BUILD_CONFIG%
if not "%REPORT_PATH%"=="" echo [validate_profile] report=%REPORT_PATH%
if not "%STRICT_SWITCH%"=="" echo [validate_profile] mode=strict

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%validate_profile_runtime.ps1" ^
    -ProfilePath "%PROFILE_PATH%" ^
    -BuildDir "%BUILD_DIR%" ^
    -BuildConfig "%BUILD_CONFIG%" ^
    -ReportPath "%REPORT_PATH%" %STRICT_SWITCH%

if errorlevel 1 (
    echo [validate_profile] failed
    exit /b 1
)

echo [validate_profile] success
exit /b 0
