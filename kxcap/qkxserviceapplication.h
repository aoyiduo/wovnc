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

#ifndef QKXCAPAPPLICATION_H
#define QKXCAPAPPLICATION_H

#include "qkxcap_share.h"

#include <QPointer>
#include <QCoreApplication>
#include "qtservice.h"

class QKxServiceProcess;
class QKxDaemonMaster;
class KXCAP_EXPORT QKxServiceApplication : public QtService<QCoreApplication>
{
public:
    explicit QKxServiceApplication(int argc, char **argv, const QString& serviceName);
    virtual ~QKxServiceApplication();
protected:
    void start();
    void stop();
    void pause();
    void resume();
    void processCommand(int code);
    void createApplication(int &argc, char **argv);
    int executeApplication();
private:
    QPointer<QKxServiceProcess> m_proxy;
    QPointer<QKxDaemonMaster> m_master;
};

#endif // QKXCAPAPPLICATION_H
