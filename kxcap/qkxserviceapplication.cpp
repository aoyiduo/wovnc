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

#include "qkxserviceapplication.h"
#include "qkxdaemonmaster.h"
#include "qkxserviceprocess.h"

#include "qkxutils.h"
#include "qkxcap_private.h"


#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>
#include <QThread>

QKxServiceApplication::QKxServiceApplication(int argc, char **argv, const QString& serviceName)
    : QtService<QCoreApplication>(argc, argv, serviceName)
{
    QStringList args = runtimeArguments();
    logMessage("args:" + args.join(","));
    if(args.contains("-i") || args.contains("-install")) {
        setServiceDescription(serviceName + QString(" is a remote desktop tools."));
        setServiceFlags(QtServiceBase::CannotBeStopped);
        setStartupType(QtServiceController::ManualStartup);
        QStringList args;
        args.append("-service");
        setStartupArguments(args);
    }
}

QKxServiceApplication::~QKxServiceApplication()
{

}

void QKxServiceApplication::start()
{
    qDebug() << "start";
}

void QKxServiceApplication::stop()
{
    qDebug() << "stop";
}

void QKxServiceApplication::pause()
{
    logMessage("pause");
}

void QKxServiceApplication::resume()
{
    logMessage("resume");
}

void QKxServiceApplication::processCommand(int code)
{
    logMessage(QString("processCommand:%1").arg(code));
    if(code == SERVICE_COMMAND_CODE_START) {
        m_proxy->start();
    }else if(code == SERVICE_COMMAND_CODE_STOP) {
        m_proxy->stop();
    }else if(code == SERVICE_COMMAND_CODE_EXIT) {
        // service exit.
        QCoreApplication::quit();
        return;
    }
}

void QKxServiceApplication::createApplication(int &argc, char **argv)
{
    QtService::createApplication(argc, argv);
    QCoreApplication *app = QCoreApplication::instance();
}

int QKxServiceApplication::executeApplication()
{
    QCoreApplication *app = QCoreApplication::instance();
    qDebug() << "serviceConfigureFilePath" << serviceConfigureFilePath();

    qint64 pid = QCoreApplication::applicationPid();
    QString svcName = QString("service:%1").arg(pid);
    QMap<QString,QString> envs;
    envs.insert("RUN_ACTION_NAME", QString("main:%1").arg(pid));
    envs.insert("DAEMON_SERVER_NAME", QString("%1").arg(svcName));
    QString path = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    QKxServiceProcess proxy(asService(), path, envs, app);
    proxy.start();
    QKxDaemonMaster master(svcName, app);
    m_master = &master;
    m_proxy = &proxy;
    return app->exec();
}
