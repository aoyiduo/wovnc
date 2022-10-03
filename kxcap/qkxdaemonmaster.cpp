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

#include "qkxdaemonmaster.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QCoreApplication>

QKxDaemonMaster::QKxDaemonMaster(const QString& name, QObject *parent)
    : QObject(parent)
    , m_name(name)
{
    m_master = new QLocalServer(this);
    m_master->setSocketOptions(QLocalServer::WorldAccessOption);
    QObject::connect(m_master, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
    QLocalServer::removeServer(name);
    if(!m_master->listen(name)){
        qWarning() << "QLocalServer" << name << m_master->errorString();
    }
}

QKxDaemonMaster::~QKxDaemonMaster()
{
}

void QKxDaemonMaster::activeSlave()
{
    for(int i = 0; i < m_slaves.length(); i++) {
        QLocalSocket *slave = m_slaves.at(i);
        if(slave) {
            slave->write("activateArrived");
        }
    }
}

void QKxDaemonMaster::onNewConnection()
{
    while(m_master->hasPendingConnections()) {
        QLocalSocket *slave = m_master->nextPendingConnection();
        QObject::connect(slave, &QLocalSocket::disconnected, this, &QKxDaemonMaster::onDisconnected);
        QObject::connect(slave, &QLocalSocket::readyRead, this, &QKxDaemonMaster::onReadyRead);
        m_slaves.append(slave);
    }
}

void QKxDaemonMaster::onDisconnected()
{
    QLocalSocket *slave = qobject_cast<QLocalSocket*>(sender());
    slave->deleteLater();
    m_slaves.removeAll(slave);
}

void QKxDaemonMaster::onReadyRead()
{
    QLocalSocket *slave = qobject_cast<QLocalSocket*>(sender());
    QByteArray buf = slave->readAll();
}
