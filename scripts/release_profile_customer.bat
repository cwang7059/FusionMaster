@echo off
setlocal

set SCRIPT_DIR=%~dp0

set PROFILE_PATH=%~1
if "%PROFILE_PATH%"=="" set PROFILE_PATH=profiles/default.json

set OUTPUT_DIR=%~2
set BUILD_DIR=%~3
if "%BUILD_DIR%"=="" set BUILD_DIR=build_vs18

set BUILD_CONFIG=%~4
if "%BUILD_CONFIG%"=="" set BUILD_CONFIG=Release

set SYNC_BUSINESS=%~5
if "%SYNC_BUSINESS%"=="" set SYNC_BUSINESS=0

set BUSINESS_ROOT=%~6
if "%BUSINESS_ROOT%"=="" set BUSINESS_ROOT=C:\ZYL\workspace\persion\osgi_business_dev

set SKIP_TOOL_SYNC=%~7
if "%SKIP_TOOL_SYNC%"=="" set SKIP_TOOL_SYNC=1

if "%OUTPUT_DIR%"=="" (
    for %%I in ("%PROFILE_PATH%") do set PROFILE_NAME=%%~nI
    set OUTPUT_DIR=dist\release_%PROFILE_NAME%
)

echo [release] profile=%PROFILE_PATH%
echo [release] output=%OUTPUT_DIR%
echo [release] build=%BUILD_DIR% config=%BUILD_CONFIG%
echo [release] sync_business=%SYNC_BUSINESS%

echo [release] Step 1/3: build
call "%SCRIPT_DIR%build_all.bat" "%BUILD_DIR%" "%BUILD_CONFIG%"
if errorlevel 1 (
    echo [release] ERROR: build failed
    exit /b 1
)

echo [release] Step 2/3: validate profile strict
call "%SCRIPT_DIR%validate_profile_runtime.bat" "%PROFILE_PATH%" "%BUILD_DIR%" "%BUILD_CONFIG%" strict
if errorlevel 1 (
    echo [release] ERROR: validation failed
    exit /b 1
)

echo [release] Step 3/3: package customer runtime
call "%SCRIPT_DIR%package_customer_runtime.bat" "%PROFILE_PATH%" "%OUTPUT_DIR%" "%BUILD_DIR%" "%BUILD_CONFIG%"
if errorlevel 1 (
    echo [release] ERROR: package failed
    exit /b 1
)

if /I "%SYNC_BUSINESS%"=="1" (
    echo [release] Step 4/4: sync sdk to business repo
    call "%SCRIPT_DIR%publish_sdk_to_business_dev.bat" "%BUSINESS_ROOT%" "%BUILD_DIR%" "%BUILD_CONFIG%" "" "" "" "%SKIP_TOOL_SYNC%"
    if errorlevel 1 (
        echo [release] ERROR: business sync failed
        exit /b 1
    )
)

echo [release] DONE: %OUTPUT_DIR%
exit /b 0
