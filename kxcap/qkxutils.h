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

#ifndef QKXUTILS_H
#define QKXUTILS_H

#include <QVariant>
#include <QString>


class QIODevice;
class QKxUtils
{
public:
    static QByteArray randomPassword(int cnt=8);
    static QByteArray randomData(int cnt=8);
    static QByteArray deviceName();
    //0: window, 1: linux, 2: macosx
    static int osType();
    static void launchProcess(const QString& path, bool isGui = false);
    static quint32 getCurrentSessionId();
    static bool isDebugBuild();
    static bool isRunAsAdmin();
    static bool isRunAsLocalSystem();
    static bool isRunAsLocalService();
    static bool isRunAsService();
    static bool createProcessWithAdmin(const QString& path, const QStringList& env, void* process);
    static bool createProcess(const QString& path, const QStringList& env, void* process);
    static bool installFireWall();
    static quint64 GetCurrentSessionId();
    static bool shellExecuteAsAdmin(const QString& path, const QStringList& argv, bool bWait);
    static bool selectInputDesktop();
    static bool inputDesktopSelected(QString& name);
    static void setCurrentProcessRealtime();
    static bool isDesktopChanged();
    static bool resetThreadDesktop();
    /*socket*/
    static bool createPair(int fd[]);
    static bool isAgain(int err);
    static void setSocketNonBlock(int sock, bool on);
    static void setSocketNoDelay(int sock, bool on);
    static int socketError();
    static int xRecv(int sock, char *buf, int len, int flag=0);
    static int xSend(int sock, char *buf, int len, int flag=0);
    static bool waitForRead(QIODevice *dev, qint64 cnt);


    /* differ area. */
    static bool findDirtyRect(uchar *data1, int step1, int width, int height, uchar *data2, int step2, QRect *prt);

    /* disable App Nap*/
    static bool disableAppNap();
    static QString applicationFilePath();
    static QString applicationDirPath();
    static QString applicationName();
    static void setVisibleOnDock(bool yes);

    /*window*/
    static void setBlankScreen(bool set);
    static void lockWorkStation();
    static void setDragWindow(bool on);

    //color space
    static void rgbToYuv(qint32 rgb, quint8 *y, quint8 *u, quint8 *v);
    static void rgbToYuv2(qint32 rgb, quint8 *y, quint8 *u, quint8 *v);
    static void rgbToYuv3(qint32 rgb, quint8 *y, quint8 *u, quint8 *v);
    static quint32 yuvToRgb(float y, float u, float v);

    static void copyRgb32(uchar *dst, qint32 dstStep, uchar *src, qint32 srcStep, int width, int height);
    static void rgb32ToRgb565(uchar *dst, qint32 dstStep, uchar *src, qint32 srcStep, int width, int height);
    static void rgb32ToRgb555(uchar *dst, qint32 dstStep, uchar *src, qint32 srcStep, int width, int height);
    static void rgb32ToRgb332(uchar *dst, qint32 dstStep, uchar *src, qint32 srcStep, int width, int height);
    static void rgb32ToRgbMap(uchar *dst, qint32 dstStep, uchar *src, qint32 srcStep, int width, int height);
    static void rgb32ToNV12(uchar *dst, uchar *src, qint32 srcStep, int width, int height);
    static void rgb32ToI420(uchar *dst, uchar *src, qint32 srcStep, int width, int height);
    static void nv12ToRgb32(uchar *dst, qint32 dstStep, uchar *src, int width, int height);
private:
    static quint32 launchServiceProcess(const QString &path, quint32 dwSessionId);
    static quint32 launchGuiProcess(const QString& path, quint32 SessionId);
};

#endif // QKXUTILS_H
