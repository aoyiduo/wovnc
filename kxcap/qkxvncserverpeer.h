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

#ifndef QKXVNCSERVERPEER_H
#define QKXVNCSERVERPEER_H

#include "qkxcap_share.h"
#include "qkxvncserver.h"
#include <QPointer>

class QLocalSocket;
class QTcpSocket;
class QTimer;

class KXCAP_EXPORT QKxVNCServerPeer : public QKxVNCServer
{
    Q_OBJECT
public:
    explicit QKxVNCServerPeer(QObject *parent = 0);
signals:
    void errorArrived(int err);
public slots:
    void onConnectTimeout();
    void onDisconnected();
private:
    QPointer<QIODevice> m_client;
    QPointer<QTimer> m_tick;
};

#endif // QKXVNCSERVERPEER_H
