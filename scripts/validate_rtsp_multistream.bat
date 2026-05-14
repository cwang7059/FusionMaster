@echo off
setlocal
chcp 65001 >nul

set SCRIPT_DIR=%~dp0
for %%I in ("%SCRIPT_DIR%..") do set REPO_ROOT=%%~fI

set BUILD_DIR=build_vs16_qt5
set BUILD_CONFIG=Release
set RTSP_URLS=cam1=rtsp://127.0.0.1:8554/live1;cam2=rtsp://127.0.0.1:8554/live2;cam3=rtsp://127.0.0.1:8554/live3
set RUN_SECONDS=12

if not "%~1"=="" set BUILD_DIR=%~1
if not "%~2"=="" set BUILD_CONFIG=%~2
if not "%~3"=="" set RTSP_URLS=%~3
if not "%~4"=="" set RUN_SECONDS=%~4

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%validate_rtsp_multistream.ps1" ^
  -RepoRoot "%REPO_ROOT%" ^
  -BuildDir "%BUILD_DIR%" ^
  -Config "%BUILD_CONFIG%" ^
  -RtspUrls "%RTSP_URLS%" ^
  -RunSeconds %RUN_SECONDS%

exit /b %errorlevel%

