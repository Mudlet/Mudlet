#ifndef MUDLET_TIRCCLIENT_H
#define MUDLET_TIRCCLIENT_H

/***************************************************************************
 *   Copyright (C) 2010-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include <IrcMessage>

#include <QObject>
#include <QPair>
#include <QPointer>
#include <QStringList>

#include "utils.h"

class Host;
class IrcBuffer;
class IrcBufferModel;
class IrcCommand;
class IrcConnection;

// A profile's IRC session: the connection, the buffers its lines are sorted
// into, and the nick, server and channels it is using. The Host owns it; the
// IRC window shows it.
class TIrcClient : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE(<IrcBuffer>)

public:
    Q_DISABLE_COPY_MOVE(TIrcClient)
    explicit TIrcClient(Host*);
    ~TIrcClient() override;

    inline static QString HostNameCfgItem = qsl("irc_host");
    inline static QString HostPortCfgItem = qsl("irc_port");
    inline static QString HostSecureCfgItem = qsl("irc_secure");
    inline static QString NickNameCfgItem = qsl("irc_nick");
    inline static QString PasswordCfgItem = qsl("irc_password");
    inline static QString ChannelsCfgItem = qsl("irc_channels");
    inline static QString DefaultHostName = qsl("irc.libera.chat");
    inline static int DefaultHostPort = 6667;
    inline static bool DefaultHostSecure = false;
    inline static QString DefaultNickName = qsl("Mudlet");
    inline static QStringList DefaultChannels = QStringList() << qsl("#mudlet");

    static QPair<bool, QString> validateMsgArguments(const QString& target, const QString& message);
    static QString readIrcHostName(Host* pH);
    static int readIrcHostPort(Host* pH);
    static bool readIrcHostSecure(Host* pH);
    static QString readIrcNickName(Host* pH);
    static QString readIrcPassword(Host* pH);
    static QStringList readIrcChannels(Host* pH);
    static QPair<bool, QString> writeIrcHostName(Host* pH, const QString& hostname);
    static QPair<bool, QString> writeIrcHostPort(Host* pH, int port);
    static QPair<bool, QString> writeIrcHostSecure(Host* pH, bool secure);
    static QPair<bool, QString> writeIrcNickName(Host* pH, const QString& nickname);
    static QPair<bool, QString> validateIrcPassword(const QString& password);
    static QPair<bool, QString> writeIrcPassword(Host* pH, const QString& password);
    static QPair<bool, QString> writeIrcChannels(Host* pH, const QStringList& channels);
    static bool validIrcChannelName(const QString& channel);

    // Set once a channel has been joined.
    bool mReadyForSending = false;

    IrcConnection* connection() const { return mpConnection; }
    // One buffer per channel and query, and one for the server that takes
    // every line no other buffer does. Null until start().
    IrcBufferModel* bufferModel() const { return mpBufferModel; }
    IrcBuffer* serverBuffer() const;
    // Queues the auto-join, opens the connection and sets up the buffers; later
    // calls do nothing.
    void start();
    // False, doing nothing, for a session that was never started
    bool restart(bool reloadConfigs = true);
    QPair<bool, QString> sendText(const QString& target, const QString& message);
    // Takes ownership of a command the IRC window's parser built.
    void sendCommand(IrcCommand* command);
    QString getHostName() const { return mHostName; }
    int getHostPort() const { return mHostPort; }
    bool getHostSecure() const { return mHostSecure; }
    QString getNickName() const { return mNickName; }
    QStringList getChannels() const { return mChannels; }
    QString getConnectedHost() const { return mConnectedHostName; }

signals:
    void signal_nickNameChanged();
    void signal_nickNameReserved(const QString& reserved, const QString& replacement);
    // Raised before the old connection is quit, while getChannels() still
    // lists the channels it had joined.
    void signal_restarting(const QString& reason);
    void signal_restarted();
    // The server does not echo our own messages back.
    void signal_messageSent(IrcMessage* message);
    // A line has reached one of the buffers, after any sysIrcMessage for it was raised.
    void signal_messageReceived(IrcBuffer* buffer, IrcMessage* message);

private slots:
    void slot_nickNameRequired(const QString& reserved, QString* alt);
    void slot_nickNameChanged(const QString& nick);
    void slot_joinedChannel(IrcJoinMessage* message);
    void slot_partedChannel(IrcPartMessage* message);
    void slot_receiveNumericMessage(IrcNumericMessage* message);
    void slot_bufferAdded(IrcBuffer* buffer);

private:
    static QString readAppDefaultIrcNick();
    static void writeAppDefaultIrcNick(const QString&);
    void receiveBufferMessage(IrcBuffer* buffer, IrcMessage* message);

    QPointer<Host> mpHost;
    IrcConnection* mpConnection = nullptr;
    IrcBufferModel* mpBufferModel = nullptr;
    QPointer<IrcBuffer> mpServerBuffer;
    bool mStarted = false;
    // When the last typed PING went out, for timing its PONG; 0 when none is awaited.
    quint64 mPingStarted = 0;
    QString mConnectedHostName;
    QString mHostName;
    int mHostPort = 0;
    bool mHostSecure = false;
    QString mNickName;
    QString mUserName = qsl("mudlet");
    QString mPassword;
    QString mRealName;
    QStringList mChannels;
};

#endif // MUDLET_TIRCCLIENT_H
