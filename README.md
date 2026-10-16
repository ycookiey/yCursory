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

## 動く間と止まる間

IME 状態とキャレット位置の確認 (16ms ごと) は、前面の窓でキャレットが取れる間だけ動く。

- キャレットが 1 秒続けて取れなければ止まる (ゲーム、キャレットを持たないアプリ、デスクトップ等)。止まっている間は何もしない
- 前面の窓の切り替えかフォーカスの移動を OS から知らされると (SetWinEventHook) 再開する
- キャレットインジケーターを無効にしている場合は、再開の直後だけ入力先を探し、見つかれば以後は IME 状態だけを確認する
- 止まっている間に IME を切り替えても、次に窓かフォーカスが動くまでカーソルの色は変わらない。キャレットを取れないアプリ (端末等) ではこうなる。IME が状態を知らせてくる場合 (下の「IME から状態を受け取る」) は、止まっていても色が変わる

## IME から状態を受け取る (プロトコル v1)

IME (または IME の状態を知っている別のツール) が、オン・オフとキャレットの位置を yCursory に知らせられる。知らせを送る IME が前面の窓を担当している間は、yCursory は前面のアプリに IME 状態を問い合わせず、知らせだけでカーソルの色を変える。キャレットを取れないアプリ (端末等) でも、知らせがあれば色が追従する。送る IME がいなければ、従来どおり自分で調べる。

### 送り先

- `FindWindowExW(HWND_MESSAGE, NULL, L"yCursory.ImeStateReceiver.v1", NULL)` で見つかる窓へ `PostMessageW` で送る。`SendMessage` は使わない (yCursory を待たない)
- メッセージ番号は `RegisterWindowMessageW` に次の名前を渡して得る

| 名前 | wParam | lParam | 意味 |
|---|---|---|---|
| `yCursory.ImeState.v1.Focus` | 窓の HWND | 1: 担当開始 / 0: 担当終了 | この窓の入力を担当し始めた・やめた。HWND は子窓でもよい (root で比べる)。0 や存在しない窓は無視される |
| `yCursory.ImeState.v1.Open` | 1: オン (日本語入力) / 0: オフ | 0 | 担当中の窓のオン・オフ |
| `yCursory.ImeState.v1.Caret` | `MAKELONG(left, top)` | `MAKELONG(right, bottom)` | キャレット (変換中の文字列) の位置。物理ピクセル、各 16 ビット符号付き。wParam・lParam とも `0x80008000` なら「位置なし」 |

各値は 32 ビットに収まるので、32 ビットのプロセスからも送れる。

### 送る側の決まり

- 担当開始 (`Focus` の 1) を送った直後に、今のオン・オフを `Open` で送る (yCursory は担当開始の時点の状態を知らない)
- 変換が終わったら `Caret` の「位置なし」を送る (送らないと、最後の位置に光が残る)
- `Caret` は変わるたびに全部送らなくてよい。まとめて最後の 1 件だけでよい

### yCursory 側の扱い

- 担当は、前面の窓の root と担当の窓の root が同じ間だけ有効。前面が別の窓に変わると失効し、送る側が改めて担当開始を送るまで、従来どおり自分で調べる
- 自分でキャレットを取れている窓では、光の位置は自分で取った値を使う。`Caret` の位置を使うのは、自分では取れず止まっている間だけ
- 受け口は誰からの知らせも受け付ける (権限の低いプロセスからも届くよう許可している)。偽の知らせで変わるのは、カーソルの色と光の表示だけ

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
