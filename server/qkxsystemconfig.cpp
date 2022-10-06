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

#include "qkxsystemconfig.h"
#include "ui_qkxsystemconfig.h"
#include "qkxsetting.h"
#include "qtservice.h"
#include "qwovncserverdef.h"
#include "qkxutils.h"
#include "qkxguiapplication.h"
#include "version.h"
#include "qkxglobal.h"
#include "qkxcapoption.h"
#include "qkxscreenlistener.h"
#include "qkxmoredialog.h"

#include <QCloseEvent>
#include <QMenu>
#include <QTimer>
#include <QIntValidator>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDir>
#include <QDebug>
#include <QScreen>
#include <QStringListModel>

QKxSystemConfig::QKxSystemConfig(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QKxSystemConfig)
{
    ui->setupUi(this);
    setWindowTitle(tr("Configure"));
    setObjectName(tr("configure"));

    QPixmap img = QPixmap(":/vncserver/resource/skin/product.png").scaledToHeight(64, Qt::SmoothTransformation);
    ui->image->setPixmap(img);
    ui->image->resize(img.width(), img.height());
    ui->image->setFixedSize(img.width(), img.height());

    ui->version->setText(QString("v%1").arg(WOVNCSERVER_VERSION));

    ui->port->setValidator(new QIntValidator(5900, 65535, this));

    QObject::connect(ui->btnPwdVisible, &QPushButton::clicked, this, [=](){
       QLineEdit::EchoMode mode = ui->password->echoMode();
       if(mode == QLineEdit::Password) {
           ui->password->setEchoMode(QLineEdit::Normal);
       }else {
           ui->password->setEchoMode(QLineEdit::Password);
       }
    });

    QKxGuiApplication *app = qobject_cast<QKxGuiApplication*>(QKxGuiApplication::instance());
    QObject::connect(ui->btnApply, &QPushButton::clicked, this, &QKxSystemConfig::onApplyConfig);
    QObject::connect(ui->btnMore, &QPushButton::clicked, this, &QKxSystemConfig::onMoreConfigure);
    QObject::connect(ui->btnStart, SIGNAL(clicked(bool)), this, SLOT(onStartService()));
    QObject::connect(ui->btnStop, SIGNAL(clicked(bool)), this, SLOT(onStopService()));
    QObject::connect(ui->btnPreview, SIGNAL(clicked()), this, SLOT(onPreviewService()));

    quint32 port = QKxSetting::listenPort();
    ui->port->setText(QString("%1").arg(port));
    QString pwd = QKxSetting::authorizePassword();
    ui->password->setText(pwd);

    QComboBox *language = ui->language;
    m_langs = QKxSetting::allLanguages();
    ui->language->setModel(new QStringListModel(m_langs.keys(), this));

    QString langFile = QKxSetting::languageFile();
    QString langKey = m_langs.key(langFile);
    ui->language->setCurrentText(langKey);

    bool brun = QKxGuiApplication::instance()->isServiceRunning();
    ui->btnStart->setVisible(!brun);
    ui->btnStop->setVisible(brun);

    m_listener = new QKxScreenListener(this);
    QObject::connect(m_listener, SIGNAL(screenChanged()), this, SLOT(onScreenChanged()));

    resetScreenChoose();

    QPointer<QKxSystemConfig> that = this;
    QTimer::singleShot(0, this, [=](){
        if(that) {
            adjustSize();
        }
    });
}

QKxSystemConfig::~QKxSystemConfig()
{
    delete ui;
}


void QKxSystemConfig::onApplyConfig()
{
    int err = tryToSaveConfigure();
    bool bRun = QKxGuiApplication::instance()->isServiceRunning();
    if(bRun && err > 0) {
        if(QMessageBox::warning(this, tr("Warning"),
                             tr("The service configuration has been modified. You need to restart the service to make it effective."),
                             QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes){
            emit apply();
        }
    }
    close();
}

void QKxSystemConfig::onStartService()
{
    if(tryToSaveConfigure() < 0) {
        return;
    }
    ui->btnStart->setEnabled(false);
    QMetaObject::invokeMethod(QKxGuiApplication::instance(), "onStartService", Qt::QueuedConnection);
}

void QKxSystemConfig::onStopService()
{
    ui->btnStop->setEnabled(false);
    QMetaObject::invokeMethod(QKxGuiApplication::instance(), "onStopService", Qt::QueuedConnection);
}

void QKxSystemConfig::onPreviewService()
{
    QMetaObject::invokeMethod(QKxGuiApplication::instance(), "onServicePreview", Qt::QueuedConnection);
}

void QKxSystemConfig::onScreenChanged()
{
    resetScreenChoose();
}

void QKxSystemConfig::onMoreConfigure()
{
    QKxMoreDialog dlg(this);
    dlg.exec();
}

void QKxSystemConfig::onSyncCaptureStatus()
{
    m_syncCountLeft--;
    if(m_syncCountLeft < 0) {
        m_syncTimer->stop();
        m_syncTimer->deleteLater();
    }
    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(true);

    bool bRunLast = QKxSetting::lastServiceStatus();
    bool bRun = QKxGuiApplication::instance()->isServiceRunning();
    ui->btnStart->setVisible(!bRun);
    ui->btnStop->setVisible(bRun);
    if(bRun != bRunLast) {
        QKxSetting::setLastServiceStatus(bRun);
    }
}

void QKxSystemConfig::resetScreenChoose()
{
    QComboBox *box = ui->screen;
    box->clear();
    int cnt = m_listener->screenCount();
    if(cnt == 1) {
        box->addItem(tr("Primary Screen"), 0);
        box->setCurrentIndex(0);
    }else if(cnt == 2) {
        box->addItem(tr("All Screen"), -1);
        box->addItem(tr("Primary Screen"), 0);
        box->addItem(tr("Second Screen"), 1);
    }else if(cnt > 2){
        box->addItem(tr("All Screen"), -1);
        box->addItem(tr("Primary Screen"), 0);
        for(int i = 1; i < cnt; i++) {
            box->addItem(tr("Second Screen")+QString::number(i), i);
        }
    }else {
        return;
    }

    qint32 idx = QKxSetting::screenIndex();
    if(idx < 0 || cnt == 1) {
        box->setCurrentIndex(0);
    }else{
        idx += 1;
        if(idx > cnt) {
            idx = 1;
        }
        box->setCurrentIndex(idx);
    }
}

int QKxSystemConfig::tryToSaveConfigure()
{
    QString lang = ui->language->currentText();
    QString langFile = m_langs.value(lang);
    if(langFile.isEmpty()) {
        return -1;
    }
    if(QKxSetting::languageFile() != langFile) {
        QKxSetting::setLanguageFile(langFile);
        QMetaObject::invokeMethod(QCoreApplication::instance(), "onLanguageSwitch", Qt::QueuedConnection);
    }

    QString pwd = ui->password->text();
    if(pwd.isEmpty()) {
        QMessageBox::information(this, tr("Information"), tr("The password can't be empty."));
        return -1;
    }
    quint16 port = static_cast<int>(ui->port->text().toInt());
    if(port < 5900 || port >= 65535) {
        QMessageBox::information(this, tr("Information"), tr("The port should be range between 5900 and 65535."));
        return -1;
    }
    int index = ui->screen->currentData().toInt();
    if(port != QKxSetting::listenPort()
            || pwd != QKxSetting::authorizePassword()
            || index != QKxSetting::screenIndex()) {
        bool bRun = QKxGuiApplication::instance()->isServiceRunning();
        QKxSetting::setListenPort(port);
        QKxSetting::setAuthorizePassword(pwd);
        QKxSetting::setScreenIndex(index);

        QKxCapOption::instance()->setScreenIndex(index);
        QKxCapOption::instance()->setAuthorizePassword(pwd);
        return 1;
    }
    return 0;
}

void QKxSystemConfig::syncCaptureStatus()
{
    if(m_syncTimer) {
        m_syncTimer->deleteLater();
    }
    m_syncTimer = new QTimer(this);
    QObject::connect(m_syncTimer, SIGNAL(timeout()), this, SLOT(onSyncCaptureStatus()));
    m_syncTimer->start(500);
    m_syncCountLeft = 30;
}

void QKxSystemConfig::closeEvent(QCloseEvent *e)
{
    QDialog *blockChild = findChild<QDialog*>();
    if(blockChild) {
        if(blockChild->close()){
            QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
        }
        e->setAccepted(false);
        return;
    }
    QMainWindow::closeEvent(e);
    deleteLater();
}

void QKxSystemConfig::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);
    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(true);
    QKxUtils::setVisibleOnDock(true);
}

void QKxSystemConfig::hideEvent(QHideEvent *e)
{
    QMainWindow::hideEvent(e);
    QKxUtils::setVisibleOnDock(false);
}
