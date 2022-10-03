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

#include "qkxx11capture.h"

#include "qkxutils.h"
#include "qkxvnccompress.h"
#include "qkxh264encoder.h"
#include "qkxdirtyframe.h"

#include <QDebug>
#include <QDateTime>
#include <QImage>
#include <QPointer>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <sys/ipc.h>
#include <X11/X.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <sys/shm.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xfixes.h>

class X11CaptureLocker {
    Display *m_d;
public:
    X11CaptureLocker(Display* d) {
        m_d = d;
        XLockDisplay(d);
    }
    ~X11CaptureLocker() {
        XSync(m_d, False);
        XUnlockDisplay(m_d);
    }
};

class X11CapturePrivate
{
    QKxX11Capture* m_pub = nullptr;
    QRect m_imageRt, m_screenRt;
    Display *m_display;
    Window m_window;
    XImage *m_image;
    XShmSegmentInfo m_shmInfo;
    bool m_useXShm;
    QKxDirtyFrame m_dirtyFrame;
public:
    explicit X11CapturePrivate(const QRect& imageRt, const QRect& screenRt, QKxX11Capture *p)
            : m_pub(p)
            , m_imageRt(imageRt)
            , m_screenRt(screenRt)
            , m_dirtyFrame(imageRt.width(), imageRt.height()) {
        m_display = nullptr;
        m_image = nullptr;
        m_shmInfo.shmid = -1;
        m_useXShm = false;
    }

    ~X11CapturePrivate() {
        clean();
    }

    void clean() {
        if(m_shmInfo.shmid >= 0) {
            XShmDetach(m_display, &m_shmInfo);
        }
        if(m_image != nullptr) {
            XDestroyImage(m_image);
            m_image = nullptr;
        }
        if(m_shmInfo.shmid >= 0) {
            shmdt(m_shmInfo.shmaddr);
            shmctl(m_shmInfo.shmid, IPC_RMID, 0);
            m_shmInfo.shmid = -1;
        }
        if(m_display != nullptr) {
            XCloseDisplay(m_display);
            m_display = nullptr;
        }
    }

    bool hasXFixes(Display *display) {
        int xfixes_event, xfixes_error;
        return XFixesQueryExtension(display, &xfixes_event, &xfixes_error);
    }

    bool hasXShm(Display *display) {
        return XShmQueryExtension(display);
    }

    bool init(bool useXShm) {
        clean();
        m_useXShm = false;
        m_display = XOpenDisplay(nullptr);
        if(m_display == nullptr) {
            return false;
        }
        char *name = DisplayString(m_display);
        qDebug() << QString(name);
        int width = XDisplayWidth(m_display, 0);
        int height = XDisplayHeight(m_display, 0);
        int depth = XDisplayPlanes(m_display, 0);
        if(depth != 24) {
            return false;
        }
        qDebug() << width << height << depth << m_imageRt;
        Visual *vis = XDefaultVisual(m_display, 0);
        m_window = XRootWindow(m_display, 0);
        if(useXShm && hasXShm(m_display)) {
            m_useXShm = true;
            m_shmInfo.shmid = -1;
            m_image = XShmCreateImage(m_display, vis, depth, ZPixmap, nullptr, &m_shmInfo, m_imageRt.width(), m_imageRt.height());
            if(m_image == nullptr) {
                return false;
            }
            m_shmInfo.shmid = shmget(IPC_PRIVATE, m_image->bytes_per_line * m_image->height, IPC_CREAT|0777);
            if(m_shmInfo.shmid < 0) {
                return false;
            }
            m_shmInfo.shmaddr = m_image->data = (char*)shmat(m_shmInfo.shmid, 0, 0);
            m_shmInfo.readOnly = False;
            if(!XShmAttach(m_display, &m_shmInfo)) {
                return false;
            }
        }else{
            m_useXShm = false;
        }
       // mylayout(m_display);
        return true;
    }

    int getFrame() {
        X11CaptureLocker locker(m_display);
        if(m_useXShm) {
            //int CompletionType = XShmGetEventBase(m_display) + ShmCompletion;
            if(!XShmGetImage(m_display, m_window, m_image, m_imageRt.x(), m_imageRt.y(), AllPlanes)) {
                return -1;
            }
        }else{
            if(m_image != nullptr) {
                XDestroyImage(m_image);
            }
            m_image = XGetImage(m_display, m_window, m_imageRt.x(), m_imageRt.y(), m_imageRt.width(), m_imageRt.height(), AllPlanes, ZPixmap);
        }
        drawMouse((uchar*)m_image->data, m_image->bytes_per_line);


        return 0;
    }

    int drawMouse(uchar *pRgbbuf, int bytesPerLine) {
        if(!hasXFixes(m_display)) {
            return 0;
        }
        //XFixesSelectCursorInput(m_display, m_window, XFixesDisplayCursorNotifyMask);
        XFixesCursorImage* ci;
        ci = XFixesGetCursorImage(m_display);
        if(ci == nullptr) {
            return 0;
        }
        // The X11 data is encoded as one-pixel-per-long, but each pixel is only
        // 32 bits of data, so every other 4 bytes is padding.
        QRect rtCursor(ci->x - m_screenRt.left() - ci->xhot,
                 ci->y - m_screenRt.top() - ci->yhot,
                 ci->width,
                 ci->height);
        QRect rt = rtCursor.intersected(m_imageRt);
        if(rt.isEmpty()) {
            XFree(ci);
            return 0;
        }
        //qDebug() << "drawMouse" << rt << ci->x << ci->y << ci->width << ci->height << ci->cursor_serial;
        uchar *src = reinterpret_cast<uchar*>(ci->pixels +
                                              (rtCursor.top() < 0 ? -rtCursor.top() * ci->width : 0) +
                                              (rtCursor.left() < 0 ? -rtCursor.left() : 0));
        uchar *dst = pRgbbuf + rt.top() * bytesPerLine + rt.left() * 4;
        for(int r = 0; r < rt.height(); r++) {
            quint8 *psrc = reinterpret_cast<quint8*>(src + r * ci->width * 8);
            quint8 *pdst = reinterpret_cast<quint8*>(dst + r * bytesPerLine);
            for(int c = 0; c < rt.width(); c++) {
                quint32 bs = *psrc++;
                quint32 gs = *psrc++;
                quint32 rs = *psrc++;
                quint32 as = *psrc++;
                quint32 bd = pdst[0];
                quint32 gd = pdst[1];
                quint32 rd = pdst[2];

                quint32 b = (bs * as + bd * (255 - as)) / 255;
                quint32 g = (gs * as + gd * (255 - as)) / 255;
                quint32 r = (rs * as + rd * (255 - as)) / 255;
                pdst[0] = b & 0xFF;
                pdst[1] = g & 0xFF;
                pdst[2] = r & 0xFF;
                psrc += 4;
                pdst += 4;
            }
        }
        XFree(ci);
        return 0;
    }

    int drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset) {
        QDataStream &ds = req->ds;
        int width = m_imageRt.width();
        int height = m_imageRt.height();
        if(m_image == nullptr) {
            return 0;
        }
        EEncodingType et = req->match(width, height, req);
        if(et == ET_OpenH264) {            
            req->srcHeader = (quint8*)m_image->data;
            req->dstHeader = pbuf;
            req->imageSize = QSize(width, height);
            req->updateRect = QRect(0, 0, width, height);
            PFunCompress func = req->codes.value(et);
            req->buf.clear();
            req->buf.reserve(width * 4 * height);
            ds << quint16(m_imageRt.left()) << quint16(m_imageRt.top()) << quint16(width) << quint16(height) << quint32(et);
            func((uchar*)pbuf, bytesPerLine, (uchar*)m_image->data, m_image->bytes_per_line, width, height, req);
            return 1;
        }else{
            QKxDirtyFrame df(width, height);
            if(!df.findDirtyRect(pbuf, bytesPerLine, (uchar*)m_image->data, m_image->bytes_per_line)) {
                return 0;
            }
            QList<QRect> rts = df.dirtyRects();
            int jpgCnt = 0;
            for(int i = 0; i < rts.length(); i++) {
                QRect rt = rts.at(i);
                req->srcHeader = (uchar*)m_image->data;
                req->dstHeader = pbuf;
                req->imageSize = QSize(width, height);
                req->updateRect = rt;

                EEncodingType et = req->match(rt.width(), rt.height(), req);
#if 0
                if(et == ET_JPEG) {
                    if(!df.jpegPreffer((uchar*)m_image->data, m_image->bytes_per_line, i)) {
                        et = ET_ZRLE3;
                    }else{
                        jpgCnt++;
                    }
                }
#endif
                ds << quint16(rt.left() + m_imageRt.left()) << quint16(rt.top() + m_imageRt.top()) << quint16(rt.width()) << quint16(rt.height()) << quint32(et);
                PFunCompress func = req->codes.value(et);
                uchar *dst = (uchar*)pbuf;
                dst += rt.top() * bytesPerLine;
                dst += rt.left() * 4;
                uchar *src = (uchar*)m_image->data;
                src += rt.top() * m_image->bytes_per_line;
                src += rt.left() * m_image->bits_per_pixel / 8;
                func(dst, bytesPerLine, src, m_image->bytes_per_line, rt.width(), rt.height(), req);
            }
            // qDebug() << "jpegCount" << jpgCnt << rts.length() << jpgCnt * 100 / rts.length() << QDateTime::currentMSecsSinceEpoch();
            return rts.length();
        }
        return 0;
    }
};

QKxX11Capture::QKxX11Capture(int idx, const QRect &imageRt, const QRect &screenRt, QObject *parent)
    : QKxAbstractCapture(parent)
    , m_imageRt(imageRt)
    , m_idx(idx)
{
    m_prv = new X11CapturePrivate(imageRt, screenRt, this);
}

QKxX11Capture::~QKxX11Capture()
{
    delete m_prv;
}

bool QKxX11Capture::reset()
{
    return m_prv->init(true);
}

int QKxX11Capture::getFrame()
{
    return m_prv->getFrame();
}

int QKxX11Capture::drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset)
{
    return m_prv->drawFrame(req, pbuf, bytesPerLine, reset);
}
