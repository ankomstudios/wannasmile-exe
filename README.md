# WannaSmile for Windows

The Windows desktop app for [WannaSmile](https://wannasmile4evr.github.io/): a small native window
(C++ and Microsoft Edge WebView2) that opens the live site. Nothing of the site is bundled, so site
updates reach the app without a new download. The app needs an internet connection.

This repo holds only the app and its installer. The website lives in
[wannasmile4evr/wannasmile4evr.github.io](https://github.com/wannasmile4evr/wannasmile4evr.github.io).

## Download

Get the latest installer from the [Releases](../../releases/latest) page, or from the site's
[download page](https://wannasmile4evr.github.io/system/pages/download/index.html).
Windows 11, 64-bit. Setup installs the Microsoft Edge WebView2 Runtime if your PC is missing it.

The installer is not code-signed yet, so Windows SmartScreen may warn about it.

## Build the app

Needs Visual Studio Build Tools 2022 with the "Desktop development with C++" workload and an internet
connection the first time. From the repo root, in cmd or PowerShell:

```bat
desktop\build.bat
```

It finds the compiler, downloads the WebView2 SDK into `packages\` and the app icon into
`desktop\WannaSmile.ico`, and writes `build\x64\Release\WannaSmile.exe` plus `WebView2Loader.dll`.
Run the exe from that folder.

## Build the installer

1. Install [Inno Setup 6](https://jrsoftware.org/isinfo.php).
2. Download Microsoft's WebView2 Evergreen Bootstrapper from Microsoft's official WebView2 page and save it as
   `desktop\redist\MicrosoftEdgeWebview2Setup.exe` (it is git-ignored).
3. Set `AppId` in `desktop\installer\WannaSmile.iss` to your own GUID, once, and never change it.
4. Run:

```powershell
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" desktop\installer\WannaSmile.iss
```

The result is `dist\WannaSmile-Setup-x64.exe`.

## Publish a release

1. Create a GitHub Release with a tag such as `v1.0.0`.
2. Attach `WannaSmile-Setup-x64.exe` under exactly that name.
3. Keep the release public and not a draft or pre-release.

The website's download page looks up the latest release of this repo by itself, so the site needs no edit.
Do not commit the exe, DLL or installer; they go on the Release only.

## Layout

| Path | What it is |
| --- | --- |
| `desktop/main.cpp` | The window and WebView2 setup |
| `desktop/native/WannaSmileWebView.h` | Navigation policy: same-window links, external links to the default browser, pop-up blocking, offline page |
| `desktop/app.rc` | Embeds the app icon in the exe |
| `desktop/build.bat` | One-command build |
| `desktop/installer/WannaSmile.iss` | Inno Setup installer script |
| `desktop/redist/` | Where the WebView2 bootstrapper goes |
| `desktop/README.md` | Technical notes: how links are routed, status and test checklist |

The app and the website recognize each other through a flag the app sets (`window.WANNA_SMILE_APP`) and a
`WannaSmileApp/<version>` user-agent suffix; details are in `desktop/README.md`.
