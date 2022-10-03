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

#ifndef QKXSENDINPUT_H
#define QKXSENDINPUT_H

#include "qkxcap_private.h"

#include <QPointer>
#include <QThread>
#include <QMutex>
#include <QRect>
#include <QList>
#include <QAtomicInt>
#include <QMap>

class QKxSendInput : public QThread
{
    Q_OBJECT
private:
    struct MyMsg {
        uchar type;
        QByteArray data;
    };
public:
    explicit QKxSendInput(const QRect& capRt, const QRect& deskRt, QObject *parent=nullptr);
    void stop();
    void sendMouseEvent(quint8 button, quint16 x, quint16 y);
    void sendKeyEvent(quint8 down, quint32 key);
protected:
    virtual void handleMouseEvent(quint8 button, quint16 x, quint16 y) = 0;
    virtual void handleKeyEvent(quint8 down, quint32 key) = 0;
private:
    void push(uchar type, const QByteArray& data = QByteArray());
    bool pop(uchar &type, QByteArray& data);
    bool takeOne(uchar type, QByteArray& data);
    void run();
    int running();
    bool init();
protected:
    QRect m_capRect;
    QRect m_deskRect;
    bool m_tryDirect;
private:
    QList<MyMsg> m_queue;
    QMutex m_mtx;
    int m_msgRead;
    int m_msgWrite;    
};

#endif // QKXSENDINPUT_H
