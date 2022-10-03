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

#ifndef QKXCAP_SHARE_H
#define QKXCAP_SHARE_H

#if defined(KXCAP_LIBRARY)
#  define KXCAP_EXPORT Q_DECL_EXPORT
#else
#  define KXCAP_EXPORT Q_DECL_IMPORT
#endif

class QGuiApplication;
typedef void (*PFunInit)(QGuiApplication *app);

#define SERVICE_COMMAND_CODE_EXIT               (0x1)
#define SERVICE_COMMAND_CODE_START              (0x2)
#define SERVICE_COMMAND_CODE_STOP               (0x3)

#endif // QKXCAP_SHARE_H
