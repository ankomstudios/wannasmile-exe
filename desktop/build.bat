@echo off
rem Builds build\x64\Release\WannaSmile.exe (+ WebView2Loader.dll).
rem Run from any terminal (cmd or PowerShell):   desktop\build.bat
rem Needs: Visual Studio Build Tools with "Desktop development with C++", and internet
rem the first time (downloads the WebView2 SDK from nuget.org into packages\).
setlocal EnableExtensions
cd /d "%~dp0.." || exit /b 1

rem ---- 1. compiler environment (skipped if already in a Developer prompt) ----
where cl >nul 2>nul
if not errorlevel 1 goto have_cl
set "VSW=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSW%" goto no_vs
set "VSROOT="
for /f "usebackq delims=" %%i in (`"%VSW%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"
if not defined VSROOT goto no_vs
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto no_vs
:have_cl

rem ---- 2. WebView2 SDK ----
set "WV2=%CD%\packages\Microsoft.Web.WebView2"
if exist "%WV2%\build\native\include\WebView2.h" goto have_sdk
echo Downloading the WebView2 SDK from nuget.org ...
if not exist packages mkdir packages
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ProgressPreference='SilentlyContinue'; Invoke-WebRequest 'https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2' -OutFile 'packages\wv2.zip'; Expand-Archive -Force 'packages\wv2.zip' 'packages\Microsoft.Web.WebView2'"
if exist packages\wv2.zip del packages\wv2.zip
:have_sdk
if not exist "%WV2%\build\native\include\WebView2.h" goto no_sdk

rem ---- 3. app icon (desktop\WannaSmile.ico, from the R2 favicons folder) ----
if not exist build\x64\Release mkdir build\x64\Release
if exist desktop\WannaSmile.ico goto have_ico
echo Downloading the app icon ...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ProgressPreference='SilentlyContinue'; Invoke-WebRequest 'https://ws.ankomstudios.com/favicons/favicon.ico' -OutFile 'desktop\WannaSmile.ico'"
:have_ico
set "RES="
if not exist desktop\WannaSmile.ico echo No icon found, building without one.
if not exist desktop\WannaSmile.ico goto compile
rc /nologo /i desktop /fo build\x64\Release\app.res desktop\app.rc
if errorlevel 1 goto failed
set "RES=build\x64\Release\app.res"

rem ---- 4. compile + link ----
:compile
if exist build\x64\Release\WannaSmile.exe del /f /q build\x64\Release\WannaSmile.exe
if exist build\x64\Release\WannaSmile.exe goto locked
cl /nologo /std:c++17 /EHsc /MT /O2 /DUNICODE /D_UNICODE /I"%WV2%\build\native\include" /Fobuild\x64\Release\ desktop\main.cpp /link /SUBSYSTEM:WINDOWS /OUT:build\x64\Release\WannaSmile.exe /LIBPATH:"%WV2%\build\native\x64" %RES% WebView2Loader.dll.lib user32.lib gdi32.lib ole32.lib shell32.lib wininet.lib advapi32.lib
if errorlevel 1 goto failed
copy /y "%WV2%\build\native\x64\WebView2Loader.dll" build\x64\Release\ >nul
if errorlevel 1 goto failed
echo.
echo Built: build\x64\Release\WannaSmile.exe
exit /b 0

:no_vs
echo Could not find the C++ compiler. Install "Visual Studio Build Tools 2022" with the "Desktop development with C++" workload, then run this again.
exit /b 1
:no_sdk
echo Could not get the WebView2 SDK. Download the Microsoft.Web.WebView2 package from https://www.nuget.org/packages/Microsoft.Web.WebView2 ^(Download package^), rename the .nupkg to .zip, and extract it into packages\Microsoft.Web.WebView2 so that packages\Microsoft.Web.WebView2\build\native\include\WebView2.h exists. Then run this again.
exit /b 1
:locked
echo build\x64\Release\WannaSmile.exe cannot be replaced. Close WannaSmile if it is running ^(Task Manager^), pause antivirus scanning of this folder, or move the project out of Downloads to a short path like C:\ws and run again.
exit /b 1
:failed
echo Build failed. Copy the messages above and send them over.
exit /b 1
