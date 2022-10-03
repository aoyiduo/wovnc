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

#include "qkxscreencapture.h"
#include "qkxutils.h"
#include "qkxabstractcapture.h"
#include "qkxh264encoder.h"
#include "qkxprivacyscreen.h"

#include <QMutexLocker>
#include <QDebug>
#include <QDataStream>
#include <QFile>
#include <QDateTime>
#include <QBuffer>

#ifdef Q_OS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fcntl.h>
#define myclosesocket  closesocket
typedef SOCKET socket_t;

#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
typedef int socket_t;
#define myclosesocket(x)    close(x)
#endif


QKxScreenCapture::QKxScreenCapture(int screenIndex, const QRect &rt, const QRect &imgRt, QObject *parent)
    : QThread(parent)
    , m_screenRect(rt)
    , m_imageRect(imgRt)
    , m_screenIndex(screenIndex)
    , m_highPerformance(true)
{
    init();
    int fsize = rt.width() * 4 * rt.height(); // just do it for yuv.
    m_frameLast = QImage(rt.width(), rt.height(), QImage::Format_RGB32);
    m_pbits = (uchar*)m_frameLast.bits();
    m_bytesPerLine = m_frameLast.bytesPerLine();
    m_rectCount = 0;
    m_request.buf.reserve(100 * 1024);
    m_request.bitUsed = 0;

    m_h264Encoder = new QKxH264Encoder(this);
    m_h264Encoder->init(rt.width(), rt.height(), false);
    m_request.h264 = m_h264Encoder;
}

QKxScreenCapture::~QKxScreenCapture()
{

}

void QKxScreenCapture::setInit(PFunMatchBest match, QKxZip *zip, QMutex *zipMtx, QKxPrivacyScreen *prvscreen)
{
    QMutexLocker lock(&m_mtx);
    m_request.match = match;
    m_request.zip = zip;
    m_request.bitUsed = 0;
    m_zipMtx = zipMtx;
    m_privacy = prvscreen;
}

void QKxScreenCapture::setFormat(const PixelFormat &_fmt)
{
    QMutexLocker lock(&m_mtx);
    PixelFormat fmt = _fmt;
    if(!m_request.codes.contains(ET_OpenH264) && fmt.extendFormat == 2) {
        fmt.extendFormat = 0;
        fmt.extendQuality = 0;
    }else if(!m_request.codes.contains(ET_JPEG) && fmt.extendFormat == 3) {
        fmt.extendFormat = 0;
        fmt.extendQuality = 0;
    }
    m_request.fmt = fmt;
}

void QKxScreenCapture::setCompressCode(const QMap<EEncodingType, PFunCompress> &codes)
{
    QMutexLocker lock(&m_mtx);
    m_request.codes = codes;
}

QRect QKxScreenCapture::imagePosition() const
{
    return m_imageRect;
}

void QKxScreenCapture::stop()
{
    push(MT_EXIT);
}

int QKxScreenCapture::draw(QDataStream &stream)
{
    int drawCount = 0;
    if(m_rectCount > 0) {
        QByteArray &buf = m_request.buf;
        stream << quint8(0) << quint8(m_request.bitUsed) << quint16(m_rectCount);
        stream.writeRawData(buf.data(), buf.length());
        drawCount =  buf.length() + 4;
    }else{
        stream << quint8(0) << quint8(m_request.bitUsed) << quint16(0);
    }
    tryToNextFrame(true, m_fullRequest);
    return drawCount;
}

void QKxScreenCapture::onNextFrame(const QRect& rt, bool full)
{
    tryToNextFrame(false, full);
}

void QKxScreenCapture::push(uchar type, const QByteArray &data)
{
    QMutexLocker locker(&m_mtx);
    MyMsg tmp;
    tmp.type = type;
    tmp.data = data;
    m_queue.append(tmp);
    //qDebug() <<  "push" << m_queue.length() << type << data;
    ::send(m_msgWrite, (char*)&type, 1, 0);
}

bool QKxScreenCapture::pop(uchar &type, QByteArray &data)
{
    QMutexLocker locker(&m_mtx);
    if(m_queue.isEmpty()) {
        return false;
    }
    MyMsg tmp = m_queue.takeFirst();
    type = tmp.type;
    data = tmp.data;
    //qDebug() <<  "pop" << m_queue.length() << type << data;
    return true;
}

bool QKxScreenCapture::takeOne(uchar type, QByteArray &data)
{
    QMutexLocker locker(&m_mtx);
    for(int i = 0; i < m_queue.length(); i++) {
        MyMsg &msg = m_queue[i];
        if(msg.type == type) {
            data.swap(msg.data);
            m_queue.removeAt(i);
            return true;
        }
    }
    return false;
}

void QKxScreenCapture::tryToNextFrame(bool drawAfter, bool full)
{
    if(drawAfter) {
        if(m_queueCount > 1 && m_highPerformance) {
            m_queueCount = 1;
            QByteArray buf(1, full ? char(1) : char(0));
            push(MT_NEXTFRAME, buf);
            m_fullRequest = false;
        }else{
            m_queueCount = 0;
        }
    }else{
        if(m_queueCount <= 0) {
            m_queueCount = 1;
            QByteArray buf(1, full ? char(1) : char(0));
            push(MT_NEXTFRAME, buf);
            m_fullRequest = false;
        }else{
            m_queueCount++;
            if(full) {
                m_fullRequest = true;
            }
        }
    }
}

void QKxScreenCapture::run()
{
    int err;
    do{
        err = running();
    }while(err > 0);
    qDebug() << "exit code" << err << m_screenIndex;
}

bool QKxScreenCapture::init()
{
    int fd[2];
    if(!QKxUtils::createPair(fd)) {
        return false;
    }
    m_msgRead = fd[0];
    m_msgWrite = fd[1];
    QKxUtils::setSocketNoDelay(fd[0], true);
    QKxUtils::setSocketNoDelay(fd[1], true);
    QKxUtils::setSocketNonBlock(m_msgRead, true);
    return true;
}
