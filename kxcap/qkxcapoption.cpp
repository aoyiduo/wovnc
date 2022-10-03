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

#include "qkxcapoption.h"

#include "qkxsharememory.h"

#include <QPointer>
#include <QCoreApplication>
#include <QCryptographicHash>

#ifdef Q_OS_WIN
#include <Windows.h>
#else
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#endif

Q_GLOBAL_STATIC(QKxCapOption, gcapOpts)

class QKxCapOptionPrivate {
    QKxCapOption *m_p;
    QPointer<QKxShareMemory> m_shared;
public:
    QKxCapOptionPrivate(QKxCapOption *p)
        : m_p(p) {
        QString appName = QCoreApplication::applicationName();
#ifdef Q_OS_WIN
        QString name = QString("capOption");
#else
        QString name = QString("capOption:%1").arg(::getuid());
#endif
        name = appName + "-" + name;
        m_shared = new QKxShareMemory(name, 2048, m_p);
    }

    ~QKxCapOptionPrivate() {
        if(m_shared){
            delete m_shared;
        }
    }

    int screenIndex()
    {
        return m_shared->value("screenIndex", -1).toInt();
    }

    void setScreenIndex(int idx)
    {
        m_shared->setValue("screenIndex", idx);
    }

    QString authorizePassword()
    {
        return m_shared->value("authorizePassword", "123456789").toString();
    }

    void setAuthorizePassword(const QString &pwd)
    {
        m_shared->setValue("authorizePassword", pwd);
    }

    int maxFPS() const
    {
        return m_shared->value("maxFPS", 30).toInt();
    }

    void setMaxFPS(int fps)
    {
        m_shared->setValue("maxFPS", fps);
    }

    bool enableEmptyFrame() {
        return m_shared->value("emptyFrame", false).toBool();
    }

    void setEmptyFrameEnable(bool ok)
    {
        m_shared->setValue("emptyFrame", ok);
    }

    void setServerUrl(const QString& url) {
        m_shared->setValue("serverUrl", url);
    }

    QString serverUrl() const {
        // don't change it, net module will use it.
        return m_shared->value("serverUrl", "").toString();
    }

    QString serverUrlNodePath() const {
        return m_shared->name() + "/" + "serverUrl";
    }

    QMap<QString, QVariant> all() {
        return m_shared->all();
    }

    bool autoLockScreen() const {
        return m_shared->value("autoLockScreen", true).toBool();
    }

    void setAutoLockScreen(bool on) {
        m_shared->setValue("autoLockScreen", on);
    }

    bool dragWithContent() const {
        return m_shared->value("dragWithContent", false).toBool();
    }

    void setDragWithContent(bool on) {
        m_shared->setValue("dragWithContent", on);
    }

    bool blackWallpaper() {
        return m_shared->value("blackWallpaper", true).toBool();
    }

    void setBlackWallpaper(bool on) {
        m_shared->setValue("blackWallpaper", on);
    }
};

QKxCapOption::QKxCapOption(QObject *parent)
    : QObject(parent)
{
    m_prv = new QKxCapOptionPrivate(this);
}

QKxCapOption::~QKxCapOption()
{
    delete m_prv;
}

QKxCapOption *QKxCapOption::instance()
{
    return gcapOpts;
}

int QKxCapOption::screenIndex() const
{
    return m_prv->screenIndex();
}

void QKxCapOption::setScreenIndex(int idx)
{
    m_prv->setScreenIndex(idx);
}

QString QKxCapOption::authorizePassword() const
{
    return m_prv->authorizePassword();
}

void QKxCapOption::setAuthorizePassword(const QString &pwd)
{
    m_prv->setAuthorizePassword(pwd);
}

int QKxCapOption::maxFPS() const
{
    return m_prv->maxFPS();
}

void QKxCapOption::setMaxFPS(int fps)
{
    m_prv->setMaxFPS(fps);
}

bool QKxCapOption::enableEmptyFrame()
{
    return m_prv->enableEmptyFrame();
}

void QKxCapOption::setEmptyFrameEnable(bool ok)
{
    m_prv->setEmptyFrameEnable(ok);
}

void QKxCapOption::setServerUrl(const QString &url)
{
    m_prv->setServerUrl(url);
}

QString QKxCapOption::serverUrl() const
{
    return m_prv->serverUrl();
}

QString QKxCapOption::serverUrlNodePath() const
{
    return m_prv->serverUrlNodePath();
}

QMap<QString, QVariant> QKxCapOption::all() const
{
    return m_prv->all();
}

bool QKxCapOption::autoLockScreen() const
{
    return m_prv->autoLockScreen();
}

void QKxCapOption::setAutoLockScreen(bool on)
{
    m_prv->setAutoLockScreen(on);
}

bool QKxCapOption::dragWithContent() const
{
    return m_prv->dragWithContent();
}

void QKxCapOption::setDragWithContent(bool on)
{
    m_prv->setDragWithContent(on);
}

bool QKxCapOption::blackWallpaper()
{
    return m_prv->blackWallpaper();
}

void QKxCapOption::setBlackWallpaper(bool on)
{
    m_prv->setBlackWallpaper(on);
}
