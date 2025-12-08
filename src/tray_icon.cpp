#include "tray_icon.h"
#include "resource.h"

TrayIcon::TrayIcon()
    : m_created(false) {
    ZeroMemory(&m_nid, sizeof(m_nid));
}

TrayIcon::~TrayIcon() {
    Remove();
}

bool TrayIcon::Create(HWND hwnd, HINSTANCE hInstance, UINT callbackMessage) {
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = callbackMessage;
    m_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    
    if (m_nid.hIcon == NULL) {
        // リソースから読み込めない場合はデフォルトアイコン
        m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    
    wcscpy_s(m_nid.szTip, L"IME Cursor Indicator");
    
    m_created = Shell_NotifyIconW(NIM_ADD, &m_nid);
    return m_created;
}

void TrayIcon::Remove() {
    if (m_created) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_created = false;
    }
}

void TrayIcon::UpdateTooltip(const wchar_t* text) {
    if (m_created) {
        wcscpy_s(m_nid.szTip, text);
        Shell_NotifyIconW(NIM_MODIFY, &m_nid);
    }
}
