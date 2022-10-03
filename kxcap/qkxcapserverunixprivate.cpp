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

#include "qkxcapserverunixprivate.h"
#include "qkxcap_private.h"
#include "qkxtcpvncserver.h"
#include "qkxcapoption.h"
#include "qkxvncserverpeer.h"
#include "qkxutils.h"

#include <QVariant>
#include <QDir>
#include <QDebug>
#include <QApplication>
#include <QDateTime>
#include <QPainter>

QKxCapServerUnixPrivate::QKxCapServerUnixPrivate(const QString& envName, const QString &host, quint16 port)
    : QKxCapServerPrivate(envName, host, port)
{

}

int QKxCapServerUnixPrivate::VncMain(int argc, char *argv[], PFunInit fn)
{
    QApplication app(argc, argv);

    fn(&app);

    QKxUtils::setVisibleOnDock(false);

    QKxVNCServerPeer peer(&app);
    QObject::connect(&peer, SIGNAL(errorArrived(int)), &app, SLOT(quit()));
    return app.exec();
}

int QKxCapServerUnixPrivate::InstallMain(int argc, char *argv[], const QByteArray &serviceName)
{
    //
    throw "do not need to impletement";
    return 0;
}

int QKxCapServerUnixPrivate::UninstallMain(int argc, char *argv[], const QByteArray &serviceName)
{
    // do not need to impletement
    throw "do not need to impletement";
    return 0;
}
