#include "MMCPServer.h"
#include "Host.h"
#include "mudlet.h"
#include "MMCPClient.h"

#include "pre_guard.h"
#include <QMap>
#include "post_guard.h"

MMCPServer::MMCPServer(Host* pHost) :
        QTcpServer(), mpHost(pHost), snoopCount(0) {

	m_chatName = pHost->getMMCPChatName();
};

MMCPServer::~MMCPServer() {

}

/**
 * Handle an incoming connection, create an MMCPClient and set its state
 * to ConnectingIn
 */
void MMCPServer::incomingConnection(qintptr socketDescriptor) {
	
	MMCPClient *client = new MMCPClient(mpHost, this);
	if (!client->incoming(socketDescriptor)) {
		client->deleteLater();
	}
	
}

/**
 * Receive mud data from our current session
 */
bool MMCPServer::receiveFromPlayer(std::string& str) {
    if (snoopCount > 0) {
        sendSnoopData(str);
	} 
}

/**
 * Send mud data from our mud to all clients snooping us
 */
void MMCPServer::sendSnoopData(std::string &line) {
	
	//Note: Fore and Back colors use MudMaster color indices which are NOT the same as
	//		ANSI color indices.  Don't ask me why. So I'll just use background BLACK
	//		foreground WHITE, defined in MudMaster Colors.h
	
	QString outData = QString("%1%2%3\n%4%5")	.arg((char)SnoopData)
												.arg(15, 2, 10)	//foreground color
												.arg(0, 2, 10)		//background color
												.arg(QString::fromStdString(line))
												.arg((char)End);

	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *cl = it.next();
		if (cl->isSnooping()) {
			cl->writeData(outData);
			break;
		}
	}
}

/**
 * Return a pointer to a client given their client name or connection Id
 */
MMCPClient *MMCPServer::clientByNameOrId(const QVariant &arg) {
	MMCPClient *client = NULL;

	bool ok;
	int id = arg.toInt(&ok);
	
	if (!ok) {
		QString lowerName = arg.toString().toLower();
	
		QListIterator<MMCPClient *> it(clients);
		while (it.hasNext()) {
			MMCPClient *cl = it.next();
			if (cl->chatName().toLower() == lowerName ) {
				client = cl;
				break;
			}
		}

	} else {
		if (id > 0 && id <= clients.size())
			client = clients[id - 1];
	}
	
	return client;
}

/**
 * Script command, attempt a connection to a new client.
 * Parse port from line
 */
QPair<bool, QString> MMCPServer::call(const QString &line) {
	QStringList args = line.split(' ');

	int n = args.length();

	if (n < 1) {
		const QString infoMsg = tr("[ CHAT ]  - You must specify a host");
        mpHost->postMessage(infoMsg);
		return QPair<bool, QString>(false, qsl("must specify a host"));
	}
		
	quint16 port = 4050;

	if (n == 2) {
		bool ok;
		quint16 tempPort = args[1].toUInt(&ok, 10);
		if (ok) port = tempPort;
	}

	call(args[0], port);
    return QPair<bool, QString>(true, qsl("command successful"));
}

/**
 * Script command, attempt a connection to a new client
 */
QPair<bool, QString> MMCPServer::call(const QString& host, int port) {
	MMCPClient *client = NULL;
	
	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *cl = it.next();
		if (cl->host() == host && client->port() == port) {
			client = cl;
			break;
		}
	}

	//Check if we're already connected to this person
	if (client != NULL) {
		const QString infoMsg = tr("[ CHAT ]  - Already connected to %1:%2.").arg(host).arg(port);
        mpHost->postMessage(infoMsg);
		
		return QPair<bool, QString>(false, qsl("already connected to that client"));
	}
		
	const QString infoMsg = tr("[ CHAT ]  - Connecting to %1:%2...").arg(host).arg(port);
    mpHost->postMessage(infoMsg);
	
	client = new MMCPClient(mpHost, this);
	
	client->tryConnect(host, port);

    return QPair<bool, QString>(true, qsl("command successful"));
}


/**
 * Script command, Send private chat.
 */
QPair<bool, QString> MMCPServer::chat(const QVariant &target, const QString &msg) {
	MMCPClient *client = clientByNameOrId(target);
	
	if (client != NULL) {
		client->sendChat(msg, TextPersonal);
		
		clientMessage(QString("You chat to %1, '%2'").arg(client->chatName()).arg(msg));
        return QPair<bool, QString>(true, qsl("command successful"));
	}

	const QString infoMsg = tr("[ CHAT ]  - Invalid client id.");
    mpHost->postMessage(infoMsg);
    return QPair<bool, QString>(false, qsl("no client by that name or id"));
}


/**
 * Script COmmand, Send a chat message to everybody.
 */
QPair<bool, QString> MMCPServer::chatAll(const QString &msg) {

    if (clients.isEmpty()) {
        return QPair<bool, QString>(false, qsl("no connected clients"));
	}
	
	QString outMsg = QString("%1%2 chats to everybody, '%3'%4")
							.arg((char)TextEveryone)
							.arg(m_chatName)
							.arg(msg)
							.arg((char)End);
	
	
	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *cl = it.next();
		cl->sendChat(outMsg, TextEveryone);
	}

	clientMessage(QString("You chat to everybody, '%1'").arg(msg));

    return QPair<bool, QString>(true, qsl("command successful"));
}

/**
 * Script command, Display a list of connected chat clients on the main console
 */
QPair<bool, QString> MMCPServer::chatList() {

    using namespace AnsiColors;

	QString strMessage;
	strMessage =  "     Name                 Address              Port  Group           Flags    ChatClient\n";
	strMessage += "     ==================== ==================== ===== =============== ======== ================\n";

	QString list;
	int i = 1;
	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *client = it.next();
		strMessage.append(QString("%1%2:%3 %4 %5\n")
                            .arg(FBLDWHT)
							.arg(i++, 3)
                            .arg(RST)
							.arg(client->getInfoString())
							.arg(client->getVersion()));
	}

	strMessage += "Flags:  A - Allow Commands, F - Firewall, I - Ignore,  P - Private   n - Allow Snooping\n";
	strMessage += "        N - Being Snooped,  S - Serving,  T - Allows File Transfers, X - Serve Exclude\n";
	
	clientMessage(strMessage);

    return QPair<bool, QString>(true, qsl("command successful"));
}

/**
 * Script Command, Set our chat name to name, and tell connected chat clients
 */
QPair<bool, QString> MMCPServer::chatName(const QString &name) {
    setChatName(name);

    if (!clients.isEmpty()) {
	
        QString outMsg = QString("%1%2%3")
                                .arg((char)NameChange)
                                .arg(name)
                                .arg((char)End);
                                
        QListIterator<MMCPClient *> it(clients);
        while (it.hasNext()) {
            MMCPClient *cl = it.next();
            cl->writeData(outMsg);
        }
    }
	
	const QString infoMsg = tr("[ CHAT ]  - You are now known as %1.").arg(name);
    mpHost->postMessage(infoMsg);

    return QPair<bool, QString>(true, qsl("command successful"));
}


/**
 * Script command, Sends an unformatted chat message to everyone.
 * Warning, this has the potential for abuse!  But whatever, the chat protocol allows it.
 */
QPair<bool, QString> MMCPServer::chatRaw(const QString &msg) {

    if (clients.isEmpty()) {
        return QPair<bool, QString>(false, qsl("no connected clients"));
	}

	QString outMsg = QString("%1%2%3")
							.arg((char)TextEveryone)
							.arg(msg)
							.arg((char)End);
	
	
	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *cl = it.next();
		cl->sendChat(outMsg, TextEveryone);
	}
		
	clientMessage(msg);

    return QPair<bool, QString>(true, qsl("command successful"));
}


/**
 * Script command, Send an emote message to everybody.
 */
QPair<bool, QString> MMCPServer::emoteAll(const QString &msg) {
	
    if (clients.isEmpty()) {
        return QPair<bool, QString>(false, qsl("no connected clients"));
	}
						
	QString outMsg = QString("%1%2\n%3")
							.arg((char)TextEveryone)
							.arg(msg)
							.arg((char)End);
	
	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *cl = it.next();
		cl->sendChat(outMsg, TextEveryone);
	}

	clientMessage(msg);

    return QPair<bool, QString>(true, qsl("command successful"));
}

/**
 * Ignore or un-ignore a person
 */
QPair<bool, QString> MMCPServer::ignore(const QString& target) {
	MMCPClient *client = clientByNameOrId(target);

	if (client != NULL) {	
		if (client->isIgnored()) {
			const QString infoMsg = tr("[ CHAT ]  - You are no longer ignoring %1.").arg(client->chatName());
        	mpHost->postMessage(infoMsg);
		} else {
			const QString infoMsg = tr("[ CHAT ]  - You are now ignoring %1.").arg(client->chatName());
        	mpHost->postMessage(infoMsg);
		}
		client->setIgnore(!client->isIgnored());
		return QPair<bool, QString>(true, qsl("command successful"));
	}

	const QString infoMsg = tr("[ CHAT ]  - Cannot find client identified by %1.").arg(target);
    mpHost->postMessage(infoMsg);

	return QPair<bool, QString>(false, qsl("no client by that name or id"));
}

/**
 * Script command, send ping request to a client
 */
QPair<bool, QString> MMCPServer::ping(const QVariant &target) {
	MMCPClient *client = clientByNameOrId(target);

	if (client != NULL) {		
		client->sendPingRequest();
		return QPair<bool, QString>(true, qsl("command successful"));
	}

    return QPair<bool, QString>(false, qsl("no client by that name or id"));
}

/**
 * Script command, send peek connections request to a client
 */
QPair<bool, QString> MMCPServer::peek(const QVariant &target) {
	MMCPClient *client = clientByNameOrId(target);

	if (client != NULL) {		
		client->sendPeekRequest();
		return QPair<bool, QString>(true, qsl("command successful"));
	}

    return QPair<bool, QString>(false, qsl("no client by that name or id"));
}

/**
 * Toggle a client's private state
 */
QPair<bool, QString> MMCPServer::chatPrivate(const QVariant& target) {
	MMCPClient *client = clientByNameOrId(target);

	if (client != NULL) {		
		if (client->isPrivate()) {
			client->setPrivate(false);
			const QString infoMsg = tr("[ CHAT ]  - %1 is no longer private.").arg(client->chatName());
        	mpHost->postMessage(infoMsg);
		} else {
			client->setPrivate(true);
			const QString infoMsg = tr("[ CHAT ]  - %1 is now set as private.").arg(client->chatName());
        	mpHost->postMessage(infoMsg);
		}
		return QPair<bool, QString>(true, qsl("command successful"));
	}

    return QPair<bool, QString>(false, qsl("no client by that name or id"));
}

/**
 * Begin serving a client to our other connections
 */
QPair<bool, QString> MMCPServer::serve(const QVariant &target) {
	MMCPClient *client = clientByNameOrId(target);
	
	if (client != NULL) {
		if (client->isServed()) {
			client->setServed(false);
			client->sendMessage(QString("<CHAT> You are no longer being served by %1.").arg(m_chatName));
			
			const QString infoMsg = tr("[ CHAT ]  - You are no longer serving %1.").arg(client->chatName());
        	mpHost->postMessage(infoMsg);
			
		} else {
			client->setServed(true);
			client->sendMessage(QString("<CHAT> You are now being served by %1.").arg(m_chatName));
			
			const QString infoMsg = tr("[ CHAT ]  - You are now serving %1.").arg(client->chatName());
        	mpHost->postMessage(infoMsg);
		}

        return QPair<bool, QString>(true, qsl("command successful"));
		
	}

    return QPair<bool, QString>(false, qsl("no client by that name"));
}

/**
 * Script command, start listening for incoming client connections
 */
QPair<bool, QString> MMCPServer::startServer(quint16 port) {
	if (!listen(QHostAddress::Any, port)) {
		const QString infoMsg = tr("[ CHAT ]  - Unable to start server: %1.").arg(errorString());
        mpHost->postMessage(infoMsg);
		return QPair<bool, QString>(false, qsl("unable to start server"));
	}

	const QString infoMsg = tr("[ CHAT ]  - Started server on port %1.").arg(port);
    mpHost->postMessage(infoMsg);
	emit serverStarted(port);
	
	return QPair<bool, QString>(true, qsl("command successful"));
}

/**
 * Script command, Stop our server from listening
 */
QPair<bool, QString> MMCPServer::stopServer() {
	if (isListening()) {
		close();
		return QPair<bool, QString>(true, qsl("command successful"));
	}

	return QPair<bool, QString>(false, qsl("unable to stop server, it is not listening"));
}


/**
 * Allow a client to snoop you (the user).
 */
QPair<bool, QString> MMCPServer::allowSnoop(const QVariant &target) {
	MMCPClient *client = clientByNameOrId(target);
	
	if (client != NULL) {
		if (client->canSnoop()) {
			client->setCanSnoop(false);
			client->setSnooped(false);
			client->sendMessage(QString("<CHAT> You are no longer allowed to snoop %1.").arg(m_chatName));
			
			const QString infoMsg = tr("[ CHAT ]  - %1 is no longer allowed to snoop you.").arg(client->chatName());
        	mpHost->postMessage(infoMsg);
			
		} else {
			client->setCanSnoop(true);
			client->sendMessage(QString("<CHAT> You are now allowed to snoop %1.").arg(m_chatName));
			
			const QString infoMsg = tr("[ CHAT ]  - %1 can now snoop you.").arg(client->chatName());
        	mpHost->postMessage(infoMsg);
		}

        return QPair<bool, QString>(true, qsl("command successful"));
	}

    return QPair<bool, QString>(false, qsl("no client by that name"));
}

/**
 * Send a request to snoop someone, or stop snooping them if snooped
 */
QPair<bool, QString> MMCPServer::snoop(const QVariant &target) {
	MMCPClient *client = clientByNameOrId(target);
	
	if (client != NULL) {
		if (client->isSnooped()) {
			client->setSnooped(false);
		} else {
			client->snoop();
		}

        return QPair<bool, QString>(true, qsl("command successful"));
	}

    return QPair<bool, QString>(false, qsl("no client by that name"));
}

/**
 * Unchat someone by name or id
 */
QPair<bool, QString> MMCPServer::unChat(const QVariant &target) {
	MMCPClient *client = clientByNameOrId(target);
	
	if (client != NULL) {
		client->disconnect();
        return QPair<bool, QString>(true, qsl("command successful"));
	}

    return QPair<bool, QString>(false, qsl("no client by that name"));
}


void MMCPServer::addConnectedClient(MMCPClient *client) {
	clients.append(client);
	client->setId(clients.indexOf(client));
	emit clientConnected(client);
}

void MMCPServer::slotClientDisconnected(MMCPClient *client) {
	clients.removeOne(client);
	emit clientDisconnected(client);
}


/**
 * Send a message to the terminal pane.
 * This converts a normal QString into an AttributedString, prepends a carriage return,
 * and encapsulates the string in the default chat color before sending the result inward.
 */
void MMCPServer::clientMessage(const QString &message) {
	
	QString trimmed = message;
	int idx = message.lastIndexOf('\n');
	int escIdx = message.lastIndexOf('\x1b');
	if (idx != -1 && escIdx != -1 && idx > escIdx)
		trimmed.replace(idx, 1, "");

    using namespace AnsiColors;

    const QString coloredStr = QString("\n%1%2%3\n").arg(FBLDRED).arg(trimmed).arg(RST);

    std::string trimmedStdStr = coloredStr.toStdString();
    mpHost->mpConsole->printOnDisplay(trimmedStdStr, false);
    mpHost->mpConsole->finalize();
}

/**
 * Received snoop data from a client, display it on screen
 */
void MMCPServer::snoopMessage(const std::string &message) {	
	using namespace AnsiColors;

	std::stringstream ss;
	ss << FBLDGRN << ">> " << RST;
	ss << message << "\n";
	
	std::string outStr = ss.str();

    mpHost->mpConsole->printOnDisplay(outStr, false);
    mpHost->mpConsole->finalize();
}


void MMCPServer::sendAll(QString& msg) {
	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *cl = it.next();
		cl->writeData(msg);
	}
}

/**
 * Send our public connections to everyone (not the client that requested)
 */
void MMCPServer::sendPublicConnections(MMCPClient *client) {
	QString list;
	
	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *cl = it.next();
	
		if (cl != client && !cl->isPrivate()) {
			list.append(QString("%1,%2").arg(cl->host()).arg(cl->port()));
			
			if (it.hasNext())
				list.append(",");
		}
	}
	
	if (!list.isEmpty()) {
		QString cmdStr = QString("%1%2%3")	.arg((char)ConnectionList)
											.arg(list)
											.arg((char)End);
											
		client->writeData(cmdStr);
	}
}

/**
 * Send a peek list of our public connections to everyone (not the client that requested)
 */
void MMCPServer::sendPublicPeek(MMCPClient *client) {
	QString list;
	
	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *cl = it.next();
	
		if (cl != client && !cl->isPrivate()) {
			qDebug() << "peeking client" << cl->chatName();
			list.append(QString("%1~%2~%3")
				.arg(cl->host())
				.arg(cl->port())
				.arg(cl->chatName()));
		}
	}
	
	if (!list.isEmpty()) {
		QString cmdStr = QString("%1%2%3")	.arg((char)PeekList)
											.arg(list)
											.arg((char)End);

		client->writeData(cmdStr);
	} else {
		client->sendMessage(QString("<CHAT> %1 doesn't have any other connections").arg(m_chatName));
	}
}

/**
 * Forward message to all clients.
 */
void MMCPServer::sendServedMessage(MMCPClient *client, const QString &msg) {
	QString cmdStr = QString("%1%2%3")	.arg((char)TextEveryone)
										.arg(msg)
										.arg((char)End);

	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *cl = it.next();
		if (cl != client)
			cl->writeData(cmdStr);
	}
}

/**
 * Forward message to all served clients.
 */
void MMCPServer::sendMessageToServed(MMCPClient *client, const QString &msg) {
	QString cmdStr = QString("%1%2%3")	.arg((char)TextEveryone)
										.arg(msg)
										.arg((char)End);

	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *cl = it.next();
		if (cl != client && cl->isServed())
			cl->writeData(cmdStr);
	}
}

/**
 * Set chat name and write to current profile
 */
void MMCPServer::setChatName(const QString &val) {
	m_chatName = val;
	// set the host chatname so it gets saved in the profile
	mpHost->setMMCPChatName(m_chatName);
}
