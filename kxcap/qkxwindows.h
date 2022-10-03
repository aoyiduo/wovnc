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

#ifndef QKXWINDOWS_H
#define QKXWINDOWS_H

#include <Windows.h>

DWORD GetLogonPid(DWORD dwSessionId, BOOL as_user);
BOOL GetSessionUserTokenWin(OUT LPHANDLE lphUserToken, DWORD dwSessionId, BOOL as_user);
HANDLE LaunchProcessWin(const QString& path, DWORD dwSessionId, BOOL as_user);
bool launchProcess(const QString& cmd, BOOL as_user);
BOOL inputDesktopSelected(QString& name);
bool selectInputDesktop();
void setBlankScreen(BOOL set);
void lockWorkStation();
HANDLE LaunchProcessWin2(const QString& path, DWORD dwSessionId);
DWORD LaunchProcess(DWORD SessionId, const char **failed_operation);
DWORD GetCurrentSessionId();
BOOL SetWinSta0Desktop(WCHAR *szDesktopName);
void PopupSessionMessage();
void PopupSessionMessage2();
void switchToLogonDesktop();
void switchToDefaultDesktop();


//netsh advfirewall firewall add rule name="app name" dir=in action=allow program="Full path of .exe" enable=yes
void setFirewallRoute(const QString& appName, const QString& appPath);

#endif
