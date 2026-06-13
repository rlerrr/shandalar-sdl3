@echo off
setlocal

call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%

set "PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
set "BUILD_DIR=out\build\facemaker-clang-x86"

cmake -S . -B "%BUILD_DIR%" -G Ninja ^
  -DCMAKE_BUILD_TYPE=MinSizeRel ^
  -DCMAKE_C_COMPILER="C:/Program Files (x86)/LLVM/bin/clang-cl.exe" ^
  -DCMAKE_CXX_COMPILER="C:/Program Files (x86)/LLVM/bin/clang-cl.exe" ^
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="%CD%/out/bin/facemaker-clang-x86" ^
  -DSHANDALAR_ASSET_DIR="C:/Magic/Program" ^
  -DSHANDALAR_BUILD_FACEMAKER=ON ^
  -DSHANDALAR_BUILD_TESTS=ON
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%"
exit /b %errorlevel%
