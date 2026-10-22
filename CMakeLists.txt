cmake_minimum_required(VERSION 3.20)
project(yCursory VERSION 1.2.1 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# コンソールを持たないWindowsのGUIアプリ
add_executable(${PROJECT_NAME} WIN32
    src/main.cpp
    src/ime_monitor.cpp
    src/cursor_manager.cpp
    src/caret_indicator.cpp
    src/tray_icon.cpp
    src/startup_manager.cpp
    src/logger.cpp
    src/ime_provider.cpp
    resources/app.rc
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} src)
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)

target_link_libraries(${PROJECT_NAME} PRIVATE
    imm32
    gdiplus
    shell32
    user32
    gdi32
    advapi32
    ole32
    oleaut32
)

if(MSVC)
    # ソースはUTF-8で書いている
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /EHsc $<$<CONFIG:Release>:/O2 /GL>)
    target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>)
endif()
