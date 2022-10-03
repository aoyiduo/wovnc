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

#ifndef QKXX11CLIPBOARD_H
#define QKXX11CLIPBOARD_H

#include "qkxclipboard.h"
#include <QPointer>


class QKxX11Clipboard : public QKxClipboard
{
public:
    explicit QKxX11Clipboard(QObject *parent = nullptr);
    virtual ~QKxX11Clipboard();
private:
    virtual QString text() const;
};

#endif // QKXX11CLIPBOARD_H
