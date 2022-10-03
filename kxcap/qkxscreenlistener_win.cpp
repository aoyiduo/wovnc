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

#include "qkxscreenlistener.h"

#include "windows.h"

BOOL CALLBACK monitorEnumCallback(HMONITOR hMonitor, HDC, LPRECT, LPARAM p)
{
    QList<QKxScreenListener::DisplayInfo> *pdis = (QList<QKxScreenListener::DisplayInfo>*)p;
    MONITORINFOEXW info;
    memset(&info, 0, sizeof(MONITORINFOEXW));
    info.cbSize = sizeof(MONITORINFOEXW);
    if (GetMonitorInfoW(hMonitor, &info) == FALSE){
        return false;
    }
    QKxScreenListener::DisplayInfo di;
    di.rect = QRect(QPoint(info.rcMonitor.left, info.rcMonitor.top), QPoint(info.rcMonitor.right - 1, info.rcMonitor.bottom - 1));
    di.name = QString::fromWCharArray(info.szDevice);
    pdis->append(di);
    return true;
}

QList<QKxScreenListener::DisplayInfo> QKxScreenListener::monitors()
{
    QList<QKxScreenListener::DisplayInfo> dis;
    EnumDisplayMonitors(0, 0, monitorEnumCallback, reinterpret_cast<LPARAM>(&dis));
    return dis;
}
