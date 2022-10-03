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

#include <windows.h>
#include <wtsapi32.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <intrin.h>
#include <string>
#include <memory>
#include <shlobj.h> // NOLINT(build/include_order)
#include <userenv.h>

#include <QDebug>
#include <QThread>
#include <QCoreApplication>

#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "wtsapi32.lib")

void flog(char const *fmt, ...)
{
    FILE *h = fopen("C:\\Windows\\temp\\trace.log", "at");
    if (!h)
        return;
    va_list arg;
    va_start(arg, fmt);
    vfprintf(h, fmt, arg);
    va_end(arg);
    fclose(h);
}

// ultravnc has rdp support
// https://github.com/veyon/ultravnc/blob/master/winvnc/winvnc/service.cpp
// https://github.com/TigerVNC/tigervnc/blob/master/win/winvnc/VNCServerService.cxx
// https://blog.csdn.net/MA540213/article/details/84638264

DWORD GetLogonPid(DWORD dwSessionId, BOOL as_user)
{
    DWORD dwLogonPid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W procEntry;
        procEntry.dwSize = sizeof procEntry;

        if (Process32FirstW(hSnap, &procEntry))
            do
            {
                DWORD dwLogonSessionId = 0;
                qDebug() << "1-procEntry.szExeFile" << QString::fromWCharArray(procEntry.szExeFile);
                if (_wcsicmp(procEntry.szExeFile, as_user ? L"explorer.exe" : L"winlogon.exe") == 0 &&
                    ProcessIdToSessionId(procEntry.th32ProcessID, &dwLogonSessionId) &&
                    dwLogonSessionId == dwSessionId)
                {
                    qDebug() << "2-procEntry.szExeFile" << QString::fromWCharArray(procEntry.szExeFile);
                    dwLogonPid = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &procEntry));
        CloseHandle(hSnap);
    }
    return dwLogonPid;
}

// if should try WTSQueryUserToken?
// https://stackoverflow.com/questions/7285666/example-code-a-service-calls-createprocessasuser-i-want-the-process-to-run-in
BOOL GetSessionUserTokenWin(OUT LPHANDLE lphUserToken, DWORD dwSessionId, BOOL as_user)
{
    BOOL bResult = FALSE;
    DWORD Id = GetLogonPid(dwSessionId, as_user);
    if (HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Id))
    {
        bResult = OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, lphUserToken);
        CloseHandle(hProcess);
    }
    return bResult;
}

// START the app as system

HANDLE LaunchProcessWin(const QString& path, DWORD dwSessionId, BOOL as_user)
{
    HANDLE hProcess = NULL;
    HANDLE hToken = NULL;
    if (GetSessionUserTokenWin(&hToken, dwSessionId, as_user))
    {
        STARTUPINFOW si = {0};
        ZeroMemory(&si, sizeof si);
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;

        PROCESS_INFORMATION pi={0};
        LPVOID lpEnvironment = NULL;
        DWORD dwCreationFlags = DETACHED_PROCESS;
        if (as_user) {

            CreateEnvironmentBlock(&lpEnvironment, // Environment block
                                   hToken,         // New token
                                   TRUE);          // Inheritance
        }
        if (lpEnvironment) {
            dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
        }
        qDebug() << "LaunchProcessWin" << 1 << path;
        if (CreateProcessAsUserW(hToken, NULL, (LPWSTR)path.constData(), NULL, NULL, FALSE, dwCreationFlags, lpEnvironment, NULL, &si, &pi))
        {
            qDebug() << "LaunchProcessWin" << QString::number(GetLastError(), 16);
            CloseHandle(pi.hThread);
            hProcess = pi.hProcess;
        }
        qDebug() << "LaunchProcessWin" << 3;
        CloseHandle(hToken);
        if (lpEnvironment)
            DestroyEnvironmentBlock(lpEnvironment);
    }
    return hProcess;
}

// Switch the current thread to the specified desktop
static bool
switchToDesktop(HDESK desktop)
{
    HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
    if (!SetThreadDesktop(desktop))
    {
        return false;
    }
    if (!CloseDesktop(old_desktop))
    {
        //
    }
    return true;
}

// https://github.com/TigerVNC/tigervnc/blob/8c6c584377feba0e3b99eecb3ef33b28cee318cb/win/rfb_win32/Service.cxx

// Determine whether the thread's current desktop is the input one
BOOL
inputDesktopSelected(QString& name)
{
    HDESK current = GetThreadDesktop(GetCurrentThreadId());
    HDESK input = OpenInputDesktop(0, FALSE,
                                   DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
                                       DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
                                       DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
                                       DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
    if (!input)
    {
        //ERROR_ACCESS_DENIED
        qDebug() << "1-inputDesktopSelected:" << ::GetLastError() << QThread::currentThreadId();
        return FALSE;
    }

    DWORD size;
    char currentname[256];
    char inputname[256];

    if (!GetUserObjectInformationA(current, UOI_NAME, currentname, sizeof(currentname), &size))
    {
        CloseDesktop(input);
        qDebug() << "2-inputDesktopSelected:" << GetLastError() << QThread::currentThreadId();
        return FALSE;
    }
    if (!GetUserObjectInformationA(input, UOI_NAME, inputname, sizeof(inputname), &size))
    {
        CloseDesktop(input);
        qDebug() << "3-inputDesktopSelected:" << GetLastError() << QThread::currentThreadId();
        return FALSE;
    }
    CloseDesktop(input);
    qDebug() <<"inputDesktopSelected: CurrentName:" << currentname << inputname << QThread::currentThreadId();
    name = QString(inputname);
    return strcmp(currentname, inputname) == 0 ? TRUE : FALSE;
}

// Switch the current thread into the input desktop


bool
selectInputDesktop()
{
    // - Open the input desktop
    HDESK desktop = OpenInputDesktop(0, FALSE,
                                     DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
                                         DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
                                         DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
                                         DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
    if (!desktop)
    {
        qDebug() << "Failed to OpenInputDesktop" << GetLastError() << QThread::currentThreadId();
        return false;
    }

    // - Switch into it
    if (!switchToDesktop(desktop))
    {
        qDebug() << "Failed to switchToDesktop" << GetLastError() << QThread::currentThreadId();
        CloseDesktop(desktop);
        return false;
    }

    // ***
    DWORD size = 256;
    char currentname[256];
    if (GetUserObjectInformationA(desktop, UOI_NAME, currentname, 256, &size))
    {
        //
        qDebug() << "desktop name" << currentname << GetLastError() << QThread::currentThreadId();
    }
    qDebug() << "current desktop name" << currentname << QThread::currentThreadId();
    return true;
}

void setBlankScreen(BOOL set)
{
    if (set) {
        SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM)2);
    } else {
        SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM)-1);
    }
}

bool launchProcess(const QString& cmd, BOOL as_user)
{
    DWORD sid = WTSGetActiveConsoleSessionId();
    qDebug() << "launchProcess" << "sid" << sid;
    bool ok = LaunchProcessWin(cmd, sid, as_user);
    //bool ok = LaunchProcessWin2(cmd, sid);
    qDebug() << "launchProcess" << "ok" << ok;
    return ok;
}

void lockWorkStation()
{
    ::LockWorkStation();
}


// Launch notepad as currently logged-on user
DWORD LaunchProcess(DWORD SessionId, const char **failed_operation)
{
    HANDLE hToken;
    BOOL ok = WTSQueryUserToken(SessionId, &hToken);
    if (!ok)
        return 0;

    void *environment = NULL;
    ok = CreateEnvironmentBlock(&environment, hToken, TRUE);

    if (!ok)
    {
        CloseHandle(hToken);
        return 0;
    }

    TOKEN_LINKED_TOKEN lto;
    DWORD nbytes;
    ok = GetTokenInformation(hToken, TokenLinkedToken, &lto, sizeof (lto), &nbytes);

    if (ok)
    {
        CloseHandle(hToken);
        hToken = lto.LinkedToken;
    }

    STARTUPINFOW si = { sizeof (si) };
    PROCESS_INFORMATION pi = {};
    si.lpDesktop = L"winsta0\\default";
    QString path = QCoreApplication::applicationFilePath();

    // Do NOT want to inherit handles here, surely
    DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | /* CREATE_NEW_CONSOLE | */ CREATE_UNICODE_ENVIRONMENT;
    ok = CreateProcessAsUserW(hToken, (LPWSTR)path.constData()/*"c:\\windows\\system32\\notepad.exe"*/, NULL, NULL, NULL, FALSE,
        dwCreationFlags, environment, NULL, &si, &pi);

    DestroyEnvironmentBlock(environment);
    CloseHandle(hToken);

    if (!ok){
        return 0;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}

// Determine the session ID of the currently logged-on user
DWORD GetCurrentSessionId()
{
    WTS_SESSION_INFO *pSessionInfo;
    DWORD n_sessions = 0;
    BOOL ok = WTSEnumerateSessions(WTS_CURRENT_SERVER, 0, 1, &pSessionInfo, &n_sessions);
    if (!ok)
        return 0;

    DWORD SessionId = 0;

    for (DWORD i = 0; i < n_sessions; ++i)
    {
        if (pSessionInfo[i].State == WTSActive)
        {
            SessionId = pSessionInfo[i].SessionId;
            break;
        }
    }

    WTSFreeMemory(pSessionInfo);
    return SessionId;
}

HANDLE LaunchProcessWin2(const QString& path, DWORD dwSessionId)
{
    HANDLE hProcess = NULL;
    HANDLE hToken = NULL;
    if (GetSessionUserTokenWin(&hToken, dwSessionId, FALSE))
    {
        QString cmdLine = QString("\"%1\" -noconsole -service_run").arg(path);
        STARTUPINFOW si;
        ZeroMemory(&si, sizeof si);
        si.cb = sizeof si;
        si.dwFlags = STARTF_USESHOWWINDOW;
        PROCESS_INFORMATION	pi;
        if (CreateProcessAsUserW(hToken, NULL, (LPWSTR)cmdLine.constData(), NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            hProcess = pi.hProcess;
        }
        CloseHandle(hToken);
    }
    return hProcess;
}

void PopupSessionMessage()
{
    DWORD sid = GetCurrentSessionId();
    QString path = QCoreApplication::applicationFilePath();
    LaunchProcessWin2(path, sid);
}

void PopupSessionMessage2()
{
#if 1
    DWORD SessionId = GetCurrentSessionId ();
    const char *failed_operation = "";
    DWORD dwResult = LaunchProcess(SessionId, &failed_operation);
#else
    const QString title = "(c) 2018 Contoso Corporation";
    for(int i = 0; i < 10; i++){
        DWORD response = IDOK;
        DWORD SessionId = GetCurrentSessionId ();
        if (SessionId == 0) {
            Sleep (2000);
            continue;
        }

        // Pop-up a message on the screen of the currently logged-on user (session 1)
        QString buf = QString("Ready to launch..., SessionId = %1").arg(SessionId);
        WTSSendMessageW(WTS_CURRENT_SERVER_HANDLE, SessionId, (LPWSTR)title.constData(), title.length(), (LPWSTR)buf.constData(), buf.length(), MB_OKCANCEL, 0, &response, TRUE);
        if (response == IDCANCEL){
            break;
        }

        const char *failed_operation = "";
        DWORD dwResult = LaunchProcess(SessionId, &failed_operation);

        // Report results
        buf = QString("LaunchProcess returned %l from %2").arg(dwResult).arg(failed_operation);
        WTSSendMessageW(WTS_CURRENT_SERVER_HANDLE, SessionId, (LPWSTR)title.constData(), title.length(), (LPWSTR)buf.length(), buf.length(), MB_OK, 0, &response, TRUE);
        break;
    }
#endif
}

void ShowLastErrorMessage(DWORD errCode, QString errTitle)
{
    WCHAR* errorText = NULL;

    FormatMessageW(
       FORMAT_MESSAGE_FROM_SYSTEM |
       FORMAT_MESSAGE_ALLOCATE_BUFFER |
       FORMAT_MESSAGE_IGNORE_INSERTS,
       NULL,
       errCode,
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
       (LPWSTR)&errorText,
       0,
       NULL);

    if ( NULL != errorText )
    {
        qDebug() << QString("Title:%1 - ErrorCode:%2 - %3").arg(errTitle).arg(errCode).arg(QString::fromWCharArray(errorText));
        LocalFree(errorText);
    }
}

BOOL SetWinSta0Desktop(WCHAR *szDesktopName)
{
    BOOL bSuccess = FALSE;

    qDebug() << "SetWinSta0Desktop" << QString::fromWCharArray(szDesktopName);
    HWINSTA hWinSta0 = OpenWindowStationW(L"WinSta0", FALSE, MAXIMUM_ALLOWED);
    if (NULL == hWinSta0) {
        ShowLastErrorMessage(GetLastError(), "OpenWindowStation");
    }

    bSuccess = SetProcessWindowStation(hWinSta0);
    if (!bSuccess) {
        ShowLastErrorMessage(GetLastError(), "SetProcessWindowStation");
    }

    HDESK hDesk = OpenDesktopW(szDesktopName, 0, FALSE, MAXIMUM_ALLOWED);
    if (NULL == hDesk) {
        ShowLastErrorMessage(GetLastError(), "OpenDesktop");
    }

    bSuccess = SetThreadDesktop(hDesk);
    if (!bSuccess) {
        ShowLastErrorMessage(GetLastError(), "SetThreadDesktop");
    }

    if (hDesk != NULL) {
        CloseDesktop(hDesk);
    }
    if (hWinSta0 != NULL) {
        CloseWindowStation(hWinSta0);
    }

    return bSuccess;
}

void switchToLogonDesktop()
{
    SetWinSta0Desktop(L"winlogon");
}

void switchToDefaultDesktop()
{
    SetWinSta0Desktop(L"default");
}

void setFirewallRoute(const QString &appName, const QString &appPath)
{
    //ALLOW: netsh advfirewall firewall add rule name="app name" dir=in action=allow program="Full path of .exe" enable=yes
    //netsh advfirewall firewall Delete rule name="Rule Name"
    //BLOCK: netsh advfirewall firewall add rule name="app name" dir=in action=block program="Full path of .exe" enable=yes
}
