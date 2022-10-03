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

#pragma once

#include <QVariant>
#include <QMap>

class QKxSetting
{
public:
    static void setValue(const QString& key, const QVariant& v);
    static QVariant value(const QString& key, const QVariant& defval);

    static void checkFirewall();
    static void updateStartTime();

    static QByteArray uuid();
    static qint64 lastDeviceID();
    static void setLastDeviceID(qint64 uid);
    static QString lastUserName();
    static void setLastUserName(const QString& name);
    static QString lastUserPassword();
    static void setLastUserPassword(const QString& pwd);
    static bool rememberAccount();
    static void setRememberAccount(bool on);
    static bool autoLogin();
    static void setAutoLogin(bool on);
    static QString authorizeLoginName();
    static void setAuthorizeLoginName(const QString& name);

    static bool lastServiceStatus();
    static void setLastServiceStatus(bool run);

    static QString ensurePath(const QString& name);
    static QString applicationConfigPath();
    static QString applicationDataPath();


    static QString logFilePath();

    static QString languageFile();
    static QMap<QString, QString> allLanguages();
    static QString languageName(const QString& path);
    static void setLanguageFile(const QString& path);

    static quint16 listenPort();
    static void setListenPort(quint16 port);
    static QString authorizePassword();
    static void setAuthorizePassword(const QString& pwd);
    static int screenIndex();
    static void setScreenIndex(int idx);

    static bool autoLockScreen();
    static void setAutoLockScreen(bool on);

    static bool dragWithContent();
    static void setDragWithContent(bool on);

    static bool blackWallpaper();
    static void setBlackWallpaper(bool on);
private:
    static QString specialFilePath(const QString& name);
};
