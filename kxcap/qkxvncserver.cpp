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

#include "qkxvncserver.h"
#include "qkxutils.h"
#include "d3des.h"
#include "qkxsendinput.h"
#include "qkxzip.h"
#include "qkxscreencapture.h"
#include "qkxsharememory.h"
#include "qkxscreenlistener.h"
#include "qkxwallpaper.h"
#include "qkxcapoption.h"
#include "qkxvnccompress.h"
#include "qkxftpworker.h"
#include "qkxaudiocapture.h"
#include "qkxwinaudiocapture.h"
#include "qkxwin8privacyscreen.h"

#if(defined(Q_OS_WIN))
#include "qkxwinsendinput.h"
#include "qkxdxgicapture.h"
#include "qkxdxgiscreencapture.h"
#include "qkxgdiscreencapture.h"
#include "qkxwinclipboard.h"
#elif(defined(Q_OS_LINUX))
#include "qkxx11sendinput.h"
#include "qkxx11screencapture.h"
#include "qkxx11clipboard.h"
#else
#include "qkxmacscreencapture.h"
#include "qkxmacsendinput.h"
#include "qkxmacclipboard.h"
#endif

#include <QDataStream>
#include <QScreen>
#include <QGuiApplication>
#include <QtEndian>
#include <QDateTime>
#include <QDebug>
#include <QBuffer>
#include <QClipboard>
#include <QDesktopWidget>
#include <QTimer>
#include <QSysInfo>

#include "keysymdef.h"

#ifdef Q_OS_WIN
#include <Windows.h>
#else
#endif

// Currently bits-per-pixel
// must be 8, 16, or 32
PixelFormat QKxVNCServer::m_rgb32_888 = {32, 24, Q_BYTE_ORDER == Q_BIG_ENDIAN ? 1 : 0, 1, 0xFF, 0xFF, 0xFF, 16, 8, 0, 0, 0, 0};
PixelFormat QKxVNCServer::m_jpeg_444 = {24, 24, Q_BYTE_ORDER == Q_BIG_ENDIAN ? 1 : 0, 1, 0xFF, 0xFF, 0xFF, 16, 8, 0, 3, 0, 0};
PixelFormat QKxVNCServer::m_rgb16_565 = {16, 16, Q_BYTE_ORDER == Q_BIG_ENDIAN ? 1 : 0, 1, 0x1F, 0x3F, 0x1F, 11, 5, 0, 0, 0, 0};
PixelFormat QKxVNCServer::m_rgb15_555 = {16, 15, Q_BYTE_ORDER == Q_BIG_ENDIAN ? 1 : 0, 1, 0x1F, 0x1F, 0x1F, 10, 5, 0, 0, 0, 0};
PixelFormat QKxVNCServer::m_rgb8_332 = {8, 8, Q_BYTE_ORDER == Q_BIG_ENDIAN ? 1 : 0, 1, 0x07, 0x07, 0x03, 5, 2, 0, 0, 0, 0};
PixelFormat QKxVNCServer::m_rgb8_map = {8, 8, Q_BYTE_ORDER == Q_BIG_ENDIAN ? 1 : 0, 0, 0x0, 0x0, 0x0, 0, 0, 0, 0, 0, 0};
PixelFormat QKxVNCServer::m_yuv_nv12 = {16, 16, Q_BYTE_ORDER == Q_BIG_ENDIAN ? 1 : 0, 1, 0x0, 0x0, 0x0, 0, 0, 0, 1, 0, 0};
PixelFormat QKxVNCServer::m_h264_low = {16, 16, Q_BYTE_ORDER == Q_BIG_ENDIAN ? 1 : 0, 1, 0x0, 0x0, 0x0, 0, 0, 0, 2, 0, 0};
PixelFormat QKxVNCServer::m_h264_normal = {16, 16, Q_BYTE_ORDER == Q_BIG_ENDIAN ? 1 : 0, 1, 0x0, 0x0, 0x0, 0, 0, 0, 2, 1, 0};
PixelFormat QKxVNCServer::m_h264_high = {16, 16, Q_BYTE_ORDER == Q_BIG_ENDIAN ? 1 : 0, 1, 0x0, 0x0, 0x0, 0, 0, 0, 2, 2, 0};

int QKxVNCServer::m_qpsMax = 30;

static QVector<quint32> initColor256() {
    QVector<quint32> clrs;
    // init color table
    int idx = 0;
    for(quint32 r = 0; r <= 0xFF; r += 0x33) {
        for(quint32 g = 0; g <= 0xFF; g += 0x33) {
            for(quint32 b = 0; b <= 0xFF; b += 0x33) {
                quint32 rgb = r << 16 | g << 8 | b;
                clrs.append(rgb);
                //qDebug() << idx << colorMapIndex(rgb) << rgb;
                //idx++;
            }
        }
    }
    return clrs;
}

QVector<quint32> QKxVNCServer::m_clrTable = initColor256();


#ifdef Q_OS_WIN
static void test()
{

}
#else
static void test() {

}
#endif

QKxVNCServer::QKxVNCServer(QObject *parent)
    : QObject(parent)
{
    test();
    m_clipboard = createClipboard(this);
    QObject::connect(m_clipboard, SIGNAL(dataChanged()), this, SLOT(onClipboardDataChanged()), Qt::QueuedConnection);

    QMap<QString, QVariant> opts = QKxCapOption::instance()->all();
    m_step = ERS_ReadyInit;

    qInfo() << "vnc cap options" << opts;

    m_zip = new QKxZip(this);
    PFunMatchBest match = reinterpret_cast<PFunMatchBest>(&QKxVNCCompress::matchBest);

    int idx = QKxCapOption::instance()->screenIndex();
    QList<QKxScreenListener::DisplayInfo> screens = QKxScreenListener::screens();
    QRect vrt = screens.at(0).rect;
    QRect rt = vrt;
    for(int i = 1; i < screens.length(); i++) {
        vrt |= screens.at(i).rect;
    }
    int x = vrt.x();
    int y = vrt.y();
    int cx = vrt.width();
    int cy = vrt.height();
    if(screens.length() == 1) {
        QVector<QRect> deskRts;
        for(int i = 0; i < screens.length(); i++) {
            const QKxScreenListener::DisplayInfo &di = screens.at(i);
            deskRts.append(di.rect);
        }
        m_privacyScreen = createPrivacyScreen(deskRts, this);
    }
    if(idx < 0) {
        m_deskRect.setRect(x, y, cx, cy);
        for(int i = 0; i < screens.length(); i++) {
            QKxScreenListener::DisplayInfo scn = screens.at(i);
            QRect scnRt = scn.rect;
            QRect imgRt = scnRt.translated(-x, -y);            
            QKxScreenCapture *cap = createCapture(i, scnRt, imgRt, this);
            cap->setInit(match, m_zip, &m_zipMtx, m_privacyScreen);
            QObject::connect(this, SIGNAL(nextFrame(QRect,bool)), cap, SLOT(onNextFrame(QRect,bool)));
            QObject::connect(cap, SIGNAL(frameResult(int,int,int)), this, SLOT(onFrameResult(int,int,int)));
            m_caps.insert(i, cap);
            cap->start();
        }
    }else{
        if(idx >= screens.length()) {
            idx = 0;
        }        
        QKxScreenListener::DisplayInfo scn = screens.at(idx);
        m_deskRect = scn.rect;
        QRect scnRt = scn.rect;
        QRect imgRt = QRect(0, 0, scnRt.width(), scnRt.height());        
        QKxScreenCapture *cap = createCapture(idx, scnRt, imgRt, this);
        cap->setInit(match, m_zip, &m_zipMtx, m_privacyScreen);
        QObject::connect(this, SIGNAL(nextFrame(QRect,bool)), cap, SLOT(onNextFrame(QRect,bool)));
        QObject::connect(cap, SIGNAL(frameResult(int,int,int)), this, SLOT(onFrameResult(int,int,int)));
        m_caps.insert(idx, cap);
        cap->start();
    }

    m_capTimeLast = QDateTime::currentMSecsSinceEpoch();
    m_tokenTimeLast = m_capTimeLast;

    m_input = createInput(m_deskRect, QRect(x, y, cx, cy), this);
    m_input->start();


    m_tokenCount = 2;

    m_qpsMax = QKxCapOption::instance()->maxFPS();
    m_enableEmptyFrame = QKxCapOption::instance()->enableEmptyFrame();
    m_wall = new QKxWallPaper(this);

    bool black = QKxCapOption::instance()->blackWallpaper();
    m_wall->disable(black);
    bool drag = QKxCapOption::instance()->dragWithContent();
    QKxUtils::setDragWindow(drag);
}

QKxVNCServer::~QKxVNCServer()
{
    m_ftpThread.quit();
    m_ftpThread.wait();


    if(m_wall) {
        delete m_wall;
    }
    if(m_input) {
        m_input->stop();
        m_input->wait();
    }
    for(auto it = m_caps.begin(); it != m_caps.end(); it++){
        QKxScreenCapture *cap = it.value();
        cap->stop();
        cap->wait();
    }
    if(QKxCapOption::instance()->autoLockScreen()) {
        QKxUtils::lockWorkStation();
    }

    if(m_privacyScreen) {
        delete m_privacyScreen;
    }
    QKxUtils::setDragWindow(true);
}

void QKxVNCServer::onReadyRead()
{
    if(m_caps.isEmpty()) {
        close();
        return;
    }
    bool ok = handleRead();
    flush();
    if(!ok) {
        close();
    }
}

void QKxVNCServer::onAboutToClose()
{

}

void QKxVNCServer::onFrameResult(int idx, int err, int cnt)
{
    if(m_step != ERS_WaitClientRequest) {
        return;
    }
    QKxScreenCapture *cap = m_caps.value(idx);
    if(cap == nullptr) {
        return;
    }
    int nBytes = cap->draw(m_stream);
    qint64 now = QDateTime::currentMSecsSinceEpoch() / 10000;
    m_sts.frameCount++;
    if(m_sts.lastSecond != now) {
        m_sts.lastSecond = now;
        qDebug() << "10 second:" << m_sts.qps
                 << "frameCount" << m_sts.frameCount
                 << "requestCount" << m_sts.requestCount
                 << "totalBytes" << m_sts.totalBytes;
        m_sts.qps = 1;
        m_sts.requestCount = 1;
        m_sts.frameCount = 1;
        m_sts.totalBytes = 0;
    }else{
        m_sts.qps++;
        m_sts.totalBytes += nBytes;
        m_sts.totalBytes += 4;
    }
}

void QKxVNCServer::onErrorArrived(int err)
{
    //QCoreApplication::quit(); plication::q2222222
    qDebug() << "onErrorArrived" << err;

}

void QKxVNCServer::onClipboardDataChanged()
{
    QString cutTxt = m_clipboard->text();
    QByteArray buf = cutTxt.toLatin1();
    m_stream << qint8(3) << quint8(0) << quint16(0) << quint32(buf.length());
    m_stream.writeRawData(buf.data(), buf.length());
    qDebug() << "onClipboardDataChange" << cutTxt;
}

void QKxVNCServer::onTokenArrived()
{
    produceToken();
}

void QKxVNCServer::onFtpResult(const QByteArray &buf)
{
    m_stream << quint8(ECST_Ftp) << buf;
    //qDebug() << "onFtpResult:length" << buf.length();
}

void QKxVNCServer::onAudioResult(const QByteArray &buf)
{
    //qDebug() << "onAudioResult:length" << buf.length();
    m_stream << quint8(ECST_PlayAudio) << qint8(3) << buf;
}

bool QKxVNCServer::restart(QIODevice *socket)
{
    qDebug() << "restart..send protocol" << m_step << m_caps.size() << m_caps;
    if(m_caps.isEmpty()) {
        return false;
    }
    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    QObject::connect(socket, SIGNAL(aboutToClose()), this, SLOT(onAboutToClose()));
    m_stream.setDevice(socket);
    m_stream.resetStatus(); // reset status for sometime status will bad.
    sendSupportProtocol();
    qDebug() << "send protocol";
    m_firstFrame = true;
    return true;
}

void QKxVNCServer::produceToken()
{
    int interval = 1000 / m_qpsMax;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 elapse = now - m_tokenTimeLast;
    if(elapse < interval) {
        return;
    }
    int cnt = elapse / interval;
    m_tokenCount += cnt;
    if(m_tokenCount > m_qpsMax) {
        m_tokenCount = m_qpsMax;
        m_tokenTimeLast = now;
    }else{
        m_tokenTimeLast += cnt * interval;
    }

    if(m_tokenCount > 2 && m_enableEmptyFrame) {
        // like realvnc, if you drop it so many request and he will give up next framne
        // so check the token and release it when no consume happen.
        //qDebug() << "m_tokenCount" << m_tokenCount;
        sendNoChangedFrame();
    }
}

bool QKxVNCServer::handleRead()
{
    //qInfo() << "handleRead" << m_step << QThread::currentThread();
    if(m_step == ERS_WaitVerifyProtocol) {
        if(!handleVerifyProtocol()) {
            return false;
        }
        sendSecurityHandshake();
    }else if(m_step == ERS_WaitSecurityHandshake) {
        if(!handleSecurityHandshake()) {
            return false;
        }
        sendChallenge();
    }else if(m_step == ERS_WaitSecurityChallenge) {
        if(!handleVerifyChallenge()) {
            return false;
        }
    }else if(m_step == ERS_WaitClientInit) {
        if(!handleInit()) {
            return false;
        }
    }else if(m_step == ERS_WaitClientRequest){
        while(m_stream.device()->bytesAvailable() > 0) {
            quint8 type;
            m_stream >> type;
            //qDebug() << "MessageType" << type << QDateTime::currentMSecsSinceEpoch();
            if(type == ECST_SetPixelFormat) {
                if(!handleSetPixelFormat()) {
                    return false;
                }
            }else if(type == ECST_SetEncoding) {
                if(!handleSetEncoding()) {
                    return false;
                }
            }else if(type == ECST_FramebufferUpdateRequest) {
                if(!handleFramebufferUpdateRequest()) {
                    return false;
                }
            }else if(type == ECST_KeyEvent) {
                if(!handleKeyEvent()) {
                    return false;
                }
            }else if(type == ECST_PointerEvent) {
                if(!handlePointerEvent()) {
                    return false;
                }
            }else if(type == ECST_ClientCutText) {
                if(!handleClientCutText()) {
                    return false;
                }
            }else if(type == ECST_ScreenBlack) {
                if(!handleScreenBlack()) {
                    return false;
                }
            }else if(type == ECST_LockScreen) {
                if(!handleLockScreen()) {
                    return false;
                }
            }else if(type == ECST_PlayAudio) {
                if(!handlePlayAudio()) {
                    return false;
                }
            }else if(type == ECST_Ftp) {
                if(!handleFtpPacket()) {
                    return false;
                }
            }else if(type == ECST_PrivacyScreen) {
                if(!handlePrivacyScreen()) {
                    return false;
                }
            }
        }
    }else{
        qDebug() << "bad bad";
    }
    return true;
}

void QKxVNCServer::sendSupportProtocol()
{
    char buf[13] = "RFB 003.008\n";
    QDataStream::Status sts = m_stream.status();
    m_stream.writeRawData(buf, 12);
    m_step = ERS_WaitVerifyProtocol;
}

bool QKxVNCServer::handleVerifyProtocol()
{
    if(!waitForRead(12)) {
        return false;
    }
    char buf[13] = {0};
    m_stream.readRawData(buf, 12);
    if(strcmp(buf, "RFB 003.008\n") != 0) {
        return false;
    }

    return true;
}

void QKxVNCServer::sendSecurityHandshake()
{
    char buf[3];
    buf[0] = 1;
    buf[1] = ST_VNCAuth; // only support password.
    m_stream.writeRawData(buf, 2);
    m_step = ERS_WaitSecurityHandshake;
    flush();
}

bool QKxVNCServer::handleSecurityHandshake()
{
    if(!waitForRead(1)) {
        return false;
    }
    char buf[3];
    m_stream.readRawData(buf, 1);
    return buf[0] == 2;
}

void QKxVNCServer::sendChallenge()
{
    m_challenge = QKxUtils::randomPassword(16);
    int length = m_challenge.length();
    qDebug() << m_challenge << length;
    m_stream.writeRawData(m_challenge.data(), m_challenge.length());
    m_step = ERS_WaitSecurityChallenge;
}

bool QKxVNCServer::handleVerifyChallenge()
{
    if(!waitForRead(16)) {
        return false;
    }
    QByteArray passwd = QKxCapOption::instance()->authorizePassword().toLatin1();
    uchar key[9] = {0};
    for (int i = 0; i < 8; i++) {
        if (i < passwd.length()) {
            key[i] = passwd[i];
        } else {
            key[i] = 0;
        }
    }
    rfbDesKey(key, EN0);
    uchar out[17] = {0};
    uchar *challenge = (uchar*)m_challenge.data();
    for (int i = 0; i < 16; i += 8) {
        rfbDes(challenge + i, out + i);
    }
    char buf[17] = {0};
    m_stream.readRawData(buf, 16);
    qint32 ok = strcmp(buf, (char*)out) == 0 ? 0 : 1;
    m_stream.writeRawData((char*)&ok, sizeof(qint32));
    if(ok == 1) {
        QByteArray msg = "bad password";
        m_stream << msg;
        return false;
    }
    m_step = ERS_WaitClientInit;
    return true;
}

bool QKxVNCServer::handleInit()
{
    qint8 shared = 0;
    if(!waitForRead(1)) {
        return false;
    }
    // ignore share feather.
    m_stream.readRawData((char*)&shared, 1);
    //
    FrameInfo frame;
    frame.height = qToBigEndian<quint16>(m_deskRect.height());
    frame.width = qToBigEndian<quint16>(m_deskRect.width());
    frame.pixel = m_rgb32_888;
    frame.pixel.blueMax = qToBigEndian<quint16>(frame.pixel.blueMax);
    frame.pixel.redMax = qToBigEndian<quint16>(frame.pixel.redMax);
    frame.pixel.greenMax = qToBigEndian<quint16>(frame.pixel.greenMax);
    m_stream.writeRawData((char*)&frame, sizeof(frame));
    QByteArray name = "WoVNCServer:" + QKxUtils::deviceName();
    m_stream << name;
    m_step = ERS_WaitClientRequest;
    return true;
}

bool QKxVNCServer::sendColorMapEntry()
{
    m_stream << quint8(1) << quint8(0) << quint16(0) << quint16(m_clrTable.length());
    for(int i = 0; i < m_clrTable.length(); i++) {
        quint32 rgb = m_clrTable.at(i);
        quint16 r = ((rgb >> 16) & 0xFF) << 8;
        quint16 g = ((rgb >> 8) & 0xFF) << 8;
        quint16 b = (rgb & 0xFF) << 8;
        m_stream << r << g << b;
    }
    return true;
}

bool QKxVNCServer::handleSetPixelFormat()
{
    quint8 reserved;
    PixelFormat fmt;
    if(!waitForRead(3 + sizeof(fmt))) {
        return false;
    }
    m_stream >> reserved >> reserved >> reserved;
    m_stream.readRawData((char*)&fmt, sizeof(fmt));    
    fmt.blueMax = qFromBigEndian<quint16>(fmt.blueMax);
    fmt.greenMax = qFromBigEndian<quint16>(fmt.greenMax);
    fmt.redMax = qFromBigEndian<quint16>(fmt.redMax);
    for(auto it = m_caps.begin(); it != m_caps.end(); it++) {
        QKxScreenCapture *cap = it.value();
        cap->setFormat(fmt);
    }
    if(!fmt.trueColor) {
        return sendColorMapEntry();
    }
    return true;
}

bool QKxVNCServer::handleSetEncoding()
{
    quint8 reserved;
    quint16 cnt;
    if(!waitForRead(3)) {
        return false;
    }
    m_stream >> reserved >> cnt;
    if(!waitForRead(4 * cnt)) {
        return false;
    }
    m_codes.clear();
    for(int i = 0; i < cnt; i++) {
        qint32 t;
        m_stream >> t;
        qInfo() << "code type" << t;
        if(t == ET_OpenH264) {
            m_codes.insert(ET_OpenH264, &QKxVNCCompress::doOpenH264);
        }else if(t == ET_JPEG) {
            m_codes.insert(ET_JPEG, &QKxVNCCompress::doOpenJpeg);
        }else if(t == ET_TRLE3) {
            m_codes.insert(ET_TRLE3, &QKxVNCCompress::doTRLE3);
        }else if(t == ET_ZRLE3) {
            m_codes.insert(ET_ZRLE3, &QKxVNCCompress::doZRLE3);
        }else if(t == ET_TRLE){
            m_codes.insert(ET_TRLE, &QKxVNCCompress::doTRLE);
        }else if(t == ET_ZRLE){
            m_codes.insert(ET_ZRLE, &QKxVNCCompress::doZRLE);
        }else if(t == ET_Raw){
            m_codes.insert(ET_Raw, &QKxVNCCompress::doRaw);
        }else if(t == ET_Hextile) {
            m_codes.insert(ET_Hextile, &QKxVNCCompress::doHextile);
        }else if(t == ET_TRLE2) {
            m_codes.insert(ET_TRLE2, &QKxVNCCompress::doTRLE2);
        }else if(t == ET_ZRLE2) {
            m_codes.insert(ET_ZRLE2, &QKxVNCCompress::doZRLE2);
        }else if(t == ET_WoVNCScreenCount) {
            m_codes.insert(ET_WoVNCScreenCount, nullptr);
        }else if(t == ET_WoVNCMessageSupport) {
            m_codes.insert(ET_WoVNCMessageSupport, nullptr);
        }
    }
    for(auto it = m_caps.begin(); it != m_caps.end(); it++) {
        QKxScreenCapture *cap = it.value();
        cap->setCompressCode(m_codes);
    }
    //qDebug() << "handleSetEncoding" << m_codes.size();
    return !m_codes.isEmpty();
}

bool QKxVNCServer::sendScreenCount()
{
    if(!m_codes.contains(ET_WoVNCScreenCount)) {
        return true;
    }
    int width = m_deskRect.width();
    int height = m_deskRect.height();
    m_stream << qint8(0) << quint8(0) << quint16(1)
             << quint16(0) << quint16(0) << quint16(width) << quint16(height)
             << qint32(ET_WoVNCScreenCount);
    quint8 cnt = m_caps.size();
    m_stream << quint8(cnt);
    for(auto it = m_caps.begin(); it != m_caps.end(); it++) {
        QKxScreenCapture* cap = it.value();
        QRect rt = cap->imagePosition();
        m_stream << quint16(rt.left()) << quint16(rt.top()) << quint16(rt.width()) << quint16(rt.height());
    }
    return true;
}

bool QKxVNCServer::sendMessageSupport()
{
    if(!m_codes.contains(ET_WoVNCMessageSupport)) {
        return true;
    }
    int width = m_deskRect.width();
    int height = m_deskRect.height();
    m_stream << qint8(0) << quint8(0) << quint16(1)
             << quint16(0) << quint16(0) << quint16(width) << quint16(height)
             << qint32(ET_WoVNCMessageSupport);
    QList<ECSMessageType> typs;
    typs.append(ECST_ScreenBlack);
    typs.append(ECST_LockScreen);
    typs.append(ECST_Ftp);
#ifdef Q_OS_WIN
    typs.append(ECST_PlayAudio);
    if(m_privacyScreen != nullptr) {
        typs.append(ECST_PrivacyScreen);
    }
#endif
    m_stream << quint8(typs.length());
    for(int i = 0; i < typs.length(); i++) {
        ECSMessageType typ = typs.at(i);
        m_stream << qint32(typ);
    }
    return true;
}

bool QKxVNCServer::sendNoChangedFrame()
{
    m_stream << qint8(0) << quint8(0) << quint16(0);
    return true;
}

bool QKxVNCServer::handleFramebufferUpdateRequest()
{
    quint8 incr;
    quint16 x, y, w, h;
    if(!waitForRead(sizeof(quint16) * 4 + 1)) {
        return false;
    }
    m_stream >> incr >> x >> y >> w >> h;
    m_sts.requestCount++;
    if(m_token == nullptr) {
        m_token = new QTimer(this);
        QObject::connect(m_token, SIGNAL(timeout()), this, SLOT(onTokenArrived()));
        m_token->start(1000 / m_qpsMax);
    }else{
        onTokenArrived();
    }

    m_clipboard->queryForAppNap();

    //qDebug() << "handleFramebufferUpdateRequest" << m_tokenCount << QDateTime::currentMSecsSinceEpoch();
    if(m_firstFrame) {
        m_firstFrame = false;
        sendScreenCount();
        sendMessageSupport();
        sendNoChangedFrame();
        return true;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if(m_tokenCount < 0) {        
        return true;
    }
    m_tokenCount--;
    m_capTimeLast = now;
    QRect rt(x, y, w, h);
    emit nextFrame(rt, incr != 1);
    emit audioRequest(1);
    return true;
}

/*
 * short cut only exist on super-case alpha or digital-number or low-level key such as [,./]
 * short cut only has upper-case letter or number or low symbol.
*/
bool QKxVNCServer::handleKeyEvent()
{
    quint8 down, reserved;
    quint32 key;
    if(!waitForRead(7)) {
        return false;
    }
    m_stream >> down >> reserved >> reserved >> key;
    m_input->sendKeyEvent(down, key);
    return true;
}

bool QKxVNCServer::handlePointerEvent()
{
    quint8 button;
    quint16 x, y;
    if(!waitForRead(5)) {
        return false;
    }
    m_stream >> button >> x >> y;
    m_input->sendMouseEvent(button, x, y);
    return true;
}

bool QKxVNCServer::handleClientCutText()
{
    quint8 reserved;
    quint32 length;
    if(!waitForRead(3 + sizeof(quint32))) {
        return false;
    }
    m_stream >> reserved >> reserved >> reserved >> length;
    if(!waitForRead(length)) {
        return false;
    }
    QByteArray txt;
    txt.resize(length);
    m_stream.readRawData(txt.data(), length);
    QString cutTxt = QString::fromLatin1(txt);
    m_clipboard->setText(cutTxt);
    //qDebug() << "cutTxt" << cutTxt;
    return true;
}

bool QKxVNCServer::handleScreenBlack()
{
    quint8 on;
    if(!waitForRead(sizeof(quint8))) {
        return false;
    }
    m_stream >> on;
    m_wall->invert();
    return true;
}

bool QKxVNCServer::handleLockScreen()
{
    quint8 on;
    if(!waitForRead(sizeof(quint8))) {
        return false;
    }
    m_stream >> on;
    QKxUtils::lockWorkStation();
    return true;
}

bool QKxVNCServer::handlePlayAudio()
{
    if(!waitForRead(1)) {
        return false;
    }
    quint8 type;
    m_stream >> type;
    if(type == 1) {
        // enable or disable audio.
        if(!waitForRead(1)) {
            return false;
        }
        quint8 on;
        m_stream >> on;
        if(m_audio == nullptr) {
#ifdef Q_OS_WIN
            m_audio = new QKxWinAudioCapture(this);
            QObject::connect(this, SIGNAL(audioRequest(int)), m_audio, SLOT(onRequest(int)));
            QObject::connect(m_audio, SIGNAL(result(QByteArray)), this, SLOT(onAudioResult(QByteArray)));
#endif
        }
        if(m_audio) {
            if(on > 0) {
                m_audio->start();
            }else{
                m_audio->stop();
            }
        }else{
            on = 0;
        }
        m_stream << quint8(ECST_PlayAudio) << qint8(1) << qint8(on);
    }else if(type == 2) {
        // request next.
        if(!waitForRead(2)) {
            return false;
        }
        qint16 cnt;
        m_stream >> cnt;
        emit audioRequest(cnt);
    }
    return true;
}

bool QKxVNCServer::handleFtpPacket()
{
    quint32 cnt;
    if(!waitForRead(4)) {
        return false;
    }
    m_stream >> cnt;
    if(!waitForRead(cnt)) {
        return false;
    }
    QByteArray buf;
    buf.resize(cnt);
    m_stream.readRawData(buf.data(), cnt);

    if(m_ftpWorker == nullptr) {
        m_ftpWorker = new QKxFtpWorker(nullptr);
        m_ftpWorker->moveToThread(&m_ftpThread);
        QObject::connect(this, SIGNAL(ftpPacket(QByteArray)), m_ftpWorker, SLOT(onPacket(QByteArray)));
        QObject::connect(m_ftpWorker, SIGNAL(result(QByteArray)), this, SLOT(onFtpResult(QByteArray)));
        m_ftpThread.start();
    }
    emit ftpPacket(buf);
    return true;
}

bool QKxVNCServer::handlePrivacyScreen()
{
    if(!waitForRead(1)) {
        return false;
    }
    qint8 on;
    m_stream >> on;
    if(m_privacyScreen == nullptr) {
        m_stream << quint8(ECST_PrivacyScreen) << qint8(0);
        return true;
    }
    m_privacyScreen->setVisible(on == 1);
    m_stream << quint8(ECST_PrivacyScreen) << qint8(on);
    return true;
}


bool QKxVNCServer::waitForRead(qint64 cnt)
{
    if(m_stream.device() == nullptr) {
        return false;
    }
    if(cnt > 512) {
        qDebug() << "should need" << cnt << "actual has" << m_stream.device()->bytesAvailable();
    }
    while(m_stream.device()->bytesAvailable() < cnt) {
        if(cnt > 512) {
            qDebug() << "should need" << cnt << "actual has" << m_stream.device()->bytesAvailable();
        }
         if(!m_stream.device()->waitForReadyRead(3000)) {
            qDebug() << "should need" << cnt << "actual has" << m_stream.device()->bytesAvailable();
            return false;
        }
    }
    return true;
}

void QKxVNCServer::close()
{
    if(m_stream.device() == nullptr) {
        return;
    }
    flush();
    m_stream.device()->close();
}

QKxClipboard *QKxVNCServer::createClipboard(QObject *parent)
{
#if(defined (Q_OS_MAC))
    return new QKxMacClipboard(parent);
#elif(defined (Q_OS_LINUX))
    return new QKxX11Clipboard(parent);
#else
    return new QKxWinClipboard(parent);
#endif
}


QKxScreenCapture *QKxVNCServer::createCapture(int screenIndex, const QRect &rt, const QRect &imgRt, QObject *parent)
{
#if(defined(Q_OS_WIN))
#ifdef DO_NOT_USE_FOR_CURSOR_OVERLAY
    // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/desktop-dup-api
    // dxgi api would not get any cursor position mostly when a cursor work on overlay mode.
    // if you do still try to do this, must spend more time to handle this situation.
    QSysInfo::WinVersion ver = QSysInfo::windowsVersion();
    qDebug() << ver;
    if(ver > QSysInfo::WV_WINDOWS7) {
        // windows 8 / 8.1 / 10
        return new QKxDXGIScreenCapture(screenIndex, rt, imgRt, parent);
    }
#endif
    // xp / windows7 / window 8 / window 10
    return new QKxGDIScreenCapture(screenIndex, rt, imgRt, parent);
#elif(defined(Q_OS_MAC) || defined(Q_OS_MACOS))
    QSysInfo::MacVersion ver = QSysInfo::macVersion();
    return new QKxMacScreenCapture(screenIndex, rt, imgRt, parent);
#else
    // linux
    return new QKxX11ScreenCapture(screenIndex, rt, imgRt, parent);
#endif
    return nullptr;
}

QKxSendInput *QKxVNCServer::createInput(const QRect &capRt, const QRect &deskRt, QObject *parent)
{
#if(defined(Q_OS_WIN))
    return new QKxWinSendInput(capRt, deskRt, parent);
#elif(defined(Q_OS_MAC) || defined(Q_OS_MACOS))
    //QSysInfo::MacVersion ver = QSysInfo::macVersion();
    return new QKxMacSendInput(capRt, deskRt, parent);
#else
    // linux
    return new QKxX11SendInput(capRt, deskRt, parent);
#endif
    return nullptr;
}

QKxPrivacyScreen *QKxVNCServer::createPrivacyScreen(const QVector<QRect> &screenRt, QObject *parent)
{
#if(defined(Q_OS_WIN))
    QSysInfo::WinVersion ver = QSysInfo::windowsVersion();
    qDebug() << ver;
    if(ver > QSysInfo::WV_WINDOWS7) {
        return new QKxWin8PrivacyScreen(screenRt, parent);
    }
#endif
    return nullptr;
}


void QKxVNCServer::flush()
{
    if(m_stream.device() == nullptr) {
        return;
    }
#if 0
    // local socket do not need check.
    int left = m_stream.device()->bytesToWrite();
    int leftLast = left;
    int trycnt = 2;
    while( left > 0 && trycnt > 0) {
        m_stream.device()->waitForBytesWritten(50);
        left = m_stream.device()->bytesToWrite();
        if(left != leftLast) {
            leftLast = left;
            trycnt = 2;
        }else{
            trycnt--;
        }
    }
    if(trycnt == 0) {
        qDebug() << "big problem happen:" << m_stream.device()->errorString();;
    }
#endif
}
