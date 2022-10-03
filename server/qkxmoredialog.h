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

#ifndef QKXMOREDIALOG_H
#define QKXMOREDIALOG_H

#include <QDialog>
#include <QPointer>

namespace Ui {
class QKxMoreDialog;
}

class QKxMoreDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QKxMoreDialog(QWidget *parent = 0);
    ~QKxMoreDialog();
private slots:
    void onButtonApply();
private:
    Ui::QKxMoreDialog *ui;
};

#endif // QKXMOREDIALOG_H
