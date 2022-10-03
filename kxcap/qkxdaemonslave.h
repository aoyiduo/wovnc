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

#ifndef QKXDAEMONSLAVE_H
#define QKXDAEMONSLAVE_H

#include <QObject>
#include <QPointer>

class QTimer;
class QLocalSocket;
class QKxDaemonSlave : public QObject
{
    Q_OBJECT
public:
    explicit QKxDaemonSlave(const QString& name, QObject *parent = 0);

signals:
    void activateArrived();
    void quitArrived();
public slots:
    void onDisconnected();
    void onReadyRead();
    void onError();
    void onStateChagned();
    void onTick();
private:
    QPointer<QLocalSocket> m_slave;
    QPointer<QTimer> m_tick;
    QString m_name;
    int m_count;
};

#endif // QKXDAEMONSLAVE_H
