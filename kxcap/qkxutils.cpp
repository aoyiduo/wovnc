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

#include <libyuv.h>

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

bool init() {
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
        return false;
    }
    if(listen(server, 5) == -1) {
        myclosesocket(server);
        return false;
    }
    int asize = sizeof(addr);
    ::getsockname(server, (struct sockaddr*)&addr, (socklen_t*)&asize);
    quint16 port = htons(addr.sin_port);
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

bool QKxUtils::findDirtyRect(uchar *data1, int step1, int _width, int _height, uchar *data2, int step2, QRect *prt)
{
    int width = _width & ~0x1;
    int height = _height & ~0x1;
    int nrow = (height + 15) / 16;
    int ncol = (width + 15) / 16;
    int left = ncol + 1;
    int right = 0;
    int top = nrow + 1;
    int bottom = 0;

    // find top left.
    for(int r = 0; r < nrow && left > 0; r++) {
        int bheight = ((r+1) == nrow) ? (height % 16) : 16;
        if(bheight == 0) {
            bheight = 16;
        }
        for(int c = 0; c < ncol && c < left; c++) {
            int bwidth = ((c+1) == ncol) ? (width % 16 ) : 16;
            if(bwidth == 0) {
                bwidth = 16;
            }
            quint8 *p1 = reinterpret_cast<quint8*>(data1 + r * 16 * step1 + c * 16 * 4);
            quint8 *p2 = reinterpret_cast<quint8*>(data2 + r * 16 * step2 + c * 16 * 4);
            for(int bh = 0; bh < bheight; bh++) {
                if(memcmp(p1, p2, bwidth * 4) != 0) {
                    if(r < top) {
                        top = r;
                    }
                    if(c < left) {
                        left = c;
                    }
                    if(c > right) {
                        right = c;
                    }
                    if(r > bottom) {
                        bottom = r;
                    }
                    break;
                }
                p1 += step1;
                p2 += step2;
            }
        }
    }

    // find bottom right.
    for(int r = nrow - 1; r > top && (right+1) != ncol; r--) {
        int bheight = ((r+1) == nrow) ? (height % 16) : 16;
        if(bheight == 0) {
            bheight = 16;
        }
        for(int c = ncol - 1; c > right; c--) {
            int bwidth = ((c+1) == ncol) ? (width % 16 ) : 16;
            if(bwidth == 0) {
                bwidth = 16;
            }
            quint8 *p1 = reinterpret_cast<quint8*>(data1 + r * 16 * step1 + c * 16 * 4);
            quint8 *p2 = reinterpret_cast<quint8*>(data2 + r * 16 * step2 + c * 16 * 4);
            for(int bh = 0; bh < bheight; bh++) {
                if(memcmp(p1, p2, bwidth * 4) != 0) {
                    if(r < top) {
                        top = r;
                    }
                    if(c < left) {
                        left = c;
                    }
                    if(c > right) {
                        right = c;
                    }
                    if(r > bottom) {
                        bottom = r;
                    }
                    break;
                }
                p1 += step1;
                p2 += step2;
            }
        }
    }

    QRect imgRt(0, 0, width, height);
    QRect rt(left * 16, top * 16, (right + 1 - left) * 16, (bottom + 1 - top) * 16);
    QRect dirty = imgRt.intersected(rt);
    //qDebug() << "bound rect" << left << top << right << bottom << dirty;
    if(prt != nullptr) {
        *prt = dirty;
    }
    return dirty.isValid();
}


#define SIZE (256)
static unsigned short Y_R[SIZE],Y_G[SIZE],Y_B[SIZE],U_R[SIZE],U_G[SIZE],U_B[SIZE],V_R[SIZE],V_G[SIZE],V_B[SIZE];
bool yuv_table_init() {
    int i;

    for(i = 0; i < SIZE; i++) {
        Y_R[i] = (i * 1224) >> 12; //Y table
        Y_G[i] = (i * 2404) >> 12;
        Y_B[i] = (i * 467) >> 12;
        U_R[i] = (i * 602) >> 12; //U table
        U_G[i] = (i * 1183) >> 12;
        U_B[i] = (i * 1785) >> 12;
        V_R[i] = (i * 2519) >> 12; //V table
        V_G[i] = (i * 2109) >> 12;
        V_B[i] = (i * 409) >> 12;
    }
    return true;
}

static bool g_yuvInit = yuv_table_init();
void QKxUtils::rgbToYuv(qint32 rgb, quint8 *y, quint8 *u, quint8 *v)
{
    // Y = 0.299R + 0.587G + 0.114B
    // U = -0.147R - 0.289G + 0.436B
    // V = 0.615R - 0.515G - 0.100B
    quint8 b = rgb & 0xFF;
    quint8 g = (rgb >> 8) & 0xFF;
    quint8 r = (rgb >> 16) & 0xFF;
    *y = Y_R[r] + Y_G[g] + Y_B[b];
    *u = U_R[r] + U_G[g] + U_B[b];
    *v = V_R[r] + V_G[g] + V_B[b];
}

//YUV = YCb[U]Cr[V]
//Ey = 0.299R+0.587G+0.114B
//Ecr = 0.713(R – Ey) = 0.500R-0.419G-0.081B
//
void QKxUtils::rgbToYuv2(qint32 rgb, quint8 *y, quint8 *u, quint8 *v)
{
    quint8 b = rgb & 0xFF;
    quint8 g = (rgb >> 8) & 0xFF;
    quint8 r = (rgb >> 16) & 0xFF;
    float Y = 0.299 * r + 0.587 * g + 0.114 * b;
    float U = -0.147 * r - 0.289 * g + 0.436 * b;
    float V = 0.615 * r - 0.515 * g - 0.100 * b;
    *y = quint8(Y);
    *u = quint8(U);
    *v = quint8(V);
}

void QKxUtils::copyRgb32(uchar *dst, qint32 dstStep, uchar *src, qint32 srcStep, int width, int height)
{
    libyuv::ARGBCopy(src, srcStep, dst, dstStep, width, height);
}


void QKxUtils::rgb32ToRgb565(uchar *dst, qint32 dstStep, uchar *src, qint32 srcStep, int width, int height)
{
#if 0
    static const quint8 kDither565_4x4[16] = {
        0, 4, 1, 5, 6, 2, 7, 3, 1, 5, 0, 4, 7, 3, 6, 2,
    };
    libyuv::ARGBToRGB565Dither(src, srcStep, dst, dstStep, kDither565_4x4, width, height);
#else
    libyuv::ARGBToRGB565(src, srcStep, dst, dstStep, width, height);
#endif
}

void QKxUtils::rgb32ToRgb555(uchar *dst, qint32 dstStep, uchar *src, qint32 srcStep, int width, int height)
{
    libyuv::ARGBToARGB1555(src, srcStep, dst, dstStep, width, height);
}

void QKxUtils::rgb32ToRgb332(uchar *dst, qint32 dstStep, uchar *src, qint32 srcStep, int width, int height)
{
    for(int h = 0; h < height; h++) {
        quint32 *psrc = reinterpret_cast<quint32*>(src + h * srcStep);
        quint8 *pdst = dst + h * dstStep;
        for(int w = 0; w < width; w++) {
            quint32 clr = psrc[w];
            quint32 r = (clr >> 16) & 0xFF;
            quint32 g = (clr >> 8) & 0xFF;
            quint32 b = clr & 0xFF;
            quint32 tmpr = (double)r / (double)0xFF * 0x7;
            quint32 tmpg = (double)g / (double)0xFF * 0x7;
            quint32 tmpb = (double)b / (double)0xFF * 0x3;
            quint32 rgb = (tmpr << 5) | (tmpg << 2) | (tmpb);
            pdst[w] = quint8(rgb);
        }
    }
}

void QKxUtils::rgb32ToRgbMap(uchar *dst, qint32 dstStep, uchar *src, qint32 srcStep, int width, int height)
{
    for(int h = 0; h < height; h++) {
        quint32 *psrc = reinterpret_cast<quint32*>(src + h * srcStep);
        quint8 *pdst = dst + h * dstStep;
        for(int w = 0; w < width; w++) {
            quint32 rgb = psrc[w];
            quint32 ri = ((rgb >> 16) & 0xFF) / 0x33;
            quint32 gi = ((rgb >> 8) & 0xFF) / 0x33;
            quint32 bi = (rgb & 0xFF) / 0x33;
            quint32 idx = ri * 36 + gi * 6 + bi;
            pdst[w] = quint8(idx);
        }
    }
}

void QKxUtils::rgb32ToNV12(uchar *dst, uchar *src, qint32 srcStep, int width, int height)
{
    int ystride = (width + 3) / 4 * 4;
    int uvstride = ystride / 2;
    uchar *dstY = dst;
    uchar *dstUV = dst + ystride * height;
    libyuv::ARGBToNV12(src, srcStep, dstY, ystride, dstUV, uvstride, width, height);
}

void QKxUtils::rgb32ToI420(uchar *dst, uchar *src, qint32 srcStep, int width, int height)
{
    int ystride = (width + 3) / 4 * 4;
    int ustride = ystride / 2;
    int vstride = ustride;
    uchar *dstY = dst;
    uchar *dstU = dst + ystride * height;
    uchar *dstV = dstU + ustride * height / 2;
    libyuv::ARGBToI420(src, srcStep, dstY, ystride, dstU, ustride, dstV, vstride, width, height);
}

void QKxUtils::nv12ToRgb32(uchar *dst, qint32 dstStep, uchar *src, int width, int height)
{
    int ystride = (width + 3) / 4 * 4;
    int uvstride = ystride / 2;
    uchar *srcY = src;
    uchar *srcUV = src + ystride * height;
    libyuv::NV12ToARGB(srcY, ystride, srcUV, uvstride, dst, dstStep, width, height);
}
