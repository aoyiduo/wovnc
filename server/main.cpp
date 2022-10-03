/**************************************************************************************
*
* Copyright (C) 2022 Guangzhou AoYiDuo Network Technology Co.,Ltd
* All Rights Reserved.
*
* Contact: http://www.aoyiduo.com
*
*   this file is used under the terms of the GPLv3[GNU GENERAL PUBLIC LICENSE v3]
* more information follow the website: https://www.gnu.org/licenses/gpl-3.0.en.html
***************************************************************************************/

#include <QApplication>
#include <QtDebug>

#include <QIcon>
#include <QMenuBar>
#include <QProcess>
#include <QMessageBox>
#include <QList>
#include <QByteArray>
#include <QStyleFactory>
#include <QMainWindow>
#include <QScrollArea>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QMap>
#include <QBuffer>
#include <QProcess>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QMutex>

#include "version.h"
#include "qkxsystemconfig.h"
#include "qkxsetting.h"
#include "qkxguiapplication.h"
#include "qkxfirewall.h"
#include "qkxserviceapplication.h"
#include "qkxutils.h"
#include "qkxcap_share.h"
#include "qkxglobal.h"
#include "qkxcapserver.h"
#include "qkxdaemonslave.h"

static QFile g_fileLog;
static QMutex g_mutexFileLog;

static void MyMessageHandler(QtMsgType type, const QMessageLogContext & context, const QString & text)
{
    const QDateTime datetime = QDateTime::currentDateTime();
    const char * typeText = NULL;
    switch (type)
    {
    case QtDebugMsg:
    case QtInfoMsg:
        typeText = "Info";
        break;
    case QtWarningMsg:
        typeText = "Warning";
        break;
    case QtCriticalMsg:
        typeText = "Critical";
        break;
    case QtFatalMsg:
        typeText = "Fatal";
        break;
    }
    const QString finalText = QString("%1 %2 %3\n").arg(datetime.toString("yyyyMMdd/hh:mm:ss.zzz")).arg(typeText).arg(text);
    if (g_fileLog.isOpen())
    {
        QMutexLocker locker(&g_mutexFileLog);
        if (g_fileLog.size() == 0)
            g_fileLog.write("\xef\xbb\xbf");
        g_fileLog.write(finalText.toUtf8());
        g_fileLog.flush();
    }
}

void setDebugMessageToFile(const QString& name, bool tryDelete = false)
{
    QString path = QKxSetting::applicationDataPath();
    QString fullFile = path + "/" + name;
    QFileInfo fi(fullFile);
    int fs = fi.size();
    if(fs > 1024 * 1024 || tryDelete) {
        QFile::remove(fullFile);
    }
    qInstallMessageHandler(MyMessageHandler);
    g_fileLog.setFileName(fullFile);
    g_fileLog.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

int PowerMain(int argc, char *argv[])
{
    QKxGuiApplication app(argc, argv);
#ifndef QT_DEBUG
    setDebugMessageToFile("gui.log");
#endif

    QApplication::setWindowIcon(QIcon(":/vncserver/resource/skin/vncserver2.png"));
    QApplication::setQuitOnLastWindowClosed(false);
    QKxCapServer::ExitWithParentProcess(&app);

    app.init();
    return app.exec();
}

void VncMainCallBack(QGuiApplication *app)
{
    setDebugMessageToFile("vnc.log");
    qDebug() << "VncMain" << QKxSetting::applicationDataPath();
    QKxSetting::updateStartTime();
}

int VncMain(int argc, char *argv[])
{
    return QKxCapServer::VncMain(argc, argv, VncMainCallBack);
}

int GuiMain(int argc, char *argv[])
{
    QKxGuiApplication app(argc, argv);
    setDebugMessageToFile("gui.log");
    QApplication::setWindowIcon(QIcon(":/vncserver/resource/skin/vncserver2.png"));
    QApplication::setQuitOnLastWindowClosed(false);

    return app.exec();
}

#ifdef Q_OS_WIN
#include "qtservice.h"

int InstallMain(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    setDebugMessageToFile("install.log", true);

    return QKxCapServer::InstallMain(argc, argv, SERVICE_NAME);
}

int UninstallMain(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    setDebugMessageToFile("uninstall.log", true);

    return QKxCapServer::UninstallMain(argc, argv, SERVICE_NAME);
}


int ActiveMain(int argc, char *argv[])
{
    QString path = QKxUtils::applicationFilePath();
    QKxUtils::checkFirewall();
    QtServiceController sc(SERVICE_NAME);    
    if(sc.isInstalled()) {
        if(sc.isSameService(path)) {
            if(sc.isRunning()) {
                //sc.sendCommand(SERVICE_COMMAND_CODE_START);
            }else if(QKxUtils::isRunAsAdmin()){
                sc.start();
            }else{
                QKxUtils::shellExecuteAsAdmin(QDir::toNativeSeparators(path), QStringList() << "-start", true);
            }
        }else{
            QKxUtils::checkFirewall();
            QKxUtils::shellExecuteAsAdmin(QDir::toNativeSeparators(path), QStringList() << "-install", true);
        }
    }else{
        QKxUtils::checkFirewall();
        QKxUtils::shellExecuteAsAdmin(QDir::toNativeSeparators(path), QStringList() << "-install", true);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    QStringList args = QKxServiceApplication::internalArguments();
    args << "-service";
    for(int i = 0; i < argc; i++) {
        if(strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "-install") == 0) {
            return InstallMain(argc, argv);
        }else if(strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "-uninstall") == 0) {
            return UninstallMain(argc, argv);
        }else if(args.contains(argv[i])) {
            QKxServiceApplication service(argc, argv, SERVICE_NAME);
            return service.exec();
        }
    }
    QByteArray name = qgetenv("RUN_ACTION_NAME");
    if(name.startsWith("main:")) {
        return PowerMain(argc, argv);
    }else if(name.startsWith("vnc:")) {
        return VncMain(argc, argv);
    }
    return ActiveMain(argc, argv);
}

#else

int main(int argc, char *argv[])
{
    QByteArray name = qgetenv("RUN_ACTION_NAME");
    if(name.startsWith("vnc:")) {
        return VncMain(argc, argv);
    }
    return PowerMain(argc, argv);
}
#endif
