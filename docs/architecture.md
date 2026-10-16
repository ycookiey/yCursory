# yCursory - プロジェクト概要

> IME状態をカーソルのグロー色で表示する軽量Windowsツール

## アーキテクチャ

```mermaid
flowchart TB
    subgraph App["yCursory"]
        Main["main.cpp<br/>メッセージループ"]
        IME["ImeMonitor<br/>IME状態検出"]
        Cursor["CursorManager<br/>カーソル制御"]
        Tray["TrayIcon<br/>システムトレイ"]
        Caret["CaretIndicator<br/>キャレットインジケーター"]
        Provider["ImeProvider<br/>IMEからの知らせ"]
        Log["Logger<br/>ログ"]
    end
    
    subgraph WinAPI["Win32 API"]
        IMM["imm32.lib"]
        User["user32.lib"]
        Shell["shell32.lib"]
    end
    
    Main --> IME
    Main --> Cursor
    Main --> Tray
    Main --> Caret
    Main --> Provider
    Main --> Log
    IME --> IMM
    Cursor --> User
    Tray --> Shell
```

## 動作フロー

```mermaid
sequenceDiagram
    participant User as ユーザー
    participant App as アプリ
    participant IME as ImeMonitor
    participant Cursor as CursorManager
    participant Win as Windows
    
    App->>Cursor: Initialize()
    Cursor->>Win: LoadCursor()
    App->>Cursor: SetCursorForState(初期, force=true)
    
    loop 前面のウィンドウでキャレットが取れる間、16msごと
        alt 担当中で、IMEから知らされている
            App->>App: ImeProvider::GetOpenState()
        else 知らされていない
            App->>IME: GetCurrentState()
            IME->>Win: ImmGetDefaultIMEWnd()
            IME->>Win: SendMessageTimeout(IMC_GETOPENSTATUS)
            IME-->>App: English / Japanese
        end
        App->>Cursor: SetCursorForState()
        Cursor->>Win: 状態が変わったときだけSetSystemCursor()
    end

    Note over App: キャレットが1秒取れなければタイマーを止める
    Win->>App: EVENT_SYSTEM_FOREGROUND / EVENT_OBJECT_FOCUS (SetWinEventHook)
    Note over App: タイマーを再開
    Note over App: キャレットインジケーターが無効のときは、再開の直後だけ入力先を探し、見つかれば以後はIME状態だけを確認する
    Note over App: IMEからのOpenの知らせは、タイマーが止まっていてもすぐ色に反映する
    
    User->>App: トレイから終了
    App->>Cursor: RestoreOriginalCursor()
    Cursor->>Win: SystemParametersInfo(SPI_SETCURSORS)
```

## ファイル構成

```mermaid
graph LR
    subgraph src["src/"]
        M[main.cpp]
        IM[ime_monitor.cpp/h]
        CM[cursor_manager.cpp/h]
        TI[tray_icon.cpp/h]
        CI[caret_indicator.cpp/h]
        IP[ime_provider.cpp/h]
        LG[logger.cpp/h]
        SM[startup_manager.cpp/h]
        RH[resource.h]
    end
    
    subgraph res["resources/"]
        EN["cursor_*_en_*.cur"]
        JP["cursor_*_jp_*.cur"]
        ICO[app.ico]
        RC[app.rc]
    end
    
    subgraph tools["Python Scripts"]
        GEN[create_icon.py]
        SYS[create_from_system_cursor.py]
    end
    
    subgraph build["build/"]
        EXE[yCursory.exe]
    end
    
    src --> build
    res --> build
    tools --> res
```

## IME状態検出ロジック

```mermaid
flowchart TD
    A[GetForegroundWindow] --> B{hwnd != NULL?}
    B -->|No| Z[English]
    B -->|Yes| C[GetKeyboardLayout]
    C --> D{日本語レイアウト?}
    D -->|No| Z
    D -->|Yes| E[ImmGetDefaultIMEWnd]
    E --> F{imeWnd != NULL?}
    F -->|No| Z
    F -->|Yes| G[SendMessageTimeout IMC_GETOPENSTATUS 50ms]
    G -->|応答なし| P[前回の状態]
    G -->|応答あり| H{IME Open?}
    H -->|No| Z
    H -->|Yes| I[ImmGetConversionStatus]
    I --> J{CMODE_NATIVE?}
    J -->|Yes| Y[Japanese]
    J -->|No| Z
```

## カーソル生成フロー

```mermaid
flowchart LR
    A["Windows標準の<br/>aero_arrow.cur"] --> B["read_cur_file()"]
    B --> C["PIL Image<br/>128x128"]
    C --> D["resize()<br/>32x32"]
    D --> E["add_glow()<br/>青/赤グロー"]
    E --> F["png_to_cur()"]
    F --> G["cursor_*_en_*.cur<br/>cursor_*_jp_*.cur"]
```

## 主要API

```mermaid
mindmap
  root((Win32 API))
    IME
      ImmGetContext
      ImmGetConversionStatus
      ImmGetDefaultIMEWnd
    Cursor
      LoadCursor
      SetSystemCursor
      CopyCursor
      SystemParametersInfo
    Window
      GetForegroundWindow
      GetKeyboardLayout
      SetTimer
      SetWinEventHook
    Tray
      Shell_NotifyIcon
      TrackPopupMenu
```

## スペック

| 項目 | 値 |
|------|-----|
| EXE | 約380KB |
| ポーリング | 16ms。キャレットが取れるウィンドウが前面にある間だけ動く。キャレットが1秒取れなければ止まり、ウィンドウかフォーカスの切り替えで再開する |
| カーソルサイズ | 32x32 |
