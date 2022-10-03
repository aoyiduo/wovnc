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

#include "qkxcapserver.h"
#include "qkxdaemonslave.h"
#include "qkxscreenlistener.h"
#include "qkxcapoption.h"
#include "qkxaudiocapture.h"
#include "qkxwinaudiocapture.h"
#include "qkxutils.h"

#ifdef Q_OS_WIN
#include "qkxcapserverwindowprivate.h"
#else
#include "qkxcapserverunixprivate.h"
#endif
#include <QCoreApplication>
#include <QDebug>

QKxCapServer::QKxCapServer(const QString& envName, const QString &host, quint16 port, QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
    m_prv = new QKxCapServerWindowPrivate(envName, host, port);
#else
    m_prv = new QKxCapServerUnixPrivate(envName, host, port);
#endif
    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));
}

QKxCapServer::~QKxCapServer()
{
    if(m_prv) {
        delete m_prv;
    }
}

quint16 QKxCapServer::listenPort()
{
    if(m_prv) {
        return m_prv->listenPort();
    }
    return 0;
}

void QKxCapServer::test()
{

}

QKxCapOption *QKxCapServer::option()
{
    return QKxCapOption::instance();
}

bool QKxCapServer::isRunAsService()
{
    QByteArray name = qgetenv("DAEMON_SERVER_NAME");
    return !name.isEmpty();
}

void QKxCapServer::ExitWithParentProcess(QCoreApplication *app)
{
    QByteArray name = qgetenv("DAEMON_SERVER_NAME");
    qDebug() << "Master Listen Name" << name;
    if(!name.isEmpty()){
        // should be from system service call.
        QKxDaemonSlave *slave = new QKxDaemonSlave(name, app);
        QObject::connect(slave, SIGNAL(quitArrived()), app, SLOT(quit()));
    }
}

int QKxCapServer::VncMain(int argc, char *argv[], PFunInit fn)
{
#ifdef Q_OS_WIN
    return QKxCapServerWindowPrivate::VncMain(argc, argv, fn);
#else
    return QKxCapServerUnixPrivate::VncMain(argc, argv, fn);
#endif
}

int QKxCapServer::InstallMain(int argc, char *argv[], const QByteArray &serviceName)
{
#ifdef Q_OS_WIN
    return QKxCapServerWindowPrivate::InstallMain(argc, argv, serviceName);
#else
    return QKxCapServerUnixPrivate::InstallMain(argc, argv, serviceName);
#endif
}

int QKxCapServer::UninstallMain(int argc, char *argv[], const QByteArray &serviceName)
{
#ifdef Q_OS_WIN
    return QKxCapServerWindowPrivate::UninstallMain(argc, argv, serviceName);
#else
    return QKxCapServerUnixPrivate::UninstallMain(argc, argv, serviceName);
#endif
}

void QKxCapServer::onAboutToQuit()
{
    if(m_prv) {
        delete m_prv;
    }
}
