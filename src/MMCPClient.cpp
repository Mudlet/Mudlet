
#include <QAbstractSocket>
#include <QRegularExpression>

#include "MMCPClient.h"
#include "MMCPServer.h"


MMCPClient::MMCPClient(Host *host, MMCPServer *serv)
	: m_state(Disconnected), mpHost(host), server(serv) {
	
	//auto ignore or ignorelist match?
	m_isIgnored = (1 == 0 ? true : false);
	
	//auto private or privatelist match?
	m_isPrivate = (1 == 0 ? true : false);
	
	//auto serve or servelist match?
	m_isServed = (1 == 0 ? true : false);
	
	m_canSnoop = false;
	m_isSnooped = false;
	m_isSnooping = false;

	
	tcpSocket = new QTcpSocket(this);
	
	//Disable Nagle's algorithm
	tcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

	connect(tcpSocket, &QTcpSocket::connected,      this, &MMCPClient::slotConnected);
	connect(tcpSocket, &QTcpSocket::disconnected,   this, &MMCPClient::slotDisconnected);
	connect(tcpSocket, &QTcpSocket::readyRead,      this, &MMCPClient::slotReadData);
	connect(tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slotDisplayError(QAbstractSocket::SocketError)));

	connect(this, &MMCPClient::clientDisconnected, server, &MMCPServer::slotClientDisconnected);
}

void MMCPClient::tryConnect(const QString& host, quint16 port) {
	m_state = ConnectingOut;
	tcpSocket->connectToHost(host, port);
}


/**
 * Disconnect the client.  This will result in the disconnected() signal.
 */
void MMCPClient::disconnect() {
	tcpSocket->disconnectFromHost();
}


/**
 * Handle an incoming chat connection.
 */
void MMCPClient::incoming(int socketDesc) {
	m_state = ConnectingIn;
	tcpSocket->setSocketDescriptor(socketDesc);
}

QString MMCPClient::host() {
	return tcpSocket->peerAddress().toString();
}

quint16 MMCPClient::port() {
	return tcpSocket->peerPort();
}


/**
 * Outgoing connection established, send chat request.
 */
void MMCPClient::slotConnected() {
	m_host = tcpSocket->peerAddress().toString();
	m_port = tcpSocket->peerPort();

	QString str = QString("CHAT:%1\n%3%2").arg(server->chatName()).arg(m_port, 5).arg(m_host);

	tcpSocket->write(str.toLatin1());
	
	server->clientMessage(QString("<CHAT> Waiting for response from %2:%1 ...").arg(m_port).arg(m_host));
}


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


void MMCPClient::slotReadData() {
		
	while (tcpSocket->bytesAvailable()) {
		buffer.append(tcpSocket->read(tcpSocket->bytesAvailable()));
	}
	
	switch (m_state) {
		case ConnectingIn: {
			QRegularExpression chatIncoming("CHAT:(.*)\n([0-9.]+)$");

            //const QString bufStr = QString(buffer);
			
            QRegularExpressionMatch match = chatIncoming.match(buffer);

            if (match.hasMatch()) {
			//if (chatIncoming.indexIn(buffer) == -1) {
			//	m_state = Disconnected;
			//} else {
				//Check auto accept?
				m_state = Connected;
				
				m_chatName = match.captured(1);
				
				writeData(QString("YES:%1\n").arg(server->chatName()));
										
				server->clientMessage(QString("<CHAT> Connection from %1 at %3:%2 accepted.")
										.arg(m_chatName)
										.arg(tcpSocket->peerPort())
										.arg(tcpSocket->peerAddress().toString()));
				
				server->slotClientConnected(this);
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
										.arg(tcpSocket->peerPort())
										.arg(tcpSocket->peerAddress().toString()));
				
				server->slotClientConnected(this);
				sendVersion();
				
				if (chatAccepted.captureCount() > 2) {
					//A command was tacked on the end of the accept string
					handleConnectedState(match.captured(2).toLatin1());
				}
			 } else {
                m_state = Disconnected;
										
				server->clientMessage(QString("<CHAT> Connection to %2:%1 refused.")
										.arg(tcpSocket->peerPort())
										.arg(tcpSocket->peerAddress().toString()));
             }


			break;
		}
			
		case Connected:
			//if (buffer.lastIndexOf(QChar::fromLatin1(0xff)) == -1)
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
			message = tr("The following error occurred: %1.").arg(tcpSocket->errorString());
	}

	server->clientMessage(message);
}


void MMCPClient::sendChat(const QString &msg, MMCPChatCommands command) {
	QString output;

	switch (command) {
		case TextEveryone:
			output = msg;	//Done in ChatFilter already for efficiency
			break;
			
		case TextPersonal:
			output = QString("%1%2 chats to you, '%3'%4")
							.arg((char)command)
							.arg(server->chatName())
							.arg(msg)
							.arg((char)End);
			break;
	}
	
	writeData(output);
}


void MMCPClient::sendMessage(const QString &msg) {
	writeData(QString("%1%2%3")	.arg((char)Message)
								.arg(msg)
								.arg((char)End));
}


void MMCPClient::sendPingRequest() {
	
	writeData(QString("%1%2%3")	.arg((char)PingRequest)
								.arg(QDateTime::currentMSecsSinceEpoch())
								.arg((char)End));

	server->clientMessage(QString("<CHAT> Pinging %1...").arg(m_chatName));
}


/**
 * TODO: Implement app name and version globals
 */
void MMCPClient::sendVersion() {
	writeData(QString("%1%2 %3%4")	.arg((char)Version)
									.arg("QtMud")
									.arg("alpha")
									.arg((char)End));
}


void MMCPClient::writeData(const QString &data) {
	tcpSocket->write(data.toLatin1());
}

/**
 * Attempt to snoop this client.
 */
void MMCPClient::snoop() {
	writeData(QString("%1%2")	.arg((char)Snoop)
								.arg((char)End));
}

void MMCPClient::handleConnectedState(const QByteArray &bytes) {
	//QByteArray bytes = str.toLatin1();
	const char *data = bytes.data();
	int cmdIdx = 0;
	while (cmdIdx < bytes.length()) {
		char cmd = data[cmdIdx];
		
		int endIdx = bytes.indexOf(0xFF, cmdIdx);
		if (endIdx == -1) {
			qDebug() << "wtf?";
			endIdx = bytes.length();
		}
		
		//Handle snoop data here to avoid an unneeded string copy
		//because we dont care about the fore and back color information
		if (cmd == MMCPChatCommand::SnoopData) {
			handleIncomingSnoopData(QString::fromLatin1(data + cmdIdx + 1 + 4, (endIdx - cmdIdx) - 1));
			return;
		}
		
		QString stringData = QString::fromLatin1(data + cmdIdx + 1, (endIdx - cmdIdx) - 1);

        qDebug() << "handleConnectedState: " << stringData;
		
		switch (cmd) {
			case MMCPChatCommand::NameChange:
				handleIncomingNameChange(stringData);
				break;
				
			case MMCPChatCommand::RequestConnections:
				handleIncomingConnectionsRequest();
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
				
			case MMCPChatCommand::Snoop:
				handleIncomingSnoop();
				break;
				
			//case MMCPChatCommand::ChannelData:
			//	server->send(stringData, IOChainLink::INWARD, QtMud::ScriptChannel);
			//	break;
				
			default:
				qDebug() << "unknown command: " << cmd << stringData;
		}
		
		cmdIdx = endIdx + 1;
	}
}


void MMCPClient::handleIncomingConnectionsRequest() {

	server->clientMessage(QString("<CHAT> %1 has requested your public connections...").arg(m_chatName));

	server->sendPublicConnections(this);
}


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


void MMCPClient::handleIncomingChatPersonal(const QString &msg) {
	server->clientMessage(msg);
}


void MMCPClient::handleIncomingNameChange(const QString &newName) {
	server->clientMessage(QString("<CHAT> %1 is now known as %2.").arg(m_chatName).arg(newName));
	m_chatName = newName;
}


void MMCPClient::handleIncomingPingRequest(const QString &msg) {
	writeData(QString("%1%2%3")	.arg((char)PingResponse)
								.arg(msg)
								.arg((char)End));
}


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
 */
void MMCPClient::handleIncomingSnoop() {
	if (!canSnoop()) {
		sendMessage(QString("<CHAT> You do not have permission to snoop %1.").arg(server->chatName()));
		return;
	}

	if (isSnooping()) {
		setSnooping(false);
		server->decrementSnoopCount();
		
		server->clientMessage(QString("<CHAT> %1 has stopped snooping you.").arg(m_chatName));
		sendMessage(QString("<CHAT> You have stopped snooping %1.").arg(server->chatName()));

	} else {
		setSnooping(true);
		server->incrementSnoopCount();
		
		server->clientMessage(QString("<CHAT> %1 has begun snooping you.").arg(m_chatName));
		sendMessage(QString("<CHAT> You have stopped snooping %1.").arg(server->chatName()));
	}
	
}


void MMCPClient::handleIncomingSnoopData(const QString &sData) {
	const QChar *inScan = sData.data();
	const QChar *inEnd = inScan + sData.length();
	QString strLine;

	//Skip over fore and back colors
	inScan += 4;
	
	for (; inScan < inEnd; inScan++) {
		char c = inScan->toLatin1();
		
		if (c == '\r')
			continue;
			
		if (c == '\n') {
			server->snoopMessage(strLine);
			strLine.clear();
			continue;
		}
		
		strLine.append(*inScan);
	}
	
	if (strLine.length() > 0)
		server->snoopMessage(strLine);
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

	
	QString strName;
	/*
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

	return QString("%1 %5 %2 %3 %4")
							.arg(m_chatName, 20)
							.arg(tcpSocket->peerPort(), 5)
							.arg("               ", 15)
							.arg(getFlagsString())
							
							//Try this last to avoid IPv6 issues
							.arg(tcpSocket->peerAddress().toString(), 15);
}
