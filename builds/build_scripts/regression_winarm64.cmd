@echo off
setlocal
set "ROOT=%~dp0..\.."
set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"
call "%~dp0build_winarm64.cmd" %CONFIG%
if errorlevel 1 exit /b 1
ctest --test-dir "%ROOT%\builds\drops\win-arm64\cmake\%CONFIG%" --output-on-failure
exit /b %errorlevel%
