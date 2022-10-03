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

#ifndef QKXDAEMONMASTER_H
#define QKXDAEMONMASTER_H

#include <QObject>
#include <QPointer>
#include <QList>

class QLocalServer;
class QLocalSocket;
class QKxDaemonMaster : public QObject
{
    Q_OBJECT
public:
    explicit QKxDaemonMaster(const QString& name, QObject *parent = 0);
    virtual ~QKxDaemonMaster();
    void activeSlave();
signals:

public slots:
    void onNewConnection();
    void onDisconnected();
    void onReadyRead();
private:
    QString m_name;
    QPointer<QLocalServer> m_master; // only for check master is exist or not.
    QList<QPointer<QLocalSocket>> m_slaves;
};

#endif // QKXDAEMONMASTER_H
