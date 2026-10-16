#include "ime_provider.h"
#include "logger.h"

namespace {
    const wchar_t* RECEIVER_CLASS = L"yCursory.ImeStateReceiver.v1";
    const wchar_t* MSG_FOCUS_NAME = L"yCursory.ImeState.v1.Focus";
    const wchar_t* MSG_OPEN_NAME = L"yCursory.ImeState.v1.Open";
    const wchar_t* MSG_CARET_NAME = L"yCursory.ImeState.v1.Caret";
    // Caret の「位置なし」: wParam と lParam の両方がこの値
    const DWORD NO_POSITION = 0x80008000;

    HWND g_receiver = NULL;
    UINT g_msgFocus = 0;
    UINT g_msgOpen = 0;
    UINT g_msgCaret = 0;
    ImeProvider::Callbacks g_callbacks = {};

    HWND g_window = NULL;   // 担当している窓 (root)。NULL なら担当なし
    int g_open = -1;        // -1: 知らされていない 0: オフ 1: オン

    HWND RootOf(HWND hwnd) {
        return hwnd ? GetAncestor(hwnd, GA_ROOT) : NULL;
    }

    void Clear(const wchar_t* reason) {
        if (!g_window) return;
        g_window = NULL;
        g_open = -1;
        LOG_INFO(L"provider: end (%s)", reason);
    }

    // 16 ビットずつ詰めた 2 つの値を取り出す (32 ビットのプロセスから送られても同じ形)
    void Unpack(WPARAM w, LPARAM l, RECT* rect) {
        DWORD a = (DWORD)w;
        DWORD b = (DWORD)l;
        rect->left = (SHORT)LOWORD(a);
        rect->top = (SHORT)HIWORD(a);
        rect->right = (SHORT)LOWORD(b);
        rect->bottom = (SHORT)HIWORD(b);
    }

    LRESULT CALLBACK ReceiverProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == g_msgFocus && g_msgFocus) {
            // HWND は 32 ビットの範囲で意味を持つので、32 ビットのプロセスから送られても復元できる
            HWND target = (HWND)(ULONG_PTR)(DWORD)wParam;
            if (lParam) {
                if (!target || !IsWindow(target)) {
                    Clear(L"unknown window");
                    return 0;
                }
                g_window = RootOf(target);
                g_open = -1;
                LOG_INFO(L"provider: begin (window=%p)", g_window);
            } else if (!target || RootOf(target) == g_window) {
                Clear(L"focus lost");
            }
            return 0;
        }
        if (msg == g_msgOpen && g_msgOpen) {
            if (!g_window) return 0;
            g_open = wParam ? 1 : 0;
            LOG_DEBUG(L"provider: open=%d", g_open);
            if (ImeProvider::CoversForeground() && g_callbacks.onOpenChanged) {
                g_callbacks.onOpenChanged(g_open ? ImeState::Japanese : ImeState::English);
            }
            return 0;
        }
        if (msg == g_msgCaret && g_msgCaret) {
            if (!ImeProvider::CoversForeground() || !g_callbacks.onCaret) return 0;
            RECT rect = {};
            bool hasPosition = !((DWORD)wParam == NO_POSITION && (DWORD)lParam == NO_POSITION);
            if (hasPosition) Unpack(wParam, lParam, &rect);
            g_callbacks.onCaret(hasPosition, rect);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

bool ImeProvider::Start(HINSTANCE hInstance, const Callbacks& callbacks) {
    g_callbacks = callbacks;
    g_msgFocus = RegisterWindowMessageW(MSG_FOCUS_NAME);
    g_msgOpen = RegisterWindowMessageW(MSG_OPEN_NAME);
    g_msgCaret = RegisterWindowMessageW(MSG_CARET_NAME);
    if (!g_msgFocus || !g_msgOpen || !g_msgCaret) {
        LOG_ERROR(L"provider: RegisterWindowMessage failed");
        return false;
    }

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ReceiverProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = RECEIVER_CLASS;
    if (!RegisterClassExW(&wc)) {
        LOG_ERROR(L"provider: RegisterClassEx failed (%lu)", GetLastError());
        return false;
    }
    g_receiver = CreateWindowExW(0, RECEIVER_CLASS, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (!g_receiver) {
        LOG_ERROR(L"provider: CreateWindowEx failed (%lu)", GetLastError());
        return false;
    }
    // 権限の低いプロセス (サンドボックスのアプリの中の IME 等) からも届くようにする
    ChangeWindowMessageFilterEx(g_receiver, g_msgFocus, MSGFLT_ALLOW, NULL);
    ChangeWindowMessageFilterEx(g_receiver, g_msgOpen, MSGFLT_ALLOW, NULL);
    ChangeWindowMessageFilterEx(g_receiver, g_msgCaret, MSGFLT_ALLOW, NULL);
    return true;
}

void ImeProvider::Stop() {
    if (g_receiver) {
        DestroyWindow(g_receiver);
        g_receiver = NULL;
    }
}

bool ImeProvider::CoversForeground() {
    if (!g_window) return false;
    if (!IsWindow(g_window)) {
        Clear(L"window closed");
        return false;
    }
    return RootOf(GetForegroundWindow()) == g_window;
}

bool ImeProvider::GetOpenState(ImeState* state) {
    if (g_open < 0 || !CoversForeground()) return false;
    *state = g_open ? ImeState::Japanese : ImeState::English;
    return true;
}

void ImeProvider::OnForegroundChanged(HWND foreground) {
    // 担当の知らせが前面の切り替えより先に届くことがあるので、同じ窓なら残す
    if (g_window && RootOf(foreground) != g_window) {
        Clear(L"foreground changed");
    }
}
