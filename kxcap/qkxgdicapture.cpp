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

#include "qkxgdicapture.h"
#include "qkxutils.h"
#include "qkxscreenlistener.h"
#include "qkxprivacyscreen.h"
#include "qkxvnccompress.h"
#include "qkxdirtyframe.h"

#include <Windows.h>
#include <QImage>
#include <QGuiApplication>
#include <QScreen>
#include <QDebug>
#include <QDateTime>
#include <QMap>


#include "windows.h"

#pragma comment (lib, "user32.lib")

using namespace std;
class GDICapturePrivate
{
private:
    struct CursorCache {
        int cnt = 0;
        bool isValid = false;
        HCURSOR hCursor = NULL;
        ICONINFO info = ICONINFO{0};
        BITMAP biMask = BITMAP{0};
        BITMAP biColor = BITMAP{0};
        QByteArray data;
    };

    HDC m_hdcDisplay = NULL;
    HDC m_hdcCache = NULL;
    HBITMAP m_bmpCache = NULL, m_bmpCacheOld = NULL;
    QKxGDICapture* m_pub = nullptr;
    uchar *m_pRgbBuffer = nullptr;
    QRect m_imageRt, m_screenRt;
    bool m_rdpMode = false;
    QByteArray m_mouseData;
    CursorCache m_cursorCache;

    int m_screenIndex;
    QByteArray m_pixel;
    QPointer<QKxPrivacyScreen> m_privacy;
public:
    explicit GDICapturePrivate(int idx, const QRect& imageRt, const QRect& screenRt, QKxPrivacyScreen* prvScreen, QKxGDICapture *p)
        : m_pub(p)
        , m_screenIndex(idx)
        , m_imageRt(imageRt)
        , m_screenRt(screenRt)
        , m_privacy(prvScreen) {
        m_mouseData.reserve(1024);
        m_cursorCache.data.reserve(1024);
        m_pixel.resize(imageRt.width() * 4 * imageRt.height());
    }

    ~GDICapturePrivate() {
        clean();
        if(m_cursorCache.isValid) {
            if(m_cursorCache.info.hbmMask) {
                ::DeleteObject(m_cursorCache.info.hbmMask);
                m_cursorCache.info.hbmMask = NULL;
            }
            if(m_cursorCache.info.hbmColor) {
                ::DeleteObject(m_cursorCache.info.hbmColor);
                m_cursorCache.info.hbmColor = NULL;
            }
        }
    }

    bool setMirrorFullVirtualScreen(wchar_t* devName) {
        int x = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
        int y = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
        int cx = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int cy = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

        DEVMODEW dm = {0};
        WORD drvExtraSaved = dm.dmDriverExtra;
        dm.dmSize = sizeof(DEVMODEW);
        dm.dmDriverExtra = drvExtraSaved;
        dm.dmPelsWidth = cx;
        dm.dmPelsHeight = cy;
        dm.dmBitsPerPel = 32;
        dm.dmPosition.x = x;
        dm.dmPosition.y = y;
        dm.dmDeviceName[0] = '\0';
        dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH |
                      DM_PELSHEIGHT | DM_POSITION;
        int err = ::ChangeDisplaySettingsExW(devName, &dm, 0, CDS_UPDATEREGISTRY, 0);
        if(err == DISP_CHANGE_NOTUPDATED || err == DISP_CHANGE_SUCCESSFUL) {
            int err = ::ChangeDisplaySettingsExW(devName, &dm, 0, 0, 0);
            if(err == DISP_CHANGE_NOTUPDATED || err == DISP_CHANGE_SUCCESSFUL) {
                return true;
            }
        }
        return false;
    }

    QString getMirrorDevice() {
        QString rdpMirror = "RDP Encoder Mirror Driver";

        DISPLAY_DEVICEW dd = {0};
        dd.cb = sizeof(dd);
        int idx = 0;
        while(EnumDisplayDevicesW(NULL, idx, &dd, EDD_GET_DEVICE_INTERFACE_NAME)) {
            QString drv = QString::fromWCharArray(dd.DeviceString);
            qDebug() << drv;
            idx++;
            if(drv == rdpMirror) {
                DEVMODEW dm = {0};
                dm.dmSize = sizeof(DEVMODEW);
                dm.dmDriverExtra = 0;
                if(EnumDisplaySettingsExW(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, EDS_ROTATEDMODE)) {
                    int x = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
                    int y = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
                    int cx = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
                    int cy = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
                    if(dm.dmPosition.x != x || dm.dmPosition.y != y ||
                            dm.dmPelsWidth != cx || dm.dmPelsHeight != cy ||
                            dm.dmBitsPerPel != 32) {
                        if(setMirrorFullVirtualScreen(dd.DeviceName)) {
                            return QString::fromWCharArray(dd.DeviceName);
                        }
                        return QString();
                    }
                    return QString::fromWCharArray(dd.DeviceName);
                }
                if(setMirrorFullVirtualScreen(dd.DeviceName)) {
                    return QString::fromWCharArray(dd.DeviceName);
                }
            }
        }
        return QString();
    }

    void clean() {
        if(m_bmpCacheOld) {
            ::SelectObject(m_hdcCache, (HGDIOBJ)m_bmpCacheOld);
            m_bmpCacheOld = NULL;
        }
        if(m_bmpCache) {
            ::DeleteObject(m_bmpCache);
            m_bmpCache = NULL;
        }
        if(m_hdcCache) {
            ::DeleteDC(m_hdcCache);
            m_hdcCache = NULL;
        }
        if(m_hdcDisplay) {
            ::DeleteDC(m_hdcDisplay);
            m_hdcDisplay = NULL;
        }
    }

    bool init(const QString& devName, bool tryMirror) {
        QString name = devName;
        m_rdpMode = false;
        if(tryMirror) {
            QString devTmp = getMirrorDevice();
            m_rdpMode = !devTmp.isEmpty();
            if(m_rdpMode) {
                name = devTmp;
            }
        }
        m_hdcDisplay = ::CreateDCW((LPWSTR)name.data(), NULL, NULL, NULL);
        if(m_hdcDisplay == NULL) {
            return false;
        }        
        int width = m_screenRt.width();
        int height = m_screenRt.height();
        BITMAPINFOHEADER bih = {0};
        bih.biSize = sizeof(BITMAPINFOHEADER);
        bih.biBitCount = 32;
        bih.biCompression = BI_RGB;
        bih.biPlanes = 1;
        bih.biWidth = width;
        bih.biHeight = -height;
        bih.biSizeImage = width * height * 4;
        m_hdcCache = ::CreateCompatibleDC(m_hdcDisplay);
        BITMAPINFO bi = {0};
        bi.bmiHeader = bih;
        m_bmpCache = ::CreateDIBSection(m_hdcCache, &bi, DIB_RGB_COLORS, (void**)&m_pRgbBuffer, NULL, 0);
        if(m_bmpCache == NULL) {
            return false;
        }
        m_bmpCacheOld = (HBITMAP)::SelectObject(m_hdcCache, (HGDIOBJ)m_bmpCache);
        return true;
    }

    int getFrame() {
        if(m_privacy != nullptr && m_privacy->isVisible()) {
            return 0;
        }
        int width = m_screenRt.width();
        int height = m_screenRt.height();
        int x = m_rdpMode ? m_screenRt.left() : 0;
        int y = m_rdpMode ? m_screenRt.top() : 0;
        ::BitBlt(m_hdcCache, 0, 0, width, height, m_hdcDisplay, x, y, SRCCOPY|CAPTUREBLT);
        return 0;
    }

    bool getBitmapData(HBITMAP hBitmap, QByteArray& rgb, BITMAP *pInfo) {
        BITMAP bmp = {0};
        if (!::GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bmp)) {
            return false;
        }
        if(pInfo != nullptr) {
            *pInfo = bmp;
        }

        int bufSize = bmp.bmWidth * bmp.bmHeight * 4;

        BITMAPINFO info = {0};
        info.bmiHeader.biSize = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth = bmp.bmWidth;
        info.bmiHeader.biHeight = -bmp.bmHeight;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        info.bmiHeader.biSizeImage = bufSize;
        rgb.resize(bufSize);
        if(!GetDIBits(m_hdcCache, hBitmap, 0, bmp.bmHeight, rgb.data(), &info, DIB_RGB_COLORS)) {
            return false;
        }
        return true;
    }

    int drawMouse(uchar *pRgbbuf, int bytesPerLine) {
        CURSORINFO cursor = {0};
        cursor.cbSize = sizeof(cursor);
        ::GetCursorInfo(&cursor);
        if(cursor.flags & CURSOR_SHOWING) {
            m_cursorCache.cnt++;
            //qDebug() << cursor.ptScreenPos.x << cursor.ptScreenPos.y << m_imageRt << m_screenRt;
            //qDebug() << "drawMouse.HitCount" << m_cursorCache.cnt;
            if(m_cursorCache.hCursor != cursor.hCursor) {
                m_cursorCache.cnt = 0;
                if(m_cursorCache.isValid) {
                    if(m_cursorCache.info.hbmMask) {
                        ::DeleteObject(m_cursorCache.info.hbmMask);
                        m_cursorCache.info.hbmMask = NULL;
                    }
                    if(m_cursorCache.info.hbmColor) {
                        ::DeleteObject(m_cursorCache.info.hbmColor);
                        m_cursorCache.info.hbmColor = NULL;
                    }
                }
                m_cursorCache.isValid = false;
                m_cursorCache.data.clear();
                ICONINFO info = { 0 };
                ::GetIconInfo(cursor.hCursor, &info);
                BITMAP biMask = {0};
                if (!::GetObject(info.hbmMask, sizeof(BITMAP), (LPVOID)&biMask)) {
                    ::DeleteObject(info.hbmMask);
                    if(info.hbmColor) {
                        ::DeleteObject(info.hbmColor);
                    }
                    return 0;
                }
                m_cursorCache.info = info;
                m_cursorCache.biMask = biMask;
                if(info.hbmColor) {
                    BITMAP biColor;
                    if(!getBitmapData(info.hbmColor, m_cursorCache.data, &biColor)) {
                        ::DeleteObject(info.hbmMask);
                        if(info.hbmColor) {
                            ::DeleteObject(info.hbmColor);
                        }
                        return 0;
                    }
                    m_cursorCache.biColor = biColor;
                }else{
                    m_cursorCache.data.resize(biMask.bmWidthBytes * biMask.bmHeight);
                    if (!GetBitmapBits(info.hbmMask, m_cursorCache.data.length(), m_cursorCache.data.data())) {
                        ::DeleteObject(info.hbmMask);
                        return 0;
                    }
                }
                m_cursorCache.info = info;
                m_cursorCache.hCursor = cursor.hCursor;
                m_cursorCache.isValid = true;
            }
            if(m_cursorCache.isValid) {
                ICONINFO &info = m_cursorCache.info;
                BITMAP &biMask = m_cursorCache.biMask;
                if(info.hbmColor) {
                    // same to DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR
                    QRect rtCursor(cursor.ptScreenPos.x - m_screenRt.left() - info.xHotspot,
                                     cursor.ptScreenPos.y - m_screenRt.top() - info.yHotspot,
                                     biMask.bmWidth,
                                     biMask.bmHeight);
                    QRect rt = rtCursor.intersected(QRect(0, 0, m_imageRt.width(), m_imageRt.height()));
                    if(!rt.isEmpty()){
                        BITMAP &biColor = m_cursorCache.biColor;
                        uchar *src = reinterpret_cast<uchar*>(m_cursorCache.data.data());
                        src += rtCursor.top() < 0 ? -rtCursor.top() * biColor.bmWidthBytes : 0;
                        src += rtCursor.left() < 0 ? -rtCursor.left() * 4 : 0;
                        uchar *dst = pRgbbuf + rt.top() * bytesPerLine + rt.left() * 4;
                        for(int r = 0; r < rt.height(); r++) {
                            quint32 *psrc = reinterpret_cast<quint32*>(src + r * biColor.bmWidthBytes);
                            quint32 *pdst = reinterpret_cast<quint32*>(dst + r * bytesPerLine);
                            for(int c = 0; c < rt.width(); c++) {
                                quint32 val = psrc[c];
                                if(val == 0x00000000) {
                                    continue;
                                }else{
                                    pdst[c] = val;
                                }
                            }
                        }
                    }
                }else {
                    int cursor_width = biMask.bmWidth;
                    int cursor_height = biMask.bmHeight;
                    if(biMask.bmBitsPixel == 1) {
                        cursor_height = cursor_height / 2;
                    }
                    QRect rtCursor(cursor.ptScreenPos.x - m_screenRt.left() - info.xHotspot,
                                     cursor.ptScreenPos.y - m_screenRt.top() - info.yHotspot,
                                     cursor_width,
                                     cursor_height);
                    QRect rt = rtCursor.intersected(QRect(0, 0, m_imageRt.width(), m_imageRt.height()));
                    if(!rt.isEmpty()) {
                        uchar *src = reinterpret_cast<uchar*>(m_cursorCache.data.data());
                        src += rtCursor.top() < 0 ? -rtCursor.top() * biMask.bmWidthBytes : 0;
                        src += rtCursor.left() < 0 ? -rtCursor.left() * 4 : 0;
                        uchar *dst = pRgbbuf + rt.top() * bytesPerLine + rt.left() * 4;
                        if(biMask.bmBitsPixel == 1) {
                            // same to DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME
                            for(int r = 0; r < rt.height(); r++) {
                                uchar *psrc = reinterpret_cast<uchar*>(src + r * biMask.bmWidthBytes);
                                quint32 *pdst = reinterpret_cast<quint32*>(dst + r * bytesPerLine);
                                uchar mask = 0x80;
                                for(int c = 0; c < rt.width(); c++) {
                                    uchar andMask = psrc[c / 8] & mask;
                                    uchar xorMask = psrc[(c / 8) + (rtCursor.height() * biMask.bmWidthBytes)] & mask;
                                    quint32 andMask32 = andMask ? 0xFFFFFFFF : 0xFF000000;
                                    quint32 xorMask32 = xorMask ? 0x00FFFFFF : 0x00000000;
                                    pdst[c] = (pdst[c] & andMask32) ^ xorMask32;

                                    mask = mask == 0x01 ? 0x80 : mask >> 1;
                                }
                            }
                        }else{
                            // same to DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR
                            for(int r = 0; r < rt.height(); r++) {
                                quint32 *psrc = reinterpret_cast<quint32*>(src + r * biMask.bmWidthBytes);
                                quint32 *pdst = reinterpret_cast<quint32*>(dst + r * bytesPerLine);
                                for(int c = 0; c < rt.width(); c++) {
                                    quint32 cval = psrc[c];
                                    quint32 sval = pdst[c];
                                    quint32 mask = 0xFF000000 & cval;
                                    pdst[c] = mask ? ((sval ^ cval) | 0xFF000000) : (cval | 0xFF000000);
                                }
                            }
                        }
                    }
                }
            }
        }
        return 0;
    }

    int drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset) {
        QDataStream &ds = req->ds;
        int width = m_imageRt.width();
        int height = m_imageRt.height();
        quint8 *rgbBuffer = m_pRgbBuffer;
        if(m_privacy != nullptr && m_privacy->isVisible()) {
            if(!m_privacy->snapshot(m_screenIndex, rgbBuffer, width *4, width, height)) {
                return 0;
            }
        }else{
            drawMouse(rgbBuffer, width * 4);
        }

        EEncodingType et = req->match(width, height, req);
        if(et == ET_OpenH264) {
            req->srcHeader = (quint8*)rgbBuffer;
            req->dstHeader = pbuf;
            req->imageSize = QSize(width, height);
            req->updateRect = QRect(0, 0, width, height);
            PFunCompress func = req->codes.value(et);
            req->buf.clear();
            req->buf.reserve(width * 4 * height);
            ds << quint16(m_imageRt.left()) << quint16(m_imageRt.top()) << quint16(width) << quint16(height) << quint32(et);
            func((uchar*)pbuf, bytesPerLine, (uchar*)rgbBuffer, width * 4, width, height, req);
            return 1;
        }
        QKxDirtyFrame df(width, height);
        if(!df.findDirtyRect(pbuf, bytesPerLine, (uchar*)rgbBuffer, width * 4)) {
            return 0;
        }
        QList<QRect> rts = df.dirtyRects();
        for(int i = 0; i < rts.length(); i++) {
            QRect rt = rts.at(i);
            req->srcHeader = rgbBuffer;
            req->dstHeader = pbuf;
            req->imageSize = QSize(width, height);
            req->updateRect = rt;

            EEncodingType et = req->match(rt.width(), rt.height(), req);
            ds << quint16(rt.left() + m_imageRt.left()) << quint16(rt.top() + m_imageRt.top()) << quint16(rt.width()) << quint16(rt.height()) << quint32(et);
            PFunCompress func = req->codes.value(et);

            uchar *dst = (uchar*)pbuf;
            dst += rt.top() * bytesPerLine;
            dst += rt.left() * 4;
            uchar *src = (uchar*)rgbBuffer;
            src += rt.top() * width * 4;
            src += rt.left() * 4;
            func(dst, bytesPerLine, src, width * 4, rt.width(), rt.height(), req);            
        }
        return rts.length();
    }
};

QKxGDICapture::QKxGDICapture(int idx, const QRect& imageRt, const QRect& screenRt, QKxPrivacyScreen* prvScreen, QObject *parent)
    : QKxAbstractCapture(parent)
    , m_idx(idx)
    , m_imageRt(imageRt)
{
    m_prv = new GDICapturePrivate(idx, imageRt, screenRt, prvScreen, this);
}

QKxGDICapture::~QKxGDICapture()
{
    delete m_prv;
}

bool QKxGDICapture::reset()
{
    m_prv->clean();
    QList<QKxScreenListener::DisplayInfo> screens = QKxScreenListener::screens();
    QKxScreenListener::DisplayInfo scn = screens.at(m_idx);
    return m_prv->init(scn.name, false);
}

int QKxGDICapture::getFrame()
{
    return m_prv->getFrame();
}

int QKxGDICapture::drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset)
{
    return m_prv->drawFrame(req, pbuf, bytesPerLine, reset);
}
