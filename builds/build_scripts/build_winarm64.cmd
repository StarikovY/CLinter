@echo off
setlocal
set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"
if /I not "%CONFIG%"=="Debug" if /I not "%CONFIG%"=="Release" (echo Usage: build_winarm64.cmd [Debug^|Release] & exit /b 2)
set "ROOT=%~dp0..\.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"
set "DROP=%ROOT%\builds\drops\win-arm64"
set "VCPKG_TOOLCHAIN=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\vcpkg\scripts\buildsystems\vcpkg.cmake"
set "VCPKG_INSTALLED_DIR=N:\vcpkg\vcpkg_installed"
set "SDL2_DIR=%VCPKG_INSTALLED_DIR%\arm64-windows\share\sdl2"
if not exist "%VCPKG_TOOLCHAIN%" (echo ERROR: vcpkg toolchain not found & exit /b 1)
if not exist "%SDL2_DIR%\SDL2Config.cmake" if not exist "%SDL2_DIR%\sdl2-config.cmake" (echo ERROR: SDL2 ARM64 configuration not found & exit /b 1)
cmake -S "%ROOT%\builds\cmake_config" -B "%DROP%\cmake" -G "Visual Studio 17 2022" -A ARM64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN%" -DVCPKG_TARGET_TRIPLET=arm64-windows -DVCPKG_INSTALLED_DIR="%VCPKG_INSTALLED_DIR%" -DVCPKG_MANIFEST_MODE=OFF -DSDL2_DIR="%SDL2_DIR%" -DCLINTER_DROP_ROOT="%DROP%"
if errorlevel 1 exit /b 1
cmake --build "%DROP%\cmake" --config %CONFIG%
if errorlevel 1 exit /b 1
if /I "%CONFIG%"=="Debug" (set "SDL2_BIN=%VCPKG_INSTALLED_DIR%\arm64-windows\debug\bin") else (set "SDL2_BIN=%VCPKG_INSTALLED_DIR%\arm64-windows\bin")
copy /Y "%SDL2_BIN%\SDL2.dll" "%DROP%\bin\%CONFIG%\SDL2.dll" >nul
if errorlevel 1 exit /b 1
echo Executable: %DROP%\bin\%CONFIG%\CLinter.exe
