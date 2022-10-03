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

#include "qkxutils.h"

#include <QHostInfo>
#include <QSysInfo>
#include <QCoreApplication>
#include <QDir>
#include <QCryptographicHash>
#include <QLibraryInfo>
#include <QThread>
#include <QIODevice>
#include <QRect>
#include <QRegion>
#include <QProcess>

bool QKxUtils::isRunAsAdmin()
{
    return true;
}

bool QKxUtils::shellExecuteAsAdmin(const QString& path, const QStringList& argv, bool bWait)
{
    QProcess::startDetached(path, argv);
    return true;
}

bool QKxUtils::isRunAsService()
{
    QByteArray v = qgetenv("QTSERVICE_RUN");
    return !v.isEmpty();
}

bool QKxUtils::selectInputDesktop()
{
    return true;
}

bool QKxUtils::isDesktopChanged()
{
    return false;
}

bool QKxUtils::resetThreadDesktop()
{
    return false;
}

bool QKxUtils::disableAppNap()
{
    return true;
}

QString QKxUtils::applicationFilePath()
{
    static QString path;
    if(path.isEmpty()) {
        char szFile[128] = {0};
        sprintf(szFile, "/proc/self/cmdline");
        FILE *f = fopen(szFile, "r");
        if(f == nullptr) {
            return "";
        }
        QByteArray cmd;
        cmd.resize(1024);
        int n = fread(cmd.data(), 1, 1023, f);
        cmd.resize(n);
        QByteArrayList args = cmd.split(' ');
        QFileInfo fi(args[0]);
        QString tmp = fi.absoluteFilePath();
        path = QDir::toNativeSeparators(QDir::cleanPath(tmp));
    }
    return path;
}

QString QKxUtils::applicationDirPath()
{
    static QString path;
    if(path.isEmpty()) {
        QString tmp = applicationFilePath();
        QStringList dps = tmp.split('/');
        dps.pop_back();
        path = dps.join('/');
    }
    return path;
}

QString QKxUtils::applicationName()
{
    static QString path;
    if(path.isEmpty()) {
        QString tmp = applicationFilePath();
        QStringList dps = tmp.split('/');
        path = dps.last();
    }
    return path;
}

void QKxUtils::setVisibleOnDock(bool yes)
{

}

void QKxUtils::lockWorkStation()
{

}

void QKxUtils::setDragWindow(bool on)
{

}
