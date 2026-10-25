#include "tray_icon.h"
#include "resource.h"

// タスクバーが明るい色か。値が読めなければ暗い色として扱う(Windowsの既定)
static bool IsLightTaskbar() {
    DWORD value = 0;
    DWORD size = sizeof(value);
    LSTATUS status = RegGetValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"SystemUsesLightTheme", RRF_RT_REG_DWORD, NULL, &value, &size);
    return status == ERROR_SUCCESS && value == 1;
}

// LoadImageWで読んだアイコンだけを破棄する。共有のデフォルトアイコンは破棄しない
static void DestroyOwnedIcon(HICON icon, bool owned) {
    if (owned && icon) {
        DestroyIcon(icon);
    }
}

TrayIcon::TrayIcon()
    : m_created(false), m_hInstance(NULL), m_iconOwned(false) {
    ZeroMemory(&m_nid, sizeof(m_nid));
}

TrayIcon::~TrayIcon() {
    Remove();
    DestroyOwnedIcon(m_nid.hIcon, m_iconOwned);
    m_nid.hIcon = NULL;
    m_iconOwned = false;
}

void TrayIcon::LoadThemeIcon() {
    UINT id = IsLightTaskbar() ? IDI_TRAY_LIGHT : IDI_TRAY_DARK;
    m_nid.hIcon = (HICON)LoadImageW(m_hInstance, MAKEINTRESOURCEW(id), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    m_iconOwned = (m_nid.hIcon != NULL);

    if (m_nid.hIcon == NULL) {
        // リソースから読み込めない場合はデフォルトアイコン
        m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
}

bool TrayIcon::Create(HWND hwnd, HINSTANCE hInstance, UINT callbackMessage) {
    m_hInstance = hInstance;
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = callbackMessage;
    LoadThemeIcon();

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

void TrayIcon::UpdateTheme() {
    if (!m_created) return;

    // 新しいアイコンを表示してから、前のアイコンを破棄する
    HICON oldIcon = m_nid.hIcon;
    bool oldOwned = m_iconOwned;
    LoadThemeIcon();
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
    DestroyOwnedIcon(oldIcon, oldOwned);
}
