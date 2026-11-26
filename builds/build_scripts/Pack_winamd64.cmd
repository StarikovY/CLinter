@echo off
setlocal EnableExtensions DisableDelayedExpansion
rem Run from x64 Native Tools Command Prompt. No build or source changes.
rem Arguments: English PDF, Russian PDF, CLinter license, project README.md.
for %%R in ("%~dp0..\..") do set "ROOT=%%~fR"
if "%~4"=="" goto usage
set "EN=%~f1"
set "RU=%~f2"
set "LICENSE=%~f3"
set "PROJECT_README=%~f4"
set "BIN=%ROOT%\builds\drops\win-amd64\bin\Release"
set "ELIZA=%ROOT%\packaging\examples\AI\Eliza.bas"
if not exist "%ELIZA%" set "ELIZA=%ROOT%\Eliza.bas"
if not exist "%ELIZA%" (
 echo ERROR: Eliza.bas not found in packaging\examples\AI or repository root.
 exit /b 1
)
set "SAMPLES=%ROOT%\tests\regression\auto"
set "OUT=%ROOT%\builds\drops\win-amd64\distribution"
set "NAME=CLinter-1.0-win-amd64"
set "STAGE=%OUT%\%NAME%"
set "ZIP=%OUT%\%NAME%.zip"
if not defined VCToolsRedistDir (
 echo ERROR: Run from Visual Studio x64 Native Tools Command Prompt.
 exit /b 1
)
set "CRT="
for /d %%D in ("%VCToolsRedistDir%\x64\Microsoft.VC*.CRT") do if exist "%%~fD\vcruntime140.dll" set "CRT=%%~fD\vcruntime140.dll"
if not defined CRT (
 echo ERROR: x64 redistributable vcruntime140.dll not found.
 exit /b 1
)
for %%F in ("%BIN%\CLinter.exe" "%BIN%\SDL2.dll" "%CRT%" "%EN%" "%RU%" "%LICENSE%" "%PROJECT_README%") do if not exist "%%~fF" (
 echo ERROR: Missing "%%~fF"
 exit /b 1
)
for %%F in (regression.bas utka.bas DuckHuntD1.bas DuckHunt720p.bas Nebula.bas spiral.bas StairwayCloudsDemo.bas) do if not exist "%SAMPLES%\%%F" (
 echo ERROR: Missing "%SAMPLES%\%%F"
 exit /b 1
)
if exist "%STAGE%" goto exists
if exist "%ZIP%" goto exists
if exist "%ZIP%.sha256" goto exists
mkdir "%STAGE%\docs" || goto failed
mkdir "%STAGE%\samples" || goto failed
copy /y "%BIN%\CLinter.exe" "%STAGE%\CLinter.exe" >nul || goto failed
copy /y "%BIN%\SDL2.dll" "%STAGE%\SDL2.dll" >nul || goto failed
copy /y "%CRT%" "%STAGE%\vcruntime140.dll" >nul || goto failed
copy /y "%EN%" "%STAGE%\docs\CLinter_BASIC_Manual_EN.pdf" >nul || goto failed
copy /y "%RU%" "%STAGE%\docs\CLinter_BASIC_Manual_RU.pdf" >nul || goto failed
copy /y "%LICENSE%" "%STAGE%\LICENSE.txt" >nul || goto failed
copy /y "%PROJECT_README%" "%STAGE%\README.md" >nul || goto failed
copy /y "%SAMPLES%\regression.bas" "%STAGE%\samples\RegTest.bas" >nul || goto failed
copy /y "%SAMPLES%\utka.bas" "%STAGE%\samples\DuckHuntText.bas" >nul || goto failed
copy /y "%ELIZA%" "%STAGE%\samples\Eliza.bas" >nul || goto failed
copy /y "%SAMPLES%\spiral.bas" "%STAGE%\samples\Spiral.bas" >nul || goto failed
for %%F in (DuckHuntD1.bas DuckHunt720p.bas Nebula.bas StairwayCloudsDemo.bas) do copy /y "%SAMPLES%\%%F" "%STAGE%\samples\%%F" >nul || goto failed
(
 echo CLinter V1.0 - Windows AMD64
 echo.
 echo Extract the entire ZIP before running CLinter.
 echo Open Command Prompt in the extracted directory.
 echo Interactive mode: CLinter.exe
 echo Run a sample: CLinter.exe samples\DuckHunt720p.bas
 echo See docs for the English and Russian BASIC manuals.
 echo Source: https://github.com/StarikovY/CLinter
 echo License: LICENSE.txt
 echo See README.md for project information.
 echo Samples include Nebula.bas, Spiral.bas, DuckHuntText.bas,
 echo DuckHuntD1.bas, DuckHunt720p.bas, StairwayCloudsDemo.bas, Eliza.bas and RegTest.bas.
) > "%STAGE%\README_FIRST.txt"
if errorlevel 1 goto failed
for %%F in (RegTest.bas DuckHuntText.bas DuckHuntD1.bas DuckHunt720p.bas Nebula.bas Spiral.bas StairwayCloudsDemo.bas Eliza.bas) do if not exist "%STAGE%\samples\%%F" goto failed
powershell -NoProfile -NonInteractive -Command "$ErrorActionPreference='Stop'; Compress-Archive -LiteralPath $env:STAGE -DestinationPath $env:ZIP; $h=(Get-FileHash -LiteralPath $env:ZIP -Algorithm SHA256).Hash.ToLowerInvariant(); Set-Content -LiteralPath ($env:ZIP+'.sha256') -Encoding ASCII -Value ($h+'  '+[IO.Path]::GetFileName($env:ZIP))" || goto failed
 echo SUCCESS: "%ZIP%"
 echo SHA256: "%ZIP%.sha256"
 exit /b 0
:exists
 echo ERROR: Distribution output already exists. Move or remove old output manually.
 exit /b 1
:failed
 echo ERROR: Packaging failed. Inspect partial output under "%OUT%" before retrying.
 exit /b 1
:usage
 echo Usage: %~nx0 "English.pdf" "Russian.pdf" "LICENSE.txt" "README.md"
 exit /b 1
