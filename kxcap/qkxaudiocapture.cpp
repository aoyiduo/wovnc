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

#include "qkxaudiocapture.h"
#include "qkxutils.h"
#include "qkxopusencoder.h"
#include "qkxaudiostream.h"

#include <portaudio.h>

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

#define MT_EXIT             (0x7f)

QKxAudioCapture::QKxAudioCapture(QObject *parent)
    : QThread(parent)
{
    init();
    QObject::connect(this, SIGNAL(internalAssemble(QByteArray)), this, SLOT(onInternalAssemble(QByteArray)));
}

QKxAudioCapture::~QKxAudioCapture()
{
    stop();
    wait(3000);
}

void QKxAudioCapture::stop()
{
    if(isRunning()) {
        push(MT_EXIT);
    }
}

void QKxAudioCapture::onRequest(int cnt)
{
    m_timeLast = QDateTime::currentMSecsSinceEpoch();
}

void QKxAudioCapture::onInternalAssemble(const QByteArray &buf)
{
    //qDebug() << "onInternalAssemble" << buf.length();
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if(now - m_timeLast < 1000) {
        // drop it when active time over 1 second.
        m_timeLast = now;
        emit result(buf);
    }
}

void QKxAudioCapture::push(uchar type, const QByteArray &data)
{
    QMutexLocker locker(&m_mtx);
    MyMsg tmp;
    tmp.type = type;
    tmp.data = data;
    m_queue.append(tmp);
    //qDebug() <<  "push" << m_queue.length() << type << data;
    ::send(m_msgWrite, (char*)&type, 1, 0);
}

bool QKxAudioCapture::pop(uchar &type, QByteArray &data)
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

bool QKxAudioCapture::takeOne(uchar type, QByteArray &data)
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

void QKxAudioCapture::popAll()
{
    QMutexLocker locker(&m_mtx);
    m_queue.clear();
}

void QKxAudioCapture::run()
{
    int err;
    QKxAudioStream::initialize();
    do{
        err = running();
    }while(err > 0);
    QKxAudioStream::terminate();
    popAll();
    qDebug() << "exit code" << err;
}

bool QKxAudioCapture::init()
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

int QKxAudioCapture::running()
{
    int idx = findDefaultLoopbackDevice();
    if(idx < 0) {
        return -1;
    }
    int sampleRate = 48000; // opus fix.
    int channelCount = 2;
    int framesPerBuffer = 960;

    QKxOpusEncoder enc;
    if(!enc.init(sampleRate, channelCount)) {
        return -2;
    }

    QByteArray buf;
    buf.resize(1024 * 1024);
    QKxAudioStream acap;
    std::function<int(char*,int)> cb = [&](char* pcm, int bytesTotal)->int {
        if(pcm == nullptr) {
            QByteArray buf;
            QDataStream ds(&buf, QIODevice::WriteOnly);
            if(enc.result(ds) > 0) {
                emit internalAssemble(buf);
            }
            return paContinue;
        }
        enc.process(pcm, bytesTotal);
        return paContinue;
    };
    if(!acap.init(idx, sampleRate, channelCount, framesPerBuffer, cb)) {
        return -3;
    }

    if(!acap.start()) {
        return -4;
    }
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
                        return -9999;
                    }else{

                    }
                }
            }
        }
    }
}
