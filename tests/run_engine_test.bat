@echo off
rem Build and run the stand-alone DSP engine tests (JUCE is not required).
setlocal
cd /d "%~dp0.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto novs
for /f "usebackq delims=" %%i in (`call "%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR goto novs
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist build-test mkdir build-test
cl /nologo /O2 /EHsc /std:c++17 /utf-8 /W3 /I Source /Fo:build-test\ /Fe:build-test\engine_test.exe tests\engine_test.cpp
if errorlevel 1 exit /b 1
build-test\engine_test.exe
exit /b %errorlevel%
:novs
echo Visual Studio C++ build tools were not found.
exit /b 1
