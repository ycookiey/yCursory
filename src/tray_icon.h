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

    // タスクバーの色が変わったら、それに合うアイコンに差し替える
    void UpdateTheme();

private:
    // タスクバーの色に合うアイコンを読み込み、m_nid.hIconにする。前のアイコンは呼び出し側が破棄する
    void LoadThemeIcon();

    NOTIFYICONDATAW m_nid;
    bool m_created;
    HINSTANCE m_hInstance;
    bool m_iconOwned;   // m_nid.hIconをLoadImageWで読んだか。共有のデフォルトアイコンは破棄しない
};
