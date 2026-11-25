@echo off
setlocal
set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"
if /I not "%CONFIG%"=="Debug" if /I not "%CONFIG%"=="Release" (echo Usage: build_win64.cmd [Debug^|Release] & exit /b 2)
set "ROOT=%~dp0..\.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"
set "DROP=%ROOT%\builds\drops\win-amd64"
set "VCPKG_ROOT=D:\vcpkg"
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo ERROR: vcpkg toolchain not found at %VCPKG_ROOT%
    exit /b 1
)
cmake -S "%ROOT%\builds\cmake_config" -B "%DROP%\makefiles" -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows -DCLINTER_DROP_ROOT="%DROP%"
if errorlevel 1 exit /b 1
cmake --build "%DROP%\makefiles" --config %CONFIG% --parallel
if errorlevel 1 exit /b 1
echo Executable: %DROP%\bin\%CONFIG%\CLinter.exe
