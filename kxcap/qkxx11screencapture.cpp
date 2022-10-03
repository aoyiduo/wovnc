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

#include "qkxx11screencapture.h"
#include "qkxx11capture.h"
#include "qkxutils.h"
#include "qkxvnccompress.h"
#include "qkxh264encoder.h"

#include <QMutexLocker>
#include <QDebug>
#include <QDataStream>
#include <QFile>
#include <QDateTime>
#include <QBuffer>
#include <QImage>

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

QKxX11ScreenCapture::QKxX11ScreenCapture(int screenIndex, const QRect &rt, const QRect &imgRt, QObject *parent)
    : QKxScreenCapture(screenIndex, rt, imgRt, parent)
{

}

int QKxX11ScreenCapture::running()
{
    QKxX11Capture cap(m_screenIndex, m_imageRect, m_screenRect, m_h264Encoder);
    if(!cap.reset()) {
        return -999;
    }
    qDebug() << "QKxX11ScreenCapture::running";
    m_cap = &cap;
    int lastErr = 0;
    qint8 lastBitUsed = 0;
    while(true) {
        timeval tm={10,0};
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(m_msgRead, &rfds);
        int fdmax = int(m_msgRead);
        int n = select(fdmax+1, &rfds, nullptr, nullptr, &tm);
        if(n == 0) {
            qDebug() << "QKxX11ScreenCapture::running-timeout:" << QDateTime::currentMSecsSinceEpoch();
            continue;
        }
        if(FD_ISSET(m_msgRead, &rfds)) {
            char type;
            if(QKxUtils::xRecv(m_msgRead, (char*)&type, 1) > 0) {
                uchar type;
                QByteArray data;
                while(pop(type, data)) {
                    if(type == MT_EXIT) {
                        return -1;
                    }else if(type == MT_NEXTFRAME){
                        if(lastErr < 0) {
                            cap.reset();
                        }
                        lastErr = cap.getFrame();
                        QMutexLocker lock(m_zipMtx);
                        m_rectCount = 0;
                        if(lastErr == 0) {                            
                            QMutexLocker lock2(&m_mtx);
                            m_request.buf.clear();
                            QBuffer dev(&m_request.buf);
                            dev.open(QIODevice::WriteOnly);
                            m_request.ds.setDevice(&dev);
                            m_request.bitUsed = m_request.fmt.depth + m_request.fmt.trueColor + m_request.fmt.extendFormat + m_request.fmt.extendQuality;
                            bool reset = false;
                            if(lastBitUsed != m_request.bitUsed) {
                                memset(m_pbits, 0, m_imageRect.width() * m_imageRect.height() * 4);
                                reset = true;
                                if(m_request.fmt.extendFormat == 2) {
                                    m_h264Encoder->setQualityLevel(m_request.fmt.extendQuality);
                                }
                            }
                            lastBitUsed = m_request.bitUsed;
                            m_rectCount = cap.drawFrame(&m_request, m_pbits, m_bytesPerLine, reset);
                        }
                        emit frameResult(m_screenIndex, lastErr, m_rectCount); // make sure the result emit under locker protected.
                    }
                }
            }
        }
    }
    return 0;
}
