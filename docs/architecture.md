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
    end
    
    subgraph WinAPI["Win32 API"]
        IMM["imm32.lib"]
        User["user32.lib"]
        Shell["shell32.lib"]
    end
    
    Main --> IME
    Main --> Cursor
    Main --> Tray
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
    
    loop 16msごと (前面の窓でキャレットが取れる間だけ)
        App->>IME: GetCurrentState()
        IME->>Win: ImmGetDefaultIMEWnd()
        IME->>Win: SendMessageTimeout(IMC_GETOPENSTATUS)
        IME-->>App: English / Japanese
        App->>Cursor: SetCursorForState()
        Cursor->>Win: SetSystemCursor() (状態が変わったときだけ)
    end

    Note over App: キャレットが1秒取れなければタイマーを止める
    Win->>App: EVENT_SYSTEM_FOREGROUND / EVENT_OBJECT_FOCUS (SetWinEventHook)
    Note over App: タイマーを再開
    
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
        RH[resource.h]
    end
    
    subgraph res["resources/"]
        EN[cursor_en.cur]
        JP[cursor_jp.cur]
        ICO[app.ico]
    end
    
    subgraph tools["Python Scripts"]
        GEN[generate_cursor.py]
        SYS[create_from_system_cursor.py]
    end
    
    subgraph build["build/"]
        EXE[ime-cursor-indicator.exe]
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
    F -->|Yes| G[SendMessage IMC_GETOPENSTATUS]
    G --> H{IME Open?}
    H -->|No| Z
    H -->|Yes| I[ImmGetConversionStatus]
    I --> J{CMODE_NATIVE?}
    J -->|Yes| Y[Japanese]
    J -->|No| Z
```

## カーソル生成フロー

```mermaid
flowchart LR
    A["aero_arrow.cur<br/>(Windows標準)"] --> B["read_cur_file()"]
    B --> C["PIL Image<br/>128x128"]
    C --> D["resize()<br/>32x32"]
    D --> E["add_glow()<br/>青/赤グロー"]
    E --> F["png_to_cur()"]
    F --> G["cursor_en.cur<br/>cursor_jp.cur"]
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
| EXE | 約120KB |
| メモリ | 約1.4MB |
| ポーリング | 16ms (キャレットが取れる窓が前面の間だけ。取れなければ 1 秒で止まり、窓・フォーカスの切り替えで再開) |
| カーソルサイズ | 32x32 |
