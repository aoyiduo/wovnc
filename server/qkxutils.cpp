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

#include "qkxutils.h"

#include <QHostInfo>
#include <QSysInfo>
#include <QCoreApplication>
#include <QDir>
#include <QCryptographicHash>
#include <QLibraryInfo>
#include <QThread>
#include <QIODevice>
#include <QRect>
#include <QRegion>
#include <QDebug>

#include <cstdio>
#include <cstdint>

#ifdef Q_OS_WIN
#include <intrin.h>
#endif

#include <string>
#include <memory>


#ifdef Q_OS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fcntl.h>
#define myclosesocket  closesocket
typedef int socklen_t;

#include <windows.h>
#include <wtsapi32.h>
#include <tlhelp32.h>
#include <shlobj.h> // NOLINT(build/include_order)
#include <userenv.h>

#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "ws2_32.lib")

static bool init() {
    WORD sockVersion = MAKEWORD(2,2);
    WSADATA wsaData;
    int err = WSAStartup(sockVersion, &wsaData);
    return err == 0;
}
static bool __init = init();
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
#define myclosesocket(x)    close(x)
#endif

quint16 QKxUtils::randomPort()
{
    qsrand(QDateTime::currentSecsSinceEpoch());
    qint16 port = (qrand() % 61000) + 3000;
    return port;
}

quint16 QKxUtils::firstPort(const QByteArray &id)
{
    quint16 port = qChecksum(id.data(), id.length());
    return qBound<quint16>(2000, port, 65000);
}

QByteArray QKxUtils::randomPassword(int cnt)
{
    QByteArray tmp="0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QByteArray pwd;
    qsrand(0x7FFFFFFF & QDateTime::currentMSecsSinceEpoch());
    while(cnt-- > 0) {
        int i = qrand() % tmp.length();
        pwd.append(tmp.at(i));
        tmp.remove(i, 1);
    }
    return pwd;
}

QByteArray QKxUtils::randomData(int cnt)
{
    QByteArray data;
    data.reserve(cnt);
    qsrand(0x7FFFFFFF & QDateTime::currentMSecsSinceEpoch());
    while(cnt-- > 0) {
        int i = qAbs(qrand()) % 0xFF;
        data.append(i+1);
    }
    return data;
}

QByteArray QKxUtils::deviceName()
{
    QString name = QHostInfo::localHostName();
    name = QSysInfo::machineHostName();
    return name.toUtf8();
}

bool QKxUtils::checkEmailOrPhone(const QString &txt)
{
    QRegExp email("[0-9a-zA-Z\\-_\\.]*@[0-9a-zA-Z]+\\.[a-zA-Z]*");
    if(email.exactMatch(txt)) {
        qDebug() << "IsEmail" << true << txt;
        return true;
    }
    /*
     * http://www.kxtry.com/archives/3424
     */
    QRegExp phone("^1([358][0-9]|4[579]|66|7[0135678]|9[89])[0-9]{8}$");
    if(phone.exactMatch(txt)) {
        qDebug() << "IsMoiblePhone" << true << txt;
        return true;
    }
    qDebug() << "Bad Phone Or Email" << false << txt;
    return false;
}

bool QKxUtils::checkPassword(const QString &txt)
{
    for(int i = 0; i < txt.length(); i++) {
        QChar ch = txt.at(i);
        if(ch > 0x7F || ch == QChar::Space) {
            return false;
        }
    }
    return true;
}

int QKxUtils::osType()
{
#if(defined(Q_OS_WIN))
    return 0;
#elif(defined(Q_OS_MAC) || defined(Q_OS_MACOS))
    return 2;
#endif
    return 1;
}

bool QKxUtils::isDebugBuild()
{
#ifdef QT_DEBUG
    return true;
#else
    return false;
#endif
}

bool QKxUtils::createPair(int fd[])
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(bind(server, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
        return 0;
    }
    if(listen(server, 5) == -1) {
        myclosesocket(server);
        return 0;
    }
    int asize = sizeof(addr);
    ::getsockname(server, (struct sockaddr*)&addr, (socklen_t*)&asize);
    quint16 port = htons(addr.sin_port);
    qInfo() << "QKxUtils::createPair listen:" << port;
    int fd1 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(::connect(fd1, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
        myclosesocket(fd1);
        myclosesocket(server);
        return false;
    }
    int fd2 = accept(server, nullptr, nullptr);
    fd[0] = int(fd1);
    fd[1] = int(fd2);
    myclosesocket(server);
    return true;
}

bool QKxUtils::isAgain(int err)
{
#ifdef Q_OS_WIN
    return err == EAGAIN || err == EWOULDBLOCK || err == WSAEWOULDBLOCK;
#else
    return err == EAGAIN;
#endif
}

void QKxUtils::setSocketNonBlock(int sock, bool on)
{
#ifdef Q_OS_WIN
    ulong nonblocking = on ? 1 : 0;
    ioctlsocket(sock, FIONBIO, &nonblocking);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if(on) {
        fcntl(sock, F_SETFL, flags|O_NONBLOCK);
    }else{
        fcntl(sock, F_SETFL, flags&~O_NONBLOCK);
    }
#endif
}

void QKxUtils::setSocketNoDelay(int sock, bool on)
{
    int opt = on ? 1 : 0;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));
}

int QKxUtils::socketError()
{
#ifdef Q_OS_WIN
    int err = WSAGetLastError();
    return err;
#else
    return errno;
#endif
}

int QKxUtils::xRecv(int sock, char *buf, int len, int flag)
{
    int left = len;
    while(left > 0) {
        int n = ::recv(sock, buf, left, flag);
        if(n < 0) {
            if(QKxUtils::isAgain(socketError())) {
                return len - left;
            }
            return n;
        }
        if(n == 0) {
            return len - left;
        }
        buf += n;
        left -= n;
    }
    return len;
}

int QKxUtils::xSend(int sock, char *buf, int len, int flag)
{
    int left = len;
    while(left > 0) {
        int n = ::send(sock, buf, left, flag);
        if(n < 0) {
            if(QKxUtils::isAgain(socketError())) {
                return len - left;
            }
            return n;
        }
        if(n == 0) {
            return len - left;
        }
        buf += n;
        left -= n;
    }
    return len;
}

bool QKxUtils::waitForRead(QIODevice *dev, qint64 cnt)
{
    if(dev == nullptr) {
        return false;
    }
    while(dev->bytesAvailable() < cnt) {
        if(!dev->waitForReadyRead(3000)) {
            return false;
        }
    }
    return true;
}

QByteArray QKxUtils::md5(const QByteArray &tmp)
{
    QByteArray md5 = QCryptographicHash::hash(tmp, QCryptographicHash::Md5);
    return md5.toBase64(QByteArray::OmitTrailingEquals);
}
