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
#include <QHttpServerResponse>
#include <QNetworkInterface>
#include <QUuid>

TMCPServer::TMCPServer(Host* pHost, QObject* parent)
: QObject(parent)
, mpHost(pHost)
, mpLuaBridge(nullptr)
, mpHttpServer(nullptr)
, mpTcpServer(nullptr)
, mPort(0)
, mServerRunning(false)
{
    mpLuaBridge = new TMCPLuaBridge(mpHost, this);
    mpHttpServer = new QHttpServer(this);
    mpTcpServer = new QTcpServer(this);
}

TMCPServer::~TMCPServer()
{
    stopServer();
    qDeleteAll(mSessions);
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

    // Set up HTTP routes for MCP
    mpHttpServer->route("/", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& request) {
            return handleMcpPost(request);
        });

    mpHttpServer->route("/", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest& request) {
            return handleMcpGet(request);
        });

    mpHttpServer->route("/", QHttpServerRequest::Method::Options,
        [this](const QHttpServerRequest& request) {
            Q_UNUSED(request)
            return QHttpServerResponse(QHttpServerResponse::StatusCode::Ok);
        });

    // Bind to localhost
    QHostAddress address = QHostAddress::LocalHost;
    if (!mpTcpServer->listen(address, port)) {
        qWarning() << "TMCPServer: Failed to start server on port" << port << ":" << mpTcpServer->errorString();
        return false;
    }

    if (!mpHttpServer->bind(mpTcpServer)) {
        qWarning() << "TMCPServer: Failed to bind HTTP server to TCP server";
        mpTcpServer->close();
        return false;
    }

    mPort = mpTcpServer->serverPort();
    mServerRunning = true;

    qDebug() << "TMCPServer: Started HTTP server on" << address.toString() << ":" << mPort;
    return true;
}

void TMCPServer::stopServer()
{
    if (!mServerRunning) {
        return;
    }

    mpHttpServer->disconnect();
    mpTcpServer->close();

    qDeleteAll(mSessions);
    mSessions.clear();

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
    return tr("MCP Server: Running on localhost:%1 (%2 sessions)").arg(mPort).arg(mSessions.size());
}

QHttpServerResponse TMCPServer::handleMcpPost(const QHttpServerRequest& request)
{
    // Validate Origin header for security
    if (request.headers().contains("Origin")) {
        QString origin = QString::fromUtf8(request.headers().value("Origin"));
        if (!origin.isEmpty() && !origin.startsWith("http://localhost") && !origin.startsWith("http://127.0.0.1")) {
            qWarning() << "TMCPServer: Rejecting request from origin:" << origin;
            return QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden);
        }
    }

    // Get or create session
    QString sessionId = QString::fromUtf8(request.headers().value("Mcp-Session-Id"));
    if (sessionId.isEmpty()) {
        sessionId = generateSessionId();
    }
    qDebug() << "TMCPServer: Processing request with sessionId:" << sessionId;
    MCPSession* session = getOrCreateSession(sessionId);
    qDebug() << "TMCPServer: Session initialized status:" << session->initialized;

    // Parse JSON-RPC request
    QByteArray requestBody = request.body();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(requestBody, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QJsonObject error = createJsonRpcError(-1, ParseError, tr("JSON parse error: %1").arg(parseError.errorString()));
        QJsonDocument errorDoc(error);

        QHttpServerResponse response(errorDoc.object(), QHttpServerResponse::StatusCode::BadRequest);
        return response;
    }

    QJsonObject jsonRequest = doc.object();
    QString method = jsonRequest[qsl("method")].toString();
    QJsonObject jsonResponse;

    // Handle the request
    if (method == qsl("initialize")) {
        handleInitializeRequest(jsonRequest, jsonResponse, session);
    } else if (method == qsl("tools/list")) {
        qDebug() << "TMCPServer: tools/list request for sessionId:" << sessionId << "initialized:" << session->initialized;
        if (!session->initialized) {
            jsonResponse = createJsonRpcError(jsonRequest[qsl("id")].toInt(), ServerError, tr("Client not initialized"));
        } else {
            handleListToolsRequest(jsonRequest, jsonResponse);
        }
    } else if (method == qsl("tools/call")) {
        if (!session->initialized) {
            jsonResponse = createJsonRpcError(jsonRequest[qsl("id")].toInt(), ServerError, tr("Client not initialized"));
        } else {
            handleCallToolRequest(jsonRequest, jsonResponse);
        }
    } else if (method == qsl("ping")) {
        handlePingRequest(jsonRequest, jsonResponse);
    } else {
        int id = jsonRequest[qsl("id")].toInt(-1);
        jsonResponse = createJsonRpcError(id, MethodNotFound, tr("Method not found: %1").arg(method));
    }

    // Send response
    QHttpServerResponse response(jsonResponse, QHttpServerResponse::StatusCode::Ok);

    // Add session ID to headers
    QHttpHeaders headers = response.headers();
    headers.append("Mcp-Session-Id", sessionId.toUtf8());
    response.setHeaders(headers);

    return response;
}

QHttpServerResponse TMCPServer::handleMcpGet(const QHttpServerRequest& request)
{
    // For GET requests, we could implement SSE streaming here if needed
    // For now, return basic server info
    Q_UNUSED(request)

    QJsonObject serverInfo = createServerInfo();
    QJsonDocument doc(serverInfo);

    QHttpServerResponse response(serverInfo, QHttpServerResponse::StatusCode::Ok);

    return response;
}

void TMCPServer::handleInitializeRequest(const QJsonObject& request, QJsonObject& response, MCPSession* session)
{
    QJsonObject params = request[qsl("params")].toObject();

    QJsonObject clientInfo = params[qsl("clientInfo")].toObject();
    session->clientName = clientInfo[qsl("name")].toString();
    session->clientVersion = clientInfo[qsl("version")].toString();
    session->initialized = true;

    QJsonObject serverInfo = createServerInfo();
    response[qsl("jsonrpc")] = qsl("2.0");
    response[qsl("id")] = request[qsl("id")];
    response[qsl("result")] = serverInfo;

    qDebug() << "TMCPServer: Initialized client:" << session->clientName << session->clientVersion << "sessionId:" << session->sessionId;
}

void TMCPServer::handleListToolsRequest(const QJsonObject& request, QJsonObject& response)
{
    QJsonObject result;
    result[qsl("tools")] = getAvailableTools();

    response[qsl("jsonrpc")] = qsl("2.0");
    response[qsl("id")] = request[qsl("id")];
    response[qsl("result")] = result;
}

void TMCPServer::handleCallToolRequest(const QJsonObject& request, QJsonObject& response)
{
    QJsonObject params = request[qsl("params")].toObject();
    QString toolName = params[qsl("name")].toString();
    QJsonObject arguments = params[qsl("arguments")].toObject();

    MCPToolResult result = mpLuaBridge->callTool(toolName, arguments);

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
        response = createJsonRpcError(request[qsl("id")].toInt(), result.errorCode, result.errorMessage);
    }
}

void TMCPServer::handlePingRequest(const QJsonObject& request, QJsonObject& response)
{
    response[qsl("jsonrpc")] = qsl("2.0");
    response[qsl("id")] = request[qsl("id")];
    response[qsl("result")] = QJsonObject();
}

QJsonObject TMCPServer::createJsonRpcError(int id, int code, const QString& message, const QJsonValue& data)
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

    return response;
}

QString TMCPServer::generateSessionId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

TMCPServer::MCPSession* TMCPServer::getOrCreateSession(const QString& sessionId)
{
    if (!mSessions.contains(sessionId)) {
        MCPSession* session = new MCPSession();
        session->sessionId = sessionId;
        session->initialized = false;
        session->requestId = 1;
        mSessions[sessionId] = session;
    }
    return mSessions[sessionId];
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