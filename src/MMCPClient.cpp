/***************************************************************************
 *   Copyright (C) 2024 by John McKisson - john.mckisson@gmail.com         *
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

MMCPClient::MMCPClient(Host* host, MMCPServer* serv) : m_state(Disconnected), tcpSocket(this), mpHost(host), server(serv)
{
    //auto ignore or ignorelist match?
    m_isIgnored = (1 == 0 ? true : false);

    //auto private or privatelist match?
    m_isPrivate = (1 == 0 ? true : false);

    //auto serve or servelist match?
    m_isServed = (1 == 0 ? true : false);

    m_isServing = false;
    m_canSnoop = false;
    m_isSnooped = false;
    m_isSnooping = false;
    m_group = tr("<none>");

    //Disable Nagle's algorithm
    tcpSocket.setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(&tcpSocket, &QTcpSocket::connected, this, &MMCPClient::slotConnected);
    connect(&tcpSocket, &QTcpSocket::disconnected, this, &MMCPClient::slotDisconnected);
    connect(&tcpSocket, &QTcpSocket::readyRead, this, &MMCPClient::slotReadData);
#if QT_VERSION >= 0x051500
    connect(&tcpSocket, &QAbstractSocket::errorOccurred, this, &MMCPClient::slotDisplayError);
#else
	connect(&tcpSocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &MMCPClient::slotDisplayError);
#endif
    connect(this, &MMCPClient::clientDisconnected, server, &MMCPServer::slotClientDisconnected);
}

/**
 * Attempt an outgoing connection to a client
 */
void MMCPClient::tryConnect(const QString& host, quint16 port)
{
    m_state = ConnectingOut;
    tcpSocket.connectToHost(host, port);
}


/**
 * Disconnect the client.  This will result in the disconnected() signal.
 */
void MMCPClient::disconnect()
{
    tcpSocket.disconnectFromHost();
}


/**
 * Handle an incoming chat connection.
 */
bool MMCPClient::incoming(qintptr socketDesc)
{
    m_state = ConnectingIn;
    return tcpSocket.setSocketDescriptor(socketDesc);
}

QString MMCPClient::host()
{
    return convertToIPv4(tcpSocket.peerAddress());
}

quint16 MMCPClient::port()
{
    return tcpSocket.peerPort();
}


/**
 * Outgoing connection established, send chat request.
 */
void MMCPClient::slotConnected()
{
    m_host = convertToIPv4(tcpSocket.peerAddress());
    m_port = tcpSocket.peerPort();

    QString str = QString("CHAT:%1\n%3%2").arg(server->getChatName()).arg(m_port, -5).arg(m_host);

    tcpSocket.write(str.toLatin1());

    const QString infoMsg = tr("[ CHAT ]  - Waiting for response from %1:%2.").arg(m_host).arg(m_port);
    mpHost->postMessage(infoMsg);
}

/**
 * A client has been disconected.
 * Clear snoop flags if we were snooping or they were snooping us.
 */
void MMCPClient::slotDisconnected()
{
    const QString infoMsg = tr("[ CHAT ]  - You are now disconnected from %1:%2.").arg(m_host).arg(m_port);
    mpHost->postMessage(infoMsg);

    if (isSnooping()) {
        setSnooping(false);
        server->decrementSnoopCount();
    }

    if (isSnooped())
        setSnooped(false);

    emit clientDisconnected(this);
}

/**
 * Process incoming data from the client socket
 */
void MMCPClient::slotReadData()
{
    while (tcpSocket.bytesAvailable()) {
        buffer.append(tcpSocket.read(tcpSocket.bytesAvailable()));
    }

    switch (m_state) {
    case ConnectingIn: {
        QRegularExpression chatIncoming("^CHAT:(.*)\n(\\d{1,3}(?:\\.\\d{1,3}){3})(\\d{1,5})\\s*$");
        //QRegularExpression chatIncoming("CHAT:(.*)\n(.*)$");

        QRegularExpressionMatch match = chatIncoming.match(buffer);

        if (match.hasMatch()) {
            //Check auto accept?
            m_state = Connected;

            m_chatName = match.captured(1);
            m_host = match.captured(2);
            m_port = match.captured(3).toUInt();

            writeData(QString("YES:%1\n").arg(server->getChatName()));

            const QString infoMsg = tr("[ CHAT ]  - Connection from %1 at %2:%3 accepted.")
                                    .arg(m_chatName)
                                    .arg(convertToIPv4(tcpSocket.peerAddress()))
                                    .arg(tcpSocket.peerPort());

            mpHost->postMessage(infoMsg);

            server->addConnectedClient(this);

            sendVersion();
        } else {
            m_state = Disconnected;
        }

        break;
    }

    case ConnectingOut: {
        //Should receive either YES or NO
        QRegularExpression chatAccepted("^YES:(.*)\n(.*)$");

        QRegularExpressionMatch match = chatAccepted.match(buffer);

        if (match.hasMatch()) {
            m_chatName = match.captured(1);
            m_state = Connected;

            const QString infoMsg = tr("[ CHAT ]  - Connection from %1 at %2:%3 accepted.")
                                    .arg(m_chatName)
                                    .arg(convertToIPv4(tcpSocket.peerAddress()))
                                    .arg(tcpSocket.peerPort());

            mpHost->postMessage(infoMsg);

            server->addConnectedClient(this);
            sendVersion();

            if (chatAccepted.captureCount() > 2) {
                //A command was tacked on the end of the accept string
                handleConnectedState(match.captured(2).toLatin1());
            }
        } else {
            m_state = Disconnected;

            const QString infoMsg = tr("[ CHAT ]  - Connection from %1:%2 refused.")
                                    .arg(tcpSocket.peerPort())
                                    .arg(convertToIPv4(tcpSocket.peerAddress()));
            mpHost->postMessage(infoMsg);
        }


        break;
    }

    case Connected:
        if (buffer.lastIndexOf('\xff') == -1)
            return;

        handleConnectedState(buffer);
        break;

    default:
        qDebug("unknown client state");
    }

    buffer.clear();
}


void MMCPClient::slotDisplayError(QAbstractSocket::SocketError socketError)
{
    QString message;

    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        message = tr("The host was not found. Please check the host name and port settings.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        message = tr("The connection was refused by the peer.");
        break;
    default:
        message = tr("The following error occurred: %1.").arg(tcpSocket.errorString());
    }

    server->clientMessage(message);
}


/**
 * Send a Message to the client socket
 */
void MMCPClient::sendMessage(const QString& msg)
{
    writeData(QString("%1%2%3")
                .arg(static_cast<char>(Message))
                .arg(msg)
                .arg(static_cast<char>(End)));
}


/**
 * Send a ping request to this client.
 */
void MMCPClient::sendPingRequest()
{
    writeData(QString("%1%2%3")
                .arg(static_cast<char>(PingRequest))
                .arg(QDateTime::currentMSecsSinceEpoch())
                .arg(static_cast<char>(End)));

    const QString infoMsg = tr("[ CHAT ]  - Pinging %1...").arg(m_chatName);
    mpHost->postMessage(infoMsg);
}

/**
 * Send a peek connections request to this client.
 */
void MMCPClient::sendPeekRequest()
{
    writeData(QString("%1%2")
                .arg(static_cast<char>(PeekConnections))
                .arg(static_cast<char>(End)));

    const QString infoMsg = tr("[ CHAT ]  - Attempting to peek at %1's public connections...").arg(m_chatName);
    mpHost->postMessage(infoMsg);
}

/**
 * Request client connections list
 */
void MMCPClient::sendRequestConnections()
{
    writeData(QString("%1%2")
                .arg(static_cast<char>(RequestConnections))
                .arg(static_cast<char>(End)));

    const QString infoMsg = tr("[ CHAT ]  - Requested connections from %1").arg(m_chatName);
    mpHost->postMessage(infoMsg);
}


/**
 * Send client version
 */
void MMCPClient::sendVersion()
{
    writeData(QString("%1%2 %3%4")
                .arg(static_cast<char>(Version))
                .arg(mudlet::self()->scmVersion)
                .arg(" (Humera's MMCP Test)")
                .arg(static_cast<char>(End)));
}

/**
 * Assign a client to a group.
 * If supplied group is 'none' or empty string, remove from group. 
 * Returns true if they were assigned to a group, false if they were removed.
 */
bool MMCPClient::setGroup(const QString& group)
{
    if ("none" == group.toLower() || group.length() == 0) {
        m_group = "<none>";
        return false;
    }

    m_group = group;
    return true;
}

/**
 * Write to this client socket.
 */
void MMCPClient::writeData(const QString& data)
{
    tcpSocket.write(data.toLatin1());
}

/**
 * Attempt to snoop this client.
 */
void MMCPClient::snoop()
{
    writeData(QString("%1%2")
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
        char cmd = data[cmdIdx];

        int endIdx = bytes.indexOf(0xFF, cmdIdx);
        if (endIdx == -1) {
            qDebug() << "wtf?";
            endIdx = bytes.length();
        }

        QString stringData = QString::fromLatin1(data + cmdIdx + 1, (endIdx - cmdIdx) - 1);

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
            server->clientMessage(stringData);
            break;

        case MMCPChatCommand::Version:
            version = stringData;
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

        default:
            qDebug() << "unknown command: " << cmd << stringData;
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
    server->clientMessage(list);
}

/**
 * Someone requested that we give them our public connections
 */
void MMCPClient::handleIncomingConnectionsRequest()
{
    if (m_isIgnored) {
        const QString infoMsg = tr("[ CHAT ]  - %1 is trying to request your connections!").arg(m_chatName);
        mpHost->postMessage(infoMsg);
        return;
    }

    if (mpHost->getMMCPAllowConnectionRequests()) {
        const QString infoMsg = tr("[ CHAT ]  - %1 has requested your public connections...").arg(m_chatName);
        mpHost->postMessage(infoMsg);

        server->sendPublicConnections(this);
    } else {
        const QString infoMsg = tr("[ CHAT ]  - %1 has requested your public connections, but you're ignoring connection requests...").arg(m_chatName);
        mpHost->postMessage(infoMsg);
    }
}

/**
 * Display a chat all message and echo it to any clients we may be serving
 */
void MMCPClient::handleIncomingChatEveryone(const QString& msg)
{
    if (m_isIgnored)
        return;

    server->clientMessage(msg);

    if (m_isServed) {
        //Echo this message to
        server->sendServedMessage(this, msg);
    } else {
        //Echo message to other clients we might be serving
        server->sendMessageToServed(this, msg);
    }
}

/**
 * Display an incoming personal chat
 */
void MMCPClient::handleIncomingChatPersonal(const QString& msg)
{
    server->clientMessage(msg);
}

/**
 * Display an incoming chat to a group
 */
void MMCPClient::handleIncomingChatGroup(const QString& msg)
{
    const QString groupStr = msg.left(15).trimmed();
    const QString trimmedMsg = msg.right(msg.length() - 15);

    using namespace AnsiColors;

    const QString groupMsg = QString("%1%2(%3%4%5)%6")
                                .arg(RST).arg(FBLDRED).arg(FBLDCYN)
                                .arg(groupStr).arg(FBLDRED)
                                .arg(trimmedMsg);

    server->clientMessage(groupMsg);
}

/**
 * A client changed their name
 */
void MMCPClient::handleIncomingNameChange(const QString& newName)
{
    const QString infoMsg = tr("[ CHAT ]  - %1 is now known as %2.").arg(m_chatName).arg(newName);
    mpHost->postMessage(infoMsg);
    m_chatName = newName;
}

/**
 * Someone has requested to peek our connections
 */
void MMCPClient::handleIncomingPeekConnections()
{
    //check if this client is ignored before doing anything drastic
    if (m_isIgnored) {
        const QString infoMsg = tr("[ CHAT ]  - %1 is trying to peek your connections!").arg(m_chatName);
        mpHost->postMessage(infoMsg);
        return;
    }

    if (mpHost->getMMCPAllowPeekRequests()) {
        const QString infoMsg = tr("[ CHAT ]  - %1 is peeking at your connections...").arg(m_chatName);
        mpHost->postMessage(infoMsg);

        server->sendPublicPeek(this);
    } else {
        const QString infoMsg = tr("[ CHAT ]  - %1 is trying to peek your connections, but you're ignoring peek requests...").arg(m_chatName);
        mpHost->postMessage(infoMsg);
    }
}

/**
 * Display someones peek list that we've requested
 */
void MMCPClient::handleIncomingPeekList(const QString& list)
{
    server->clientMessage(list);

    QStringList parts = list.split("~");

    if (parts.size() % 3 != 0) {
        const QString infoMsg = tr("[ CHAT ]  - Badly formatted peek list from %1").arg(m_chatName);
        mpHost->postMessage(infoMsg);
        return;
    }

    QString listOut = QString("     Name                 Address         Port\n");
    listOut.append("     ==================== =============== =====\n");

    using namespace AnsiColors;

    quint16 count = 1;
    for (int i = 0; i < parts.size(); i += 3) {
        const QString host = parts.at(i);
        const QString port = parts.at(i + 1);
        const QString name = parts.at(i + 2);

        listOut.append(QString("%1%2:%3 %4 %5 %6\n").arg(FBLDWHT).arg(count++, 3, QChar('0')).arg(RST).arg(name).arg(host).arg(port));
    }

    server->clientMessage(listOut);
}

/**
 * Respond to someones ping request
 */
void MMCPClient::handleIncomingPingRequest(const QString& msg)
{
    writeData(QString("%1%2%3")
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
        const QString infoMsg = tr("[ CHAT ]  - Ping returned from %1: %2 ms").arg(m_chatName).arg(QDateTime::currentMSecsSinceEpoch() - returnTime);
        mpHost->postMessage(infoMsg);
    } else {
        const QString infoMsg = tr("[ CHAT ]  - Bad Ping response from %1: %2").arg(m_chatName).arg(data);
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
        sendMessage(QString("<CHAT> You do not have permission to snoop %1.").arg(server->getChatName()));
        return;
    }

    if (isSnooping()) {
        setSnooping(false);
        server->decrementSnoopCount();

        const QString infoMsg = tr("[ CHAT ]  - %1 has stopped snooping you.").arg(m_chatName);
        mpHost->postMessage(infoMsg);
        sendMessage(QString("<CHAT> You have stopped snooping %1.").arg(server->getChatName()));

    } else {
        setSnooping(true);
        server->incrementSnoopCount();

        const QString infoMsg = tr("[ CHAT ]  - %1 has begun snooping you.").arg(m_chatName);
        mpHost->postMessage(infoMsg);
        sendMessage(QString("<CHAT> You have stopped snooping %1.").arg(server->getChatName()));
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

        if (c == '\r')
            continue;

        if (c == '\n') {
            ss.clear();
            ss.str("");
            continue;
        }

        ss << *inScan;
    }

    if (ss.tellp() > 0) {
        qDebug() << "no CR";
        server->snoopMessage(ss.str());
    }
}

const QString& MMCPClient::getVersion()
{
    return version;
}

const QString MMCPClient::getFlagsString()
{
    return QString("%1%2%3%4%5%6%7%8")
            //.arg(GetCommands() ? 'A' : ' ')
            .arg(' ')
            //.arg(GetTransfers() ? 'T' : ' '),
            .arg(' ')
            .arg(m_isPrivate ? 'P' : ' ')
            .arg(m_isIgnored ? 'I' : ' ')
            .arg(m_isServed ? 'S' : ' ')
            //.arg(GetExcludeServe() ? 'X' : ' '),
            .arg(convertToIPv4(tcpSocket.peerAddress()) != m_host ? 'F' : ' ')
            .arg(m_isSnooping ? 'N' : (m_canSnoop ? 'n' : ' '))
            //.arg(' ')
            .arg(' ');
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

    const QString groupStr = m_group == "<none>" ? "               " : m_group;

    return QString("%1 %2 %3 %4 %5")
                .arg(m_chatName, -20)
                .arg(convertToIPv4(tcpSocket.peerAddress()), -20)
                .arg(tcpSocket.peerPort(), -5)
                .arg(groupStr, -15)
                .arg(getFlagsString(), -8);
}
