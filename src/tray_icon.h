#pragma once

#include <windows.h>
#include <shellapi.h>

class TrayIcon {
public:
    TrayIcon();
    ~TrayIcon();
    
    // トレイアイコンを作成
    bool Create(HWND hwnd, HINSTANCE hInstance, UINT callbackMessage);
    
    // トレイアイコンを削除
    void Remove();
    
    // ツールチップを更新
    void UpdateTooltip(const wchar_t* text);
    
private:
    NOTIFYICONDATAW m_nid;
    bool m_created;
};
