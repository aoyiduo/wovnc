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

#include "qkxmacsendinput.h"
#include "keysymdef.h"

#include <QDebug>
#include <QDateTime>
#include <QTimer>

#include <dispatch/dispatch.h>
#include <IOKit/IOKitLib.h>
#include <IOSurface/IOSurface.h>
#include <CoreVideo/CoreVideo.h>
#include <CoreGraphics/CoreGraphics.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#define MOUSEEVENTF_MOVE        0x0001 /* mouse move */
#define MOUSEEVENTF_LEFTDOWN    0x0002 /* left button down */
#define MOUSEEVENTF_LEFTUP      0x0004 /* left button up */
#define MOUSEEVENTF_RIGHTDOWN   0x0008 /* right button down */
#define MOUSEEVENTF_RIGHTUP     0x0010 /* right button up */
#define MOUSEEVENTF_MIDDLEDOWN  0x0020 /* middle button down */
#define MOUSEEVENTF_MIDDLEUP    0x0040 /* middle button up */
#define MOUSEEVENTF_XDOWN       0x0080 /* x button down */
#define MOUSEEVENTF_XUP         0x0100 /* x button down */
#define MOUSEEVENTF_WHEEL       0x0800 /* wheel button rolled */
#define MOUSEEVENTF_ABSOLUTE    0x8000 /* absolute move */

static int keyTable[] = {
    /* The alphabet */
    VNC_KEY_A,                  0,      /* A */
    VNC_KEY_B,                 11,      /* B */
    VNC_KEY_C,                  8,      /* C */
    VNC_KEY_D,                  2,      /* D */
    VNC_KEY_E,                 14,      /* E */
    VNC_KEY_F,                  3,      /* F */
    VNC_KEY_G,                  5,      /* G */
    VNC_KEY_H,                  4,      /* H */
    VNC_KEY_I,                 34,      /* I */
    VNC_KEY_J,                 38,      /* J */
    VNC_KEY_K,                 40,      /* K */
    VNC_KEY_L,                 37,      /* L */
    VNC_KEY_M,                 46,      /* M */
    VNC_KEY_N,                 45,      /* N */
    VNC_KEY_O,                 31,      /* O */
    VNC_KEY_P,                 35,      /* P */
    VNC_KEY_Q,                 12,      /* Q */
    VNC_KEY_R,                 15,      /* R */
    VNC_KEY_S,                  1,      /* S */
    VNC_KEY_T,                 17,      /* T */
    VNC_KEY_U,                 32,      /* U */
    VNC_KEY_V,                  9,      /* V */
    VNC_KEY_W,                 13,      /* W */
    VNC_KEY_X,                  7,      /* X */
    VNC_KEY_Y,                 16,      /* Y */
    VNC_KEY_Z,                  6,      /* Z */
    VNC_KEY_a,                  0,      /* a */
    VNC_KEY_b,                 11,      /* b */
    VNC_KEY_c,                  8,      /* c */
    VNC_KEY_d,                  2,      /* d */
    VNC_KEY_e,                 14,      /* e */
    VNC_KEY_f,                  3,      /* f */
    VNC_KEY_g,                  5,      /* g */
    VNC_KEY_h,                  4,      /* h */
    VNC_KEY_i,                 34,      /* i */
    VNC_KEY_j,                 38,      /* j */
    VNC_KEY_k,                 40,      /* k */
    VNC_KEY_l,                 37,      /* l */
    VNC_KEY_m,                 46,      /* m */
    VNC_KEY_n,                 45,      /* n */
    VNC_KEY_o,                 31,      /* o */
    VNC_KEY_p,                 35,      /* p */
    VNC_KEY_q,                 12,      /* q */
    VNC_KEY_r,                 15,      /* r */
    VNC_KEY_s,                  1,      /* s */
    VNC_KEY_t,                 17,      /* t */
    VNC_KEY_u,                 32,      /* u */
    VNC_KEY_v,                  9,      /* v */
    VNC_KEY_w,                 13,      /* w */
    VNC_KEY_x,                  7,      /* x */
    VNC_KEY_y,                 16,      /* y */
    VNC_KEY_z,                  6,      /* z */
    /* Numbers */
    VNC_KEY_0,                 29,      /* 0 */
    VNC_KEY_1,                 18,      /* 1 */
    VNC_KEY_2,                 19,      /* 2 */
    VNC_KEY_3,                 20,      /* 3 */
    VNC_KEY_4,                 21,      /* 4 */
    VNC_KEY_5,                 23,      /* 5 */
    VNC_KEY_6,                 22,      /* 6 */
    VNC_KEY_7,                 26,      /* 7 */
    VNC_KEY_8,                 28,      /* 8 */
    VNC_KEY_9,                 25,      /* 9 */
    /* Symbols */
    VNC_KEY_Exclam,            18,      /* ! */
    VNC_KEY_At,                19,      /* @ */
    VNC_KEY_Numbersign,        20,      /* # */
    VNC_KEY_Dollar,            21,      /* $ */
    VNC_KEY_Percent,           23,      /* % */
    VNC_KEY_Asciicircum,       22,      /* ^ */
    VNC_KEY_Ampersand,         26,      /* & */
    VNC_KEY_Asterisk,          28,      /* * */
    VNC_KEY_Parenleft,         25,      /* ( */
    VNC_KEY_Parenright,        29,      /* ) */
    VNC_KEY_Minus,             27,      /* - */
    VNC_KEY_Underscore,        27,      /* _ */
    VNC_KEY_Equal,             24,      /* = */
    VNC_KEY_Plus,              24,      /* + */
    VNC_KEY_Grave,             50,      /* ` */  /* XXX ? */
    VNC_KEY_Asciitilde,        50,      /* ~ */
    VNC_KEY_Bracketleft,       33,      /* [ */
    VNC_KEY_Braceleft,         33,      /* { */
    VNC_KEY_Bracketright,      30,      /* ] */
    VNC_KEY_Braceright,        30,      /* } */
    VNC_KEY_Semicolon,         41,      /* ; */
    VNC_KEY_Colon,             41,      /* : */
    VNC_KEY_Apostrophe,        39,      /* ' */
    VNC_KEY_Quotedbl,          39,      /* " */
    VNC_KEY_Comma,             43,      /* , */
    VNC_KEY_Less,              43,      /* < */
    VNC_KEY_Period,            47,      /* . */
    VNC_KEY_Greater,           47,      /* > */
    VNC_KEY_Slash,             44,      /* / */
    VNC_KEY_Question,          44,      /* ? */
    VNC_KEY_Backslash,         42,      /* \ */
    VNC_KEY_Bar,               42,      /* | */
    /* "Special" keys */
    VNC_KEY_Space,             49,      /* Space */
    VNC_KEY_ReturnOrEnter,     36,      /* Return */
    VNC_KEY_Delete,           117,      /* Delete */
    VNC_KEY_Tab,               48,      /* Tab */
    VNC_KEY_Escape,            53,      /* Esc */
#ifdef HAS_LOCK_KEY
    VNC_KEY_Caps_Lock,         57,      /* Caps Lock */
    VNC_KEY_Num_Lock,          71,      /* Num Lock */
    VNC_KEY_Scroll_Lock,      107,      /* Scroll Lock */
    VNC_KEY_Pause,            113,      /* Pause */
#endif
    VNC_KEY_BackSpace,         51,      /* Backspace */
    VNC_KEY_Insert,           114,      /* Insert */
    /* Cursor movement */
    VNC_KEY_Up,               126,      /* Cursor Up */
    VNC_KEY_Down,             125,      /* Cursor Down */
    VNC_KEY_Left,             123,      /* Cursor Left */
    VNC_KEY_Right,            124,      /* Cursor Right */
    VNC_KEY_Page_Up,          116,      /* Page Up */
    VNC_KEY_Page_Down,        121,      /* Page Down */
    VNC_KEY_Home,             115,      /* Home */
    VNC_KEY_End,              119,      /* End */
#ifdef HAS_NUMBER_KEYPAD
    /* Numeric keypad */
    VNC_KEY_KP_0,              82,      /* KP 0 */
    VNC_KEY_KP_1,              83,      /* KP 1 */
    VNC_KEY_KP_2,              84,      /* KP 2 */
    VNC_KEY_KP_3,              85,      /* KP 3 */
    VNC_KEY_KP_4,              86,      /* KP 4 */
    VNC_KEY_KP_5,              87,      /* KP 5 */
    VNC_KEY_KP_6,              88,      /* KP 6 */
    VNC_KEY_KP_7,              89,      /* KP 7 */
    VNC_KEY_KP_8,              91,      /* KP 8 */
    VNC_KEY_KP_9,              92,      /* KP 9 */
    VNC_KEY_KP_Enter,          76,      /* KP Enter */
    VNC_KEY_KP_Decimal,        65,      /* KP . */
    VNC_KEY_KP_Add,            69,      /* KP + */
    VNC_KEY_KP_Subtract,       78,      /* KP - */
    VNC_KEY_KP_Multiply,       67,      /* KP * */
    VNC_KEY_KP_Divide,         75,      /* KP / */
#endif
    /* Function keys */
    VNC_KEY_F1,               122,      /* F1 */
    VNC_KEY_F2,               120,      /* F2 */
    VNC_KEY_F3,                99,      /* F3 */
    VNC_KEY_F4,               118,      /* F4 */
    VNC_KEY_F5,                96,      /* F5 */
    VNC_KEY_F6,                97,      /* F6 */
    VNC_KEY_F7,                98,      /* F7 */
    VNC_KEY_F8,               100,      /* F8 */
    VNC_KEY_F9,               101,      /* F9 */
    VNC_KEY_F10,              109,      /* F10 */
    VNC_KEY_F11,              103,      /* F11 */
    VNC_KEY_F12,              111,      /* F12 */
    /* Modifier keys */
    VNC_KEY_Shift_Left,           56,      /* Shift Left */
    VNC_KEY_Shift_Right,           56,      /* Shift Right */
    VNC_KEY_Control_Left,         59,      /* Ctrl Left */
    VNC_KEY_Control_Right,         59,      /* Ctrl Right */
    VNC_KEY_Meta_Left,            58,      /* Logo Left (-> Option) */
    VNC_KEY_Meta_Right,            58,      /* Logo Right (-> Option) */
    VNC_KEY_Alt_Left,             55,      /* Alt Left (-> Command) */
    VNC_KEY_Alt_Right,             55,      /* Alt Right (-> Command) */
};

class MacSendInputPrivate {
    QKxMacSendInput *m_pub = nullptr;
    quint32 m_mouseButtonFlags = 0;
    qint16 m_xLast, m_yLast;
    QMap<int, int> m_xkeyToKeyCode;

    struct LeftClickInfo {
        qint16 x, y;
        qint64 tm;

        LeftClickInfo() {
            x = y = 0;
            tm = 0;
        }

        LeftClickInfo(qint16 x, qint16 y) {
            this->x = x;
            this->y = y;
            tm = QDateTime::currentMSecsSinceEpoch();
        }
    };
    QVector<LeftClickInfo> m_lci;
public:
    explicit MacSendInputPrivate(QKxMacSendInput *p)
        : m_pub(p) {
        for(int i = 0; i < sizeof(keyTable) / sizeof(keyTable[0]); i+=2) {
            m_xkeyToKeyCode.insert(keyTable[i], keyTable[i+1]);
        }
    }

    virtual ~MacSendInputPrivate() {

    }

    void pushLeftClick(qint16 x, qint16 y) {
        LeftClickInfo lci(x, y);
        if(m_lci.isEmpty()) {
            m_lci.push_back(lci);
        }else{
            LeftClickInfo last = m_lci.last();
            if(qAbs(last.x - x) < 2
                    && qAbs(last.y - y) < 2
                    && (lci.tm - last.tm) < 500) {
                m_lci.append(lci);
            }else{
                m_lci.clear();
                m_lci.append(lci);
            }
        }
    }

    void clearLeftClick() {
        m_lci.clear();
    }

    void handleMouseEvent(quint8 button, quint16 x, quint16 y) {
        bool leftButtonDown = button & 0x1 ? true : false;
        bool midButtonDown = button & 0x2 ? true : false;
        bool rightButtonDown = button & 0x4 ? true : false;
        bool wheelUp = button & 0x8 ? true : false;
        bool wheelDown = button & 0x10 ? true : false;

        if(wheelDown || wheelUp) {
            CGWheelCount wheelCount = 2;
            quint32 delta = 0;
            if(wheelUp) {
                delta = 1;
            }else{
                delta = -1;
            }
            /* must be range -10 to +10 */
            setWheelEvent(delta);
        } else {
            QString msg;
            if(x != m_xLast || y != m_yLast) {
                m_xLast = x;
                m_yLast = y;
                if(leftButtonDown) {
                    setMouseEvent(kCGEventLeftMouseDragged, kCGMouseButtonLeft, x, y);
                }else if(rightButtonDown) {
                    setMouseEvent(kCGEventRightMouseDragged, kCGMouseButtonRight, x, y);
                }else if(midButtonDown){
                    setMouseEvent(kCGEventOtherMouseDragged, kCGMouseButtonCenter, x, y);
                }else{
                    setMouseEvent(kCGEventMouseMoved, kCGMouseButtonLeft, x, y);
                }
            }

            if(leftButtonDown) {
                if((m_mouseButtonFlags & MOUSEEVENTF_LEFTDOWN) != MOUSEEVENTF_LEFTDOWN){
                    m_mouseButtonFlags &= ~MOUSEEVENTF_LEFTUP;
                    m_mouseButtonFlags |= MOUSEEVENTF_LEFTDOWN;
                    msg.append("LEFTDOWN");
                    pushLeftClick(x, y);
                    //XTestFakeButtonEvent(m_display, 1, true, 0);
                    setMouseEvent(kCGEventLeftMouseDown, kCGMouseButtonLeft, x, y);
                }
            }else if(midButtonDown) {
                clearLeftClick();
                if((m_mouseButtonFlags & MOUSEEVENTF_MIDDLEDOWN) != MOUSEEVENTF_MIDDLEDOWN){
                    m_mouseButtonFlags &= ~MOUSEEVENTF_MIDDLEUP;
                    m_mouseButtonFlags |= MOUSEEVENTF_MIDDLEDOWN;
                    msg.append("|MIDDLEDOWN");
                    //XTestFakeButtonEvent(m_display, 2, true, 0);
                    setMouseEvent(kCGEventOtherMouseDown, kCGMouseButtonCenter, x, y);
                }
            }else if(rightButtonDown) {
                clearLeftClick();
                if((m_mouseButtonFlags & MOUSEEVENTF_RIGHTDOWN) != MOUSEEVENTF_RIGHTDOWN){
                    m_mouseButtonFlags &= ~MOUSEEVENTF_RIGHTUP;
                    m_mouseButtonFlags |= MOUSEEVENTF_RIGHTDOWN;
                    msg.append("|RIGHTDOWN");
                    //XTestFakeButtonEvent(m_display, 3, true, 0);
                    setMouseEvent(kCGEventRightMouseDown, kCGMouseButtonRight, x, y);
                }
            }else if(m_mouseButtonFlags > 0) {
                if((m_mouseButtonFlags & MOUSEEVENTF_LEFTUP) != MOUSEEVENTF_LEFTUP) {
                    m_mouseButtonFlags &= ~MOUSEEVENTF_LEFTDOWN;
                    m_mouseButtonFlags |= MOUSEEVENTF_LEFTUP;
                    msg.append("LEFTUP");
                    //XTestFakeButtonEvent(m_display, 1, false, 0);
                    setMouseEvent(kCGEventLeftMouseUp, kCGMouseButtonLeft, x, y);
                }

                if((m_mouseButtonFlags & MOUSEEVENTF_MIDDLEUP) != MOUSEEVENTF_MIDDLEUP) {
                    m_mouseButtonFlags &= ~MOUSEEVENTF_MIDDLEDOWN;
                    m_mouseButtonFlags |= MOUSEEVENTF_MIDDLEUP;
                    msg.append("|MIDDLEUP");
                    //XTestFakeButtonEvent(m_display, 2, false, 0);
                    setMouseEvent(kCGEventOtherMouseUp, kCGMouseButtonCenter, x, y);
                }

                if((m_mouseButtonFlags & MOUSEEVENTF_RIGHTUP) != MOUSEEVENTF_RIGHTUP) {
                    m_mouseButtonFlags &= ~MOUSEEVENTF_RIGHTDOWN;
                    m_mouseButtonFlags |= MOUSEEVENTF_RIGHTUP;
                    msg.append("|RIGHTUP");
                    //XTestFakeButtonEvent(m_display, 3, false, 0);
                    setMouseEvent(kCGEventRightMouseUp, kCGMouseButtonRight, x, y);
                }
            }

            //qDebug() << "mouse Action" << msg;
        }
    }

    void handleKeyEvent(quint8 down, quint32 key) {
        int keycode = m_xkeyToKeyCode.value(key, key);
        //qDebug() << down << keycode << key;
#if 0
        CGPostKeyboardEvent((CGCharCode)0, keycode, down);
#else
        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
        CGEventRef evt = CGEventCreateKeyboardEvent(source, (CGKeyCode)keycode, down);
        CGEventPost(kCGHIDEventTap,evt);
        CFRelease(evt);
        CFRelease(source);
#endif
    }

    /**
     * https://android.googlesource.com/platform/external/libvncserver/+/refs/heads/marshmallow-dr-dev/examples/mac.c
     * https://stackoverflow.com/questions/1483657/performing-a-double-click-using-cgeventcreatemouseevent
     *
     */
    void setWheelEvent(qint16 delta) {
        //qDebug() << "setWheelEvent" << delta;
        CGWheelCount wheelCount = 1;
        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
        CGEventRef scroll = CGEventCreateScrollWheelEvent(source, kCGScrollEventUnitLine, wheelCount, delta);
        CGEventPost(kCGHIDEventTap, scroll);
        CFRelease(scroll);
        CFRelease(source);
    }

    void setMouseEvent(CGEventType type, CGMouseButton button,  quint16 x, quint16 y) {
        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
        CGEventRef mouseEvent = CGEventCreateMouseEvent(source, type, CGPointMake(x, y), button);
        if(type == kCGEventLeftMouseDown) {
            int cnt = m_lci.count();
            CGEventSetIntegerValueField(mouseEvent, kCGMouseEventClickState, cnt);
            qDebug() << "LeftButtonDown count:" << cnt;
        }
        CGEventPost(kCGHIDEventTap, mouseEvent);
        CFRelease(mouseEvent);
        CFRelease(source);
    }
};

QKxMacSendInput::QKxMacSendInput(const QRect &capRt, const QRect &deskRt, QObject *parent)
    : QKxSendInput(capRt, deskRt, parent)
{
    m_prv = new MacSendInputPrivate(this);
}

QKxMacSendInput::~QKxMacSendInput()
{
    delete m_prv;
}

void QKxMacSendInput::handleMouseEvent(quint8 button, quint16 x, quint16 y)
{
    m_prv->handleMouseEvent(button, x, y);
}

void QKxMacSendInput::handleKeyEvent(quint8 down, quint32 key)
{
    m_prv->handleKeyEvent(down, key);
}
