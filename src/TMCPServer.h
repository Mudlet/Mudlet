#ifndef MUDLET_TMCPSERVER_H
#define MUDLET_TMCPSERVER_H

/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vperetokin@gmail.com          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "Host.h"
#include "utils.h"

#include "pre_guard.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QTimer>
#include <QVariantMap>
#include "post_guard.h"

class TMCPLuaBridge;

class TMCPServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit TMCPServer(Host* pHost, QObject* parent = nullptr);
    ~TMCPServer();

    bool startServer(int port = 0);
    void stopServer();
    bool isRunning() const;
    int getPort() const;
    QString getServerInfo() const;

    static constexpr const char* MCP_PROTOCOL_VERSION = "2024-11-05";
    static constexpr const char* MCP_SERVER_NAME = "mudlet";
    static constexpr const char* MCP_SERVER_VERSION = "1.0.0";

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void handleClientConnected();
    void handleClientDisconnected();
    void handleClientReadyRead();
    void handleClientError(QAbstractSocket::SocketError error);

private:
    struct MCPClient {
        QTcpSocket* socket;
        QString clientName;
        QString clientVersion;
        bool initialized;
        int requestId;
    };

    void sendJsonRpcResponse(MCPClient* client, const QJsonObject& response);
    void sendJsonRpcError(MCPClient* client, int id, int code, const QString& message, const QJsonValue& data = QJsonValue());
    void handleInitializeRequest(MCPClient* client, const QJsonObject& request);
    void handleListToolsRequest(MCPClient* client, const QJsonObject& request);
    void handleCallToolRequest(MCPClient* client, const QJsonObject& request);
    void handlePingRequest(MCPClient* client, const QJsonObject& request);

    QJsonObject createServerInfo() const;
    QJsonArray getAvailableTools() const;

    Host* mpHost;
    TMCPLuaBridge* mpLuaBridge;
    QMap<QTcpSocket*, MCPClient*> mClients;
    int mPort;
    bool mServerRunning;

    enum JsonRpcErrorCode {
        ParseError = -32700,
        InvalidRequest = -32600,
        MethodNotFound = -32601,
        InvalidParams = -32602,
        InternalError = -32603,
        ServerError = -32000
    };
};

#endif // MUDLET_TMCPSERVER_H