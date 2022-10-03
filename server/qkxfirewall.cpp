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

#include "qkxfirewall.h"

#include <QProcess>
#include <QDebug>
#include <QDir>
// netsh advfirewall firewall show rule name=WoVNCService2
// netsh advfirewall firewall add rule name=\"WoVNCService\" dir=in action=allow program=\"D:\wovnc\src\bin\wovncserver.exe\" enable=yes
// echo %errorlevel%

QKxFirewall::QKxFirewall(const QString &name, const QString &path, QObject *parent)
    : QObject(parent)
    , m_name(name)
    , m_path(QDir::toNativeSeparators(path))
{

}

bool QKxFirewall::add()
{
    QString cmd = QString("netsh advfirewall firewall add rule name=\"%1\" dir=in action=allow program=\"%2\" enable=yes").arg(m_name).arg(m_path);
    QString result;
    int err = runCommand(cmd, result);
    //qDebug() << "cmd" << cmd;
    //qDebug() << err << result;
    return err == 0;
}

bool QKxFirewall::remove()
{
    QString cmd = QString("netsh advfirewall firewall delete rule name=\"%1\"").arg(m_name);
    QString result;
    int err = runCommand(cmd, result);
    //qDebug() << "cmd" << cmd;
    //qDebug() << err << result;
    return err == 0;
}

bool QKxFirewall::exist()
{
    QString cmd = QString("netsh advfirewall firewall show rule name=\"%1\"").arg(m_name);
    QString result;
    int err = runCommand(cmd, result);
    //qDebug() << "cmd" << cmd;
    //qDebug() << err << result;
    return err == 0;
}

bool QKxFirewall::isActive()
{
    QString cmd = QString("netsh advfirewall firewall show rule name=\"%1\"").arg(m_name);
    QString result;
    int err = runCommand(cmd, result);
    //qDebug() << "cmd" << cmd;
    //qDebug() << err << result;
    return err == 0;
}

int QKxFirewall::runCommand(const QString &cmd, QString &result)
{
    QProcess process;
    process.start(cmd);
    if (!process.waitForFinished(-1) || process.error() == QProcess::FailedToStart){
        return -2;
    }
    if(process.exitStatus() == QProcess::NormalExit) {
        result = process.readAllStandardOutput();
        return process.exitCode();
    }
    return -3;
}
