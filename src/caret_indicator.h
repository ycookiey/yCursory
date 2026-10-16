#pragma once
#include <windows.h>
#include <msctf.h>
#include <UIAutomation.h>
#include <gdiplus.h>
#include "ime_monitor.h"

// TSFはCOMベースなのでole32.libのみで動作（msctf.libは不要）
#pragma comment(lib, "gdiplus.lib")

class CaretIndicator {
public:
    CaretIndicator();
    ~CaretIndicator();

    bool Initialize(HINSTANCE hInstance);
    void Update(ImeState state);  // 後方互換性のために保持（内部でUpdatePosition+UpdateDisplayを呼ぶ）
    void UpdatePosition(ImeState state);  // 位置検出のみ（低頻度：100ms）
    void UpdateDisplay();                 // 補間と描画のみ（高頻度：16ms）
    void Show();
    void Hide();
    void Cleanup();

    // 有効/無効の切り替え
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return m_enabled; }

    // 直近のキャレット位置の取得に使った方法 (ログ用)。取れなかったときは "none"
    const wchar_t* GetLastMethodName() const;

private:
    // レイヤードウィンドウ
    HWND m_hwnd;
    HINSTANCE m_hInstance;

    // 状態管理
    ImeState m_currentState;
    bool m_enabled;
    bool m_visible;
    POINT m_lastCaretPos;      // 取得したキャレット位置（目標位置）
    POINT m_currentDisplayPos; // 現在の表示位置
    HWND m_lastCaretWindow;
    DWORD m_lastUpdateTick;

    // TSF関連
    ITfThreadMgr* m_threadMgr;
    TfClientId m_clientId;

    // UI Automation関連
    IUIAutomation* m_uiAutomation;

    ULONG_PTR m_gdiplusToken;

    // GDI+リソース
    Gdiplus::Bitmap* m_backBuffer;

    // 検出方式
    enum DetectionMethod {
        METHOD_GUITHREADINFO,  // 標準アプリ
        METHOD_IMM32,          // IMEコンテキスト
        METHOD_TSF,            // TSF (同一プロセス)
        METHOD_UIAUTOMATION,   // UI Automation (別プロセス対応)
        METHOD_NONE
    };
    DetectionMethod m_lastMethod;

    // キャレット位置取得
    bool GetCaretPosition(POINT* pos, HWND* caretWnd);
    bool TryGetFromGUIThreadInfo(POINT* pos, HWND* caretWnd);
    bool TryGetFromIMM32(POINT* pos, HWND* caretWnd);
    bool TryGetFromTSF(POINT* pos, HWND* caretWnd);
    bool TryGetFromUIAutomation(POINT* pos, HWND* caretWnd);
    bool TryGetCaretFromElement(IUIAutomationElement* element, POINT* pos, HWND* caretWnd);

    // ウィンドウ管理
    bool CreateIndicatorWindow();
    void UpdateWindowPosition(POINT caretPos);
    void RenderGlow();
    void DrawGlow(Gdiplus::Graphics* graphics, Gdiplus::Color color);

    // ヘルパー
    bool ShouldUpdate(POINT newPos, HWND newWnd);
    Gdiplus::Color GetGlowColor(ImeState state);

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
