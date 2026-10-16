#include "caret_indicator.h"
#include <imm.h>
#include <cmath>
#pragma comment(lib, "imm32.lib")

using namespace Gdiplus;

// TSFのEditSessionを実行する補助クラス
class TsfCaretEditSession : public ITfEditSession {
private:
    LONG m_refCount;
    ITfContext* m_context;
    ITfContextView* m_view;
    RECT* m_result;
    bool* m_success;

public:
    TsfCaretEditSession(ITfContext* context, ITfContextView* view, RECT* result, bool* success)
        : m_refCount(1), m_context(context), m_view(view), m_result(result), m_success(success) {
        m_context->AddRef();
        m_view->AddRef();
        *m_success = false;
    }

    virtual ~TsfCaretEditSession() {
        if (m_context) m_context->Release();
        if (m_view) m_view->Release();
    }

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) {
        if (ppvObj == NULL) return E_INVALIDARG;
        *ppvObj = NULL;

        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession)) {
            *ppvObj = (ITfEditSession*)this;
        }

        if (*ppvObj) {
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() {
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG) Release() {
        LONG count = InterlockedDecrement(&m_refCount);
        if (count == 0) {
            delete this;
        }
        return count;
    }

    // ITfEditSession
    STDMETHODIMP DoEditSession(TfEditCookie ec) {
        TF_SELECTION sel;
        ULONG fetched = 0;

        HRESULT hr = m_context->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &sel, &fetched);
        if (FAILED(hr) || fetched == 0) {
            return hr;
        }

        RECT rect;
        BOOL clipped;
        hr = m_view->GetTextExt(ec, sel.range, &rect, &clipped);

        if (SUCCEEDED(hr)) {
            *m_result = rect;
            *m_success = true;
        }

        sel.range->Release();
        return hr;
    }
};

// グロー設定
static const int GLOW_WIDTH = 25;          // グロー幅（細く）
static const int GLOW_HEIGHT = 30;         // グロー高さ（キャレットサイズ）
static const int UPDATE_THRESHOLD = 1;     // 更新閾値（ピクセル）- 小さな動きも検出
static const DWORD UPDATE_INTERVAL = 16;   // 最小更新間隔（ミリ秒）- 約60fps

CaretIndicator::CaretIndicator()
    : m_hwnd(NULL)
    , m_hInstance(NULL)
    , m_currentState(ImeState::English)
    , m_enabled(false)
    , m_visible(false)
    , m_lastCaretPos({0, 0})
    , m_currentDisplayPos({0, 0})
    , m_lastCaretWindow(NULL)
    , m_lastUpdateTick(0)
    , m_rendered(false)
    , m_renderedPos({0, 0})
    , m_renderedState(ImeState::English)
    , m_threadMgr(NULL)
    , m_clientId(TF_CLIENTID_NULL)
    , m_uiAutomation(NULL)
    , m_gdiplusToken(0)
    , m_backBuffer(NULL)
    , m_lastMethod(METHOD_NONE) {
}

CaretIndicator::~CaretIndicator() {
    Cleanup();
}

bool CaretIndicator::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    // GDI+ 初期化
    GdiplusStartupInput gdiplusStartupInput;
    if (GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL) != Ok) {
        return false;
    }

    // TSF初期化
    CoInitialize(NULL);
    HRESULT hr = CoCreateInstance(
        CLSID_TF_ThreadMgr,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITfThreadMgr,
        (void**)&m_threadMgr
    );

    if (SUCCEEDED(hr) && m_threadMgr != NULL) {
        // クライアントIDを取得
        hr = m_threadMgr->Activate(&m_clientId);
        if (FAILED(hr)) {
            m_threadMgr->Release();
            m_threadMgr = NULL;
            m_clientId = TF_CLIENTID_NULL;
        }
    } else {
        m_threadMgr = NULL;
        m_clientId = TF_CLIENTID_NULL;
        // TSF失敗しても続行（他の方法でフォールバック）
    }

    // UI Automation初期化
    hr = CoCreateInstance(
        CLSID_CUIAutomation,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IUIAutomation,
        (void**)&m_uiAutomation
    );

    if (FAILED(hr)) {
        m_uiAutomation = NULL;
        // UI Automation失敗しても続行（他の方法でフォールバック）
    }

    // レイヤードウィンドウ作成
    if (!CreateIndicatorWindow()) {
        Cleanup();
        return false;
    }

    // バックバッファ作成
    m_backBuffer = new Bitmap(GLOW_WIDTH, GLOW_HEIGHT, PixelFormat32bppARGB);

    return true;
}

bool CaretIndicator::CreateIndicatorWindow() {
    // ウィンドウクラス登録
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = L"CaretIndicatorWindow";
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);

    RegisterClassExW(&wc);

    // レイヤードウィンドウ作成
    m_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        L"CaretIndicatorWindow",
        L"Caret Indicator",
        WS_POPUP,
        0, 0, GLOW_WIDTH, GLOW_HEIGHT,
        NULL, NULL, m_hInstance, NULL
    );

    if (m_hwnd == NULL) {
        return false;
    }

    // スクリーンショットに写らないように設定 (WDA_EXCLUDEFROMCAPTURE = 0x00000011)
    // 定数が未定義の場合に備えて直接値を使用するか、定義を確認するが、
    // ここでは安全のため定数がなければ定義するようにコードを追加するアプローチではなく、
    // 既存のSDKで定義されていることを期待しつつ、マジックナンバーは避けたいが
    // ユーザー環境のSDKバージョンが不明なため、念の為コメントで値を記載し、定数を使う。
    // もしコンパイルエラーになるなら定数を定義する必要がある。
    // まずは定数を使ってみる。
    const DWORD WDA_EXCLUDEFROMCAPTURE_VAL = 0x00000011;
    SetWindowDisplayAffinity(m_hwnd, WDA_EXCLUDEFROMCAPTURE_VAL);

    // ウィンドウにthisポインタを関連付け
    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

    return true;
}

void CaretIndicator::SetEnabled(bool enabled) {
    m_enabled = enabled;
    if (!enabled) {
        Hide();
    }
}

void CaretIndicator::Update(ImeState state) {
    // 後方互換性のため、UpdatePosition + UpdateDisplayを呼ぶ
    UpdatePosition(state);
    UpdateDisplay();
}

bool CaretIndicator::UpdatePosition(ImeState state) {
    if (!m_enabled) {
        return false;
    }

    // IME状態が変わった場合は強制更新
    bool stateChanged = (m_currentState != state);
    m_currentState = state;

    // キャレット位置取得
    POINT caretPos;
    HWND caretWnd;
    bool positionFound = GetCaretPosition(&caretPos, &caretWnd);

    if (!positionFound) {
        // 位置取得失敗時は、前回の位置を保持
        if (m_lastCaretWindow == NULL) {
            // 初回の取得失敗時のみ非表示フラグを立てる
            return false;
        }
        // 前回の位置を使用（色のみ更新される）
        return false;
    }

    // 更新が必要かチェック（IME状態変更時は強制更新）
    if (!stateChanged && !ShouldUpdate(caretPos, caretWnd)) {
        return true;
    }

    // 位置更新（取得成功時のみ）
    // ウィンドウが変わった場合は即座に移動（補間なし）
    if (caretWnd != m_lastCaretWindow) {
        m_currentDisplayPos = caretPos;
    }

    m_lastCaretPos = caretPos;
    m_lastCaretWindow = caretWnd;
    m_lastUpdateTick = GetTickCount();
    return true;
}

bool CaretIndicator::Probe() {
    POINT pos;
    HWND wnd;
    return GetCaretPosition(&pos, &wnd);
}

void CaretIndicator::ResetCaret() {
    m_lastCaretWindow = NULL;
    Hide();
}

void CaretIndicator::SetState(ImeState state) {
    m_currentState = state;
    if (m_enabled && m_lastCaretWindow != NULL) {
        UpdateDisplay();
    }
}

void CaretIndicator::ShowAtPhysical(POINT physical) {
    if (!m_enabled || m_hwnd == NULL) return;
    // このプロセスはDPI非対応なので、物理ピクセルを自分の座標に直す（拡大率100%なら同じ値）
    POINT pos = physical;
    PhysicalToLogicalPointForPerMonitorDPI(m_hwnd, &pos);
    m_lastCaretPos = pos;
    m_lastCaretWindow = GetForegroundWindow();
    m_lastUpdateTick = GetTickCount();
    UpdateDisplay();
}

void CaretIndicator::UpdateDisplay() {
    if (!m_enabled) {
        Hide();
        return;
    }

    // 位置がまだ取得されていない場合は非表示
    if (m_lastCaretWindow == NULL) {
        Hide();
        return;
    }

    // キャレットの実際の位置を即座に表示（LERPなし、軌跡なし）
    m_currentDisplayPos = m_lastCaretPos;

    // 位置も状態も前回描いたままなら何もしない（最前面のウィンドウを毎回動かし描き直すと、前面のアプリの描画に響く）
    if (m_visible && m_rendered
        && m_renderedPos.x == m_currentDisplayPos.x && m_renderedPos.y == m_currentDisplayPos.y
        && m_renderedState == m_currentState) {
        return;
    }

    UpdateWindowPosition(m_currentDisplayPos);
    RenderGlow();
    Show();
    m_rendered = true;
    m_renderedPos = m_currentDisplayPos;
    m_renderedState = m_currentState;
}

const wchar_t* CaretIndicator::GetLastMethodName() const {
    switch (m_lastMethod) {
    case METHOD_GUITHREADINFO: return L"GUIThreadInfo";
    case METHOD_IMM32:         return L"IMM32";
    case METHOD_TSF:           return L"TSF";
    case METHOD_UIAUTOMATION:  return L"UIAutomation";
    default:                   return L"none";
    }
}

bool CaretIndicator::GetCaretPosition(POINT* pos, HWND* caretWnd) {
    // 優先順位1: GetGUIThreadInfo（軽量・標準アプリ対応）
    if (TryGetFromGUIThreadInfo(pos, caretWnd)) {
        m_lastMethod = METHOD_GUITHREADINFO;
        return true;
    }

    // 優先順位2: IMM32（変換中のみ有効）
    if (TryGetFromIMM32(pos, caretWnd)) {
        m_lastMethod = METHOD_IMM32;
        return true;
    }

    // 優先順位3: UI Automation（Electron/Chrome等の別プロセスアプリ）
    if (TryGetFromUIAutomation(pos, caretWnd)) {
        m_lastMethod = METHOD_UIAUTOMATION;
        return true;
    }

    // 優先順位4: TSF（同一プロセス用、通常は使われない）
    if (TryGetFromTSF(pos, caretWnd)) {
        m_lastMethod = METHOD_TSF;
        return true;
    }

    m_lastMethod = METHOD_NONE;
    return false;
}

bool CaretIndicator::TryGetFromGUIThreadInfo(POINT* pos, HWND* caretWnd) {
    GUITHREADINFO gti = {sizeof(GUITHREADINFO)};
    if (!GetGUIThreadInfo(0, &gti)) {
        return false;
    }

    if (gti.hwndCaret == NULL) {
        return false;
    }

    // キャレット矩形の左上を使用
    pos->x = gti.rcCaret.left;
    pos->y = gti.rcCaret.top;
    *caretWnd = gti.hwndCaret;

    // クライアント座標→画面座標
    ClientToScreen(gti.hwndCaret, pos);

    return true;
}

bool CaretIndicator::TryGetFromIMM32(POINT* pos, HWND* caretWnd) {
    HWND foreground = GetForegroundWindow();
    if (foreground == NULL) {
        return false;
    }

    HIMC himc = ImmGetContext(foreground);
    if (himc == NULL) {
        return false;
    }

    COMPOSITIONFORM cf;
    bool success = false;

    if (ImmGetCompositionWindow(himc, &cf)) {
        pos->x = cf.ptCurrentPos.x;
        pos->y = cf.ptCurrentPos.y;
        *caretWnd = foreground;

        // クライアント座標→画面座標
        ClientToScreen(foreground, pos);
        success = true;
    }

    ImmReleaseContext(foreground, himc);
    return success;
}

bool CaretIndicator::TryGetFromTSF(POINT* pos, HWND* caretWnd) {
    if (m_threadMgr == NULL) {
        return false;
    }

    ITfDocumentMgr* docMgr = NULL;
    HRESULT hr = m_threadMgr->GetFocus(&docMgr);

    if (FAILED(hr) || docMgr == NULL) {
        return false;
    }

    ITfContext* context = NULL;
    hr = docMgr->GetTop(&context);

    if (FAILED(hr) || context == NULL) {
        docMgr->Release();
        return false;
    }

    ITfContextView* view = NULL;
    hr = context->GetActiveView(&view);

    bool success = false;

    if (SUCCEEDED(hr) && view != NULL) {
        // ウィンドウハンドルを取得
        HWND hwnd = NULL;
        hr = view->GetWnd(&hwnd);

        if (SUCCEEDED(hr) && hwnd != NULL) {
            // EditSessionを使って正確なキャレット位置を取得
            RECT caretRect = {0};
            bool editSuccess = false;

            TsfCaretEditSession* editSession = new TsfCaretEditSession(context, view, &caretRect, &editSuccess);

            HRESULT hrSession;
            hr = context->RequestEditSession(
                m_clientId,
                editSession,
                TF_ES_READ | TF_ES_SYNC,
                &hrSession
            );

            editSession->Release();

            if (SUCCEEDED(hr) && editSuccess) {
                // 矩形の左上をキャレット位置として使用
                pos->x = caretRect.left;
                pos->y = caretRect.top;
                *caretWnd = hwnd;
                success = true;
            }
        }

        view->Release();
    }

    context->Release();
    docMgr->Release();

    return success;
}

bool CaretIndicator::TryGetFromUIAutomation(POINT* pos, HWND* caretWnd) {
    if (m_uiAutomation == NULL) {
        return false;
    }

    IUIAutomationElement* focusedElement = NULL;
    HRESULT hr = m_uiAutomation->GetFocusedElement(&focusedElement);

    if (FAILED(hr) || focusedElement == NULL) {
        return false;
    }

    // まず、フォーカスされた要素でテキストパターンを試す
    bool success = TryGetCaretFromElement(focusedElement, pos, caretWnd);

    // 失敗した場合、親要素を複数階層探索
    if (!success) {
        IUIAutomationTreeWalker* walker = NULL;
        hr = m_uiAutomation->get_ControlViewWalker(&walker);

        if (SUCCEEDED(hr) && walker != NULL) {
            IUIAutomationElement* current = focusedElement;
            current->AddRef();

            // 最大5階層まで親を探索
            for (int i = 0; i < 5 && !success; i++) {
                IUIAutomationElement* parent = NULL;
                hr = walker->GetParentElement(current, &parent);

                if (FAILED(hr) || parent == NULL) {
                    break;
                }

                current->Release();
                current = parent;

                success = TryGetCaretFromElement(current, pos, caretWnd);
            }

            current->Release();
            walker->Release();
        }
    }

    // まだ失敗している場合、子要素を探索
    if (!success) {
        IUIAutomationCondition* condition = NULL;
        VARIANT varProp;
        varProp.vt = VT_BOOL;
        varProp.boolVal = VARIANT_TRUE;

        hr = m_uiAutomation->CreatePropertyCondition(UIA_IsTextPatternAvailablePropertyId, varProp, &condition);

        if (SUCCEEDED(hr) && condition != NULL) {
            IUIAutomationElement* textElement = NULL;
            hr = focusedElement->FindFirst(TreeScope_Descendants, condition, &textElement);

            if (SUCCEEDED(hr) && textElement != NULL) {
                success = TryGetCaretFromElement(textElement, pos, caretWnd);
                textElement->Release();
            }

            condition->Release();
        }
    }

    focusedElement->Release();
    return success;
}

bool CaretIndicator::TryGetCaretFromElement(IUIAutomationElement* element, POINT* pos, HWND* caretWnd) {
    if (element == NULL) {
        return false;
    }

    // テキストパターンを取得
    IUIAutomationTextPattern* textPattern = NULL;
    HRESULT hr = element->GetCurrentPatternAs(UIA_TextPatternId, IID_IUIAutomationTextPattern, (void**)&textPattern);

    bool success = false;

    if (SUCCEEDED(hr) && textPattern != NULL) {
        // TextPattern2でキャレット位置を直接取得（Windows 8以降）
        IUIAutomationTextPattern2* textPattern2 = NULL;
        hr = textPattern->QueryInterface(IID_IUIAutomationTextPattern2, (void**)&textPattern2);

        if (SUCCEEDED(hr) && textPattern2 != NULL) {
            // GetCaretRange()でキャレット位置を直接取得
            IUIAutomationTextRange* caretRange = NULL;
            BOOL isActive = FALSE;
            hr = textPattern2->GetCaretRange(&isActive, &caretRange);

            if (SUCCEEDED(hr) && caretRange != NULL && isActive) {
                // 境界矩形を取得
                SAFEARRAY* rectArray = NULL;
                hr = caretRange->GetBoundingRectangles(&rectArray);

                if (SUCCEEDED(hr) && rectArray != NULL) {
                    double* rects = NULL;
                    SafeArrayAccessData(rectArray, (void**)&rects);

                    LONG lBound, uBound;
                    SafeArrayGetLBound(rectArray, 1, &lBound);
                    SafeArrayGetUBound(rectArray, 1, &uBound);

                    int count = uBound - lBound + 1;

                    if (count >= 4) {
                        // キャレット位置の矩形（通常は幅0）
                        LONG x = (LONG)rects[0];
                        LONG y = (LONG)rects[1];

                        // 明らかにおかしい座標（画面左端など）はスキップ
                        // VSCodeなどエディタは左端に近い場合があるので閾値は低めに
                        if (x > 10 && y > 0) {
                            pos->x = x;
                            pos->y = y;

                            // ウィンドウハンドルを取得
                            HWND hwnd = GetForegroundWindow();
                            *caretWnd = hwnd;

                            success = true;
                        }
                    }

                    SafeArrayUnaccessData(rectArray);
                    SafeArrayDestroy(rectArray);
                }

                caretRange->Release();
            }

            textPattern2->Release();
        }

        // TextPattern2が使えない場合は従来の方法
        if (!success) {
            IUIAutomationTextRangeArray* rangeArray = NULL;
            hr = textPattern->GetSelection(&rangeArray);

            if (SUCCEEDED(hr) && rangeArray != NULL) {
                int length = 0;
                rangeArray->get_Length(&length);

                if (length > 0) {
                    IUIAutomationTextRange* range = NULL;
                    hr = rangeArray->GetElement(0, &range);

                    if (SUCCEEDED(hr) && range != NULL) {
                        // 境界矩形を取得
                        SAFEARRAY* rectArray = NULL;
                        hr = range->GetBoundingRectangles(&rectArray);

                        if (SUCCEEDED(hr) && rectArray != NULL) {
                            double* rects = NULL;
                            SafeArrayAccessData(rectArray, (void**)&rects);

                            LONG lBound, uBound;
                            SafeArrayGetLBound(rectArray, 1, &lBound);
                            SafeArrayGetUBound(rectArray, 1, &uBound);

                            int count = uBound - lBound + 1;

                            if (count >= 4) {
                                // 最初の矩形の左上を使用
                                pos->x = (LONG)rects[0];
                                pos->y = (LONG)rects[1];

                                // ウィンドウハンドルを取得
                                HWND hwnd = GetForegroundWindow();
                                *caretWnd = hwnd;

                                success = true;
                            }

                            SafeArrayUnaccessData(rectArray);
                            SafeArrayDestroy(rectArray);
                        }

                        range->Release();
                    }
                }

                rangeArray->Release();
            }
        }

        textPattern->Release();
    }

    return success;
}

bool CaretIndicator::ShouldUpdate(POINT newPos, HWND newWnd) {
    // ウィンドウが変わった場合は更新
    if (newWnd != m_lastCaretWindow) {
        return true;
    }

    // 最小更新間隔チェック
    DWORD currentTick = GetTickCount();
    if (currentTick - m_lastUpdateTick < UPDATE_INTERVAL) {
        return false;
    }

    // 位置の変化が閾値以上か
    int dx = abs(newPos.x - m_lastCaretPos.x);
    int dy = abs(newPos.y - m_lastCaretPos.y);

    return (dx >= UPDATE_THRESHOLD || dy >= UPDATE_THRESHOLD);
}

void CaretIndicator::UpdateWindowPosition(POINT caretPos) {
    // キャレット位置を基準に左下に配置
    int x = caretPos.x - GLOW_WIDTH / 4;
    int y = caretPos.y - GLOW_HEIGHT / 4;

    SetWindowPos(m_hwnd, HWND_TOPMOST,
        x, y, GLOW_WIDTH, GLOW_HEIGHT,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void CaretIndicator::RenderGlow() {
    if (m_backBuffer == NULL) return;

    Graphics graphics(m_backBuffer);
    graphics.Clear(Color(0, 0, 0, 0));  // 透明でクリア

    Color glowColor = GetGlowColor(m_currentState);
    DrawGlow(&graphics, glowColor);

    // レイヤードウィンドウに描画
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap;
    m_backBuffer->GetHBITMAP(Color(0, 0, 0, 0), &hBitmap);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    POINT ptSrc = {0, 0};
    SIZE size = {GLOW_WIDTH, GLOW_HEIGHT};
    POINT ptDst = {0, 0};
    GetWindowRect(m_hwnd, (LPRECT)&ptDst);

    UpdateLayeredWindow(m_hwnd, hdcScreen, &ptDst, &size, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void CaretIndicator::DrawGlow(Graphics* graphics, Color color) {
    // グラデーションブラシでグロー効果を描画（キャレットサイズ）
    int centerX = GLOW_WIDTH / 4;      // 中心を左寄りに
    int centerY = GLOW_HEIGHT / 2;     // 中心を中央に
    int radiusX = GLOW_WIDTH / 3;      // 横幅
    int radiusY = GLOW_HEIGHT / 2;     // キャレットの高さに合わせる

    GraphicsPath path;
    path.AddEllipse(centerX - radiusX, centerY - radiusY, radiusX * 2, radiusY * 2);

    PathGradientBrush brush(&path);

    // 中心色（半透明）
    Color centerColor(100, color.GetR(), color.GetG(), color.GetB());
    brush.SetCenterColor(centerColor);

    // 外側の色（透明）
    Color edgeColor(0, color.GetR(), color.GetG(), color.GetB());
    int count = 1;
    brush.SetSurroundColors(&edgeColor, &count);

    graphics->SetSmoothingMode(SmoothingModeAntiAlias);
    graphics->FillEllipse(&brush, centerX - radiusX, centerY - radiusY, radiusX * 2, radiusY * 2);
}

Color CaretIndicator::GetGlowColor(ImeState state) {
    if (state == ImeState::Japanese) {
        return Color(255, 255, 80, 80);  // 赤
    } else {
        return Color(255, 80, 80, 255);  // 青
    }
}

void CaretIndicator::Show() {
    if (!m_visible) {
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        m_visible = true;
    }
}

void CaretIndicator::Hide() {
    m_rendered = false;
    if (m_visible) {
        ShowWindow(m_hwnd, SW_HIDE);
        m_visible = false;
    }
}

void CaretIndicator::Cleanup() {
    Hide();

    if (m_backBuffer) {
        delete m_backBuffer;
        m_backBuffer = NULL;
    }

    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }

    if (m_threadMgr) {
        if (m_clientId != TF_CLIENTID_NULL) {
            m_threadMgr->Deactivate();
            m_clientId = TF_CLIENTID_NULL;
        }
        m_threadMgr->Release();
        m_threadMgr = NULL;
    }

    if (m_uiAutomation) {
        m_uiAutomation->Release();
        m_uiAutomation = NULL;
    }

    if (m_gdiplusToken) {
        GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }

    CoUninitialize();
}

LRESULT CALLBACK CaretIndicator::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
