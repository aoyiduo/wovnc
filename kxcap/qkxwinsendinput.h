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

#ifndef QKXWINSENDINPUT_H
#define QKXWINSENDINPUT_H

#include "qkxsendinput.h"

class QKxWinSendInput : public QKxSendInput
{
    Q_OBJECT
public:
    explicit QKxWinSendInput(const QRect& capRt, const QRect& deskRt, QObject *parent=nullptr);
protected:
    void handleMouseEvent(quint8 button, quint16 x, quint16 y);
    void handleKeyEvent(quint8 down, quint32 key);
private:
    void setMouseEvent(quint32 flags, qint32 delta, int x, int y);
    void setKeybdEvent(quint16 vkey, bool down);
    bool isKeyToggled(quint16 vkey);
    bool isKeyPressed(quint16 vkey);
    QFlags<Qt::KeyboardModifier> shortCutHint(quint8 ch);
private:
    quint32 m_mouseButtonFlags;
};
#endif // QKXWINSENDINPUT_H
