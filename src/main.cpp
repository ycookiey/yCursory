#include <windows.h>
#include "ime_monitor.h"
#include "cursor_manager.h"
#include "caret_indicator.h"
#include "tray_icon.h"
#include "startup_manager.h"
#include "logger.h"
#include "resource.h"

// Global variables
static ImeMonitor* g_imeMonitor = nullptr;
static CursorManager* g_cursorManager = nullptr;
static CaretIndicator* g_caretIndicator = nullptr;
static TrayIcon* g_trayIcon = nullptr;
static HWND g_hwnd = NULL;

// Timer IDs
#define TIMER_CHECK_IME 1
#define TIMER_UPDATE_DISPLAY 2
#define CHECK_INTERVAL_MS 16       // 位置検出は60fps（16ms）
#define DISPLAY_UPDATE_MS 16       // 描画更新は60fps（16ms）

// Registry Key
const wchar_t* REG_KEY_PATH = L"Software\\yCursory";
const wchar_t* REG_VALUE_STYLE = L"GlowStyle";
const wchar_t* REG_VALUE_CARET_ENABLED = L"CaretIndicatorEnabled";

// Settings
void SaveSettings() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD style = (DWORD)g_cursorManager->GetGlowStyle();
        RegSetValueExW(hKey, REG_VALUE_STYLE, 0, REG_DWORD, (const BYTE*)&style, sizeof(style));

        DWORD caretEnabled = g_caretIndicator->IsEnabled() ? 1 : 0;
        RegSetValueExW(hKey, REG_VALUE_CARET_ENABLED, 0, REG_DWORD, (const BYTE*)&caretEnabled, sizeof(caretEnabled));

        RegCloseKey(hKey);
    }
}

void LoadSettings() {
    HKEY hKey;
    DWORD caretEnabled = 1;  // デフォルトは有効

    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD style = 0;
        DWORD size = sizeof(style);
        if (RegQueryValueExW(hKey, REG_VALUE_STYLE, NULL, NULL, (LPBYTE)&style, &size) == ERROR_SUCCESS) {
            g_cursorManager->SetGlowStyle((CursorManager::GlowStyle)style);
        }

        size = sizeof(caretEnabled);
        RegQueryValueExW(hKey, REG_VALUE_CARET_ENABLED, NULL, NULL, (LPBYTE)&caretEnabled, &size);

        RegCloseKey(hKey);
    }

    // レジストリの値に関わらず設定を適用（初回起動時はデフォルト値）
    if (g_caretIndicator) {
        g_caretIndicator->SetEnabled(caretEnabled != 0);
    }
}

// ログ用: 窓のプロセスの実行ファイル名 (取れなければ "?")
static void ProcessNameOf(HWND hwnd, wchar_t* out, DWORD outLen, DWORD* pidOut) {
    wcscpy_s(out, outLen, L"?");
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    *pidOut = pid;
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return;
    wchar_t path[MAX_PATH];
    DWORD len = MAX_PATH;
    if (QueryFullProcessImageNameW(process, 0, path, &len)) {
        const wchar_t* name = wcsrchr(path, L'\\');
        wcscpy_s(out, outLen, name ? name + 1 : path);
    }
    CloseHandle(process);
}

// ログ: 前面の窓・IME 状態・キャレットの取得方法が変わったときだけ書く
static void LogChanges(ImeState state) {
    static HWND lastForeground = NULL;
    static int lastState = -1;
    static const wchar_t* lastMethod = nullptr;

    HWND fg = GetForegroundWindow();
    if (fg != lastForeground) {
        lastForeground = fg;
        if (Logger::Enabled(LogLevel::Info)) {
            wchar_t name[MAX_PATH];
            DWORD pid = 0;
            ProcessNameOf(fg, name, MAX_PATH, &pid);
            LOG_INFO(L"foreground: %s (pid=%u)", name, pid);
        }
    }
    if ((int)state != lastState) {
        lastState = (int)state;
        LOG_DEBUG(L"ime: %s", state == ImeState::Japanese ? L"japanese" : L"english");
    }
    if (g_caretIndicator && g_caretIndicator->IsEnabled()) {
        const wchar_t* method = g_caretIndicator->GetLastMethodName();
        if (method != lastMethod) {
            lastMethod = method;
            LOG_INFO(L"caret: %s", method);
        }
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // Start timers
        SetTimer(hwnd, TIMER_CHECK_IME, CHECK_INTERVAL_MS, NULL);      // 位置検出（100ms）
        SetTimer(hwnd, TIMER_UPDATE_DISPLAY, DISPLAY_UPDATE_MS, NULL); // 描画更新（16ms）
        return 0;
        
    case WM_TIMER:
        if (wParam == TIMER_CHECK_IME && g_imeMonitor && g_cursorManager) {
            ImeState state = g_imeMonitor->GetCurrentState();
            g_cursorManager->SetCursorForState(state);

            // Update caret indicator position (100ms interval)
            if (g_caretIndicator) {
                g_caretIndicator->UpdatePosition(state);
            }
            LogChanges(state);

            // Update tooltip
            if (g_trayIcon) {
                g_trayIcon->UpdateTooltip(
                    state == ImeState::Japanese ? L"IME: Japanese" : L"IME: English"
                );
            }
        }
        else if (wParam == TIMER_UPDATE_DISPLAY && g_caretIndicator) {
            // Update caret indicator display (16ms interval for smooth interpolation)
            g_caretIndicator->UpdateDisplay();
        }
        return 0;
        
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            // Show context menu
            POINT pt;
            GetCursorPos(&pt);
            
            HMENU hMenu = CreatePopupMenu();
            
            // スタートアップ登録メニュー（チェックマーク付き）
            UINT startupFlags = MF_STRING;
            if (StartupManager::IsRegistered()) {
                startupFlags |= MF_CHECKED;
            }
            AppendMenuW(hMenu, startupFlags, IDM_STARTUP, L"スタートアップに登録");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            
            // グロースタイルメニュー
            HMENU hSubMenu = CreatePopupMenu();
            CursorManager::GlowStyle currentStyle = g_cursorManager->GetGlowStyle();
            
            AppendMenuW(hSubMenu, MF_STRING | (currentStyle == CursorManager::StyleLight ? MF_CHECKED : 0), 
                IDM_STYLE_LIGHT, L"Light");
            AppendMenuW(hSubMenu, MF_STRING | (currentStyle == CursorManager::StyleHeavy ? MF_CHECKED : 0), 
                IDM_STYLE_HEAVY, L"Heavy");
                
            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, L"Glow Style");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

            // キャレットインジケーター
            UINT caretFlags = MF_STRING;
            if (g_caretIndicator && g_caretIndicator->IsEnabled()) {
                caretFlags |= MF_CHECKED;
            }
            AppendMenuW(hMenu, caretFlags, IDM_CARET_INDICATOR, L"Caret Indicator");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

            AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"Exit");
            
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                          pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        return 0;
        
    case WM_COMMAND:
        if (LOWORD(wParam) == IDM_EXIT) {
            PostQuitMessage(0);
        } else if (LOWORD(wParam) == IDM_STARTUP) {
            // スタートアップ登録をトグル
            if (StartupManager::IsRegistered()) {
                StartupManager::Unregister();
            } else {
                StartupManager::Register();
            }
        } else if (LOWORD(wParam) == IDM_STYLE_LIGHT) {
            g_cursorManager->SetGlowStyle(CursorManager::StyleLight);
            SaveSettings();
        } else if (LOWORD(wParam) == IDM_STYLE_HEAVY) {
            g_cursorManager->SetGlowStyle(CursorManager::StyleHeavy);
            SaveSettings();
        } else if (LOWORD(wParam) == IDM_CARET_INDICATOR) {
            // キャレットインジケーターのトグル
            if (g_caretIndicator) {
                g_caretIndicator->SetEnabled(!g_caretIndicator->IsEnabled());
                SaveSettings();
            }
        }
        return 0;
        
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_CHECK_IME);
        KillTimer(hwnd, TIMER_UPDATE_DISPLAY);
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    Logger::Init(REG_KEY_PATH);
    LOG_INFO(L"start (pid=%u)", GetCurrentProcessId());

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ImeCursorIndicator";
    
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Window class registration failed", L"Error", MB_ICONERROR);
        return 1;
    }
    
    // Create hidden window
    g_hwnd = CreateWindowExW(
        0, L"ImeCursorIndicator", L"IME Cursor Indicator",
        0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL
    );
    
    if (g_hwnd == NULL) {
        MessageBoxW(NULL, L"Window creation failed", L"Error", MB_ICONERROR);
        return 1;
    }
    
    // Initialize components
    g_imeMonitor = new ImeMonitor();
    g_cursorManager = new CursorManager();
    g_caretIndicator = new CaretIndicator();
    g_trayIcon = new TrayIcon();

    if (!g_cursorManager->Initialize(hInstance)) {
        MessageBoxW(NULL,
            L"Failed to load cursor files.\n"
            L"Please place cursor_en.cur and cursor_jp.cur in the resources folder.",
            L"Error", MB_ICONERROR);
        delete g_trayIcon;
        delete g_caretIndicator;
        delete g_cursorManager;
        delete g_imeMonitor;
        return 1;
    }

    if (!g_caretIndicator->Initialize(hInstance)) {
        MessageBoxW(NULL, L"Failed to initialize caret indicator", L"Warning", MB_ICONWARNING);
        // 警告のみで続行
    }

    if (!g_trayIcon->Create(g_hwnd, hInstance, WM_TRAYICON)) {
        MessageBoxW(NULL, L"Failed to create tray icon", L"Error", MB_ICONERROR);
    }

    // Set initial state (force=true to ensure cursor is set)
    LoadSettings(); // 設定読み込み（キャレットインジケーターの設定も含む）
    ImeState initialState = g_imeMonitor->GetCurrentState();
    g_cursorManager->SetCursorForState(initialState, true);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    LOG_INFO(L"exit");

    // Cleanup
    delete g_trayIcon;
    delete g_caretIndicator;
    delete g_cursorManager;
    delete g_imeMonitor;

    return (int)msg.wParam;
}
