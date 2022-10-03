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

#ifndef QKXGUIAPPLICATION_H
#define QKXGUIAPPLICATION_H

#include <QApplication>
#include <QPointer>
#include <QAction>
#include <QSystemTrayIcon>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

class QKxSystemConfig;
class QMenu;
class QTranslator;
class QKxCapServer;

class QKxGuiApplication : public QApplication
{
    Q_OBJECT
public:
    explicit QKxGuiApplication(int &argc, char **argv);

    static QKxGuiApplication *instance();

    void init();

    bool isServiceRunning();

signals:
    void adjustLayout();
public slots:
    void onMenuAboutToShow();
    void onShowWindow();
    void onTrayActive(QSystemTrayIcon::ActivationReason reason);
    void onConfigureApply();
    void onStartService();
    void onStopService();
    void onLanguageSwitch();
    void onAboutToExit();
    void onRestartApplication();
private:
    QKxSystemConfig *get();
private:
    QPointer<QKxCapServer> m_capServer;
    QSystemTrayIcon m_tray;
    QPointer<QKxSystemConfig> m_dlgCfg;
    QPointer<QMenu> m_menu;
    QPointer<QAction> m_config;
    QPointer<QAction> m_start;
    QPointer<QAction> m_stop;
    QPointer<QTranslator> m_translator;
    bool m_isProxyRunning;
};

#endif // QKXGUIAPPLICATION_H
