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

#include "qkxguiapplication.h"
#include "qkxsystemconfig.h"
#include "qkxutils.h"
#include "qwovncserverdef.h"
#include "qkxsetting.h"
#include "qkxglobal.h"
#include "qkxcapserver.h"
#include "qkxcapoption.h"
#include "qtservice.h"

#include <QTranslator>
#include <QMenu>
#include <QLocalSocket>
#include <QTimer>
#include <QDir>
#include <QThread>
#include <QJsonDocument>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QProcess>

QKxGuiApplication::QKxGuiApplication(int &argc, char **argv)
    : QApplication(argc, argv)
{
    onLanguageSwitch();

    bool brun = QKxSetting::lastServiceStatus();    
    if(brun) {
        QTimer::singleShot(100, this, SLOT(onStartService()));
    }
    int index = QKxSetting::screenIndex();
    QKxCapOption::instance()->setScreenIndex(index);
    QString pwd = QKxSetting::authorizePassword();
    QKxCapOption::instance()->setAuthorizePassword(pwd);
    QKxCapOption::instance()->setBlackWallpaper(QKxSetting::blackWallpaper());
    QKxCapOption::instance()->setAutoLockScreen(QKxSetting::autoLockScreen());
    QKxCapOption::instance()->setDragWithContent(QKxSetting::dragWithContent());
    QKxCapOption::instance()->setEmptyFrameEnable(true);
    QKxCapOption::instance()->setMaxFPS(30);
}

QKxGuiApplication *QKxGuiApplication::instance()
{
    return qobject_cast<QKxGuiApplication*>(QCoreApplication::instance());
}

void QKxGuiApplication::onMenuAboutToShow()
{
    bool brun = m_capServer != nullptr;
    m_start->setVisible(!brun);
    m_stop->setVisible(brun);
}

void QKxGuiApplication::onShowWindow()
{
    QKxSystemConfig *cfg = get();
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect rt = screen->geometry();
    cfg->setVisible(true);
    cfg->raise();
    cfg->activateWindow();
    QSize sz = cfg->size();
    QPoint pt = rt.center();
    cfg->move(pt.x() - sz.width(), pt.y() - sz.height());
}

void QKxGuiApplication::onTrayActive(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::DoubleClick) {
        onShowWindow();
    }
}

void QKxGuiApplication::onConfigureApply()
{
    if(m_capServer) {
        m_capServer->deleteLater();
    }
    QTimer::singleShot(100, this, SLOT(onStartService()));
}

void QKxGuiApplication::onStartService()
{
    if(m_capServer) {
        m_capServer->deleteLater();
        QTimer::singleShot(100, this, SLOT(onStartService()));
        return;
    }
    quint16 port = QKxSetting::listenPort();
    m_capServer = new QKxCapServer("VNC_SERVER_URL", "0.0.0.0", port,  this);
    if(m_dlgCfg) {
        if(m_capServer->listenPort() == 0) {
            QMessageBox::warning(m_dlgCfg, tr("Listen Error"), tr("bad listen port ") + QString::number(port));
        }
        QMetaObject::invokeMethod(m_dlgCfg, "syncCaptureStatus", Qt::QueuedConnection);
    }
}

void QKxGuiApplication::onStopService()
{
    m_capServer->deleteLater();
    if(m_dlgCfg) {
        QMetaObject::invokeMethod(m_dlgCfg, "syncCaptureStatus", Qt::QueuedConnection);
    }
}

void QKxGuiApplication::onLanguageSwitch()
{
    if(m_translator == nullptr) {
        m_translator = new QTranslator(this);
    }
    QString lang = QKxSetting::languageFile();
     if(!lang.isEmpty() && m_translator->load(lang)){
        installTranslator(m_translator);
     }
}

void QKxGuiApplication::onAboutToExit()
{
    m_tray.hide();
#ifdef Q_OS_WIN    
    if(m_capServer->isRunAsService()) {
        QtServiceController sc(SERVICE_NAME);
        if(sc.isRunning()) {
            sc.sendCommand(SERVICE_COMMAND_CODE_EXIT);
        }
    }
#endif
    delete m_capServer;
    QCoreApplication::quit();
}

void QKxGuiApplication::onRestartApplication()
{
    if(m_capServer != nullptr && m_capServer->isRunAsService()) {
        quit();
    }
}

void QKxGuiApplication::onServicePreview()
{
    if(m_capServer != nullptr) {
        QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
        path += "/wovncviewer.exe";
#else
        path += "/wovncviewer";
#endif
        QString pass = QKxSetting::authorizePassword();
        quint16 port = QKxSetting::listenPort();
        QStringList args;
        args.append("--host=127.0.0.1");
        args.append(QString("--port=%1").arg(port));
        args.append(QString("--pass=%1").arg(pass));
        args.append(QString("--viewOnly=true"));
        QProcess* proc = new QProcess(this);
        proc->setProgram(path);
        proc->setArguments(args);
        proc->start();
        QObject::connect(proc, SIGNAL(finished(int)), proc, SLOT(deleteLater()));
        QObject::connect(proc, SIGNAL(errorOccurred(QProcess::ProcessError)), proc, SLOT(deleteLater()));
    }else{
        QMessageBox::information(m_dlgCfg, tr("Error"), tr("Service Not Start"));
    }
}

QKxSystemConfig *QKxGuiApplication::get()
{
    if(m_dlgCfg == nullptr){
        m_dlgCfg = new QKxSystemConfig();
        QObject::connect(m_dlgCfg, SIGNAL(apply()), this, SLOT(onConfigureApply()));
    }
    return m_dlgCfg;
}

void QKxGuiApplication::init()
{
    QMenu *menu = new QMenu();
    menu->addAction(QIcon(":/vncserver/resource/skin/connect.png"), QObject::tr("Preview"), this, SLOT(onServicePreview()));
    m_start = menu->addAction(QIcon(":/vncserver/resource/skin/play.png"), QObject::tr("Start Service"), this, SLOT(onStartService()));
    m_stop = menu->addAction(QIcon(":/vncserver/resource/skin/stop.png"), QObject::tr("Stop Service"), this, SLOT(onStopService()));
    m_stop->setVisible(false);
    m_config = menu->addAction(QIcon(":/vncserver/resource/skin/configure.png"), QObject::tr("Configure"), this, SLOT(onShowWindow()));
    menu->addAction(QIcon(":/vncserver/resource/skin/exit.png"), QObject::tr("Exit"), this, SLOT(onAboutToExit()));
    m_menu = menu;

    m_tray.setIcon(QIcon(":/vncserver/resource/skin/vncserver2.png"));
    m_tray.setToolTip(QObject::tr("WoVNCServer"));
    m_tray.setContextMenu(menu);
    m_tray.show();
    QObject::connect(menu, SIGNAL(aboutToShow()), this, SLOT(onMenuAboutToShow()));
    QObject::connect(&m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onTrayActive(QSystemTrayIcon::ActivationReason)));
    QObject::connect(&m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onTrayActive(QSystemTrayIcon::ActivationReason)));

    m_isProxyRunning = false;
}

bool QKxGuiApplication::isServiceRunning()
{
    return m_capServer != nullptr && m_capServer->listenPort() > 0;
}
