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

#include "qkxserviceprocess.h"
#include "qkxutils.h"

#include <QCoreApplication>
#include <QDebug>

#ifdef Q_OS_WIN
#include <Windows.h>
#else
#endif

QKxServiceProcess::QKxServiceProcess(bool asService, const QString& path, const QMap<QString,QString>& envs, QObject *parent)
    : QThread(parent)
    , m_asService(asService)
    , m_envs(envs)
    , m_stop(false)
    , m_path(path)
    , m_daemon(true)
{
    m_hProcess = (qint64)-1;
    bool ok = QObject::connect(this, SIGNAL(finished()), this, SLOT(onFinished()), Qt::QueuedConnection);
    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));
}

QKxServiceProcess::QKxServiceProcess(const QMap<QString, QString> &envs, QObject *parent)
    :QKxServiceProcess(false, QCoreApplication::applicationFilePath(), envs, parent)
{

}

QKxServiceProcess::~QKxServiceProcess()
{
    if(!m_stop) {
        stop();
    }
}

void QKxServiceProcess::setDaemon(bool on)
{
    m_daemon = on;
}

void QKxServiceProcess::onFinished()
{
    if(!m_stop && m_daemon) {
        start();
    }
}

void QKxServiceProcess::onAboutToQuit()
{
    stop();
}
