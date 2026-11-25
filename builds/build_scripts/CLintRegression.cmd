@echo off
setlocal EnableExtensions DisableDelayedExpansion
rem Install in builds\build_scripts. Optional argument: win-amd64 or win-arm64.
for %%R in ("%~dp0..\..") do set "ROOT=%%~fR"
set "PLATFORM=%~1"
if not defined PLATFORM set "PLATFORM=win-amd64"
if /i "%PROCESSOR_ARCHITECTURE%"=="ARM64" if "%~1"=="" set "PLATFORM=win-arm64"
if /i "%PROCESSOR_ARCHITEW6432%"=="ARM64" if "%~1"=="" set "PLATFORM=win-arm64"
if /i "%PLATFORM%"=="win-amd64" goto platform_ok
if /i "%PLATFORM%"=="win-arm64" goto platform_ok
echo ERROR: Expected win-amd64 or win-arm64.
exit /b 1
:platform_ok
set "EXE=%ROOT%\builds\drops\%PLATFORM%\bin\Release\CLinter.exe"
if not exist "%EXE%" (
    echo ERROR: Release executable not found: "%EXE%"
    exit /b 1
)
pushd "%ROOT%" || exit /b 1
set "PASSED=0"
echo CLinter R1 - %PLATFORM% Release
call :run B01 GenericRegTest.bas || goto failed
call :run B02 regression.bas || goto failed
call :run B03 FloatIntegration.bas || goto failed
call :run B04 ColorScreen1.bas || goto failed
call :run B05 Screen20Test.bas || goto failed
call :run B06 GraphicsDay5.bas || goto failed
call :run B07 SpritePhaseG6.bas || goto failed
call :run B08 utka.bas || goto failed
call :run B09 DuckHuntD1.bas || goto failed
call :run B10 DuckHunt720p.bas || goto failed
call :run B11 Life720p.bas || goto failed
call :run B12 spiral.bas || goto failed
call :run B13 StairwayCloudsDemo.bas || goto failed
echo ALL 13 BASIC TESTS PASSED.
popd
exit /b 0
:failed
echo REGRESSION FAILED. Passed %PASSED% of 13. No further tests run.
popd
exit /b 1
:run
set "TEST=%~1"
set "NAME=%~2"
set "FILE="
set "MATCHES=0"
rem Resolve only under tests; reject ambiguous filenames.
for /r "%ROOT%\tests" %%F in (*.bas) do if /i "%%~nxF"=="%NAME%" call :match "%%~fF"
if not "%MATCHES%"=="1" (
    echo FAIL %TEST%: %NAME% - found %MATCHES% matching files.
    exit /b 1
)
echo.
echo [%TEST%] "%FILE%"
"%EXE%" "%FILE%"
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" (
    echo FAIL %TEST%: exit code %RC%.
    exit /b 1
)
choice /c PF /n /m "Confirm output and behavior: [P] PASS / [F] FAIL: "
if not "%ERRORLEVEL%"=="1" (
    echo FAIL %TEST%: operator rejected test.
    exit /b 1
)
set /a PASSED+=1 >nul
echo PASS %TEST%: %NAME%
exit /b 0
:match
set /a MATCHES+=1 >nul
set "FILE=%~1"
exit /b 0
