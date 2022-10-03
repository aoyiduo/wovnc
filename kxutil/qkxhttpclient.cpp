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

#include "qkxhttpclient.h"

#ifdef Q_OS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#define myclosesocket  closesocket
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int SOCKET;
#define myclosesocket(x)    close(x)
#endif

#include <QUrl>
#include <QDebug>
#include <QMetaObject>

#ifdef Q_OS_WIN
bool init() {
    WORD sockVersion = MAKEWORD(2,2);
    WSADATA wsaData;
    int err = WSAStartup(sockVersion, &wsaData);
    return err == 0;
}
static bool __init = init();
#endif

QKxHttpClient::QKxHttpClient(QObject *parent)
{

}

QKxHttpClient::~QKxHttpClient()
{
    if(isRunning()) {
        wait(3000);
        if(isRunning()) {
            terminate();
        }
    }
}

bool QKxHttpClient::get(const QString &url)
{
    if(isRunning()) {
        return false;
    }
    m_url = url;
    m_type = HC_GET;
    start();
    return true;
}

bool QKxHttpClient::post(const QString &url, const QByteArray &data)
{
    if(isRunning()) {
        return false;
    }
    m_type = HC_POST;
    m_url = url;
    m_data = data;
    start();
    return true;
}

void QKxHttpClient::onResult(int code, const QByteArray& body)
{
    wait(3000);
    emit result(code, body);
}

static int getAddrInfos(const char *host, int port, addrinfo **ai)
{
    const char *service = nullptr;
    struct addrinfo hints;
    char s_port[10];

    memset(&hints, 0, sizeof(hints));

    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (port == 0) {
        hints.ai_flags = AI_PASSIVE;
    } else {
        qsnprintf(s_port, sizeof(s_port), "%hu", (unsigned short)port);
        service = s_port;
#ifdef AI_NUMERICSERV
        hints.ai_flags = AI_NUMERICSERV;
#endif
    }

    return getaddrinfo(host, service, &hints, ai);
}

bool QKxHttpClient::fetch(QByteArray& hdr, QByteArray& buffer)
{
    QUrl url(m_url);
    const QByteArray &host = url.host().toUtf8();
    const QByteArray &path = url.path().toUtf8();
    const QByteArray &arg = url.query().toUtf8();
    int port = url.port(80);
    QByteArray body;
    if(m_type == HC_GET) {
        body.append("GET ");
        body.append(path+"?"+arg);
        body.append(" HTTP/1.1");
        body.append("\nAccept: */*");
        body.append("\nAccept-Encoding: identity");
        body.append("\nConnection: close");
        body.append("\nHost: ");
        body.append(host);
        body.append("\r\n\r\n");
    }else if(m_type == HC_POST){
        body.append("POST ");
        body.append(path+"?"+arg);
        body.append(" HTTP/1.1");
        body.append("\nAccept: */*");
        body.append("\nAccept-Encoding: identity");
        body.append("\nConnection: close");
        body.append("\nHost: ");
        body.append(host);
        body.append("\nContentLength: ");
        body.append(QString("%1").arg(m_data.length()).toUtf8());
        body.append("\r\n\r\n");
        body.append(m_data);
    }
    struct addrinfo *ai = nullptr;
    struct addrinfo *itr = nullptr;
    int err = getAddrInfos(host.data(), port, &ai);
    if(err != 0) {
        return false;
    }
    for (itr = ai; itr != nullptr; itr = itr->ai_next) {
        /* create socket */
        SOCKET fd = socket(itr->ai_family, itr->ai_socktype, itr->ai_protocol);
        err = ::connect(fd, itr->ai_addr, itr->ai_addrlen);
        if(err != 0){
            myclosesocket(fd);
            continue;
        }
        int n = ::send(fd, body, body.length(), 0);
        if(n != body.length()) {
            myclosesocket(fd);
            freeaddrinfo(ai);
            return false;
        }
        while(1) {
            QByteArray buf(10240, Qt::Uninitialized);
            int n = ::recv(fd, buf.data(), buf.capacity(), 0);
            if(n < 0) {
                myclosesocket(fd);
                freeaddrinfo(ai);
                return false;
            }
            buf.resize(n);
            buffer.append(buf);
            for(int i = 0; i < buffer.length(); i++) {
                if(buffer.at(i) == '\n') {
                    if(buffer.at(i+1) == '\n') {
                        hdr.append(buffer.mid(0, i+2));
                        buffer.remove(0, i+2);
                        break;
                    }else if(buffer.at(i+1) == '\r' && buffer.at(i+2) == '\n') {
                        hdr.append(buffer.mid(0, i+3));
                        buffer.remove(0, i+3);
                        break;
                    }
                }
            }
            if(hdr.isEmpty()) {
                myclosesocket(fd);
                freeaddrinfo(ai);
                return false;
            }
            QByteArrayList hdrs = hdr.split('\n');
            bool ok;
            int contentLength = 0;
            bool isChunk = false;
            for(int i = 0; i < hdrs.length(); i++) {
                QByteArray field = hdrs.at(i).toLower();
                if(field.startsWith("content-length:")) {
                    field.remove(0, 15);
                    contentLength = field.toInt(&ok);
                }else if(field.startsWith("transfer-encoding:")) {
                    field.remove(0, 18);
                    field = field.simplified();
                    if(field == "chunked") {
                        isChunk = true;
                    }
                }
            }
            if(isChunk) {
                for(int i = 0; i < buffer.length(); i++) {
                    if(buffer.at(i) == '\n') {
                        QByteArray number = buffer.mid(0, i+1).simplified();
                        buffer.remove(0, i+1);
                        contentLength = number.toInt(&ok, 16);
                        break;
                    }
                }
            }
            if(contentLength < 0) {
                myclosesocket(fd);
                freeaddrinfo(ai);
                return false;
            }
            if(buffer.length() > contentLength) {
                buffer.resize(contentLength);
                myclosesocket(fd);
                freeaddrinfo(ai);
                return true;
            }
            int left = contentLength - buffer.length();
            buf.resize(left);
            n = ::recv(fd, buf.data(), buf.length(), 0);
            if(n < 0) {
                myclosesocket(fd);
                freeaddrinfo(ai);
                return false;
            }
            buffer.append(buf);
            myclosesocket(fd);
            freeaddrinfo(ai);
            return true;
        }
    }
    freeaddrinfo(ai);
    return false;
}

void QKxHttpClient::run()
{
    QByteArray hdr, body;
    if(!fetch(hdr, body)) {
        QMetaObject::invokeMethod(this, "onResult", Qt::QueuedConnection, Q_ARG(int, -1), Q_ARG(QByteArray, QByteArray()));
        return;
    }
    QByteArrayList fields = hdr.split('\n');
    QByteArrayList codes = fields.takeFirst().split(QChar::Space);
    bool ok;
    int code = codes.at(1).toInt(&ok);
    QMetaObject::invokeMethod(this, "onResult", Qt::QueuedConnection, Q_ARG(int, code), Q_ARG(QByteArray, body));
}
