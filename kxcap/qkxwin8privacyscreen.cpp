/*******************************************************************************************
*
* Copyright (C) 2022 Guangzhou AoYiDuo Network Technology Co.,Ltd. All Rights Reserved.
*
* Contact: http://www.aoyiduo.com
*
*   this file is used under the terms of the GPLv3[GNU GENERAL PUBLIC LICENSE v3]
* more information follow the website: https://www.gnu.org/licenses/gpl-3.0.en.html
*
*******************************************************************************************/

#include "qkxwin8privacyscreen.h"
#include "qkxutils.h"

#include <windows.h>
#include <magnification.h>
#include <QRect>
#include <QDebug>
#include <QImage>
#include <QLibraryInfo>
#include <QMutex>
#include <QMutexLocker>

#pragma comment(lib, "Magnification.lib")

#define FEIDESK_PRIVACY_MASK    (L"FeiDeskPrivateMask")
#define FEIDESK_PRIVACY_HOST    (L"FeiDeskPrivateHost")
#define FEIDESK_HOTKEY_ID       (WM_USER+1)

class QKxWin8SubPrivacyScreen : public QThread
{
    HWND m_hWndMag;
    HWND m_hWndHost;
    HWND m_hWndMask;
    QKxWin8PrivacyScreen *m_p;
    QRect m_screenRt;

    QStringList m_tips;
    QString m_key;

    QMutex m_mtx;

    struct FrameRequest {
        uchar *pbits;
        int bytesPerline;
        int width;
        int height;
    };
    FrameRequest m_request;
public:
    QKxWin8SubPrivacyScreen(const QStringList& tips, const QRect &screenRt, QKxWin8PrivacyScreen *p)
        : m_p(p)
        , m_tips(tips)
        , m_screenRt(screenRt) {
        start();
    }

    ~QKxWin8SubPrivacyScreen() {

    }

    void RegisterHostClass(HINSTANCE hinst)
    {
        static bool bInit = false;
        WNDCLASSEXW wcex = {};
        if(bInit) {
            return;
        }
        bInit = true;

        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style          = 0;
        wcex.cbWndExtra     = sizeof(ULONG);
        wcex.lpfnWndProc    = HostWndProc;
        wcex.hInstance      = hinst;
        wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
        wcex.lpszClassName  = FEIDESK_PRIVACY_HOST;

        RegisterClassExW(&wcex);
    }

    void RegisterMaskClass(HINSTANCE hinst)
    {
        static bool bInit = false;
        WNDCLASSEXW wcex = {};
        if(bInit) {
            return;
        }
        bInit = true;

        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style          = 0;
        wcex.cbWndExtra     = sizeof(ULONG);
        wcex.lpfnWndProc    = MaskWndProc;
        wcex.hInstance      = hinst;
        wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
        wcex.lpszClassName  = FEIDESK_PRIVACY_MASK;

        RegisterClassExW(&wcex);
    }

    static LRESULT CALLBACK HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    static LRESULT CALLBACK MaskWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        QKxWin8SubPrivacyScreen *prv = (QKxWin8SubPrivacyScreen*)::GetWindowLongA(hWnd, 0);
        if(prv == nullptr) {
            return ::DefWindowProc(hWnd, message, wParam, lParam);
        }

        return prv->CallBack(hWnd, message, wParam, lParam);
    }

    LRESULT CALLBACK CallBack(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message)
        {
        case WM_ERASEBKGND:            
            {
                PAINTSTRUCT ps;
                HDC hdc = ::BeginPaint(hWnd, &ps);             
                RECT rt;
                ::GetClientRect(hWnd, &rt);
                HBRUSH hBrush = ::CreateSolidBrush(0);
                ::FillRect(hdc, &rt, hBrush);

                if(1||!m_tips.isEmpty()) {
                    ::SetBkMode(hdc, TRANSPARENT);
                    HGDIOBJ hBrushOld = ::SelectObject(hdc, hBrush);
                    HFONT hFont = ::CreateFont(48,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                                                   CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,TEXT("Impact"));
                    HGDIOBJ hFontOld = ::SelectObject(hdc, hFont);
                    ::SetTextColor(hdc, RGB(188, 188, 188));
                    int y = 10;
                    for(int i = 0; i < m_tips.length(); i++) {
                        QString tip = m_tips.at(i);
                        const WCHAR *wstr = (WCHAR*)tip.data();
                        SIZE sz;
                        ::GetTextExtentPointW(hdc, wstr, tip.length(), &sz);
                        ::TextOutW(hdc, 10, y, (WCHAR*)tip.data(), tip.length());
                        y += sz.cy;
                    }
                    ::SelectObject(hdc, hBrushOld);
                    ::SelectObject(hdc, hFontOld);
                    ::DeleteObject(hFont);
                }
                ::DeleteObject(hBrush);
                ::ReleaseDC(hWnd, hdc);
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    void stop() {
        ::PostMessageW(m_hWndMag, WM_QUIT, 0, 0);
        ::PostMessageW(m_hWndHost, WM_QUIT, 0, 0);
        ::PostMessageW(m_hWndMask, WM_QUIT, 0, 0);
    }

    int running(HINSTANCE hInst) {
        if(!init(hInst)) {
            return -1;
        }
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) {
            if(msg.message == WM_HOTKEY) {
                if(msg.wParam == FEIDESK_HOTKEY_ID) {
                    emit m_p->deactivedArrived();
                }
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return 0;
    }
    /***
     *  https://docs.microsoft.com/zh-cn/previous-versions/windows/desktop/magapi/magapi-intro?redirectedfrom=MSDN
     **/
    void run() {
        HINSTANCE hInst = ::GetModuleHandleW(NULL);
        if(!QKxUtils::selectInputDesktop()) {
            qDebug() << "bad to select input desktop before mag api.";
        }
        RegisterHostClass(hInst);
        RegisterMaskClass(hInst);
        MagInitialize();
        try{
            running(hInst);
        }catch(...) {
            qDebug() << "PrivateScreen Exception...";
        }
        MagUninitialize();
    }

    BOOL SetMagnificationFactor(HWND hwndMag, float magFactor) {
        MAGTRANSFORM matrix;
        memset(&matrix, 0, sizeof(matrix));
        matrix.v[0][0] = magFactor;
        matrix.v[1][1] = magFactor;
        matrix.v[2][2] = 1.0f;

        return MagSetWindowTransform(hwndMag, &matrix);
    }

    static HWND findHostWindow(HWND hwnd) {
        while(1) {
            wchar_t clsName[50] = {0};
            HWND hwndParent = ::GetParent(hwnd);
            ::GetClassNameW(hwndParent, clsName, 50);
            if(wcscmp(clsName, FEIDESK_PRIVACY_HOST) == 0) {
                return hwndParent;
            }else if(hwndParent == nullptr) {
                return nullptr;
            }
            hwnd = hwndParent;
        }
    }

    static BOOL CALLBACK MagImageScalingCallback(HWND hwnd,
                                                     void * srcdata,
                                                     MAGIMAGEHEADER srcheader,
                                                     void * destdata,
                                                     MAGIMAGEHEADER destheader,
                                                     RECT unclipped,
                                                     RECT clipped,
                                                     HRGN dirty) {
        HWND hwndHost = findHostWindow(hwnd);
        if(hwndHost == nullptr) {
            return FALSE;
        }
        QKxWin8SubPrivacyScreen *prv = (QKxWin8SubPrivacyScreen*)::GetWindowLongA(hwndHost, 0);
        if(prv == nullptr) {
            return TRUE;
        }
        return prv->imageCallback(srcdata, srcheader);
    }

    BOOL imageCallback(void * srcdata, MAGIMAGEHEADER srcheader) {
        uchar *psrc = (uchar*)srcdata;
        uchar *pdst = (uchar*)m_request.pbits;
        for(int h = 0; h < srcheader.height; h++) {
            memcpy(pdst, psrc, m_request.width * 4);
            psrc += srcheader.stride;
            pdst += m_request.bytesPerline;
        }
        return TRUE;
    }

    bool init(HINSTANCE hInst) {
        QRect deskRt = m_screenRt;
        m_hWndHost = ::CreateWindowExW(WS_EX_TOPMOST|WS_EX_LAYERED|WS_EX_TRANSPARENT,
                                           FEIDESK_PRIVACY_HOST, FEIDESK_PRIVACY_HOST,
                                           WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
                                           deskRt.left(), deskRt.top(), deskRt.width(), deskRt.height(),
                                           NULL, NULL, hInst, NULL);
        if(!m_hWndHost) {
            return false;
        }

        ::SetLayeredWindowAttributes(m_hWndHost, 0, 255, LWA_ALPHA);
        m_hWndMag = ::CreateWindowW(WC_MAGNIFIERW,
                                   L"Magnifier",
                                   WS_CHILD | MS_SHOWMAGNIFIEDCURSOR | WS_VISIBLE,
                                   0, 0, deskRt.width(), deskRt.height(),
                                   m_hWndHost, NULL, hInst, NULL);
        if(!m_hWndMag) {
            return false;
        }

        ::SetWindowLongA(m_hWndHost, 0, (ULONG)this);

        if(!MagSetImageScalingCallback(m_hWndMag, MagImageScalingCallback)) {
            int err = GetLastError();
            qDebug() << "Mag Error" << err;
            return FALSE;
        }

        m_hWndMask = ::CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_LAYERED|WS_EX_TRANSPARENT,
                                           FEIDESK_PRIVACY_MASK, FEIDESK_PRIVACY_MASK,
                                           WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
                                           deskRt.left(), deskRt.top(), deskRt.width(), deskRt.height(),
                                           NULL, NULL, hInst, NULL);
        if(!m_hWndMask) {
            return false;
        }
#ifdef QT_DEBUG
        ::SetLayeredWindowAttributes(m_hWndMask, 0, 128, LWA_ALPHA);
#else
        ::SetLayeredWindowAttributes(m_hWndMask, 0, 255, LWA_ALPHA);
#endif

        ::SetWindowLongA(m_hWndMask, 0, (ULONG)this);

        if(!m_tips.isEmpty()) {
            int fsModifiers = MOD_CONTROL|MOD_SHIFT|MOD_NOREPEAT;
            m_key = "CTRL+SHIFT+Q";
            if(::RegisterHotKey(NULL, FEIDESK_HOTKEY_ID, fsModifiers, 'Q') == FALSE) {
                m_key = "CTRL+SHIFT+E";
                if(::RegisterHotKey(NULL, FEIDESK_HOTKEY_ID, fsModifiers, 'E') == FALSE) {
                    m_key = "CTRL+SHIFT+S";
                    if(::RegisterHotKey(NULL, FEIDESK_HOTKEY_ID, fsModifiers, 'S') == FALSE) {
                        m_key = "";
                        qDebug() << "Failed to register system hotkey.";
                    }
                }
            }
            if(m_key.isEmpty()) {
                m_tips.append(QObject::tr("Failed to register hotkey."));
            }else{
                m_tips.append(m_key);
            }
        }
        return true;
    }

    bool isVisible() {
        return ::IsWindowVisible(m_hWndMask);
    }

    void show() {
        ::SetWindowPos(m_hWndMask, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    }

    void hide() {
        ::SetWindowPos(m_hWndMask, HWND_TOPMOST, 0, 0, 0, 0, SWP_HIDEWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    }

    bool snapshot(int idx, uchar *pbuf, int bytesPerline, int width, int height) {
        QMutexLocker lk(&m_mtx);
        if(!::IsWindowVisible(m_hWndMask)) {
            return false;
        }

        HWND filterList[1] = {m_hWndMask};

        if (!MagSetWindowFilterList(m_hWndMag, MW_FILTERMODE_EXCLUDE, 1, filterList)) {
            return false;
        }
        QRect rt = m_screenRt;
        m_request.height = qMin<int>(height, rt.height());
        m_request.width = qMin<int>(width, rt.width());
        m_request.bytesPerline = bytesPerline;
        m_request.pbits = pbuf;
        //RECT frameRt{rt.left(), rt.top(), rt.left() + m_request.width, rt.top() + m_request.height};
        RECT frameRt{0, 0, m_request.width, m_request.height};
        if(!MagSetWindowSource(m_hWndMag, frameRt)) {
            return false;
        }
        return true;
    }
};


QKxWin8PrivacyScreen::QKxWin8PrivacyScreen(const QVector<QRect> &screenRt, QObject *parent)
    : QKxPrivacyScreen(parent)
{
    QStringList tips;
    tips.append(tr("The computer is currently under remote control."));
    tips.append(tr("Press the following shortcut to disconnect the remote control immediately."));
    for(int i = 0; i < screenRt.length(); i++) {
        QKxWin8SubPrivacyScreen *sub = new QKxWin8SubPrivacyScreen(tips, screenRt.at(i), this);
        m_subs.append(sub);
        tips.clear();
    }
}

QKxWin8PrivacyScreen::~QKxWin8PrivacyScreen()
{
    for(int i = 0; i < m_subs.length(); i++) {
        QKxWin8SubPrivacyScreen *sub = m_subs.at(i);
        sub->stop();
        sub->wait();
    }
}

bool QKxWin8PrivacyScreen::isVisible()
{
    QKxWin8SubPrivacyScreen *sub = m_subs.at(0);
    return sub->isVisible();
}

void QKxWin8PrivacyScreen::setVisible(bool on)
{
    for(int i = 0; i < m_subs.length(); i++) {
        QKxWin8SubPrivacyScreen *sub = m_subs.at(i);
        if(on) {
            sub->show();
        }else{
            sub->hide();
        }
    }
}

bool QKxWin8PrivacyScreen::snapshot(int idx, uchar *pbuf, int bytesPerline, int width, int height) const
{
    if(idx >= m_subs.count()) {
        return false;
    }
    QKxWin8SubPrivacyScreen *sub = m_subs.at(idx);
    return sub->snapshot(idx, pbuf, bytesPerline, width, height);
}
