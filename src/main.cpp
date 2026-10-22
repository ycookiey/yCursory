#include <windows.h>
#include "ime_monitor.h"
#include "cursor_manager.h"
#include "caret_indicator.h"
#include "tray_icon.h"
#include "startup_manager.h"
#include "logger.h"
#include "ime_provider.h"
#include "resource.h"

static ImeMonitor* g_imeMonitor = nullptr;
static CursorManager* g_cursorManager = nullptr;
static CaretIndicator* g_caretIndicator = nullptr;
static TrayIcon* g_trayIcon = nullptr;
static HWND g_hwnd = NULL;

#define TIMER_TICK 1
#define TICK_MS 16                 // IME状態とキャレット位置を確認する間隔。60fpsに当たる
#define GRACE_MS 1000              // キャレットが取れないまま1秒続いたら、確認を止める

static bool g_active = false;
static DWORD g_graceStart = 0;      // キャレットが取れなくなった時刻か、確認を再開した時刻
static bool g_caretSeen = false;    // 直近の確認でキャレットが取れたか
static bool g_textTarget = false;   // 表示が無効のとき: 再開後に入力先が見つかったか。見つかれば以後は探さない
static HWINEVENTHOOK g_foregroundHook = NULL;
static HWINEVENTHOOK g_focusHook = NULL;

const wchar_t* REG_KEY_PATH = L"Software\\yCursory";
const wchar_t* REG_VALUE_STYLE = L"GlowStyle";
const wchar_t* REG_VALUE_CARET_ENABLED = L"CaretIndicatorEnabled";

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

// ログ用: ウィンドウのプロセスの実行ファイル名。取れなければ"?"
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
    if (g_caretIndicator) {
        const wchar_t* method = g_caretIndicator->GetLastMethodName();
        if (method != lastMethod) {
            lastMethod = method;
            LOG_INFO(L"caret: %s", method);
        }
    }
}

static void Pause(const wchar_t* reason) {
    if (!g_active) return;
    KillTimer(g_hwnd, TIMER_TICK);
    g_active = false;
    // タイマーを止めるだけでなく、覚えている位置も破棄する。位置を残すと、同じウィンドウでフォーカスの切り替えの通知で再開したとき、古い位置にキャレットインジケーターが出る
    if (g_caretIndicator) g_caretIndicator->ResetCaret();
    LOG_INFO(L"pause: %s", reason);
}

// newWindow: 前面のウィンドウが変わった。trueなら前のウィンドウのキャレット位置を破棄する
static void Resume(const wchar_t* reason, bool newWindow) {
    g_graceStart = GetTickCount();
    g_caretSeen = false;
    g_textTarget = false;
    if (newWindow && g_caretIndicator) g_caretIndicator->ResetCaret();
    if (g_active) return;
    SetTimer(g_hwnd, TIMER_TICK, TICK_MS, NULL);
    g_active = true;
    LOG_INFO(L"resume: %s", reason);
}

static void ApplyState(ImeState state) {
    g_cursorManager->SetCursorForState(state);

    // トレイのツールチップは、状態が変わったときだけ書き換える。書き換えるたびにexplorerへ通知が飛ぶ
    static int tooltipState = -1;
    if (g_trayIcon && (int)state != tooltipState) {
        tooltipState = (int)state;
        g_trayIcon->UpdateTooltip(state == ImeState::Japanese ? L"IME: Japanese" : L"IME: English");
    }
}

// タイマーが止まっていても、知らせを受けた時点でカーソルとキャレットインジケーターの色を変える
static void OnProviderOpen(ImeState state) {
    ApplyState(state);
    if (g_caretIndicator) g_caretIndicator->SetState(state);
}

// g_activeの間は自分でキャレットを取れているので、知らせの位置は使わない
static void OnProviderCaret(bool hasPosition, RECT physical) {
    if (g_active || !g_caretIndicator) return;
    if (hasPosition) {
        g_caretIndicator->ShowAtPhysical({physical.left, physical.top});
    } else {
        g_caretIndicator->ResetCaret();
    }
}

static void Tick() {
    ImeState state;
    if (!ImeProvider::GetOpenState(&state)) {
        state = g_imeMonitor->GetCurrentState();
    }
    ApplyState(state);

    bool caretFound;
    if (g_caretIndicator->IsEnabled()) {
        caretFound = g_caretIndicator->UpdatePosition(state);
        g_caretIndicator->UpdateDisplay();
    } else if (!g_textTarget) {
        // 表示が無効なら位置は不要。止めるかの判断のため、再開の直後に入力先の有無だけ調べる
        caretFound = g_caretIndicator->Probe();
        g_textTarget = caretFound;
    } else {
        caretFound = true;
    }
    LogChanges(state);

    if (caretFound) {
        g_caretSeen = true;
        return;
    }
    if (g_caretSeen) {
        g_caretSeen = false;
        g_graceStart = GetTickCount();
    }
    // 再開のきっかけを受け取れないときは止めない。止めると再開できない
    if (GetTickCount() - g_graceStart >= GRACE_MS && g_foregroundHook && g_focusHook) {
        Pause(L"no caret");
    }
}

// OSからの通知。WINEVENT_OUTOFCONTEXTなので、SetWinEventHookを呼んだスレッドのメッセージループの中で呼ばれる。
// 前面のアプリへの問い合わせは、次のタイマーまで遅らせる
static void CALLBACK OnWinEvent(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG, LONG, DWORD, DWORD) {
    if (event == EVENT_SYSTEM_FOREGROUND) {
        ImeProvider::OnForegroundChanged(hwnd);
        Resume(L"foreground", true);
    } else if (event == EVENT_OBJECT_FOCUS) {
        Resume(L"focus", false);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // タイマーは、部品の初期化が終わってからWinMainで始める
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_TICK && g_imeMonitor && g_cursorManager && g_caretIndicator) {
            Tick();
        }
        return 0;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
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
                Resume(L"setting", false);
            }
        }
        return 0;
        
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_TICK);
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    Logger::Init(REG_KEY_PATH);
    LOG_INFO(L"start (pid=%u)", GetCurrentProcessId());

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ImeCursorIndicator";
    
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Window class registration failed", L"Error", MB_ICONERROR);
        return 1;
    }
    
    g_hwnd = CreateWindowExW(
        0, L"ImeCursorIndicator", L"IME Cursor Indicator",
        0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL
    );
    
    if (g_hwnd == NULL) {
        MessageBoxW(NULL, L"Window creation failed", L"Error", MB_ICONERROR);
        return 1;
    }
    
    g_imeMonitor = new ImeMonitor();
    g_cursorManager = new CursorManager();
    g_caretIndicator = new CaretIndicator();
    g_trayIcon = new TrayIcon();

    if (!g_cursorManager->Initialize(hInstance)) {
        MessageBoxW(NULL,
            L"Failed to load the cursors embedded in yCursory.exe.\n"
            L"The executable may be damaged. Please reinstall yCursory.",
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

    LoadSettings(); // 設定読み込み（キャレットインジケーターの設定も含む）
    ImeState initialState = g_imeMonitor->GetCurrentState();
    // 状態が前回と同じでも、起動時はforce=trueで必ずカーソルを設定する
    g_cursorManager->SetCursorForState(initialState, true);

    ImeProvider::Callbacks providerCallbacks = { OnProviderOpen, OnProviderCaret };
    ImeProvider::Start(hInstance, providerCallbacks);

    // 再開のきっかけ。範囲で登録すると間にある大量のイベントまで届くので、1種類ずつ登録する
    g_foregroundHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, OnWinEvent,
        0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    g_focusHook = SetWinEventHook(EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, NULL, OnWinEvent,
        0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    if (!g_foregroundHook || !g_focusHook) {
        LOG_ERROR(L"SetWinEventHook failed (foreground=%p focus=%p)", g_foregroundHook, g_focusHook);
    }
    Resume(L"start", true);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    if (g_foregroundHook) UnhookWinEvent(g_foregroundHook);
    if (g_focusHook) UnhookWinEvent(g_focusHook);
    ImeProvider::Stop();
    LOG_INFO(L"exit");

    delete g_trayIcon;
    delete g_caretIndicator;
    delete g_cursorManager;
    delete g_imeMonitor;

    return (int)msg.wParam;
}
