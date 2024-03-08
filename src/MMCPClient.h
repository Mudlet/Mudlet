#ifndef MUDLET_MMCPCLIENT_H
#define MUDLET_MMCPCLIENT_H
/***************************************************************************
 *   Copyright (C) 2024 by John McKisson - john.mckisson@gmail.com         *
 *   Copyright (C) 2024 by Stephen Lyons - slysven@virginmedia.com         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "pre_guard.h"
#include <QtNetwork>
#include <QObject>
#include <QString>
#include "post_guard.h"

#include "MMCP.h"


class MMCPServer;
class Host;

class MMCPClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int id READ id WRITE setId)
    Q_PROPERTY(bool canSnoop READ canSnoop WRITE setCanSnoop)
    Q_PROPERTY(QString chatName READ chatName)
    Q_PROPERTY(bool isIgnored READ isIgnored WRITE setIgnore)
    Q_PROPERTY(bool isPrivate READ isPrivate WRITE setPrivate)
    Q_PROPERTY(bool isServed READ isServed WRITE setServed)
    Q_PROPERTY(bool isServing READ isServing WRITE setServing)
    Q_PROPERTY(bool isSnooped READ isSnooped WRITE setSnooped)
    Q_PROPERTY(bool isSnooping READ isSnooping WRITE setSnooping)
    Q_PROPERTY(int state READ state)
    Q_PROPERTY(QString host READ host)
    Q_PROPERTY(quint16 port READ port)

    enum ClientState {
        Disconnected = 0,
        ConnectingIn,
        ConnectingOut,
        Connected
    };

public:
    MMCPClient(Host*, MMCPServer*);

    bool incoming(qintptr);
    void tryConnect(const QString&, quint16);
    void writeData(const QString&);
    void sendMessage(const QString&);
    void sendPingRequest();
    void sendPeekRequest();
    void sendRequestConnections();

    void disconnect();

    void snoop();

    const QString getInfoString();
    const QString getFlagsString();
    const QString& getVersion() const { return mPeerVersion; }
    QString host();
    quint16 port();

    //Property Accessors/Mutators
    int id() const { return mId; }
    void setId(const int val) { mId = val; }

    bool canSnoop() const { return mEnableSnooping; }
    void setCanSnoop(const bool val) { mEnableSnooping = val; }

    const QString& chatName() const { return mPeerName; }

    bool isIgnored() const { return mIsIgnored; }
    void setIgnore(const bool val) { mIsIgnored = val; }

    bool isPrivate() const { return mIsPrivate; }
    void setPrivate(const bool val) { mIsPrivate = val; }

    bool isServed() const { return mIsServed; }
    void setServed(const bool val) { mIsServed = val; }

    bool isServing() const { return mIsServing; }
    void setServing(const bool val) { mIsServing = val; }

    bool isSnooped() const { return mIsSnooped; }
    void setSnooped(const bool val) { mIsSnooped = val; }

    bool isSnooping() const { return mIsSnooping; }
    void setSnooping(const bool val) { mIsSnooping = val; }

    const QString& getGroup() const { return mGroup; }
    bool setGroup(const QString&);

    ClientState state() const { return mState; }

    MMCPServer* getServer() { return mpMMCPServer; }

signals:
    void signal_clientDisconnected(MMCPClient*);

private slots:
    void slot_connected();
    void slot_disconnected();
    void slot_readData();
    void slot_displayError(QAbstractSocket::SocketError socketError);

private:
    Host* mpHost = nullptr;
    MMCPServer* mpMMCPServer = nullptr;
    ClientState mState = Disconnected;
    QTcpSocket mTcpSocket;
    QString mGroup = csDefaultMMCPGroupName;
    int mId;
    bool mEnableSnooping = false;
    QString mPeerName;
    //auto ignore or ignorelist match?
    bool mIsIgnored = false;
    //auto private or privatelist match?
    bool mIsPrivate = false;
    //auto serve or servelist match?
    bool mIsServed = false;
    // do we actually know this?
    bool mIsServing = false;
    // are we snooping THEM?
    bool mIsSnooped = false;
    // is this client snooping US?
    bool mIsSnooping = false;
    QString mPeerAddress;
    quint16 mPeerPort;
    QByteArray mPeerBuffer;
    QString mPeerVersion;


    void sendVersion();
    void handleConnectedState(const QByteArray&);
    void handleIncomingChannelData(const QString&);
    void handleIncomingChatEveryone(const QString&);
    void handleIncomingChatPersonal(const QString&);
    void handleIncomingChatGroup(const QString&);
    void handleIncomingConnectionList(const QString&);
    void handleIncomingConnectionsRequest();
    void handleIncomingMessage(const QString&);
    void handleIncomingNameChange(const QString&);
    void handleIncomingPeekConnections();
    void handleIncomingPeekList(const QString&);
    void handleIncomingPingRequest(const QString&);
    void handleIncomingPingResponse(const QString&);
    void handleIncomingSnoop();
    void handleIncomingSnoopData(const char*, quint16);
    void handleIncomingSideChannelData(const QString&);
};
#endif // MUDLET_MMCPCLIENT_H
