#pragma once

#include <windows.h>
#include "ime_monitor.h"

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
    HCURSOR m_cursorEn;      // 英字モード用カーソル
    HCURSOR m_cursorJp;      // 日本語モード用カーソル
    HCURSOR m_originalArrow; // 元の矢印カーソル
    ImeState m_currentState;
    bool m_initialized;
};
