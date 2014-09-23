/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "session.h"
#include <QApplication>
#include <IrcCommand>
#include <IrcMessage>
#include <QNetworkConfigurationManager>
#include <Irc>
#ifndef QT_NO_OPENSSL
#include <QSslSocket>
#endif // QT_NO_OPENSSL

QNetworkSession* Session::s_network = 0;

Session::Session(QObject *parent) : IrcSession(parent),
    m_currentLag(-1), m_maxLag(120000), m_quit(false)
{
    connect(this, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(this, SIGNAL(password(QString*)), this, SLOT(onPassword(QString*)));
    connect(this, SIGNAL(capabilities(QStringList,QStringList*)), this, SLOT(onCapabilities(QStringList,QStringList*)));
    connect(this, SIGNAL(messageReceived(IrcMessage*)), SLOT(handleMessage(IrcMessage*)));

    setAutoReconnectDelay(15);
    connect(&m_reconnectTimer, SIGNAL(timeout()), this, SLOT(open()));

    connect(&m_pingTimer, SIGNAL(timeout()), this, SLOT(pingServer()));
    connect(this, SIGNAL(connectedChanged(bool)), SLOT(togglePingTimer(bool)));
}

QString Session::name() const
{
    return m_name;
}

void Session::setName(const QString& name)
{
    if (m_name != name)
    {
        m_name = name;
        emit nameChanged(name);
    }
}

QString Session::network() const
{
    return m_info.value("NETWORK");
}

int Session::autoReconnectDelay() const
{
    return m_reconnectTimer.interval() / 1000;
}

void Session::setAutoReconnectDelay(int delay)
{
    m_reconnectTimer.setInterval(delay * 1000);
}

ChannelInfos Session::channels() const
{
    return m_channels;
}

void Session::addChannel(const QString& channel)
{
    const QString lower = channel.toLower();
    foreach (const ChannelInfo& info, m_channels)
        if (info.channel.toLower() == lower)
            return;

    ChannelInfo info;
    info.channel = channel;
    m_channels.append(info);
}

void Session::setChannelKey(const QString& channel, const QString& key)
{
    int idx = -1;
    const QString lower = channel.toLower();
    for (int i = 0; idx == -1 && i < m_channels.count(); ++i)
        if (m_channels.at(i).channel.toLower() == lower)
            idx = i;
    if (idx != -1)
    {
        ChannelInfo info = m_channels.at(idx);
        info.key = key;
        m_channels.replace(idx, info);
    }
}

void Session::removeChannel(const QString& channel)
{
    int idx = -1;
    const QString lower = channel.toLower();
    for (int i = 0; idx == -1 && i < m_channels.count(); ++i)
        if (m_channels.at(i).channel.toLower() == lower)
            idx = i;
    if (idx != -1)
        m_channels.removeAt(idx);
}

void Session::setChannels(const ChannelInfos& channels)
{
    m_channels = channels;
}

QString Session::channelTypes() const
{
    return m_info.value("CHANTYPES", "#&");
}

bool Session::isChannel(const QString& receiver) const
{
    return receiver.length() > 1 && channelTypes().contains(receiver.at(0));
}

QString Session::prefixTypes() const
{
    QString pfx = m_info.value("PREFIX", "(ov)@+");
    return pfx.mid(1, pfx.indexOf(')') - 1);
}

QString Session::prefixModes() const
{
    QString pfx = m_info.value("PREFIX", "(ov)@+");
    return pfx.mid(pfx.indexOf(')') + 1);
}

QString Session::prefixTypeToMode(const QString& type) const
{
    int i = prefixTypes().indexOf(type);
    return prefixModes().mid(i, 1);
}

QString Session::userPrefix(const QString& user) const
{
    QString prefixes = prefixModes();
    int i = 0;
    while (i < user.length() && prefixes.contains(user.at(i)))
        ++i;
    return user.left(i);
}

QString Session::unprefixedUser(const QString& user) const
{
    QString prefixes = prefixModes();
    QString copy = user;
    while (!copy.isEmpty() && prefixes.contains(copy.at(0)))
        copy.remove(0, 1);
    IrcSender sender(copy);
    if (sender.isValid())
        copy = sender.name();
    return copy;
}

bool Session::isSecure() const
{
#ifdef QT_NO_OPENSSL
    return false;
#else
    return qobject_cast<QSslSocket*>(socket());
#endif // QT_NO_OPENSSL
}

void Session::setSecure(bool secure)
{
#ifdef QT_NO_OPENSSL
    Q_UNUSED(secure)
#else
    QSslSocket* sslSocket = qobject_cast<QSslSocket*>(socket());
    if (secure && !sslSocket)
    {
        sslSocket = new QSslSocket(this);
        sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
        sslSocket->ignoreSslErrors();
        setSocket(sslSocket);
    }
    else if (!secure && sslSocket)
    {
        setSocket(new QTcpSocket(this));
    }
#endif // QT_NO_OPENSSL
}

QString Session::password() const
{
    return m_password;
}

void Session::setPassword(const QString& password)
{
    m_password = password;
}

ConnectionInfo Session::toConnection() const
{
    ConnectionInfo connection;
    connection.name = name();
    connection.secure = isSecure();
    connection.host = host();
    connection.port = port();
    connection.user = userName();
    connection.nick = nickName();
    connection.real = realName();
    connection.pass = password();
    connection.channels = channels();
    connection.quit = m_quit;
    return connection;
}

void Session::initFrom(const ConnectionInfo& connection)
{
    setName(connection.name);
    setSecure(connection.secure);
    setPassword(connection.pass);
    setHost(connection.host);
    setPort(connection.port);
    setNickName(connection.nick);
    QString appName = QApplication::applicationName();
    setUserName(connection.user.isEmpty() ? appName : connection.user);
    setRealName(connection.real.isEmpty() ? appName : connection.real);
    setChannels(connection.channels);
    m_quit = connection.quit;
}

Session* Session::fromConnection(const ConnectionInfo& connection, QObject* parent)
{
    Session* session = new Session(parent);
    session->initFrom(connection);
    return session;
}

int Session::pingInterval() const
{
    return m_pingTimer.interval() / 1000;
}

void Session::setPingInterval(int interval)
{
    m_pingTimer.start(interval * 1000);
}

int Session::currentLag() const
{
    return m_currentLag;
}

int Session::maximumLag() const
{
    return m_maxLag;
}

void Session::setMaximumLag(int lag)
{
    m_maxLag = lag;
}

bool Session::hasQuit() const
{
    return m_quit;
}

bool Session::ensureNetwork()
{
    QNetworkConfigurationManager manager;
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired)
    {
        if (!s_network)
            s_network = new QNetworkSession(manager.defaultConfiguration(), qApp);
        s_network->open();
    }
    // TODO: return value?
    return true;
}

bool Session::sendUiCommand(IrcCommand* command)
{
    if (command->type() == IrcCommand::Join)
    {
        QString key = command->parameters().value(1);
        if (!key.isEmpty())
            setChannelKey(command->parameters().value(0), key);
    }
    return sendCommand(command);
}

void Session::reconnect()
{
    connect(this, SIGNAL(connecting()), &m_reconnectTimer, SLOT(stop()));
    connect(this, SIGNAL(socketError(QAbstractSocket::SocketError)), &m_reconnectTimer, SLOT(start()));

    if (ensureNetwork())
        open();
}

void Session::quit(const QString& reason)
{
    disconnect(this, SIGNAL(connecting()), &m_reconnectTimer, SLOT(stop()));
    disconnect(this, SIGNAL(socketError(QAbstractSocket::SocketError)), &m_reconnectTimer, SLOT(start()));

    QString message = reason;
    if (message.isEmpty())
        message = tr("%1 %2").arg(QApplication::applicationName())
                             .arg(QApplication::applicationVersion());

    if (isConnected())
        sendCommand(IrcCommand::createQuit(message));
    socket()->waitForDisconnected(1000);
    close();
    m_quit = true;
}

void Session::destructLater()
{
    if (isConnected())
    {
        connect(this, SIGNAL(disconnected()), SLOT(deleteLater()));
        connect(this, SIGNAL(socketError(QAbstractSocket::SocketError)), SLOT(deleteLater()));
        QTimer::singleShot(1000, this, SLOT(deleteLater()));
    }
    else
    {
        deleteLater();
    }
}

void Session::onConnected()
{
    foreach (const ChannelInfo& channel, m_channels)
    {
        if (!channel.channel.isEmpty())
            sendCommand(IrcCommand::createJoin(channel.channel, channel.key));
    }
    m_quit = false;
}

void Session::onPassword(QString* password)
{
    *password = m_password;
}

void Session::onCapabilities(const QStringList& available, QStringList* request)
{
    if (available.contains("identify-msg"))
        request->append("identify-msg");
}

void Session::handleMessage(IrcMessage* message)
{
    // 20s delay since the last message was received
    setPingInterval(20);

    if (message->type() == IrcMessage::Join)
    {
        if (message->isOwn())
            addChannel(static_cast<IrcJoinMessage*>(message)->channel());
    }
    else if (message->type() == IrcMessage::Part)
    {
        if (message->isOwn())
            removeChannel(static_cast<IrcPartMessage*>(message)->channel());
    }
    else if (message->type() == IrcMessage::Pong)
    {
        if (message->parameters().contains("_C_o_m_m_u_n_i_"))
        {
            // slow down to 60s intervals
            setPingInterval(60);

            updateLag(static_cast<int>(m_lagTimer.elapsed()));
            m_lagTimer.invalidate();
        }
    }
    else if (message->type() == IrcMessage::Numeric)
    {
        int code = static_cast<IrcNumericMessage*>(message)->code();
        if (code == Irc::RPL_ISUPPORT)
        {
            foreach (const QString& param, message->parameters().mid(1))
            {
                QStringList keyValue = param.split("=", QString::SkipEmptyParts);
                m_info.insert(keyValue.value(0), keyValue.value(1));
            }
            if (m_info.contains("NETWORK"))
                emit networkChanged(network());
            emit serverInfoReceived();
        }
        else if (code == Irc::ERR_NICKNAMEINUSE)
        {
            if (m_alternateNicks.isEmpty())
            {
                QString currentNick = nickName();
                m_alternateNicks << (currentNick + "_")
                                 <<  currentNick
                                 << (currentNick + "__")
                                 <<  currentNick;
            }
            setNickName(m_alternateNicks.takeFirst());
        }
    }
}

void Session::pingServer()
{
    if (m_lagTimer.isValid())
    {
        // still lagging (no response since last PING)
        updateLag(static_cast<int>(m_lagTimer.elapsed()));

        // decrease the interval (60s => 20s => 6s => 2s)
        int interval = pingInterval();
        if (interval >= 6)
            setPingInterval(interval / 3);
    }
    else
    {
        // (re-)PING!
        m_lagTimer.start();
        sendData("PING _C_o_m_m_u_n_i_");
    }
}

void Session::updateLag(int lag)
{
    if (m_currentLag != lag)
    {
        m_currentLag = lag;
        emit currentLagChanged(lag);

        // TODO: https://github.com/communi/communi/issues/6
//        if (lag > m_maxLag)
//        {
//            close();
//            if (ensureNetwork())
//                open();
//        }
    }
}

void Session::togglePingTimer(bool enabled)
{
    if (enabled)
    {
        m_lagTimer.invalidate();
        m_pingTimer.start();
        pingServer();
    }
    else
    {
        m_pingTimer.stop();
        updateLag(-1);
    }
}
