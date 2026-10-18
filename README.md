# yCursory

日本語IMEの入力モードをマウスカーソルの色で示すWindowsの常駐ツール。入力欄を見なくても、現在の入力モードが日本語か英字かを判別できる。

## 特徴

- マウスカーソルの周りに、色付きの光（グロー）を表示する。英字入力時は青、日本語入力時は赤
- 文字の入力位置（キャレット）にも同じ色のグローを表示する
- IMEや外部ツールから、IMEのオン・オフとキャレットの位置を受信できる。詳細は[プロトコルv1](docs/ime-state-protocol.md)を参照

## インストール

[Scoop](https://scoop.sh/)でインストールする。

```powershell
scoop bucket add yscoopy https://github.com/ycookiey/yscoopy
scoop install yscoopy/ycursory
```

または、[Releases](https://github.com/ycookiey/yCursory/releases)から`yCursory.exe`をダウンロードして起動する。

## 使い方

`yCursory.exe`を起動すると、タスクトレイにアイコンが表示される。アイコンを右クリックすると、次の項目のメニューが開く。

- Glow Style: グロースタイルをHeavyとLightから選択する
- Caret Indicator: キャレットのグローの表示を切り替える
- スタートアップに登録: サインイン時の自動起動を切り替える
- Exit: 終了する。カーソルは元に戻る

## 既知の制限

- キャレットの位置を取得できないアプリでは、IMEを切り替えても、次に前面のウィンドウかフォーカスが切り替わるまで、カーソルの色が変化しない
- オン・オフを取得できないIMEがある
- 動作確認はWindows 11のみ

## アンインストール

1. タスクトレイのメニューの「スタートアップに登録」のチェックを解除する
2. タスクトレイのメニューの「Exit」で終了する
3. `scoop uninstall ycursory`を実行する

アンインストール後も、設定はレジストリの`HKCU\Software\yCursory`に、ログは`%LOCALAPPDATA%\yCursory\logs\`に残る。不要なら削除する。

タスクマネージャーなどでyCursoryを強制終了してカーソルが元に戻らない場合は、yCursoryを再起動して「Exit」で終了するか、サインアウトして再度サインインする。

## グロースタイル

- Heavy: 既定。右下を強くした、目立つグロー
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

Visual StudioのC++のビルドツールとCMakeでビルドする。Visual Studioの「Developer PowerShell for VS」で次を実行する。出力先は`build\yCursory.exe`。

```powershell
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## ライセンス

[MIT License](LICENSE)
