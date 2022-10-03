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

#ifndef QKXSERVICEPROCESS_H
#define QKXSERVICEPROCESS_H

#include <QThread>
#include <QStringList>
#include <QPointer>
#include <QMap>

class QProcess;
class QKxServiceProcess : public QThread
{
    Q_OBJECT
public:
    explicit QKxServiceProcess(bool asService, const QString& path, const QMap<QString,QString>& envs, QObject *parent);
    explicit QKxServiceProcess(const QMap<QString,QString>& envs, QObject *parent);
    virtual ~QKxServiceProcess();
    void setDaemon(bool on);
    void start();
    void stop();
private slots:
    void onFinished();
    void onAboutToQuit();
private:
    void run();
    bool createWinProcess();
    void recreate();
private:
    bool m_asService;
    QMap<QString, QString> m_envs;
    bool m_stop;
    QString m_path;
    qint64 m_hProcess;
    bool m_daemon;
    QPointer<QProcess> m_process;
};

#endif // QKXSERVICEPROCESS_H
