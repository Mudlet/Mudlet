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

#include "MMCPServer.h"
#include "Host.h"
#include "MMCP.h"
#include "MMCPClient.h"
#include "mudlet.h"
#include "TEvent.h"

#include <string>

#include <QTcpServer>
#include <QHostAddress>
#include <QListIterator>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

MMCPServer::MMCPServer(Host* pHost)
: QTcpServer(pHost)
, mpHost(pHost)
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
void MMCPServer::sendSnoopData(std::string& lines)
{

    // Split the block into individual lines
    std::istringstream iss(lines);
    std::string line;

    //Note: Fore and Back colors use MudMaster color indices which are NOT the same as
    //      ANSI color indices.  Don't ask me why. So I'll just use background BLACK
    //      foreground WHITE, defined in MudMaster Colors.h

    // We're creating two byte arrays here, one that will be send to MudMaster
    // clients and the other (outData2) which will be send to all other clients

    while (std::getline(iss, line)) {
        QByteArray outData1, outData2;
        outData1.append(static_cast<char>(SnoopData));
        outData2.append(static_cast<char>(SnoopData));

        char colorBuf[16];
        snprintf(colorBuf, sizeof(colorBuf), "%02d%02d\n", 15, 0);
        outData1.append(colorBuf);

        outData1.append(line.data(), line.size());
        outData2.append(line.data(), line.size());

        // If line already had an 0xff at the end, don't bother adding one back here
        if (!line.empty() && static_cast<unsigned char>(line.back()) != static_cast<unsigned char>(End)) {
            outData1.append(static_cast<char>(End));
            outData2.append(static_cast<char>(End));
        }

        QListIterator<QPointer<MMCPClient>> it(mPeersList);
        while (it.hasNext()) {
            MMCPClient* cl = it.next();
            if (cl && cl->isSnooping()) {
                if (cl->getVersion().contains("MudMaster")) {
                    cl->writeData(outData1);
                } else {
                    cl->writeData(outData2);
                }
            }
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
        QListIterator<QPointer<MMCPClient>> it(mPeersList);
        while (it.hasNext()) {
            MMCPClient* cl = it.next();
            if (cl && !name.compare(cl->chatName(), Qt::CaseInsensitive)) {
                pClient = cl;
                break;
            }
        }

    } else {
        // It is an integer
        QListIterator<QPointer<MMCPClient>> it(mPeersList);
        while (it.hasNext()) {
            MMCPClient* cl = it.next();
            if (cl && cl->id() == id) {
                pClient = cl;
                break;
            }
        }
        /*
        if (id > 0 && id <= mPeersList.size()) {
            // And it is in range
            pClient = mPeersList[id - 1];
        }
        */
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
        bool ok = false;
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

    QListIterator<QPointer<MMCPClient>> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        if (cl && cl->host() == host && cl->port() == port) {
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

    // Don't add to the connection list here, we'll add it if the connection is
    // successful in the slot_connected() handler of MMCPClient
    pClient->tryConnect(host, port);

    return {true, QString()};
}


/**
 * Script command, Send private chat.
 */
QPair<bool, QString> MMCPServer::chatTo(const QVariant& target, const QString& msg)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        QByteArray out;
        out.append(static_cast<char>(TextPersonal));
        out.append(getChatName().toUtf8());
        out.append(" chats to you, '");
        out.append(msg.toUtf8());
        out.append("'\n");
        out.append(static_cast<char>(End));

        pClient->writeData(out);

        //const QString outMsg = qsl("%1%2 chats to you, '%3'\n%4")
        //                               .arg(static_cast<char>(TextPersonal), getChatName(), msg, static_cast<char>(End));
        //pClient->writeData(outMsg);

        using namespace AnsiColors;
        //: %1 is the name of the peer receiving the message %2

        clientMessage(pClient, tr("You chat to %1, '%2'")
                              .arg(pClient->chatName(), msg)
                              .prepend(FBLDRED + mpHost->getMMCPChatPrefix())
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

    QByteArray outMsg;
    outMsg.append(static_cast<char>(TextEveryone));
    outMsg.append(getChatName().toUtf8());
    outMsg.append(" chats to everybody, '");
    outMsg.append(msg.toUtf8());
    outMsg.append("'\n");
    outMsg.append(static_cast<char>(End));


    QListIterator<QPointer<MMCPClient>> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        if (cl) {
            cl->writeData(outMsg);
        }
    }

    using namespace AnsiColors;
    //: %1 is message sent to everyone

    clientMessage(tr("You chat to everybody, '%1'")
                          .arg(msg)
                          .prepend(FBLDRED + mpHost->getMMCPChatPrefix())
                          .append(RST));

    return {true, QString()};
}

QPair<bool, QString> MMCPServer::chatAccept(const QVariant& target)
{
    if (mPeersList.isEmpty()) {
        return {false, qsl("no connected clients")};
    }

    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient && pClient->isPending()) {
        pClient->acceptCall();
        return {true, QString()};
    }

    return {false, qsl("failed to accept incoming call")};
}

QPair<bool, QString> MMCPServer::chatDeny(const QVariant& target)
{
    if (mPeersList.isEmpty()) {
        return {false, qsl("no connected clients")};
    }

    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient && pClient->isPending()) {
        pClient->denyCall();
        return {true, QString()};
    }

    return {false, qsl("failed to deny incoming call")};
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
                            .arg(getChatName(), FBLDRED, message)
                            .arg(static_cast<char>(End));

    bool groupNotEmpty = false;
    QListIterator<QPointer<MMCPClient>> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        if (cl && cl->getGroup() == group) {
            groupNotEmpty = true;
            cl->writeData(outMsg);
        }
    }

    /*: %1 and %3 are ASCII ESC color codes that need to be included BEFORE a
     * portion of text (the main message %5) and (the group name %4)
     * respectively and %6 is another code at the very end to reset the colors
     * back to "normal". %2 is the prefix added to all chat messages display to us.
     * Please try and reproduce the positioning of those codes around the translation.
     */
    if (groupNotEmpty) {
        clientMessage(tr("%1%2You chat to %3<%4>%1, '%5'%6")
                              .arg(FBLDRED, mpHost->getMMCPChatPrefix(), FBLDCYN, group, message, RST));
        return {true, QString()};
    }

    /*: %1 and %3 are ASCII ESC color codes that need to be included BEFORE a
     * portion of text (the main message %5) and (the group name %4)
     * respectively and %5 is another code at the very end to reset the colors
     * back to "normal". %2 is the prefix added to all chat messages display to us.
     * Please try and reproduce the positioning of those codes around the translation.
     */
    clientMessage(tr("%1%2You try to chat to <%3%4%1> but it is empty and no-one hears you say: '%5'%6")
                          .arg(FBLDRED, mpHost->getMMCPChatPrefix(), FBLDCYN, group, message, RST));
    return {false, qsl("nobody in group '%1' now").arg(group)};
}

/**
 * Script command, Display a list of connected chat clients on the main console
 */
QPair<bool, QString> MMCPServer::displayClientList()
{
    using namespace AnsiColors;

    // Translated column headers
    const QString hdrId      = tr("Id");
    const QString hdrName    = tr("Name");
    const QString hdrAddress = tr("Address");
    const QString hdrPort    = tr("Port");
    const QString hdrGroup   = tr("Group");
    const QString hdrFlags   = tr("Flags");
    const QString hdrClient  = tr("ChatClient");

    // Compute column widths as max of translated header and minimum data width
    const int colId      = qMax(4,  hdrId.length());
    const int colName    = qMax(20, hdrName.length());
    const int colAddress = qMax(20, hdrAddress.length());
    const int colPort    = qMax(5,  hdrPort.length());
    const int colGroup   = qMax(15, hdrGroup.length());
    const int colFlags   = qMax(8,  hdrFlags.length());
    const int colClient  = qMax(16, hdrClient.length());

    auto sep = [](int w) { return QString(w, QChar('=')); };

    const QString header  = qsl("%1%2 %3 %4 %5 %6 %7 %8")
                                .arg(RST)
                                .arg(hdrId, -colId)
                                .arg(hdrName, -colName)
                                .arg(hdrAddress, -colAddress)
                                .arg(hdrPort, colPort)
                                .arg(hdrGroup, -colGroup)
                                .arg(hdrFlags, -colFlags)
                                .arg(hdrClient);

    const QString divider = qsl("%1 %2 %3 %4 %5 %6 %7")
                                .arg(sep(colId), sep(colName), sep(colAddress), sep(colPort),
                                     sep(colGroup), sep(colFlags), sep(colClient));

    QStringList peersList;
    QListIterator<QPointer<MMCPClient>> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* pClient = it.next();
        if (!pClient) {
            continue;
        }

        peersList << qsl("%1%2 %3%4 %5")
                            .arg(pClient->isPending() ? FBLDYEL : FBLDGRN)
                            .arg(pClient->id(), colId)
                            .arg(RST, pClient->getInfoString(colName, colAddress, colPort, colGroup, colFlags), pClient->getVersion());
    }

    // Maybe one day we'll implement Transfers, although there are other/better ways of
    // sharing scripts with Mudlet

    const QString strMessage = qsl("%1\n%2\n%3%4\n"
        // Label in chat client list and color key for connected peers
        "%5%6  %7%8%9\n"
        // Flags legend in chat client list
        "%10%9")
        .arg(header, divider,
            peersList.isEmpty() ? QString() : qsl("%1\n").arg(peersList.join(QChar::LineFeed)),
            divider,
            qsl("%1: ").arg(tr("Color Key")), qsl("%1%2").arg(FBLDGRN, tr("Connected")),
            qsl("%1%2").arg(FBLDYEL, tr("Pending")),
            RST,
            RST,
            // %1 is the translated word for "Flags". The letters F, I, P, S, n, N are
            // fixed flag codes and must NOT be translated. Only translate the descriptions.
            tr("%1:  F - %2,  I - %3,  P - %4,  S - %5\n"
               "        n - %6,  N - %7")
                .arg(tr("Flags"), tr("Firewall"), tr("Ignored"), tr("Private"),
                     tr("Serving"), tr("Allow Snooping"), tr("Being Snooped")));

    clientMessage(strMessage);
    return {true, QString()};
}


/**
 * Script Command, Set our chat name to name, and tell connected chat clients
 */
QPair<bool, QString> MMCPServer::chatName(const QString& name)
{
    QRegularExpression rx(qsl("~|,"));
    if (rx.match(name).hasMatch()) {
        const QString errorMsg = tr("[ CHAT ]  - Invalid chat name: tilde (~) and comma (,) are not allowed.");
        mpHost->postMessage(errorMsg);
        return {false, qsl("Invalid chat name: tilde (~) and comma (,) are not allowed.")};
    }

    if (!mPeersList.isEmpty()) {
        QByteArray outMsg;
        outMsg.append(static_cast<char>(NameChange));
        outMsg.append(name.toUtf8());
        outMsg.append(static_cast<char>(End));

        QListIterator<QPointer<MMCPClient>> it(mPeersList);
        while (it.hasNext()) {
            MMCPClient* cl = it.next();
            if (cl) {
                cl->writeData(outMsg);
            }
        }
    }

    const QString infoMsg = tr("[ CHAT ]  - You are now known as %1.").arg(name);
    mpHost->postMessage(infoMsg);

    return {true, QString()};
}

/**
 * Script command, sends side channel data to all clients
 */
QPair<bool, QString> MMCPServer::sendSideChannel(const QString& channel, const QString& msg)
{
    if (mPeersList.isEmpty()) {
        return {false, qsl("no connected clients")};
    }

    QByteArray outMsg;
    outMsg.append(static_cast<char>(SideChannel));
    outMsg.append(channel.toUtf8());
    outMsg.append(',');
    outMsg.append(msg.toUtf8());
    outMsg.append(static_cast<char>(End));
    QListIterator<QPointer<MMCPClient>> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        //Only send to Mudlet clients
        if (cl && cl->getVersion().contains("Mudlet", Qt::CaseInsensitive)) {
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
//     QListIterator<QPointer<MMCPClient>> it(mPeersList);
//     while (it.hasNext()) {
//         MMCPClient* cl = it.next();
//         if (cl) {
//             cl->writeData(outMsg);
//         }
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

    QByteArray outMsg;
    outMsg.append(static_cast<char>(TextEveryone));
    outMsg.append(getChatName().toUtf8());
    outMsg.append(' ');
    outMsg.append(msg.toUtf8());
    outMsg.append(static_cast<char>(End));

    QListIterator<QPointer<MMCPClient>> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        if (cl) {
            cl->writeData(outMsg);
        }
    }

    using namespace AnsiColors;
    //: %1 is player's name,  %2 is the emote message sent to everyone

    if (mpHost->getMMCPPrefixEmotes()) {
        clientMessage(tr("You emote to everyone: '%1 %2'")
                          .arg(getChatName(), msg)
                          .prepend(FBLDRED + mpHost->getMMCPChatPrefix())
                          .append(RST));
    } else {
        clientMessage(tr("%1 %2")
                          .arg(getChatName(), msg)
                          .prepend(FBLDRED + mpHost->getMMCPChatPrefix())
                          .append(RST));
    }

    return {true, QString()};
}

/**
 * Script command, return client flags string
 */
QPair<bool, QString> MMCPServer::getClientFlags(const QString& target)
{
    if (mPeersList.isEmpty()) {
        return {false, qsl("no connected clients")};
    }

    MMCPClient* pClient = clientByNameOrId(target);

    if (!pClient) {
        return {false, qsl("invalid client name or id: %1").arg(target)};
    }

    return {true, pClient->getFlagsString()};
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
 * Script command, send connections request to a client
 */
QPair<bool, QString> MMCPServer::request(const QVariant& target)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        pClient->sendRequestConnections();
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
            pClient->sendMessage(qsl("<CHAT> You are no longer being served by %1.").arg(getChatName()));

            const QString infoMsg = tr("[ CHAT ]  - You are no longer serving %1.").arg(pClient->chatName());
            mpHost->postMessage(infoMsg);

        } else {
            pClient->setServed(true);
            pClient->sendMessage(qsl("<CHAT> You are now being served by %1.").arg(getChatName()));

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
        QString errorStr = errorString();
        // The OS may return an unusefull error message (just a period)
        // It's likey in that case that the port is already in use
        if (errorStr.startsWith(".")) {
            errorStr = qsl("Port already in use?");
        }
        const QString infoMsg = tr("[ CHAT ]  - Unable to start server: %1.").arg(errorStr);
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

        mpHost->postMessage(tr("[ CHAT ]  - Stopped server"));

        return {true, QString()};
    }

    mpHost->postMessage(tr("[ CHAT ]  - Unable to stop server, it is not running"));

    return {false, qsl("unable to stop server, it is not listening")};
}


/**
 * Toggle the server's Do Not Disturb
 */
void MMCPServer::toggleDoNotDisturb()
{
    mDoNotDisturb = !mDoNotDisturb;

    if (mDoNotDisturb) {
        mpHost->postMessage(tr("[ CHAT ]  - DoNotDisturb enabled."));
    } else {
        mpHost->postMessage(tr("[ CHAT ]  - DoNotDisturb disabled."));
    }
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
            pClient->setSnooping(false);
            pClient->sendMessage(qsl("<CHAT> You are no longer allowed to snoop %1.").arg(getChatName()));

            const QString infoMsg = tr("[ CHAT ]  - %1 is no longer allowed to snoop you.").arg(pClient->chatName());
            mpHost->postMessage(infoMsg);

        } else {
            pClient->setCanSnoop(true);
            pClient->sendMessage(qsl("<CHAT> You are now allowed to snoop %1.").arg(getChatName()));

            const QString infoMsg = tr("[ CHAT ]  - %1 can now snoop you.").arg(pClient->chatName());
            mpHost->postMessage(infoMsg);
        }

        return {true, QString()};
    }

    return {false, qsl("no client by that name or id")};
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

    return {false, qsl("no client by that name or id")};
}

/**
 * Disconnect someone by name or id
 */
QPair<bool, QString> MMCPServer::disconnect(const QVariant& target)
{
    MMCPClient* pClient = clientByNameOrId(target);

    if (pClient) {
        pClient->disconnect();
        return {true, QString()};
    }

    return {false, qsl("no client by that name or id")};
}


quint16 MMCPServer::addConnectedClient(MMCPClient* pClient)
{
    if (!mPeersList.contains(pClient)) {
        mPeersList.append(pClient);
    }

    pClient->setId(mPeersList.indexOf(pClient) + 1);

    // Raise event after client has been added to mPeersList
    // in case they want to use getClientList in response to the event
    TEvent event {};
    event.mArgumentList << csMMCPPeerUpdateEvent;
    event.mArgumentList << pClient->chatName();
    event.mArgumentTypeList << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING;
    mpHost->raiseEvent(event);

    return pClient->id();
}


void MMCPServer::slot_clientDisconnected(MMCPClient* pClient)
{

    mPeersList.removeOne(pClient);
    QListIterator<QPointer<MMCPClient>> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        if (!cl) {
            continue;
        }
        cl->setId(mPeersList.indexOf(cl) + 1);
    }

    // Raise event after client has been removed from mPeersList
    // in case they want to use getClientList in response to the event
    TEvent event {};
    event.mArgumentList << csMMCPPeerUpdateEvent;
    event.mArgumentList << pClient->chatName();
    event.mArgumentTypeList << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING;
    mpHost->raiseEvent(event);

    pClient->deleteLater();
}

void MMCPServer::postChatMessage(const QString &peerName, const QString& msg) {
    TEvent event {};
    event.mArgumentList << csMMCPChatChannelEvent;
    event.mArgumentList << peerName;
    event.mArgumentList << msg;
    event.mArgumentTypeList << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING;
    mpHost->raiseEvent(event);
}


/**
 * Send a message to the terminal pane - coloration HAS to be done by the caller
 * because group messages in particular can have multiple colors which cannot
 * be done here.
 */
void MMCPServer::clientMessage(const QString& message)
{
    clientMessage("System", message);
}
void MMCPServer::clientMessage(MMCPClient *fromClient, const QString& message)
{
    clientMessage(fromClient->chatName(), message);
}
void MMCPServer::clientMessage(const QString& fromStr, const QString& message)
{
    if (!mpHost || mpHost->isClosingDown()) {
        // Don't try to process any messages if the profile is dying - otherwise
        // we can get seg. faults when we try to use
        // TMainConsole::printOnDisplay(...) - I found this the hard way! Slysven
        return;
    }

    using namespace AnsiColors;

    // We need to encapsulate displayed string in the default chat color because
    // many people are terrible at writing custom chat scripts and do not terminate
    // their chats with a RESET, this avoids color bleeding
    // Additionally I suppose that we need to provide a setting for COMPACT, but until then
    // we need to prepend a carriage return so multiple chat messages have spacing between them.
    const QString coloredStr = QString("%1%2%3")
        .arg(FBLDRED)
        .arg(message.trimmed())
        .arg(RST)
        .prepend(mpHost->getMMCPAddChatMessageNewline() ? "\n" : "");

    // Raise MMCPChatMessage event with our client name and the trimmed & colorized message
    // from the client
    postChatMessage(fromStr, coloredStr);

    // This uses a UTF-8 encoding:
    std::string trimmedStdStr = coloredStr.toStdString();
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
    ss << FBLDGRN << ">>" << RST;
    ss << message << "\n";

    std::string outStr = ss.str();

    mpHost->mpConsole->printOnDisplay(outStr, false);
    mpHost->mpConsole->finalize();
}


void MMCPServer::sendAll(QString& msg)
{
    QListIterator<QPointer<MMCPClient>> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        if (cl) {
            cl->writeData(msg);
        }
    }
}

/**
 * Send our public connections to everyone (not the client that requested)
 */
void MMCPServer::sendPublicConnections(MMCPClient* pClient)
{
    QStringList peerList;
    QListIterator<QPointer<MMCPClient>> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();

        if (cl && cl != pClient && !cl->isPrivate()) {
            peerList << qsl("%1,%2").arg(cl->host(), QString::number(cl->port()));
        }
    }

    if (!peerList.isEmpty()) {
        const QString cmdStr = peerList.join(QLatin1Char(','))
                                       .prepend(static_cast<char>(ConnectionList))
                                       .append(static_cast<char>(End));
        pClient->writeData(cmdStr);
    }
    /*
    else {
        // Not sure about this, but how else would a peer know that there aren't any connections?
        pClient->writeData(qsl("%1%2").arg(static_cast<char>(ConnectionList), static_cast<char>(End)));
    }
    */
}

/**
 * Send a peek list of our public connections to everyone (not the client that requested)
 */
void MMCPServer::sendPublicPeek(MMCPClient* pClient)
{
    QStringList peerList;

    QListIterator<QPointer<MMCPClient>> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();

        if (cl && cl != pClient && !cl->isPrivate()) {
            peerList << qsl("%1~%2~%3").arg(cl->host(), QString::number(cl->port()), cl->chatName());
        }
    }

    if (!peerList.isEmpty()) {
        const QString cmdStr = peerList.join(QLatin1Char('~'))
                                       .prepend(static_cast<char>(PeekList))
                                       .append(QLatin1Char('~'))
                                       .append(static_cast<char>(End));

        pClient->writeData(cmdStr);
    } else {
        pClient->sendMessage(qsl("<CHAT> %1 doesn't have any other connections").arg(getChatName()));
    }
}

/**
 * Forward message to all clients.
 * If onlyToServed is set, only send this message to served clients
 */
void MMCPServer::sendServedMessage(MMCPClient* pClient, const QString& msg, bool onlyToServed)
{
    QByteArray cmdStr;
    cmdStr.append(static_cast<char>(TextEveryone));
    cmdStr.append(msg.toUtf8());
    cmdStr.append(static_cast<char>(End));

    QListIterator<QPointer<MMCPClient>> it(mPeersList);
    while (it.hasNext()) {
        MMCPClient* cl = it.next();
        // If onlyToServed == false -> (!onlyToServed == true) -> send to all
        // If onlyToServed == true -> only send if cl->isServed() is true
        if (cl && cl != pClient && (!onlyToServed || cl->isServed())) {
            cl->writeData(cmdStr);
        }
    }
}

const QString& MMCPServer::getChatName() const
{
    if (mpHost) {
        return mpHost->getMMCPChatName();
    }

    return csDefaultMMCPChatName;
}


