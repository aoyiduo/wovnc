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

#include "qkxvncviewer.h"
#include "ui_qkxvncviewer.h"
#include "qkxconnectiondialog.h"

#include "qkxvncwidget.h"

#include <QLabel>
#include <QResizeEvent>
#include <QDebug>
#include <QCoreApplication>
#include <QMessageBox>

QKxVNCViewer::QKxVNCViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QKxVNCViewer)
{
    ui->setupUi(this);
    setWindowTitle(tr("WoVNCViewer"));

    QLabel *woterm = new QLabel(ui->statusbar);
    woterm->setTextFormat(Qt::RichText);
    woterm->setOpenExternalLinks(true);
    woterm->setTextInteractionFlags(Qt::TextBrowserInteraction);
    QString suggest = tr("It is recommended to use the");
    suggest += " <a href=\"http://woterm.com\">WoTerm</a> ";
    suggest += tr("or");
    suggest += " <a href=\"http://feidesk.com\">FeiDesk</a> ";
    suggest += tr("tool to enable the full feathers");
    woterm->setText(suggest);
    ui->statusbar->addWidget(woterm);
    QObject::connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(onActionNewConnection()));

    m_vnc = new QKxVncWidget(this);
    setCentralWidget(m_vnc);
    QObject::connect(m_vnc, SIGNAL(finished()), this, SLOT(onVncFinished()));
    QObject::connect(m_vnc, SIGNAL(connectionStart()), this, SLOT(onVncConnectionStart()));
    QObject::connect(m_vnc, SIGNAL(connectionFinished(bool)), this, SLOT(onVncConnectionFinished(bool)));
    QObject::connect(m_vnc, SIGNAL(errorArrived(QByteArray)), this, SLOT(onVncErrorArrived(QByteArray)));
    QObject::connect(m_vnc, SIGNAL(passwordResult(QByteArray,bool)), this, SLOT(onVncPasswordResult(QByteArray,bool)));

    QMetaObject::invokeMethod(this, "onAppStart", Qt::QueuedConnection);
}

QKxVNCViewer::~QKxVNCViewer()
{
    delete ui;
}

void QKxVNCViewer::showInput(const QString& msg)
{
    if(m_dlgConnect != nullptr){
        return;
    }
    QKxConnectionDialog dlg(this);
    dlg.setErrorMessage(msg);
    m_dlgConnect = &dlg;

    if(dlg.exec() == dlg.Accepted) {
        QString server = dlg.serverName();
        QString pass = dlg.password();
        int port = dlg.port();
        reconnect(server, port, pass, false);
    }
}

void QKxVNCViewer::onActionNewConnection()
{
    showInput();
}

void QKxVNCViewer::onAppStart()
{
    QStringList args = QCoreApplication::arguments();
    QString host, pass;
    int port = 0;
    bool viewOnly = true;
    for(int i = 0; i < args.length(); i++) {
        QString arg = args.at(i);
        if(arg.startsWith("--host=")) {
            host = arg.mid(7);
        }else if(arg.startsWith("--port=")) {
            QString tmp = arg.mid(7);
            port = tmp.toInt();
        }else if(arg.startsWith("--pass=")) {
            pass = arg.mid(7);
        }else if(arg.startsWith("--viewOnly=")) {
            QVariant tmp = arg.mid(11);
            viewOnly = tmp.toBool();
        }
    }
    //QMessageBox::information(this, "prameter", args.join(","));
    if(host.isEmpty() || pass.isEmpty() || port == 0) {
        showInput();
    }else{
        reconnect(host, port, pass, viewOnly);
    }
}

void QKxVNCViewer::onVncFinished()
{
    qDebug() << "onVncFinished";
    QMetaObject::invokeMethod(this, "showInput", Qt::QueuedConnection, Q_ARG(QString, "Failed to connect server."));
}

void QKxVNCViewer::onVncConnectionStart()
{
    qDebug() << "onVncConnectionStart";
}

void QKxVNCViewer::onVncConnectionFinished(bool noErr)
{
    qDebug() << "onVncConnectionFinished";
    if(!noErr) {
        QMetaObject::invokeMethod(this, "showInput", Qt::QueuedConnection, Q_ARG(QString, "Failed to connect server."));
    }
}

void QKxVNCViewer::onVncErrorArrived(const QByteArray &msg)
{
    qDebug() << "onVncErrorArrived";
}

void QKxVNCViewer::onVncPasswordResult(const QByteArray &msg, bool isSuccess)
{
    qDebug() << "onVncPasswordResult";
}

void QKxVNCViewer::reconnect(const QString &host, int port, const QString &pass, bool viewOnly)
{
    QVector<QKxVNC::EEncodingType> encs;
    encs << QKxVNC::JPEG
         << QKxVNC::OpenH264
         << QKxVNC::ZRLE
         << QKxVNC::TRLE
         << QKxVNC::ZRLE2
         << QKxVNC::TRLE2
         << QKxVNC::ZRLE3
         << QKxVNC::TRLE3
         << QKxVNC::Hextile
         << QKxVNC::RRE
         << QKxVNC::CopyRect
         << QKxVNC::Raw
         << QKxVNC::DesktopSizePseudoEncoding;
    m_vnc->start(host, port, pass, QKxVNC::RGB32_888, QKxVNC::RFB_38, encs);
    m_vnc->setViewOnly(viewOnly);
}
