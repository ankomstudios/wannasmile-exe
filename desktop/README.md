# WannaSmile for Windows (WebView2)

The desktop app is a native C++ window that loads the live WannaSmile site with
Microsoft Edge WebView2. It needs an internet connection: the pages, data,
assets and the Cloudflare/API requests all come from the live site. Nothing is
bundled offline and no secrets live in the executable.

## Status

| Part | State |
| --- | --- |
| `system/js/desktop.js` (website repo: flag, window.open, link routing) | Written and unit-tested in jsdom (browser mode and app mode) |
| Download page `system/pages/download/index.html` (website repo) | Written; button logic tested in jsdom. Layout not checked in a real browser yet |
| `desktop/native/WannaSmileWebView.h` | Compiles and runs on the repo owner's machine (reported by them). Each behavior in the checklist below is still to be verified one by one |
| `desktop/installer/WannaSmile.iss` | Written, **never built** |
| `desktop/main.cpp` | Merged from your `main.cpp` (virtual host removed, loads github.io, `Configure()` wired, user data in `%LOCALAPPDATA%\WannaSmile\WebView2`). Builds and runs on the repo owner's machine (reported by them) |
| Icon | `build.bat` downloads `desktop/WannaSmile.ico` from the R2 favicons folder and embeds it via `app.rc` |
| Installer published | Not until a Release exists. The download page then finds it by itself and shows "Coming soon" until then |

## Why links opened new tabs

* Hardcoded `target="_blank"`: header and footer links in `index.html`,
  `debug.html`, `system/pages/main/index.html` (Home, Discovery, Settings, Store,
  News, Credits, DMCA, Discord, Suggestions, source) and several pages.
* `window.open(...)`: `system/js/main.js` (library asset launches, the
  `about:blank` + `document.write` "incognito" modes and `blob:` pages),
  `daily.js`, `quotes.js`, `ui.js` (update pages, the DM button), `msgcore.js`
  (named `wannachat` window), `devtools.js`, `dmca/index.html`.
* `system/js/ui.js` also sets `target="_blank"` on the footer commit link.

In a browser those are intentional, so they were left alone.

## How the app is told apart from the browser

1. Native (`Configure()`): `window.WANNA_SMILE_APP = true` before any page script
   (top-level document only) and `WannaSmileApp/<version>` in the user agent.
2. Frontend: `system/js/desktop.js`, the first script in `<head>` of every page,
   checks the flag or the user agent. In a browser it does nothing at all.

## What happens to each kind of "new tab" in the app

| Request | Result |
| --- | --- |
| Link or `window.open` to a page of this site (including `target="_blank"`, named targets, links added later) | Same window |
| `window.open()` / `about:blank` / `blob:` (library asset launches) | Full-window viewer over the current page with a Back button; the page underneath keeps its state |
| Link to another website (Padlet, GitHub, Discord, ...) | Default browser (native `ShellExecute`, http/https only) |
| Pop-ups nobody clicked | Ignored |
| `javascript:` and unknown schemes | Dropped |

`desktop.js` handles what it can see; `NewWindowRequested` / `NavigationStarting`
in `WannaSmileWebView.h` catch the rest (detached `a.click()`, plain external links,
anything inside iframes), so WebView2 never opens a second window.

## Hooking it into main.cpp

```cpp
#include "desktop/native/WannaSmileWebView.h"

// before CreateCoreWebView2EnvironmentWithOptions:
if (!wannasmile::RuntimeInstalled()) { /* message box: run the installer again */ return 1; }

// in the controller-created callback, once you have the ICoreWebView2* (call it webview):
wannasmile::Configure(webview.Get());   // BEFORE Navigate
webview->Navigate(L"https://wannasmile4evr.github.io/");
```

Edit `HomeUrl()` / `InternalHosts()` in the header if the app should load a different host.

## Build

Easiest, from any terminal (cmd or PowerShell) at the repo root:

```bat
desktop\build.bat
```

It finds the compiler, downloads the WebView2 SDK into `packages\` on first run, and
writes `build\x64\Release\WannaSmile.exe` plus `WebView2Loader.dll`. The manual
commands (Developer Command Prompt for VS, x64, **cmd syntax, not PowerShell**):

```bat
set WV2=C:\path\to\packages\Microsoft.Web.WebView2.<version>
mkdir build\x64\Release
cl /nologo /std:c++17 /EHsc /O2 /DUNICODE /D_UNICODE /I"%WV2%\build\native\include" desktop\main.cpp ^
  /link /SUBSYSTEM:WINDOWS /OUT:build\x64\Release\WannaSmile.exe /LIBPATH:"%WV2%\build\native\x64" ^
  WebView2Loader.dll.lib user32.lib gdi32.lib ole32.lib shell32.lib wininet.lib advapi32.lib
copy "%WV2%\build\native\x64\WebView2Loader.dll" build\x64\Release\
```

Adjust file names and libraries to match what your current build already uses.

Run it from the repo root. Your old `main.exe` / `main.obj` and the generated
`main.exe.WebView2` folder are not needed (that folder is just WebView2's cache and
profile; the app now keeps its profile in `%LOCALAPPDATA%\WannaSmile\WebView2`).

## Run

```bat
build\x64\Release\WannaSmile.exe
```

## Package

1. Copy Microsoft's Evergreen WebView2 Bootstrapper to
   `desktop\redist\MicrosoftEdgeWebview2Setup.exe` (from Microsoft's official WebView2 page).
2. Put a real GUID in `AppId` in `desktop\installer\WannaSmile.iss`; add an icon if you have one.
3. `"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" desktop\installer\WannaSmile.iss`
   -> `dist\WannaSmile-Setup-x64.exe`.

The installer installs to Program Files (or the user's profile), adds a Start Menu
entry, and runs the bootstrapper only if the WebView2 Runtime is missing
(detected through Microsoft's documented `pv` registry value). The Runtime is a
separate Microsoft component, not part of the app.

## Publish

1. Create a GitHub Release in the repo named in `DESKTOP_APP.repo`
   (`system/pages/download/index.html`; currently the ASSUMED `ankomstudios/wannasmile-exe`,
   change it if the real repo differs). Tag it like `v1.0.0`.
2. Attach `dist\WannaSmile-Setup-x64.exe` with exactly that file name (`DESKTOP_APP.asset`).
3. Not a draft, not a pre-release. The download page then finds it by itself: link,
   version, size and (from GitHub's asset digest, if present) SHA-256. No edit to the
   website is needed per release. Direct link that always points at the newest one:
   `https://github.com/<repo>/releases/latest/download/WannaSmile-Setup-x64.exe`.

Do not commit the exe, DLL or installer to the Pages repo.

## Test checklist (nothing below has been run)

* Browser: `window.WANNA_SMILE_APP` is `undefined`, header links still open new tabs.
* App: header/footer links change the page in the same window; no second window ever appears.
* App: a library asset opens in the viewer and Back returns to the same library page.
* App: Padlet / Discord / GitHub open in the default browser.
* App: with Wi-Fi off, the first launch shows the offline message; Retry works once online.
* Clean Windows 11 VM without the WebView2 Runtime: the installer installs it, then the app starts.
