@echo off
rem SYNTHI PRO - full build: engine tests, Standalone EXE, VST3, and (if Inno Setup is installed) the installer.
rem Usage: build.bat [path-to-JUCE]     (default: .\JUCE)
setlocal
cd /d "%~dp0"
set "JUCE_DIR=%~1"
if "%JUCE_DIR%"=="" set "JUCE_DIR=%~dp0JUCE"
if not exist "%JUCE_DIR%\CMakeLists.txt" goto nojuce

if not exist build-test mkdir build-test
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR="%JUCE_DIR%"
if errorlevel 1 exit /b 1
cmake --build build --config Release --target engine_test
if errorlevel 1 exit /b 1
build\Release\engine_test.exe
if errorlevel 1 goto testfail
cmake --build build --config Release --target SynthiPro_Standalone SynthiPro_VST3
if errorlevel 1 exit /b 1

echo.
echo Built:
echo   build\SynthiPro_artefacts\Release\Standalone\SYNTHI PRO.exe
echo   build\SynthiPro_artefacts\Release\VST3\SYNTHI PRO.vst3

set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if exist "%ISCC%" (
    "%ISCC%" installer\SynthiPro.iss
) else (
    echo Inno Setup 6 not found - installer skipped. See docs\BUILD.md.
)
exit /b 0

:nojuce
echo JUCE was not found in "%JUCE_DIR%".
echo Put the JUCE source tree into "%~dp0JUCE" or run: build.bat C:\path\to\JUCE
exit /b 1
:testfail
echo Engine tests failed - plugin build aborted.
exit /b 1
