# yCursory - IME Cursor Indicator

WindowsでIMEの英字・日本語の状態を、マウスカーソルの色で判別するための軽量な常駐ツール。日本語IMEで入力する人が、今の入力が日本語か英字かを、入力欄を見なくても分かるようにする。

## 特徴

- **マウスカーソルグロー**
  - 英字モード: 青グロー
  - 日本語モード: 赤グロー
- **キャレットインジケーター**
  - テキスト入力位置（キャレット）にもIME状態を表示
  - 16msごとに位置を読んで追従する
  - キャレットの位置は、Windowsが管理するキャレットか、UI Automationから取る。メモ帳のようにWindowsが管理するキャレットを使うアプリと、ChromiumやElectronで作られたアプリで動く
- IMEや外部ツールからIME状態とキャレット位置を受け取れる。仕様は[IME状態の受け取り](docs/ime-state-protocol.md)
- システムトレイ常駐
- 軽量。EXEは約380KB

## 動作環境

- Windows 11で動作を確認した。ほかの版は未確認
- 日本語の入力言語で使う。前面のアプリの入力言語が日本語でないときは、常に英字として表示する

### IME状態の読み方

- 前面のアプリの既定のIMEウィンドウに、IMEのオン・オフを問い合わせる（`WM_IME_CONTROL`の`IMC_GETOPENSTATUS`）。オンのときは、変換モードを`ImmGetConversionStatus`で読む。読めて日本語入力なら日本語、読めて日本語入力でなければ英字、読めなければ日本語とする。50ms以内に応答がなければ、前回の状態を保つ
- IMEが状態をTSFだけに持つ場合、IMEのオン・オフの問い合わせが状態を反映しないことがある。状態をTSFだけに持つIMEをメモ帳で使ったとき、反映されなかった例を1つ確認した。TSFだけに状態を持つIMEは、[プロトコルv1](docs/ime-state-protocol.md)で状態をyCursoryに送れる。送る側の実装はIMEが用意する

### キャレットの取得方法

次の順に試し、最初に取れた方法を使う。括弧の中は、ログに書かれる名前。

1. Windowsが管理するキャレット（`GUIThreadInfo`）
2. IMEの変換ウィンドウの位置（`IMM32`）
3. UI Automation（`UIAutomation`）。ChromiumやElectronで作られたアプリが当てはまる
4. TSF（`TSF`）。yCursory自身のスレッドのキャレットだけ取れるので、通常は使われない

どれでも取れないアプリが前面にある間は、確認を止める。詳しくは[既知の制限](#既知の制限)。

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
6. **終了**: トレイアイコンを右クリック → Exit。カーソルは元に戻る

## アンインストール

1. トレイアイコンを右クリックし、スタートアップに登録のチェックを外す。チェックが付いている間は、レジストリ`HKCU\Software\Microsoft\Windows\CurrentVersion\Run`に、値`yCursory`が登録されている。チェックを外すと、値`yCursory`を削除する。チェックを付けたまま`yCursory.exe`を削除してしまった場合は、次のコマンドで値を削除する

```powershell
reg delete HKCU\Software\Microsoft\Windows\CurrentVersion\Run /v yCursory /f
```
2. トレイアイコンを右クリック → Exitで終了する。終了するとき、カーソルを元に戻す
3. Scoopで入れた場合は、次を実行する。Releasesから取得した場合は、`yCursory.exe`を削除する

```powershell
scoop uninstall ycursory
```

アンインストールしても、次のデータは残る。完全に消すときは、削除する。

- ログのフォルダ`%LOCALAPPDATA%\yCursory`
- 設定を保存したレジストリ`HKCU\Software\yCursory`

```powershell
Remove-Item -Recurse "$env:LOCALAPPDATA\yCursory"
reg delete HKCU\Software\yCursory /f
```

## 困ったとき

### カーソルがグロー色のまま元に戻らない

異常終了や強制終了をすると、カーソルが変えられたまま残ることがある。カーソルの変更はWindowsのメモリ上だけにあり、レジストリには書かれず、サインアウトすると消える（[SetSystemCursor](https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setsystemcursor)）。次のどちらかで戻る。

- `yCursory.exe`を起動し直し、トレイアイコンのExitで終了する。終了するとき、現在のポインターの設定を読み直す（`SystemParametersInfo`の`SPI_SETCURSORS`）
- Windowsからサインアウトして、もう一度サインインする

### カーソルの色が変わらない

- キャレットを取れないアプリが前面にある間は、確認を止めている。[既知の制限](#既知の制限)を参照する
- 前面のアプリの入力言語が日本語でないと、常に英字として表示する。[動作環境](#動作環境)を参照する
- 状態をTSFだけに持つIMEでは、IMEのオン・オフの問い合わせが状態を反映しないことがある。[IME状態の読み方](#ime状態の読み方)を参照する
- ログで、前面のアプリと、キャレットの取得方法を確認する

### ログ

`%LOCALAPPDATA%\yCursory\logs\yCursory.log`に、起動・終了、確認の停止・再開、前面のアプリ、キャレットの取得方法、IMEからの知らせの担当開始・終了、エラーを、変わったときだけ書く。キャレットの取得方法は、GUIThreadInfo、IMM32、UIAutomation、TSF、noneのいずれかで書く。noneは、キャレットを取れなかったことを表す。1MBを超えると`yCursory.1.log`に退避する。

記録の量は、レジストリ`HKCU\Software\yCursory`のDWORD値`LogLevel`で変える: 0=なし、1=error、2=warn、3=info、4=debug。既定は3。debugでは、IMEのオン・オフの変化と、IMEから受けたオン・オフの知らせも書く。値を変えたら、yCursoryを終了して、起動し直す。debugにするコマンドは次のとおり。

```powershell
reg add HKCU\Software\yCursory /v LogLevel /t REG_DWORD /d 4 /f
```

## 既知の制限

- Windowsが管理するキャレットを使わず、UI Automationでもキャレットの位置を知らせないアプリが前面にある間は、負荷を抑えるためIME状態の確認を止める。ゲームや端末の中に、当てはまるアプリがある。止まっている間にIMEを切り替えても、次にウィンドウかフォーカスが切り替わるまでカーソルの色は変わらない。IMEが[IME状態の受け取り](docs/ime-state-protocol.md)の方法で状態を送る場合は、止まっていても色が変わる。ログに、そのアプリの`foreground:`の行、`caret: none`の行、`pause: no caret`の行が続くアプリは、当てはまる。

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

Visual StudioのC++のビルドツールとCMakeでビルドする。Visual Studioの「Developer PowerShell for VS」で次を実行すると、`build\yCursory.exe`ができる。

```powershell
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

`build.bat`は、Visual Studio 2019 Build Toolsを既定の場所にインストールした環境で使える。

## ファイル構成

```
yCursory/
├── src/                    # C++ソースコード
├── resources/              # カーソル・アイコンファイル
├── docs/                   # ドキュメント
│   ├── ime-state-protocol.md  # IME状態の受け取り
│   └── architecture.md        # アーキテクチャ図
├── CMakeLists.txt          # CMakeのビルド定義
├── build.bat               # ビルドスクリプト
└── create_from_system_cursor.py  # カーソル生成
```

## ドキュメント

- [アーキテクチャ](docs/architecture.md) - mermaidの設計図
- [IME状態の受け取り](docs/ime-state-protocol.md) - プロトコルv1で、IMEや外部ツールからyCursoryへ状態を送る方法

## ライセンス

[MIT License](LICENSE)
