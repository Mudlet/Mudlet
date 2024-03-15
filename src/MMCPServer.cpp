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

#include "MMCPServer.h"
#include "Host.h"
#include "MMCP.h"
#include "MMCPClient.h"
#include "mudlet.h"

#include <string>

#include "pre_guard.h"
#include <QTcpServer>
#include <QHostAddress>
#include <QListIterator>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>
#include "post_guard.h"

MMCPServer::MMCPServer(Host* pHost)
: QTcpServer()
, mpHost(pHost)
, mChatName(pHost->getMMCPChatName())
{
}

/**
 * Handle an incoming connection, create an MMCPClient and set its state
 * to ConnectingIn
 */
void MMCPServer::incomingConnection(qintptr socketDescriptor)
{
    MMCPClient* pClient = new MMCPClient(mpHost, this);
    if (!pClient->incoming(socketDescriptor)) {
        pClient->deleteLater();
    }
}

/**
 * Receive mud data from our current session
 */
void MMCPServer::receiveFromPlayer(std::string& str)
{
    if (mSnoopCount > 0) {
        sendSnoopData(str);
    }
}

/**
 * Send mud data from our mud to all clients snooping us
 */
void MMCPServer::sendSnoopData(std::string& line)
{
    //Note: Fore and Back colors use MudMaster color indices which are NOT the same as
    //		ANSI color indices.  Don't ask me why. So I'll just use background BLACK
    //		foreground WHITE, defined in MudMaster Colors.h

    const QString outData = qsl("%1%2%3\n%4%5")
                                    .arg(static_cast<char>(SnoopData))
                                    .arg(15, 2, 10) //foreground color
                                    .arg(0, 2, 10)  //background color
                                    .arg(QString::fromStdString(line), static_cast<char>(End));

    QListIterator<MMCPClient*> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        if (cl->isSnooping()) {
            cl->writeData(outData);
            break;
        }
    }
}

/**
 * Return a pointer to a client given their client name or connection Id
 */
MMCPClient* MMCPServer::clientByNameOrId(const QVariant& arg)
{
    MMCPClient* pClient = nullptr;

    bool ok;
    int id = arg.toInt(&ok);

    if (!ok) {
        // Arg does NOT appear to be an integer so it is probably a name:
        const QString name = arg.toString();
        QListIterator<MMCPClient*> it(mPeersList);
        while (it.hasNext()) {
            MMCPClient* cl = it.next();
            if (!name.compare(cl->chatName(), Qt::CaseInsensitive)) {
                pClient = cl;
                break;
            }
        }

    } else {
        // It is an integer
        if (id > 0 && id <= mPeersList.size()) {
            // And it is in range
            pClient = mPeersList[id - 1];
        }
    }

    return pClient;
}

/**
 * Script command, attempt a connection to a new client.
 * Parse port from line
 */
QPair<bool, QString> MMCPServer::call(const QString& line)
{
    QStringList args = line.split(' ');

    int n = args.length();

    if (n < 1) {
        const QString infoMsg = tr("[ CHAT ]  - You must specify a host.");
        mpHost->postMessage(infoMsg);
        return {false, qsl("must specify a host")};
    }

    quint16 port = csDefaultMMCPHostPort;

    if (n == 2) {
        bool ok;
        quint16 tempPort = args[1].toUInt(&ok, 10);
        if (ok) {
            port = tempPort;
        }
    }

    call(args[0], port);
    return {true, QString()};
}

/**
 * Script command, attempt a connection to a new client
 */
QPair<bool, QString> MMCPServer::call(const QString& host, int port)
{
    MMCPClient* pClient = nullptr;

    QListIterator<MMCPClient*> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        if (cl->host() == host && pClient->port() == port) {
            pClient = cl;
            break;
        }
    }

    //Check if we're already connected to this person
    if (pClient) {
        const QString infoMsg = tr("[ CHAT ]  - Already connected to %1:%2.").arg(host).arg(port);
        mpHost->postMessage(infoMsg);

        return {false, qsl("already connected to that client")};
    }

    const QString infoMsg = tr("[ CHAT ]  - Connecting to %1:%2...").arg(host).arg(port);
    mpHost->postMessage(infoMsg);

    pClient = new MMCPClient(mpHost, this);

    pClient->tryConnect(host, port);

    return {true, QString()};
}


/**
 * Script command, Send private chat.
 */
QPair<bool, QString> MMCPServer::chat(const QVariant& target, const QString& msg)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        const QString outMsg = qsl("%1%2 chats to you, '%3'\n%4")
                                       .arg(static_cast<char>(TextPersonal))
                                       .arg(mChatName, msg)
                                       .arg(static_cast<char>(End));
        pClient->writeData(outMsg);

        using namespace AnsiColors;
        //: %1 is the name of the peer receiving the message %2
        clientMessage(tr("<CHAT>You chat to %1, '%2'")
                              .arg(pClient->chatName(), msg)
                              .prepend(FBLDRED)
                              .append(RST));
        return {true, QString()};
    }

    const QString infoMsg = tr("[ CHAT ]  - Invalid client id '%1'.").arg(target.toString());
    mpHost->postMessage(infoMsg);
    return {false, qsl("no client by that name or id")};
}


/**
 * Script Command, Send a chat message to everybody.
 */
QPair<bool, QString> MMCPServer::chatAll(const QString& msg)
{
    if (mPeersList.isEmpty()) {
        return {false, qsl("no connected clients")};
    }

    const QString outMsg = qsl("%1\n%2 chats to everybody, '%3'%4")
                                   .arg(static_cast<char>(TextEveryone))
                                   .arg(mChatName)
                                   .arg(msg)
                                   .arg(static_cast<char>(End));

    QListIterator<MMCPClient*> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        cl->writeData(outMsg);
    }

    using namespace AnsiColors;
    //: %1 is message sent to everyone
    clientMessage(tr("<CHAT>You chat to everybody, '%1'")
                          .arg(msg)
                          .prepend(FBLDRED)
                          .append(RST));
    return {true, QString()};
}

/**
 * Send a chat message to a specific group
 */
QPair<bool, QString> MMCPServer::chatGroup(const QString& group, const QString& message)
{
    if (mPeersList.isEmpty()) {
        return {false, qsl("no connected clients")};
    }

    using namespace AnsiColors;

    QString outMsg = qsl("%1%2\n%3%4 chats to the group, '%5'\n%6")
                            .arg(static_cast<char>(TextGroup))
                            .arg(group, -15)
                            .arg(mChatName, FBLDRED, message)
                            .arg(static_cast<char>(End));

    QListIterator<MMCPClient*> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        if (cl->getGroup() == group) {
            cl->writeData(outMsg);
        }
    }

    using namespace AnsiColors;
    /*: %1 and %2 are ASCII ESC color codes that need to be included BEFORE a
     * portion of text (the main message %4) and (the group name %4)
     * respectively and %5 is another code at the very end to reset the colors
     * back to "normal". Please try and reproduce the positioning of those codes
     * around the translation.
     */
    clientMessage(tr("%1<CHAT>You chat to <%2%3%1>, '%4'%5")
                          .arg(FBLDCYN, FBLDRED, group, message, RST));
    return {true, QString()};
}

/**
 * Script command, Display a list of connected chat clients on the main console
 */
QPair<bool, QString> MMCPServer::chatList()
{
    using namespace AnsiColors;

    QStringList peersList;
    int peerCount = 0;
    QListIterator<MMCPClient*> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* pClient = it.next();
        peersList << qsl("%1%2 %3 %4 %5")
                             .arg(FBLDWHT)
                             .arg(++peerCount, 3, 10, QLatin1Char('0'))
                             .arg(RST, pClient->getInfoString(), pClient->getVersion());
    }

    const QString strMessage = tr("%1     Name                 Address              Port  Group           Flags    ChatClient\n"
                                  "     ==================== ==================== ===== =============== ======== ================\n"
                                  "%2"
                                  "     ==================== ==================== ===== =============== ======== ================\n"
                                  "Flags:  A - Allow Commands, F - Firewall, I - Ignore,  P - Private   n - Allow Snooping\n"
                                  "        N - Being Snooped,  S - Serving,  T - Allows File Transfers, X - Serve Exclude%1")
                                       .arg(RST, peersList.join(QChar::LineFeed).append(peersList.isEmpty() ? QChar::Null : QChar::LineFeed));

    clientMessage(strMessage);
    return {true, QString()};
}

/**
 * Script Command, Set our chat name to name, and tell connected chat clients
 */
QPair<bool, QString> MMCPServer::chatName(const QString& name)
{
    setChatName(name);

    if (!mPeersList.isEmpty()) {
        const QString outMsg = qsl("%1%2%3")
                                       .arg(static_cast<char>(NameChange))
                                       .arg(name)
                                       .arg(static_cast<char>(End));

        QListIterator<MMCPClient*> it(mPeersList);
        while (it.hasNext()) {
            MMCPClient* cl = it.next();
            cl->writeData(outMsg);
        }
    }

    const QString infoMsg = tr("[ CHAT ]  - You are now known as %1.").arg(name);
    mpHost->postMessage(infoMsg);

    return {true, QString()};
}

/**
 * Script command, sends side channel data to all clients
 */
QPair<bool, QString> MMCPServer::chatSideChannel(const QString& channel, const QString& msg)
{
    if (mPeersList.isEmpty()) {
        return {false, qsl("no connected clients")};
    }

    const QString outMsg = qsl("%1[%2]%3%4")
                                   .arg(static_cast<char>(SideChannel))
                                   .arg(channel, msg)
                                   .arg(static_cast<char>(End));
    QListIterator<MMCPClient*> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        //Only send to Mudlet clients
        if (cl->getVersion().contains("Mudlet", Qt::CaseInsensitive)) {
            cl->writeData(outMsg);
        }
    }

    return {true, QString()};
}


/**
 * Script command, Sends an unformatted chat message to everyone.
 * Warning, this has the potential for abuse!  But whatever, the chat protocol allows it.
 */
// Not currently used:
// QPair<bool, QString> MMCPServer::chatRaw(const QString& msg)
// {
//     if (mPeersList.isEmpty()) {
//         return {false, qsl("no connected clients")};
//     }

//     const QString outMsg = qsl("%1%2%3").arg(static_cast<char>(TextEveryone), msg, static_cast<char>(End));
//     QListIterator<MMCPClient*> it(mPeersList);
//     while (it.hasNext()) {
//         MMCPClient* cl = it.next();
//         cl->writeData(outMsg);
//     }

//     clientMessage(tr("<CHAT>%1")
//                           .arg(msg)
//                           .prepend(FBLDRED)
//                           .append(RST));
//     return {true, QString()};
// }


/**
 * Assigns a client to a chat group
 */
QPair<bool, QString> MMCPServer::chatSetGroup(const QVariant& target, const QString& group)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        const QString currentGroup = pClient->getGroup();
        bool assigned = pClient->setGroup(group);

        QString infoMsg;
        if (assigned) {
            infoMsg = tr("[ CHAT ]  - Assigned '%1' to group '%2'.")
                              .arg(pClient->chatName(), group);
        } else {
            infoMsg = tr("[ CHAT ]  - Removed '%1' from group '%2'.")
                              .arg(pClient->chatName(), currentGroup);
        }

        mpHost->postMessage(infoMsg);
        return {true, QString()};
    }

    const QString infoMsg = tr("[ CHAT ]  - Invalid client id '%1'.").arg(target.toString());
    mpHost->postMessage(infoMsg);
    return {false, qsl("no client by that name or id")};
}

/**
 * Script command, Send an emote message to everybody.
 */
QPair<bool, QString> MMCPServer::emoteAll(const QString& msg)
{
    if (mPeersList.isEmpty()) {
        return {false, qsl("no connected clients")};
    }

    const QString outMsg = qsl("%1%2 %3\n%4")
                                   .arg(static_cast<char>(TextEveryone))
                                   .arg(mChatName, msg)
                                   .arg(static_cast<char>(End));
    QListIterator<MMCPClient*> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        cl->writeData(outMsg);
    }

    using namespace AnsiColors;
    //: %1 is emote message sent to everyone
    clientMessage(tr("<CHAT>You emote to everyone: '%1'")
                          .arg(msg)
                          .prepend(FBLDRED)
                          .append(RST));
    return {true, QString()};
}

/**
 * Ignore or un-ignore a person
 */
QPair<bool, QString> MMCPServer::ignore(const QString& target)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        if (pClient->isIgnored()) {
            const QString infoMsg = tr("[ CHAT ]  - You are no longer ignoring %1.").arg(pClient->chatName());
            mpHost->postMessage(infoMsg);
        } else {
            const QString infoMsg = tr("[ CHAT ]  - You are now ignoring %1.").arg(pClient->chatName());
            mpHost->postMessage(infoMsg);
        }
        pClient->setIgnore(!pClient->isIgnored());
        return {true, QString()};
    }

    const QString infoMsg = tr("[ CHAT ]  - Cannot find client identified by '%1'.").arg(target);
    mpHost->postMessage(infoMsg);

    return {false, qsl("no client by that name or id")};
}

/**
 * Script command, send ping request to a client
 */
QPair<bool, QString> MMCPServer::ping(const QVariant& target)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        pClient->sendPingRequest();
        return {true, QString()};
    }

    return {false, qsl("no client by that name or id")};
}

/**
 * Script command, send peek connections request to a client
 */
QPair<bool, QString> MMCPServer::peek(const QVariant& target)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        pClient->sendPeekRequest();
        return {true, QString()};
    }

    return {false, qsl("no client by that name or id")};
}

/**
 * Toggle a client's private state
 */
QPair<bool, QString> MMCPServer::chatPrivate(const QVariant& target)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        if (pClient->isPrivate()) {
            pClient->setPrivate(false);
            const QString infoMsg = tr("[ CHAT ]  - %1 is no longer private.").arg(pClient->chatName());
            mpHost->postMessage(infoMsg);
        } else {
            pClient->setPrivate(true);
            const QString infoMsg = tr("[ CHAT ]  - %1 is now set as private.").arg(pClient->chatName());
            mpHost->postMessage(infoMsg);
        }
        return {true, QString()};
    }

    return {false, qsl("no client by that name or id")};
}

/**
 * Begin serving a client to our other connections
 */
QPair<bool, QString> MMCPServer::serve(const QVariant& target)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        if (pClient->isServed()) {
            pClient->setServed(false);
            pClient->sendMessage(qsl("<CHAT> You are no longer being served by %1.").arg(mChatName));

            const QString infoMsg = tr("[ CHAT ]  - You are no longer serving %1.").arg(pClient->chatName());
            mpHost->postMessage(infoMsg);

        } else {
            pClient->setServed(true);
            pClient->sendMessage(qsl("<CHAT> You are now being served by %1.").arg(mChatName));

            const QString infoMsg = tr("[ CHAT ]  - You are now serving %1.").arg(pClient->chatName());
            mpHost->postMessage(infoMsg);
        }

        return {true, QString()};
    }

    return {false, qsl("no client by that name or id")};
}

/**
 * Script command, start listening for incoming client connections
 */
QPair<bool, QString> MMCPServer::startServer(quint16 port)
{
    if (!listen(QHostAddress::Any, port)) {
        const QString infoMsg = tr("[ CHAT ]  - Unable to start server: %1.").arg(errorString());
        mpHost->postMessage(infoMsg);
        return {false, qsl("unable to start server")};
    }

    const QString infoMsg = tr("[ CHAT ]  - Started server on port %1.").arg(port);
    mpHost->postMessage(infoMsg);

    return {true, QString()};
}

/**
 * Script command, Stop our server from listening
 */
QPair<bool, QString> MMCPServer::stopServer()
{
    if (isListening()) {
        close();
        return {true, QString()};
    }

    return {false, qsl("unable to stop server, it is not listening")};
}


/**
 * Allow a client to snoop you (the user).
 */
QPair<bool, QString> MMCPServer::allowSnoop(const QVariant& target)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        if (pClient->canSnoop()) {
            pClient->setCanSnoop(false);
            pClient->setSnooped(false);
            pClient->sendMessage(qsl("<CHAT> You are no longer allowed to snoop %1.").arg(mChatName));

            const QString infoMsg = tr("[ CHAT ]  - %1 is no longer allowed to snoop you.").arg(pClient->chatName());
            mpHost->postMessage(infoMsg);

        } else {
            pClient->setCanSnoop(true);
            pClient->sendMessage(qsl("<CHAT> You are now allowed to snoop %1.").arg(mChatName));

            const QString infoMsg = tr("[ CHAT ]  - %1 can now snoop you.").arg(pClient->chatName());
            mpHost->postMessage(infoMsg);
        }

        return {true, QString()};
    }

    return {false, qsl("no client by that name")};
}

/**
 * Send a request to snoop someone, or stop snooping them if snooped
 */
QPair<bool, QString> MMCPServer::snoop(const QVariant& target)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        if (pClient->isSnooped()) {
            pClient->setSnooped(false);
        } else {
            pClient->snoop();
        }

        return {true, QString()};
    }

    return {false, qsl("no client by that name")};
}

/**
 * Unchat someone by name or id
 */
QPair<bool, QString> MMCPServer::unChat(const QVariant& target)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        pClient->disconnect();
        return {true, QString()};
    }

    return {false, qsl("no client by that name")};
}


void MMCPServer::addConnectedClient(MMCPClient* pClient)
{
    mPeersList.append(pClient);
    pClient->setId(mPeersList.indexOf(pClient));
}

void MMCPServer::slot_clientDisconnected(MMCPClient* pClient)
{
    mPeersList.removeOne(pClient);
}


/**
 * Send a message to the terminal pane - coloration has to be done by the caller
 */
void MMCPServer::clientMessage(const QString& message)
{
    // This uses a UTF-8 encoding:
    std::string trimmedStdStr = message.trimmed().toStdString();
    // The message sent to TMainConsole::printOnDisplay(...) MUST be in the
    // current Game Server Encoding - so we are going to have to transcode the
    // data if it is anything other than ASCII. Given that the primary usage for
    // MMCP is initially the Medievia MUD and that will be using the custom
    // encoder then longterm we may need a better way to inject stuff into the
    // system (so that the trigger engine can see it amongst other reasons).
    if (trimmedStdStr.empty()) {
        // If there is NO message then don't go further
        return;
    }

    // TMainConsole::printOnDisplay(...) calls TBuffer::translateToPlainText(...)
    // and if the data sent to that does NOT end with a Line-Feed - or certain
    // other end-of-line indications the text does not get flushed to the
    // display until it does - so actually we need to re-append a final
    // line-feed that we may have previously trimmed off!
    mpHost->mpConsole->printOnDisplay(trimmedStdStr.append(1, '\n'), false);
    mpHost->mpConsole->finalize();
}

/**
 * Received snoop data from a client, display it on screen
 */
void MMCPServer::snoopMessage(const std::string& message)
{
    using namespace AnsiColors;

    std::stringstream ss;
    ss << FBLDGRN << ">> " << RST;
    ss << message << "\n";

    std::string outStr = ss.str();

    mpHost->mpConsole->printOnDisplay(outStr, false);
    mpHost->mpConsole->finalize();
}


void MMCPServer::sendAll(QString& msg)
{
    QListIterator<MMCPClient*> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        cl->writeData(msg);
    }
}

/**
 * Send our public connections to everyone (not the client that requested)
 */
void MMCPServer::sendPublicConnections(MMCPClient* pClient)
{
    QStringList peerList;
    QListIterator<MMCPClient*> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();

        if (cl != pClient && !cl->isPrivate()) {
            peerList << qsl("%1,%2").arg(cl->host(), QString::number(cl->port()));
        }
    }

    if (!peerList.isEmpty()) {
        const QString cmdStr = peerList.join(QLatin1Char(','))
                                       .prepend(static_cast<char>(ConnectionList))
                                       .append(static_cast<char>(End));
        pClient->writeData(cmdStr);
    } /* else {
        // Not sure about this, but how else would a peer know that there aren't any connections?
        pClient->writeData(qsl("%1%2").arg(static_cast<char>(ConnectionList), static_cast<char>(End)));
    } */
}

/**
 * Send a peek list of our public connections to everyone (not the client that requested)
 */
void MMCPServer::sendPublicPeek(MMCPClient* pClient)
{
    QStringList peerList;

    QListIterator<MMCPClient*> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();

        if (cl != pClient && !cl->isPrivate()) {
            peerList << qsl("%1~%2~%3").arg(cl->host(), QString::number(cl->port()), cl->chatName());
        }
    }

    if (!peerList.isEmpty()) {
        const QString cmdStr = peerList.join(QLatin1Char('~'))
                                       .prepend(static_cast<char>(PeekList))
                                       .append(static_cast<char>(End));
        pClient->writeData(cmdStr);
    } else {
        pClient->sendMessage(qsl("<CHAT> %1 doesn't have any other connections").arg(mChatName));
    }
}

/**
 * Forward message to all clients.
 */
void MMCPServer::sendServedMessage(MMCPClient* pClient, const QString& msg)
{
    const QString cmdStr = qsl("%1%2%3")
                                   .arg(static_cast<char>(TextEveryone))
                                   .arg(msg)
                                   .arg(static_cast<char>(End));

    QListIterator<MMCPClient*> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        if (cl != pClient) {
            cl->writeData(cmdStr);
        }
    }
}

/**
 * Forward message to all served clients.
 */
// DRY: This method is identical to sendServedMessage(...) EXCEPT for extra "&& cl->isServed()" test condition
void MMCPServer::sendMessageToServed(MMCPClient* pClient, const QString& msg)
{
    const QString cmdStr = qsl("%1%2%3")
                                   .arg(static_cast<char>(TextEveryone))
                                   .arg(msg)
                                   .arg(static_cast<char>(End));

    QListIterator<MMCPClient*> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        if (cl != pClient && cl->isServed()) {
            cl->writeData(cmdStr);
        }
    }
}

/**
 * Set chat name and write to current profile
 */
void MMCPServer::setChatName(const QString& val)
{
    mChatName = val;
    // set the host chatname so it gets saved in the profile
    mpHost->setMMCPChatName(mChatName);
}
