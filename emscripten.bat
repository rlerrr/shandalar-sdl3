@echo off
setlocal

if "%EMSDK%"=="" (
  echo EMSDK is not set. Run emsdk_env.bat first.
  exit /b 1
)

if "%SHANDALAR_ASSET_DIR%"=="" set "SHANDALAR_ASSET_DIR=C:/Magic/Program"
set "PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"

set "APP=%~1"
if "%APP%"=="" set "APP=facemaker"

if /I "%APP%"=="deck" (
  set "BUILD_DIR=out\build\deck-wasm"
  set "BIN_DIR=deck-wasm"
  set "BUILD_FACEMAKER=OFF"
  set "BUILD_DECK=ON"
) else (
  set "BUILD_DIR=out\build\facemaker-wasm"
  set "BIN_DIR=facemaker-wasm"
  set "BUILD_FACEMAKER=ON"
  set "BUILD_DECK=OFF"
)

call emcmake cmake -S . -B "%BUILD_DIR%" -G Ninja ^
  -DCMAKE_BUILD_TYPE=MinSizeRel ^
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="%CD%/out/bin/%BIN_DIR%" ^
  -DSHANDALAR_ASSET_DIR="%SHANDALAR_ASSET_DIR%" ^
  -DSHANDALAR_BUILD_FACEMAKER=%BUILD_FACEMAKER% ^
  -DSHANDALAR_BUILD_DECK=%BUILD_DECK% ^
  -DSHANDALAR_BUILD_TESTS=OFF
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%"
exit /b %errorlevel%
