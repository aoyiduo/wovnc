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

#ifndef QKXFIREWALL_H
#define QKXFIREWALL_H

#include <QObject>

class QKxFirewall : public QObject
{
    Q_OBJECT
public:
    explicit QKxFirewall(const QString& name, const QString& path, QObject *parent = 0);    
    bool add();
    bool remove();
    bool exist();
    bool isActive();
private:
    int runCommand(const QString& cmd, QString& result);
private:
    QString m_name;
    QString m_path;
};

#endif // QKXFIREWALL_H
