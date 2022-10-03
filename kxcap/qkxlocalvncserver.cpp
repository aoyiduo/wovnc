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

#include "qkxlocalvncserver.h"
#include "qkxutils.h"
#include "qkxvncserverpeer.h"
#include "qkxscreenlistener.h"
#include "qkxsharememory.h"
#include "qkxcapoption.h"

#include <QTcpServer>
#include <QLocalSocket>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QCoreApplication>

QKxLocalVNCServer::QKxLocalVNCServer(QObject *parent)
    : QObject(parent)
{
    m_openServer = new QLocalServer(this);
    m_openServer->setSocketOptions(QLocalServer::WorldAccessOption);

    QObject::connect(m_openServer, SIGNAL(newConnection()), this, SLOT(onOpenNewConnection()));

    m_localServer = new QLocalServer(this);
    m_localServer->setSocketOptions(QLocalServer::WorldAccessOption);
    QObject::connect(m_localServer, SIGNAL(newConnection()), this, SLOT(onLocalNewConnection()));
    QString svcName = QString("ProxyServer:%1").arg(QCoreApplication::applicationPid());
    if(!m_localServer->listen(svcName)) {
        QString msg = QString("never come here:%1").arg(svcName);
        qWarning() << msg;
        throw msg;
    }

    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));

    QKxScreenListener *listener = new QKxScreenListener(this);
    QObject::connect(listener, SIGNAL(screenChanged()), this, SLOT(onScreenChanged()));
}

QKxLocalVNCServer::~QKxLocalVNCServer()
{
    stop();
}

bool QKxLocalVNCServer::start(const QString &name)
{
    QLocalServer::removeServer(name);
    if(!m_openServer->listen(name)){
        QString msg = QString("failed to listen:%2, QLocalServer:%1").arg(name).arg(m_openServer->errorString());
        qWarning() << msg;
        return false;
    }
    return true;
}

int QKxLocalVNCServer::screenIndex()
{
    return QKxCapOption::instance()->screenIndex();
}

void QKxLocalVNCServer::setScreenIndex(int idx)
{
    QKxCapOption::instance()->setScreenIndex(idx);
}

QString QKxLocalVNCServer::authorizePassword()
{
    return QKxCapOption::instance()->authorizePassword();
}

void QKxLocalVNCServer::setAuthorizePassword(const QString &pwd)
{
    QKxCapOption::instance()->setAuthorizePassword(pwd);
}

int QKxLocalVNCServer::maxFPS() const
{
    return QKxCapOption::instance()->maxFPS();
}

void QKxLocalVNCServer::setMaxFPS(int fps)
{
    QKxCapOption::instance()->setMaxFPS(fps);
}

void QKxLocalVNCServer::setEmptyFrameEnable(bool ok)
{
    QKxCapOption::instance()->setEmptyFrameEnable(ok);
}

void QKxLocalVNCServer::stop()
{
    if(m_openServer){
        m_openServer->close();
    }
    if(m_localServer) {
        m_localServer->close();
    }
    stopAllClient();
}

void QKxLocalVNCServer::onOpenNewConnection()
{
    static int idx = 0;
    while(m_openServer->hasPendingConnections()) {
        QLocalSocket *client = m_openServer->nextPendingConnection();
        QObject::connect(client, &QLocalSocket::disconnected, this, &QKxLocalVNCServer::onOpenDisconnected);
        QObject::connect(client, &QLocalSocket::readyRead, this, &QKxLocalVNCServer::onReadyRead);
        m_ready.append(client);
        client->setObjectName(QString("OpenSocket:%1").arg(idx++));
        createPeer(false);
        //createPeer(true);
    }
}

void QKxLocalVNCServer::onLocalNewConnection()
{
    static int idx = 0;
    while(m_localServer->hasPendingConnections()) {
        QLocalSocket *local = m_localServer->nextPendingConnection();
        QObject::connect(local, &QLocalSocket::disconnected, this, &QKxLocalVNCServer::onLocalDisconnected);
        QObject::connect(local, &QLocalSocket::readyRead, this, &QKxLocalVNCServer::onReadyRead);
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

void QKxLocalVNCServer::onOpenDisconnected()
{
    QLocalSocket *tmp = qobject_cast<QLocalSocket*>(sender());
    if(m_ready.removeOne(tmp)) {
        tmp->deleteLater();
        return;
    }
    tmp->flush();
    tmp->close();
    tmp->deleteLater();
    QLocalSocket *peer = qobject_cast<QLocalSocket*>(m_pairs.take(tmp));
    if(peer) {
        peer->flush();
        peer->close();
        peer->deleteLater();
    }
    qDebug() << "onTcpDisconnected" << tmp << peer << m_pairs.count();
}

void QKxLocalVNCServer::onLocalDisconnected()
{
    QLocalSocket *tmp = qobject_cast<QLocalSocket*>(sender());
    tmp->flush();
    tmp->close();
    tmp->deleteLater();
    QLocalSocket *peer = qobject_cast<QLocalSocket*>(m_pairs.take(tmp));
    if(peer) {
        peer->flush();
        peer->close();
        peer->deleteLater();
    }
    qDebug() << "onLocalDisconnected" << tmp << peer << m_pairs.count();
}

void QKxLocalVNCServer::onReadyRead()
{
    QIODevice *tmp = qobject_cast<QIODevice*>(sender());
    QIODevice *peer = m_pairs.value(tmp);    
    if(peer && tmp){
        QByteArray all = tmp->readAll();
        //qDebug() << "onReadyRead" << tmp->objectName() << all.length() << all.mid(0, 16);
        peer->write(all);
    }
}

void QKxLocalVNCServer::onAboutToQuit()
{
    stop();
}

void QKxLocalVNCServer::onScreenChanged()
{
    stopAllClient();
}

void QKxLocalVNCServer::onVncProcessFinish()
{
    QProcess *p = qobject_cast<QProcess*>(sender());
    QProcess::ProcessError err = p->error();
    QString errTxt = p->errorString();
    qDebug() << "onVncProcessFinish" << err << errTxt;
    m_vncs.removeOne(p);
    if(m_vncs.isEmpty()) {
        QKxUtils::lockWorkStation();
    }
}

void QKxLocalVNCServer::createPeer(bool bInner)
{
    QString svcName = "local:" + m_localServer->serverName();
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

void QKxLocalVNCServer::stopAllClient()
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
