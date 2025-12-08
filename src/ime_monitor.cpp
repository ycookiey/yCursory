#include "ime_monitor.h"

#pragma comment(lib, "imm32.lib")

// IMC_GETOPENSTATUS is not defined in some SDK versions
#ifndef IMC_GETOPENSTATUS
#define IMC_GETOPENSTATUS 0x0005
#endif

ImeMonitor::ImeMonitor() {
}

ImeMonitor::~ImeMonitor() {
}

ImeState ImeMonitor::GetCurrentState() {
    return GetStateForForegroundWindow();
}

ImeState ImeMonitor::GetStateForForegroundWindow() {
    HWND hwnd = GetForegroundWindow();
    if (hwnd == NULL) {
        return ImeState::English;
    }
    
    // フォアグラウンドウィンドウのスレッドIDを取得
    DWORD threadId = GetWindowThreadProcessId(hwnd, NULL);
    
    // キーボードレイアウトを取得
    HKL hkl = GetKeyboardLayout(threadId);
    
    // 日本語IMEレイアウトかどうかをチェック
    LANGID langId = LOWORD((DWORD_PTR)hkl);
    if (PRIMARYLANGID(langId) != LANG_JAPANESE) {
        return ImeState::English;
    }
    
    // IMEウィンドウを取得
    HWND imeWnd = ImmGetDefaultIMEWnd(hwnd);
    if (imeWnd == NULL) {
        return ImeState::English;
    }
    
    // IMEの開閉状態を取得（WM_IME_CONTROLメッセージを送信）
    LRESULT imeOpen = SendMessage(imeWnd, WM_IME_CONTROL, IMC_GETOPENSTATUS, 0);
    if (!imeOpen) {
        return ImeState::English;
    }
    
    // IMEコンテキストを取得して変換モードをチェック
    HIMC himc = ImmGetContext(hwnd);
    if (himc == NULL) {
        // コンテキストがなくてもIMEがOpenなら日本語モード
        return ImeState::Japanese;
    }
    
    DWORD conversion = 0;
    DWORD sentence = 0;
    
    if (ImmGetConversionStatus(himc, &conversion, &sentence)) {
        ImmReleaseContext(hwnd, himc);
        
        // IME_CMODE_NATIVE: 日本語入力モード
        if (conversion & IME_CMODE_NATIVE) {
            return ImeState::Japanese;
        }
        return ImeState::English;
    }
    
    ImmReleaseContext(hwnd, himc);
    
    // デフォルトで日本語（IMEがOpenなので）
    return ImeState::Japanese;
}

ImeState ImeMonitor::GetStateForWindow(HWND hwnd) {
    // この関数は現在未使用だが互換性のため残す
    return GetStateForForegroundWindow();
}
