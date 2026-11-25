@echo off
setlocal
rem CLinter Windows AMD64 Release - configure with SDL2 via D:\vcpkg
cd /d "%~dp0" || exit /b 1
set "SOURCE=%~dp0..\cmake_config"
set "BUILD=%~dp0..\drops\win-amd64\makefiles"
set "TOOLCHAIN=D:\vcpkg\scripts\buildsystems\vcpkg.cmake"
if not exist "%TOOLCHAIN%" (
    echo ERROR: vcpkg toolchain not found: %TOOLCHAIN%
    exit /b 1
)
if not exist "D:\vcpkg\installed\x64-windows\share\sdl2\SDL2Config.cmake" (
    echo WARNING: SDL2Config.cmake not found at expected vcpkg location.
    echo Install SDL2 using D:\vcpkg\vcpkg.exe install sdl2:x64-windows
)
rem --fresh clears stale CMake cache, including previous SDL2_DIR-NOTFOUND.
cmake -S "%SOURCE%" -B "%BUILD%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" -DVCPKG_TARGET_TRIPLET=x64-windows --fresh
if errorlevel 1 goto failed
rem The project must enable its SDL2 implementation, not merely discover the package.
findstr /I /C:"CLINTER_HAVE_SDL2" "%BUILD%\CLinter.vcxproj" >nul
if errorlevel 1 (
    echo ERROR: CLINTER_HAVE_SDL2 is missing from CLinter.vcxproj.
    echo Check cmake_config\CMakeLists.txt SDL2 detection and target_compile_definitions.
    exit /b 1
)
cmake --build "%BUILD%" --config Release --parallel
if errorlevel 1 goto failed
echo CLinter Windows AMD64 Release build completed with SDL2 enabled.
exit /b 0
:failed
echo ERROR: CLinter Windows AMD64 Release configure/build failed.
exit /b 1
