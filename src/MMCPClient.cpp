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

#include "MMCPClient.h"
#include "Host.h"
#include "MMCPServer.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QHostAddress>
#include <QTcpSocket>
#include <QRegularExpression>
#include <QtGlobal>
#include "post_guard.h"

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
{
    //Disable Nagle's algorithm
    mTcpSocket.setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(&mTcpSocket, &QTcpSocket::connected, this, &MMCPClient::slot_connected);
    connect(&mTcpSocket, &QTcpSocket::disconnected, this, &MMCPClient::slot_disconnected);
    connect(&mTcpSocket, &QTcpSocket::readyRead, this, &MMCPClient::slot_readData);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(&mTcpSocket, &QAbstractSocket::errorOccurred, this, &MMCPClient::slot_displayError);
#else
    connect(&mTcpSocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &MMCPClient::slot_displayError);
#endif
    connect(this, &MMCPClient::signal_clientDisconnected, mpMMCPServer, &MMCPServer::slot_clientDisconnected);
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

    QString str = qsl("CHAT:%1\n%2%3").arg(mpMMCPServer->getChatName()).arg(mPeerAddress).arg(mPeerPort, -5);

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
 * Process incoming data from the client socket
 */
void MMCPClient::slot_readData()
{
    while (mTcpSocket.bytesAvailable()) {
        if (mPeerBuffer.length() > 0) {
            qDebug().noquote().nospace() << "MMCPCLient::slot_readData() INFO - appending to previous partial buffer.";
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
        }

        const QByteArray peerName = mPeerBuffer.mid(5, nlPos - 5);
        const QByteArray ipAndPort = mPeerBuffer.mid(nlPos + 1);
        // Exclude the last 5 characters for the IP address
        const QByteArray ipAddress = ipAndPort.left(ipAndPort.size() - 5);
        // Last 5 characters for the port
        const QByteArray port = ipAndPort.right(5);

        QHostAddress host(QString::fromUtf8(ipAddress));
        if (host.isNull()) {
            mState = Disconnected;
        }

        // Check auto accept?
        if (mpMMCPServer->isDoNotDisturb()) {
            mState = Disconnected;
            writeData(qsl("NO:%1\n").arg(mpMMCPServer->getChatName()));

            const QString infoMsg = tr("[ CHAT ]  - Connection from %1 at %2:%3 denied (DoNotDisturb).")
                                            .arg(mPeerName,
                                                 convertToIPv4(mTcpSocket.peerAddress()),
                                                 QString::number(mTcpSocket.peerPort()));

            mpHost->postMessage(infoMsg);
        } else {

            mPeerName = QString::fromUtf8(peerName);
            mPeerAddress = convertToIPv4(host);
            bool ok;
            mPeerPort = QString::fromUtf8(port).toUInt(&ok);
            if (!ok) {
                mState = Disconnected;
            } else {
                mState = Connected;
                writeData(QString("YES:%1\n").arg(mpMMCPServer->getChatName()));

                const QString infoMsg = tr("[ CHAT ]  - Connection from %1 at %2:%3 accepted.")
                                                 .arg(mPeerName,
                                                      convertToIPv4(mTcpSocket.peerAddress()),
                                                      QString::number(mTcpSocket.peerPort()));

                mpHost->postMessage(infoMsg);

                mpMMCPServer->addConnectedClient(this);

                sendVersion();
            }
        }

        break;
    }

    case ConnectingOut: {
        // We have requested to another peer and they should be accepting or
        // rejecting our attempt with either a YES or a NO:
        const int colonPos = mPeerBuffer.indexOf(':');
        const int nlPos = mPeerBuffer.indexOf('\n');

        if (!(mPeerBuffer.startsWith("YES:") || mPeerBuffer.startsWith("NO:")) || nlPos == -1 || colonPos == -1) {
            mState = Disconnected;

            // In this case we do not get details of the connection from the
            // other end - instead we can only report the apparent details from
            // the socket:
            const QString infoMsg = tr("[ CHAT ]  - Connection to %1:%2 refused.")
                                            .arg(convertToIPv4(mTcpSocket.peerAddress()),
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

        handleConnectedState(mPeerBuffer);
        break;

    default:
        qDebug("unknown client state");
    }

    mPeerBuffer.clear();
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
}


/**
 * Send a Message to the client socket
 */
void MMCPClient::sendMessage(const QString& msg)
{
    writeData(qsl("%1%2%3")
                      .arg(static_cast<char>(Message))
                      .arg(msg)
                      .arg(static_cast<char>(End)));
}


/**
 * Send a ping request to this client.
 */
void MMCPClient::sendPingRequest()
{
    writeData(qsl("%1%2%3")
                      .arg(static_cast<char>(PingRequest))
                      .arg(QString::number(QDateTime::currentMSecsSinceEpoch()))
                      .arg(static_cast<char>(End)));

    const QString infoMsg = tr("[ CHAT ]  - Pinging %1...").arg(mPeerName);
    mpHost->postMessage(infoMsg);
}

/**
 * Send a peek connections request to this client.
 */
void MMCPClient::sendPeekRequest()
{
    writeData(qsl("%1%2")
                      .arg(static_cast<char>(PeekConnections))
                      .arg(static_cast<char>(End)));

    const QString infoMsg = tr("[ CHAT ]  - Attempting to peek at %1's public connections...").arg(mPeerName);
    mpHost->postMessage(infoMsg);
}

/**
 * Request client connections list
 */
void MMCPClient::sendRequestConnections()
{
    writeData(qsl("%1%2")
                      .arg(static_cast<char>(RequestConnections))
                      .arg(static_cast<char>(End)));

    const QString infoMsg = tr("[ CHAT ]  - Requested connections from %1").arg(mPeerName);
    mpHost->postMessage(infoMsg);
}


/**
 * Send client version
 */
void MMCPClient::sendVersion()
{
    writeData(qsl("%1%2%4")
                      .arg(static_cast<char>(Version))
                      .arg(mudlet::self()->scmVersion)
                      .arg(static_cast<char>(End)));
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
    mTcpSocket.write(data.toLatin1());
}

/**
 * Attempt to snoop this client.
 */
void MMCPClient::snoop()
{
    writeData(qsl("%1%2")
                      .arg(static_cast<char>(Snoop))
                      .arg(static_cast<char>(End)));
}

/**
 * Process incoming data from this client socket.
 * Find and handle chat commands.
 */
void MMCPClient::handleConnectedState(const QByteArray& bytes)
{
    const char* data = bytes.data();
    int cmdIdx = 0;
    while (cmdIdx < bytes.length()) {
        const char cmd = data[cmdIdx];

        const int endIdx = bytes.indexOf(MMCPChatCommand::End, cmdIdx);
        if (endIdx == -1) {
            qDebug().noquote().nospace() << "MMCPClient::handleConnectedState(...) INFO - partial buffer detected...";
            return;
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
            mpMMCPServer->clientMessage(stringData);
            break;

        case MMCPChatCommand::Version:
            mPeerVersion = stringData;
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
}

/**
 * We were sent a connection list, we're supposed to connect to the clients
 * given to us
 */
void MMCPClient::handleIncomingConnectionList(const QString& list)
{
    // Maybe check option if we want to auto call everyone in this list
    mpMMCPServer->clientMessage(list);
}

/**
 * Someone requested that we give them our public connections
 */
void MMCPClient::handleIncomingConnectionsRequest()
{
    if (mIsIgnored) {
        const QString infoMsg = tr("[ CHAT ]  - %1 is trying to request your connections!").arg(mPeerName);
        mpHost->postMessage(infoMsg);
        return;
    }

    if (mpHost->getMMCPAllowConnectionRequests()) {
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
 */
void MMCPClient::handleIncomingChatEveryone(const QString& msg)
{
    if (mIsIgnored) {
        return;
    }

    mpMMCPServer->clientMessage(msg);

    if (mIsServed) {
        //Echo this message to
        mpMMCPServer->sendServedMessage(this, msg);
    } else {
        //Echo message to other clients we might be serving
        mpMMCPServer->sendMessageToServed(this, msg);
    }
}

/**
 * Display an incoming personal chat
 */
void MMCPClient::handleIncomingChatPersonal(const QString& msg)
{
    mpMMCPServer->clientMessage(msg);
}

/**
 * Display an incoming chat to a group
 */
void MMCPClient::handleIncomingChatGroup(const QString& msg)
{
    const QString groupStr = msg.left(15).trimmed();
    const QString trimmedMsg = msg.right(msg.length() - 15);

    using namespace AnsiColors;

    //: Incoming group message
    const QString groupMsg = tr("%1%2<CHAT>%3(%4)%1%2%5%1")
                                     .arg(RST)
                                     .arg(FBLDRED)
                                     .arg(FBLDCYN)
                                     .arg(groupStr)
                                     .arg(trimmedMsg);

    mpMMCPServer->clientMessage(groupMsg);
}

/**
 * A client changed their name
 */
void MMCPClient::handleIncomingNameChange(const QString& newName)
{
    const QString infoMsg = tr("[ CHAT ]  - %1 is now known as %2.").arg(mPeerName).arg(newName);
    mpHost->postMessage(infoMsg);
    mPeerName = newName;
}

/**
 * Someone has requested to peek our connections
 */
void MMCPClient::handleIncomingPeekConnections()
{
    //check if this client is ignored before doing anything drastic
    if (mIsIgnored) {
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
    const QStringList parts = list.split("~");
    if (parts.size() % 3 != 1) {
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
                               "==== ==================== =============== =====\n")
                                    .arg(messageList.join(QChar::LineFeed));
    mpMMCPServer->clientMessage(listOut);
}

/**
 * Respond to someones ping request
 */
void MMCPClient::handleIncomingPingRequest(const QString& msg)
{
    writeData(qsl("%1%2%3")
                      .arg(static_cast<char>(PingResponse))
                      .arg(msg)
                      .arg(static_cast<char>(End)));
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
        const QString infoMsg = tr("[ CHAT ]  - Bad Ping response from %1: %2").arg(mPeerName).arg(data);
        mpHost->postMessage(infoMsg);
    }
}


/**
 * This client sent a request to snoop you (the user).
 * Apparently the client is responsible for telling the connecting user.
 */
void MMCPClient::handleIncomingSnoop()
{
    if (!canSnoop()) {
        sendMessage(qsl("<CHAT> You do not have permission to snoop %1.").arg(mpMMCPServer->getChatName()));
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
        sendMessage(qsl("<CHAT> You have stopped snooping %1.").arg(mpMMCPServer->getChatName()));
    }
}

/**
 * Handle someone's incoming snoop data.
 * Skip over the color data they sent as we'll be using our own
 */
void MMCPClient::handleIncomingSnoopData(const char* sData, quint16 len)
{
    const char* inScan = sData;
    const char* inEnd = inScan + len;
    std::stringstream ss;

    //Skip over fore and back colors
    inScan += 4;

    for (; inScan < inEnd; inScan++) {
        char c = *inScan;

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            ss.clear();
            ss.str("");
            continue;
        }

        ss << *inScan;
    }

    if (ss.tellp() > 0) {
        qDebug() << "no CR";
        mpMMCPServer->snoopMessage(ss.str());
    }
}

/**
 * Handle incoming side channel data from this client.
 * Posts a sysChatChannelMessage event with arguments:
 *     chatname, channel, message
 */ 
void MMCPClient::handleIncomingSideChannelData(const QString& stringData)
{
    QRegularExpression chatChannel("^\\[(\\w+)\\](.*)$");

    QRegularExpressionMatch match = chatChannel.match(stringData);

    if (match.hasMatch()) {
        mpHost->postChatChannelMessage(mPeerName, match.captured(1), match.captured(2));
    }

}

// To return exactly 8 ASCII characters:
const QString MMCPClient::getFlagsString()
{
    return qsl("%1%2%3%4%5%6%7%8")
            //.arg(GetCommands() ? 'A' : ' ') - "Allow commands" - we don't do those
            .arg(QLatin1Char(' '))
            //.arg(GetTransfers() ? 'T' : ' ') - "Allows transfers" - not exactly - we can't know that until a transfer has been accepted/rejected
            .arg(QLatin1Char(' '))
            .arg(QLatin1Char(mIsPrivate ? 'P' : ' ')) // "Private"
            .arg(QLatin1Char(mIsIgnored ? 'I' : ' ')) // "Ignored"
            .arg(QLatin1Char(mIsServed ? 'S' : ' ')) // "Serving"
            //.arg(GetExcludeServe() ? 'X' : ' ') - "Serving excluded" - not suported?
            .arg(QLatin1Char((convertToIPv4(mTcpSocket.peerAddress()) != mPeerAddress) ? 'F' : ' ')) // "Firewall" - Humm?
            .arg(QLatin1Char(mIsSnooping ? 'N' : (mEnableSnooping ? 'n' : ' ')))
            //.arg(' ') - something else?
            .arg(QLatin1Char(' '));
}

const QString MMCPClient::getInfoString()
{
    /*
	QString strName;
	
	// Color the name to reflect the status of a transfer.
	
	switch(GetTransferType()) {
		case CChat::TransferType::None:
			strName = (GetName().GetLength() > 20 ? GetName().Left(20) : GetName());
			break;

		case CChat::TransferType::Send:
			strName.Format("\x1b[1;32m%s\x1b[0;37m",
				(GetName().GetLength() > 20 ? (LPCSTR)GetName().Left(20) : (LPCSTR)GetName()));
			break;

		case CChat::TransferType::Receive:
			strName.Format("\x1b[1;34m%s\x1b[0;37m",
				(GetName().GetLength() > 20 ? (LPCSTR)GetName().Left(20) : (LPCSTR)GetName()));
			break;
	}
	*/

    const QString groupStr = (mGroup == csDefaultMMCPGroupName) ? QString(QChar::Space).repeated(15) : mGroup;

    return qsl("%1 %2 %3 %4 %5")
            .arg(mPeerName, -20)
            .arg(convertToIPv4(mTcpSocket.peerAddress()), -20)
            .arg(mTcpSocket.peerPort(), 5)
            .arg(groupStr, -15)
            .arg(getFlagsString(), -8);
}
