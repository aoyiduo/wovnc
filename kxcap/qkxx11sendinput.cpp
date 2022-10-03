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

#include "qkxx11sendinput.h"
#include "qkxutils.h"
#include "keysymdef.h"

#include <QDebug>
#include <QVector>
#include <QProcess>
#include <QCoreApplication>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#include <X11/X.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <sys/shm.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XTest.h>

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

class X11InuptLocker {
    Display *m_display;
public:
    X11InuptLocker(Display* d) {
        m_display = d;
        XLockDisplay(m_display);
        XTestGrabControl(m_display, True);
    }
    ~X11InuptLocker() {
        XTestGrabControl(m_display, False);
        XFlush(m_display);
        XUnlockDisplay(m_display);
    }
};

// fix keyboard.
// xmodmap -pke
// xmodmap -e 'keycode 94 = comma less'
// xmodmap -e 'keycode 94 = less greater' orginal.


class X11SendInputPrivate {
public:
    QKxX11SendInput* m_pub = nullptr;
    QRect m_imageRt, m_screenRt;
    Display *m_display;
    Window m_window;
    XModifierKeymap *m_modifierKeymap;
    bool m_useTest;
    quint32 m_mouseButtonFlags;
public:
    explicit X11SendInputPrivate(const QRect& imageRt, const QRect& screenRt, QKxX11SendInput *p)
        : m_pub(p)
        , m_imageRt(imageRt)
        , m_screenRt(screenRt)
    {
        m_display = XOpenDisplay(nullptr);
        m_window = DefaultRootWindow(m_display);
        m_modifierKeymap = XGetModifierMapping(m_display);
        int ignor = 0;
        m_useTest = XTestQueryExtension(m_display, &ignor, &ignor, &ignor, &ignor);
        qDebug() << m_useTest;
        m_mouseButtonFlags = 0;
    }

    virtual ~X11SendInputPrivate() {
        XFreeModifiermap(m_modifierKeymap);
        XCloseDisplay(m_display);
    }

    void handleMouseEvent(quint8 button, quint16 x, quint16 y) {
        bool leftButtonDown = button & 0x1 ? true : false;
        bool midButtonDown = button & 0x2 ? true : false;
        bool rightButtonDown = button & 0x4 ? true : false;
        bool wheelUp = button & 0x8 ? true : false;
        bool wheelDown = button & 0x10 ? true : false;

        if(!m_useTest) {
            return;
        }

        X11InuptLocker locker(m_display);

        if(wheelDown || wheelUp) {
            button = (wheelDown) ? 5 : 4;
            XTestFakeButtonEvent(m_display, button, True, 0);
            XTestFakeButtonEvent(m_display, button, False, 0);
        }else{
            XTestFakeMotionEvent(m_display, -1, x, y, 0);
            //mouse_event
            QString msg;
            quint32 flags = 0;
            if(leftButtonDown) {
                if((m_mouseButtonFlags & MOUSEEVENTF_LEFTDOWN) != MOUSEEVENTF_LEFTDOWN){
                    m_mouseButtonFlags &= ~MOUSEEVENTF_LEFTUP;
                    m_mouseButtonFlags |= MOUSEEVENTF_LEFTDOWN;
                    msg.append("LEFTDOWN");
                    XTestFakeButtonEvent(m_display, 1, true, 0);
                }
            }else if(midButtonDown) {
                if((m_mouseButtonFlags & MOUSEEVENTF_MIDDLEDOWN) != MOUSEEVENTF_MIDDLEDOWN){
                    m_mouseButtonFlags &= ~MOUSEEVENTF_MIDDLEUP;
                    m_mouseButtonFlags |= MOUSEEVENTF_MIDDLEDOWN;
                    msg.append("|MIDDLEDOWN");
                    XTestFakeButtonEvent(m_display, 2, true, 0);
                }
            }else if(rightButtonDown) {
                if((m_mouseButtonFlags & MOUSEEVENTF_RIGHTDOWN) != MOUSEEVENTF_RIGHTDOWN){
                    m_mouseButtonFlags &= ~MOUSEEVENTF_RIGHTUP;
                    m_mouseButtonFlags |= MOUSEEVENTF_RIGHTDOWN;
                    msg.append("|RIGHTDOWN");
                    XTestFakeButtonEvent(m_display, 3, true, 0);
                }
            }else if(m_mouseButtonFlags > 0) {
                if((m_mouseButtonFlags & MOUSEEVENTF_LEFTUP) != MOUSEEVENTF_LEFTUP) {
                    m_mouseButtonFlags &= ~MOUSEEVENTF_LEFTDOWN;
                    m_mouseButtonFlags |= MOUSEEVENTF_LEFTUP;
                    msg.append("LEFTUP");
                    XTestFakeButtonEvent(m_display, 1, false, 0);
                }
                if((m_mouseButtonFlags & MOUSEEVENTF_MIDDLEUP) != MOUSEEVENTF_MIDDLEUP) {
                    m_mouseButtonFlags &= ~MOUSEEVENTF_MIDDLEDOWN;
                    m_mouseButtonFlags |= MOUSEEVENTF_MIDDLEUP;
                    msg.append("|MIDDLEUP");
                    XTestFakeButtonEvent(m_display, 2, false, 0);
                }
                if((m_mouseButtonFlags & MOUSEEVENTF_RIGHTUP) != MOUSEEVENTF_RIGHTUP) {
                    m_mouseButtonFlags &= ~MOUSEEVENTF_RIGHTDOWN;
                    m_mouseButtonFlags |= MOUSEEVENTF_RIGHTUP;
                    msg.append("|RIGHTUP");
                    XTestFakeButtonEvent(m_display, 3, false, 0);
                }
            }

            //qDebug() << "mouse action" << msg;
        }
    }

    void handleKeyEvent(quint8 down, quint32 key) {
        if(!m_useTest) {
            return;
        }
        X11InuptLocker locker(m_display);
        setKeybdEvent(key, down);
    }

    int getKeymask(int keysym) {
        int modifierpos, key, keysymMask = 0;
        KeyCode keycode = XKeysymToKeycode(m_display, keysym);
        qDebug() << "getKeymask" << keycode << keysym;
        if (keycode == NoSymbol){
            return 0;
        }
        for(modifierpos = 0; modifierpos < 8; modifierpos++) {
            int offset = m_modifierKeymap->max_keypermod * modifierpos;
            for (key = 0; key < m_modifierKeymap->max_keypermod; key++) {
                if (m_modifierKeymap->modifiermap[offset + key] == keycode) {
                    keysymMask |= 1 << modifierpos;
                }
            }
        }
        return keysymMask;
    }

    QMap<KeySym, bool> getKeyDownOrToggle(QVector<KeySym> vks) {
        int dummy;
        Window wdummy;
        quint32 state = 0;
        QMap<KeySym, bool> downs;
        if(XQueryPointer(m_display, m_window, &wdummy, &wdummy, &dummy, &dummy, &dummy, &dummy, &state)) {
            for(int i = 0; i < vks.length(); i++) {
                KeySym keysym = vks.at(i);
                if(getKeyState(state, keysym)) {
                    downs.insert(keysym, true);
                }
            }
        }
        return downs;
    }

    bool getKeyState(int state, int keysym) {
        int keysymMask = getKeymask(keysym);
        if (!keysymMask) {
            return false;
        }
        qDebug() << "keysybMask" << keysym << keysymMask;
        return (state & keysymMask) ? true : false;
    }

    bool isNumberLock() {
        QMap<KeySym, bool> downs = getKeyDownOrToggle(QVector<KeySym>() << XK_Num_Lock);
        return !downs.isEmpty();
    }

    bool isCapsLock() {
        QMap<KeySym, bool> downs = getKeyDownOrToggle(QVector<KeySym>() << XK_Caps_Lock);
        return !downs.isEmpty();
    }

    void setKeybdEvent(KeySym key, bool down) {
        KeyCode code = XKeysymToKeycode(m_display, key);
        XTestFakeKeyEvent(m_display, code, down, 0);
    }
};

QKxX11SendInput::QKxX11SendInput(const QRect &capRt, const QRect &deskRt, QObject *parent)
    : QKxSendInput(capRt, deskRt, parent)
{
    m_prv = new X11SendInputPrivate(capRt, deskRt, this);
    changeKeyboard();
}

QKxX11SendInput::~QKxX11SendInput()
{
    delete m_prv;
}

void QKxX11SendInput::handleMouseEvent(quint8 button, quint16 x, quint16 y)
{
    m_prv->handleMouseEvent(button, x, y);
}

void QKxX11SendInput::handleKeyEvent(quint8 down, quint32 key)
{
    m_prv->handleKeyEvent(down, key);
}

void QKxX11SendInput::changeKeyboard()
{
    // setxkbmap -print
    // /usr/share/X11/xkb/symbols/
    // fix keyboard.
    // xmodmap -pke
    // xmodmap -e 'keycode 94 = comma less'
    // xmodmap -e 'keycode 94 = less greater' orginal.
    // xmodmap -e 'keycode 94 ='
    QProcess *proc = new QProcess(this);
    QObject::connect(proc, SIGNAL(finished(int)), this, SLOT(onFindAllLessGreater()));
    QStringList args;
    args << "-c" << "xmodmap -pke|grep 'less'|grep 'greate'";
    proc->start("/bin/bash", args);
}

void QKxX11SendInput::onFindAllLessGreater()
{
    QProcess *proc = qobject_cast<QProcess*>(sender());
    proc->deleteLater();
    QByteArray all = proc->readAll();
    if(all.isEmpty()) {
        return;
    }
    qDebug() << "less greater on same key:" << all;
    all = all.replace("\r\n", "\n");
    QByteArrayList lines = all.split('\n');
    for(int i = 0; i < lines.length(); i++) {
        QByteArray line = lines.at(i);
        int ipos = line.indexOf('=');
        if(ipos > 0) {
            QByteArray key = line.mid(0, ipos+1);
            QStringList args;
            args << "-c" << "xmodmap -e '" + key + "'";
            qDebug() << "delete less greater key." << args;
            QProcess *proc = new QProcess(this);
            QObject::connect(proc, SIGNAL(finished(int)), proc, SLOT(deleteLater()));
            proc->start("/bin/bash", args);
        }
    }
}
