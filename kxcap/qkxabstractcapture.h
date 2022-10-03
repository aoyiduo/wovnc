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

#ifndef QKXABSTRACTCAPTURE_H
#define QKXABSTRACTCAPTURE_H

#include <QObject>
#include "qkxcap_private.h"


class QKxAbstractCapture : public QObject
{
    Q_OBJECT
public:
    explicit QKxAbstractCapture(QObject *parent = 0);
    virtual void showError(const QString& msg);
    virtual int drawFrame(UpdateRequest *req, uchar *pbuf, int bytesPerLine, bool reset) = 0;
};

#endif // QKXABSTRACTCAPTURE_H
