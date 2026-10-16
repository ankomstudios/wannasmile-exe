// WannaSmile for Windows: native WebView2 host window.
//
// Loads the live site (wannasmile::HomeUrl(), see native/WannaSmileWebView.h).
// Navigation policy (same-window links, external links to the default browser,
// pop-up blocking, offline page, WANNA_SMILE_APP flag) lives in the header.
//
// Changes from the original main.cpp (merge):
//   * the wannasmile.local virtual-host mapping and the inline
//     "window.WANNA_SMILE_APP = true" script are gone; Configure() sets the flag
//     (top-level document only) and the site is loaded from HomeUrl()
//   * WebView2 user data (localStorage, cache) now lives in
//     %LOCALAPPDATA%\WannaSmile\WebView2 instead of next to the exe, which is
//     not writable under C:\Program Files
//   * COM is initialised (STA) before WebView2 is created
//   * WebView2 Runtime presence is checked up front

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <wrl.h>
#include <string>
#include <WebView2.h>

#include "native/WannaSmileWebView.h"

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

static HWND g_window = nullptr;
static ComPtr<ICoreWebView2Controller> g_controller;
static ComPtr<ICoreWebView2> g_webview;

// %LOCALAPPDATA%\WannaSmile\WebView2 (created if missing). Empty string = let
// WebView2 pick its default.
static std::wstring GetUserDataFolder()
{
    wchar_t base[MAX_PATH]{};
    const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return L"";

    std::wstring dir = std::wstring(base) + L"\\WannaSmile";
    CreateDirectoryW(dir.c_str(), nullptr);          // ok if it already exists
    dir += L"\\WebView2";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

static void ResizeWebView()
{
    if (!g_controller || !g_window)
        return;

    RECT bounds{};
    GetClientRect(g_window, &bounds);
    g_controller->put_Bounds(bounds);
}

static LRESULT CALLBACK WindowProc(
    HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        ResizeWebView();
        return 0;

    case WM_DESTROY:
        g_webview.Reset();
        g_controller.Reset();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

int WINAPI wWinMain(
    HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    if (!wannasmile::RuntimeInstalled())
    {
        MessageBoxW(nullptr,
            L"WannaSmile needs the Microsoft Edge WebView2 Runtime. "
            L"Run the WannaSmile installer again to install it.",
            L"WannaSmile", MB_ICONERROR);
        return 1;
    }

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
        return 4;

    const wchar_t CLASS_NAME[] = L"WannaSmileWindow";

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassW(&wc))
        return 1;

    g_window = CreateWindowExW(
        0,
        CLASS_NAME,
        L"WannaSmile",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 800,
        nullptr, nullptr, instance, nullptr);

    if (!g_window)
        return 2;

    // Icon embedded by desktop\app.rc (resource id 1). Harmless if it is missing.
    SendMessageW(g_window, WM_SETICON, ICON_BIG,
        reinterpret_cast<LPARAM>(LoadImageW(instance, MAKEINTRESOURCEW(1), IMAGE_ICON,
            GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR)));
    SendMessageW(g_window, WM_SETICON, ICON_SMALL,
        reinterpret_cast<LPARAM>(LoadImageW(instance, MAKEINTRESOURCEW(1), IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR)));

    ShowWindow(g_window, showCommand);

    const std::wstring userData = GetUserDataFolder();

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        userData.empty() ? nullptr : userData.c_str(),
        nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT
            {
                if (FAILED(result) || !environment)
                {
                    MessageBoxW(g_window,
                        L"Could not initialize WebView2.",
                        L"WannaSmile", MB_ICONERROR);
                    return result;
                }

                return environment->CreateCoreWebView2Controller(
                    g_window,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [](HRESULT controllerResult,
                           ICoreWebView2Controller* controller) -> HRESULT
                        {
                            if (FAILED(controllerResult) || !controller)
                            {
                                MessageBoxW(g_window,
                                    L"Could not create the browser window.",
                                    L"WannaSmile", MB_ICONERROR);
                                return controllerResult;
                            }

                            g_controller = controller;

                            HRESULT hr = g_controller->get_CoreWebView2(
                                &g_webview);

                            if (FAILED(hr) || !g_webview)
                                return hr;

                            ResizeWebView();

                            // Navigation policy, user agent, app flag: BEFORE Navigate.
                            wannasmile::Configure(g_webview.Get());

                            return g_webview->Navigate(
                                wannasmile::HomeUrl().c_str());
                        }).Get());
            }).Get());

    if (FAILED(hr))
    {
        MessageBoxW(g_window,
            L"WebView2 initialization failed to start.",
            L"WannaSmile", MB_ICONERROR);
        return 3;
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    CoUninitialize();
    return static_cast<int>(message.wParam);
}
