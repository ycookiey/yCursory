#pragma once

#include <windows.h>

// スタートアップ登録管理クラス
// レジストリHKCU\Software\Microsoft\Windows\CurrentVersion\Runを使用
class StartupManager {
public:
    // スタートアップに登録済みか確認
    static bool IsRegistered();
    
    // スタートアップに登録
    static bool Register();
    
    // スタートアップから解除
    static bool Unregister();
    
private:
    static const wchar_t* REGISTRY_KEY;
    static const wchar_t* APP_NAME;
};
