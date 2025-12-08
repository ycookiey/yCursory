# yCursory - IME Cursor Indicator

Windows 11でマウスカーソル周囲にIME状態（英字/日本語）をグロー色で表示する軽量常駐ツール。

## 特徴

- **英字モード**: 青グロー
- **日本語モード**: 赤グロー
- システムトレイ常駐
- 軽量（EXE: 約120KB, メモリ: 約1.4MB）

## ビルド

```powershell
# Visual Studio Build Tools が必要
.\build.bat
```

## 使い方

1. `build\ime-cursor-indicator.exe` を実行
2. システムトレイにアイコンが表示される
3. IMEを切り替えるとカーソル周囲のグロー色が変化
4. 終了: トレイアイコンを右クリック → Exit

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
