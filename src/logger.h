#pragma once
#include <windows.h>

// ファイルへの簡易ログ。1 行 = "時刻 レベル 出来事"。
// 置き場所: %LOCALAPPDATA%\yCursory\logs\yCursory.log (1 MB を超えたら yCursory.1.log へ退避し 1 世代だけ残す)
// レベルはレジストリ HKCU\Software\yCursory の LogLevel (DWORD) で変える: 0=なし 1=error 2=warn 3=info 4=debug (既定 3)
// 呼び出し側は、毎回の処理ではなく状態が変わったときだけ書く (常に動く経路で書くとそれ自体が負荷になる)
enum class LogLevel : DWORD {
    Off = 0,
    Error = 1,
    Warn = 2,
    Info = 3,
    Debug = 4,
};

namespace Logger {
    void Init(const wchar_t* regKeyPath);
    bool Enabled(LogLevel level);
    void Write(LogLevel level, const wchar_t* format, ...);
}

#define LOG_ERROR(...) Logger::Write(LogLevel::Error, __VA_ARGS__)
#define LOG_WARN(...)  Logger::Write(LogLevel::Warn, __VA_ARGS__)
#define LOG_INFO(...)  Logger::Write(LogLevel::Info, __VA_ARGS__)
#define LOG_DEBUG(...) Logger::Write(LogLevel::Debug, __VA_ARGS__)
