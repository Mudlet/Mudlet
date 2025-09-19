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

    // Connect Lua bridge signals to notification slots
    connect(mpLuaBridge, &TMCPLuaBridge::toolsChanged, this, &TMCPServer::notifyToolsChanged);
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
    qDebug() << "TMCPServer: Processing request with sessionId:" << sessionId;

    // Parse the method early to determine if this is initialization
    QByteArray requestBody = request.body();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(requestBody, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        QJsonObject error = createJsonRpcError(-1, ParseError, tr("JSON parse error: %1").arg(parseError.errorString()));
        return QHttpServerResponse(error, QHttpServerResponse::StatusCode::BadRequest);
    }
    QJsonObject jsonRequest = doc.object();
    QString method = jsonRequest[qsl("method")].toString();

    MCPSession* session = nullptr;

    if (method == qsl("initialize")) {
        // For initialization, create a new session if no session ID provided
        if (sessionId.isEmpty()) {
            sessionId = generateSessionId();
        }
        session = getOrCreateSession(sessionId);
    } else {
        // For other requests, require session ID (except ping)
        if (method != qsl("ping") && sessionId.isEmpty()) {
            QJsonObject error = createJsonRpcError(jsonRequest[qsl("id")].toInt(-1), ServerError, tr("Session ID required. Must initialize first."));
            return QHttpServerResponse(error, QHttpServerResponse::StatusCode::BadRequest);
        }
        session = getOrCreateSession(sessionId);

        // Verify session is initialized for non-ping requests
        if (method != qsl("ping") && !session->initialized) {
            QJsonObject error = createJsonRpcError(jsonRequest[qsl("id")].toInt(-1), ServerError, tr("Client not initialized"));
            return QHttpServerResponse(error, QHttpServerResponse::StatusCode::BadRequest);
        }
    }


    QJsonObject jsonResponse;

    // Handle the request
    if (method == qsl("initialize")) {
        // Validate JSON-RPC format for initialization
        if (!jsonRequest.contains(qsl("jsonrpc")) || jsonRequest[qsl("jsonrpc")].toString() != qsl("2.0")) {
            jsonResponse = createJsonRpcError(-1, InvalidRequest, tr("Invalid JSON-RPC 2.0 request: missing or invalid 'jsonrpc' field"));
        } else if (!jsonRequest.contains(qsl("id"))) {
            jsonResponse = createJsonRpcError(-1, InvalidRequest, tr("Invalid JSON-RPC 2.0 request: missing 'id' field"));
        } else if (!jsonRequest.contains(qsl("params"))) {
            jsonResponse = createJsonRpcError(jsonRequest[qsl("id")].toInt(), InvalidParams, tr("Initialize request missing 'params'"));
        } else {
            handleInitializeRequest(jsonRequest, jsonResponse, session);
        }
    } else if (method == qsl("tools/list")) {
        qDebug() << "TMCPServer: tools/list request for sessionId:" << sessionId << "initialized:" << session->initialized;
        handleListToolsRequest(jsonRequest, jsonResponse);
    } else if (method == qsl("tools/call")) {
        handleCallToolRequest(jsonRequest, jsonResponse);
    } else if (method == qsl("ping")) {
        handlePingRequest(jsonRequest, jsonResponse);
    } else if (method.startsWith(qsl("notifications/"))) {
        handleNotificationRequest(jsonRequest);
        // Notifications don't require a response, return empty response
        return QHttpServerResponse(QHttpServerResponse::StatusCode::Ok);
    } else {
        int id = jsonRequest[qsl("id")].toInt(-1);
        jsonResponse = createJsonRpcError(id, MethodNotFound, tr("Method not found: %1").arg(method));
    }

    // Send response
    QHttpServerResponse response(jsonResponse, QHttpServerResponse::StatusCode::Ok);

    // Add session ID header for initialization requests
    if (method == qsl("initialize")) {
        QHttpHeaders headers = response.headers();
        headers.append("Mcp-Session-Id", session->sessionId.toUtf8());
        response.setHeaders(headers);
    }

    return response;
}

QHttpServerResponse TMCPServer::handleMcpGet(const QHttpServerRequest& request)
{
    // For GET requests, we could implement SSE streaming here if needed
    // For now, return HTTP 405 Method Not Allowed as we don't support SSE yet
    Q_UNUSED(request)

    return QHttpServerResponse(QHttpServerResponse::StatusCode::MethodNotAllowed);
}

void TMCPServer::handleInitializeRequest(const QJsonObject& request, QJsonObject& response, MCPSession* session)
{
    QJsonObject params = request[qsl("params")].toObject();

    // Validate required parameters
    if (!params.contains(qsl("protocolVersion"))) {
        response = createJsonRpcError(request[qsl("id")].toInt(), InvalidParams, tr("Missing required parameter: protocolVersion"));
        return;
    }

    if (!params.contains(qsl("clientInfo"))) {
        response = createJsonRpcError(request[qsl("id")].toInt(), InvalidParams, tr("Missing required parameter: clientInfo"));
        return;
    }

    QJsonObject clientInfo = params[qsl("clientInfo")].toObject();
    if (!clientInfo.contains(qsl("name"))) {
        response = createJsonRpcError(request[qsl("id")].toInt(), InvalidParams, tr("Missing required clientInfo.name"));
        return;
    }

    // Protocol version negotiation
    QString clientVersion = params[qsl("protocolVersion")].toString();
    QString serverVersion = QString::fromLatin1(MCP_PROTOCOL_VERSION);
    QString negotiatedVersion;

    if (clientVersion == serverVersion) {
        // Perfect match
        negotiatedVersion = serverVersion;
    } else {
        // For now, we only support our current version
        // In a full implementation, you'd check for compatible versions
        QJsonObject errorData;
        errorData[qsl("supported")] = QJsonArray{serverVersion};
        errorData[qsl("requested")] = clientVersion;
        response = createJsonRpcError(request[qsl("id")].toInt(), InvalidParams,
                                    tr("Unsupported protocol version"), QJsonValue(errorData));
        return;
    }

    session->clientName = clientInfo[qsl("name")].toString();
    session->clientVersion = clientInfo[qsl("version")].toString();
    session->initialized = true;

    QJsonObject initializeResult = createInitializeResult(negotiatedVersion);
    response[qsl("jsonrpc")] = qsl("2.0");
    response[qsl("id")] = request[qsl("id")];
    response[qsl("result")] = initializeResult;

    qDebug() << "TMCPServer: Initialized client:" << session->clientName << session->clientVersion
             << "sessionId:" << session->sessionId << "protocol:" << negotiatedVersion;
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

QJsonObject TMCPServer::createInitializeResult(const QString& negotiatedVersion) const
{
    QJsonObject serverInfo;
    serverInfo[qsl("name")] = QString::fromLatin1(MCP_SERVER_NAME);
    serverInfo[qsl("version")] = QString::fromLatin1(MCP_SERVER_VERSION);

    QJsonObject capabilities;

    // Tools capability
    QJsonObject tools;
    tools[qsl("listChanged")] = true;
    capabilities[qsl("tools")] = tools;

    QJsonObject result;
    result[qsl("protocolVersion")] = negotiatedVersion;
    result[qsl("serverInfo")] = serverInfo;
    result[qsl("capabilities")] = capabilities;

    return result;
}

QJsonArray TMCPServer::getAvailableTools() const
{
    return mpLuaBridge->getAvailableTools();
}

void TMCPServer::handleNotificationRequest(const QJsonObject& request)
{
    QString method = request[qsl("method")].toString();

    if (method == qsl("notifications/initialized")) {
        qDebug() << "TMCPServer: Client sent initialized notification";
        // Client is confirming it's ready - no action needed
    } else {
        qDebug() << "TMCPServer: Received unknown notification:" << method;
    }
}

void TMCPServer::sendNotificationToAllSessions(const QString& method, const QJsonObject& params)
{
    if (!mServerRunning || mSessions.isEmpty()) {
        return;
    }

    QJsonObject notification;
    notification[qsl("jsonrpc")] = qsl("2.0");
    notification[qsl("method")] = method;
    if (!params.isEmpty()) {
        notification[qsl("params")] = params;
    }

    qDebug() << "TMCPServer: Sending notification" << method << "to" << mSessions.size() << "sessions";

    // Note: For HTTP transport, we would need to implement Server-Sent Events (SSE)
    // to push notifications to clients. For now, we'll store pending notifications
    // and send them when clients poll or make requests.
    // This is a limitation of HTTP transport vs WebSocket or stdio transport.
}

void TMCPServer::notifyToolsChanged()
{
    sendNotificationToAllSessions(qsl("notifications/tools/list_changed"));
}