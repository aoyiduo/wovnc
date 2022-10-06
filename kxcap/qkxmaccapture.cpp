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

#include "qkxmaccapture.h"
#include "qkxvnccompress.h"
#include "qkxdirtyframe.h"

#include "qkxutils.h"

#include <QDebug>
#include <QDateTime>
#include <QImage>
#include <QApplication>
#include <QMutex>
#include <QMutexLocker>

#include <dispatch/dispatch.h>
#include <IOKit/IOKitLib.h>
#include <IOSurface/IOSurface.h>
#include <CoreVideo/CoreVideo.h>
#include <CoreGraphics/CoreGraphics.h>


class MacCapturePrivate {
    QKxMacCapture* m_pub = nullptr;
    QRect m_imageRt, m_screenRt;
    CGDirectDisplayID m_displayId;
    CGDisplayStreamRef m_stream = {};
    CGDisplayStreamUpdateRef m_updateRefLast = {};
    dispatch_queue_t m_queue = {};
    bool m_retina = false;
    QByteArray m_image;
    QRegion m_invalidRegion;
    QMutex m_mtx;
    int m_idx;

    QAtomicInt m_frameIndex;
    QAtomicInt m_frameLastIndex;
public:
    explicit MacCapturePrivate(int idx, const QRect& imageRt, const QRect& screenRt, QKxMacCapture *p)
                : m_pub(p)
                , m_imageRt(imageRt)
                , m_screenRt(screenRt)
                , m_frameIndex(0)
                , m_frameLastIndex(0)
                , m_idx(idx) {
        QString lable = QString("capture.aoyiduo.com:%1").arg(idx);
        m_queue = dispatch_queue_create(lable.toLatin1(), DISPATCH_QUEUE_SERIAL);
        m_image.resize(m_imageRt.width() * 4 * m_imageRt.height());
        m_image.fill(0);
    }

    ~MacCapturePrivate() {
        if(m_stream) {
            CGError err = CGDisplayStreamStop(m_stream);
            if(err != kCGErrorSuccess) {
                qWarning() << "failed to start the display stream capturer. CGDisplayStreamStart failed" << err;
            }
            m_stream = {};
        }
    }

    int image_copy(uchar* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight, uchar* pSrcData, int nSrcStep, int nXSrc, int nYSrc)
    {
        uchar *pSrcPixel = pSrcData + (nYSrc * nSrcStep) + (nXSrc * 4);
        uchar *pDstPixel = pDstData + (nYDst * nDstStep) + (nXDst * 4);

        for (int y = 0; y < nHeight; y++)
        {
            memcpy(pDstPixel, pSrcPixel, nWidth * 4);
            pSrcPixel = pSrcPixel + nSrcStep;
            pDstPixel = pDstPixel + nDstStep;
        }

        return 1;
    }

    int image_copy_from_retina(uchar* pDstData, int nDstStep, int nXDst, int nYDst, int nWidth, int nHeight, uchar* pSrcData, int nSrcStep, int nXSrc, int nYSrc)
    {
        uchar* pSrcPixel;
        uchar* pDstPixel;
        int x, y;
        int nSrcPad;
        int nDstPad;
        int srcBitsPerPixel;
        int srcBytesPerPixel;
        int dstBitsPerPixel;
        int dstBytesPerPixel;
        srcBitsPerPixel = 32;
        srcBytesPerPixel = 8;

        if (nSrcStep < 0){
            nSrcStep = srcBytesPerPixel * nWidth;
        }

        dstBitsPerPixel = 32;
        dstBytesPerPixel = 4;

        if (nDstStep < 0){
            nDstStep = dstBytesPerPixel * nWidth;
        }

        nSrcPad = (nSrcStep - (nWidth * srcBytesPerPixel));
        nDstPad = (nDstStep - (nWidth * dstBytesPerPixel));
        pSrcPixel = &pSrcData[(nYSrc * nSrcStep) + (nXSrc * 4)];
        pDstPixel = &pDstData[(nYDst * nDstStep) + (nXDst * 4)];

        for (y = 0; y < nHeight; y++)
        {
            for (x = 0; x < nWidth; x++)
            {
                quint32 R, G, B;
                quint32 color;
                /* simple box filter scaling, could be improved with better algorithm */
                B = pSrcPixel[0] + pSrcPixel[4] + pSrcPixel[nSrcStep + 0] + pSrcPixel[nSrcStep + 4];
                G = pSrcPixel[1] + pSrcPixel[5] + pSrcPixel[nSrcStep + 1] + pSrcPixel[nSrcStep + 5];
                R = pSrcPixel[2] + pSrcPixel[6] + pSrcPixel[nSrcStep + 2] + pSrcPixel[nSrcStep + 6];
                pSrcPixel += 8;
                pDstPixel[0] = (B >> 2) & 0xFF;
                pDstPixel[1] = (G >> 2) & 0xFF;
                pDstPixel[2] = (R >> 2) & 0xFF;
                pDstPixel[3] = 0xFF;
                pDstPixel += dstBytesPerPixel;
            }

            pSrcPixel = &pSrcPixel[nSrcPad + nSrcStep];
            pDstPixel = &pDstPixel[nDstPad];
        }

        return 1;
    }

    /*
         n.b. when the user needs to handle the pixel buffer after the call, it needs to make
         a copy. CGDisplayStream* supports a feature where you can increment the in-use-count, see
         the documention: https://developer.apple.com/library/mac/documentation/GraphicsImaging/Reference/Quartz_Services_Ref/index.html#//apple_ref/doc/c_ref/CGDisplayStreamFrameAvailableHandler
         https://time.geekbang.org/dailylesson/detail/100056834?tid=146
         */
    void callback(CGDisplayStreamFrameStatus status, uint64_t displayTime, IOSurfaceRef frame, CGDisplayStreamUpdateRef updateRef) {
        size_t numRects = 0;
        const CGRect* rects = CGDisplayStreamUpdateGetRects(updateRef, kCGDisplayStreamUpdateDirtyRects, &numRects);
        //qDebug() << "numRects" << numRects << status;
        if(numRects <= 0 || status != kCGDisplayStreamFrameStatusFrameComplete) {
            return;
        }

        QMutexLocker lock(&m_mtx);
        IOReturn err = IOSurfaceLock(frame, kIOSurfaceLockReadOnly, NULL);
        if(err != kIOReturnCannotLock) {
            uchar *src = (uchar*)IOSurfaceGetBaseAddress(frame);
            int step = (int)IOSurfaceGetBytesPerRow(frame);
            for(int i = 0; i < numRects; i++) {
                if(m_retina) {
                    QRect rt((int)rects[i].origin.x / 2, (int)rects[i].origin.y / 2, (int)rects[i].size.width / 2, (int)rects[i].size.height / 2);
                    rt = rt.intersected(m_imageRt);
                    image_copy_from_retina((uchar*)m_image.data(), m_imageRt.width() * 4, rt.x(), rt.y(), rt.width(), rt.height(), src, step, rt.x(), rt.y());
                }else{
                    QRect rt((int)rects[i].origin.x, (int)rects[i].origin.y, (int)rects[i].size.width, (int)rects[i].size.height);
                    rt = rt.intersected(m_imageRt);
                    image_copy((uchar*)m_image.data(), m_imageRt.width() * 4, rt.x(), rt.y(), rt.width(), rt.height(), src, step, rt.x(), rt.y());
                }
            }
            m_frameIndex++;
            //qDebug() << "image_copy" << rt << step;
            IOSurfaceUnlock(frame, kIOSurfaceLockReadOnly, NULL);
        }
    }

    bool init(int idx) {
        CGDisplayCount displayCount;
        CGDirectDisplayID displays[32];

        // grab the active displays
        CGGetActiveDisplayList(32, displays, &displayCount);
        if(idx >= displayCount) {
            return false;
        }
        m_displayId = displays[idx];
        m_displayId = CGMainDisplayID();
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(m_displayId);
        int pixelWidth = CGDisplayModeGetPixelWidth(mode);
        int pixelHeight = CGDisplayModeGetPixelHeight(mode);
        CGDisplayModeRelease(mode);
        int wide = CGDisplayPixelsWide(m_displayId);
        int high = CGDisplayPixelsHigh(m_displayId);
        m_retina = (pixelWidth / wide) == 2;
        if(m_stream) {
            CGError err = CGDisplayStreamStop(m_stream);
            if(err != kCGErrorSuccess) {
                qWarning() << "failed to start the display stream capturer. CGDisplayStreamStart failed" << err;
            }
            m_stream = {};
        }

        void* keys[2];
        void* values[2];
        CFDictionaryRef opts;
        keys[0] = (void*)kCGDisplayStreamShowCursor;
        values[0] = (void*)kCFBooleanFalse;
        opts = CFDictionaryCreate(kCFAllocatorDefault, (const void**)keys, (const void**)values, 1, NULL, NULL);
        m_stream = CGDisplayStreamCreateWithDispatchQueue(m_displayId,
                                                          pixelWidth,
                                                          pixelHeight,
                                                          'BGRA',
                                                          NULL,
                                                          m_queue,
                                                          ^(CGDisplayStreamFrameStatus status, uint64_t displayTime, IOSurfaceRef frameSurface, CGDisplayStreamUpdateRef updateRef) {
              this->callback(status, displayTime, frameSurface, updateRef);
        });
        CFRelease(opts);
        CGError err = CGDisplayStreamStart(m_stream);
        if (err != kCGErrorSuccess) {
            qWarning() << "failed to start the display stream capturer. CGDisplayStreamStart failed" << err;
            return 1;
        }        
        return true;
    }   

    int getFrame() {
        return 0;
    }    

    int drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset) {
        // the otpimization below is workable under vnc standard draw mode such as RLE/TRLE etc.
        // but maybe bad or delay on h264.
        // so mark it and never use it.
#ifdef DRAW_DELAY_WHEN_H264_UNDER_MULT_THREAD
        if(m_frameLastIndex == m_frameIndex) {
            return 0;
        }

        m_frameLastIndex = m_frameIndex;
#endif
        return drawFrame(req, pbuf, bytesPerLine);
    }

    int drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine) {
        QMutexLocker lock(&m_mtx);
        QDataStream &ds = req->ds;
        int width = m_imageRt.width();
        int height = m_imageRt.height();

        EEncodingType et = req->match(width, height, req);
        if(et == ET_OpenH264) {
            req->srcHeader = (quint8*)m_image.data();
            req->dstHeader = pbuf;
            req->imageSize = QSize(width, height);
            req->updateRect = QRect(0, 0, width, height);
            PFunCompress func = req->codes.value(et);
            req->buf.clear();
            req->buf.reserve(width * 4 * height);
            ds << quint16(m_imageRt.left()) << quint16(m_imageRt.top()) << quint16(width) << quint16(height) << quint32(et);
            func((uchar*)pbuf, bytesPerLine, (uchar*)m_image.data(), width * 4, width, height, req);
            return 1;
        }

        QKxDirtyFrame df(width, height);
        if(!df.findDirtyRect(pbuf, bytesPerLine, (uchar*)m_image.data(), width * 4)) {
            return 0;
        }
        QList<QRect> rts = df.dirtyRects();
        for(int i = 0; i < rts.length(); i++) {
            QRect rt = rts.at(i);
            req->srcHeader = (uchar*)m_image.data();
            req->dstHeader = pbuf;
            req->imageSize = QSize(width, height);
            req->updateRect = rt;

            EEncodingType et = req->match(rt.width(), rt.height(), req);
            ds << quint16(rt.left() + m_imageRt.left()) << quint16(rt.top() + m_imageRt.top()) << quint16(rt.width()) << quint16(rt.height()) << quint32(et);
            PFunCompress func = req->codes.value(et);

            uchar *dst = (uchar*)pbuf;
            dst += rt.top() * bytesPerLine;
            dst += rt.left() * 4;
            uchar *src = (uchar*)m_image.data();
            src += rt.top() * width * 4;
            src += rt.left() * 4;

            func(dst, bytesPerLine, src, width * 4, rt.width(), rt.height(), req);
        }
        return rts.length();
    }
};

QKxMacCapture::QKxMacCapture(int idx, const QRect &imageRt, const QRect &screenRt, QObject *parent)
    : QKxAbstractCapture(parent)
    , m_imageRt(imageRt)
    , m_idx(idx)
{
    m_prv = new MacCapturePrivate(idx, imageRt, screenRt, this);
}

QKxMacCapture::~QKxMacCapture()
{
    delete m_prv;
}

bool QKxMacCapture::reset()
{
    return m_prv->init(m_idx);
}

int QKxMacCapture::getFrame()
{
    return m_prv->getFrame();
}

int QKxMacCapture::drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset)
{
    return m_prv->drawFrame(req, pbuf, bytesPerLine, reset);
}
