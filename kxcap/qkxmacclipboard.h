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

#ifndef QKXMACCLIPBOARD_H
#define QKXMACCLIPBOARD_H

#include "qkxclipboard.h"

#include <QTimer>
#include <QPointer>

class QKxMacClipboard : public QKxClipboard
{
    Q_OBJECT
public:
    explicit QKxMacClipboard(QObject *parent = nullptr);
    ~QKxMacClipboard();
protected:
    virtual QString text() const;
    virtual void queryForAppNap();
private slots:
    void onTimeout();
private:
    void copyFromClipboard();
private:
    QPointer<QTimer> m_timer;
    int m_countLast;
    QString m_plainText;
    qint64 m_tmLast;
};

#endif // QKXMACCLIPBOARD_H
