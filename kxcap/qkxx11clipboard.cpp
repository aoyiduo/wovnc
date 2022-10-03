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

#include "qkxx11clipboard.h"

#include <QClipboard>
#include <QGuiApplication>

QKxX11Clipboard::QKxX11Clipboard(QObject *parent)
    : QKxClipboard(parent)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QObject::connect(clipboard, SIGNAL(dataChanged()), this, SIGNAL(dataChanged()));
}

QKxX11Clipboard::~QKxX11Clipboard()
{

}

QString QKxX11Clipboard::text() const
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QString clipTxt = clipboard->text();
    return clipTxt;
}
