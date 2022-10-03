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

#include "qkxtcpvncserver.h"
#include "qkxutils.h"
#include "qkxvncserverpeer.h"
#include "qkxscreenlistener.h"
#include "qkxsharememory.h"
#include "qkxcapoption.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QProcess>
#include <QCoreApplication>

#define LOCAL_DEFAULT_HOST      ("127.0.0.1")

QKxTcpVNCServer::QKxTcpVNCServer(QObject *parent)
    : QObject(parent)
{
    m_openServer = new QTcpServer(this);
    QObject::connect(m_openServer, SIGNAL(newConnection()), this, SLOT(onOpenNewConnection()));

    m_localServer = new QTcpServer(this);
    QObject::connect(m_localServer, SIGNAL(newConnection()), this, SLOT(onLocalNewConnection()));
    QByteArray id = "vnc:" + QByteArray::number(QCoreApplication::applicationPid());
    m_localServer->listen(QHostAddress(LOCAL_DEFAULT_HOST), 0);
    quint16 port = m_localServer->serverPort();
    qInfo() << "TcpVNCServer internal listen address:" << port;
    QKxScreenListener *listener = new QKxScreenListener(this);
    QObject::connect(listener, SIGNAL(screenChanged()), this, SLOT(onScreenChanged()));    

    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));
}

QKxTcpVNCServer::~QKxTcpVNCServer()
{
    stop();
}

quint16 QKxTcpVNCServer::start(const QString &host, quint16 port)
{
    QHostAddress addr;
    if(host.isEmpty() || host == "0.0.0.0") {
        addr.setAddress(QHostAddress::Any);
    }else{
        addr.setAddress(host);
    }
    if(!m_openServer->listen(addr, port)) {
        qWarning() << "failed to listen server:" << m_openServer->errorString();
        return 0;
    }
    port = m_openServer->serverPort();
    qInfo() << "QKxTcpVNCServer open listen address:" << addr.toString() << port;
    QString hp = QString("%1:%2").arg(host).arg(port);
    qputenv("VNC_SERVER_ADDRESS", hp.toLatin1());
    QString url = QString("tcp://%1:%2").arg(host).arg(port);
    QKxCapOption::instance()->setServerUrl(url);
    return port;
}

int QKxTcpVNCServer::screenIndex()
{
    return QKxCapOption::instance()->screenIndex();
}

void QKxTcpVNCServer::setScreenIndex(int idx)
{
    QKxCapOption::instance()->setScreenIndex(idx);
}

QString QKxTcpVNCServer::authorizePassword()
{
    return QKxCapOption::instance()->authorizePassword();
}

void QKxTcpVNCServer::setAuthorizePassword(const QString &pwd)
{
    QKxCapOption::instance()->setAuthorizePassword(pwd);
}

int QKxTcpVNCServer::maxFPS() const
{
    return QKxCapOption::instance()->maxFPS();
}

void QKxTcpVNCServer::setMaxFPS(int fps)
{
    QKxCapOption::instance()->setMaxFPS(fps);
}

void QKxTcpVNCServer::setEmptyFrameEnable(bool ok)
{
    QKxCapOption::instance()->setEmptyFrameEnable(ok);
}

void QKxTcpVNCServer::stop()
{
    if(m_openServer){
        m_openServer->close();
    }
    if(m_localServer) {
        m_localServer->close();
    }
    stopAllClient();
}

void QKxTcpVNCServer::onOpenNewConnection()
{
    static int idx = 0;
    while(m_openServer->hasPendingConnections()) {
        QTcpSocket *client = m_openServer->nextPendingConnection();
        QObject::connect(client, &QTcpSocket::disconnected, this, &QKxTcpVNCServer::onOpenDisconnected);
        QObject::connect(client, &QTcpSocket::readyRead, this, &QKxTcpVNCServer::onReadyRead);
        m_ready.append(client);
        client->setObjectName(QString("TcpSocket:%1").arg(idx++));
        createPeer(false);
        //createPeer(true);
    }
}

void QKxTcpVNCServer::onLocalNewConnection()
{
    static int idx = 0;
    while(m_localServer->hasPendingConnections()) {
        QTcpSocket *local = m_localServer->nextPendingConnection();
        QObject::connect(local, &QTcpSocket::disconnected, this, &QKxTcpVNCServer::onLocalDisconnected);
        QObject::connect(local, &QTcpSocket::readyRead, this, &QKxTcpVNCServer::onReadyRead);
        if(m_ready.isEmpty()) {
            local->close();
            local->deleteLater();
            continue;
        }
        local->setObjectName(QString("LocalSocket:%1").arg(idx++));
        QIODevice *tmp = m_ready.takeFirst();
        m_pairs.insert(tmp, local);
        m_pairs.insert(local, tmp);
        qDebug() << "onLocalNewConnection" << tmp << local << m_pairs.count();
    }
}

void QKxTcpVNCServer::onOpenDisconnected()
{
    QTcpSocket *tmp = qobject_cast<QTcpSocket*>(sender());
    if(m_ready.removeOne(tmp)) {
        tmp->deleteLater();
        return;
    }
    tmp->flush();
    tmp->close();
    tmp->deleteLater();
    QTcpSocket *peer = qobject_cast<QTcpSocket*>(m_pairs.take(tmp));
    if(peer) {
        peer->flush();
        peer->close();
        peer->deleteLater();
    }
    qDebug() << "onTcpDisconnected" << tmp << peer << m_pairs.count();
}

void QKxTcpVNCServer::onLocalDisconnected()
{
    QTcpSocket *tmp = qobject_cast<QTcpSocket*>(sender());
    tmp->flush();
    tmp->close();
    tmp->deleteLater();
    QTcpSocket *peer = qobject_cast<QTcpSocket*>(m_pairs.take(tmp));
    if(peer) {
        peer->flush();
        peer->close();
        peer->deleteLater();
    }
    qDebug() << "onLocalDisconnected" << tmp << peer << m_pairs.count();
}

void QKxTcpVNCServer::onReadyRead()
{
    QIODevice *tmp = qobject_cast<QIODevice*>(sender());
    QIODevice *peer = m_pairs.value(tmp);    
    if(peer && tmp){
        QByteArray all = tmp->readAll();
        //qDebug() << "onReadyRead" << tmp->objectName() << all.length() << all.mid(0, 16);
        peer->write(all);
    }
}

void QKxTcpVNCServer::onAboutToQuit()
{
    stop();
}

void QKxTcpVNCServer::onScreenChanged()
{
    stopAllClient();
}

void QKxTcpVNCServer::onVncProcessFinish()
{
    QProcess *p = qobject_cast<QProcess*>(sender());
    QProcess::ProcessError err = p->error();
    QString errTxt = p->errorString();
    qDebug() << "onVncProcessFinish" << err << errTxt;
    m_vncs.removeOne(p);
    bool lock = QKxCapOption::instance()->autoLockScreen();
    if(m_vncs.isEmpty() && lock) {
        QKxUtils::lockWorkStation();
    }
}

void QKxTcpVNCServer::createPeer(bool bInner)
{
    QString host = m_localServer->serverAddress().toString();
    quint16 port = m_localServer->serverPort();
    QString svcName = QString("tcp:%1:%2").arg(host).arg(port);
    if(bInner) {
        qputenv("RPOXY_SERVER_NAME", svcName.toLatin1());
        QKxVNCServerPeer *peer = new QKxVNCServerPeer(this);
        QObject::connect(peer, SIGNAL(errorArrived(int)), peer, SLOT(deleteLater()));
    }else{
        QProcess* peer = new QProcess(this);
        m_vncs.append(peer);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("RPOXY_SERVER_NAME", svcName);
        env.insert("RUN_ACTION_NAME", QString("vnc:%1").arg(QCoreApplication::applicationPid()));
        peer->setProcessEnvironment(env);

        peer->setProgram(QCoreApplication::applicationFilePath());
        peer->start();
        QObject::connect(peer, SIGNAL(finished(int)), peer, SLOT(deleteLater()));
        QObject::connect(peer, SIGNAL(finished(int)), this, SLOT(onVncProcessFinish()));
        QObject::connect(peer, SIGNAL(errorOccurred(QProcess::ProcessError)), peer, SLOT(deleteLater()));
        QObject::connect(peer, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(onVncProcessFinish()));
    }
}

void QKxTcpVNCServer::stopAllClient()
{
    QList<QPointer<QIODevice> > ready = m_ready;
    for(int i = 0; i < ready.length(); i++) {
        QIODevice *dev = ready[i];
        dev->close();
    }
    QMap<QPointer<QIODevice>, QPointer<QIODevice> > pairs = m_pairs;
    for(auto it = pairs.begin(); it != pairs.end(); it++) {
        QIODevice *dev = it.key();
        dev->close();
    }
}
