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

#include "qkxsendinput.h"
#include "qkxutils.h"

#include <QMutexLocker>
#include <QDebug>
#include <QDataStream>
#include <QFile>
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

#define MT_MOUSE            (0x1)
#define MT_KEY              (0x2)
#define MT_EXIT             (0x7f)

QKxSendInput::QKxSendInput(const QRect& capRt, const QRect& deskRt, QObject *parent)
    : QThread(parent)
    , m_tryDirect(true)
{
    m_capRect = capRt.translated(-deskRt.topLeft());
    m_deskRect = QRect(0, 0, deskRt.width(), deskRt.height());
    init();
}

void QKxSendInput::stop()
{
    push(MT_EXIT);
}

void QKxSendInput::sendMouseEvent(quint8 button, quint16 x, quint16 y)
{
    if(m_tryDirect) {
        handleMouseEvent(button, x, y);
    }else{
        QByteArray buf;
        QDataStream ds(&buf, QIODevice::WriteOnly);
        ds << button << x << y;
        push(MT_MOUSE, buf);
    }
}

void QKxSendInput::sendKeyEvent(quint8 down, quint32 key)
{
    if(m_tryDirect) {
        handleKeyEvent(down, key);
    }else{
        QByteArray buf;
        QDataStream ds(&buf, QIODevice::WriteOnly);
        ds << down << key;
        push(MT_KEY, buf);
    }
}

void QKxSendInput::push(uchar type, const QByteArray &data)
{
    QMutexLocker locker(&m_mtx);
    MyMsg tmp;
    tmp.type = type;
    tmp.data = data;
    m_queue.append(tmp);
    //qDebug() <<  "push" << m_queue.length() << type << data;
    ::send(m_msgWrite, (char*)&type, 1, 0);
}

bool QKxSendInput::pop(uchar &type, QByteArray &data)
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

bool QKxSendInput::takeOne(uchar type, QByteArray &data)
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

void QKxSendInput::run()
{
    int err = running();
    qDebug() << "exit code" << err;
}

int QKxSendInput::running()
{
    QKxUtils::selectInputDesktop();
    QKxUtils::setSocketNonBlock(m_msgRead, true);
    while(true) {
        timeval tm={3,0};
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(m_msgRead, &rfds);
        int fdmax = int(m_msgRead);
        int n = select(fdmax+1, &rfds, nullptr, nullptr, &tm);
        if(n == 0) {
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
                    }else{
                        if(QKxUtils::isDesktopChanged()) {
                            QKxUtils::resetThreadDesktop();
                        }
                        if(type == MT_MOUSE){
                            QDataStream ds(data);
                            quint8 button;
                            quint16 x, y;
                            ds >> button >> x >> y;
                            handleMouseEvent(button, x, y);
                        }else if(type == MT_KEY){
                            QDataStream ds(data);
                            quint8 down;
                            quint32 key;
                             ds >> down >> key;
                            handleKeyEvent(down, key);
                        }
                    }
                }
            }
        }
    }
}

bool QKxSendInput::init()
{
    int fd[2];
    if(!QKxUtils::createPair(fd)) {
        return false;
    }
    m_msgRead = fd[0];
    m_msgWrite = fd[1];
    QKxUtils::setSocketNoDelay(fd[0], true);
    QKxUtils::setSocketNoDelay(fd[1], true);
    return true;
}

