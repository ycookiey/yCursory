@echo off
REM Build script for IME Cursor Indicator
REM Using Visual Studio 2019 Build Tools

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64
if errorlevel 1 (
    echo Failed to initialize Visual Studio environment!
    pause
    exit /b 1
)

cd /d "%~dp0"

REM Terminate running instance to avoid LNK1104
taskkill /F /IM yCursory.exe >nul 2>&1
timeout /t 1 /nobreak >nul

if not exist build mkdir build

REM Resource compile
echo Compiling resources...
rc /nologo /fo build\app.res resources\app.rc
if errorlevel 1 (
    echo Resource compilation failed!
    pause
    exit /b 1
)

REM C++ compile
echo Compiling C++ sources...
cl /nologo /c /O2 /EHsc /std:c++17 /DUNICODE /D_UNICODE /utf-8 ^
    /I. /Isrc ^
    /Fo:build\ ^
    src\main.cpp src\ime_monitor.cpp src\cursor_manager.cpp src\caret_indicator.cpp src\tray_icon.cpp src\startup_manager.cpp
if errorlevel 1 (
    echo Compilation failed!
    pause
    exit /b 1
)

REM Link
echo Linking...
link /nologo /OUT:build\yCursory.exe ^
    /SUBSYSTEM:WINDOWS ^
    build\main.obj build\ime_monitor.obj build\cursor_manager.obj build\caret_indicator.obj build\tray_icon.obj build\startup_manager.obj ^
    build\app.res ^
    imm32.lib gdiplus.lib shell32.lib user32.lib gdi32.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib
if errorlevel 1 (
    echo Linking failed!
    pause
    exit /b 1
)

echo.
echo Build successful: build\yCursory.exe
echo.
exit /b 0
