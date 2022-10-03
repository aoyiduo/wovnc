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

#include "qkxwinclipboard.h"

#include <QClipboard>
#include <QGuiApplication>

QKxWinClipboard::QKxWinClipboard(QObject *parent)
    : QKxClipboard(parent)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QObject::connect(clipboard, SIGNAL(dataChanged()), this, SIGNAL(dataChanged()));
}

QKxWinClipboard::~QKxWinClipboard()
{

}

QString QKxWinClipboard::text() const
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QString clipTxt = clipboard->text();
    return clipTxt;
}
