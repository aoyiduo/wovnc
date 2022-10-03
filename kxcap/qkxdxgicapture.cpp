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

#include "qkxdxgicapture.h"
#include "qkxcap_private.h"
#include "qkxwindows.h"
#include "qkxutils.h"
#include "qkxvnccompress.h"

#include <windows.h>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi.h>

#include <dxgi.h>
#include <d3d11.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <Psapi.h>

#include <QDebug>
#include <QImage>
#include <QDateTime>
#include <QThread>

#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"Shcore.lib")
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"windowscodecs.lib")
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "dxguid.lib")

#define COUNTOF(e) (sizeof(e)/sizeof(e[0]))

class DXGICapturePrivate
{
    typedef struct MouseAreaBackup {
        QRect rt;
        QByteArray data;
    }MouseAreaBackup;

    typedef struct MouseData {
        DXGI_OUTDUPL_POINTER_SHAPE_INFO shape;
        QByteArray cursor;
        bool visible;
        quint64 updateTime;
        POINT pt;

        MouseAreaBackup last;
    }MouseData;


    typedef struct FrameData {
        ID3D11Texture2D *pDeskTexture;
        ID3D11Texture2D *pCacheTexture;
        IDXGISurface *pCacheSurface;
        QByteArray clip;
        DXGI_OUTDUPL_MOVE_RECT *pMove;
        int moveCount;
        RECT *pDirty;
        int dirtyCount;
    }FrameData;

private:
    QKxDXGICapture* m_pub = nullptr;
    ID3D11Device *m_pDevice = nullptr;
    ID3D11DeviceContext *m_pContext = nullptr;
    IDXGIFactory1 *m_pFactory = nullptr;
    IDXGIOutputDuplication* m_pDuplication = nullptr;
    DXGI_OUTPUT_DESC m_outDesc = DXGI_OUTPUT_DESC{0};
    DXGI_OUTDUPL_DESC m_dupDesc = DXGI_OUTDUPL_DESC{0};
    DXGI_OUTDUPL_FRAME_INFO m_info = DXGI_OUTDUPL_FRAME_INFO{}; // frame info.
    FrameData m_frame=FrameData{0}; // desktop image.
    MouseData m_mouse=MouseData{0}; // mouse info.
    QRect m_imageRt;
    QRect m_screenRt;
    QByteArray m_pixel, m_copy;
    QRect m_updateRect, m_mouseRect;
public:
    explicit DXGICapturePrivate(const QRect& imageRt, const QRect& screenRt, QKxDXGICapture *p)
        : m_pub(p)
        , m_imageRt(imageRt)
        , m_screenRt(screenRt) {
        m_pixel.resize(imageRt.width() * 4 * imageRt.height());
        m_copy.resize(imageRt.width() * 4 * imageRt.height());
    }

    ~DXGICapturePrivate() {
        if(m_frame.pCacheSurface) {
            m_frame.pCacheSurface->Release();
        }
        if(m_frame.pCacheTexture) {
            m_frame.pCacheTexture->Release();
        }
        if(m_frame.pDeskTexture) {
            m_frame.pDeskTexture->Release();
        }
        if(m_pDuplication) {
            m_pDuplication->Release();
            m_pDuplication = nullptr;
        }
        if(m_pContext) {
            m_pContext->Release();
        }
        if(m_pDevice) {
            m_pDevice->Release();
        }
        if(m_pFactory) {
            m_pFactory->Release();
            m_pFactory = nullptr;
        }
    }

    bool init(int idx) {
        HRESULT hr = CreateDXGIFactory1(IID_IDXGIFactory1, reinterpret_cast<void**>(&m_pFactory));
        if(FAILED(hr)) {
            m_pub->showError(QObject::tr("Error create dxgi factory: %1").arg(hr));
            return false;
        }
        D3D_FEATURE_LEVEL feature_levels[] =
        {
            //D3D_FEATURE_LEVEL_12_1,
            //D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1,
        };

        D3D_FEATURE_LEVEL fl;
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                               feature_levels, COUNTOF(feature_levels), D3D11_SDK_VERSION, &m_pDevice, &fl, &m_pContext);
        if(FAILED(hr)) {
            m_pub->showError(QObject::tr("Error create d3d11 device: %1").arg(QString::number(hr, 16)));
            return false;
        }
        IDXGIDevice* pDxgiDevice = nullptr;
        hr = m_pDevice->QueryInterface(IID_IDXGIDevice, reinterpret_cast<void**>(&pDxgiDevice));
        if(FAILED(hr)) {
            m_pub->showError(QObject::tr("Failed to QI for DXGI Device: %1").arg(QString::number(hr, 16)));
            return false;
        }
        IDXGIAdapter* pDxgiAdapter = nullptr;
        hr = pDxgiDevice->GetParent(IID_IDXGIAdapter, reinterpret_cast<void**>(&pDxgiAdapter));
        pDxgiDevice->Release();
        if(FAILED(hr)) {
            m_pub->showError(QObject::tr("Failed to get parent DXGI Adapte: %1").arg(QString::number(hr, 16)));
            return false;
        }
        IDXGIOutput* pDxgiOutput;
        hr = pDxgiAdapter->EnumOutputs(idx, &pDxgiOutput);
        pDxgiAdapter->Release();
        if (FAILED(hr)) {
            m_pub->showError(QObject::tr("Failed to get specified output in DUPLICATIONMANAGER:%1").arg(QString::number(hr, 16)));
            return false;
        }
        pDxgiOutput->GetDesc(&m_outDesc);
        qDebug() << "pDxgiOutput_m_outDesc"
                 << m_outDesc.AttachedToDesktop
                 << m_outDesc.DeviceName
                 << m_outDesc.DesktopCoordinates.right - m_outDesc.DesktopCoordinates.left
                 << m_outDesc.DesktopCoordinates.bottom - m_outDesc.DesktopCoordinates.top;
        IDXGIOutput1* pDxgiOutput1;
        hr = pDxgiOutput->QueryInterface(IID_IDXGIOutput1, reinterpret_cast<void**>(&pDxgiOutput1));
        pDxgiOutput->Release();
        if (FAILED(hr)) {
            m_pub->showError(QObject::tr("Failed to QI for DxgiOutput1 in DUPLICATIONMANAGER：%1").arg(QString::number(hr, 16)));
            return false;
        }
        // Create desktop duplication
        hr = pDxgiOutput1->DuplicateOutput(m_pDevice, &m_pDuplication);
        pDxgiOutput1->Release();
        if (FAILED(hr)) {
            if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
                m_pub->showError(QObject::tr("There is already the maximum number of applications using the Desktop Duplication API running, please close one of those applications and then try again.").arg(hr));
                return false;
            }
            m_pub->showError(QObject::tr("Failed to get duplicate output in DUPLICATIONMANAGER").arg(QString::number(hr, 16)));
            return false;
        }
        m_pDuplication->GetDesc(&m_dupDesc);
        if(m_dupDesc.ModeDesc.Format != DXGI_FORMAT_B8G8R8A8_UNORM) {
            m_pub->showError(QObject::tr("duplication format is not support.").arg(QString::number(hr, 16)));
            return false;
        }
        D3D11_TEXTURE2D_DESC dst;
        memset(&dst, 0, sizeof(dst));
        dst.Width = m_dupDesc.ModeDesc.Width;
        dst.Height = m_dupDesc.ModeDesc.Height;
        dst.MipLevels = 1;
        dst.ArraySize = 1;
        dst.SampleDesc.Count = 1;
        dst.SampleDesc.Quality = 0;
        dst.Usage = D3D11_USAGE_STAGING;
        dst.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // DXGI_FORMAT_B8G8R8A8_UNORM;
        dst.BindFlags = 0;
        dst.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        dst.MiscFlags = 0;

        ID3D11Texture2D *pCache = nullptr;
        if(FAILED(m_pDevice->CreateTexture2D(&dst, nullptr, &pCache))) {
            m_pub->showError(QObject::tr("Failed to create cache texture."));
            return false;
        }
        m_frame.pCacheTexture = pCache;
        IDXGISurface *pSurface = nullptr;
        if(FAILED(pCache->QueryInterface(IID_IDXGISurface, reinterpret_cast<void**>(&pSurface)))) {
            m_pub->showError(QObject::tr("Failed to create cache texture."));
            return false;
        }        
        m_frame.pCacheSurface = pSurface;
        return true;
    }

    bool isDeviceRemove() {
        HRESULT reason = m_pDevice->GetDeviceRemovedReason();
        switch(reason) {
        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DEVICE_RESET:
        case E_OUTOFMEMORY:
            return true;
        case S_OK:
            return false;
        }
        return true;
    }

    QRect desktopGeometry() {
        RECT& rt = m_outDesc.DesktopCoordinates;
        return QRect(rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top);
    }

    int getFrame() {
        DXGI_OUTDUPL_FRAME_INFO info;
        IDXGIResource *pDxgiResource;
        HRESULT hr = m_pDuplication->AcquireNextFrame(0, &info, &pDxgiResource);
        if(hr == DXGI_ERROR_WAIT_TIMEOUT) {
            return hr;
        }else if(hr == DXGI_ERROR_ACCESS_LOST) {
            return hr;
        }else if(hr == DXGI_ERROR_INVALID_CALL) {
            return hr;
        }

        if (FAILED(hr)) {
            qDebug() << "AcquireNextFrame:Error" << QString::number(hr, 16);
            return hr;
        }        
        // desktop image.
        if(m_frame.pDeskTexture) {
            m_frame.pDeskTexture->Release();
            m_frame.pDeskTexture = nullptr;
        }
        hr = pDxgiResource->QueryInterface(IID_ID3D11Texture2D, reinterpret_cast<void **>(&m_frame.pDeskTexture));
        pDxgiResource->Release();
        if(FAILED(hr)) {
            qDebug() << QObject::tr("Failed to QI for ID3D11Texture2D from acquired IDXGIResource in DUPLICATIONMANAGER:%1").arg(hr);
            return hr;
        }
        m_frame.dirtyCount = m_frame.moveCount = 0;
        if(info.TotalMetadataBufferSize > 0) {
            // update rect.
            if(info.TotalMetadataBufferSize > m_frame.clip.length()) {
                m_frame.clip.resize(info.TotalMetadataBufferSize);
            }
            quint32 bufSize = 11;
            m_frame.pMove = reinterpret_cast<DXGI_OUTDUPL_MOVE_RECT*>(m_frame.clip.data());
            hr = m_pDuplication->GetFrameMoveRects(info.TotalMetadataBufferSize, m_frame.pMove, &bufSize);
            if(FAILED(hr)){
                qDebug() << QObject::tr("Error: Failed GetFrame Move Rect:%1").arg(hr);
                return hr;
            }
            m_frame.moveCount = bufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);
            m_frame.pDirty =reinterpret_cast<RECT*>(m_frame.clip.data() + bufSize);
            hr = m_pDuplication->GetFrameDirtyRects(info.TotalMetadataBufferSize - bufSize, reinterpret_cast<RECT*>(m_frame.pDirty), &bufSize);
            if(FAILED(hr)) {
                qDebug() << QObject::tr("Error: Failed GetFrame Dirty Rect:%1").arg(hr);
                return hr;
            }
            m_frame.dirtyCount = bufSize / sizeof(RECT);
        }

        if(info.LastMouseUpdateTime.QuadPart > m_mouse.updateTime) {
            // mouse.
            m_mouse.pt = info.PointerPosition.Position;
            m_mouse.updateTime = info.LastMouseUpdateTime.QuadPart;
            m_mouse.visible = info.PointerPosition.Visible;

            if(info.PointerShapeBufferSize > m_mouse.cursor.length()) {
                m_mouse.cursor.resize(info.PointerShapeBufferSize);
            }
            if(info.PointerShapeBufferSize > 0) {
                UINT bufSize;
                HRESULT hr = m_pDuplication->GetFramePointerShape(info.PointerShapeBufferSize,
                                                                  reinterpret_cast<VOID*>(m_mouse.cursor.data()),
                                                                  &bufSize,
                                                                  &m_mouse.shape);
                if(FAILED(hr)) {
                    qDebug() << QObject::tr("Failed to get frame pointer shape in DUPLICATIONMANAGER:%1").arg(hr);
                    return hr;
                }
            }
        }
        m_info = info;

        copyFrame((uchar*)m_copy.data(), m_imageRt.width() * 4, 0, 0, m_imageRt.width(), m_imageRt.height());
        m_updateRect = mergeUpdateRect();
        drawMouse((uchar*)m_copy.data(), m_imageRt.width() * 4);
        qDebug() << "boundRt" << m_updateRect;
        return 0;
    }

    void releaseFrame() {
        HRESULT hr1 = m_pDuplication->ReleaseFrame();
        if(m_frame.pDeskTexture) {
            m_frame.pDeskTexture->Release();
            m_frame.pDeskTexture = nullptr;
        }
    }

    int copyFrame(uchar *pRgbbuf, int bytePerline, int x, int y, int width, int height) {
        ID3D11Texture2D *pCopy = m_frame.pCacheTexture;
        IDXGISurface *pSurface = m_frame.pCacheSurface;
        D3D11_BOX box;

        box.front = 0;
        box.back = 1;
        box.left = x;
        box.top = y;
        box.right = width;
        box.bottom = height;
        m_pContext->CopySubresourceRegion(static_cast<ID3D11Resource*>(pCopy), 0, 0, 0, 0, static_cast<ID3D11Resource*>(m_frame.pDeskTexture), 0, &box);
        DXGI_MAPPED_RECT rgba;
        if(FAILED(pSurface->Map(&rgba, DXGI_MAP_READ))) {
            return -1;
        }
        uchar *dst = pRgbbuf;
        uchar *src = rgba.pBits;
        QKxUtils::copyRgb32(dst, m_imageRt.width() * 4, rgba.pBits, rgba.Pitch, m_imageRt.width(), m_imageRt.height());
        pSurface->Unmap();
        return 0;
    }

    bool drawMouse(uchar *pRgbbuf, int bytesPerLine) {
        if(m_mouse.updateTime > 0) {
            int left = m_mouse.pt.x;
            int top = m_mouse.pt.y;

            //qDebug() << "left" << left << "top" << top;
            if(left >= 0 && left < m_dupDesc.ModeDesc.Width && top >= 0 && top < m_dupDesc.ModeDesc.Height) {
                int cursor_width = m_mouse.shape.Width;
                int cursor_height = m_mouse.shape.Height;
                if(m_mouse.shape.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME){
                    cursor_height /= 2;
                }
                int cw = min(m_dupDesc.ModeDesc.Width - left, cursor_width);
                int ch = min(m_dupDesc.ModeDesc.Height - top, cursor_height);                         

                uchar *src = reinterpret_cast<uchar*>(m_mouse.cursor.data());
                uchar *dst = pRgbbuf + top * bytesPerLine + left * 4;
                //ds << quint16(left) << quint16(top) << quint16(cw) << quint16(ch) << quint32(et);
                if(m_mouse.shape.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR) {
                    for(int r = 0; r < ch; r++) {
                        quint32 *psrc = reinterpret_cast<quint32*>(src + r * m_mouse.shape.Pitch);
                        quint32 *pdst = reinterpret_cast<quint32*>(dst + r * bytesPerLine);
                        for(int c = 0; c < cw; c++) {
                            quint32 val = psrc[c];
                            if(val == 0x00000000) {
                                continue;
                            }else{
                                pdst[c] = val;
                            }
                        }
                    }
                } else if(m_mouse.shape.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME) {
                    for(int r = 0; r < ch; r++) {
                        uchar *psrc = reinterpret_cast<uchar*>(src + r * m_mouse.shape.Pitch);
                        quint32 *pdst = reinterpret_cast<quint32*>(dst + r * bytesPerLine);
                        uchar mask = 0x80;
                        for(int c = 0; c < cw; c++) {
                            uchar andMask = psrc[c / 8] & mask;
                            uchar xorMask = psrc[(c / 8) + (cursor_height * m_mouse.shape.Pitch)] & mask;
                            quint32 andMask32 = andMask ? 0xFFFFFFFF : 0xFF000000;
                            quint32 xorMask32 = xorMask ? 0x00FFFFFF : 0x00000000;
                            pdst[c] = (pdst[c] & andMask32) ^ xorMask32;

                            mask = mask == 0x01 ? 0x80 : mask >> 1;
                        }
                    }
                    //qDebug() << "mono" <<QDateTime::currentMSecsSinceEpoch();
                } else if(m_mouse.shape.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR) {
                    for(int r = 0; r < ch; r++) {
                        quint32 *psrc = reinterpret_cast<quint32*>(src + r * m_mouse.shape.Pitch);
                        quint32 *pdst = reinterpret_cast<quint32*>(dst + r * bytesPerLine);
                        for(int c = 0; c < cw; c++) {
                            quint32 cval = psrc[c];
                            quint32 sval = pdst[c];
                            quint32 mask = 0xFF000000 & cval;
                            pdst[c] = mask ? ((sval ^ cval) | 0xFF000000) : (cval | 0xFF000000);
                        }
                    }
                }

                m_mouseRect.setRect(left, top, cw, ch);
                return true;
            }
        }
        return false;
    }

    QRect mergeUpdateRect() {
        // draw move rect
        DXGI_OUTDUPL_MOVE_RECT *pMove = m_frame.pMove;
        QRect rt;
        for(int i = 0; i < m_frame.moveCount; i++) {
            if(rt.isEmpty()) {
                rt.setRect(pMove->DestinationRect.left,
                           pMove->DestinationRect.top,
                           pMove->DestinationRect.right - pMove->DestinationRect.left,
                           pMove->DestinationRect.bottom - pMove->DestinationRect.top);
            }else {
                rt = rt.united(QRect(pMove->DestinationRect.left,
                                pMove->DestinationRect.top,
                                pMove->DestinationRect.right - pMove->DestinationRect.left,
                                pMove->DestinationRect.bottom - pMove->DestinationRect.top));
            }
            pMove++;
        }
        //draw dirty rect
        RECT *pDirty = m_frame.pDirty;
        for(int i = 0; i < m_frame.dirtyCount; i++) {
            if(rt.isEmpty()) {
                rt.setRect(pDirty->left,
                           pDirty->top,
                           pDirty->right - pDirty->left,
                           pDirty->bottom - pDirty->top);
            }else {
                rt = rt.united(QRect(pDirty->left,
                                pDirty->top,
                                pDirty->right - pDirty->left,
                                pDirty->bottom - pDirty->top));
            }
            pDirty++;
        }
        return rt;
    }

    QRect mergeMouseArea(uchar *pbuf, QRect rt){
        if(m_mouse.last.rt == rt) {
            int srcStep = m_imageRt.width() * 4;
            int dstStep = rt.width() * 4;
            uchar *dst = reinterpret_cast<uchar*>(m_mouse.last.data.data());
            uchar *src = pbuf + rt.top() * srcStep + rt.left() * 4;
            bool isSame = false;
            for(int r = 0; r < rt.height(); r++) {
                if(!isSame) {
                    isSame = memcmp(dst, src, rt.width() * 4);
                }
                memcpy(dst, src, rt.width() * 4);
                dst += dstStep;
                src += srcStep;
            }

            return isSame ? QRect() : rt;
        }else if(rt.isEmpty()) {
            rt = m_mouse.last.rt;
            m_mouse.last.rt = QRect();
            m_mouse.last.data.clear();
            return rt;
        }
        QRect mrt = m_mouse.last.rt;
        int srcStep = m_imageRt.width() * 4;
        int dstStep = rt.width() * 4;
        m_mouse.last.rt = rt;
        m_mouse.last.data.resize(rt.width() * rt.height() * 4);
        uchar *dst = reinterpret_cast<uchar*>(m_mouse.last.data.data());
        uchar *src = pbuf + rt.top() * srcStep + rt.left() * 4;
        for(int r = 0; r < rt.height(); r++) {
            memcpy(dst, src, rt.width() * 4);
            dst += dstStep;
            src += srcStep;
        }
        if(mrt.isEmpty()) {
            return rt;
        }
        return mrt.united(rt);
    }

    int drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset) {
        QDataStream &ds = req->ds;
        int width = m_imageRt.width();
        int height = m_imageRt.height();
        int rectCount = 0;
        if(m_frame.pDeskTexture == nullptr) {
            return 0;
        }
        if(m_updateRect.isEmpty()) {
            return 0;
        }
        QRect mrt = mergeMouseArea((uchar*)m_copy.data(), m_mouseRect);
        if(!mrt.isEmpty()) {
            m_updateRect = m_updateRect.united(mrt);
        }

        QRect &rt = m_updateRect;
        req->srcHeader = (uchar*)m_copy.data();
        req->dstHeader = pbuf;
        req->imageSize = QSize(width, height);
        req->updateRect = rt;
        EEncodingType et = req->match(rt.width(), rt.height(), req);
        ds << quint16(rt.left() + m_imageRt.left()) << quint16(rt.top() + m_imageRt.top()) << quint16(rt.width()) << quint16(rt.height()) << quint32(et);
        PFunCompress func = req->codes.value(et);

        uchar *dst = (uchar*)pbuf;
        dst += rt.top() * bytesPerLine;
        dst += rt.left() * 4;
        uchar *src = (uchar*)m_copy.data();
        src += rt.top() * bytesPerLine;
        src += rt.left() * 4;
        func(dst, bytesPerLine, src, bytesPerLine, rt.width(), rt.height(), req);
        return 1;
    }
};

QKxDXGICapture::QKxDXGICapture(int idx, const QRect& imageRt, const QRect& screenRt, QObject *parent)
    : QKxAbstractCapture(parent)
    , m_idx(idx)
    , m_imageRt(imageRt)
{
    m_prv = new DXGICapturePrivate(imageRt, screenRt, this);
}

QKxDXGICapture::~QKxDXGICapture()
{
    qDebug() << "~QKxDXGICapture";
    delete m_prv;
}

bool QKxDXGICapture::init()
{
    return m_prv->init(m_idx);
}

int QKxDXGICapture::getFrame()
{
    return m_prv->getFrame();
}

int QKxDXGICapture::drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset)
{
    return m_prv->drawFrame(req, pbuf, bytesPerLine, reset);
}

void QKxDXGICapture::releaseFrame()
{
    m_prv->releaseFrame();
}
