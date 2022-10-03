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

#ifndef QKXCAP_PRIVATE_H
#define QKXCAP_PRIVATE_H

#include <QtGlobal>
#include <QObject>
#include <QRect>
#include <QByteArray>
#include <QVector>
#include <QMap>
#include <QDataStream>

enum ESecurityType {
    ST_Invalid = 0,
    ST_None = 1,
    ST_VNCAuth = 2,
    ST_RA2 = 5,
    ST_RA2ne = 6,
    ST_Tight = 16,
    ST_Ultra = 17,
    ST_TLS = 18,
    ST_VeNCrypt = 19,
    ST_GTKVNCSASL = 20,
    ST_MD5Hash = 21,
    ST_ColinDeanXvp = 22
};

enum ECSMessageType {
    ECST_SetPixelFormat = 0,
    ECST_SetEncoding = 2,
    ECST_FramebufferUpdateRequest = 3,
    ECST_KeyEvent = 4,
    ECST_PointerEvent = 5,
    ECST_ClientCutText = 6,

    ECST_ScreenBlack = 100,
    ECST_LockScreen = 101,
    ECST_Ftp = 102,
    ECST_PlayAudio = 103,
    ECST_PrivacyScreen = 104
};

enum EEncodingType
{
    ET_Raw = 0,
    ET_CopyRect = 1,
    ET_RRE = 2,
    ET_Hextile = 5,
    ET_TRLE = 15,
    ET_ZRLE = 16,
    ET_OpenH264 = 50, // follow the rfbproto.
    ET_JPEG = 51, // follow the rfbproto.
    ET_TRLE2 = 100,
    ET_ZRLE2 = 101,
    ET_TRLE3 = 102,
    ET_ZRLE3 = 103,
    ET_WoVNCScreenCount = -100,
    ET_WoVNCMessageSupport = -101,
    ET_CursorPseudoEncoding = -239,
    ET_DesktopSizePseudoEncoding = -223,
    ET_FillPadding = 0x7FFFFFFF
};

#pragma pack(push, 1)
struct PixelFormat {
    quint8 bitsPerPixel;
    quint8 depth;
    quint8 bigEndian;
    quint8 trueColor;
    quint16 redMax;
    quint16 greenMax;
    quint16 blueMax;
    quint8 redShift;
    quint8 greenShift;
    quint8 blueShift;
    quint8 extendFormat; //0:default and compatible, 1: yuv, 2: h264
    quint8 extendQuality; // 0:high, 1:normal, 2:low.
    quint8 reserved;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct FrameInfo {
    quint16 width;
    quint16 height;
    PixelFormat pixel;
};
#pragma pack(pop)

typedef EEncodingType (*PFunMatchBest)(quint16 width, quint16 height, void *p);
typedef int (*PFunCompress)(uchar *dst, int dstPitch, uchar *src, int srcPitch, quint16 width, quint16 height, void *p);
class QKxZip;
class QKxH264Encoder;
struct UpdateRequest {
    QByteArray buf;
    QDataStream ds;
    PFunMatchBest match;
    QMap<EEncodingType, PFunCompress> codes;
    PixelFormat fmt;
    qint8 bitUsed;
    QKxZip *zip;
    QKxH264Encoder *h264;

    quint8 *srcHeader;
    quint8 *dstHeader;
    QRect updateRect;
    QSize imageSize;
};

#endif // QKXCAP_PRIVATE_H
