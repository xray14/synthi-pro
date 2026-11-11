@echo off
rem SYNTHI PRO - sign the EXE and VST3, rebuild the installer from the signed binaries, sign the installer.
rem Usage: sign.bat <certificate SHA-1 thumbprint>      (or set SIGN_THUMBPRINT)
rem The certificate must be in the Windows certificate store (Current User \ Personal) with its private key.
rem With a certificate bought from a certificate authority the same script produces trusted signatures.
setlocal
cd /d "%~dp0"
set "THUMB=%~1"
if "%THUMB%"=="" set "THUMB=%SIGN_THUMBPRINT%"
if "%THUMB%"=="" goto usage

set "SIGNTOOL="
for /f "delims=" %%i in ('dir /b /s "%ProgramFiles(x86)%\Windows Kits\10\bin\signtool.exe" 2^>nul ^| findstr /i "\\x64\\"') do set "SIGNTOOL=%%i"
if not defined SIGNTOOL goto notool
set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"

set "ART=build\SynthiPro_artefacts\Release"
set "OPTS=/fd SHA256 /sha1 %THUMB% /tr http://timestamp.digicert.com /td SHA256 /d "SYNTHI PRO""

"%SIGNTOOL%" sign %OPTS% "%ART%\Standalone\SYNTHI PRO.exe" "%ART%\VST3\SYNTHI PRO.vst3\Contents\x86_64-win\SYNTHI PRO.vst3"
if errorlevel 1 exit /b 1

if not exist dist mkdir dist
copy /y "%ART%\Standalone\SYNTHI PRO.exe" dist\ >nul
if exist "dist\SYNTHI PRO.vst3" rmdir /s /q "dist\SYNTHI PRO.vst3"
xcopy /e /i /q /y "%ART%\VST3\SYNTHI PRO.vst3" "dist\SYNTHI PRO.vst3" >nul

copy /y LICENSE.txt dist\ >nul
copy /y NOTICE.txt dist\ >nul
if not exist "%ISCC%" goto noinno
"%ISCC%" /Q installer\SynthiPro.iss
if errorlevel 1 exit /b 1
"%SIGNTOOL%" sign %OPTS% "dist\SYNTHI-PRO-1.0.1-Setup.exe"
if errorlevel 1 exit /b 1
echo Signed: EXE, VST3 and installer are in the dist folder.
exit /b 0

:usage
echo Usage: sign.bat ^<certificate SHA-1 thumbprint^>
exit /b 1
:notool
echo signtool.exe was not found (install the Windows SDK).
exit /b 1
:noinno
echo Inno Setup 6 was not found - binaries are signed, the installer was not rebuilt.
exit /b 1
