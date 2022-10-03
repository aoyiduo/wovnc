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

#include "qkxdxgiscreencapture.h"

#include "qkxdxgicapture.h"
#include "qkxutils.h"

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

QKxDXGIScreenCapture::QKxDXGIScreenCapture(int screenIndex, const QRect &rt, const QRect &imgRt, QObject *parent)
    : QKxScreenCapture(screenIndex, rt, imgRt, parent)
{

}

int QKxDXGIScreenCapture::running()
{
    if(QKxUtils::isDesktopChanged()) {
        QKxUtils::resetThreadDesktop();
    }
    QKxDXGICapture cap(m_screenIndex, m_imageRect, m_screenRect);
    if(!cap.init()) {
        return -999;
    }
    qDebug() << "QKxDXGIScreenCapture::running";
    m_cap = &cap;
    int lastErr = -1;
    qint8 lastBitUsed = 0;
    while(true) {
        timeval tm={10,0};
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(m_msgRead, &rfds);
        int fdmax = int(m_msgRead);
        int n = select(fdmax+1, &rfds, nullptr, nullptr, &tm);
        if(n == 0) {
            qDebug() << "QKxDXGIScreenCapture::running-timeout:" << QDateTime::currentMSecsSinceEpoch();
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
                        if(lastErr == 0) {
                            cap.releaseFrame();
                        }
                        lastErr = cap.getFrame();
                        m_rectCount = 0;
                        if(lastErr == 0) {
                            QMutexLocker lock(m_zipMtx);
                            QMutexLocker lock2(&m_mtx);
                            m_request.buf.clear();
                            QBuffer dev(&m_request.buf);
                            dev.open(QIODevice::WriteOnly);
                            m_request.ds.setDevice(&dev);
                            m_request.bitUsed = m_request.fmt.depth + m_request.fmt.trueColor + m_request.fmt.extendFormat + m_request.fmt.extendQuality;
                            bool reset = false;
                            if(lastBitUsed != m_request.bitUsed) {
                                reset = true;
                                memset(m_pbits, 0, m_imageRect.width() * m_imageRect.height() * 4);
                            }
                            m_rectCount = cap.drawFrame(&m_request, m_pbits, m_bytesPerLine, reset);
                            emit frameResult(m_screenIndex, lastErr, m_rectCount); // make sure the result emit under locker protected.
                        }else{
                            emit frameResult(m_screenIndex, lastErr, 0);
                            if(lastErr == DXGI_ERROR_WAIT_TIMEOUT) {
                                //qDebug() << "QKxDXGIScreenCapture::running:DXGI_ERROR_WAIT_TIMEOUT";
                                // when cursor
                            }else if(lastErr == DXGI_ERROR_ACCESS_LOST) {
                                qDebug() << "QKxDXGIScreenCapture::running:DXGI_ERROR_ACCESS_LOST";
                                return 1;
                            }else if(lastErr == DXGI_ERROR_INVALID_CALL) {
                                qDebug() << "QKxDXGIScreenCapture::running:DXGI_ERROR_INVALID_CALL";
                                return 2;
                            }else if(lastErr != 0) {
                                qDebug() << "QKxDXGIScreenCapture::running:lastErr" << lastErr;
                                return 3;
                            }
                        }
                    }
                }
            }
        }
    }
}
