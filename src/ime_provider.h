#pragma once
#include <windows.h>
#include "ime_monitor.h"

// IME (または IME の状態を知っている別のツール) から状態を知らせてもらう受け口。
// 知らせの形 (プロトコル v1) は README の「IME から状態を受け取る」を参照。
// 知らせを送ってくる IME が前面の窓を担当している間は、前面のアプリへの IME 状態の問い合わせをせず、
// 知らせだけでカーソルの色を変える。送ってくる IME がいなければ何もしない (従来どおり自分で調べる)。
namespace ImeProvider {
    struct Callbacks {
        // 担当中の窓で、オン・オフが知らされた
        void (*onOpenChanged)(ImeState state);
        // 担当中の窓で、キャレットの位置 (物理ピクセル) が知らされた。hasPosition=false は「位置なし」(変換の終わり等)
        void (*onCaret)(bool hasPosition, RECT physical);
    };

    bool Start(HINSTANCE hInstance, const Callbacks& callbacks);
    void Stop();

    // 前面の窓を、知らせを送ってきた IME が担当しているか
    bool CoversForeground();
    // 担当中で、オン・オフを知らされていれば true を返し state に入れる
    bool GetOpenState(ImeState* state);
    // 前面の窓が変わったときに呼ぶ。担当の窓と root が違えば担当を失効させる
    // (送り手が落ちた・別の IME に切り替えた等で「担当終了」が来ない場合に備える)
    void OnForegroundChanged(HWND foreground);
}
