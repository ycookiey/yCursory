#pragma once
#include <windows.h>
#include "ime_monitor.h"

// IMEや外部ツールから、オン・オフとキャレットの位置を知らせてもらう受け口
namespace ImeProvider {
    struct Callbacks {
        // 担当中のウィンドウで、オン・オフが知らされた
        void (*onOpenChanged)(ImeState state);
        // 担当中のウィンドウで、キャレットの位置（物理ピクセル）が知らされた。hasPosition=falseは「位置なし」（変換の終わり等）
        void (*onCaret)(bool hasPosition, RECT physical);
    };

    bool Start(HINSTANCE hInstance, const Callbacks& callbacks);
    void Stop();

    bool CoversForeground();
    bool GetOpenState(ImeState* state);
    // 送り手が落ちた・別のIMEに切り替えた等で「担当終了」が来ない場合に備えて、担当を失効させる
    void OnForegroundChanged(HWND eventWindow);
}
