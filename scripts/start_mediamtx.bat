@echo off
setlocal

set ROOT_DIR=%~dp0..
set MEDIA_MTX_EXE=%ROOT_DIR%\third_party\mediamtx\win-x64\mediamtx.exe
set MEDIA_MTX_CFG=%ROOT_DIR%\third_party\mediamtx\win-x64\mediamtx.yml

if not exist "%MEDIA_MTX_EXE%" (
    echo [ERROR] mediamtx not found: %MEDIA_MTX_EXE%
    exit /b 1
)

if not exist "%MEDIA_MTX_CFG%" (
    echo [ERROR] mediamtx config not found: %MEDIA_MTX_CFG%
    exit /b 1
)

echo [INFO] starting mediamtx...
"%MEDIA_MTX_EXE%" "%MEDIA_MTX_CFG%"
