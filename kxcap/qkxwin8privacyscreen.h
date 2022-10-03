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

#ifndef QKXWIN8PRIVACYSCREEN_H
#define QKXWIN8PRIVACYSCREEN_H

#include "qkxprivacyscreen.h"

#include <QVector>
#include <QPointer>


class QKxWin8SubPrivacyScreen;
class QKxWin8PrivacyScreen : public QKxPrivacyScreen
{
    Q_OBJECT
public:
    explicit QKxWin8PrivacyScreen(const QVector<QRect>& screenRt, QObject *parent = nullptr);
    ~QKxWin8PrivacyScreen();
    bool isVisible();
    void setVisible(bool on);
    bool snapshot(int idx, uchar *pbuf, int bytesPerline, int width, int height) const;
private:
    QVector<QPointer<QKxWin8SubPrivacyScreen>> m_subs;
};

#endif // QKXWIN8PRIVACYSCREEN_H
