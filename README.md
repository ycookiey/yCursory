# yCursory - IME Cursor Indicator

Windows 11でマウスカーソル周囲に、IMEの英字・日本語の状態をグロー色で表示する軽量常駐ツール。

## 特徴

- **マウスカーソルグロー**
  - 英字モード: 青グロー
  - 日本語モード: 赤グロー
- **キャレットインジケーター**
  - テキスト入力位置（キャレット）にもIME状態を表示
  - 60fps高速追従、遅延ゼロ
  - メモ帳、Word、Excel、Vivaldiなど複数のアプリで動く
- IMEや外部ツールからIME状態とキャレット位置を受け取れる。仕様は[IME状態の受け取り](docs/ime-state-protocol.md)
- システムトレイ常駐
- 軽量。EXEは約380KB

## インストール

[Releases](https://github.com/ycookiey/yCursory/releases)から`yCursory.exe`を取得するか、Scoopで入れる。

```powershell
scoop bucket add yscoopy https://github.com/ycookiey/yscoopy
scoop install yscoopy/ycursory
```

## 使い方

1. `yCursory.exe`を実行する。ビルドした場合は`build\yCursory.exe`を実行する
2. システムトレイにアイコンが表示される
3. **グロースタイル切り替え**: トレイアイコンを右クリック → **Glow Style** → **Heavy / Light**
4. **キャレットインジケーター**: トレイアイコンを右クリック → **Caret Indicator**。チェックを付けると有効、外すと無効になる
5. **スタートアップ登録**: トレイアイコンを右クリック → スタートアップに登録
6. **終了**: トレイアイコンを右クリック → Exit

## ログ

`%LOCALAPPDATA%\yCursory\logs\yCursory.log`に、起動・終了、確認の停止・再開、前面のアプリ、キャレットの取得方法、IMEからの知らせの担当開始・終了、エラーを、変わったときだけ書く。キャレットの取得方法は、GUIThreadInfo、IMM32、UIAutomation、TSF、noneのいずれかで書く。1MBを超えると`yCursory.1.log`に退避する。

レベルはレジストリ`HKCU\Software\yCursory`のDWORD値`LogLevel`で変える: 0=なし、1=error、2=warn、3=info、4=debug。既定は3。debugでは、IMEのオン・オフの変化と、IMEから受けたオン・オフの知らせも書く。

## 既知の制限

- ゲームや端末など、キャレットを取れないアプリが前面にある間は、負荷を抑えるためIME状態の確認を止める。止まっている間にIMEを切り替えても、次にウィンドウかフォーカスが切り替わるまでカーソルの色は変わらない。IMEが[IME状態の受け取り](docs/ime-state-protocol.md)の方法で状態を送る場合は、止まっていても色が変わる。

## Glow Style（グロースタイル）

シーンに合わせて選べる2種類のスタイルがある。

- **Heavy**: 既定。右下方向に強いグローを加え、視認性を高めたスタイル。
- **Light**: 全方向に均等な柔らかいグロー。

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
# Visual Studio 2019 Build Toolsが必要
.\build.bat
```

## ファイル構成

```
yCursory/
├── src/                    # C++ソースコード
├── resources/              # カーソル・アイコンファイル
├── docs/                   # ドキュメント
│   ├── ime-state-protocol.md  # IME状態の受け取り
│   └── architecture.md        # アーキテクチャ図
├── build.bat               # ビルドスクリプト
└── create_from_system_cursor.py  # カーソル生成
```

## ドキュメント

- [アーキテクチャ](docs/architecture.md) - mermaidの設計図
- [IME状態の受け取り](docs/ime-state-protocol.md) - プロトコルv1で、IMEや外部ツールからyCursoryへ状態を送る方法

## ライセンス

[MIT License](LICENSE)
