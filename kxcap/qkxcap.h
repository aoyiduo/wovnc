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

#ifndef QKXCAP_H
#define QKXCAP_H

#include "qkxcap_share.h"

#include <QObject>

class KXCAP_EXPORT QKxCap : public QObject
{
    Q_OBJECT
public:
    explicit QKxCap(QObject *parent = nullptr);

signals:

public slots:
};

#endif // QKXCAP_H
