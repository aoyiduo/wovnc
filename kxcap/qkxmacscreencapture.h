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

#ifndef QKXMACSCREENCAPTURE_H
#define QKXMACSCREENCAPTURE_H

#include "qkxscreencapture.h"

class QKxMacScreenCapture : public QKxScreenCapture
{
public:
public:
    explicit QKxMacScreenCapture(int screenIndex, const QRect& rt, const QRect& imgRt, QObject *parent = nullptr);
private:
    virtual int running();
};

#endif // QKXMACSCREENCAPTURE_H
