; SYNTHI PRO - Inno Setup 6 script. Build the project first (build.bat), then compile this file with ISCC.exe.
#define AppName "SYNTHI PRO"
#define AppVersion "1.1.0"
#define Artefacts "..\build\SynthiPro_artefacts\Release"

[Setup]
AppId={{6F2B6C1E-5A0B-4F7B-9C55-3B1B1F7A51D0}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Khafizov Dinar
DefaultDirName={autopf}\SYNTHI PRO
DefaultGroupName=SYNTHI PRO
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=SYNTHI-PRO-{#AppVersion}-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
DisableProgramGroupPage=yes
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\SYNTHI PRO.exe
SetupIconFile=..\assets\icons\synthipro.ico
LicenseFile=..\LICENSE.txt
AppContact=Dinar899@gmail.com
AppPublisherURL=https://github.com/xray14/synthi-pro
AppSupportURL=https://github.com/xray14/synthi-pro/issues
AppCopyright=Copyright (C) 2026 Khafizov Dinar

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "standalone"; Description: "Standalone application (SYNTHI PRO.exe)"; Types: full custom
Name: "vst3"; Description: "VST3 plug-in for DAWs"; Types: full custom
Name: "docs"; Description: "Documentation (English / Russian)"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; Components: standalone; Flags: unchecked

[Files]
Source: "{#Artefacts}\Standalone\SYNTHI PRO.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "{#Artefacts}\VST3\SYNTHI PRO.vst3\*"; DestDir: "{commoncf64}\VST3\SYNTHI PRO.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\docs\*.md"; DestDir: "{app}\docs"; Components: docs; Flags: ignoreversion
Source: "..\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\NOTICE.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\SYNTHI PRO"; Filename: "{app}\SYNTHI PRO.exe"; Components: standalone
Name: "{group}\User Guide (EN)"; Filename: "{app}\docs\USER_GUIDE_EN.md"; Components: docs
Name: "{group}\User Guide (RU)"; Filename: "{app}\docs\USER_GUIDE_RU.md"; Components: docs
Name: "{autodesktop}\SYNTHI PRO"; Filename: "{app}\SYNTHI PRO.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\SYNTHI PRO.exe"; Description: "{cm:LaunchProgram,SYNTHI PRO}"; Components: standalone; Flags: nowait postinstall skipifsilent
