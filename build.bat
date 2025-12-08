@echo off
REM Build script for IME Cursor Indicator
REM Using Visual Studio 2019 Build Tools

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64

cd /d "%~dp0"

if not exist build mkdir build

REM Resource compile
echo Compiling resources...
rc /nologo /fo build\app.res resources\app.rc
if errorlevel 1 (
    echo Resource compilation failed!
    exit /b 1
)

REM C++ compile
echo Compiling C++ sources...
cl /nologo /c /O2 /EHsc /std:c++17 /DUNICODE /D_UNICODE /utf-8 ^
    /I. ^
    /Fo:build\ ^
    src\main.cpp src\ime_monitor.cpp src\cursor_manager.cpp src\tray_icon.cpp
if errorlevel 1 (
    echo Compilation failed!
    exit /b 1
)

REM Link
echo Linking...
link /nologo /OUT:build\ime-cursor-indicator.exe ^
    /SUBSYSTEM:WINDOWS ^
    build\main.obj build\ime_monitor.obj build\cursor_manager.obj build\tray_icon.obj ^
    build\app.res ^
    imm32.lib shell32.lib user32.lib gdi32.lib kernel32.lib
if errorlevel 1 (
    echo Linking failed!
    exit /b 1
)

echo.
echo Build successful: build\ime-cursor-indicator.exe
