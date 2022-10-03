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

#ifndef QKXVNCSERVER_H
#define QKXVNCSERVER_H

#include "qkxcap_private.h"

#include <QObject>
#include <QImage>
#include <QThread>
#include <QVector>
#include <QPointer>
#include <QMutex>

class QIODevice;
class QKxDXGICapture;
class QScreen;


class QKxScreenCapture;
class QKxCaptureAssist;
class QKxSendInput;
class QKxAudioCapture;
class QKxTimer;
class QKxWallPaper;
class QKxFtpWorker;
class QKxPrivacyScreen;
class QKxClipboard;

class QKxVNCServer : public QObject
{
    Q_OBJECT
    enum ERunStep{
        ERS_ReadyInit,
        ERS_FinishInit,
        ERS_WaitVerifyProtocol,
        ERS_WaitSecurityHandshake,
        ERS_WaitSecurityChallenge,
        ERS_WaitClientInit,
        ERS_WaitClientRequest  // ECSMessageType.
    };

    struct RunStatistic {
        int frameCount = 0;
        int requestCount = 0;
        int qps = 0;
        int totalBytes = 0;
        quint64 lastSecond = 0;
    };


    struct CaptureInfo {
        QThread *thread;
        QKxDXGICapture *dxgi;
        QRect geometry;
        QRect offsetGeometry;
        uchar *pbits; // draw;
        int bytesPerline;
    };

public:
    explicit QKxVNCServer(QObject *parent = 0);
    ~QKxVNCServer();

signals:
    void nextFrame(const QRect& rt, bool full);
    void ftpPacket(const QByteArray& buf);
    void audioRequest(int cnt);
public slots:
    void onReadyRead();
    void onAboutToClose();
    void onFrameResult(int idx, int err, int cnt);
    void onErrorArrived(int err);
    void onClipboardDataChanged();
    void onTokenArrived();

    void onFtpResult(const QByteArray& buf);
    void onAudioResult(const QByteArray& buf);
protected:
    bool restart(QIODevice *socket);
    void produceToken();

    bool handleRead();
    // protocol
    void sendSupportProtocol();
    bool handleVerifyProtocol();

    // security
    void sendSecurityHandshake();
    bool handleSecurityHandshake();
    void sendChallenge();
    bool handleVerifyChallenge();

    // client init
    bool handleInit();

    //
    bool sendColorMapEntry();
    bool handleSetPixelFormat();
    bool handleSetEncoding();
    bool sendScreenCount();
    bool sendMessageSupport();
    bool sendNoChangedFrame();
    bool handleFramebufferUpdateRequest();    
    bool handleKeyEvent();
    bool handlePointerEvent();
    bool handleClientCutText();
    bool handleScreenBlack();
    bool handleLockScreen();
    bool handlePlayAudio();
    bool handleFtpPacket();
    bool handlePrivacyScreen();

    //

    bool waitForRead(qint64 cnt);
    void flush();
    void close();
private:
    static QKxClipboard *createClipboard(QObject *parent=nullptr);
    static QKxScreenCapture *createCapture(int screenIndex, const QRect& rt, const QRect& imgRt, QObject *parent = nullptr);
    static QKxSendInput *createInput(const QRect& capRt, const QRect& deskRt, QObject *parent = nullptr);
    static QKxPrivacyScreen *createPrivacyScreen(const QVector<QRect>& screenRt, QObject *parent = nullptr);
protected:
    QRect m_deskRect;
    QDataStream m_stream;
    ERunStep m_step;
    QByteArray m_challenge;
    RunStatistic m_sts;

    QPointer<QKxClipboard> m_clipboard;

    QMutex m_zipMtx;
    QPointer<QKxZip> m_zip;
    QMap<int, QKxScreenCapture*> m_caps;
    QPointer<QKxSendInput> m_input;

    qint64 m_capTimeLast;
    bool m_firstFrame;
    int m_initCount;


    QPointer<QTimer> m_token;
    int m_tokenCount;
    qint64 m_tokenTimeLast;
    bool m_enableEmptyFrame;

    QPointer<QKxWallPaper> m_wall;

    QThread m_ftpThread;
    QPointer<QKxFtpWorker> m_ftpWorker;

    QPointer<QKxAudioCapture> m_audio;

    QPointer<QKxPrivacyScreen> m_privacyScreen;


    QMap<EEncodingType, PFunCompress> m_codes;

    static int m_qpsMax;
    static QVector<quint32> m_clrTable;
    static PixelFormat m_rgb32_888;
    static PixelFormat m_jpeg_444;
    static PixelFormat m_rgb16_565;
    static PixelFormat m_rgb15_555;
    static PixelFormat m_rgb8_332;
    static PixelFormat m_rgb8_map;
    static PixelFormat m_yuv_nv12;
    static PixelFormat m_h264_high;
    static PixelFormat m_h264_normal;
    static PixelFormat m_h264_low;
};

#endif // QKXVNCSERVER_H
