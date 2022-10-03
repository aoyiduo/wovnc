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

#include "qkxwinsendinput.h"
#include "keysymdef.h"
#include "qkxutils.h"

#include <Windows.h>

QKxWinSendInput::QKxWinSendInput(const QRect& capRt, const QRect& deskRt, QObject *parent)
    : QKxSendInput(capRt, deskRt, parent)
    , m_mouseButtonFlags(0)
{

}

void QKxWinSendInput::handleMouseEvent(quint8 button, quint16 x, quint16 y)
{
    bool leftButtonDown = button & 0x1 ? true : false;
    bool midButtonDown = button & 0x2 ? true : false;
    bool rightButtonDown = button & 0x4 ? true : false;
    bool wheelUp = button & 0x8 ? true : false;
    bool wheelDown = button & 0x10 ? true : false;

    //mouse_event
    QString msg;
    quint32 flags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK;
    if(leftButtonDown) {
        if((m_mouseButtonFlags & MOUSEEVENTF_LEFTDOWN) != MOUSEEVENTF_LEFTDOWN){
            flags |= MOUSEEVENTF_LEFTDOWN;
            m_mouseButtonFlags &= ~MOUSEEVENTF_LEFTUP;
            m_mouseButtonFlags |= MOUSEEVENTF_LEFTDOWN;
            msg.append("LEFTDOWN");
        }
    }else if(midButtonDown) {
        if((m_mouseButtonFlags & MOUSEEVENTF_MIDDLEDOWN) != MOUSEEVENTF_MIDDLEDOWN){
            flags |= MOUSEEVENTF_MIDDLEDOWN;
            m_mouseButtonFlags &= ~MOUSEEVENTF_MIDDLEUP;
            m_mouseButtonFlags |= MOUSEEVENTF_MIDDLEDOWN;
            msg.append("|MIDDLEDOWN");
        }
    }else if(rightButtonDown) {
        if((m_mouseButtonFlags & MOUSEEVENTF_RIGHTDOWN) != MOUSEEVENTF_RIGHTDOWN){
            flags |= MOUSEEVENTF_RIGHTDOWN;
            m_mouseButtonFlags &= ~MOUSEEVENTF_RIGHTUP;
            m_mouseButtonFlags |= MOUSEEVENTF_RIGHTDOWN;
            msg.append("|RIGHTDOWN");
        }
    }else if(m_mouseButtonFlags > 0) {
        if((m_mouseButtonFlags & MOUSEEVENTF_LEFTUP) != MOUSEEVENTF_LEFTUP) {
            flags |= MOUSEEVENTF_LEFTUP;
            m_mouseButtonFlags &= ~MOUSEEVENTF_LEFTDOWN;
            m_mouseButtonFlags |= MOUSEEVENTF_LEFTUP;
            msg.append("LEFTUP");
        }

        if((m_mouseButtonFlags & MOUSEEVENTF_MIDDLEUP) != MOUSEEVENTF_MIDDLEUP) {
            flags |= MOUSEEVENTF_MIDDLEUP;
            m_mouseButtonFlags &= ~MOUSEEVENTF_MIDDLEDOWN;
            m_mouseButtonFlags |= MOUSEEVENTF_MIDDLEUP;
            msg.append("|MIDDLEUP");
        }

        if((m_mouseButtonFlags & MOUSEEVENTF_RIGHTUP) != MOUSEEVENTF_RIGHTUP) {
            flags |= MOUSEEVENTF_RIGHTUP;
            m_mouseButtonFlags &= ~MOUSEEVENTF_RIGHTDOWN;
            m_mouseButtonFlags |= MOUSEEVENTF_RIGHTUP;
            msg.append("|RIGHTUP");
        }
    }

    int delta = 0;
    if(wheelUp || wheelDown) {
        delta = wheelUp ? WHEEL_DELTA : -WHEEL_DELTA;
        flags |= MOUSEEVENTF_WHEEL;
    }else{
        flags &= ~MOUSEEVENTF_WHEEL;
    }

    setMouseEvent(flags, delta, x, y);
}

void QKxWinSendInput::handleKeyEvent(quint8 down, quint32 key)
{
    if(key >= VNC_KEY_F1 && key <= VNC_KEY_F12) {
        // F1 ~ F12
        quint16 vkey = key - VNC_KEY_F1 + VK_F1;
        setKeybdEvent(vkey, down);
    }else if(key >= VNC_KEY_Space && key <= VNC_KEY_Del) {
        quint16 vkey = ::VkKeyScan(key);
        //QFlags<Qt::KeyboardModifier> hints = shortCutHint(key);
        if(down) {
            if(vkey > 0xFF) {
                // shift + key, such as A ~ Z, ~+..
                if(key >= VNC_KEY_A && key <= VNC_KEY_Z){
                    if(isKeyPressed(VK_LCONTROL)
                            || isKeyPressed(VK_RCONTROL)
                            || isKeyPressed(VK_LMENU)
                            || isKeyPressed(VK_RMENU)) {
                        setKeybdEvent(vkey, true);
                    }else if(isKeyPressed(VK_LSHIFT) || isKeyPressed(VK_RSHIFT)) {
                        if(isKeyToggled(VK_CAPITAL)) {
                            // guess the real situation and fix it.
                            setKeybdEvent(VK_CAPITAL, true);
                            setKeybdEvent(VK_CAPITAL, false);
                        }
                        setKeybdEvent(vkey, true);
                    }else if(isKeyToggled(VK_CAPITAL)) {
                        setKeybdEvent(vkey, true);
                    }else{
                        setKeybdEvent(VK_CAPITAL, true);
                        setKeybdEvent(VK_CAPITAL, false);
                        setKeybdEvent(vkey, true);
                    }
                }else {
                    /* 双字键的上排字符，不可能伴随Ctrl/Alt功能键，但有shift健。 */
                    if(isKeyPressed(VK_LCONTROL)) {
                        setKeybdEvent(VK_LCONTROL, false);
                    }
                    if(isKeyPressed(VK_RCONTROL)) {
                        setKeybdEvent(VK_RCONTROL, false);
                    }
                    if(isKeyPressed(VK_LMENU)) {
                        setKeybdEvent(VK_LMENU, false);
                    }
                    if(isKeyPressed(VK_RMENU)) {
                        setKeybdEvent(VK_RMENU, false);
                    }
                    setKeybdEvent(vkey, true);
                }
            }else{
                /* 小写字母、双字键下排字符*/
                if(key >= VNC_KEY_a && key <= VNC_KEY_z) {
                    /* 小写字母不可能有快捷键 */
                    if(isKeyPressed(VK_LCONTROL)) {
                        setKeybdEvent(VK_LCONTROL, false);
                    }
                    if(isKeyPressed(VK_RCONTROL)) {
                        setKeybdEvent(VK_RCONTROL, false);
                    }
                    if(isKeyPressed(VK_LSHIFT)) {
                        setKeybdEvent(VK_LSHIFT, false);
                    }
                    if(isKeyPressed(VK_RSHIFT)) {
                        setKeybdEvent(VK_RSHIFT, false);
                    }
                    if(isKeyPressed(VK_LMENU)) {
                        setKeybdEvent(VK_LMENU, false);
                    }
                    if(isKeyPressed(VK_RMENU)) {
                        setKeybdEvent(VK_RMENU, false);
                    }
                    if(isKeyToggled(VK_CAPITAL)){
                        setKeybdEvent(VK_CAPITAL, true);
                        setKeybdEvent(VK_CAPITAL, false);
                    }
                    setKeybdEvent(vkey, true);
                }else {
                    /* 双字键的下排数字或符号有快捷键 */
                    if(isKeyPressed(VK_LCONTROL)
                            || isKeyPressed(VK_RCONTROL)
                            || isKeyPressed(VK_LMENU)
                            || isKeyPressed(VK_RMENU)) {
                        // has shotcut.
                        setKeybdEvent(vkey, true);
                    }else {
                        /* shift键在下排键时，不可能单独存在。*/
                        /* Capital键只能影响字母。 */
                        if(isKeyPressed(VK_LSHIFT)) {
                            setKeybdEvent(VK_LSHIFT, false);
                        }
                        if(isKeyPressed(VK_RSHIFT)) {
                            setKeybdEvent(VK_RSHIFT, false);
                        }
                        setKeybdEvent(vkey, true);
                    }
                }
            }
        }else{
            setKeybdEvent(vkey, false);
        }
    } else if(key == VNC_KEY_BackSpace) {
        quint16 vkey = VK_BACK;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Tab) {
        quint16 vkey = VK_TAB;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_ReturnOrEnter) {
        quint16 vkey = VK_RETURN;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Escape) {
        quint16 vkey = VK_ESCAPE;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Insert) {
        quint16 vkey = VK_INSERT;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Delete) {
        quint16 vkey = VK_DELETE;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Home) {
        quint16 vkey = VK_HOME;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_End) {
        quint16 vkey = VK_END;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Page_Up) {
        quint16 vkey = VK_PRIOR;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Page_Down) {
        quint16 vkey = VK_NEXT;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Left) {
        quint16 vkey = VK_LEFT;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Up) {
        quint16 vkey = VK_UP;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Right) {
        quint16 vkey = VK_RIGHT;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Down) {
        quint16 vkey = VK_DOWN;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Shift_Left) {
        quint16 vkey = VK_LSHIFT;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Shift_Right) {
        quint16 vkey = VK_RSHIFT;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Control_Left) {
        quint16 vkey = VK_LCONTROL;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Control_Right) {
        quint16 vkey = VK_RCONTROL;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Meta_Left) {
        quint16 vkey = VK_LWIN;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Meta_Right) {
        quint16 vkey = VK_RWIN;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Alt_Left) {
        quint16 vkey = VK_LMENU;
        setKeybdEvent(vkey, down);
    }else if(key == VNC_KEY_Alt_Right) {
        quint16 vkey = VK_RMENU;
        setKeybdEvent(vkey, down);
    }else{
        setKeybdEvent(key, down);
    }
}

void QKxWinSendInput::setMouseEvent(quint32 flags, qint32 delta, int x, int y)
{
    /* If MOUSEEVENTF_ABSOLUTE value is specified, dx and dy contain normalized absolute coordinates between 0 and 65,535.
     * The event procedure maps these coordinates onto the display surface.
     * Coordinate (0,0) maps onto the upper-left corner of the display surface, (65535,65535) maps onto the lower-right corner.
     */
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flags;
    input.mi.mouseData = delta;
    input.mi.dx = (x + m_capRect.left()) * 65535 / m_deskRect.width();
    input.mi.dy = (y + m_capRect.top()) * 65535 / m_deskRect.height();

    int cnt = ::SendInput(1, &input, sizeof(INPUT));
    if(cnt != 1) {
        if(QKxUtils::isDesktopChanged()) {
            m_tryDirect = QKxUtils::resetThreadDesktop();
        }else{
            m_tryDirect = false;
        }
        //qDebug() << "setMouseEvent" << cnt << flags << delta << x << y;
    }
}

void QKxWinSendInput::setKeybdEvent(quint16 vkey, bool down)
{
    //keybd_event
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY|KEYEVENTF_UNICODE;
    input.ki.wVk = vkey;

    if(down == 0) {
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
    }
    int cnt = ::SendInput(1, &input, sizeof(INPUT));
    if(cnt != 1) {
        m_tryDirect = false;
        //qDebug() << "setKeybdEvent" << vkey << down << cnt;
    }
}

bool QKxWinSendInput::isKeyToggled(quint16 vkey)
{
    return (GetKeyState(vkey) & 0x01) != 0;
}

bool QKxWinSendInput::isKeyPressed(quint16 vkey)
{
    return (GetKeyState(vkey) & 0x8000) != 0;
}

QFlags<Qt::KeyboardModifier> QKxWinSendInput::shortCutHint(quint8 ch)
{
    QFlags<Qt::KeyboardModifier> flags;
    if(ch >= VNC_KEY_0 && ch <= VNC_KEY_9) {
        flags |= Qt::ControlModifier;
        flags |= Qt::AltModifier;
        return flags;
    }
    if(ch >= VNC_KEY_A && ch <= VNC_KEY_Z) {
        flags |= Qt::ControlModifier;
        flags |= Qt::AltModifier;
        flags |= Qt::ShiftModifier;
        return flags;
    }
    static QMap<quint8, QFlags<Qt::KeyboardModifier> > cuts;
    if(cuts.isEmpty()) {
        char syms[] = " -=[]\\;',./";
        flags |= Qt::ControlModifier;
        flags |= Qt::AltModifier;
        flags |= Qt::ShiftModifier;
        for(int i = 0; i < sizeof(syms) / sizeof(char); i++) {
            cuts.insert(syms[i], flags);
        }
    }
    return cuts.value(ch, 0);
}
