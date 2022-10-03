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

#include "qkxmoredialog.h"
#include "ui_qkxmoredialog.h"
#include "qkxcapoption.h"
#include "qkxsetting.h"

#include <QTimer>

QKxMoreDialog::QKxMoreDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QKxMoreDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("More Configure"));

    QObject::connect(ui->btnApply, SIGNAL(clicked(bool)), this, SLOT(onButtonApply()));
    QObject::connect(ui->btnCancel, SIGNAL(clicked(bool)), this, SLOT(close()));

    bool drag = QKxSetting::dragWithContent();
    ui->chkDrag->setChecked(drag);
    bool lock = QKxSetting::autoLockScreen();
    ui->chkLock->setChecked(lock);
    bool black = QKxSetting::blackWallpaper();
    ui->chkBlack->setChecked(black);

    QPointer<QKxMoreDialog> that = this;
    QTimer::singleShot(0, this, [=](){
        if(that) {
            adjustSize();
        }
    });
}

QKxMoreDialog::~QKxMoreDialog()
{
    delete ui;
}

void QKxMoreDialog::onButtonApply()
{
    bool lock = ui->chkLock->isChecked();
    bool drag = ui->chkDrag->isChecked();
    bool black = ui->chkBlack->isChecked();
    QKxSetting::setAutoLockScreen(lock);
    QKxSetting::setDragWithContent(drag);
    QKxSetting::setBlackWallpaper(black);

    QKxCapOption::instance()->setBlackWallpaper(black);
    QKxCapOption::instance()->setDragWithContent(drag);
    QKxCapOption::instance()->setAutoLockScreen(lock);
    close();
}
