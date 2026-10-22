# yCursory

日本語IMEの入力モードをマウスカーソルの色で示すWindowsの常駐ツール。入力欄を見なくても、現在の入力モードが日本語か英字かを判別できる。

## 特徴

- マウスカーソルの周りに、色付きの光（グロー）を表示する。英字入力時は青、日本語入力時は赤
- 文字の入力位置（キャレット）にも同じ色のグローを表示する
- 状態を送信するIMEや外部ツールを使うと、キャレットの位置を取得できないアプリでも、IMEのオン・オフを切り替えた時点でカーソルの色が変わる。詳細は[プロトコルv1](docs/ime-state-protocol.md)を参照

## インストール

[Scoop](https://scoop.sh/)でインストールする。

```powershell
scoop bucket add yscoopy https://github.com/ycookiey/yscoopy
scoop install yscoopy/ycursory
```

または、[Releases](https://github.com/ycookiey/yCursory/releases)から`yCursory.exe`をダウンロードして起動する。

## 使い方

`yCursory.exe`を起動すると、タスクトレイにアイコンが表示される。アイコンを右クリックすると、次のメニューが開く。

- スタートアップに登録: チェックを付けると、サインイン時にyCursoryが自動で起動する
- Glow Style: グローの見た目をHeavyかLightに切り替える。見た目の違いは[グロースタイル](#グロースタイル)を参照
- Caret Indicator: チェックを付けると、キャレットにもグローを表示する。既定はチェックあり
- Exit: 終了する。カーソルは元に戻る

## 既知の制限

- キャレットの位置を取得できないアプリでは、入力モードを切り替えても、別のウィンドウに切り替えるか、フォーカスがほかの入力欄などに移るまで、カーソルの色が変わらない
- IMEによっては、yCursoryがオン・オフを取得できず、カーソルの色が入力モードと合わないことがある
- 動作確認はWindows 11のみ

## アンインストール

1. タスクトレイのメニューの「スタートアップに登録」のチェックを外す
2. タスクトレイのメニューの「Exit」で終了する
3. Scoopでインストールした場合は`scoop uninstall ycursory`を実行する。Releasesからダウンロードした場合は`yCursory.exe`を削除する

アンインストール後も、設定はレジストリの`HKCU\Software\yCursory`に、ログは`%LOCALAPPDATA%\yCursory\logs\`に残る。不要なら、レジストリのキーとログのフォルダを削除する。

タスクマネージャーなどでyCursoryを強制終了すると、カーソルはグローが付いたまま残る。元に戻すには、yCursoryを再起動して「Exit」で終了するか、サインアウトして再度サインインする。

## グロースタイル

- Heavy: 既定。グローを右下の方向に強く出し、目立たせる
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
