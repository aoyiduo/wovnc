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
#include <QProcess>

void QKxServiceProcess::start()
{
    if(isRunning()) {
        return;
    }
    QThread::start();
    m_stop = false;
}

void QKxServiceProcess::stop()
{
    m_stop = true;
    if(m_process != nullptr) {
        m_process->kill();
        wait();
    }
}

void QKxServiceProcess::run()
{
    createWinProcess();
}

bool QKxServiceProcess::createWinProcess()
{
    QProcess proc;
    m_process = &proc;
    proc.setProgram(QCoreApplication::applicationFilePath());
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for(QMap<QString, QString>::Iterator it = m_envs.begin(); it != m_envs.end(); it++) {
        env.insert(it.key(), it.value());
    }
    proc.setProcessEnvironment(env);
    proc.start();
    proc.waitForFinished(-1);
    return true;
}
