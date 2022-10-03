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
#include <QDebug>

#include <string>
#include <memory>
#include <windows.h>
#include <wtsapi32.h>
#include <tlhelp32.h>
#include <shlobj.h> // NOLINT(build/include_order)
#include <userenv.h>

#pragma comment(lib, "wtsapi32.lib")

// ultravnc has rdp support
// https://github.com/veyon/ultravnc/blob/master/winvnc/winvnc/service.cpp
// https://github.com/TigerVNC/tigervnc/blob/master/win/winvnc/VNCServerService.cxx
// https://blog.csdn.net/MA540213/article/details/84638264

static DWORD GetLogonPid(DWORD dwSessionId, BOOL as_user)
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
                if (_wcsicmp(procEntry.szExeFile, as_user ? L"explorer.exe" : L"winlogon.exe") == 0 &&
                    ProcessIdToSessionId(procEntry.th32ProcessID, &dwLogonSessionId) &&
                    dwLogonSessionId == dwSessionId)
                {
                    dwLogonPid = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnap, &procEntry));
        CloseHandle(hSnap);
    }
    return dwLogonPid;
}

static BOOL GetSessionUserTokenWin(OUT LPHANDLE lphUserToken, DWORD dwSessionId, BOOL as_user)
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

void QKxUtils::launchProcess(const QString &path, bool isGui)
{
    DWORD sid = getCurrentSessionId();
    if(isGui) {
        launchGuiProcess(path, sid);
    }else{
        launchServiceProcess(path, sid);
    }
}

quint32 QKxUtils::getCurrentSessionId()
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

bool QKxUtils::isRunAsLocalSystem()
{
    BOOL bIsLocalSystem = FALSE;
    PSID psidLocalSystem;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    BOOL fSuccess = ::AllocateAndInitializeSid(&ntAuthority, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &psidLocalSystem);
    if (fSuccess) {
        fSuccess = ::CheckTokenMembership(0, psidLocalSystem, &bIsLocalSystem);
        ::FreeSid(psidLocalSystem);
    }
    return bIsLocalSystem;
}

bool QKxUtils::isRunAsLocalService()
{
    BOOL bIsLocalSystem = FALSE;
    PSID psidLocalSystem;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    BOOL fSuccess = ::AllocateAndInitializeSid(&ntAuthority, 1, SECURITY_LOCAL_SERVICE_RID, 0, 0, 0, 0, 0, 0, 0, &psidLocalSystem);
    if (fSuccess) {
        fSuccess = ::CheckTokenMembership(0, psidLocalSystem, &bIsLocalSystem);
        ::FreeSid(psidLocalSystem);
    }
    return bIsLocalSystem;
}

bool QKxUtils::isRunAsService()
{
    return isRunAsLocalService() || isRunAsLocalSystem();
}

quint32 QKxUtils::launchGuiProcess(const QString& path, quint32 SessionId)
{
    HANDLE hToken;
    BOOL ok = WTSQueryUserToken(SessionId, &hToken);
    if (!ok){
        return 0;
    }

    void *environment = NULL;
    ok = CreateEnvironmentBlock(&environment, hToken, TRUE);

    if (!ok) {
        CloseHandle(hToken);
        return 0;
    }

    TOKEN_LINKED_TOKEN lto;
    DWORD nbytes;
    ok = GetTokenInformation(hToken, TokenLinkedToken, &lto, sizeof (lto), &nbytes);

    if (ok) {
        CloseHandle(hToken);
        hToken = lto.LinkedToken;
    }

    STARTUPINFOW si = { sizeof (si) };
    PROCESS_INFORMATION pi = {};
    si.lpDesktop = L"winsta0\\default";

    // Do NOT want to inherit handles here, surely
    DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;
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

quint32 QKxUtils::launchServiceProcess(const QString &path, quint32 dwSessionId)
{
    HANDLE hProcess = NULL;
    HANDLE hToken = NULL;
    if (GetSessionUserTokenWin(&hToken, dwSessionId, FALSE))
    {
        STARTUPINFOW si;
        ZeroMemory(&si, sizeof si);
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        //si.lpDesktop = L"winsta0\\default";
        PROCESS_INFORMATION	pi={0};
        if (CreateProcessAsUserW(hToken, NULL, (LPWSTR)path.constData(), NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            hProcess = pi.hProcess;
        }
        CloseHandle(hToken);
        CloseHandle(hProcess);
        return 0;
    }
    return -1;
}

bool QKxUtils::isRunAsAdmin()
{
    struct Data
    {
        PACL   pACL;
        PSID   psidAdmin;
        HANDLE hToken;
        HANDLE hImpersonationToken;
        PSECURITY_DESCRIPTOR     psdAdmin;
        Data() : pACL(NULL), psidAdmin(NULL), hToken(NULL),
            hImpersonationToken(NULL), psdAdmin(NULL)
        {}
        ~Data()
        {
            if (pACL)
                LocalFree(pACL);
            if (psdAdmin)
                LocalFree(psdAdmin);
            if (psidAdmin)
                FreeSid(psidAdmin);
            if (hImpersonationToken)
                CloseHandle (hImpersonationToken);
            if (hToken)
                CloseHandle (hToken);
        }
    } data;

    BOOL   fReturn         = FALSE;
    DWORD  dwStatus;
    DWORD  dwAccessMask;
    DWORD  dwAccessDesired;
    DWORD  dwACLSize;
    DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);

    PRIVILEGE_SET   ps;
    GENERIC_MAPPING GenericMapping;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    const DWORD ACCESS_READ  = 1;
    const DWORD ACCESS_WRITE = 2;

    if (!OpenThreadToken (GetCurrentThread(), TOKEN_DUPLICATE|TOKEN_QUERY, TRUE, &data.hToken))
    {
        if (GetLastError() != ERROR_NO_TOKEN)
            return false;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE|TOKEN_QUERY, &data.hToken))
            return false;
    }

    if (!DuplicateToken (data.hToken, SecurityImpersonation, &data.hImpersonationToken))
        return false;

    if (!AllocateAndInitializeSid(&SystemSidAuthority, 2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0, &data.psidAdmin))
        return false;

    data.psdAdmin = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (data.psdAdmin == NULL)
        return false;

    if (!InitializeSecurityDescriptor(data.psdAdmin, SECURITY_DESCRIPTOR_REVISION))
        return false;

    // Compute size needed for the ACL.
    dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(data.psidAdmin) - sizeof(DWORD);

    data.pACL = (PACL)LocalAlloc(LPTR, dwACLSize);
    if (data.pACL == NULL)
        return false;

    if (!InitializeAcl(data.pACL, dwACLSize, ACL_REVISION2))
        return false;

    dwAccessMask = ACCESS_READ | ACCESS_WRITE;

    if (!AddAccessAllowedAce(data.pACL, ACL_REVISION2, dwAccessMask, data.psidAdmin))
        return false;

    if (!SetSecurityDescriptorDacl(data.psdAdmin, TRUE, data.pACL, FALSE))
        return false;

    // AccessCheck validates a security descriptor somewhat; set the group
    // and owner so that enough of the security descriptor is filled out
    // to make AccessCheck happy.

    SetSecurityDescriptorGroup(data.psdAdmin, data.psidAdmin, FALSE);
    SetSecurityDescriptorOwner(data.psdAdmin, data.psidAdmin, FALSE);

    if (!IsValidSecurityDescriptor(data.psdAdmin))
        return false;

    dwAccessDesired = ACCESS_READ;

    GenericMapping.GenericRead    = ACCESS_READ;
    GenericMapping.GenericWrite   = ACCESS_WRITE;
    GenericMapping.GenericExecute = 0;
    GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;

    if (!AccessCheck(data.psdAdmin, data.hImpersonationToken, dwAccessDesired,
                    &GenericMapping, &ps, &dwStructureSize, &dwStatus,
                    &fReturn))
    {
        return false;
    }

    return fReturn;
}
using namespace std;
wstring lowercase(const wstring& s)
{
    std::wstring str(s);
    std::transform(str.begin(), str.end(), str.begin(), [](wchar_t c) { return tolower(c); });
    return str;
}

wstring appendToEnvironmentBlock(const void* pEnvBlock, const wstring& varname, const wstring& varvalue)
{
    map<wstring, wstring> env;
    const wchar_t* currentEnv = (const wchar_t*)pEnvBlock;
    wstring result;

    // parse the current block into a map of key/value pairs
    while (*currentEnv)
    {
        wstring keyvalue = currentEnv;
        wstring key;
        wstring value;

        size_t pos = keyvalue.find_last_of(L'=');
        if (pos != wstring::npos)
        {
            key = keyvalue.substr(0, pos);
            value = keyvalue; // entire string
        }
        else
        {
            // ??? no '=' sign, just save it off
            key = keyvalue;
            value = keyvalue;
        }
        value += L'\0'; // reappend the null char

        env[lowercase(key)] = value;
        currentEnv += keyvalue.size() + 1;
    }

    // add the new key and value to the map
    if (varvalue.empty())
    {
        env.erase(lowercase(varname)); // if varvalue is empty, just assume this means, "delete this environment variable"
    }
    else
    {
        env[lowercase(varname)] = varname + L'=' + varvalue + L'\0';
    }

    // serialize the map into the buffer we just allocated
    for (auto& item : env)
    {
        result += item.second;
    }
    result += L'\0';
    auto ptr = result.c_str();

    return result;
}

bool QKxUtils::createProcessWithAdmin(const QString& path, const QStringList& envs, void *process)
{
    HANDLE hToken = NULL;
    HANDLE hTokenDup = NULL;
    PROCESS_INFORMATION *pInfo = (PPROCESS_INFORMATION)process;

    DWORD dwSessionId = getCurrentSessionId();
    if (!WTSQueryUserToken(dwSessionId, &hToken)){
        return false;
    }

    if (!DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityAnonymous, TokenPrimary, &hTokenDup)) {
        CloseHandle(hToken);
        return false;
    }

    LPVOID pEnvOrg = NULL;
    if (!CreateEnvironmentBlock(&pEnvOrg, hTokenDup, FALSE)) {
        CloseHandle(hToken);
        CloseHandle(hTokenDup);
        return false;
    }

    wstring result;
    LPVOID pEnv = pEnvOrg;
    for(int i = 0; i < envs.length(); i++) {
        QString env = envs.at(i);
        QStringList kv = env.split('=');
        QString key = kv.at(0);
        QString val = kv.at(1);
        result = appendToEnvironmentBlock(pEnv, key.toStdWString(), val.toStdWString());
        pEnv = (LPVOID)result.c_str();
    }

    if (!SetTokenInformation(hTokenDup, TokenSessionId, &dwSessionId, sizeof(DWORD))) {
        CloseHandle(hToken);
        CloseHandle(hTokenDup);
        if (pEnvOrg) {
            DestroyEnvironmentBlock(pEnvOrg);
        }
        return false;
    }

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);
    si.lpDesktop = L"WinSta0\\Default";
    si.wShowWindow = SW_SHOW;
    si.dwFlags = STARTF_USESHOWWINDOW;
    if (!CreateProcessAsUserW(hTokenDup, (LPWSTR)path.constData(), NULL, NULL, NULL, FALSE,
        NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
        pEnv, NULL, &si, pInfo)) {
        CloseHandle(hToken);
        CloseHandle(hTokenDup);
        if (pEnvOrg) {
            DestroyEnvironmentBlock(pEnvOrg);
        }
        return false;
    }

    if (pEnvOrg) {
        DestroyEnvironmentBlock(pEnvOrg);
    }

    CloseHandle(hToken);
    CloseHandle(hTokenDup);
    return true;
}

bool QKxUtils::createProcess(const QString &path, const QStringList &envs, void *process)
{
    PROCESS_INFORMATION *pInfo = (PPROCESS_INFORMATION)process;
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);

    for(int i = 0; i < envs.length(); i++) {
        QString env = envs.at(i);
        QStringList kv = env.split('=');
        QString key = kv.at(0);
        QString val = kv.at(1);
        qputenv(key.toLatin1(), val.toLatin1());
    }
    if (!::CreateProcessW(NULL, (LPWSTR)path.constData(),
                         NULL, NULL, FALSE,
                         NORMAL_PRIORITY_CLASS /*| CREATE_NEW_CONSOLE*/ | CREATE_UNICODE_ENVIRONMENT,
                         NULL, NULL, &si, pInfo)) {
        return false;
    }
    return true;
}

bool QKxUtils::installFireWall()
{
    char szPath[256] = {0};
    GetModuleFileNameA(NULL, szPath, 256);
    char *pName = strrchr(szPath, '\\');
    pName +=1;
    // netsh advfirewall firewall add rule name=\"WoVNCService\" dir=in action=allow program=\"C:\Program Files (x86)\WoVNC\wovncserver.exe\" enable=yes
    QString cmd = QString("netsh advfirewall firewall add rule name=\"%1\" dir=in action=allow program=\"%2\" enable=yes").arg(pName).arg(szPath);
    ::ShellExecuteW(NULL, (LPWSTR)cmd.constData(), NULL, NULL, NULL, SW_HIDE);
    return true;
}

quint64 QKxUtils::GetCurrentSessionId()
{
    WTS_SESSION_INFO *pSessionInfo;
    quint64 n_sessions = 0;
    BOOL ok = WTSEnumerateSessions(WTS_CURRENT_SERVER, 0, 1, &pSessionInfo, (DWORD*)&n_sessions);
    if (!ok)
        return 0;

    quint64 SessionId = 0;

    for (quint64 i = 0; i < n_sessions; ++i)
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

bool QKxUtils::shellExecuteAsAdmin(const QString& path, const QStringList& argv, bool bWait){
    QString params;
    for(int i = 0; i < argv.length(); i++) {
        params += QString(" \"%1\"").arg(argv.at(i));
    }

    SHELLEXECUTEINFOW si = {0};
    memset(&si, 0, sizeof(si));
    si.lpFile			= (LPWSTR)path.constData();
    si.cbSize			= sizeof(si);
    si.lpVerb			= L"runas";
    si.fMask			= SEE_MASK_NOCLOSEPROCESS;
    si.nShow			= SW_SHOWDEFAULT;
    if(!params.isEmpty()) {
        si.lpParameters     = (LPWSTR)params.constData();
    }

    ShellExecuteExW(&si);

    if(bWait && si.hProcess != 0){
        WaitForSingleObject(si.hProcess, INFINITE);
    }
    if(si.hProcess != 0) {
        ::CloseHandle(si.hProcess);
    }
    return si.hProcess != 0;
}

static bool switchToDesktop(HDESK desktop)
{
    HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
    if (!SetThreadDesktop(desktop)) {
        return false;
    }
    if (!CloseDesktop(old_desktop)) {
        //
    }
    return true;
}

bool QKxUtils::selectInputDesktop()
{
    HDESK desktop = OpenInputDesktop(0, FALSE, DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
                                         DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
                                         DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
                                         DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
    if (!desktop) {
        qDebug() << "Failed to OpenInputDesktop" << GetLastError() << QThread::currentThreadId();
        return false;
    }

    // - Switch into it
    if (!switchToDesktop(desktop)) {
        qDebug() << "Failed to switchToDesktop" << GetLastError() << QThread::currentThreadId();
        CloseDesktop(desktop);
        return false;
    }

    // ***
    DWORD size = 256;
    char currentname[256];
    if (GetUserObjectInformationA(desktop, UOI_NAME, currentname, 256, &size)) {
        qDebug() << "desktop name" << currentname << GetLastError() << QThread::currentThreadId();
    }
    qDebug() << "current desktop name" << currentname << QThread::currentThreadId();
    return true;
}

bool QKxUtils::inputDesktopSelected(QString &name)
{
    HDESK current = GetThreadDesktop(GetCurrentThreadId());
    HDESK input = OpenInputDesktop(0, FALSE,
                                   DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
                                   DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
                                   DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
                                   DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
    if (!input) {
        //ERROR_ACCESS_DENIED
        qDebug() << "1-inputDesktopSelected:" << ::GetLastError() << QThread::currentThreadId();
        return FALSE;
    }

    DWORD size;
    char currentname[256];
    char inputname[256];
    if (!GetUserObjectInformationA(current, UOI_NAME, currentname, sizeof(currentname), &size)) {
        CloseDesktop(input);
        qDebug() << "2-inputDesktopSelected:" << GetLastError() << QThread::currentThreadId();
        return FALSE;
    }
    if (!GetUserObjectInformationA(input, UOI_NAME, inputname, sizeof(inputname), &size)) {
        CloseDesktop(input);
        qDebug() << "3-inputDesktopSelected:" << GetLastError() << QThread::currentThreadId();
        return FALSE;
    }
    CloseDesktop(input);
    //qDebug() <<"inputDesktopSelected: CurrentName:" << currentname << inputname << QThread::currentThreadId();
    name = QString(inputname);
    return strcmp(currentname, inputname) == 0 ? true : false;
}

void QKxUtils::setCurrentProcessRealtime()
{
    ::SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
}

bool QKxUtils::isDesktopChanged()
{
    HDESK hDeskNow = GetThreadDesktop(GetCurrentThreadId());
    HDESK hDeskInput = OpenInputDesktop(0, FALSE, GENERIC_ALL);
    if (hDeskInput == NULL) {
        return false;
    }

    DWORD size;
    char nowName[256];
    char inputName[256];
    if (!GetUserObjectInformationA(hDeskNow, UOI_NAME, nowName, sizeof(nowName), &size)) {
        CloseDesktop(hDeskInput);
        return false;
    }
    if (!GetUserObjectInformationA(hDeskInput, UOI_NAME, inputName, sizeof(inputName), &size)) {
        CloseDesktop(hDeskInput);
        return false;
    }
    CloseDesktop(hDeskInput);
    //qDebug() << "nowName" << nowName << "inputName" << inputName;
    return strcmp(nowName, inputName) != 0;
}

bool QKxUtils::resetThreadDesktop()
{
    HDESK hDeskInput = OpenInputDesktop(0, FALSE, GENERIC_ALL);
    if (hDeskInput == NULL) {
        return false;
    }
    BOOL ok = SetThreadDesktop(hDeskInput);
    //qDebug() << "SetThreadDesktop" << ok;
    CloseHandle(hDeskInput);
    return ok;
}

bool QKxUtils::disableAppNap()
{
    return true;
}

QString QKxUtils::applicationFilePath()
{
    static QString path;
    if(path.isEmpty()) {
        char buffer[1024] = {0};
        ::GetModuleFileNameA(0, buffer, 1023);
        path = buffer;
    }
    return path;
}

QString QKxUtils::applicationDirPath()
{
    static QString path;
    if(path.isEmpty()) {
        char buffer[1024] = {0};
        ::GetModuleFileNameA(0, buffer, 1023);
        char* pName = strrchr(buffer, '\\');
        if(pName != nullptr) {
            pName[0] = '\0';
        }
        path = buffer;
    }
    return path;
}

QString QKxUtils::applicationName()
{
    static QString appName;
    if(appName.isEmpty()) {
        char buffer[1024] = {0};
        ::GetModuleFileNameA(0, buffer, 1023);
        char* pName = strrchr(buffer, '\\');
        if(pName != nullptr) {
            pName++;
        }
        char *pDot = strrchr(pName, '.');
        if(pDot != nullptr) {
            pDot[0] = '\0';
        }
        appName = pName;
    }
    return appName;
}

void QKxUtils::setVisibleOnDock(bool yes)
{

}

void QKxUtils::setBlankScreen(bool set)
{
    if (set) {
        SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM)2);
    } else {
        SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM)-1);
    }
}

void QKxUtils::lockWorkStation()
{
    ::LockWorkStation();
}

void QKxUtils::setDragWindow(bool on)
{
    ::SystemParametersInfoW(SPI_SETDRAGFULLWINDOWS, on ? TRUE : FALSE, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}
