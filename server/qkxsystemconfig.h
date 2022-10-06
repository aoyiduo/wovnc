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

#ifndef QKXSYSTEMCONFIG_H
#define QKXSYSTEMCONFIG_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QJsonObject>
#include <QMap>
#include <QPointer>

namespace Ui {
class QKxSystemConfig;
}

class QKxScreenListener;
class QKxSystemConfig : public QMainWindow
{
    Q_OBJECT

public:
    explicit QKxSystemConfig(QWidget *parent = 0);
    ~QKxSystemConfig();

signals:
    void apply();

public slots:
    void onApplyConfig();
    void onStartService();
    void onStopService();
    void onPreviewService();

    void onScreenChanged();
    void onMoreConfigure();

    void onSyncCaptureStatus();
private:
    void resetScreenChoose();
    int tryToSaveConfigure();
    Q_INVOKABLE void syncCaptureStatus();
private:
    void closeEvent(QCloseEvent *e);
    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent *e);
private:
    Ui::QKxSystemConfig *ui;
    QMap<QString, QString> m_langs;
    QKxScreenListener *listener;
    QPointer<QKxScreenListener> m_listener;
    QPointer<QTimer> m_syncTimer;
    int m_syncCountLeft;
};

#endif // QKXSYSTEMCONFIG_H
