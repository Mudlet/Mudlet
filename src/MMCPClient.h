
#pragma once

#include <QtNetwork>
#include <QObject>
#include <QString>

#include "MMCP.h"


class MMCPServer;
class Host;

class MMCPClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(int id READ id WRITE setId)
    Q_PROPERTY(bool canSnoop READ canSnoop WRITE setCanSnoop)
    Q_PROPERTY(QString chatName READ chatName)
    Q_PROPERTY(bool isIgnored READ isIgnored WRITE setIgnore)
    Q_PROPERTY(bool isPrivate READ isPrivate WRITE setPrivate)
    Q_PROPERTY(bool isServed READ isServed WRITE setServed)
    Q_PROPERTY(bool isServing READ isServing WRITE setServing)
    Q_PROPERTY(bool isSnooped READ isSnooped WRITE setSnooped)
    Q_PROPERTY(bool isSnooping READ isSnooping WRITE setSnooping)
    Q_PROPERTY(int state READ state)
    Q_PROPERTY(QString host READ host);
    Q_PROPERTY(quint16 port READ port);
    
    enum ClientState {
        Disconnected = 0,
        ConnectingIn,
        ConnectingOut,
        Connected
    };

    public:
        MMCPClient(Host*, MMCPServer *);
        
        void incoming(int);
        void tryConnect(const QString&, quint16);
        void writeData(const QString &);
        void sendMessage(const QString &);
        void sendPingRequest();
        
        void disconnect();
        
        void sendChat(const QString &msg, MMCPChatCommands);
        
        void snoop();
        
        const QString getInfoString();
        const QString getFlagsString();
        const QString& getVersion();
        QString host();
        quint16 port();
        
        //Property Accessors/Mutators
        int id() { return m_id; }
        void setId(int val) { m_id = val; }
        
        bool canSnoop() { return m_canSnoop; }
        void setCanSnoop(bool val) { m_canSnoop = val; }
        
        const QString& chatName() { return m_chatName; }
        
        bool isIgnored() { return m_isIgnored; }
        void setIgnore(bool val) { m_isIgnored = val; }
        
        bool isPrivate() { return m_isPrivate; }
        void setPrivate(bool val) { m_isPrivate = val; }
        
        bool isServed() { return m_isServed; }
        void setServed(bool val) { m_isServed = val; }
        
        bool isServing() { return m_isServing; }
        void setServing(bool val) { m_isServing = val; }
        
        bool isSnooped() { return m_isSnooped; }
        void setSnooped(bool val) { m_isSnooped = val; }
        
        bool isSnooping() { return m_isSnooping; }
        void setSnooping(bool val) { m_isSnooping = val; }
        
        int state() { return m_state; }
        
        MMCPServer *getServer() { return server; }
        
    signals:
        void clientDisconnected(MMCPClient *);
    
    private slots:
        void slotConnected();
        void slotDisconnected();
        void slotReadData();
        void slotDisplayError(QAbstractSocket::SocketError socketError);
    
    private:
        int m_id;
        bool m_canSnoop;
        QString m_chatName;
        bool m_isIgnored;
        bool m_isPrivate;
        bool m_isServed;
        bool m_isServing;	//do we actually know this?
        bool m_isSnooped;
        bool m_isSnooping;
        int m_state;
        QString m_host;
        quint16 m_port;
        QByteArray buffer;
    
        QTcpSocket tcpSocket;
        Host *mpHost;
        MMCPServer *server;
        QString version;
        
        void sendVersion();
        void handleConnectedState(const QByteArray &);
        void handleIncomingChannelData(const QString &);
        void handleIncomingChatEveryone(const QString &);
        void handleIncomingChatPersonal(const QString &);
        void handleIncomingConnectionsRequest();
        void handleIncomingMessage(const QString &);
        void handleIncomingNameChange(const QString &);
        void handleIncomingPingRequest(const QString &);
        void handleIncomingPingResponse(const QString &);
        void handleIncomingSnoop();
        void handleIncomingSnoopData(const QString &);
        
};