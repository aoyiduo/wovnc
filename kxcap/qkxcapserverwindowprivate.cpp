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

#include "qkxcapserverwindowprivate.h"
#include "qtservice.h"
#include "qkxutils.h"
#include "qkxdaemonmaster.h"
#include "qkxdaemonslave.h"
#include "qkxtcpvncserver.h"
#include "qkxsharememory.h"
#include "qkxcap_private.h"
#include "qkxvncserverpeer.h"
#include "qkxutils.h"
#include "qkxserviceapplication.h"
#include "qkxcapoption.h"


#include <QDir>
#include <QDebug>
#include <QHostAddress>
#include <QApplication>

QKxCapServerWindowPrivate::QKxCapServerWindowPrivate(const QString &envName, const QString &host, quint16 port)
    : QKxCapServerPrivate(envName, host, port)
{
}

int QKxCapServerWindowPrivate::VncMain(int argc, char *argv[], PFunInit fn)
{
    QApplication app(argc, argv);

    fn(&app);

    QKxVNCServerPeer peer(&app);
    QObject::connect(&peer, SIGNAL(errorArrived(int)), &app, SLOT(quit()));
    return app.exec();
}

int QKxCapServerWindowPrivate::InstallMain(int argc, char *argv[], const QByteArray &serviceName)
{
    QtServiceController ctrl(serviceName);
    if(ctrl.isInstalled()) {
        QString path = ctrl.serviceFilePath();
        QString path2 = QDir::cleanPath(path);
        QString tmp = QCoreApplication::applicationFilePath();
        if(path2.contains(tmp)) {
            if(ctrl.isRunning()) {
                return 0;
            }
            ctrl.start();
            return 0;
        }
        if(ctrl.isRunning()) {
            ctrl.stop();
        }
        ctrl.uninstall();
        QThread::sleep(2);
    }
    QKxServiceApplication service(argc, argv, serviceName);
    int err = service.exec();
    if(ctrl.isInstalled()) {
        if(!ctrl.isRunning()) {
            ctrl.start();
        }
    }
    return err;
}

int QKxCapServerWindowPrivate::UninstallMain(int argc, char *argv[], const QByteArray &serviceName)
{
    QtServiceController ctrl(serviceName);
    if(ctrl.isInstalled()) {
        if(ctrl.isRunning()) {
            ctrl.stop();
        }
        ctrl.uninstall();
    }
    return 0;
}
