#define OEMRESOURCE
#include "cursor_manager.h"
#include "resource.h"

// 各カーソル種類に対応するOCR ID
const DWORD CursorManager::s_ocrIds[CURSOR_TYPE_COUNT] = {
    OCR_NORMAL,    // CURSOR_ARROW
    OCR_IBEAM,     // CURSOR_IBEAM
    OCR_WAIT,      // CURSOR_WAIT
    OCR_CROSS,     // CURSOR_CROSS
    OCR_UP,        // CURSOR_UPARROW
    OCR_SIZENWSE,  // CURSOR_SIZENWSE
    OCR_SIZENESW,  // CURSOR_SIZENESW
    OCR_SIZEWE,    // CURSOR_SIZEWE
    OCR_SIZENS,    // CURSOR_SIZENS
    OCR_SIZEALL,   // CURSOR_SIZEALL
    OCR_NO,        // CURSOR_NO
    OCR_HAND       // CURSOR_HAND
};

// 英語モード（Light）のリソースID
static const int s_resourceIdsEnLight[CURSOR_TYPE_COUNT] = {
    IDC_CURSOR_ARROW_EN_LIGHT,
    IDC_CURSOR_IBEAM_EN_LIGHT,
    IDC_CURSOR_WAIT_EN_LIGHT,
    IDC_CURSOR_CROSS_EN_LIGHT,
    IDC_CURSOR_UPARROW_EN_LIGHT,
    IDC_CURSOR_SIZENWSE_EN_LIGHT,
    IDC_CURSOR_SIZENESW_EN_LIGHT,
    IDC_CURSOR_SIZEWE_EN_LIGHT,
    IDC_CURSOR_SIZENS_EN_LIGHT,
    IDC_CURSOR_SIZEALL_EN_LIGHT,
    IDC_CURSOR_NO_EN_LIGHT,
    IDC_CURSOR_HAND_EN_LIGHT
};

// 日本語モード（Light）のリソースID
static const int s_resourceIdsJpLight[CURSOR_TYPE_COUNT] = {
    IDC_CURSOR_ARROW_JP_LIGHT,
    IDC_CURSOR_IBEAM_JP_LIGHT,
    IDC_CURSOR_WAIT_JP_LIGHT,
    IDC_CURSOR_CROSS_JP_LIGHT,
    IDC_CURSOR_UPARROW_JP_LIGHT,
    IDC_CURSOR_SIZENWSE_JP_LIGHT,
    IDC_CURSOR_SIZENESW_JP_LIGHT,
    IDC_CURSOR_SIZEWE_JP_LIGHT,
    IDC_CURSOR_SIZENS_JP_LIGHT,
    IDC_CURSOR_SIZEALL_JP_LIGHT,
    IDC_CURSOR_NO_JP_LIGHT,
    IDC_CURSOR_HAND_JP_LIGHT
};

// 英語モード（Heavy）のリソースID
static const int s_resourceIdsEnHeavy[CURSOR_TYPE_COUNT] = {
    IDC_CURSOR_ARROW_EN_HEAVY,
    IDC_CURSOR_IBEAM_EN_HEAVY,
    IDC_CURSOR_WAIT_EN_HEAVY,
    IDC_CURSOR_CROSS_EN_HEAVY,
    IDC_CURSOR_UPARROW_EN_HEAVY,
    IDC_CURSOR_SIZENWSE_EN_HEAVY,
    IDC_CURSOR_SIZENESW_EN_HEAVY,
    IDC_CURSOR_SIZEWE_EN_HEAVY,
    IDC_CURSOR_SIZENS_EN_HEAVY,
    IDC_CURSOR_SIZEALL_EN_HEAVY,
    IDC_CURSOR_NO_EN_HEAVY,
    IDC_CURSOR_HAND_EN_HEAVY
};

// 日本語モード（Heavy）のリソースID
static const int s_resourceIdsJpHeavy[CURSOR_TYPE_COUNT] = {
    IDC_CURSOR_ARROW_JP_HEAVY,
    IDC_CURSOR_IBEAM_JP_HEAVY,
    IDC_CURSOR_WAIT_JP_HEAVY,
    IDC_CURSOR_CROSS_JP_HEAVY,
    IDC_CURSOR_UPARROW_JP_HEAVY,
    IDC_CURSOR_SIZENWSE_JP_HEAVY,
    IDC_CURSOR_SIZENESW_JP_HEAVY,
    IDC_CURSOR_SIZEWE_JP_HEAVY,
    IDC_CURSOR_SIZENS_JP_HEAVY,
    IDC_CURSOR_SIZEALL_JP_HEAVY,
    IDC_CURSOR_NO_JP_HEAVY,
    IDC_CURSOR_HAND_JP_HEAVY
};


// 英語モード（右下強調）のリソースID...は削除（上で追加済み）

// 日本語モード（右下強調）のリソースID...は削除（上で追加済み）

// システムカーソルを取得するためのIDC定数
static const LPCTSTR s_systemCursorIds[CURSOR_TYPE_COUNT] = {
    IDC_ARROW,
    IDC_IBEAM,
    IDC_WAIT,
    IDC_CROSS,
    IDC_UPARROW,
    IDC_SIZENWSE,
    IDC_SIZENESW,
    IDC_SIZEWE,
    IDC_SIZENS,
    IDC_SIZEALL,
    IDC_NO,
    IDC_HAND
};

CursorManager::CursorManager()
    : m_currentState(ImeState::English)
    , m_currentStyle(GlowStyle::StyleHeavy) // デフォルトはHeavy（旧右下強調）
    , m_initialized(false) {
    for (int i = 0; i < CURSOR_TYPE_COUNT; i++) {
        m_cursorsEnLight[i] = NULL;
        m_cursorsJpLight[i] = NULL;
        m_cursorsEnHeavy[i] = NULL;
        m_cursorsJpHeavy[i] = NULL;
        m_originals[i] = NULL;
    }
}

CursorManager::~CursorManager() {
    RestoreOriginalCursor();
}

bool CursorManager::Initialize(HINSTANCE hInstance) {
    bool success = true;
    
    // 全種類のカーソルを読み込み・保存
    for (int i = 0; i < CURSOR_TYPE_COUNT; i++) {
        // 元のカーソルを保存
        m_originals[i] = CopyCursor(LoadCursor(NULL, s_systemCursorIds[i]));
        
        // カスタムカーソルをリソースから読み込み
        m_cursorsEnLight[i] = LoadCursor(hInstance, MAKEINTRESOURCE(s_resourceIdsEnLight[i]));
        m_cursorsJpLight[i] = LoadCursor(hInstance, MAKEINTRESOURCE(s_resourceIdsJpLight[i]));
        m_cursorsEnHeavy[i] = LoadCursor(hInstance, MAKEINTRESOURCE(s_resourceIdsEnHeavy[i]));
        m_cursorsJpHeavy[i] = LoadCursor(hInstance, MAKEINTRESOURCE(s_resourceIdsJpHeavy[i]));
        
        // リソース読み込み失敗時のフォールバック (Light)
        if (m_cursorsEnLight[i] == NULL && m_originals[i] != NULL) {
            m_cursorsEnLight[i] = CopyCursor(m_originals[i]);
        }
        if (m_cursorsJpLight[i] == NULL && m_originals[i] != NULL) {
            m_cursorsJpLight[i] = CopyCursor(m_originals[i]);
        }
        
        // リソース読み込み失敗時のフォールバック (Heavy)
        if (m_cursorsEnHeavy[i] == NULL) {
            m_cursorsEnHeavy[i] = (m_cursorsEnLight[i] != NULL) ? CopyCursor(m_cursorsEnLight[i]) : CopyCursor(m_originals[i]);
        }
        if (m_cursorsJpHeavy[i] == NULL) {
            m_cursorsJpHeavy[i] = (m_cursorsJpLight[i] != NULL) ? CopyCursor(m_cursorsJpLight[i]) : CopyCursor(m_originals[i]);
        }
    }
    
    m_initialized = true;
    return success;
}

void CursorManager::SetCursorForState(ImeState state, bool force) {
    if (!m_initialized) return;
    if (!force && m_currentState == state) return;
    
    m_currentState = state;
    
    // 全種類のカーソルを更新
    for (int i = 0; i < CURSOR_TYPE_COUNT; i++) {
        HCURSOR newCursor = NULL;
        
        if (m_currentStyle == GlowStyle::StyleHeavy) {
            newCursor = (state == ImeState::English) ? m_cursorsEnHeavy[i] : m_cursorsJpHeavy[i];
        } else {
            newCursor = (state == ImeState::English) ? m_cursorsEnLight[i] : m_cursorsJpLight[i];
        }

        if (newCursor != NULL) {
            HCURSOR cursorCopy = CopyCursor(newCursor);
            if (cursorCopy != NULL) {
                SetSystemCursor(cursorCopy, s_ocrIds[i]);
            }
        }
    }
}

void CursorManager::SetGlowStyle(GlowStyle style) {
    if (m_currentStyle == style) return;
    
    m_currentStyle = style;
    // スタイル変更を反映するためにカーソルを強制更新
    SetCursorForState(m_currentState, true);
}

void CursorManager::RestoreOriginalCursor() {
    // 全種類の元のカーソルを復元
    for (int i = 0; i < CURSOR_TYPE_COUNT; i++) {
        if (m_originals[i] != NULL) {
            HCURSOR cursorCopy = CopyCursor(m_originals[i]);
            if (cursorCopy != NULL) {
                SetSystemCursor(cursorCopy, s_ocrIds[i]);
            }
            DestroyCursor(m_originals[i]);
            m_originals[i] = NULL;
        }
    }
    
    // システムカーソルをリセット
    SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
    
    m_initialized = false;
}
