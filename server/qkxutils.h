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
#include <QMap>


class QIODevice;

class QKxUtils
{
public:
    static quint16 randomPort();
    static quint16 firstPort(const QByteArray& id);
    static QByteArray randomPassword(int cnt=8);
    static QByteArray randomData(int cnt=8);
    static QByteArray deviceName();
    static bool checkEmailOrPhone(const QString& txt);
    static bool checkPassword(const QString& pwd);
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
    static bool createWinProcess(const QString& path, const QStringList& env, void* process);
    static bool createProcess(const QString& path, const QStringList& env, void* process);
    static bool startDetachProcess(const QString& path, const QMap<QString,QString>& env);
    static bool installFireWall();
    static quint64 GetCurrentSessionId();
    static bool shellExecuteAsAdmin(const QString& path, const QStringList& argv, bool bWait);
    static bool selectInputDesktop();
    static bool inputDesktopSelected(QString& name);
    static void setCurrentProcessRealtime();
    static bool isDesktopChanged();
    static void resetThreadDesktop();
    /*socket*/
    static bool createPair(int fd[]);
    static bool isAgain(int err);
    static void setSocketNonBlock(int sock, bool on);
    static void setSocketNoDelay(int sock, bool on);
    static int socketError();
    static int xRecv(int sock, char *buf, int len, int flag=0);
    static int xSend(int sock, char *buf, int len, int flag=0);
    static bool waitForRead(QIODevice *dev, qint64 cnt);


    /* disable App Nap*/
    static bool disableAppNap();
    static QString applicationFilePath();
    static QString applicationDirPath();
    static QString applicationName();
    static void setVisibleOnDock(bool yes);

    /**/
    static QByteArray md5(const QByteArray& tmp);

    /* must be run on common user mode, or would not popup firewall request */
    static void checkFirewall();
private:
    static quint32 launchServiceProcess(const QString &path, quint32 dwSessionId);
    static quint32 launchGuiProcess(const QString& path, quint32 SessionId);
};

#endif // QKXUTILS_H
