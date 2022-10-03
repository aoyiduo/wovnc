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

#include "qkxsetting.h"
#include "qkxutils.h"
#include "qwovncserverdef.h"

#include <QSettings>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QProcess>
#include <QTranslator>
#include <QTcpServer>
#include <QUuid>
#include <QDateTime>
#include <QDebug>
#include <QCryptographicHash>

#define APPLICATION_DATA_PATH ("FEIDESK_DATA_PATH")

Q_GLOBAL_STATIC_WITH_ARGS(QSettings, gSettings, (QKxSetting::applicationConfigPath(), QSettings::IniFormat))

void QKxSetting::setValue(const QString &key, const QVariant &v)
{
    gSettings->setValue(key, v);
    gSettings->sync();
}

QVariant QKxSetting::value(const QString &key, const QVariant &defval)
{
    qDebug() << "fileName" << gSettings->fileName();
    return gSettings->value(key, defval);
}

void QKxSetting::checkFirewall()
{
    QString path = QKxUtils::applicationFilePath();
    QString appPath = value("lastAppPath", "").toString();
    qDebug() << "checkFirewall" << path << appPath;
    QTcpServer hit;
    hit.listen();
}

void QKxSetting::updateStartTime()
{
    QDateTime t = QDateTime::currentDateTime();
    QString tm = t.toString(Qt::ISODate);
    QKxSetting::setValue("startTime", tm);
}

QByteArray QKxSetting::uuid()
{
    QString path = QKxSetting::applicationDataPath();
    path.append("/uid");
    QDir d(path);
    path = d.absoluteFilePath(path);
    QFile f(path);
    if(f.exists()) {
        if(f.open(QFile::ReadOnly)) {
            QByteArray line = f.readLine(100);
            QUuid uid = QUuid::fromRfc4122(QByteArray::fromBase64(line));
            if(!uid.isNull()) {
                return line;
            }
        }
        f.remove();
    }
    if(f.open(QFile::WriteOnly)) {
        QUuid uid = QUuid::createUuid();
        QByteArray id = uid.toRfc4122().toBase64(QByteArray::OmitTrailingEquals);
        f.write(id);
        return id;
    }
    return "";
}

qint64 QKxSetting::lastDeviceID()
{
    return gSettings->value("lastDeviceID", 0).toLongLong();
}

void QKxSetting::setLastDeviceID(qint64 uid)
{
    gSettings->setValue("lastDeviceID", uid);
}

QString QKxSetting::lastUserName()
{
    return gSettings->value("userName", "").toString();
}

void QKxSetting::setLastUserName(const QString &name)
{
    gSettings->setValue("userName", name);
}

QString QKxSetting::lastUserPassword()
{
    QString pwd = gSettings->value("userPassword", "").toString();
    return pwd;
}

void QKxSetting::setLastUserPassword(const QString &pwd)
{
    gSettings->setValue("userPassword", pwd);
}

bool QKxSetting::rememberAccount()
{
    return gSettings->value("rememberAccount", false).toBool();
}

void QKxSetting::setRememberAccount(bool on)
{
    gSettings->setValue("rememberAccount", on);
}

bool QKxSetting::autoLogin()
{
    return gSettings->value("autoLogin", false).toBool();
}

void QKxSetting::setAutoLogin(bool on)
{
    gSettings->setValue("autoLogin", on);
}

QString QKxSetting::authorizeLoginName()
{
    return gSettings->value("authorizeLoginName").toString();
}

void QKxSetting::setAuthorizeLoginName(const QString &name)
{
    gSettings->setValue("authorizeLoginName", name);
}

bool QKxSetting::lastServiceStatus()
{
    return gSettings->value("lastServiceStatus", true).toBool();
}

void QKxSetting::setLastServiceStatus(bool run)
{
    gSettings->setValue("lastServiceStatus", run);
}

QString QKxSetting::ensurePath(const QString &id)
{
    QString path = QKxSetting::applicationDataPath() + "/" + id;
    QFileInfo fi(path);
    if(fi.exists()) {
        if(!fi.isDir()) {
            if(!fi.isFile()) {
                return QString("");
            }
            if(!QFile::remove(path)) {
                return QString("");
            }
            QDir d;
            d.mkpath(path);
            return path;
        }
    }else{
        QDir d;
        d.mkpath(path);
    }
    return path;
}

QString QKxSetting::applicationConfigPath()
{
    static QString path = QDir::cleanPath(QString("%1/%2.ini").arg(QKxSetting::applicationDataPath()).arg(QKxUtils::applicationName()));
    return path;
}

QString QKxSetting::logFilePath()
{
    return specialFilePath("log");
}


QString QKxSetting::languageFile()
{
    QString name = QKxSetting::value("language/path", "English").toString();
    if(name.isEmpty()) {
        QLocale local;
        QStringList langs = local.uiLanguages();
        if(!langs.isEmpty()) {
            QString lang = langs.at(0);
            lang = lang.split('-').at(0);
            QString path = QString(":/vncserver/language/vncserver_%1.qm").arg(lang.toLower());
            QMap<QString,QString> langs = allLanguages();
            if(langs.values().contains(path)) {
                return path;
            }
        }
        return QString(":/vncserver/language/vncserver_en.qm");;
    }
    return name;
}

QMap<QString, QString> QKxSetting::allLanguages()
{
    QMap<QString, QString> langs;
    QDir dir(":/vncserver/language");
    QStringList fileNames = dir.entryList(QStringList("*.qm"), QDir::Files, QDir::Name);
    for(int i = 0;i < fileNames.length(); i++) {
        QString path = dir.filePath(fileNames.at(i));
        QString type = QKxSetting::languageName(path);
        if(!type.isEmpty()){
            langs.insert(type, path);
        }else{
            QMessageBox::warning(nullptr, "warning", QString("The language file has no name:%1").arg(path));
        }
    }
    return langs;
}

QString QKxSetting::languageName(const QString &path)
{
    QTranslator translator;
    translator.load(path);
    return translator.translate("English", "English");
}

void QKxSetting::setLanguageFile(const QString &path)
{
    QKxSetting::setValue("language/path", path);
}

QString QKxSetting::applicationDataPath()
{
    static QString userDataPath;

    if(userDataPath.isEmpty()){
        QByteArray wap = qgetenv(APPLICATION_DATA_PATH);
        if(wap.isEmpty()){
            QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
            if(path.isEmpty()) {
                path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
                if(path.isEmpty()) {
                    path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
                }
            }
            if(!path.isEmpty()) {
                QString dataPath;
                if(path.endsWith(QKxUtils::applicationName())) {
                    dataPath = path;
                }else if(path.at(0) == '/'){
                    dataPath = path + "/."+QKxUtils::applicationName()+"/";
                }else {
                    dataPath = path + "/" + QKxUtils::applicationName()+"/";
                }
                userDataPath = QDir::cleanPath(dataPath);                
            }
        }else{
            userDataPath = wap;
        }
        QFileInfo fi(userDataPath);
        if(!fi.exists() || !fi.isDir()) {
            QDir dir;
            dir.rmpath(userDataPath);
            dir.mkpath(userDataPath);
        }
        {
            QString path = QKxUtils::applicationDirPath();
            QFile f(path + "/" + "datapath");
            if(f.open(QFile::WriteOnly)) {
                f.write(userDataPath.toUtf8());
            }
        }
        QProcess::execute(QString("chmod -R a+rw %1").arg(userDataPath));
        qputenv(APPLICATION_DATA_PATH, userDataPath.toUtf8());
    }
    return userDataPath;
}

QString QKxSetting::specialFilePath(const QString &name)
{
    QString path = QDir::cleanPath(applicationDataPath() + "/" + name);
    QFileInfo fi(path);
    if(!fi.exists() || !fi.isDir()) {
        QDir dir;
        dir.rmpath(path);
        dir.mkpath(path);
    }
    return path;
}

quint16 QKxSetting::listenPort()
{
    return gSettings->value("port", "5901").toInt();
}

void QKxSetting::setListenPort(quint16 port)
{
    gSettings->setValue("port", port);
}

QString QKxSetting::authorizePassword()
{
    return gSettings->value("password", "123456789").toString();
}

void QKxSetting::setAuthorizePassword(const QString &pwd)
{
    gSettings->setValue("password", pwd);
}

int QKxSetting::screenIndex()
{
    int v = gSettings->value("screen/index", 0).toInt();
    return v;
}

void QKxSetting::setScreenIndex(int idx)
{
    gSettings->setValue("screen/index", idx);
}

bool QKxSetting::autoLockScreen()
{
    return gSettings->value("screen/lockScreen", true).toBool();
}

void QKxSetting::setAutoLockScreen(bool on)
{
    gSettings->setValue("screen/lockScreen", on);
}

bool QKxSetting::dragWithContent()
{
    return gSettings->value("screen/dragContent", true).toBool();
}

void QKxSetting::setDragWithContent(bool on)
{
    gSettings->setValue("screen/dragContent", on);
}

bool QKxSetting::blackWallpaper()
{
    return gSettings->value("screen/blackWallpaper", true).toBool();
}

void QKxSetting::setBlackWallpaper(bool on)
{
    gSettings->setValue("screen/blackWallpaper", on);
}
