@echo off
setlocal

if "%EMSDK%"=="" (
  echo EMSDK is not set. Run emsdk_env.bat first.
  exit /b 1
)

if "%SHANDALAR_ASSET_DIR%"=="" set "SHANDALAR_ASSET_DIR=C:/Magic/Program"
set "PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"

set "BUILD_DIR=out\build\facemaker-wasm"
call emcmake cmake -S . -B "%BUILD_DIR%" -G Ninja ^
  -DCMAKE_BUILD_TYPE=MinSizeRel ^
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="%CD%/out/bin/facemaker-wasm" ^
  -DSHANDALAR_ASSET_DIR="%SHANDALAR_ASSET_DIR%" ^
  -DSHANDALAR_BUILD_FACEMAKER=ON ^
  -DSHANDALAR_BUILD_TESTS=OFF
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%"
exit /b %errorlevel%
