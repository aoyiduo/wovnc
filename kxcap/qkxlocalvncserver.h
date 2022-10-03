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

#ifndef QKXLOCALVNCSERVER_H
#define QKXLOCALVNCSERVER_H

#include "qkxcap_share.h"

#include <QObject>
#include <QPointer>
#include <QMap>
#include <QList>

class QLocalServer;
class QLocalSocket;
class QIODevice;
class QKxShareMemory;
class QProcess;

/***
 *    do not use this class, because the performance is worse than tcp.
 *    http://www.kxtry.com/archives/3432
 *
 *    do not use it.
 *    do not use it.
 *    do not use it.
*/

class QKxLocalVNCServer : public QObject
{
    Q_OBJECT
public:
    explicit QKxLocalVNCServer(QObject *parent = 0);
    virtual ~QKxLocalVNCServer();
    bool start(const QString& name);
    int screenIndex();
    void setScreenIndex(int idx);
    QString authorizePassword();
    void setAuthorizePassword(const QString& pwd);
    int maxFPS() const;
    void setMaxFPS(int fps);
    void setEmptyFrameEnable(bool ok);
    void stop();
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
    QPointer<QLocalServer> m_openServer;
    QPointer<QLocalServer> m_localServer;
    QMap<QPointer<QIODevice>, QPointer<QIODevice> > m_pairs;
    QList<QPointer<QIODevice> > m_ready;
    QList<QPointer<QProcess> > m_vncs;
};

#endif // QKXLOCALVNCSERVER_H
