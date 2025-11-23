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
#include <QString>
#include <QTimer>
#include <QVariantMap>
#include <QUrl>
#include <QUrlQuery>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QHttpHeaders>
#include <QTcpServer>
#include "post_guard.h"

class TMCPLuaBridge;

class TMCPServer : public QObject
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

    static constexpr const char* MCP_PROTOCOL_VERSION = "2025-06-18";
    static constexpr const char* MCP_SERVER_NAME = "mudlet";
    static constexpr const char* MCP_SERVER_VERSION = "1.0.0";

private slots:
    void notifyToolsChanged();

private:
    struct MCPSession {
        QString sessionId;
        QString clientName;
        QString clientVersion;
        bool initialized;
        int requestId;
    };

    QHttpServerResponse handleHttpRequest(const QHttpServerRequest& request);
    QHttpServerResponse handleMcpPost(const QHttpServerRequest& request);
    QHttpServerResponse handleMcpGet(const QHttpServerRequest& request);

    void handleInitializeRequest(const QJsonObject& request, QJsonObject& response, MCPSession* session);
    void handleListToolsRequest(const QJsonObject& request, QJsonObject& response);
    void handleCallToolRequest(const QJsonObject& request, QJsonObject& response);
    void handlePingRequest(const QJsonObject& request, QJsonObject& response);
    void handleNotificationRequest(const QJsonObject& request);

    QJsonObject createJsonRpcError(int id, int code, const QString& message, const QJsonValue& data = QJsonValue());
    QString generateSessionId();
    MCPSession* getOrCreateSession(const QString& sessionId);

    QJsonObject createInitializeResult(const QString& negotiatedVersion) const;
    QJsonArray getAvailableTools() const;
    void sendNotificationToAllSessions(const QString& method, const QJsonObject& params = QJsonObject());

    Host* mpHost;
    TMCPLuaBridge* mpLuaBridge;
    QHttpServer* mpHttpServer;
    QTcpServer* mpTcpServer;
    QMap<QString, MCPSession*> mSessions;
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
