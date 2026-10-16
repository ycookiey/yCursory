#pragma once
#include <windows.h>
#include "ime_monitor.h"

// IMEや外部ツールから、オン・オフとキャレットの位置を知らせてもらう受け口
namespace ImeProvider {
    struct Callbacks {
        // 担当ウィンドウで、オン・オフが知らされた
        void (*onOpenChanged)(ImeState state);
        // 担当ウィンドウで、キャレットの物理ピクセルの位置が知らされた。変換が終わったときなど、位置なしが知らされたらhasPosition=false
        void (*onCaret)(bool hasPosition, RECT physical);
    };

    bool Start(HINSTANCE hInstance, const Callbacks& callbacks);
    void Stop();

    bool CoversForeground();
    bool GetOpenState(ImeState* state);
    // 送る側が落ちた・別のIMEに切り替えた等で担当終了が来ない場合に備えて、yCursoryから担当を終える
    void OnForegroundChanged(HWND eventWindow);
}
