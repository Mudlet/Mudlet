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

#ifndef SESSION_H
#define SESSION_H

#include <QSet>
#include <QTimer>
#include <IrcSession>
#include <IrcCommand>
#include <QStringList>
#include <QElapsedTimer>
#include <QAbstractSocket>
#include <QNetworkSession>
#include "connectioninfo.h"

class Session : public IrcSession
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString network READ network NOTIFY networkChanged)
    Q_PROPERTY(int autoReconnectDelay READ autoReconnectDelay WRITE setAutoReconnectDelay)
    Q_PROPERTY(ChannelInfos channels READ channels WRITE setChannels)
    Q_PROPERTY(QString channelTypes READ channelTypes NOTIFY serverInfoReceived)
    Q_PROPERTY(QString prefixTypes READ prefixTypes NOTIFY serverInfoReceived)
    Q_PROPERTY(QString prefixModes READ prefixModes NOTIFY serverInfoReceived)
    Q_PROPERTY(bool secure READ isSecure WRITE setSecure)
    Q_PROPERTY(QString password READ password WRITE setPassword)
    Q_PROPERTY(int pingInterval READ pingInterval WRITE setPingInterval)
    Q_PROPERTY(int currentLag READ currentLag NOTIFY currentLagChanged)
    Q_PROPERTY(int maximumLag READ maximumLag WRITE setMaximumLag)
    Q_PROPERTY(bool hasQuit READ hasQuit)

public:
    explicit Session(QObject *parent = 0);

    QString name() const;
    void setName(const QString& name);

    QString network() const;

    int autoReconnectDelay() const;
    void setAutoReconnectDelay(int delay);

    ChannelInfos channels() const;
    Q_INVOKABLE void addChannel(const QString& channel);
    Q_INVOKABLE void setChannelKey(const QString& channel, const QString& key);
    Q_INVOKABLE void removeChannel(const QString& channel);
    void setChannels(const ChannelInfos& channels);

    QString channelTypes() const;
    Q_INVOKABLE bool isChannel(const QString& receiver) const;

    QString prefixTypes() const;
    QString prefixModes() const;
    QString prefixTypeToMode(const QString& type) const;
    Q_INVOKABLE QString userPrefix(const QString& user) const;
    Q_INVOKABLE QString unprefixedUser(const QString& user) const;

    bool isSecure() const;
    void setSecure(bool secure);

    QString password() const;
    void setPassword(const QString& password);

    ConnectionInfo toConnection() const;
    void initFrom(const ConnectionInfo& connection);
    static Session* fromConnection(const ConnectionInfo& connection, QObject* parent = 0);

    int pingInterval() const;
    void setPingInterval(int interval);

    int currentLag() const;
    int maximumLag() const;
    void setMaximumLag(int lag);

    bool hasQuit() const;
    Q_INVOKABLE bool ensureNetwork();

    Q_INVOKABLE bool sendUiCommand(IrcCommand* command);

public slots:
    void reconnect();
    void quit(const QString& reason = QString());
    void destructLater();

signals:
    void nameChanged(const QString& name);
    void networkChanged(const QString& network);
    void currentLagChanged(int lag);
    void serverInfoReceived();

private slots:
    void onConnected();
    void onPassword(QString* password);
    void onCapabilities(const QStringList& available, QStringList* request);
    void handleMessage(IrcMessage* message);
    void togglePingTimer(bool enabled);
    void pingServer();

private:
    void updateLag(int lag);

    QString m_name;
    QTimer m_reconnectTimer;
    QString m_password;
    ChannelInfos m_channels;
    QElapsedTimer m_lagTimer;
    QTimer m_pingTimer;
    int m_currentLag;
    int m_maxLag;
    QHash<QString,QString> m_info;
    bool m_quit;
    QStringList m_alternateNicks;
    static QNetworkSession* s_network;
};

#endif // SESSION_H
