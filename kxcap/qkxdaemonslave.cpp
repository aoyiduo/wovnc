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

#include "qkxdaemonslave.h"

#include <QLocalSocket>
#include <QMessageBox>
#include <QTimer>

QKxDaemonSlave::QKxDaemonSlave(const QString& name, QObject *parent)
    : QObject(parent)
    , m_name(name)
    , m_count(0)
{
    m_slave = new QLocalSocket(this);
    m_slave->connectToServer(m_name);
    QObject::connect(m_slave, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    QObject::connect(m_slave, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    bool ok = QObject::connect(m_slave, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(onError()));
    bool ok2 = QObject::connect(m_slave, SIGNAL(stateChanged(QLocalSocket::LocalSocketState)), this, SLOT(onStateChagned()));
    m_tick = new QTimer(this);
    QObject::connect(m_tick, SIGNAL(timeout()), this, SLOT(onTick()));
    m_tick->start(10 * 1000);
}

void QKxDaemonSlave::onDisconnected()
{
    qDebug() << "QKxDaemonSlave::onDisconnected";
    emit quitArrived();
}

void QKxDaemonSlave::onReadyRead()
{
    QByteArray cmd = m_slave->readAll();
    if(cmd == "activateArrived") {
        QMetaObject::invokeMethod(this, cmd, Qt::QueuedConnection);
    }
}

void QKxDaemonSlave::onError()
{
    qDebug() << "QKxDaemonSlave::onError";
    emit quitArrived();
}

void QKxDaemonSlave::onStateChagned()
{
    qDebug() << "stateChanged";
}

void QKxDaemonSlave::onTick()
{
    if(!m_slave->isOpen()) {
        qDebug() << "QKxDaemonSlave::onTick";
        emit quitArrived();
        return;
    }
    m_slave->write("hello");
}
