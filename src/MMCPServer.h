#ifndef _MMCPSERVER_H_
#define _MMCPSERVER_H_

#include "pre_guard.h"
#include <QTcpServer>
#include "post_guard.h"

#include "utils.h"

class QString;

class MMCPClient;
class Host;

class MMCPServer : public QTcpServer {
    Q_OBJECT
    Q_PROPERTY(QString getChatName READ getChatName WRITE setChatName)
    
    public:
        inline static int MMCPDefaultHostPort = 4050;
        inline static QString DefaultMMCPChatName = qsl("MudletMMCP");

        explicit MMCPServer(Host*);
        ~MMCPServer();

        bool receiveFromPlayer(std::string &);

        QPair<bool, QString> startServer(quint16);
        QPair<bool, QString> stopServer();
        
        QPair<bool, QString> call(const QString&);
        QPair<bool, QString> call(const QString&, int);
        QPair<bool, QString> chat(const QVariant&, const QString&);
        QPair<bool, QString> chatAll(const QString&);
        QPair<bool, QString> chatName(const QString&);
        QPair<bool, QString> chatList();
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
        
        QList<MMCPClient *> *getClients() { return &clients; }
        
        void decrementSnoopCount() { snoopCount--; }
        void incrementSnoopCount() { snoopCount++; }
        
        void sendPublicConnections(MMCPClient *);
        void sendPublicPeek(MMCPClient *);
        void sendServedMessage(MMCPClient *, const QString &);
        void sendMessageToServed(MMCPClient *, const QString &);
    
        void addConnectedClient(MMCPClient *);
        void disconnectClient(MMCPClient *);
        
        QString getChatName() const { return m_chatName; }
        void setChatName(const QString &);

    signals:
        void serverStarted(int);
        void clientConnected(MMCPClient *);
        void clientDisconnected(MMCPClient *);
        void printStatusMessage(const QString &);
        
    public slots:
        void slotClientDisconnected(MMCPClient *);

    protected:
        void incomingConnection(qintptr) override;

    private:

        Host *mpHost = nullptr;
        QString m_chatName;
    
        QList<MMCPClient *> clients;
        int snoopCount;
        
        void sendSnoopData(std::string &);
        
        MMCPClient *clientByNameOrId(const QVariant &);
        
        void sendAll(QString&);

};

#endif
