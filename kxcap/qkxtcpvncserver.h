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

#ifndef QKXTCPVNCSERVER_H
#define QKXTCPVNCSERVER_H

#include "qkxcap_share.h"

#include <QObject>
#include <QPointer>
#include <QMap>
#include <QList>

class QTcpServer;
class QTcpSocket;
class QIODevice;
class QKxShareMemory;
class QProcess;

class KXCAP_EXPORT QKxTcpVNCServer : public QObject
{
    Q_OBJECT    
public:
    explicit QKxTcpVNCServer(QObject *parent = 0);
    virtual ~QKxTcpVNCServer();
    quint16 start(const QString& host, quint16 port = 0);
    void stop();

    int screenIndex();
    void setScreenIndex(int idx);
    QString authorizePassword();
    void setAuthorizePassword(const QString& pwd);
    int maxFPS() const;
    void setMaxFPS(int fps);
    void setEmptyFrameEnable(bool ok);
signals:

public slots:
    void onOpenNewConnection();
    void onLocalNewConnection();
    void onOpenDisconnected();
    void onLocalDisconnected();
    void onReadyRead();
    void onAboutToQuit();
    void onScreenChanged();
    void onVncProcessFinish();
private:
    void createPeer(bool bInner);
    void stopAllClient();
private:    
    QPointer<QTcpServer> m_openServer;
    QPointer<QTcpServer> m_localServer;
    QMap<QPointer<QIODevice>, QPointer<QIODevice> > m_pairs;
    QList<QPointer<QIODevice> > m_ready;
    QList<QPointer<QProcess> > m_vncs;
};

#endif // QKXTCPVNCSERVER_H
