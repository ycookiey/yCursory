cmake_minimum_required(VERSION 3.20)
project(ime-cursor-indicator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows GUI アプリケーション (コンソール非表示)
add_executable(${PROJECT_NAME} WIN32
    src/main.cpp
    src/ime_monitor.cpp
    src/ime_monitor.h
    src/cursor_manager.cpp
    src/cursor_manager.h
    src/tray_icon.cpp
    src/tray_icon.h
    src/startup_manager.cpp
    src/startup_manager.h
    src/resource.h
    resources/app.rc
)

# 必要なライブラリ
target_link_libraries(${PROJECT_NAME} PRIVATE
    imm32      # IME API
    shell32    # シェル・トレイアイコン
    user32     # ウィンドウ・カーソルAPI
    gdi32      # 描画
)

# リリースビルド最適化
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /O2 /GL)
    target_link_options(${PROJECT_NAME} PRIVATE /LTCG /OPT:REF /OPT:ICF)
endif()
