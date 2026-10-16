# yCursory

日本語IMEの入力モードを、マウスカーソルの色で示すWindowsの常駐ツール。入力欄を見なくても、今の入力が日本語か英字かが分かる。

## 特徴

- 英字のときは青、日本語のときは赤のグローを、マウスカーソルに付ける
- 文字の入力位置（キャレット）にも、同じ色のグローを出す
- グローの形を、HeavyとLightから選べる
- IMEからIMEの状態を受け取れる。送る形は[プロトコルv1](docs/ime-state-protocol.md)に書いた

## インストール

[Scoop](https://scoop.sh/)で入れる。[Releases](https://github.com/ycookiey/yCursory/releases)から`yCursory.exe`を取得してもよい。

```powershell
scoop bucket add yscoopy https://github.com/ycookiey/yscoopy
scoop install yscoopy/ycursory
```

## 使い方

`yCursory.exe`を起動すると、タスクトレイにアイコンが出る。アイコンを右クリックして、次を選ぶ。

- Glow Style: グローの形をHeavyかLightにする
- Caret Indicator: キャレットのグローを出すかを切り替える
- スタートアップに登録: サインインしたときに自動で起動するかを切り替える
- Exit: 終了する。カーソルは元に戻る

## 既知の制限

- キャレットの位置を取れないアプリでは、IMEを切り替えても、次に前面のウィンドウかフォーカスが切り替わるまで、カーソルの色が変わらない
- IMEのオン・オフを読み取れないIMEがある。プロトコルv1でIMEの状態を送るIMEなら、カーソルの色が変わる
- 動作を確認したのはWindows 11だけ

## アンインストール

1. トレイのメニューの「スタートアップに登録」のチェックを外す
2. トレイのメニューのExitで終了する
3. `scoop uninstall ycursory`を実行する

設定はレジストリの`HKCU\Software\yCursory`に、ログは`%LOCALAPPDATA%\yCursory\logs\`に残る。不要なら削除する。終了せずにyCursoryを止めてカーソルが元に戻らないときは、yCursoryを起動し直してExitで終了するか、サインアウトしてサインインし直す。

## グロースタイル

- Heavy: 既定。右下に強いグローを加え、カーソルを見つけやすくする
- Light: 全方向に均等な、柔らかいグロー

| Cursor | Light (EN) | Light (JP) | Heavy (EN) | Heavy (JP) |
|---|:-:|:-:|:-:|:-:|
| **Arrow** | ![L-EN](docs/images/cursor_arrow_en_light.png) | ![L-JP](docs/images/cursor_arrow_jp_light.png) | ![H-EN](docs/images/cursor_arrow_en_heavy.png) | ![H-JP](docs/images/cursor_arrow_jp_heavy.png) |
| **IBeam** | ![L-EN](docs/images/cursor_ibeam_en_light.png) | ![L-JP](docs/images/cursor_ibeam_jp_light.png) | ![H-EN](docs/images/cursor_ibeam_en_heavy.png) | ![H-JP](docs/images/cursor_ibeam_jp_heavy.png) |
| **Hand** | ![L-EN](docs/images/cursor_hand_en_light.png) | ![L-JP](docs/images/cursor_hand_jp_light.png) | ![H-EN](docs/images/cursor_hand_en_heavy.png) | ![H-JP](docs/images/cursor_hand_jp_heavy.png) |
| **Wait** | ![L-EN](docs/images/cursor_wait_en_light.png) | ![L-JP](docs/images/cursor_wait_jp_light.png) | ![H-EN](docs/images/cursor_wait_en_heavy.png) | ![H-JP](docs/images/cursor_wait_jp_heavy.png) |
| **Cross** | ![L-EN](docs/images/cursor_cross_en_light.png) | ![L-JP](docs/images/cursor_cross_jp_light.png) | ![H-EN](docs/images/cursor_cross_en_heavy.png) | ![H-JP](docs/images/cursor_cross_jp_heavy.png) |
| **No** | ![L-EN](docs/images/cursor_no_en_light.png) | ![L-JP](docs/images/cursor_no_jp_light.png) | ![H-EN](docs/images/cursor_no_en_heavy.png) | ![H-JP](docs/images/cursor_no_jp_heavy.png) |
| **SizeAll** | ![L-EN](docs/images/cursor_sizeall_en_light.png) | ![L-JP](docs/images/cursor_sizeall_jp_light.png) | ![H-EN](docs/images/cursor_sizeall_en_heavy.png) | ![H-JP](docs/images/cursor_sizeall_jp_heavy.png) |
| **SizeNS** | ![L-EN](docs/images/cursor_sizens_en_light.png) | ![L-JP](docs/images/cursor_sizens_jp_light.png) | ![H-EN](docs/images/cursor_sizens_en_heavy.png) | ![H-JP](docs/images/cursor_sizens_jp_heavy.png) |
| **SizeWE** | ![L-EN](docs/images/cursor_sizewe_en_light.png) | ![L-JP](docs/images/cursor_sizewe_jp_light.png) | ![H-EN](docs/images/cursor_sizewe_en_heavy.png) | ![H-JP](docs/images/cursor_sizewe_jp_heavy.png) |
| **SizeNWSE** | ![L-EN](docs/images/cursor_sizenwse_en_light.png) | ![L-JP](docs/images/cursor_sizenwse_jp_light.png) | ![H-EN](docs/images/cursor_sizenwse_en_heavy.png) | ![H-JP](docs/images/cursor_sizenwse_jp_heavy.png) |
| **SizeNESW** | ![L-EN](docs/images/cursor_sizenesw_en_light.png) | ![L-JP](docs/images/cursor_sizenesw_jp_light.png) | ![H-EN](docs/images/cursor_sizenesw_en_heavy.png) | ![H-JP](docs/images/cursor_sizenesw_jp_heavy.png) |
| **UpArrow** | ![L-EN](docs/images/cursor_uparrow_en_light.png) | ![L-JP](docs/images/cursor_uparrow_jp_light.png) | ![H-EN](docs/images/cursor_uparrow_en_heavy.png) | ![H-JP](docs/images/cursor_uparrow_jp_heavy.png) |

## ビルド

Visual StudioのC++のビルドツールとCMakeでビルドする。Visual Studioの「Developer PowerShell for VS」で次を実行すると、`build\yCursory.exe`ができる。

```powershell
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## ライセンス

[MIT License](LICENSE)
