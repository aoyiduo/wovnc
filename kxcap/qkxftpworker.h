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

#ifndef QKXFTPWORKER_H
#define QKXFTPWORKER_H

#include <QObject>
#include <QPointer>

class QKxFtpResponse;
class QKxLengthBodyPacket;
class QKxFtpWorker : public QObject
{
    Q_OBJECT
public:
    explicit QKxFtpWorker(QObject *parent = nullptr);
    ~QKxFtpWorker();
signals:
    void result(const QByteArray& buf);
private slots:
    void onPacket(const QByteArray& buf);
private:
    QPointer<QKxFtpResponse> m_ftp;
};

#endif // QKXFTPWORKER_H
