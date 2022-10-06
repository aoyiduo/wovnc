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

#include "qkxconnectiondialog.h"
#include "ui_qkxconnectiondialog.h"

#include <QIntValidator>
#include <QMessageBox>
#include <QTimer>

QKxConnectionDialog::QKxConnectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QKxConnectionDialog)
{
    ui->setupUi(this);
    Qt::WindowFlags flags = windowFlags();
    setWindowFlags(flags &~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("New Connection"));
    ui->port->setValidator(new QIntValidator(1024, 65535, this));
    QObject::connect(ui->connect, SIGNAL(clicked()), this, SLOT(onButtonConnectClicked()));
    QTimer::singleShot(10, this, SLOT(onAdjustSize()));
    ui->err->setVisible(false);
}

QKxConnectionDialog::~QKxConnectionDialog()
{
    delete ui;
}

QString QKxConnectionDialog::serverName() const
{
    return ui->server->text();
}

int QKxConnectionDialog::port() const
{
    QString txt = ui->port->text();
    return txt.toInt();
}

QString QKxConnectionDialog::password() const
{
    return ui->password->text();
}

void QKxConnectionDialog::setErrorMessage(const QString &msg)
{
    ui->err->setText(msg);
    ui->err->setVisible(!msg.isEmpty());
}

void QKxConnectionDialog::onButtonConnectClicked()
{
    QString server=ui->server->text();
    QString pass = ui->password->text();
    QString port = ui->port->text();
    if(server.isEmpty()) {
        QMessageBox::information(this, tr("Bad Parameter"), tr("please fill the server address"));
        return;
    }
    if(pass.isEmpty()) {
        QMessageBox::information(this, tr("Bad Parameter"), tr("please fill the password"));
        return;
    }
    if(port.isEmpty()) {
        QMessageBox::information(this, tr("Bad Parameter"), tr("please fill the port"));
        return;
    }
    done(Accepted);
}

void QKxConnectionDialog::onAdjustSize()
{
    //adjustSize();
    QSize sz = minimumSize();
    resize(sz);
}
