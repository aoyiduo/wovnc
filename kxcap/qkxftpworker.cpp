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

#include "qkxftpworker.h"
#include "qkxftpresponse.h"

#include <QDebug>

QKxFtpWorker::QKxFtpWorker(QObject *parent)
    : QObject(parent)
{
    m_ftp = new QKxFtpResponse(this);
    QObject::connect(m_ftp, SIGNAL(result(QByteArray)), this, SIGNAL(result(QByteArray)));
}

QKxFtpWorker::~QKxFtpWorker()
{

}

void QKxFtpWorker::onPacket(const QByteArray &buf)
{
    qDebug() << "onPacket" << buf.length();
    m_ftp->handlePacket(buf);
}
