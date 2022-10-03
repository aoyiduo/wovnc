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

#ifndef QKXCAPSERVER_H
#define QKXCAPSERVER_H

#include "qkxcap_share.h"

#include <QObject>
#include <QMap>
#include <QPointer>

class QCoreApplication;
class QKxCapOption;
class QKxCapServerPrivate;

class KXCAP_EXPORT QKxCapServer : public QObject
{
    Q_OBJECT
public:
    explicit QKxCapServer(const QString& envName, const QString& host, quint16 port, QObject *parent = nullptr);
    ~QKxCapServer();
    quint16 listenPort();

    Q_INVOKABLE void test();

    // same as QKxCapOption::instance();
    static QKxCapOption *option();
    static bool isRunAsService();
    // for window service
    static void ExitWithParentProcess(QCoreApplication *app);
    // vnc module.
    static int VncMain(int argc, char *argv[], PFunInit fn);
    // install service.
    static int InstallMain(int argc, char *argv[], const QByteArray &serviceName);
    // uninstall service
    static int UninstallMain(int argc, char *argv[], const QByteArray &serviceName);
private slots:
    void onAboutToQuit();
private:
    QPointer<QKxCapServerPrivate> m_prv;
};

#endif // QKXCAPSERVER_H
