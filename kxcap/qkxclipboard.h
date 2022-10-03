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

#ifndef QKXCLIPBOARD_H
#define QKXCLIPBOARD_H

#include <QObject>

class QKxClipboard : public QObject
{
    Q_OBJECT
public:
    explicit QKxClipboard(QObject *parent = nullptr);
    virtual QString text() const;
    virtual void setText(const QString& txt);
    virtual void queryForAppNap();
signals:
    void dataChanged();
};

#endif // QKXCLIPBOARD_H
