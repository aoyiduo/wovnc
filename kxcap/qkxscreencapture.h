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

#ifndef QKXSCREENCAPTURE_H
#define QKXSCREENCAPTURE_H

#include "qkxcap_private.h"

#include <QPointer>
#include <QThread>
#include <QMutex>
#include <QRect>
#include <QList>
#include <QAtomicInt>
#include <QImage>

#define MT_NEXTFRAME        (0x1)
#define MT_EXIT             (0x7f)

class QKxH264Encoder;
class QKxAbstractCapture;
class QKxPrivacyScreen;

class QKxScreenCapture : public QThread
{
    Q_OBJECT
private:
    struct MyMsg {
        uchar type;
        QByteArray data;
    };
public:
    explicit QKxScreenCapture(int screenIndex, const QRect& rt, const QRect& imgRt, QObject *parent = nullptr);
    virtual ~QKxScreenCapture();
    void setInit(PFunMatchBest match, QKxZip *zip, QMutex *zipMtx, QKxPrivacyScreen *prvscreen);
    void setFormat(const PixelFormat& fmt);
    void setCompressCode(const QMap<EEncodingType, PFunCompress> &codes);
    QRect imagePosition() const;

    void stop();
    int draw(QDataStream& stream);
signals:
    void frameResult(int idx, int err, int cnt);
private slots:
    void onNextFrame(const QRect& rt, bool full);
protected:
    void push(uchar type, const QByteArray& data = QByteArray());
    bool pop(uchar &type, QByteArray& data);
    bool takeOne(uchar type, QByteArray& data);
    void tryToNextFrame(bool drawAfter, bool full);
    void run();
    bool init();
protected:
    virtual int running() = 0;
protected:
    QImage m_frameLast;
    uchar *m_pbits;
    int m_bytesPerLine;
    QRect m_screenRect;
    QRect m_imageRect;
    int m_screenIndex;
    QAtomicInt m_queueCount;
    int m_fullRequest;
    bool m_highPerformance;

    QPointer<QKxH264Encoder> m_h264Encoder;

    QList<MyMsg> m_queue;
    QMutex m_mtx;
    int m_msgRead;
    int m_msgWrite;

    QMutex *m_zipMtx; // use for mult-screen zip compress protected.
    UpdateRequest m_request;
    int m_rectCount;

    QPointer<QKxPrivacyScreen> m_privacy;
    QPointer<QKxAbstractCapture> m_cap;
};
#endif // QKXSCREENCAPTURE_H
