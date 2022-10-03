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

#ifndef QKXDXGISCREENCAPTURE_H
#define QKXDXGISCREENCAPTURE_H

#include "qkxscreencapture.h"

class QKxDXGIScreenCapture : public QKxScreenCapture
{
    Q_OBJECT
public:
    explicit QKxDXGIScreenCapture(int screenIndex, const QRect& rt, const QRect& imgRt, QObject *parent = nullptr);
protected:
    virtual int running();
};

#endif // QKXDXGISCREENCAPTURE_H
