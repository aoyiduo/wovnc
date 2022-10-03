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

#ifndef QKXSCREENLISTENER_H
#define QKXSCREENLISTENER_H

#include "qkxcap_share.h"

#include <QRect>
#include <QObject>
#include <QPointer>

class QDesktopWidget;
class KXCAP_EXPORT QKxScreenListener : public QObject
{
    Q_OBJECT
public:
    struct DisplayInfo {
        QRect rect;
        QString name;
    };
public:
    explicit QKxScreenListener(QObject *parent = 0);
    int screenCount() const;
    static QList<DisplayInfo> screens();
signals:
    void screenChanged();
private slots:
    void onDisplayCheck();
protected:
    QList<DisplayInfo> monitors();
    Q_INVOKABLE void init();
    void test();
private:
    QList<DisplayInfo> m_dis;
};

#endif // QKXSCREENLISTENER_H
