# yCursory - IME Cursor Indicator

Windows 11でマウスカーソル周囲にIME状態（英字/日本語）をグロー色で表示する軽量常駐ツール。

## 特徴

- **英字モード**: 青グロー
- **日本語モード**: 赤グロー
- システムトレイ常駐
- 軽量（EXE: 約120KB, メモリ: 約1.4MB）

## カーソルサンプル

| カーソル | 英字 (EN) | 日本語 (JP) |
|---------|-----------|-------------|
| Arrow | ![EN](docs/images/cursor_arrow_en.png) | ![JP](docs/images/cursor_arrow_jp.png) |
| IBeam | ![EN](docs/images/cursor_ibeam_en.png) | ![JP](docs/images/cursor_ibeam_jp.png) |
| Hand | ![EN](docs/images/cursor_hand_en.png) | ![JP](docs/images/cursor_hand_jp.png) |
| Wait | ![EN](docs/images/cursor_wait_en.png) | ![JP](docs/images/cursor_wait_jp.png) |
| Cross | ![EN](docs/images/cursor_cross_en.png) | ![JP](docs/images/cursor_cross_jp.png) |
| No | ![EN](docs/images/cursor_no_en.png) | ![JP](docs/images/cursor_no_jp.png) |
| SizeAll | ![EN](docs/images/cursor_sizeall_en.png) | ![JP](docs/images/cursor_sizeall_jp.png) |
| SizeNS | ![EN](docs/images/cursor_sizens_en.png) | ![JP](docs/images/cursor_sizens_jp.png) |
| SizeWE | ![EN](docs/images/cursor_sizewe_en.png) | ![JP](docs/images/cursor_sizewe_jp.png) |
| SizeNWSE | ![EN](docs/images/cursor_sizenwse_en.png) | ![JP](docs/images/cursor_sizenwse_jp.png) |
| SizeNESW | ![EN](docs/images/cursor_sizenesw_en.png) | ![JP](docs/images/cursor_sizenesw_jp.png) |
| UpArrow | ![EN](docs/images/cursor_uparrow_en.png) | ![JP](docs/images/cursor_uparrow_jp.png) |

## ビルド

```powershell
# Visual Studio Build Tools が必要
.\build.bat
```

## 使い方

1. `build\ime-cursor-indicator.exe` を実行
2. システムトレイにアイコンが表示される
3. IMEを切り替えるとカーソル周囲のグロー色が変化
4. スタートアップ登録: トレイアイコンを右クリック → スタートアップに登録
5. 終了: トレイアイコンを右クリック → Exit

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
