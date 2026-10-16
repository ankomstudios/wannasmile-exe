// WannaSmile desktop: WebView2 navigation policy.   (UNTESTED, see ../README.md)
//
// Header-only. Include it from main.cpp and call
//
//     wannasmile::Configure(webview);      // ICoreWebView2*
//
// ONCE, right after the WebView2 is created and BEFORE you call Navigate().
// That is all main.cpp has to change. Link with wininet.lib.
//
// What it does
//   * Sets window.WANNA_SMILE_APP = true before any page script runs (top-level
//     document only) and adds "WannaSmileApp/<version>" to the user agent.
//     system/js/desktop.js reads either one.
//   * NewWindowRequested (target="_blank", window.open(), middle-click, ...):
//     always Handled, so WebView2 never creates a second window.
//       - this site            -> navigates the existing window
//       - other http(s) site   -> opens the default browser
//       - anything else        -> dropped
//     Both only happen for user-initiated requests, so pop-ups an embedded game
//     opens on its own are ignored.
//   * NavigationStarting (top-level only): a navigation to another website opens
//     the default browser instead and is cancelled here, so the app window stays
//     on WannaSmile. Requests the page makes (fetch, XHR, images, scripts, the
//     Cloudflare Workers / API calls) are NOT navigations and are not affected.
//   * NavigationCompleted: if a page fails to load (offline, DNS, server down)
//     an offline message with a Retry button is shown instead of a blank window.
//   * Turns the JS->native channel off (no web messages, no host objects).
//
// To allow another host inside the app (a login page, a second domain of the
// site), add it to InternalHosts().
#pragma once

#include <windows.h>
#include <shellapi.h>
#include <wininet.h>
#include <wrl.h>

#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>

#include "WebView2.h"

namespace wannasmile {

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

// The page Retry goes back to. Use the same URL main.cpp navigates to.
inline const std::wstring& HomeUrl() {
    static const std::wstring u = L"https://wannasmile4evr.github.io/";
    return u;
}

// Hosts that count as "the app". Everything else is an external website.
inline const std::vector<std::wstring>& InternalHosts() {
    static const std::vector<std::wstring> hosts = { L"wannasmile4evr.github.io" };
    return hosts;
}

inline const wchar_t* kAppVersion = L"1.0.0";   // keep in step with the installer

// ── URL helpers ───────────────────────────────────────────────────────────
inline std::wstring Lower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return (wchar_t)towlower(c); });
    return s;
}

inline bool StartsWith(const std::wstring& s, const wchar_t* prefix) {
    return Lower(s).rfind(prefix, 0) == 0;
}

struct Url { std::wstring scheme, host; bool ok = false; };

inline Url Parse(const std::wstring& url) {
    Url out;
    wchar_t scheme[16] = {}, host[256] = {};
    URL_COMPONENTSW uc = {};
    uc.dwStructSize = sizeof(uc);
    uc.lpszScheme = scheme;   uc.dwSchemeLength = _countof(scheme);
    uc.lpszHostName = host;   uc.dwHostNameLength = _countof(host);
    if (url.size() < 2048 && InternetCrackUrlW(url.c_str(), (DWORD)url.size(), 0, &uc)) {
        out.scheme = Lower(scheme);
        out.host = Lower(host);
        out.ok = true;
    }
    return out;
}

inline bool IsWeb(const Url& u) { return u.ok && (u.scheme == L"http" || u.scheme == L"https"); }

inline bool IsInternal(const std::wstring& url) {
    const Url u = Parse(url);
    if (!IsWeb(u)) return false;
    for (const auto& h : InternalHosts()) if (u.host == h) return true;
    return false;
}

// http(s) only, never file:, javascript:, ms-*: or custom protocols.
inline void OpenInBrowser(const std::wstring& url) {
    if (!IsWeb(Parse(url))) return;
    ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

inline std::wstring TakeString(LPWSTR raw) {
    std::wstring s = raw ? raw : L"";
    if (raw) CoTaskMemFree(raw);
    return s;
}

inline std::wstring WithoutFragment(std::wstring s) {
    const auto i = s.find(L'#');
    return i == std::wstring::npos ? s : s.substr(0, i);
}

// ── Offline page ──────────────────────────────────────────────────────────
inline void ShowOfflinePage(ICoreWebView2* wv) {
    std::wstring html =
        L"<!doctype html><meta charset=utf-8><title>WannaSmile</title>"
        L"<style>html,body{height:100%;margin:0;background:#0d0f14;color:#e9ebf2;"
        L"font:16px 'Segoe UI',system-ui,sans-serif}"
        L"body{display:grid;place-items:center;text-align:center}"
        L".c{max-width:420px;padding:24px}h1{font-size:1.5rem;margin:0 0 10px}"
        L"p{color:#b4b9c8;line-height:1.6;margin:0 0 20px}"
        L"a{display:inline-block;padding:12px 28px;border-radius:999px;background:#ff4444;"
        L"color:#fff;text-decoration:none;font-weight:700}</style>"
        L"<div class=c><h1>Can't reach WannaSmile</h1>"
        L"<p>WannaSmile needs an internet connection to load the site and its data. "
        L"Check your connection, then try again.</p>"
        L"<a href=\"" + HomeUrl() + L"\">Retry</a></div>";
    wv->NavigateToString(html.c_str());
}

// ── Setup ─────────────────────────────────────────────────────────────────
inline void Configure(ICoreWebView2* wv) {
    // Settings: no JS->native channel at all.
    ComPtr<ICoreWebView2Settings> settings;
    if (SUCCEEDED(wv->get_Settings(&settings)) && settings) {
        settings->put_IsWebMessageEnabled(FALSE);
        settings->put_AreHostObjectsAllowed(FALSE);
        settings->put_IsStatusBarEnabled(FALSE);
        // Release builds that should not expose DevTools: settings->put_AreDevToolsEnabled(FALSE);

        // "WannaSmileApp/<version>" in the user agent (second way desktop.js detects the app).
        ComPtr<ICoreWebView2Settings2> settings2;
        if (SUCCEEDED(settings.As(&settings2)) && settings2) {
            LPWSTR raw = nullptr;
            if (SUCCEEDED(settings2->get_UserAgent(&raw))) {
                std::wstring ua = TakeString(raw);
                ua += L" WannaSmileApp/";
                ua += kAppVersion;
                settings2->put_UserAgent(ua.c_str());
            }
        }
    }

    // window.WANNA_SMILE_APP = true, before any page script, top-level document
    // only (not in the Discord / Padlet / game iframes), read-only.
    wv->AddScriptToExecuteOnDocumentCreated(
        L"(function(){try{if(window.top===window){"
        L"Object.defineProperty(window,'WANNA_SMILE_APP',{value:true,enumerable:true});}}catch(e){}})();",
        nullptr);

    EventRegistrationToken token;

    // target="_blank", window.open(), dynamically created links, middle-click.
    wv->add_NewWindowRequested(
        Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                args->put_Handled(TRUE);   // never let WebView2 open another window
                const std::wstring url = TakeString([&] { LPWSTR r = nullptr; args->get_Uri(&r); return r; }());
                BOOL userInitiated = FALSE;
                args->get_IsUserInitiated(&userInitiated);
                if (!userInitiated) return S_OK;               // ignore pop-ups nobody asked for

                if (IsInternal(url)) {
                    LPWSTR cur = nullptr;
                    sender->get_Source(&cur);
                    if (WithoutFragment(TakeString(cur)) != WithoutFragment(url)) sender->Navigate(url.c_str());
                } else {
                    OpenInBrowser(url);                        // http(s) only; other schemes are dropped
                }
                return S_OK;
            }).Get(),
        &token);

    // A plain link / redirect to another website from the top-level page.
    wv->add_NavigationStarting(
        Callback<ICoreWebView2NavigationStartingEventHandler>(
            [](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                const std::wstring url = TakeString([&] { LPWSTR r = nullptr; args->get_Uri(&r); return r; }());
                if (IsInternal(url) || StartsWith(url, L"about:") || StartsWith(url, L"data:") || StartsWith(url, L"blob:"))
                    return S_OK;                               // the site, the offline page, in-page blobs
                args->put_Cancel(TRUE);                        // stay on WannaSmile
                BOOL userInitiated = FALSE;
                args->get_IsUserInitiated(&userInitiated);
                if (userInitiated) OpenInBrowser(url);
                return S_OK;
            }).Get(),
        &token);

    // First load (or a later page) failed: show the offline message.
    wv->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                BOOL ok = TRUE;
                args->get_IsSuccess(&ok);
                if (ok) return S_OK;
                COREWEBVIEW2_WEB_ERROR_STATUS status = COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
                args->get_WebErrorStatus(&status);
                if (status == COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED) return S_OK;   // we cancelled it (external link)
                ShowOfflinePage(sender);
                return S_OK;
            }).Get(),
        &token);
}

// True if a WebView2 Runtime is installed. Call before creating the environment;
// if false, tell the user to run the installer again (it installs the Runtime).
inline bool RuntimeInstalled() {
    LPWSTR version = nullptr;
    const HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &version);
    const bool ok = SUCCEEDED(hr) && version != nullptr;
    if (version) CoTaskMemFree(version);
    return ok;
}

}  // namespace wannasmile
