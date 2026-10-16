#include "logger.h"
#include <cstdarg>
#include <cstdio>
#include <string>

namespace {
    LogLevel g_level = LogLevel::Info;
    std::wstring g_path;
    std::wstring g_oldPath;
    const LONGLONG MAX_BYTES = 1024 * 1024;

    const wchar_t* LevelName(LogLevel level) {
        switch (level) {
        case LogLevel::Error: return L"ERROR";
        case LogLevel::Warn:  return L"WARN";
        case LogLevel::Info:  return L"INFO";
        case LogLevel::Debug: return L"DEBUG";
        default:              return L"-";
        }
    }

    void RotateIfLarge() {
        WIN32_FILE_ATTRIBUTE_DATA attr;
        if (!GetFileAttributesExW(g_path.c_str(), GetFileExInfoStandard, &attr)) return;
        LONGLONG size = ((LONGLONG)attr.nFileSizeHigh << 32) | attr.nFileSizeLow;
        if (size < MAX_BYTES) return;
        MoveFileExW(g_path.c_str(), g_oldPath.c_str(), MOVEFILE_REPLACE_EXISTING);
    }
}

void Logger::Init(const wchar_t* regKeyPath) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, regKeyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 0;
        DWORD size = sizeof(value);
        if (RegQueryValueExW(hKey, L"LogLevel", NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS
            && value <= (DWORD)LogLevel::Debug) {
            g_level = (LogLevel)value;
        }
        RegCloseKey(hKey);
    }
    if (g_level == LogLevel::Off) return;

    wchar_t base[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        g_level = LogLevel::Off;
        return;
    }
    std::wstring dir = std::wstring(base) + L"\\yCursory";
    CreateDirectoryW(dir.c_str(), NULL);
    dir += L"\\logs";
    CreateDirectoryW(dir.c_str(), NULL);
    g_path = dir + L"\\yCursory.log";
    g_oldPath = dir + L"\\yCursory.1.log";
}

bool Logger::Enabled(LogLevel level) {
    return level != LogLevel::Off && level <= g_level && !g_path.empty();
}

void Logger::Write(LogLevel level, const wchar_t* format, ...) {
    if (!Enabled(level)) return;

    wchar_t message[1024];
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(message, _countof(message), _TRUNCATE, format, args);
    va_end(args);

    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t line[1200];
    _snwprintf_s(line, _countof(line), _TRUNCATE, L"%04d-%02d-%02d %02d:%02d:%02d.%03d %-5s %s\n",
        t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, LevelName(level), message);

    char utf8[3600];
    int bytes = WideCharToMultiByte(CP_UTF8, 0, line, -1, utf8, sizeof(utf8), NULL, NULL);
    if (bytes <= 1) return;

    RotateIfLarge();
    // 書く頻度は低い前提なので、毎回開いて閉じる (異常終了しても行が残る)
    HANDLE file = CreateFileW(g_path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(file, utf8, (DWORD)(bytes - 1), &written, NULL);
    CloseHandle(file);
}
