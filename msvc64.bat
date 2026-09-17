@echo off
setlocal

call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%

set "APP=%~1"
if "%APP%"=="" set "APP=facemaker"

if /I "%APP%"=="deck" (
  set "BUILD_DIR=out\build\deck-msvc-x64"
  set "BIN_DIR=deck-msvc-x64"
  set "BUILD_FACEMAKER=OFF"
  set "BUILD_SHANDALAR=OFF"
  set "BUILD_DECK=ON"
  set "BUILD_TESTS=OFF"
) else if /I "%APP%"=="shandalar" (
  set "BUILD_DIR=out\build\shandalar-msvc-x64"
  set "BIN_DIR=shandalar-msvc-x64"
  set "BUILD_FACEMAKER=OFF"
  set "BUILD_SHANDALAR=ON"
  set "BUILD_DECK=OFF"
  set "BUILD_TESTS=OFF"
) else (
  set "BUILD_DIR=out\build\facemaker-msvc-x64"
  set "BIN_DIR=facemaker-msvc-x64"
  set "BUILD_FACEMAKER=ON"
  set "BUILD_SHANDALAR=OFF"
  set "BUILD_DECK=OFF"
  set "BUILD_TESTS=ON"
)

cmake -S . -B "%BUILD_DIR%" -G Ninja ^
  -DCMAKE_BUILD_TYPE=MinSizeRel ^
  -DCMAKE_C_COMPILER=cl.exe ^
  -DCMAKE_CXX_COMPILER=cl.exe ^
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="%CD%/out/bin/%BIN_DIR%" ^
  -DSHANDALAR_ASSET_DIR="C:/Magic/Program" ^
  -DSHANDALAR_BUILD_FACEMAKER=%BUILD_FACEMAKER% ^
  -DSHANDALAR_BUILD_SHANDALAR=%BUILD_SHANDALAR% ^
  -DSHANDALAR_BUILD_DECK=%BUILD_DECK% ^
  -DSHANDALAR_BUILD_TESTS=%BUILD_TESTS%
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%"
if errorlevel 1 exit /b 1

if /I "%BUILD_TESTS%"=="ON" ctest --test-dir "%BUILD_DIR%" --output-on-failure
exit /b %errorlevel%
