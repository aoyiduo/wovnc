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

#include "qkxvncserverpeer.h"
#include <QTimer>
#include <QLocalSocket>
#include <QTcpSocket>

QKxVNCServerPeer::QKxVNCServerPeer(QObject *parent)
    : QKxVNCServer(parent)
{
    QByteArray name = qgetenv("RPOXY_SERVER_NAME");
    if(name.startsWith("tcp:")) {
        QTcpSocket *tcp = new QTcpSocket(this);
        QByteArrayList hps = name.split(':');
        QByteArray host = hps.at(1);
        QByteArray port = hps.at(2);
        tcp->connectToHost(host, port.toInt());
        QObject::connect(tcp, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
        m_client = tcp;
    }else if(name.startsWith("local:")){
        QString svcName = name.mid(6);
        QLocalSocket *local = new QLocalSocket(this);
        local->connectToServer(svcName);
        QObject::connect(local, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
        m_client = local;
    }else {
        qWarning() << "never come here";
    }
    QTimer::singleShot(5 * 1000, this, SLOT(onConnectTimeout()));
    restart(m_client);
}

void QKxVNCServerPeer::onConnectTimeout()
{
    if(!m_client->isOpen()) {
        emit errorArrived(-1);
        return;
    }
}

void QKxVNCServerPeer::onDisconnected()
{
    qDebug() << "QKxVNCServerPeer::onDisconnected" << sender();
    emit errorArrived(-2);
}
