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

#ifndef QKXCAPOPTION_H
#define QKXCAPOPTION_H

#include "qkxcap_share.h"

#include <QObject>
#include <QVariant>

class QKxCapOptionPrivate;
class KXCAP_EXPORT QKxCapOption : public QObject
{
    Q_OBJECT
public:
    explicit QKxCapOption(QObject *parent = nullptr);
    virtual ~QKxCapOption();
    static QKxCapOption *instance();
    int screenIndex() const;
    void setScreenIndex(int idx);
    QString authorizePassword() const;
    void setAuthorizePassword(const QString& pwd);
    int maxFPS() const;
    void setMaxFPS(int fps);
    bool enableEmptyFrame();
    void setEmptyFrameEnable(bool ok);
    void setServerUrl(const QString& url);
    QString serverUrl() const;
    QString serverUrlNodePath() const;
    QMap<QString,QVariant> all() const;

    bool autoLockScreen() const;
    void setAutoLockScreen(bool on);

    bool dragWithContent() const;
    void setDragWithContent(bool on);

    bool blackWallpaper();
    void setBlackWallpaper(bool on);
private:
    QKxCapOptionPrivate *m_prv;
};

#endif // QKXCAPOPTION_H
