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

#ifndef QKXVNCVIEWER_H
#define QKXVNCVIEWER_H

#include <QMainWindow>
#include <QPointer>

namespace Ui {
class QKxVNCViewer;
}

class QKxConnectionDialog;
class QKxVncWidget;
class QKxVNCViewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit QKxVNCViewer(QWidget *parent = nullptr);
    ~QKxVNCViewer();
    Q_INVOKABLE void showInput(const QString& msg=QString());
private slots:
    void onActionNewConnection();
    void onAppStart();

    // vnc
    void onVncFinished();
    void onVncConnectionStart();
    void onVncConnectionFinished(bool noErr);
    void onVncErrorArrived(const QByteArray& msg);
    void onVncPasswordResult(const QByteArray& msg, bool isSuccess);
private:
    void reconnect(const QString& host, int port, const QString& pass, bool viewOnly);
private:
    Ui::QKxVNCViewer *ui;
    QPointer<QKxVncWidget> m_vnc;
    QPointer<QKxConnectionDialog> m_dlgConnect;
};

#endif // QKXVNCVIEWER_H
