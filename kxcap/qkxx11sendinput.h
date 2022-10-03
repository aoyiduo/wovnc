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

#ifndef QKXUNIXSENDINPUT_H
#define QKXUNIXSENDINPUT_H

#include "qkxsendinput.h"

class X11SendInputPrivate;
class QKxX11SendInput : public QKxSendInput
{
    Q_OBJECT
public:
    explicit QKxX11SendInput(const QRect& capRt, const QRect& deskRt, QObject *parent=nullptr);
    virtual ~QKxX11SendInput();
protected:
    void handleMouseEvent(quint8 button, quint16 x, quint16 y);
    void handleKeyEvent(quint8 down, quint32 key);
    void changeKeyboard();
private slots:
    void onFindAllLessGreater();
private:
    X11SendInputPrivate *m_prv;
};

#endif // QKXUNIXSENDINPUT_H
