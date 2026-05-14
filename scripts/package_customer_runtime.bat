@echo off
setlocal
chcp 65001 >nul

set SCRIPT_DIR=%~dp0

set PROFILE_PATH=%~1
set OUTPUT_DIR=%~2
set BUILD_DIR=%~3
set BUILD_CONFIG=%~4

if "%PROFILE_PATH%"=="" (
    echo [package_customer] ??: package_customer_runtime.bat ^<profile.json^> [output_dir] [build_dir] [config]
    echo [package_customer] ??: package_customer_runtime.bat profiles/default.json dist/customer_default build_vs18 Release
    exit /b 1
)

if "%OUTPUT_DIR%"=="" set OUTPUT_DIR=dist\customer_runtime
if "%BUILD_DIR%"=="" set BUILD_DIR=build_vs18
if "%BUILD_CONFIG%"=="" set BUILD_CONFIG=Release

echo [package_customer] profile=%PROFILE_PATH%
echo [package_customer] output=%OUTPUT_DIR%
echo [package_customer] build=%BUILD_DIR% config=%BUILD_CONFIG%
echo [package_customer] mode=runtime_only (?????????)

call "%SCRIPT_DIR%package_profile_runtime.bat" "%PROFILE_PATH%" "%OUTPUT_DIR%" "%BUILD_DIR%" "%BUILD_CONFIG%"
if errorlevel 1 (
    echo [package_customer] ERROR: ????
    exit /b 1
)

echo [package_customer] DONE
endlocal
exit /b 0
