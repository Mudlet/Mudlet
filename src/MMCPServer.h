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
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled)
    Q_PROPERTY(QString getChatName READ getChatName WRITE setChatName)
    Q_PROPERTY(QString address READ address WRITE setAddress)
    Q_PROPERTY(int port READ port WRITE setPort)
    
    public:
        inline static int MMCPDefaultHostPort = 4050;
        inline static QString DefaultMMCPChatName = qsl("MudletMMCP");
        inline static QString MMCPHostNameCfgItem = qsl("mmcp_host");
        inline static QString MMCPHostPortCfgItem = qsl("mmcp_port");
        inline static QString MMCPChatNameCfgItem = qsl("mmcp_chatname");

        static quint16 readMMCPHostPort(Host*);
        static QString readMMCPChatName(Host*);

        static QPair<bool, QString> writeChatName(Host*, const QString&);

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
        
        
        //Property Accessors/Mutators
        
        bool enabled() const { return m_enabled; }
        void setEnabled(bool val) { m_enabled = val; }
        
        QString getChatName() const { return m_chatName; }
        void setChatName(const QString &);
        
        QString address() const { return m_address; }
        void setAddress(const QString &val) { m_address = val; }
        
        int port() { return m_port; }
        void setPort(int val) { m_port = val; }

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

        static QString readAppDefaultMMCPChatName();
        static void writeAppDefaultMMCPChatName(const QString&);
        static QString readAppDefaultMMCPAutostart();

        Host *mpHost = nullptr;
        bool m_enabled;
        QString m_chatName;
        QString m_address;
        int m_port;
        int mMessageBufferLimit = 0;
    
        QList<MMCPClient *> clients;
        int snoopCount;
        
        void sendSnoopData(std::string &);
        
        MMCPClient *clientByNameOrId(const QVariant &);
        
        void sendAll(QString&);

};

#endif
