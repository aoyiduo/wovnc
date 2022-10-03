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

#ifndef QKXAUDIOCAPTURE_H
#define QKXAUDIOCAPTURE_H

#include <QPointer>
#include <QThread>
#include <QMutex>
#include <QList>
#include <QAtomicInt>


class QKxAudioCapture : public QThread
{
    Q_OBJECT
private:
     struct MyMsg {
         uchar type;
         QByteArray data;
     };
public:
    explicit QKxAudioCapture(QObject *parent = nullptr);
    virtual ~QKxAudioCapture();
    void stop();
signals:
    void result(const QByteArray& buf);
    void internalAssemble(const QByteArray& buf);
private slots:
    void onRequest(int cnt);
    void onInternalAssemble(const QByteArray& buf);
protected:
    void push(uchar type, const QByteArray& data = QByteArray());
    bool pop(uchar &type, QByteArray& data);
    bool takeOne(uchar type, QByteArray& data);
    void popAll();
    void run();
    bool init();
protected:
    virtual int running();
    virtual int findDefaultLoopbackDevice() = 0;
protected:
    QList<MyMsg> m_queue;
    QMutex m_mtx;
    int m_msgRead;
    int m_msgWrite;

    qint64 m_timeLast;
};

#endif // QKXAUDIOCAPTURE_H
