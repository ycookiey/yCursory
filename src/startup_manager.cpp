#include "startup_manager.h"
#include <cstdio>

const wchar_t* StartupManager::REGISTRY_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t* StartupManager::APP_NAME = L"yCursory";

bool StartupManager::IsRegistered() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    DWORD type;
    DWORD size = 0;
    bool exists = (RegQueryValueExW(hKey, APP_NAME, NULL, &type, NULL, &size) == ERROR_SUCCESS);
    
    RegCloseKey(hKey);
    return exists;
}

bool StartupManager::Register() {
    // 現在のEXEパスを取得
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0) {
        return false;
    }
    
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    // パスを引用符で囲む（スペース対策）
    wchar_t quotedPath[MAX_PATH + 2];
    swprintf_s(quotedPath, L"\"%s\"", exePath);
    
    DWORD size = (DWORD)((wcslen(quotedPath) + 1) * sizeof(wchar_t));
    bool success = (RegSetValueExW(hKey, APP_NAME, 0, REG_SZ, 
                                   (const BYTE*)quotedPath, size) == ERROR_SUCCESS);
    
    RegCloseKey(hKey);
    return success;
}

bool StartupManager::Unregister() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    bool success = (RegDeleteValueW(hKey, APP_NAME) == ERROR_SUCCESS);
    
    RegCloseKey(hKey);
    return success;
}
