#include <windows.h>
#include "ime_monitor.h"
#include "cursor_manager.h"
#include "tray_icon.h"
#include "startup_manager.h"
#include "resource.h"

// Global variables
static ImeMonitor* g_imeMonitor = nullptr;
static CursorManager* g_cursorManager = nullptr;
static TrayIcon* g_trayIcon = nullptr;
static HWND g_hwnd = NULL;

// Timer ID
#define TIMER_CHECK_IME 1
#define CHECK_INTERVAL_MS 100

// Registry Key
const wchar_t* REG_KEY_PATH = L"Software\\yCursory";
const wchar_t* REG_VALUE_STYLE = L"GlowStyle";

// Settings
void SaveSettings() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD style = (DWORD)g_cursorManager->GetGlowStyle();
        RegSetValueExW(hKey, REG_VALUE_STYLE, 0, REG_DWORD, (const BYTE*)&style, sizeof(style));
        RegCloseKey(hKey);
    }
}

void LoadSettings() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD style = 0;
        DWORD size = sizeof(style);
        if (RegQueryValueExW(hKey, REG_VALUE_STYLE, NULL, NULL, (LPBYTE)&style, &size) == ERROR_SUCCESS) {
            g_cursorManager->SetGlowStyle((CursorManager::GlowStyle)style);
        }
        RegCloseKey(hKey);
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // Start IME state check timer
        SetTimer(hwnd, TIMER_CHECK_IME, CHECK_INTERVAL_MS, NULL);
        return 0;
        
    case WM_TIMER:
        if (wParam == TIMER_CHECK_IME && g_imeMonitor && g_cursorManager) {
            ImeState state = g_imeMonitor->GetCurrentState();
            g_cursorManager->SetCursorForState(state);
            
            // Update tooltip
            if (g_trayIcon) {
                g_trayIcon->UpdateTooltip(
                    state == ImeState::Japanese ? L"IME: Japanese" : L"IME: English"
                );
            }
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
        }
        return 0;
        
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_CHECK_IME);
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
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
    g_trayIcon = new TrayIcon();
    
    if (!g_cursorManager->Initialize(hInstance)) {
        MessageBoxW(NULL, 
            L"Failed to load cursor files.\n"
            L"Please place cursor_en.cur and cursor_jp.cur in the resources folder.",
            L"Error", MB_ICONERROR);
        delete g_trayIcon;
        delete g_cursorManager;
        delete g_imeMonitor;
        return 1;
    }
    
    if (!g_trayIcon->Create(g_hwnd, hInstance, WM_TRAYICON)) {
        MessageBoxW(NULL, L"Failed to create tray icon", L"Error", MB_ICONERROR);
    }
    
    // Set initial state (force=true to ensure cursor is set)
    LoadSettings(); // 設定読み込み
    ImeState initialState = g_imeMonitor->GetCurrentState();
    g_cursorManager->SetCursorForState(initialState, true);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    delete g_trayIcon;
    delete g_cursorManager;
    delete g_imeMonitor;
    
    return (int)msg.wParam;
}
