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

#ifndef QKXCONNECTIONDIALOG_H
#define QKXCONNECTIONDIALOG_H

#include <QDialog>

namespace Ui {
class QKxConnectionDialog;
}

class QKxConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QKxConnectionDialog(QWidget *parent = nullptr);
    ~QKxConnectionDialog();
    QString serverName() const;
    int port() const;
    QString password() const;

    void setErrorMessage(const QString& msg);
private slots:
    void onButtonConnectClicked();
    void onAdjustSize();
private:
    Ui::QKxConnectionDialog *ui;
};

#endif // QKXCONNECTIONDIALOG_H
