#define OEMRESOURCE
#include "cursor_manager.h"
#include "resource.h"

CursorManager::CursorManager()
    : m_cursorEn(NULL)
    , m_cursorJp(NULL)
    , m_originalArrow(NULL)
    , m_currentState(ImeState::English)
    , m_initialized(false) {
}

CursorManager::~CursorManager() {
    RestoreOriginalCursor();
}

bool CursorManager::Initialize(HINSTANCE hInstance) {
    // Save original cursor
    m_originalArrow = CopyCursor(LoadCursor(NULL, IDC_ARROW));
    if (m_originalArrow == NULL) {
        return false;
    }
    
    // Load custom cursors from resource
    m_cursorEn = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR_EN));
    m_cursorJp = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR_JP));
    
    if (m_cursorEn == NULL || m_cursorJp == NULL) {
        // Try loading from file if resource loading fails
        m_cursorEn = LoadCursorFromFileW(L"resources\\cursor_en.cur");
        m_cursorJp = LoadCursorFromFileW(L"resources\\cursor_jp.cur");
    }
    
    if (m_cursorEn == NULL || m_cursorJp == NULL) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void CursorManager::SetCursorForState(ImeState state, bool force) {
    if (!m_initialized) return;
    if (!force && m_currentState == state) return;
    
    m_currentState = state;
    
    HCURSOR newCursor = (state == ImeState::English) ? m_cursorEn : m_cursorJp;
    HCURSOR cursorCopy = CopyCursor(newCursor);
    
    if (cursorCopy != NULL) {
        // Replace system arrow cursor
        SetSystemCursor(cursorCopy, OCR_NORMAL);
    }
}

void CursorManager::RestoreOriginalCursor() {
    if (m_originalArrow != NULL) {
        // Restore original cursor
        HCURSOR cursorCopy = CopyCursor(m_originalArrow);
        if (cursorCopy != NULL) {
            SetSystemCursor(cursorCopy, OCR_NORMAL);
        }
        DestroyCursor(m_originalArrow);
        m_originalArrow = NULL;
    }
    
    // Reset system cursors
    SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
    
    m_initialized = false;
}
