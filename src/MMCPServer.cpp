
#include "MMCPServer.h"
#include "Host.h"
#include "mudlet.h"
#include "MMCPClient.h"


MMCPServer::MMCPServer(Host* pHost) :
        QTcpServer(), mpHost(pHost), mRealName(mudlet::self()->scmVersion), snoopCount(0) {


    bool isIntOk = false;

    m_chatName = readMMCPChatName(pHost);
};

MMCPServer::~MMCPServer() {

}


/* Maybe used for connecting to a default server
QString MMCPServer::readMMCPHostName(Host* pH) {
    QString hostname = pH->readProfileData(MMCPServer::HostNameCfgItem);
    if (hostname.isEmpty()) {
        hostname = MMCPServer::DefaultHostName;
    }
    return hostname;
}
*/

int MMCPServer::readMMCPHostPort(Host* pH) {
    const QString portStr = pH->readProfileData(MMCPServer::MMCPHostPortCfgItem);
    bool ok;
    int port = portStr.toInt(&ok);
    if (portStr.isEmpty() || !ok) {
        port = MMCPServer::MMCPDefaultHostPort;
    } else if (port > 65535 || port < 1) {
        port = MMCPServer::MMCPDefaultHostPort;
    }
    return port;
}

QString MMCPServer::readMMCPChatName(Host* pH) {
    QString chatName = pH->readProfileData(MMCPServer::MMCPChatNameCfgItem);
    if (chatName.isEmpty()) {
        // if the new config doesn't exist, try loading the old one.
        chatName = readAppDefaultMMCPChatName();

        if (chatName.isEmpty()) {
            chatName = qsl("%1%2").arg(MMCPServer::DefaultMMCPChatName, QString::number(rand() % 10000));
        }
    }
    return chatName;
}

QPair<bool, QString> MMCPServer::writeChatName(Host* pH, const QString& chatname) {
    // update app-wide file to set a default nick as whatever the last-used nick was.
    writeAppDefaultMMCPChatName(chatname);

    return pH->writeProfileData(MMCPServer::MMCPChatNameCfgItem, chatname);
}

QString MMCPServer::readAppDefaultMMCPChatName() {
    QFile file(mudlet::getMudletPath(mudlet::mainDataItemPath, qsl("mmcp_chatname")));
    const bool opened = file.open(QIODevice::ReadOnly);
    QString rstr;
    if (opened) {
        QDataStream ifs(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            ifs.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        ifs >> rstr;
        file.close();
    }
    return rstr;
}

void MMCPServer::writeAppDefaultMMCPChatName(const QString& nick) {
    QSaveFile file(mudlet::getMudletPath(mudlet::mainDataItemPath, qsl("mmcp_chatname")));
    const bool opened = file.open(QIODevice::WriteOnly);
    if (opened) {
        QDataStream ofs(&file);
        if (mudlet::scmRunTimeQtVersion >= QVersionNumber(5, 13, 0)) {
            ofs.setVersion(mudlet::scmQDataStreamFormat_5_12);
        }
        ofs << nick;
        if (!file.commit()) {
            qDebug() << "MMCPServer::writeAppDefaultMMCPChatName: error saving default chatname: " << file.errorString();
        }
    }
}


void MMCPServer::startServer(quint16 port) {
	if (!listen(QHostAddress::Any, port)) {
		clientMessage(QString("<CHAT> Unable to start server: %1").arg(errorString()));
	
		//QMessageBox::critical(0, tr("Chat Server"),
        //                       tr("Unable to start the server: %1.")
        //                       .arg(errorString()));
	} else {
		clientMessage(QString("<CHAT> Started server on port: %1").arg(port));
		emit serverStarted(port);
	}
}


/**
 * The ChatFilter is installed between the TelnetFilter and ScriptFilter.
 * This allows triggering on text from chat clients.
 */
/*
bool MMCPServer::receiveFromPlayer(QString &obj) {


    if (processCommand(obj))
        return true;
        
    //Send our output to snoopers if local echo is enabled, but not if its a password or such!
    //if (snoopCount > 0 && session->localEcho() && !session->shouldOmit())
    //    sendSnoopData(obj);
		
    if (snoopCount > 0)
        sendSnoopData(obj);
		
        
	} //else {	//fromDir == OUTWARD
	//	if (snoopCount > 0)
		//	sendSnoopData(obj);
			
	//	return send(obj, INWARD, option);
	//}
    
}
*/


/*
bool MMCPServer::processCommand(const QString &l) {
	bool valid = true;
	
	QString line = l.trimmed();
		
	int argsIdx = line.indexOf(' ');
	
	QString cmd = line.mid(1, (argsIdx == -1 ? argsIdx : argsIdx - 1));
	QString args = (argsIdx == -1 ? "" : line.mid(argsIdx + 1));
	
	if (cmd.compare("Chat", Qt::CaseInsensitive) == 0) {
		int targIdx = args.indexOf(' ');
		if (targIdx == -1) return true;
		QVariant target = QVariant(args.left(targIdx));
		QString msg = args.mid(targIdx + 1);
		
		chat(target, msg);
				
	} else if (cmd.compare("ChatAll", Qt::CaseInsensitive) == 0) {
		chatAll(args);
		
	} else if (cmd.compare("ChatCall", Qt::CaseInsensitive) == 0) {
		call(args);
		
	} else if (cmd.compare("ChatList", Qt::CaseInsensitive) == 0) {
		displayChatList();
		
	} else if (cmd.compare("ChatName", Qt::CaseInsensitive) == 0) {
		chatName(args);
		
	} else if (cmd.compare("ChatRaw", Qt::CaseInsensitive) == 0) {
		chatRaw(args);
		
	} else if (cmd.compare("EmoteAll", Qt::CaseInsensitive) == 0) {
		emoteAll(args);
		
	} else if (cmd.compare("Ping", Qt::CaseInsensitive) == 0) {
		ping(QVariant(args));
		
	} else if (cmd.compare("UnChat", Qt::CaseInsensitive) == 0) {
		unChat(QVariant(args));
		
	} else if (cmd.compare("AllowSnoop", Qt::CaseInsensitive) == 0) {
		allowSnoop(QVariant(args));
		
	} else if (cmd.compare("Snoop", Qt::CaseInsensitive) == 0) {
		snoop(QVariant(args));
		
	} else
		valid = false;
			
	return valid;
}
*/

void MMCPServer::sendSnoopData(const QString &line) {
	
	//Note: Fore and Back colors use MudMaster color indices which are NOT the same as
	//		ANSI color indices.  Don't ask me why. So I'll just use background BLACK
	//		foreground WHITE, defined in MudMaster Colors.h
	
	QString outData = QString("%1%2%3\n%4%5")	.arg((char)SnoopData)
												.arg(15, 2, 10)	//foreground color
												.arg(0, 2, 10)		//background color
												.arg(line)
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


void MMCPServer::incomingConnection(int socketDescriptor) {
	
	MMCPClient *client = new MMCPClient(mpHost, this);
	client->incoming(socketDescriptor);
	
}


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
		//qDebug() << id << clients.size();
		if (id > 0 && id <= clients.size())
			client = clients[id - 1];
	}
	
	return client;
}


QPair<bool, QString> MMCPServer::call(const QString &line) {
	QStringList args = line.split(' ');

	int n = args.length();

	if (n < 1) {
		clientMessage(QString("<CHAT> You must specify a host"));
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
		clientMessage(QString("<CHAT> Already connected to %2:%1.").arg(port).arg(host));
		
		return QPair<bool, QString>(false, qsl("already connected to that client"));
	}
		
	clientMessage(QString("<CHAT> Connecting to %2:%1...").arg(port).arg(host));
	
	client = new MMCPClient(mpHost, this);
	
	client->tryConnect(host, port);

    return QPair<bool, QString>(true, qsl("command successful"));
}


/**
 * Send private chat.
 */
QPair<bool, QString> MMCPServer::chat(const QVariant &target, const QString &msg) {
	MMCPClient *client = clientByNameOrId(target);
	
	if (client != NULL) {
		client->sendChat(msg, TextPersonal);
		
		clientMessage(QString("You chat to %1, '%2'").arg(client->chatName()).arg(msg));
        return QPair<bool, QString>(true, qsl("command successful"));
	}

	clientMessage(QString("<CHAT> Invalid Client ID"));
    return QPair<bool, QString>(false, qsl("no client by that name or id"));
}


/**
 * Send a chat message to everybody.
 */
QPair<bool, QString> MMCPServer::chatAll(const QString &msg) {

    if (clients.isEmpty())
        return QPair<bool, QString>(false, qsl("no connected clients"));
	
	QString outMsg = QString("%1%2 chats to everybody, '%3'%4")
							.arg((char)TextEveryone)
							.arg(readMMCPChatName(mpHost))
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


QPair<bool, QString> MMCPServer::chatName(const QString &name) {
    writeChatName(mpHost, name);
	
	QString outMsg = QString("%1%2%3")
							.arg((char)NameChange)
							.arg(name)
							.arg((char)End);
							
	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *cl = it.next();
		cl->writeData(outMsg);
	}
	
	clientMessage(QString("<CHAT> You are now known as %1").arg(name));

    return QPair<bool, QString>(true, qsl("command successful"));
}


/**
 * Sends an unformatted chat message to everyone.
 * Warning, this has the potential for abuse!  But whatever, the chat protocol allows it.
 */
QPair<bool, QString> MMCPServer::chatRaw(const QString &msg) {

    if (clients.isEmpty())
        return QPair<bool, QString>(false, qsl("no connected clients"));

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
 * Send an emote message to everybody.
 */
QPair<bool, QString> MMCPServer::emoteAll(const QString &msg) {
	
    if (clients.isEmpty())
        return QPair<bool, QString>(false, qsl("no connected clients"));

	/*
	QString outMsg = QString("%1%2 %3\n%4")
							.arg((char)TextEveryone)
							.arg(session->getChatName())
							.arg(msg)
							.arg((char)End);
	*/						
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


QPair<bool, QString> MMCPServer::ping(const QVariant &target) {
	MMCPClient *client = clientByNameOrId(target);

	if (client != NULL) {		
		client->sendPingRequest();
		return QPair<bool, QString>(true, qsl("command successful"));
	}

    return QPair<bool, QString>(false, qsl("no client by that name"));
}


void MMCPServer::slotClientConnected(MMCPClient *client) {
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

    const QString coloredStr = QString("\n\n%1%2%3").arg(FBLDRED).arg(trimmed).arg(RST);

    std::string trimmedStdStr = coloredStr.toStdString();
    //mpHost->mpConsole->printOnDisplay(trimmedStdStr, true);
    //mpHost->mpConsole->finalize();
    mpHost->mTelnet.gotPrompt(trimmedStdStr);
    //mpHost->postMessage(coloredStr);

	//AttributedString astr(QString("\n\n%1%2%3").arg(FBLDRED).arg(trimmed).arg(RST));

	//send(astr, INWARD);
}

/**
 * Received snoop data from a client, display it on screen
 */
void MMCPServer::snoopMessage(const QString &message) {
    std::string trimmedStdStr = message.toStdString();
    mpHost->mTelnet.gotPrompt(trimmedStdStr);
    //mpHost->mpConsole->printOnDisplay(trimmedStdStr);
    //mpHost->mpConsole->finalize();

	//AttributedString astr(QString("\n%1%2%3").arg(FBLDGRN).arg(message).arg(RST));

	//send(astr, INWARD);
}



void MMCPServer::sendAll(QString& msg) {
	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *cl = it.next();
		cl->writeData(msg);
	}
}


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


QPair<bool, QString> MMCPServer::serve(const QVariant &target) {
	MMCPClient *client = clientByNameOrId(target);

    const QString chatName = readMMCPChatName(mpHost);
	
	if (client != NULL) {
		if (client->isServed()) {
			client->setServed(false);
			client->sendMessage(QString("<CHAT> You are no longer being served by %1.").arg(chatName));
			
			clientMessage(QString("<CHAT> You are no longer serving %1.").arg(chatName));
			
		} else {
			client->setServed(true);
			client->sendMessage(QString("<CHAT> You are now being served by %1.").arg(chatName));
			
			clientMessage(QString("<CHAT> You are now serving %1.").arg(chatName));
		}

        return QPair<bool, QString>(true, qsl("command successful"));
		
	}

    return QPair<bool, QString>(false, qsl("no client by that name"));
}


/**
 * Allow a client to snoop you (the user).
 */
QPair<bool, QString> MMCPServer::allowSnoop(const QVariant &target) {
	MMCPClient *client = clientByNameOrId(target);

    const QString chatName = readMMCPChatName(mpHost);
	
	if (client != NULL) {
		if (client->canSnoop()) {
			client->setCanSnoop(false);
			client->setSnooped(false);
			client->sendMessage(QString("<CHAT> You are no longer allowed to snoop %1.").arg(chatName));
			
			clientMessage(QString("<CHAT> %1 is no longer allowed to snoop you.").arg(chatName));
			
		} else {
			client->setCanSnoop(true);
			client->sendMessage(QString("<CHAT> You are now allowed to snoop %1.").arg(chatName));
			
			clientMessage(QString("<CHAT> %1 can now snoop you.").arg(chatName));
		}

        return QPair<bool, QString>(true, qsl("command successful"));
	}

    return QPair<bool, QString>(false, qsl("no client by that name"));
}

/**
 * Send a request to snoop someone
 */
QPair<bool, QString> MMCPServer::snoop(const QVariant &target) {
	MMCPClient *client = clientByNameOrId(target);
	
	if (client != NULL) {
		client->snoop();
        return QPair<bool, QString>(true, qsl("command successful"));
	}

    return QPair<bool, QString>(false, qsl("no client by that name"));
}


QPair<bool, QString> MMCPServer::unChat(const QVariant &target) {
	MMCPClient *client = clientByNameOrId(target);
	
	if (client != NULL) {
		client->disconnect();
        return QPair<bool, QString>(true, qsl("command successful"));
	}

    return QPair<bool, QString>(false, qsl("no client by that name"));
}



QPair<bool, QString> MMCPServer::chatList() {

    using namespace AnsiColors;

	QString strMessage;
	strMessage =  "     Name                 Address         Port  Group           Flags\n";
	strMessage += "     ==================== =============== ===== =============== ========\n";

	QString list;
	int i = 1;
	QListIterator<MMCPClient *> it(clients);
	while (it.hasNext()) {
		MMCPClient *client = it.next();
		strMessage.append(QString("%1%2:%3 %4\n")
                            .arg(FBLDWHT)
							.arg(i++, 3)
                            .arg(RST)
							.arg(client->getInfoString()));
	}

	strMessage += "Flags:  A - Allow Commands, F - Firewall, I - Ignore,  P - Private n - Allow Snooping\n";
	strMessage += "        N - Being Snooped,  S - Serving, T - Allows File Transfers, X - Serve Exclude\n";
	
	clientMessage(strMessage);

    return QPair<bool, QString>(true, qsl("command successful"));
}

#pragma mark -- Scripting Interface
//////////////////////
// Scripting Interface
//////////////////////

void MMCPServer::setChatName(const QString &val) {
	m_chatName = val;
	
	writeChatName(mpHost, m_chatName);
}

/*
void MMCPServer::Chat(const QString &target, const QString &message) {
	chat(target, message);
}

void MMCPServer::ChatAll(const QString &msg) {
	chatAll(msg);
}

void MMCPServer::ChatCall() {
	call(QString("%1 %2").arg(m_address).arg(m_port));
}

void MMCPServer::ChatCall(const QString &args, const QString &port) {
	QString arg = args;
	
	if (!port.isEmpty())
			arg.append(" %1").arg(port);
		
	call(arg);
}

void MMCPServer::ChatEmoteAll(const QString &msg) {
	emoteAll(msg);
}

void MMCPServer::ChatUnChat(const QString &target) {
	unChat(target);
}

void MMCPServer::ChatPing(const QString &target) {
	ping(target);
}

void MMCPServer::ChatRaw(const QString &msg) {	
	chatRaw(msg);
}

void MMCPServer::ChatServe(const QString &target) {	
	serve(target);
}

void MMCPServer::ChatSnoopAllow(const QString &target) {	
	allowSnoop(target);
}

void MMCPServer::ChatSnoop(const QString &target) {	
	snoop(target);
}
*/