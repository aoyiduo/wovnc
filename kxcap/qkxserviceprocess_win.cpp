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

#include <Windows.h>


void QKxServiceProcess::start()
{
    HANDLE hdl = (HANDLE)m_hProcess;
    if(hdl == INVALID_HANDLE_VALUE) {
        createWinProcess();
        QThread::start();
    }
    m_stop = false;
}

void QKxServiceProcess::stop()
{
    HANDLE hdl = (HANDLE)m_hProcess;
    m_stop = true;
    if(hdl != INVALID_HANDLE_VALUE){
        ::TerminateProcess(hdl, 0);
        wait();
    }
}

void QKxServiceProcess::run()
{
    HANDLE hdl = (HANDLE)m_hProcess;
    ::WaitForSingleObject(hdl, INFINITE);
    ::CloseHandle(hdl);
    m_hProcess = (qint64)INVALID_HANDLE_VALUE;
}

bool QKxServiceProcess::createWinProcess()
{
    PROCESS_INFORMATION info;
    m_hProcess = (qint64)INVALID_HANDLE_VALUE;
    qDebug() <<"QKxServiceProcess::createWinProcess" << m_asService << m_path << m_envs;
    QStringList envs;
    for(QMap<QString,QString>::Iterator it = m_envs.begin(); it != m_envs.end(); it++) {
        envs.append(it.key() + "=" + it.value());
    }
    if(m_asService) {
        if(QKxUtils::createProcessWithAdmin(m_path, envs, (void*)&info)) {
            m_hProcess = (qint64)info.hProcess;
            return true;
        }
    }else{
        if(QKxUtils::createProcess(m_path, envs, (void*)&info)) {
            m_hProcess = (qint64)info.hProcess;
            return true;
        }
    }
    return false;
}
