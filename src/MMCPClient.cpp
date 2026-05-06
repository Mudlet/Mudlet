/***************************************************************************
 *   Copyright (C) 2024 by John McKisson - john.mckisson@gmail.com         *
 *   Copyright (C) 2024, 2026 by Stephen Lyons - slysven@virginmedia.com   *
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

#include "MMCPClient.h"
#include "Host.h"
#include "MMCPServer.h"
#include "TEvent.h"
#include "mudlet.h"

#include <QHostAddress>
#include <QTcpSocket>
#include <QRegularExpression>
#include <QtGlobal>

QString convertToIPv4(QHostAddress addr)
{
    // Check if the address is an IPv4-mapped IPv6 address
    if (addr.protocol() == QAbstractSocket::IPv6Protocol && addr.isInSubnet(QHostAddress::parseSubnet("::ffff:0:0/96"))) {
        // Convert to IPv4
        QHostAddress ipv4addr = QHostAddress(addr.toIPv4Address());
        return ipv4addr.toString();
    }
    // Return the original address if it's not an IPv4-mapped IPv6 address
    // or conversion is not applicable
    return addr.toString();
}

MMCPClient::MMCPClient(Host* pHost, MMCPServer* pServer)
: mpHost(pHost)
, mpMMCPServer(pServer)
, mTcpSocket(this)
, mLastColorBold(false)
, mNeedsColorTracking(false)
, mNeedsColorSkip(false)
{
    //Disable Nagle's algorithm
    mTcpSocket.setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(&mTcpSocket, &QTcpSocket::connected, this, &MMCPClient::slot_connected);
    connect(&mTcpSocket, &QTcpSocket::disconnected, this, &MMCPClient::slot_disconnected);
    connect(&mTcpSocket, &QTcpSocket::readyRead, this, &MMCPClient::slot_readData);
    connect(&mTcpSocket, &QAbstractSocket::errorOccurred, this, &MMCPClient::slot_displayError);
    connect(this, &MMCPClient::signal_clientDisconnected, mpMMCPServer, &MMCPServer::slot_clientDisconnected);

    // Setup pending connection timeout
    mPendingTimer.setSingleShot(true);
    connect(&mPendingTimer, &QTimer::timeout, this, &MMCPClient::slot_pendingTimeout);
}

/**
 * Attempt an outgoing connection to a client
 */
void MMCPClient::tryConnect(const QString& host, quint16 port)
{
    mState = ConnectingOut;
    mTcpSocket.connectToHost(host, port);
}


/**
 * Disconnect the client.  This will result in the disconnected() signal.
 */
void MMCPClient::disconnect()
{
    mTcpSocket.disconnectFromHost();
}


/**
 * Handle an incoming chat connection.
 */
bool MMCPClient::incoming(qintptr socketDesc)
{
    mState = ConnectingIn;
    return mTcpSocket.setSocketDescriptor(socketDesc);
}

QString MMCPClient::host()
{
    return convertToIPv4(mTcpSocket.peerAddress());
}

quint16 MMCPClient::port()
{
    return mTcpSocket.peerPort();
}


/**
 * Outgoing connection established, send chat request.
 */
void MMCPClient::slot_connected()
{
    mPeerAddress = convertToIPv4(mTcpSocket.peerAddress());
    mPeerPort = mTcpSocket.peerPort();

    QString str = qsl("CHAT:%1\n%2%3")
                          .arg(mpMMCPServer->getChatName(),
                               mPeerAddress)
                          .arg(mPeerPort, -5);

    mTcpSocket.write(str.toLatin1());

    const QString infoMsg = tr("[ CHAT ]  - Waiting for response from %1:%2...").arg(mPeerAddress, QString::number(mPeerPort));
    mpHost->postMessage(infoMsg);
}

/**
 * A client has been disconected.
 * Clear snoop flags if we were snooping or they were snooping us.
 */
void MMCPClient::slot_disconnected()
{
    // Stop pending timer if it's running
    if (mPendingTimer.isActive()) {
        mPendingTimer.stop();
    }

    /*: This message is used when a MMCP peer without a name disconnects,
     * %1 is the peer's IP address (numbers or URL), %2 is the port they are
     * listening on. Should be similiar to the one when we do have a name.
     */
    const QString infoMsg = mPeerName.isEmpty() ? tr("[ CHAT ]  - You are now disconnected from <unknown> - %1:%2.").arg(mPeerAddress, QString::number(mPeerPort))
    /*: This message is used when a MMCP peer with a name disconnects,
     * %1 is the peer's name, %2 is the peer's IP address (numbers or URL),
     * %3 is the port they are listening on. Should be similiar to the one when
     * we do not have a name.
     */
                                                : tr("[ CHAT ]  - You are now disconnected from %1 - %2:%3.").arg(mPeerName, mPeerAddress, QString::number(mPeerPort));
    mpHost->postMessage(infoMsg);

    if (isSnooping()) {
        setSnooping(false);
        mpMMCPServer->decrementSnoopCount();
    }

    if (isSnooped()) {
        setSnooped(false);
    }

    emit signal_clientDisconnected(this);
}

/**
 * Pending connection was never accepted or denied by us.
 * Notify the user and disconnect.
 */
void MMCPClient::slot_pendingTimeout()
{
    if (mState == Pending) {
        const QString infoMsg = tr("[ CHAT ]  - Connection from %1 at %2:%3 timed out (not accepted or denied by you).")
                                        .arg(mPeerName,
                                            convertToIPv4(mTcpSocket.peerAddress()),
                                            QString::number(mTcpSocket.peerPort()));
        mpHost->postMessage(infoMsg);

        mState = Disconnected;
        writeData(qsl("NO:%1\n").arg(mpMMCPServer->getChatName()));
        disconnect();
    }
}

/**
 * Process incoming data from the client socket
 */
void MMCPClient::slot_readData()
{
    while (mTcpSocket.bytesAvailable()) {
        if (mPeerBuffer.length() > 0) {
            qDebug().noquote().nospace() << "MMCPClient::slot_readData() INFO - appending to previous partial buffer.";
        }

        mPeerBuffer.append(mTcpSocket.read(mTcpSocket.bytesAvailable()));
    }

    switch (mState) {
    case ConnectingIn: {
        // Another peer is asking to connect to us and we need to reply
        // appropriately:

        const int nlPos = mPeerBuffer.indexOf('\n');
        // As mPeerBuffer is a QByteArray we do not need to "wrap" a const char*
        // to test against it:
        if (!mPeerBuffer.startsWith("CHAT:") || nlPos == -1) {
            mState = Disconnected;
            disconnect();
            return;
        }

        const QByteArray peerName = mPeerBuffer.mid(5, nlPos - 5);

        // The spec really doesnt dictate a max length, but we should have one to avoid abuse
        if (peerName.length() > 64) {
            qWarning() << "MMCPClient::slot_readData() - Rejecting connection: peer name too long (" << peerName.length() << " bytes)";
            mState = Disconnected;
            writeData(qsl("NO:%1\n").arg(mpMMCPServer->getChatName()));
            disconnect();

            const QString infoMsg = tr("[ CHAT ]  - Connection from %1 at %2:%3 denied (Peer name too long (64 chars max)).")
                                            .arg(mPeerName,
                                                 convertToIPv4(mTcpSocket.peerAddress()),
                                                 QString::number(mTcpSocket.peerPort()));

            mpHost->postMessage(infoMsg);

            return;
        }

        const QByteArray ipAndPort = mPeerBuffer.mid(nlPos + 1);

        // Validate that we have enough data for IP and port (at least 5 chars for port)
        if (ipAndPort.size() < 5) {
            qWarning() << "MMCPClient::slot_readData() - Malformed handshake: insufficient data for IP and port";
            mState = Disconnected;
            disconnect();
            return;
        }

        // Exclude the last 5 characters for the IP address
        const QByteArray ipAddress = ipAndPort.left(ipAndPort.size() - 5);
        // Last 5 characters for the port
        const QByteArray port = ipAndPort.right(5);

        QHostAddress host(QString::fromUtf8(ipAddress));

        if (mpMMCPServer->isDoNotDisturb()) {
            mState = Disconnected;
            writeData(qsl("NO:%1\n").arg(mpMMCPServer->getChatName()));
            disconnect();

            const QString infoMsg = tr("[ CHAT ]  - Connection from %1 at %2:%3 denied (DoNotDisturb).")
                                            .arg(mPeerName,
                                                 convertToIPv4(mTcpSocket.peerAddress()),
                                                 QString::number(mTcpSocket.peerPort()));

            mpHost->postMessage(infoMsg);
        } else {

            mPeerName = QString::fromUtf8(peerName);

            // The ipAddress string to QHostAddress may have failed
            mPeerAddress = host.isNull() ? "<Unknown>" : convertToIPv4(host);
            bool ok;
            mPeerPort = QString::fromUtf8(port).toUInt(&ok);
            if (!ok) {
                mState = Disconnected;
                disconnect();
            } else {

                quint16 clientId = mpMMCPServer->addConnectedClient(this);

                if (mpHost->getMMCPAutoAcceptCalls()) {
                    acceptCall();

                } else {
                    mState = Pending;

                    const QString infoMsg = tr("[ CHAT ]  - Connection from %1 at %2:%3 is pending, use mmcp.accept(%4) or mmcp.deny(%4) to accept or deny.")
                                                    .arg(mPeerName,
                                                        convertToIPv4(mTcpSocket.peerAddress()),
                                                        QString::number(mTcpSocket.peerPort()))
                                                    .arg(clientId);

                    mpHost->postMessage(infoMsg);

                    // Start 60 second timeout for pending connection
                    // We don't know what client the peer is using, their client may timeout the socket, or may not
                    // Regardless, we'll time it out after 60 seconds
                    mPendingTimer.start(60000);
                }
            }
        }

        break;
    }

    case ConnectingOut: {
        // We have requested to another peer and they should be accepting or
        // rejecting our attempt with either a YES or a NO:
        const int colonPos = mPeerBuffer.indexOf(':');
        const int nlPos = mPeerBuffer.indexOf('\n');

        bool isNo = mPeerBuffer.startsWith("NO:");

        if (!(mPeerBuffer.startsWith("YES:") || isNo) || nlPos == -1 || colonPos == -1) {
            mState = Disconnected;

            // In this case we do not get details of the connection from the
            // other end - instead we can only report the apparent details from
            // the socket:
            const QString infoMsg = tr("[ CHAT ]  - Connection to %1:%2 refused.")
                                            .arg(convertToIPv4(mTcpSocket.peerAddress()),
                                                 QString::number(mTcpSocket.peerPort()));
            mpHost->postMessage(infoMsg);

        } else if (isNo) {
            // We were rejected by the other end, but we do get details of the connection from them, so we can report that:
            mState = Disconnected;
            const QByteArray peerName = mPeerBuffer.mid(colonPos + 1, nlPos - colonPos - 1);
            const QString rejectedBy = QString::fromUtf8(peerName);
            const QString infoMsg = tr("[ CHAT ]  - Connection to %1 at %2:%3 rejected.")
                                            .arg(rejectedBy,
                                                convertToIPv4(mTcpSocket.peerAddress()),
                                                QString::number(mTcpSocket.peerPort()));
            mpHost->postMessage(infoMsg);

        } else {
            // We have got a "YES:" - yippee!
            const QByteArray peerName = mPeerBuffer.mid(colonPos + 1, nlPos - colonPos - 1);
            const QByteArray cmdData = mPeerBuffer.right(mPeerBuffer.size() - nlPos - 1);

            mPeerName = QString::fromUtf8(peerName);
            mState = Connected;

            const QString infoMsg = tr("[ CHAT ]  - Connection to %1 at %2:%3 accepted.")
                                            .arg(mPeerName,
                                                 convertToIPv4(mTcpSocket.peerAddress()),
                                                 QString::number(mTcpSocket.peerPort()));

            mpHost->postMessage(infoMsg);
            mpMMCPServer->addConnectedClient(this);
            sendVersion();

            if (cmdData.length() > 0) {
                qDebug().noquote().nospace() << "MMCPClient::slot_readData() INFO - additional cmd at connect: \"" << cmdData << "\".";
                //A command was tacked on the end of the accept string
                handleConnectedState(cmdData);
            }
        }

        break;
    }

    case Connected:
        if (!mPeerBuffer.endsWith(static_cast<char>(0xff))) {
            qDebug().noquote().nospace() << "MMCPClient::slot_readData() INFO - partial buffer received, waiting for the rest...";
            return;
        }

        if (!handleConnectedState(mPeerBuffer)) {
            // We had a partial command in the buffer, wait for more data before clearing the buffer
            return;
        }
        break;

    case Pending:
        // We got something from a pending peer, dont do anything with it
        break;

    default:
        qDebug("unknown client state");
    }

    mPeerBuffer.clear();
}

/**
 * Accepts an incoming chat connection
 * Set's the client state to Connected, sends the success handshake response
 * and our client version to the new peer
 */
void MMCPClient::acceptCall() {
    mPendingTimer.stop();  // Stop timeout
    mState = Connected;
    writeData(QString("YES:%1\n").arg(mpMMCPServer->getChatName()));

    const QString infoMsg = tr("[ CHAT ]  - Connection from %1 at %2:%3 accepted.")
                                    .arg(mPeerName,
                                        convertToIPv4(mTcpSocket.peerAddress()),
                                        QString::number(mTcpSocket.peerPort()));

    mpHost->postMessage(infoMsg);

    sendVersion();
}

/**
 * Denies an incoming chat connection
 *
 */
void MMCPClient::denyCall() {
    mPendingTimer.stop();  // Stop timeout
    mState = Disconnected;

    writeData(qsl("NO:%1\n").arg(mpMMCPServer->getChatName()));

    disconnect();

    const QString infoMsg = tr("[ CHAT ]  - Connection from %1 at %2:%3 denied.")
                                    .arg(mPeerName,
                                            convertToIPv4(mTcpSocket.peerAddress()),
                                            QString::number(mTcpSocket.peerPort()));

    mpHost->postMessage(infoMsg);
}


void MMCPClient::slot_displayError(QAbstractSocket::SocketError socketError)
{
    QString message;

    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        // This will be hit if a TinTin++ client is in DND mode and is not
        // accepting connections.
        message = tr("[ CHAT ]  - The peer closed or refused the connection.");
        break;
    case QAbstractSocket::HostNotFoundError:
        message = tr("[ CHAT ]  - The peer was not found. Please check the host name and port settings.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        message = tr("[ CHAT ]  - The connection was refused by the peer.");
        break;
    default:
        message = tr("[ CHAT ]  - The following error occurred: %1.").arg(mTcpSocket.errorString());
    }

    mpHost->postMessage(message);

    // If we failed to connect, we are not yet in the peers list and need to clean up
    if (mState == ConnectingOut) {
        deleteLater();
    }
}


/**
 * Send a Message to the client socket
 */
void MMCPClient::sendMessage(const QString& msg)
{
    QByteArray out;
    out.append(static_cast<char>(Message));
    out.append(msg.toUtf8());
    out.append(static_cast<char>(End));
    writeData(out);
}


/**
 * Send a ping request to this client.
 */
void MMCPClient::sendPingRequest()
{
    QByteArray pingData;
    pingData.append(static_cast<char>(PingRequest));
    pingData.append(QByteArray::number(QDateTime::currentMSecsSinceEpoch()));
    pingData.append(static_cast<char>(End));
    writeData(pingData);

    const QString infoMsg = tr("[ CHAT ]  - Pinging %1...").arg(mPeerName);
    mpHost->postMessage(infoMsg);
}

/**
 * Send a peek connections request to this client.
 */
void MMCPClient::sendPeekRequest()
{
    QByteArray peekData;
    peekData.append(static_cast<char>(PeekConnections));
    peekData.append(static_cast<char>(End));
    writeData(peekData);

    const QString infoMsg = tr("[ CHAT ]  - Attempting to peek at %1's public connections...").arg(mPeerName);
    mpHost->postMessage(infoMsg);
}

/**
 * Request client connections list
 */
void MMCPClient::sendRequestConnections()
{
    QByteArray requestData;
    requestData.append(static_cast<char>(RequestConnections));
    requestData.append(static_cast<char>(End));
    writeData(requestData);

    const QString infoMsg = tr("[ CHAT ]  - Requested connections from %1").arg(mPeerName);
    mpHost->postMessage(infoMsg);
}


/**
 * Send client version
 */
void MMCPClient::sendVersion()
{
    QByteArray versionData;
    versionData.append(static_cast<char>(Version));
    versionData.append(mudlet::self()->scmVersion.toUtf8());
    versionData.append(static_cast<char>(End));
    writeData(versionData);
}

/**
 * Assign a client to a group.
 * If supplied group is "none", "<none" or empty string, remove from group.
 * Returns true if they were assigned to a group, false if they were removed.
 */
bool MMCPClient::setGroup(const QString& group)
{
    if (group.isEmpty() || !group.compare(qsl("none"), Qt::CaseInsensitive) || !group.compare(csDefaultMMCPGroupName, Qt::CaseInsensitive)) {
        mGroup = csDefaultMMCPGroupName;
        return false;
    }

    mGroup = group;
    return true;
}

/**
 * Write to this client socket.
 */
void MMCPClient::writeData(const QString& data)
{
    qint64 bytesWritten = mTcpSocket.write(data.toLatin1());
    if (bytesWritten <= 0) {
        const QString identifier = mPeerName.isEmpty() ? convertToIPv4(mTcpSocket.peerAddress()) : mPeerName;
        if (bytesWritten == 0) {
            qWarning() << "MMCPClient::writeData(QString&) Failed to write data to socket for client " << identifier;
        } else if (bytesWritten == -1) {
            qWarning() << "MMCPClient::writeData(QString&) - Failed to write data to socket:" << mTcpSocket.errorString() << " for client " << identifier;
        }
    }
}

void MMCPClient::writeData(const QByteArray& data)
{
    qint64 bytesWritten = mTcpSocket.write(data);
    if (bytesWritten <= 0) {
        const QString identifier = mPeerName.isEmpty() ? convertToIPv4(mTcpSocket.peerAddress()) : mPeerName;
        if (bytesWritten == 0) {
            qWarning() << "MMCPClient::writeData(QByteArray&) Failed to write data to socket for client " << identifier;
        } else if (bytesWritten == -1) {
            qWarning() << "MMCPClient::writeData(QByteArray&) - Failed to write data to socket:" << mTcpSocket.errorString() << " for client " << identifier;
        }
    }
}

/**
 * Attempt to snoop this client.
 */
void MMCPClient::snoop()
{
    QByteArray snoopCmd;
    snoopCmd.append(static_cast<char>(Snoop));
    snoopCmd.append(static_cast<char>(End));
    writeData(snoopCmd);
}

/**
 * Process incoming data from this client socket.
 * Find and handle chat commands.
 * @return True if we completely processed the buffer
 * @return False if we encountered a partial command need to wait for additional data
 */
bool MMCPClient::handleConnectedState(const QByteArray& bytes)
{
    const char* data = bytes.data();
    int cmdIdx = 0;
    while (cmdIdx < bytes.length()) {
        const char cmd = data[cmdIdx];

        const int endIdx = bytes.indexOf(MMCPChatCommand::End, cmdIdx);
        if (endIdx == -1) {
            qDebug().noquote().nospace() << "MMCPClient::handleConnectedState(...) INFO - partial buffer detected...";
            return false;
        }

        const QString stringData = QString::fromLatin1(data + cmdIdx + 1, (endIdx - cmdIdx) - 1);

        switch (cmd) {
        case MMCPChatCommand::NameChange:
            handleIncomingNameChange(stringData);
            break;

        case MMCPChatCommand::RequestConnections:
            handleIncomingConnectionsRequest();
            break;

        case MMCPChatCommand::ConnectionList:
            handleIncomingConnectionList(stringData);
            break;

        case MMCPChatCommand::TextEveryone:
            handleIncomingChatEveryone(stringData);
            break;

        case MMCPChatCommand::TextPersonal:
            handleIncomingChatPersonal(stringData);
            break;

        case MMCPChatCommand::TextGroup:
            handleIncomingChatGroup(stringData);
            break;

        case MMCPChatCommand::Message:
            mpMMCPServer->clientMessage(this, stringData);
            break;

        case MMCPChatCommand::Version:
            handleIncomingClientVersion(stringData);
            break;

        case MMCPChatCommand::PingRequest:
            handleIncomingPingRequest(stringData);
            break;

        case MMCPChatCommand::PingResponse:
            handleIncomingPingResponse(stringData);
            break;

        case MMCPChatCommand::PeekConnections:
            handleIncomingPeekConnections();
            break;

        case MMCPChatCommand::PeekList:
            handleIncomingPeekList(stringData);
            break;

        case MMCPChatCommand::Snoop:
            handleIncomingSnoop();
            break;

        case MMCPChatCommand::SnoopData:
            handleIncomingSnoopData(data + cmdIdx + 1, (endIdx - cmdIdx) - 1);
            break;

        case MMCPChatCommand::SideChannel:
            handleIncomingSideChannelData(stringData);
            break;

        default:
            qDebug().noquote().nospace() << "MMCPClient::handleConnectedState(...) INFO - unknown command: 0x" << QString::number(static_cast<char>(cmd), 16).toUpper() << ": \"" << stringData << "\".";
        }

        cmdIdx = endIdx + 1;
    }

    return true;
}


void MMCPClient::handleIncomingClientVersion(const QString& version) {
    mPeerVersion = version;

    bool isMudMaster = mPeerVersion.contains("MudMaster");
    bool isMudlet = mPeerVersion.contains("Mudlet");
    bool isMUSHClient = mPeerVersion.contains("MUSHc");
    bool isTinTin = mPeerVersion.contains("TinTin");

    mNeedsColorTracking = isMudMaster || isMudlet || isMUSHClient || isTinTin;

    mNeedsColorSkip = isMudMaster || isMUSHClient;
}

/**
 * We were sent a connection list, we're supposed to connect to the clients
 * given to us
 * Maybe check option if we want to auto call everyone in this list
 */
void MMCPClient::handleIncomingConnectionList(const QString& list)
{
    QStringList parts = list.split(',');

    if (parts.size() % 2 != 0) {
        mpHost->postMessage(tr("[ CHAT ]  - Badly formatted connection list from %1")
            .arg(mPeerName));
        return;
    }
    for (int i = 0; i < parts.size(); i++) {
        const QString host = parts[i];
        bool ok = false;
        uint16_t port = parts[++i].toUInt(&ok);

        if (!ok) {
            mpHost->postMessage(tr("[ CHAT ]  - Error parsing host value from connection: %1")
                .arg(parts[i]));
            continue;
        }

        mpHost->postMessage(tr("[ CHAT ]  - Attempting to connect to %1:%2 provided by %3")
            .arg(host).arg(port).arg(mPeerName));

        mpMMCPServer->call(host, port);
    }

}

/**
 * Someone requested that we give them our public connections
 */
void MMCPClient::handleIncomingConnectionsRequest()
{
    if (mIgnored) {
        const QString infoMsg = tr("[ CHAT ]  - %1 is trying to request your connections!").arg(mPeerName);
        mpHost->postMessage(infoMsg);
        return;
    }

    if (mpHost->getMMCPAllowPeekRequests()) {
        const QString infoMsg = tr("[ CHAT ]  - %1 has requested your public connections...").arg(mPeerName);
        mpHost->postMessage(infoMsg);

        mpMMCPServer->sendPublicConnections(this);
    } else {
        const QString infoMsg = tr("[ CHAT ]  - %1 has requested your public connections, but you're ignoring connection requests...").arg(mPeerName);
        mpHost->postMessage(infoMsg);
    }
}

/**
 * Display a chat all message and echo it to any clients we may be serving
 * Or, if we're serving someone forward it to them
 */
void MMCPClient::handleIncomingChatEveryone(const QString& msg)
{
    // We're ignoring chats from this person
    if (mIgnored) {
        return;
    }

    mpMMCPServer->clientMessage(this, msg);

    // If mServed is true:
    // We are serving this client, forward their message to everyone
    // except them and ourself
    // If mServed is false:
    // Echo message to other clients we might be serving

    // sendServedMessage expects an onlyToServed boolean
    mpMMCPServer->sendServedMessage(this, msg, !mServed);
}

/**
 * Display an incoming personal chat
 */
void MMCPClient::handleIncomingChatPersonal(const QString& msg)
{
    mpMMCPServer->clientMessage(this, msg);
}

/**
 * Display an incoming chat to a group
 */
void MMCPClient::handleIncomingChatGroup(const QString& msg)
{
    const QString groupStr = msg.left(15).trimmed();
    const QString trimmedMsg = msg.right(msg.length() - 15);

    using namespace AnsiColors;

    //: Incoming group message, %1, %2 and %4 are ANSI Escape codes
    const QString groupMsg = tr("%1%2%3%4(%5)%1%2%6%1")
                                     .arg(RST,
                                          FBLDRED,
                                          mpHost->getMMCPChatPrefix(),
                                          FBLDCYN,
                                          groupStr,
                                          trimmedMsg);

    mpMMCPServer->clientMessage(this, groupMsg);
}

/**
 * A client changed their name
 */
void MMCPClient::handleIncomingNameChange(const QString& newName)
{
    const QString infoMsg = tr("[ CHAT ]  - %1 is now known as %2.").arg(mPeerName, newName);
    mpHost->postMessage(infoMsg);
    mPeerName = newName;
}

/**
 * Someone has requested to peek our connections
 */
void MMCPClient::handleIncomingPeekConnections()
{
    //check if this client is ignored before doing anything drastic
    if (mIgnored) {
        const QString infoMsg = tr("[ CHAT ]  - %1 is trying to peek your connections!").arg(mPeerName);
        mpHost->postMessage(infoMsg);
        return;
    }

    if (mpHost->getMMCPAllowPeekRequests()) {
        const QString infoMsg = tr("[ CHAT ]  - %1 is peeking at your connections...").arg(mPeerName);
        mpHost->postMessage(infoMsg);

        mpMMCPServer->sendPublicPeek(this);
    } else {
        const QString infoMsg = tr("[ CHAT ]  - %1 is trying to peek your connections, but you're ignoring peek requests...").arg(mPeerName);
        mpHost->postMessage(infoMsg);
    }
}

/**
 * Display someones peek list that we've requested
 */
void MMCPClient::handleIncomingPeekList(const QString& list)
{
    // Drop the trailing ~ if it exists so modulus will do the trick after
    // a split
    QString str = list;
    if (str.endsWith("~")) {
        str.chop(1);
    }
    const QStringList parts = str.split("~");
    if (parts.size() % 3 != 0) {
        const QString infoMsg = tr("[ CHAT ]  - Badly formatted peek list from %1.").arg(mPeerName);
        mpHost->postMessage(infoMsg);
        return;
    }

    QStringList messageList;
    using namespace AnsiColors;

    quint16 count = 0;
    for (int i = 0; (i + 1) < parts.size(); i += 3) {
        messageList << qsl("%1%2 %3%4 %5 %6")
                               .arg(FBLDWHT)
                               .arg(++count, 4)
                               .arg(RST)
                               .arg(parts.at(i + 2), -20) // Peer name
                               .arg(parts.at(i), -15) // Peer address
                               .arg(parts.at(i + 1), 5); // Peer port
    }

    // We need to include ID here as otherwise any leading spaces get trimmed
    // off inside MMCPServer::clientMessage(...)!
    const QString listOut = tr("Id   Name                 Address         Port\n"
                               "==== ==================== =============== =====\n"
                               "%1\n"
                               "%2==== ==================== =============== =====%3\n")
                                    .arg(messageList.join(QChar::LineFeed))
                                    .arg(FBLDRED).arg(RST);
    mpMMCPServer->clientMessage(this, listOut);
}

/**
 * Respond to someones ping request
 */
void MMCPClient::handleIncomingPingRequest(const QString& msg)
{
    QByteArray out;
    out.append(static_cast<char>(PingResponse));
    out.append(msg.toUtf8());
    out.append(static_cast<char>(End));
    writeData(out);
}

/**
 * Handle ping data that we've requested
 */
void MMCPClient::handleIncomingPingResponse(const QString& data)
{
    bool ok;
    qint64 returnTime = data.toLongLong(&ok);

    if (ok) {
        const QString infoMsg = tr("[ CHAT ]  - Ping returned from %1: %2 ms").arg(mPeerName).arg(QDateTime::currentMSecsSinceEpoch() - returnTime);
        mpHost->postMessage(infoMsg);
    } else {
        const QString infoMsg = tr("[ CHAT ]  - Bad Ping response from %1: %2").arg(mPeerName, data);
        mpHost->postMessage(infoMsg);
    }
}


/**
 * This client sent a request to snoop you (the user).
 * Apparently the client is responsible for telling the connecting user.
 *
 * We're sending a message back to the client to let them know if they
 * can or cannot snoop us. We don't know what language the remote client is using...
 * Send it in English for now and hope for the best? Maybe noone is using this snoop feature?
 */
void MMCPClient::handleIncomingSnoop()
{
    if (!canSnoop()) {
        sendMessage(qsl("<CHAT> You do not have permission to snoop %1.").arg(mpMMCPServer->getChatName()));
        const QString infoMsg = tr("[ CHAT ]  - %1 tried to snoop you but doesn't have permission.").arg(mPeerName);
        mpHost->postMessage(infoMsg);
        return;
    }

    if (isSnooping()) {
        setSnooping(false);
        mpMMCPServer->decrementSnoopCount();

        const QString infoMsg = tr("[ CHAT ]  - %1 has stopped snooping you.").arg(mPeerName);
        mpHost->postMessage(infoMsg);
        sendMessage(qsl("<CHAT> You have stopped snooping %1.").arg(mpMMCPServer->getChatName()));

    } else {
        setSnooping(true);
        mpMMCPServer->incrementSnoopCount();

        const QString infoMsg = tr("[ CHAT ]  - %1 has begun snooping you.").arg(mPeerName);
        mpHost->postMessage(infoMsg);
        sendMessage(qsl("<CHAT> You have begun snooping %1.").arg(mpMMCPServer->getChatName()));
    }
}

void MMCPClient::updateSgrState(const std::string &ansiSeq)
{
    // We expect SGR sequences of the form "\033[<codes>m"
    if (ansiSeq.size() < 3 || ansiSeq.front() != '\033' || ansiSeq[1] != '[' || ansiSeq.back() != 'm')
        return;

    // Extract the inner code string (e.g., "1;35")
    std::string codes = ansiSeq.substr(2, ansiSeq.size() - 3);

    // If this is a full reset, clear our cumulative state.
    if (codes == "0") {
        mLastColorBold = false;
        mLastSnoopColor = "";
    } else {
        // Tokenize the codes by ';'
        std::istringstream iss(codes);
        std::string token;
        while (std::getline(iss, token, ';')) {
            if (token == "1") {
                mLastColorBold = true;
            } else if (token == "22") { // Normal intensity – typically turns off bold.
                mLastColorBold = false;
            } else {
                // Check for standard foreground color codes (30-37 or 90-97).
                bool ok = false;
                int val = QString::fromStdString(token).toInt(&ok);
                if (ok && ((val >= 30 && val <= 37) || (val >= 90 && val <= 97))) {
                    mLastSnoopColor = token;
                }
            }
        }
    }

    // Reconstruct the cumulative ANSI sequence.
    std::string newState = "\033[";
    bool needSemicolon = false;
    if (mLastColorBold) {
        newState += "1";
        needSemicolon = true;
    }
    if (!mLastSnoopColor.empty()) {
        if (needSemicolon)
            newState += ";";
        newState += mLastSnoopColor;
    }
    newState += "m";
    mLastSgrState = newState;
}

/**
 * Handle remote client incoming snoop data.
 *
 * Client    | needsSkip | needsTrack | snoop Mudlet     | Mudlet can snoop
 * Mudlet    | no          yes          yes                yes
 * MudMaster | yes	       yes          yes                yes
 * TinTin    | ?           yes          no?                yes
 * MUSH      | yes         yes          yes (color issue)  yes
 * Zmud      | ?           ?            ?                  ?
 * Cmud      | ?           ?            ?                  ?
 *
 * mNeedsColorSkip and mNeedsColorTracking are defined when we receive
 * client version in handleIncomingClientVersion
 *
 */
void MMCPClient::handleIncomingSnoopData(const char* sData, quint16 len)
{
    const char* inScan = sData;
    const char* inEnd = inScan + len;
    std::stringstream ss;
    std::string outputMessage;

    // If the remote client has prepended color information,
    // skip over it as we'll be using our own.
    // MudMaster seems to do this, other clients (TT++) may not
    if (mNeedsColorSkip) {
        if (len < 4) {
            // Not enough data to skip color prefix - malformed packet?
            qWarning() << "MMCPClient::handleIncomingSnoopData() - Insufficient data for color skip (len=" << len << ")";
            return;
        }
        inScan += 4;
    }

    for (; inScan < inEnd; inScan++) {
        char c = *inScan;

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            // End of a line: get the current line.
            std::string line = ss.str();

            // For clients like MudMaster that do not resend the color,
            // if the line does not begin with an ANSI escape sequence,
            // prepend the last known color sequence.
            if (mNeedsColorTracking && !line.empty()) {
                line = mLastSgrState + line;
            }

            // Append the processed line
            outputMessage += line;

            ss.str("");
            ss.clear();
            continue;
        }

        // Look for ANSI escapes
        if (c == '\033') {
            std::string ansiSeq;
            ansiSeq.push_back(c);

            if (inScan + 1 < inEnd) {
                inScan++;
                char nextChar = *inScan;
                ansiSeq.push_back(nextChar);

                if (nextChar == '[') {

                    // Read until we hit an m (yes while bad)
                    while (inScan + 1 < inEnd) {
                        inScan++;
                        char ch = *inScan;
                        ansiSeq.push_back(ch);

                        if (ch == 'm') {
                            break;
                        }
                    }
                }
            }

            updateSgrState(ansiSeq);

            // Append the full ANSI sequence to the current line
            ss << ansiSeq;
            continue;
        }

        ss << c;
    }

    // Process any remaining data that didn't end with a newline
    std::string remaining = ss.str();
    if (!remaining.empty() && mNeedsColorTracking) {
        remaining = mLastSgrState + remaining;
    }

    outputMessage += remaining;

    if (!outputMessage.empty()) {

        postSnoopDataEvent(QString::fromStdString(outputMessage));

        if (mpHost->getMMCPShowSnoopInMainConsole()) {
            mpMMCPServer->snoopMessage(outputMessage);
        }
    }
}

void MMCPClient::postSnoopDataEvent(const QString& message) {
    TEvent event {};
    event.mArgumentList << csMMCPIncomingSnoopEvent;
    event.mArgumentList << mPeerName << message;
    event.mArgumentTypeList << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING;
    mpHost->raiseEvent(event);
}

void MMCPClient::postSideChannelMessage(const QString& from, const QString& channel, const QString& message)
{
    TEvent event {};
    event.mArgumentList << csMMCPChatSideChannelEvent;
    event.mArgumentList << from << channel << message;
    event.mArgumentTypeList << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING;
    mpHost->raiseEvent(event);
}

/**
 * Handle incoming side channel data from this client.
 * Posts a sysMMCPSideChannelMessage event with arguments:
 *     chatname, channel, message
 */
void MMCPClient::handleIncomingSideChannelData(const QString& stringData)
{
    QRegularExpression chatChannel("^(\\w+),(.*)$");

    QRegularExpressionMatch match = chatChannel.match(stringData);

    if (match.hasMatch()) {
        postSideChannelMessage(mPeerName, match.captured(1), match.captured(2));
    }

}

// To return exactly 8 ASCII characters:
const QString MMCPClient::getFlagsString()
{
    return qsl("%1%2%3%4%5%6%7%8")
            .arg(QLatin1Char(' '))
            .arg(QLatin1Char(' '))
            .arg(QLatin1Char(mPrivate ? 'P' : ' ')) // "Private"
            .arg(QLatin1Char(mIgnored ? 'I' : ' ')) // "Ignored"
            .arg(QLatin1Char(mServed ? 'S' : ' ')) // "Serving"
            .arg(QLatin1Char((convertToIPv4(mTcpSocket.peerAddress()) != mPeerAddress) ? 'F' : ' ')) // "Firewall" - Humm?
            .arg(QLatin1Char(mSnooping ? 'N' : (mEnableSnooping ? 'n' : ' ')))
            .arg(QLatin1Char(' '));
}

const QString MMCPClient::getInfoString(int colName, int colAddr, int colPort, int colGroup, int colFlags)
{
    using namespace AnsiColors;

    const QString groupStr = (mGroup == csDefaultMMCPGroupName) ? QString(QChar::Space).repeated(colGroup) : mGroup;

    return qsl("%1%6 %2 %3 %4 %5")
            .arg(mPeerName, -colName)
            .arg(convertToIPv4(mTcpSocket.peerAddress()), -colAddr)
            .arg(mTcpSocket.peerPort(), colPort)
            .arg(groupStr, -colGroup)
            .arg(getFlagsString(), -colFlags)
            .arg(RST);
}
