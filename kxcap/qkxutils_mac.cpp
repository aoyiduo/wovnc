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

#include "qkxutils.h"

#include <QHostInfo>
#include <QSysInfo>
#include <QCoreApplication>
#include <QDir>
#include <QCryptographicHash>
#include <QLibraryInfo>
#include <QThread>
#include <QIODevice>
#include <QRect>
#include <QRegion>
#include <QProcess>

#include "macnap.h"
#include "objcapi.h"
#include <CoreFoundation/CoreFoundation.h>
#include <mach-o/dyld.h>
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1050
# include <objc/runtime.h>
# include <objc/message.h>
#else
# include <objc/objc-runtime.h>
#endif


#include <dispatch/dispatch.h>
#include <IOKit/IOKitLib.h>
#include <IOSurface/IOSurface.h>
#include <CoreVideo/CoreVideo.h>
#include <CoreGraphics/CoreGraphics.h>

bool QKxUtils::isRunAsAdmin()
{
    return true;
}

bool QKxUtils::shellExecuteAsAdmin(const QString& path, const QStringList& argv, bool bWait)
{
    QProcess::startDetached(path, argv);
    return true;
}

bool QKxUtils::isRunAsService()
{
    QByteArray v = qgetenv("QTSERVICE_RUN");
    return !v.isEmpty();
}

bool QKxUtils::selectInputDesktop()
{
    return true;
}

bool QKxUtils::isDesktopChanged()
{
    return false;
}

bool QKxUtils::resetThreadDesktop()
{
    return false;
}

bool QKxUtils::disableAppNap()
{
    static LatencyCriticalLock lock{LatencyCriticalLock::NoLock()};
    lock.lock();
    return lock.isLocked();
}

static QString toQString(CFStringRef str)
{
    if (!str)
        return QString();

    CFIndex length = CFStringGetLength(str);
    if (length == 0)
        return QString();

    QString string(length, Qt::Uninitialized);
    CFStringGetCharacters(str, CFRangeMake(0, length), reinterpret_cast<UniChar *>
        (const_cast<QChar *>(string.unicode())));
    return string;
}

QString QKxUtils::applicationFilePath()
{
    static QString path;
    if(!path.isEmpty()) {
        return path;
    }

    CFURLRef bundleURL(CFBundleCopyExecutableURL(CFBundleGetMainBundle()));
    if(bundleURL) {
        CFStringRef cfPath(CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle));
        if(cfPath){
            path = QString::fromCFString(cfPath);
        }
    }

    if(path.isEmpty()) {
        char tmp[1024] = {0};
        int cnt = 1024;
        if(_NSGetExecutablePath(tmp, (uint32_t*)&cnt) == 0) {
            path = QString::fromLatin1(tmp);
        }
    }
    if(path.isEmpty()) {
        path = QCoreApplication::applicationFilePath();
    }
    return path;
}

QString QKxUtils::applicationDirPath()
{
    static QString path;
    if(path.isEmpty()) {
        QString tmp = applicationFilePath();
        QStringList dps = tmp.split('/');
        dps.pop_back();
        path = dps.join('/');
    }
    return path;
}

QString QKxUtils::applicationName()
{
    static QString name;
    if(name.isEmpty()) {
        QString tmp = applicationFilePath();
        QStringList dps = tmp.split('/');
        name = dps.last();
    }
    return name;
}



extern id NSApp;
constexpr int NSApplicationActivationPolicyRegular = 0;
constexpr int NSApplicationActivationPolicyAccessory = 1;
constexpr int NSApplicationActivationPolicyProhibited = 2;
void QKxUtils::setVisibleOnDock(bool yes)
{
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    OSStatus err = TransformProcessType(&psn, yes ? kProcessTransformToForegroundApplication : kProcessTransformToBackgroundApplication);
    qDebug() << "setVisibleOnDock" << (int)err << yes;
}

void QKxUtils::lockWorkStation()
{

}

void QKxUtils::setDragWindow(bool on)
{

}
