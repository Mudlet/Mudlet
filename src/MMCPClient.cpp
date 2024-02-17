#include "Host.h"
#include "mudlet.h"
#include "MMCPClient.h"
#include "MMCPServer.h"

#include "pre_guard.h"
#include <QAbstractSocket>
#include <QRegularExpression>
#include "post_guard.h"

MMCPClient::MMCPClient(Host *host, MMCPServer *serv)
	: m_state(Disconnected), tcpSocket(this), mpHost(host), server(serv) {
	
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
	
	//Disable Nagle's algorithm
	tcpSocket.setSocketOption(QAbstractSocket::LowDelayOption, 1);

	connect(&tcpSocket, &QTcpSocket::connected,          this, &MMCPClient::slotConnected);
	connect(&tcpSocket, &QTcpSocket::disconnected,       this, &MMCPClient::slotDisconnected);
	connect(&tcpSocket, &QTcpSocket::readyRead,          this, &MMCPClient::slotReadData);
	connect(&tcpSocket, &QAbstractSocket::errorOccurred, this, &MMCPClient::slotDisplayError);

	connect(this, &MMCPClient::clientDisconnected, server, &MMCPServer::slotClientDisconnected);
}

/**
 * Attempt an outgoing connection to a client
 */
void MMCPClient::tryConnect(const QString& host, quint16 port) {
	m_state = ConnectingOut;
	tcpSocket.connectToHost(host, port);
}


/**
 * Disconnect the client.  This will result in the disconnected() signal.
 */
void MMCPClient::disconnect() {
	tcpSocket.disconnectFromHost();
}


/**
 * Handle an incoming chat connection.
 */
bool MMCPClient::incoming(qintptr socketDesc) {
	m_state = ConnectingIn;
	return tcpSocket.setSocketDescriptor(socketDesc);
}

QString MMCPClient::host() {
	return tcpSocket.peerAddress().toString();
}

quint16 MMCPClient::port() {
	return tcpSocket.peerPort();
}


/**
 * Outgoing connection established, send chat request.
 */
void MMCPClient::slotConnected() {
	m_host = tcpSocket.peerAddress().toString();
	m_port = tcpSocket.peerPort();

	QString str = QString("CHAT:%1\n%3%2").arg(server->getChatName()).arg(m_port, 5).arg(m_host);

	tcpSocket.write(str.toLatin1());
	
	server->clientMessage(QString("<CHAT> Waiting for response from %2:%1 ...").arg(m_port).arg(m_host));
}

/**
 * A client has been disconected.
 * Clear snoop flags if we were snooping or they were snooping us.
 */
void MMCPClient::slotDisconnected() {
	server->clientMessage(QString("<CHAT> You are disconnected from %2:%1 ...").arg(m_port).arg(m_host));
	
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
void MMCPClient::slotReadData() {
		
	while (tcpSocket.bytesAvailable()) {
		buffer.append(tcpSocket.read(tcpSocket.bytesAvailable()));
	}
	
	switch (m_state) {
		case ConnectingIn: {

			//QRegularExpression chatIncoming("CHAT:(.*)\n([0-9.]+)$");
			QRegularExpression chatIncoming("CHAT:(.*)\n(.*)$");
			
            QRegularExpressionMatch match = chatIncoming.match(buffer);

            if (match.hasMatch()) {
				//Check auto accept?
				m_state = Connected;
				
				m_chatName = match.captured(1);
				
				writeData(QString("YES:%1\n").arg(server->getChatName()));
										
				server->clientMessage(QString("<CHAT> Connection from %1 at %3:%2 accepted.")
										.arg(m_chatName)
										.arg(tcpSocket.peerPort())
										.arg(tcpSocket.peerAddress().toString()));
				
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
										
				server->clientMessage(QString("<CHAT> Connection to %1 at %3:%2 accepted.")
										.arg(m_chatName)
										.arg(tcpSocket.peerPort())
										.arg(tcpSocket.peerAddress().toString()));
				
				server->addConnectedClient(this);
				sendVersion();
				
				if (chatAccepted.captureCount() > 2) {
					//A command was tacked on the end of the accept string
					handleConnectedState(match.captured(2).toLatin1());
				}
			 } else {
                m_state = Disconnected;
										
				server->clientMessage(QString("<CHAT> Connection to %2:%1 refused.")
										.arg(tcpSocket.peerPort())
										.arg(tcpSocket.peerAddress().toString()));
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


void MMCPClient::slotDisplayError(QAbstractSocket::SocketError socketError) {

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
 * Send a chat message to this client, either Everyone or Personal.
 */
void MMCPClient::sendChat(const QString &msg, MMCPChatCommands command) {
	QString output;

	switch (command) {
		case TextEveryone:
			output = msg;
			break;
			
		case TextPersonal:
			output = QString("%1%2 chats to you, '%3'\n%4")
							.arg((char)command)
							.arg(server->getChatName())
							.arg(msg)
							.arg((char)End);
			break;
	}
	
	writeData(output);
}

/**
 * Send a Message to the client socket
 */
void MMCPClient::sendMessage(const QString &msg) {
	writeData(QString("%1%2%3")	.arg((char)Message)
								.arg(msg)
								.arg((char)End));
}


/**
 * Send a ping request to this client.
 */
void MMCPClient::sendPingRequest() {
	
	writeData(QString("%1%2%3")	.arg((char)PingRequest)
								.arg(QDateTime::currentMSecsSinceEpoch())
								.arg((char)End));

	server->clientMessage(QString("<CHAT> Pinging %1...").arg(m_chatName));
}

/**
 * Request client connections list
 */
void MMCPClient::sendRequestConnections() {
	writeData(QString("%1%2")	.arg((char)RequestConnections)
								.arg((char)End));

	server->clientMessage(QString("<CHAT> Requested connections from %1").arg(m_chatName));
}


/**
 * TODO: Implement app name and version globals
 */
void MMCPClient::sendVersion() {
	writeData(QString("%1%2 %3%4")	.arg((char)Version)
									.arg(mudlet::self()->scmVersion)
									.arg(" (Humera's MMCP Test)")
									.arg((char)End));
}

/**
 * Write to this client socket.
 */
void MMCPClient::writeData(const QString &data) {
	tcpSocket.write(data.toLatin1());
}

/**
 * Attempt to snoop this client.
 */
void MMCPClient::snoop() {
	writeData(QString("%1%2")	.arg((char)Snoop)
								.arg((char)End));
}

/**
 * Process incoming data from this client socket.
 * Find and handle chat commands.
 */
void MMCPClient::handleConnectedState(const QByteArray &bytes) {
	const char *data = bytes.data();
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
void MMCPClient::handleIncomingConnectionList(const QString &list) {
	// TODO: check option if we want to auto call everyone in this list
	server->clientMessage(list);
}

/**
 * Someone requested that we give them our public connections
 */
void MMCPClient::handleIncomingConnectionsRequest() {

	if (m_isIgnored) {
		server->clientMessage(QString("<CHAT> %1 is trying to request your connections!").arg(m_chatName));
		return;
	}

	if (mpHost->getMMCPAllowConnectionRequests()) {

		server->clientMessage(QString("<CHAT> %1 has requested your public connections...").arg(m_chatName));

		server->sendPublicConnections(this);
	} else {
		server->clientMessage(QString("<CHAT> %1 has requested your public connections, but you're ignoring connection requests...").arg(m_chatName));
	}
}

/**
 * Display a chat all message and echo it to any clients we may be serving
 */
void MMCPClient::handleIncomingChatEveryone(const QString &msg) {
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
void MMCPClient::handleIncomingChatPersonal(const QString &msg) {
	server->clientMessage(msg);
}

/**
 * A client changed their name
 */
void MMCPClient::handleIncomingNameChange(const QString &newName) {
	server->clientMessage(QString("<CHAT> %1 is now known as %2.").arg(m_chatName).arg(newName));
	m_chatName = newName;
}

/**
 * Someone has requested to peek our connections
 */
void MMCPClient::handleIncomingPeekConnections() {
	//check if this client is ignored before doing anything drastic
	if (m_isIgnored) {
		server->clientMessage(QString("<CHAT> %1 is trying to peek your connections!").arg(m_chatName));
		return;
	}

	if (mpHost->getMMCPAllowPeekRequests()) {

		server->clientMessage(QString("<CHAT> %1 is peeking at your connections...").arg(m_chatName));

		server->sendPublicPeek(this);
	} else {
		server->clientMessage(QString("<CHAT> %1 is peeking at your connections, but you're ignoring peek requests...").arg(m_chatName));
	}
}

/**
 * Display someones peek list that we've requested
 */
void MMCPClient::handleIncomingPeekList(const QString &list) {
	server->clientMessage(list);

	QStringList parts = list.split("~");

	if (parts.size() % 3 != 0) {
		server->clientMessage(QString("<Chat Error> Badly formatted peek list from %1").arg(m_chatName));
		return;
	}

	QString listOut = QString( "     Name                 Address         Port\n");
				listOut.append("     ==================== =============== =====\n");

	using namespace AnsiColors;

	quint16 count = 1;
	for (int i = 0; i < parts.size(); i += 3) {
		const QString host = parts.at(i);
		const QString port = parts.at(i + 1);
		const QString name = parts.at(i + 2);

		listOut.append(QString("%1%2:%3 %4 %5 %6\n")
			.arg(FBLDWHT)
			.arg(count++, 3, QChar('0'))
			.arg(RST)
			.arg(name)
			.arg(host)
			.arg(port));

	}

	server->clientMessage(listOut);
}

/**
 * Respond to someones ping request
 */
void MMCPClient::handleIncomingPingRequest(const QString &msg) {
	writeData(QString("%1%2%3")	.arg((char)PingResponse)
								.arg(msg)
								.arg((char)End));
}

/**
 * Handle ping data that we've requested
 */
void MMCPClient::handleIncomingPingResponse(const QString &data) {
	bool ok;
	qint64 returnTime = data.toLongLong(&ok);
	
	if (ok){
		server->clientMessage(QString("<CHAT> Ping returned from %1: %2 ms")
								.arg(m_chatName)
								.arg(QDateTime::currentMSecsSinceEpoch() - returnTime));
	} else {
		server->clientMessage(QString("<CHAT> Bad Ping response from %1: %2")
								.arg(m_chatName)
								.arg(data));
	}
}


/**
 * This client sent a request to snoop you (the user).
 * Apparently the client is responsible for telling the connecting user.
 */
void MMCPClient::handleIncomingSnoop() {
	if (!canSnoop()) {
		sendMessage(QString("<CHAT> You do not have permission to snoop %1.").arg(server->getChatName()));
		return;
	}

	if (isSnooping()) {
		setSnooping(false);
		server->decrementSnoopCount();
		
		server->clientMessage(QString("<CHAT> %1 has stopped snooping you.").arg(m_chatName));
		sendMessage(QString("<CHAT> You have stopped snooping %1.").arg(server->getChatName()));

	} else {
		setSnooping(true);
		server->incrementSnoopCount();
		
		server->clientMessage(QString("<CHAT> %1 has begun snooping you.").arg(m_chatName));
		sendMessage(QString("<CHAT> You have stopped snooping %1.").arg(server->getChatName()));
	}
	
}

/**
 * Handle someone's incoming snoop data.
 * Skip over the color data they sent as we'll be using our own
 */
void MMCPClient::handleIncomingSnoopData(const char* sData, quint16 len) {
	const char *inScan = sData;
	const char *inEnd = inScan + len;
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

const QString& MMCPClient::getVersion() {
    return version;
}

const QString MMCPClient::getFlagsString() {
	
	return QString("%1%2%3%4%5%6%7%8")
				//.arg(GetCommands() ? 'A' : ' ')
				.arg(' ')
				//.arg(GetTransfers() ? 'T' : ' '),
				.arg(' ')
				.arg(m_isPrivate ? 'P' : ' ')
				.arg(m_isIgnored ? 'I' : ' ')
				.arg(m_isServed ? 'S' : ' ')
				//.arg(GetExcludeServe() ? 'X' : ' '),
				//.arg(GetAddress() != GetReportedAddress() ? 'F' : ' '),
				.arg(m_isSnooping ? 'N' : (m_canSnoop ? 'n' : ' '))
				//.arg(' ')
				.arg(' ')
				.arg(' ');

}

const QString MMCPClient::getInfoString() {

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

	return QString("%1 %2 %3 %4 %5")
							.arg(m_chatName, 20)
							.arg(tcpSocket.peerAddress().toString(), 15)
							.arg(tcpSocket.peerPort(), 5)
							.arg("               ", 15)
							.arg(getFlagsString(), 8)
							
							//Try this last to avoid IPv6 issues
							;
}
