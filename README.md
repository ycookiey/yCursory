# yCursory - IME Cursor Indicator

Windows 11でマウスカーソル周囲にIME状態（英字/日本語）をグロー色で表示する軽量常駐ツール。

## 特徴

- **マウスカーソルグロー**
  - 英字モード: 青グロー
  - 日本語モード: 赤グロー
- **キャレット（テキストカーソル）インジケーター**
  - テキスト入力位置にもIME状態を表示
  - 60fps高速追従、遅延ゼロ
  - 複数アプリ対応（Notepad、Word、Excel、Vivaldi等）
- システムトレイ常駐
- 軽量（EXE: 約120KB, メモリ: 約1.4MB）

## ログ

`%LOCALAPPDATA%\yCursory\logs\yCursory.log` に、起動・終了、止まった・再開した、前面のアプリ、キャレットの取得方法 (GUIThreadInfo / IMM32 / UIAutomation / TSF / none) が変わったときだけ書く。1 MB を超えると `yCursory.1.log` に退避する。

レベルはレジストリ `HKCU\Software\yCursory` の `LogLevel` (DWORD) で変える: 0=なし 1=error 2=warn 3=info (既定) 4=debug (IME のオン・オフも書く)。

## Glow Style (グロースタイル)

シーンに合わせて選択できる2種類のスタイルを用意しました。

- **Heavy (右下強調)**: デフォルト。右下方向に強いグローを追加し、視認性を高めたスタイル。
- **Light (対称)**: 従来のデザイン。全方向に均等な柔らかいグロー。

## カーソル一覧

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

```powershell
# Visual Studio Build Tools が必要
.\build.bat
```

## 使い方

1. `build\ime-cursor-indicator.exe` を実行
2. システムトレイにアイコンが表示される
3. **グロースタイル切り替え**: トレイアイコンを右クリック → **Glow Style** → **Heavy / Light**
4. **キャレットインジケーター**: トレイアイコンを右クリック → **Caret Indicator** （チェックでON/OFF）
5. **スタートアップ登録**: トレイアイコンを右クリック → スタートアップに登録
6. **終了**: トレイアイコンを右クリック → Exit

## ファイル構成

```
yCursory/
├── src/                    # C++ ソースコード
├── resources/              # カーソル・アイコンファイル
├── docs/                   # ドキュメント
│   └── architecture.md        # アーキテクチャ図
├── build.bat               # ビルドスクリプト
└── create_from_system_cursor.py  # カーソル生成
```

## ドキュメント

- [アーキテクチャ](docs/architecture.md) - 設計図（mermaid）
