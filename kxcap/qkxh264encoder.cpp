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

#include "qkxh264encoder.h"

#include <wels/codec_api.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDataStream>
#include <QLibrary>
#include <QFileInfo>
#include <QDir>

#ifdef Q_OS_WIN
#include <Windows.h>
typedef long (WINAPI *PFUNWelsCreateSVCEncoder)(ISVCEncoder** ppEncoder);
typedef void (WINAPI *PFUNWelsDestroySVCEncoder)(ISVCEncoder* pEncoder);
static PFUNWelsCreateSVCEncoder pWelsCreateSVCEncoder = nullptr;
static PFUNWelsDestroySVCEncoder pWelsDestroySVCEncoder = nullptr;

#define LIBOPENH264_NAME        ("openh264.dll")
#else
typedef int ( *PFUNWelsCreateSVCEncoder)(ISVCEncoder** ppEncoder);
typedef void (  *PFUNWelsDestroySVCEncoder)(ISVCEncoder* pEncoder);
static PFUNWelsCreateSVCEncoder pWelsCreateSVCEncoder = nullptr;
static PFUNWelsDestroySVCEncoder pWelsDestroySVCEncoder = nullptr;
#define LIBOPENH264_NAME        ("libopenh264.so")
#endif

#define RESOLVE(name) p##name = (PFUN##name)lib.resolve(#name);

class QKxH264EncoderPrivate
{
private:
    QKxH264Encoder *m_p;
    ISVCEncoder *m_encoder;
    int m_width, m_height;
    bool m_reset;
public:
    explicit QKxH264EncoderPrivate(QKxH264Encoder *p)
        : m_p(p) {
        m_encoder = nullptr;
        m_reset = false;
#if 1
        int rv = WelsCreateSVCEncoder(&m_encoder);
        qDebug() << "QKxH264DecoderPrivate:" << rv;
#else
        if(pWelsCreateSVCEncoder == nullptr) {
            // only full path can be work, why?.
            QString path = QString("%1/%2").arg(QCoreApplication::applicationDirPath()).arg(LIBOPENH264_NAME);
            path = QDir::cleanPath(path);
            QLibrary lib(path);
            RESOLVE(WelsCreateSVCEncoder);
            RESOLVE(WelsDestroySVCEncoder);
        }
        if(pWelsCreateSVCEncoder) {
            int rv = pWelsCreateSVCEncoder(&m_encoder);
            qDebug() << "QKxH264DecoderPrivate:" << rv;
        }
#endif
    }

    ~QKxH264EncoderPrivate() {
#if 1
        if(m_encoder != nullptr) {
            m_encoder->Uninitialize();
            WelsDestroySVCEncoder(m_encoder);
        }
#else
        if(m_encoder != nullptr) {
            m_encoder->Uninitialize();
            pWelsDestroySVCEncoder(m_encoder);
        }
#endif
    }

    // SEncParamExt
    bool init(int width, int height, bool isCamera) {
        if(m_encoder == nullptr) {
            return false;
        }
        m_width = width;
        m_height = height;
        SEncParamExt param;
        prepareParamDefault(1, 2, width, height, 30, &param);
        param.iUsageType = isCamera ? CAMERA_VIDEO_REAL_TIME : SCREEN_CONTENT_REAL_TIME; //from EUsageType enum;
        param.iTargetBitrate = 500 * 1000 * 10;
        param.bEnableDenoise = false;
        param.bEnableFrameSkip = true;
        param.fMaxFrameRate = 30;
        param.iRCMode = RC_QUALITY_MODE;
        param.iMaxQp = 40;
        param.iMinQp = 16;
        int err = m_encoder->InitializeExt(&param);
        return err == 0;
    }

    void prepareParam (int iLayers, int iSlices, int width, int height, float framerate, SEncParamExt* pParam) {
        pParam->iUsageType = SCREEN_CONTENT_REAL_TIME;
        pParam->iPicWidth = width;
        pParam->iPicHeight = height;
        pParam->fMaxFrameRate = framerate;
        pParam->iRCMode = RC_QUALITY_MODE; //rc off
        pParam->iMultipleThreadIdc = 1; //single thread
        pParam->iSpatialLayerNum = iLayers;
        pParam->iNumRefFrame = AUTO_REF_PIC_COUNT;
        for (int i = 0; i < iLayers; i++) {
            pParam->sSpatialLayers[i].iVideoWidth = width >> (iLayers - i - 1);
            pParam->sSpatialLayers[i].iVideoHeight = height >> (iLayers - i - 1);
            pParam->sSpatialLayers[i].fFrameRate = framerate;
            pParam->sSpatialLayers[i].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
            pParam->sSpatialLayers[i].sSliceArgument.uiSliceNum = iSlices;
        }
    }

    void prepareParamDefault(int iLayers, int iSlices, int width, int height, float framerate, SEncParamExt* pParam) {
        memset(pParam, 0, sizeof (SEncParamExt));
        m_encoder->GetDefaultParams(pParam);
        prepareParam(iLayers, iSlices, width, height, framerate, pParam);
    }

    void setQualityLevel(int lv) {
        if(m_encoder == nullptr) {
            return;
        }
        if(lv == 3) {
            SBitrateInfo info;
            info.iBitrate = 800*1000*10; // IPTV HD: 8Mbps.
            info.iLayer = SPATIAL_LAYER_ALL;
            m_encoder->SetOption(ENCODER_OPTION_MAX_BITRATE, (void*)&info);
            m_encoder->SetOption(ENCODER_OPTION_BITRATE, (void*)&info);
            m_encoder->ForceIntraFrame(true);
            m_reset = true;
        }else if(lv == 2) {
            SBitrateInfo info;
            info.iBitrate = 320*1000*10;
            info.iLayer = SPATIAL_LAYER_ALL;
            m_encoder->SetOption(ENCODER_OPTION_MAX_BITRATE, (void*)&info);
            m_encoder->SetOption(ENCODER_OPTION_BITRATE, (void*)&info);
            m_encoder->ForceIntraFrame(true);
            m_reset = true;
        }else if(lv == 1) {
            SBitrateInfo info;
            info.iBitrate = 160*1000*10;
            info.iLayer = SPATIAL_LAYER_ALL;
            m_encoder->SetOption(ENCODER_OPTION_MAX_BITRATE, (void*)&info);
            m_encoder->SetOption(ENCODER_OPTION_BITRATE, (void*)&info);
            m_encoder->ForceIntraFrame(true);
            m_reset = true;
        }else{
            SBitrateInfo info;
            info.iBitrate = 80*1000*10;
            info.iLayer = SPATIAL_LAYER_ALL;
            m_encoder->SetOption(ENCODER_OPTION_MAX_BITRATE, (void*)&info);
            m_encoder->SetOption(ENCODER_OPTION_BITRATE, (void*)&info);
            m_encoder->ForceIntraFrame(true);
            m_reset = true;
        }
    }

    int encode(QDataStream &ds, uchar *src) {
        if(m_encoder == nullptr) {
            return -1;
        }
        SFrameBSInfo info;
        memset(&info, 0, sizeof (SFrameBSInfo));

        int width = m_width;
        int height = m_height;
        int ystride = (width + 3) / 4 * 4; // 4 byte align.
        int ustride = ystride / 2;
        int vstride = ustride;

        SSourcePicture pic;
        memset(&pic, 0, sizeof (SSourcePicture));
        pic.iPicWidth = width;
        pic.iPicHeight = height;
        pic.iColorFormat = videoFormatI420;
        pic.iStride[0] = ystride;
        pic.iStride[1] = ustride;
        pic.iStride[2] = vstride;
        pic.pData[0] = src;
        pic.pData[1] = src + ystride * height;
        pic.pData[2] = pic.pData[1] + ustride * height / 2;
        int rv = m_encoder->EncodeFrame(&pic, &info);
        int total = 0;
        if(rv == cmResultSuccess) {
            if(info.eFrameType != videoFrameTypeSkip) {
                for(int i = 0; i < info.iLayerNum; ++i) {
                    SLayerBSInfo& layerInfo = info.sLayerInfo[i];
                    for(int j = 0; j < layerInfo.iNalCount; ++j) {
                        int cnt = layerInfo.pNalLengthInByte[j];
                        total += cnt;
                    }
                }
                //qDebug() << "frameType:" << info.eFrameType;
                ds.writeRawData((char*)info.sLayerInfo[0].pBsBuf, total);
                if(m_reset && info.eFrameType == videoFrameTypeIDR) {
                    m_reset = false;
                    return 1;
                }
                return 2;
            }
            return 0;
        }
        return rv - 1000;
    }
};

QKxH264Encoder::QKxH264Encoder(QObject *parent)
    : QObject(parent)
{
    m_prv = new QKxH264EncoderPrivate(this);
}

QKxH264Encoder::~QKxH264Encoder()
{
    delete m_prv;
}

bool QKxH264Encoder::init(int width, int height, bool isCamera)
{
    return m_prv->init(width, height, isCamera);
}

void QKxH264Encoder::setQualityLevel(int lv)
{
    m_prv->setQualityLevel(lv);
}

int QKxH264Encoder::encode(QDataStream &ds, uchar *src)
{
    return m_prv->encode(ds, src);
}


