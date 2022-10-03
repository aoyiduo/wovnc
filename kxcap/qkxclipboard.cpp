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

#include "qkxclipboard.h"

#include <QClipboard>
#include <QGuiApplication>

QKxClipboard::QKxClipboard(QObject *parent)
    : QObject(parent)
{

}

QString QKxClipboard::text() const
{
    return QString();
}

void QKxClipboard::setText(const QString &txt)
{
    QClipboard *clip = QGuiApplication::clipboard();
    QString clipTxt = clip->text();
    if(clipTxt != txt){
        clip->setText(txt);
    }
}

void QKxClipboard::queryForAppNap()
{

}
