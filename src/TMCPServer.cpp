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

#include "TMCPServer.h"
#include "TMCPLuaBridge.h"
#include "Host.h"

#include <QDebug>
#include <QJsonParseError>
#include <QNetworkInterface>

TMCPServer::TMCPServer(Host* pHost, QObject* parent)
: QTcpServer(parent)
, mpHost(pHost)
, mpLuaBridge(nullptr)
, mPort(0)
, mServerRunning(false)
{
    mpLuaBridge = new TMCPLuaBridge(mpHost, this);
}

TMCPServer::~TMCPServer()
{
    stopServer();
}

bool TMCPServer::startServer(int port)
{
    if (mServerRunning) {
        return false;
    }

    if (!mpLuaBridge->loadLuaFunctions()) {
        qWarning() << "TMCPServer: Failed to load Lua functions";
        return false;
    }

    QHostAddress address = QHostAddress::LocalHost;
    if (!listen(address, port)) {
        qWarning() << "TMCPServer: Failed to start server on port" << port << ":" << errorString();
        return false;
    }

    mPort = serverPort();
    mServerRunning = true;

    qDebug() << "TMCPServer: Started on" << address.toString() << ":" << mPort;
    return true;
}

void TMCPServer::stopServer()
{
    if (!mServerRunning) {
        return;
    }

    for (auto client : mClients) {
        client->socket->disconnectFromHost();
        client->socket->deleteLater();
        delete client;
    }
    mClients.clear();

    close();
    mServerRunning = false;
    mPort = 0;

    qDebug() << "TMCPServer: Stopped";
}

bool TMCPServer::isRunning() const
{
    return mServerRunning;
}

int TMCPServer::getPort() const
{
    return mPort;
}

QString TMCPServer::getServerInfo() const
{
    if (!mServerRunning) {
        return tr("MCP Server: Not running");
    }
    return tr("MCP Server: Running on localhost:%1 (%2 clients)").arg(mPort).arg(mClients.size());
}

void TMCPServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket* socket = new QTcpSocket(this);
    if (!socket->setSocketDescriptor(socketDescriptor)) {
        delete socket;
        return;
    }

    MCPClient* client = new MCPClient{
        socket,
        QString(),
        QString(),
        false,
        1
    };

    mClients[socket] = client;

    connect(socket, &QTcpSocket::connected, this, &TMCPServer::handleClientConnected);
    connect(socket, &QTcpSocket::disconnected, this, &TMCPServer::handleClientDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &TMCPServer::handleClientReadyRead);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &TMCPServer::handleClientError);

    qDebug() << "TMCPServer: New client connected from" << socket->peerAddress().toString();
}

void TMCPServer::handleClientConnected()
{
    qDebug() << "TMCPServer: Client fully connected";
}

void TMCPServer::handleClientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !mClients.contains(socket)) {
        return;
    }

    MCPClient* client = mClients.take(socket);
    qDebug() << "TMCPServer: Client disconnected:" << client->clientName;
    delete client;
    socket->deleteLater();
}

void TMCPServer::handleClientError(QAbstractSocket::SocketError error)
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    qWarning() << "TMCPServer: Client socket error:" << error << socket->errorString();
}

void TMCPServer::handleClientReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !mClients.contains(socket)) {
        return;
    }

    MCPClient* client = mClients[socket];

    while (socket->canReadLine()) {
        QByteArray data = socket->readLine().trimmed();
        if (data.isEmpty()) {
            continue;
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            sendJsonRpcError(client, -1, ParseError, tr("JSON parse error: %1").arg(parseError.errorString()));
            continue;
        }

        QJsonObject request = doc.object();
        QString method = request[qsl("method")].toString();

        if (method == qsl("initialize")) {
            handleInitializeRequest(client, request);
        } else if (method == qsl("tools/list")) {
            handleListToolsRequest(client, request);
        } else if (method == qsl("tools/call")) {
            handleCallToolRequest(client, request);
        } else if (method == qsl("ping")) {
            handlePingRequest(client, request);
        } else {
            int id = request[qsl("id")].toInt(-1);
            sendJsonRpcError(client, id, MethodNotFound, tr("Method not found: %1").arg(method));
        }
    }
}

void TMCPServer::sendJsonRpcResponse(MCPClient* client, const QJsonObject& response)
{
    QJsonDocument doc(response);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    client->socket->write(data);
}

void TMCPServer::sendJsonRpcError(MCPClient* client, int id, int code, const QString& message, const QJsonValue& data)
{
    QJsonObject error;
    error[qsl("code")] = code;
    error[qsl("message")] = message;
    if (!data.isNull()) {
        error[qsl("data")] = data;
    }

    QJsonObject response;
    response[qsl("jsonrpc")] = qsl("2.0");
    response[qsl("id")] = id;
    response[qsl("error")] = error;

    sendJsonRpcResponse(client, response);
}

void TMCPServer::handleInitializeRequest(MCPClient* client, const QJsonObject& request)
{
    QJsonObject params = request[qsl("params")].toObject();
    client->clientName = params[qsl("clientInfo")].toObject()[qsl("name")].toString();
    client->clientVersion = params[qsl("clientInfo")].toObject()[qsl("version")].toString();
    client->initialized = true;

    QJsonObject serverInfo = createServerInfo();
    QJsonObject response;
    response[qsl("jsonrpc")] = qsl("2.0");
    response[qsl("id")] = request[qsl("id")];
    response[qsl("result")] = serverInfo;

    sendJsonRpcResponse(client, response);
    qDebug() << "TMCPServer: Initialized client:" << client->clientName << client->clientVersion;
}

void TMCPServer::handleListToolsRequest(MCPClient* client, const QJsonObject& request)
{
    if (!client->initialized) {
        sendJsonRpcError(client, request[qsl("id")].toInt(), ServerError, tr("Client not initialized"));
        return;
    }

    QJsonObject result;
    result[qsl("tools")] = getAvailableTools();

    QJsonObject response;
    response[qsl("jsonrpc")] = qsl("2.0");
    response[qsl("id")] = request[qsl("id")];
    response[qsl("result")] = result;

    sendJsonRpcResponse(client, response);
}

void TMCPServer::handleCallToolRequest(MCPClient* client, const QJsonObject& request)
{
    if (!client->initialized) {
        sendJsonRpcError(client, request[qsl("id")].toInt(), ServerError, tr("Client not initialized"));
        return;
    }

    QJsonObject params = request[qsl("params")].toObject();
    QString toolName = params[qsl("name")].toString();
    QJsonObject arguments = params[qsl("arguments")].toObject();

    MCPToolResult result = mpLuaBridge->callTool(toolName, arguments);

    QJsonObject response;
    response[qsl("jsonrpc")] = qsl("2.0");
    response[qsl("id")] = request[qsl("id")];

    if (result.success) {
        QJsonObject content;
        content[qsl("type")] = qsl("text");
        content[qsl("text")] = result.result.toString();

        QJsonObject resultObj;
        resultObj[qsl("content")] = QJsonArray{content};

        response[qsl("result")] = resultObj;
    } else {
        sendJsonRpcError(client, request[qsl("id")].toInt(), result.errorCode, result.errorMessage);
        return;
    }

    sendJsonRpcResponse(client, response);
}

void TMCPServer::handlePingRequest(MCPClient* client, const QJsonObject& request)
{
    QJsonObject response;
    response[qsl("jsonrpc")] = qsl("2.0");
    response[qsl("id")] = request[qsl("id")];
    response[qsl("result")] = QJsonObject();

    sendJsonRpcResponse(client, response);
}

QJsonObject TMCPServer::createServerInfo() const
{
    QJsonObject serverInfo;
    serverInfo[qsl("name")] = QString::fromLatin1(MCP_SERVER_NAME);
    serverInfo[qsl("version")] = QString::fromLatin1(MCP_SERVER_VERSION);

    QJsonObject capabilities;
    QJsonObject tools;
    tools[qsl("listChanged")] = false;
    capabilities[qsl("tools")] = tools;

    QJsonObject result;
    result[qsl("protocolVersion")] = QString::fromLatin1(MCP_PROTOCOL_VERSION);
    result[qsl("serverInfo")] = serverInfo;
    result[qsl("capabilities")] = capabilities;

    return result;
}

QJsonArray TMCPServer::getAvailableTools() const
{
    return mpLuaBridge->getAvailableTools();
}