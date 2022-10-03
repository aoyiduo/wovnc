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
#include <QApplication>
#include <QScreen>

QList<QKxScreenListener::DisplayInfo> QKxScreenListener::monitors()
{
    DisplayInfo di;
    QList<DisplayInfo> dis;
    QList<QScreen*> screens = QApplication::screens();
    for(int i = 0; i < screens.length(); i++) {
        QScreen *scn = screens.at(i);
        di.name = scn->name();
        di.rect = scn->geometry();
        dis.append(di);
    }
    return dis;
}
