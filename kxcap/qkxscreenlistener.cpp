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

#include "qkxscreenlistener.h"
#include "qkxutils.h"

#include <QApplication>
#include <QScreen>
#include <QDesktopWidget>
#include <QDebug>
#include <QWindow>
#include <QTimer>

QKxScreenListener::QKxScreenListener(QObject *parent)
    : QObject(parent)
{
    QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
}

int QKxScreenListener::screenCount() const
{
    return m_dis.length();
}

QList<QKxScreenListener::DisplayInfo> QKxScreenListener::screens()
{
    QKxScreenListener listener;
    return listener.monitors();
}

void QKxScreenListener::onDisplayCheck()
{
    //test();
    QList<DisplayInfo> dis = monitors();
    if(dis.length() != m_dis.length()) {
        m_dis.swap(dis);
        emit screenChanged();
    }else{
        bool changed = false;
        for(int i = 0; i < dis.length(); i++) {
            const DisplayInfo &old = m_dis.at(i);
            const DisplayInfo &now = dis.at(i);
            if(old.name != now.name || old.rect != now.rect) {
                changed = true;
                break;
            }
        }
        if(changed) {
            m_dis.swap(dis);
            emit screenChanged();
        }
    }
}

void QKxScreenListener::init()
{
    m_dis = monitors();
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, this, &QKxScreenListener::onDisplayCheck);
    timer->start(1000);
    emit screenChanged();
}

void QKxScreenListener::test()
{
    if(QKxUtils::isDesktopChanged()) {
        QKxUtils::resetThreadDesktop();
    }
}
