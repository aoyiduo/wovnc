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

#ifndef QKXMACSENDINPUT_H
#define QKXMACSENDINPUT_H

#include "qkxsendinput.h"

class MacSendInputPrivate;
class QKxMacSendInput : public QKxSendInput
{
    Q_OBJECT
public:
    explicit QKxMacSendInput(const QRect& capRt, const QRect& deskRt, QObject *parent=nullptr);
    virtual ~QKxMacSendInput();
protected:
    void handleMouseEvent(quint8 button, quint16 x, quint16 y);
    void handleKeyEvent(quint8 down, quint32 key);
private:
    MacSendInputPrivate *m_prv;
};

#endif // QKXMACSENDINPUT_H
