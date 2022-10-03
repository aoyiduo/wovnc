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

#ifndef QKXPRIVACYSCREEN_H
#define QKXPRIVACYSCREEN_H

#include <QThread>
#include <QVector>
#include <QRect>

class QKxPrivacyScreen : public QObject
{
    Q_OBJECT
public:
    explicit QKxPrivacyScreen(QObject *parent = nullptr);
    ~QKxPrivacyScreen();
    virtual bool isVisible() = 0;
    virtual void setVisible(bool on) = 0;
    virtual bool snapshot(int idx, uchar *pbuf, int bytesPerline, int width, int height) const = 0;
signals:
    void deactivedArrived();
};

#endif // QKXPRIVACYSCREEN_H
