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

#include "qkxmacclipboard.h"
#include "qkxmactimer.h"

#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QGuiApplication>
#include <QClipboard>

#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

QKxMacClipboard::QKxMacClipboard(QObject *parent)
    : QKxClipboard(parent)
{
    QTimer *timer = new QTimer(this);
    m_timer = timer;
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    timer->setInterval(250);
    timer->start();

    m_tmLast = QDateTime::currentMSecsSinceEpoch();
}

QKxMacClipboard::~QKxMacClipboard()
{

}

QString QKxMacClipboard::text() const
{
    return m_plainText;
}

void QKxMacClipboard::queryForAppNap()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    int ms = m_timer->interval();
    if(now - m_tmLast > ms) {
        copyFromClipboard();
        m_tmLast = now;
    }
}

void QKxMacClipboard::onTimeout()
{
    copyFromClipboard();
    m_tmLast = QDateTime::currentMSecsSinceEpoch();
}

void QKxMacClipboard::copyFromClipboard()
{
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    NSInteger newCount = [pb changeCount];
    if (newCount != m_countLast) {
        m_countLast = newCount;
        NSString *text = [pb stringForType:NSPasteboardTypeString];
        if (text){
            m_plainText = QString::fromNSString(text);
        }
        //qDebug() << "copyFromClipboard" << m_plainText;
        emit dataChanged();
    }
}
