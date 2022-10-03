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

#include "qkxcapserverprivate.h"
#include "qkxcapoption.h"
#include "qkxtcpvncserver.h"

#include <QDebug>
#include <QThread>
#include <QMutexLocker>

QKxCapServerPrivate::QKxCapServerPrivate(const QString& envName, const QString &host, quint16 port)
    : QObject(nullptr)
    , m_bClose(false)
    , m_envName(envName)
    , m_port(0)
{
    m_worker = new QThread(nullptr);
    moveToThread(m_worker);
    m_worker->start();
    QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection, Q_ARG(QString, host), Q_ARG(quint16, port));

    QMutexLocker lk(&m_mtx);
    m_wait.wait(&m_mtx);
}

QKxCapServerPrivate::~QKxCapServerPrivate()
{
    QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
    m_worker->wait();
    delete m_worker;
}

quint16 QKxCapServerPrivate::listenPort()
{
    return m_port;
}

void QKxCapServerPrivate::init(const QString &host, quint16 port)
{
    m_server = new QKxTcpVNCServer(this);
    port = m_server->start(host, port);
    QString vncServer;
    if(port > 0) {
        vncServer = QString("tcp:%1:%2").arg(host).arg(port);
        m_port = port;
    }
    updateServerUrl(vncServer);
    m_wait.wakeAll();
}

void QKxCapServerPrivate::close()
{
    if(m_server) {
        m_server->stop();
        delete m_server;
    }
    updateServerUrl("");
    m_worker->quit();
}

void QKxCapServerPrivate::updateServerUrl(const QString &name)
{
    QByteArray envName = m_envName.toUtf8();
    qputenv(envName, name.toUtf8());
    QKxCapOption::instance()->setServerUrl(name);
}
