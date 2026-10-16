#pragma once
#include <windows.h>

// 呼び出し側は、状態が変わったときだけ書く。常に動く経路で書くと、ログの書き込みが負荷を増やす
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
