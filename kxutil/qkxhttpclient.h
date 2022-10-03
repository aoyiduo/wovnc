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

#ifndef QKXHTTPCLIENT_H
#define QKXHTTPCLIENT_H

#include "qkxutil_share.h"

#include <QPointer>
#include <QThread>

class KXUTIL_EXPORT QKxHttpClient : public QThread
{
    Q_OBJECT
public:
    explicit QKxHttpClient(QObject *parent = nullptr);
    ~QKxHttpClient();

    bool get(const QString& url);
    bool post(const QString& url, const QByteArray& data);
signals:
    void result(int code, const QByteArray& body);
private slots:
    void onResult(int code, const QByteArray& body=QByteArray());
private:
    void run();
    bool fetch(QByteArray& hdr, QByteArray& buffer);
private:
    enum EType {
        HC_GET = 0,
        HC_POST = 1,
        HC_PUT = 2
    };
    QString m_url;
    QByteArray m_data;
    EType m_type;
};
#endif // QKXHTTPCLIENT_H
