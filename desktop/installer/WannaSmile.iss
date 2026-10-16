; WannaSmile for Windows 11 x64 - Inno Setup 6.3+ script.   (UNTESTED, see ..\README.md)
;
; Build:   ISCC.exe desktop\installer\WannaSmile.iss      (output: dist\WannaSmile-Setup-x64.exe)
; Paths below are relative to the REPO ROOT (the script itself lives in desktop\installer).
;
; Before building, put these in place (none of them is checked into the repo):
;   build\x64\Release\WannaSmile.exe             the compiled app (desktop\build.bat)
;   build\x64\Release\WebView2Loader.dll         copied there by desktop\build.bat
;   redist\MicrosoftEdgeWebview2Setup.exe        Microsoft's Evergreen Bootstrapper, downloaded
;                                                from Microsoft's official WebView2 download page
;   desktop\WannaSmile.ico                        app icon; desktop\build.bat downloads it from ws.ankomstudios.com/favicons/favicon.ico
;
; The installer carries the app and, when needed, the small Microsoft bootstrapper.
; It does NOT carry the WebView2 SDK, compilers, source files or the website.

#define AppName      "WannaSmile"
#define AppVersion   "1.0.0"
#define AppPublisher "WannaSmile4Evr"
#define AppExe       "WannaSmile.exe"

[Setup]
; Replace with your own GUID (Inno IDE: Tools > Generate GUID) and never change it afterwards.
AppId={{7D977B95-EB52-4D4B-94B3-DFE75EA95CF7}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
VersionInfoVersion={#AppVersion}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#AppExe}
OutputDir=..\..\dist
; Fixed file name (no version) so the download page can link ...releases/latest/download/WannaSmile-Setup-x64.exe
OutputBaseFilename=WannaSmile-Setup-x64
; Windows 11 (build 22000) and 64-bit only.
MinVersion=10.0.22000
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Per-user install by default (no admin prompt); the user can choose "all users" in the dialog.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\WannaSmile.ico

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; Flags: unchecked

[Files]
Source: "..\..\build\x64\Release\{#AppExe}";        DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\build\x64\Release\WebView2Loader.dll"; DestDir: "{app}"; Flags: ignoreversion
; Only unpacked (and only run) when the WebView2 Runtime is missing.
Source: "..\redist\MicrosoftEdgeWebview2Setup.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Check: not WebView2RuntimeInstalled

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
; Official Evergreen Bootstrapper; downloads the Runtime from Microsoft (needs internet).
Filename: "{tmp}\MicrosoftEdgeWebview2Setup.exe"; Parameters: "/silent /install"; \
  StatusMsg: "Installing Microsoft Edge WebView2 Runtime..."; Flags: waituntilterminated; \
  Check: not WebView2RuntimeInstalled
Filename: "{app}\{#AppExe}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent

[Code]
// Microsoft's documented detection: the "pv" value under the WebView2 Runtime's EdgeUpdate
// client key, in HKLM (32-bit view) or HKCU (per-user installs).
function WebView2VersionOk(RootKey: Integer; const SubKey: String): Boolean;
var
  Version: String;
begin
  Result := RegQueryStringValue(RootKey, SubKey, 'pv', Version) and
            (Version <> '') and (Version <> '0.0.0.0');
end;

function WebView2RuntimeInstalled: Boolean;
const
  Client = 'Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}';
begin
  Result := WebView2VersionOk(HKLM32, 'SOFTWARE\' + Client) or
            WebView2VersionOk(HKCU, 'Software\' + Client);
end;
