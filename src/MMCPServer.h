#ifndef _MMCPSERVER_H_
#define _MMCPSERVER_H_

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

#include "pre_guard.h"
#include <QTcpServer>
#include "post_guard.h"

#include "utils.h"

class QString;

class MMCPClient;
class Host;

class MMCPServer : public QTcpServer
{
    Q_OBJECT
    Q_PROPERTY(QString getChatName READ getChatName WRITE setChatName)

public:
    inline static int MMCPDefaultHostPort = 4050;
    inline static QString DefaultMMCPChatName = qsl("MudletMMCP");
    inline static QLatin1String MMCPChatSideChannelEvent = QLatin1String("sysChatChannelMessage");

    explicit MMCPServer(Host*);
    ~MMCPServer();

    void receiveFromPlayer(std::string&);

    QPair<bool, QString> startServer(quint16);
    QPair<bool, QString> stopServer();

    QPair<bool, QString> call(const QString&);
    QPair<bool, QString> call(const QString&, int);
    QPair<bool, QString> chat(const QVariant&, const QString&);
    QPair<bool, QString> chatAll(const QString&);
    QPair<bool, QString> chatGroup(const QString&, const QString&);
    QPair<bool, QString> chatName(const QString&);
    QPair<bool, QString> chatList();
    QPair<bool, QString> chatSetGroup(const QVariant&, const QString&);
    QPair<bool, QString> chatSideChannel(const QString&, const QString&);
    QPair<bool, QString> chatRaw(const QString&);
    QPair<bool, QString> emoteAll(const QString&);
    QPair<bool, QString> ignore(const QString&);
    QPair<bool, QString> ping(const QVariant&);
    QPair<bool, QString> peek(const QVariant&);
    QPair<bool, QString> chatPrivate(const QVariant&);
    QPair<bool, QString> serve(const QVariant&);
    QPair<bool, QString> allowSnoop(const QVariant&);
    QPair<bool, QString> snoop(const QVariant&);
    QPair<bool, QString> unChat(const QVariant&);

    void clientMessage(const QString&);
    void snoopMessage(const std::string&);

    QList<MMCPClient*>* getClients() { return &clients; }

    void decrementSnoopCount() { snoopCount--; }
    void incrementSnoopCount() { snoopCount++; }

    void sendPublicConnections(MMCPClient*);
    void sendPublicPeek(MMCPClient*);
    void sendServedMessage(MMCPClient*, const QString&);
    void sendMessageToServed(MMCPClient*, const QString&);

    void addConnectedClient(MMCPClient*);
    void disconnectClient(MMCPClient*);

    QString getChatName() const { return m_chatName; }
    void setChatName(const QString&);

signals:
    void serverStarted(int);
    void clientConnected(MMCPClient*);
    void clientDisconnected(MMCPClient*);
    void printStatusMessage(const QString&);

public slots:
    void slotClientDisconnected(MMCPClient*);

protected:
    void incomingConnection(qintptr) override;

private:
    Host* mpHost = nullptr;
    QString m_chatName;

    QList<MMCPClient*> clients;
    int snoopCount;

    void sendSnoopData(std::string&);

    MMCPClient* clientByNameOrId(const QVariant&);

    void sendAll(QString&);
};

#endif
