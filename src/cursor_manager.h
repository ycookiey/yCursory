#pragma once

#include <windows.h>
#include "ime_monitor.h"

// 対応するシステムカーソルの数
#define CURSOR_TYPE_COUNT 12

class CursorManager {
public:
    CursorManager();
    ~CursorManager();
    
    // 初期化（カーソルリソース読み込み）
    bool Initialize(HINSTANCE hInstance);
    
    // IME状態に応じてカーソルを設定
    void SetCursorForState(ImeState state, bool force = false);
    
    // 元のカーソルに復元
    void RestoreOriginalCursor();
    
private:
    // カーソル種類のインデックス
    enum CursorType {
        CURSOR_ARROW = 0,
        CURSOR_IBEAM,
        CURSOR_WAIT,
        CURSOR_CROSS,
        CURSOR_UPARROW,
        CURSOR_SIZENWSE,
        CURSOR_SIZENESW,
        CURSOR_SIZEWE,
        CURSOR_SIZENS,
        CURSOR_SIZEALL,
        CURSOR_NO,
        CURSOR_HAND
    };
    
    // 各カーソル種類に対応するOCR ID
    static const DWORD s_ocrIds[CURSOR_TYPE_COUNT];
    
    HCURSOR m_cursorsEn[CURSOR_TYPE_COUNT];  // 英字モード用カーソル
    HCURSOR m_cursorsJp[CURSOR_TYPE_COUNT];  // 日本語モード用カーソル
    HCURSOR m_originals[CURSOR_TYPE_COUNT];  // 元のカーソル
    ImeState m_currentState;
    bool m_initialized;
};
