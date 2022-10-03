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

#ifndef QKXCAPSERVERWINDOWPRIVATE_H
#define QKXCAPSERVERWINDOWPRIVATE_H

#include "qkxcap_share.h"
#include "qkxcapserverprivate.h"

#include <QPointer>
#include <QObject>

class QKxTcpVNCServer;
class QKxCapServerWindowPrivate : public QKxCapServerPrivate
{
    Q_OBJECT
public:
    explicit QKxCapServerWindowPrivate(const QString& envName, const QString &host, quint16 port);

    // vnc module.
    static int VncMain(int argc, char *argv[], PFunInit fn);
    // install service.
    static int InstallMain(int argc, char *argv[], const QByteArray &serviceName);
    // uninstall service.
    static int UninstallMain(int argc, char *argv[], const QByteArray &serviceName);
};

#endif // QKXCAPSERVERWINDOWPRIVATE_H
