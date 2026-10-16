#pragma once

#include <windows.h>
#include <imm.h>

// IME状態
enum class ImeState {
    English,   // 英字入力
    Japanese   // 日本語入力
};

class ImeMonitor {
public:
    ImeMonitor();
    ~ImeMonitor();
    
    // 現在のIME状態を取得
    ImeState GetCurrentState();
    
    // フォアグラウンドウィンドウのIME状態を取得
    ImeState GetStateForForegroundWindow();
    
private:
    ImeState GetStateForWindow(HWND hwnd);

    // 前面のアプリが応答しないときに返す、前回の状態
    ImeState m_lastState = ImeState::English;
};
