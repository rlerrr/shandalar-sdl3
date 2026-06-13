@echo off
setlocal

call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%

set "PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
set "APP=%~1"
if "%APP%"=="" set "APP=facemaker"

if /I "%APP%"=="deck" (
  set "BUILD_DIR=out\build\deck-clang-x86"
  set "BIN_DIR=deck-clang-x86"
  set "BUILD_FACEMAKER=OFF"
  set "BUILD_DECK=ON"
  set "BUILD_TESTS=OFF"
) else (
  set "BUILD_DIR=out\build\facemaker-clang-x86"
  set "BIN_DIR=facemaker-clang-x86"
  set "BUILD_FACEMAKER=ON"
  set "BUILD_DECK=OFF"
  set "BUILD_TESTS=ON"
)

cmake -S . -B "%BUILD_DIR%" -G Ninja ^
  -DCMAKE_BUILD_TYPE=MinSizeRel ^
  -DCMAKE_C_COMPILER="C:/Program Files (x86)/LLVM/bin/clang-cl.exe" ^
  -DCMAKE_CXX_COMPILER="C:/Program Files (x86)/LLVM/bin/clang-cl.exe" ^
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="%CD%/out/bin/%BIN_DIR%" ^
  -DSHANDALAR_ASSET_DIR="C:/Magic/Program" ^
  -DSHANDALAR_BUILD_FACEMAKER=%BUILD_FACEMAKER% ^
  -DSHANDALAR_BUILD_DECK=%BUILD_DECK% ^
  -DSHANDALAR_BUILD_TESTS=%BUILD_TESTS%
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%"
exit /b %errorlevel%
