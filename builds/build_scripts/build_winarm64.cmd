@echo off
setlocal
set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"
if /I not "%CONFIG%"=="Debug" if /I not "%CONFIG%"=="Release" (echo Usage: build_winarm64.cmd [Debug^|Release] & exit /b 2)
set "ROOT=%~dp0..\.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"
set "DROP=%ROOT%\builds\drops\win-arm64"
cmake -S "%ROOT%\builds\cmake_config" -B "%DROP%\cmake" -G "Visual Studio 17 2022" -A ARM64 -DCLINTER_DROP_ROOT="%DROP%"
if errorlevel 1 exit /b 1
cmake --build "%DROP%\cmake" --config %CONFIG%
if errorlevel 1 exit /b 1
echo Executable: %DROP%\bin\%CONFIG%\CLinter.exe
