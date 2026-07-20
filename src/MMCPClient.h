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


#include <QTcpSocket>
#include <QObject>
#include <QString>
#include <QTimer>

#include "MMCP.h"


class MMCPServer;
class Host;

class MMCPClient : public QObject
{
    Q_OBJECT

    enum ClientState {
        Disconnected = 0,
        ConnectingIn,
        ConnectingOut,
        Connected,
        Pending
    };

public:
    MMCPClient(Host*, MMCPServer*);

    bool incoming(qintptr);
    void tryConnect(const QString&, quint16);
    void writeData(const QString&);
    void writeData(const QByteArray&);
    void sendMessage(const QString&);
    void sendPingRequest();
    void sendPeekRequest();
    void sendRequestConnections();
    bool isPending() { return mState == Pending; }

    void disconnect();

    void snoop();

    const QString getInfoString(int colName, int colAddr, int colPort, int colGroup, int colFlags);
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

    bool isIgnored() const { return mIgnored; }
    void setIgnore(const bool val) { mIgnored = val; }

    bool isPrivate() const { return mPrivate; }
    void setPrivate(const bool val) { mPrivate = val; }

    bool isServed() const { return mServed; }
    void setServed(const bool val) { mServed = val; }

    bool isServing() const { return mServing; }
    void setServing(const bool val) { mServing = val; }

    bool isSnooped() const { return mSnooped; }
    void setSnooped(const bool val) { mSnooped = val; }

    bool isSnooping() const { return mSnooping; }
    void setSnooping(const bool val) { mSnooping = val; }

    const QString& getGroup() const { return mGroup; }
    bool setGroup(const QString&);

    ClientState state() const { return mState; }

    void acceptCall();
    void denyCall();

    MMCPServer* getServer() { return mpMMCPServer; }

signals:
    void signal_clientDisconnected(MMCPClient*);

private slots:
    void slot_connected();
    void slot_disconnected();
    void slot_pendingTimeout();
    void slot_readData();
    void slot_displayError(QAbstractSocket::SocketError socketError);

private:
    Host* mpHost = nullptr;
    MMCPServer* mpMMCPServer = nullptr;
    ClientState mState = Disconnected;
    QTcpSocket mTcpSocket;
    QString mGroup = csDefaultMMCPGroupName;
    int mId = 0;
    bool mEnableSnooping = false;
    QString mPeerName;
    //auto ignore or ignorelist match?
    bool mIgnored = false;
    //auto private or privatelist match?
    bool mPrivate = false;
    //auto serve or servelist match?
    bool mServed = false;
    // do we actually know this?
    bool mServing = false;
    // are we snooping THEM?
    bool mSnooped = false;
    // is this client snooping US?
    bool mSnooping = false;
    QString mPeerAddress;
    quint16 mPeerPort = 0;
    QByteArray mPeerBuffer;
    QString mPeerVersion;
    QTimer mPendingTimer;

    // Snoop color handling
    std::string mLastSnoopColor;
    std::string mLastSgrState;
    bool mLastColorBold;
    bool mNeedsColorTracking;
    bool mNeedsColorSkip;

    void sendVersion();
    bool handleConnectedState(const QByteArray&);
    void handleIncomingChatEveryone(const QString&);
    void handleIncomingChatPersonal(const QString&);
    void handleIncomingChatGroup(const QString&);
    void handleIncomingClientVersion(const QString&);
    void handleIncomingConnectionList(const QString&);
    void handleIncomingConnectionsRequest();
    void handleIncomingNameChange(const QString&);
    void handleIncomingPeekConnections();
    void handleIncomingPeekList(const QString&);
    void handleIncomingPingRequest(const QString&);
    void handleIncomingPingResponse(const QString&);
    void handleIncomingSnoop();
    void handleIncomingSnoopData(const char*, quint16);
    void handleIncomingSideChannelData(const QString&);

    void updateSgrState(const std::string&);

    void postSnoopDataEvent(const QString&);
    void postSideChannelMessage(const QString&, const QString&, const QString&);
};
#endif // MUDLET_MMCPCLIENT_H
