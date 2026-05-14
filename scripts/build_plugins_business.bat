@echo off
setlocal

set BUILD_DIR=build
set PLUGINS=*

if not "%~1"=="" set PLUGINS=%~1
if not "%~2"=="" set BUILD_DIR=%~2

cmake -S . -B %BUILD_DIR% -DBUSINESS_PLUGIN_FILTER="%PLUGINS%"
if errorlevel 1 exit /b 1

cmake --build %BUILD_DIR% --config Debug
if errorlevel 1 exit /b 1

endlocal
