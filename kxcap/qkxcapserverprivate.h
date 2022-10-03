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

#ifndef QKXCAPSERVERPRIVATE_H
#define QKXCAPSERVERPRIVATE_H

#include <QObject>
#include <QPointer>
#include <QMap>
#include <QWaitCondition>
#include <QMutex>

class QThread;
class QKxTcpVNCServer;
class QKxCapServerPrivate : public QObject
{
    Q_OBJECT
public:
    explicit QKxCapServerPrivate(const QString& envName, const QString &host, quint16 port);
    virtual ~QKxCapServerPrivate();
    quint16 listenPort();
private:
    // port: 0 will auto assign port by os system and new port will be return value.
    Q_INVOKABLE void init(const QString &host, quint16 port = 0);
    Q_INVOKABLE void close();
    void updateServerUrl(const QString& name);
signals:

public slots:
protected:
    QPointer<QThread> m_worker;
    QPointer<QKxTcpVNCServer> m_server;
    QString m_envName;
    bool m_bClose;

    quint16 m_port;

    QWaitCondition m_wait;
    QMutex m_mtx;
};

#endif // QKXCAPSERVERPRIVATE_H
