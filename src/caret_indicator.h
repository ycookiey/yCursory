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
    bool UpdatePosition(ImeState state);  // 位置検出のみ。キャレットが取れたらtrue。表示が無効のときはfalse
    void UpdateDisplay();                 // 位置か状態が変わったときだけ描き直す
    bool Probe();                         // 表示の有無に関係なく、キャレットが取れるかを1回だけ調べる
    void ResetCaret();                    // 覚えているキャレット位置を破棄して隠す
    void SetState(ImeState state);        // 外から知らされたときに、IME状態だけを変える。表示中なら色を描き直す
    void ShowAtPhysical(POINT physical);  // 外から知らされた、物理ピクセルの位置に出す
    void Show();
    void Hide();
    void Cleanup();

    // 有効/無効の切り替え
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return m_enabled; }

    // ログに書く、直近のキャレット位置の取得に使った方法。取れなかったときは"none"
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

    // 最後に描いた内容。次に描く内容が同じなら描き直さない
    bool m_rendered;
    POINT m_renderedPos;
    ImeState m_renderedState;

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
        METHOD_TSF,            // TSF。yCursory自身のスレッドのキャレットだけ取れる
        METHOD_UIAUTOMATION,   // UI Automation。別プロセスのキャレットも取れる
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
