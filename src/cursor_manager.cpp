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

// 英語モードのリソースID
static const int s_resourceIdsEn[CURSOR_TYPE_COUNT] = {
    IDC_CURSOR_ARROW_EN,
    IDC_CURSOR_IBEAM_EN,
    IDC_CURSOR_WAIT_EN,
    IDC_CURSOR_CROSS_EN,
    IDC_CURSOR_UPARROW_EN,
    IDC_CURSOR_SIZENWSE_EN,
    IDC_CURSOR_SIZENESW_EN,
    IDC_CURSOR_SIZEWE_EN,
    IDC_CURSOR_SIZENS_EN,
    IDC_CURSOR_SIZEALL_EN,
    IDC_CURSOR_NO_EN,
    IDC_CURSOR_HAND_EN
};

// 日本語モードのリソースID
static const int s_resourceIdsJp[CURSOR_TYPE_COUNT] = {
    IDC_CURSOR_ARROW_JP,
    IDC_CURSOR_IBEAM_JP,
    IDC_CURSOR_WAIT_JP,
    IDC_CURSOR_CROSS_JP,
    IDC_CURSOR_UPARROW_JP,
    IDC_CURSOR_SIZENWSE_JP,
    IDC_CURSOR_SIZENESW_JP,
    IDC_CURSOR_SIZEWE_JP,
    IDC_CURSOR_SIZENS_JP,
    IDC_CURSOR_SIZEALL_JP,
    IDC_CURSOR_NO_JP,
    IDC_CURSOR_HAND_JP
};

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
    , m_initialized(false) {
    for (int i = 0; i < CURSOR_TYPE_COUNT; i++) {
        m_cursorsEn[i] = NULL;
        m_cursorsJp[i] = NULL;
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
        m_cursorsEn[i] = LoadCursor(hInstance, MAKEINTRESOURCE(s_resourceIdsEn[i]));
        m_cursorsJp[i] = LoadCursor(hInstance, MAKEINTRESOURCE(s_resourceIdsJp[i]));
        
        if (m_cursorsEn[i] == NULL || m_cursorsJp[i] == NULL) {
            // リソースから読み込めない場合、元のカーソルを使用（グロー効果なし）
            // これにより、一部のカーソルが欠けても動作継続可能
            if (m_cursorsEn[i] == NULL && m_originals[i] != NULL) {
                m_cursorsEn[i] = CopyCursor(m_originals[i]);
            }
            if (m_cursorsJp[i] == NULL && m_originals[i] != NULL) {
                m_cursorsJp[i] = CopyCursor(m_originals[i]);
            }
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
        HCURSOR newCursor = (state == ImeState::English) ? m_cursorsEn[i] : m_cursorsJp[i];
        if (newCursor != NULL) {
            HCURSOR cursorCopy = CopyCursor(newCursor);
            if (cursorCopy != NULL) {
                SetSystemCursor(cursorCopy, s_ocrIds[i]);
            }
        }
    }
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
