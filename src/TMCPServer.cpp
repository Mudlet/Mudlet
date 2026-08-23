/***************************************************************************
 *   Copyright (C) 2025-2026 by Vadim Peretokin - vadim.peretokin@mudlet.org *
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
#include "utils.h"

#include <QDebug>
#include <QHostAddress>
#include <QHttpHeaders>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTcpServer>
#include <QUrl>

// Neither the tool list nor the server's identity changes while Mudlet runs, so clients
// are told they may cache both for an hour rather than re-ask on every turn.
static constexpr int csmCacheTtlMs = 3600000;

TMCPServer::TMCPServer(Host* pHost, QObject* parent)
: QObject(parent)
, mpLuaBridge(new TMCPLuaBridge(pHost, this))
{
}

TMCPServer::~TMCPServer()
{
    stopServer();
}

bool TMCPServer::startServer(int port)
{
    if (running()) {
        return false;
    }

    auto* httpServer = new QHttpServer(this);

    // Everything rides one POST endpoint in this revision. /mcp is the conventional
    // path, and / is accepted as well so a bare host:port still reaches the server.
    const auto onPost = [this](const QHttpServerRequest& request) {
        return respondToPost(request);
    };
    httpServer->route(qsl("/"), QHttpServerRequest::Method::Post, onPost);
    httpServer->route(qsl("/mcp"), QHttpServerRequest::Method::Post, onPost);

    // GET opened the standalone SSE stream and DELETE ended a session in revisions up to
    // 2025-11-25. Neither exists now, and the spec asks a modern-only server to say so
    // with a 405 rather than let an old client hang waiting for a stream.
    const auto onRetiredMethod = [](const QHttpServerRequest&) {
        return QHttpServerResponse(QHttpServerResponse::StatusCode::MethodNotAllowed);
    };
    const QHttpServerRequest::Methods retiredMethods = QHttpServerRequest::Method::Get | QHttpServerRequest::Method::Delete;
    httpServer->route(qsl("/"), retiredMethods, onRetiredMethod);
    httpServer->route(qsl("/mcp"), retiredMethods, onRetiredMethod);

    auto* tcpServer = new QTcpServer(httpServer);
    if (!tcpServer->listen(QHostAddress::LocalHost, port)) {
        qWarning() << "TMCPServer: could not listen on port" << port << "-" << tcpServer->errorString();
        delete httpServer;
        return false;
    }

    if (!httpServer->bind(tcpServer)) {
        qWarning() << "TMCPServer: could not bind the HTTP server to port" << tcpServer->serverPort();
        delete httpServer;
        return false;
    }

    mPort = tcpServer->serverPort();
    mpHttpServer = httpServer;
    return true;
}

void TMCPServer::stopServer()
{
    if (!running()) {
        return;
    }

    // This takes the QTcpServer with it - bind() reparented the socket to the HTTP server.
    delete mpHttpServer;
    mpHttpServer = nullptr;
    mPort = 0;
}

QString TMCPServer::getEndpoint() const
{
    if (!running()) {
        return QString();
    }
    return qsl("http://127.0.0.1:%1/mcp").arg(mPort);
}

QHttpServerResponse TMCPServer::respondToPost(const QHttpServerRequest& request)
{
    const MCPReply reply = handleMessage(request.body(), request.headers());
    if (reply.body.isEmpty()) {
        return QHttpServerResponse(reply.status);
    }
    return QHttpServerResponse(reply.body, reply.status);
}

MCPReply TMCPServer::handleMessage(const QByteArray& requestBody, const QHttpHeaders& headers)
{
    // Guards against DNS rebinding. A browser always attaches Origin, a genuine MCP
    // client never does, so anything with a non-local Origin is a page in a tab.
    const QByteArrayView origin = headers.value("Origin");
    if (!origin.isEmpty() && !originAllowed(QString::fromUtf8(origin))) {
        qWarning() << "TMCPServer: refused a request from origin" << origin;
        return {QJsonObject(), QHttpServerResponse::StatusCode::Forbidden};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(requestBody, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return error(QJsonValue(), ParseError, tr("JSON parse error: %1").arg(parseError.errorString()), QJsonValue(), QHttpServerResponse::StatusCode::BadRequest);
    }

    const QJsonObject message = document.object();
    const QJsonValue id = message.value(qsl("id"));

    if (message.value(qsl("jsonrpc")).toString() != qsl("2.0")) {
        return error(id, InvalidRequest, tr("Not a JSON-RPC 2.0 message"), QJsonValue(), QHttpServerResponse::StatusCode::BadRequest);
    }

    // A message without an id is a notification and is never answered. The core protocol
    // defines no client-to-server notification over HTTP, so there is nothing to act on.
    if (id.isUndefined() || id.isNull()) {
        return {QJsonObject(), QHttpServerResponse::StatusCode::Accepted};
    }

    const QString method = message.value(qsl("method")).toString();
    const QJsonObject params = message.value(qsl("params")).toObject();
    const QJsonObject meta = params.value(qsl("_meta")).toObject();

    // A pre-2026-07-28 client opens with initialize and has no way to fall forward, so
    // this error is the only diagnostic it can put in front of a user - name the version.
    if (method == qsl("initialize")) {
        QJsonObject data;
        data[qsl("supported")] = QJsonArray{QString::fromLatin1(MCP_PROTOCOL_VERSION)};
        data[qsl("requested")] = params.value(qsl("protocolVersion"));
        return error(id,
                     UnsupportedProtocolVersion,
                     tr("This server speaks MCP %1 only, which has no initialize handshake.").arg(QString::fromLatin1(MCP_PROTOCOL_VERSION)),
                     data,
                     QHttpServerResponse::StatusCode::BadRequest);
    }

    if (!meta.contains(QString::fromLatin1(META_CLIENT_CAPABILITIES))) {
        return error(id, InvalidParams, tr("Missing required _meta field: %1").arg(QString::fromLatin1(META_CLIENT_CAPABILITIES)), QJsonValue(), QHttpServerResponse::StatusCode::BadRequest);
    }

    const QString mismatch = headerMismatch(headers, method, params, meta);
    if (!mismatch.isEmpty()) {
        return error(id, HeaderMismatch, mismatch, QJsonValue(), QHttpServerResponse::StatusCode::BadRequest);
    }

    const QString requestedVersion = meta.value(QString::fromLatin1(META_PROTOCOL_VERSION)).toString();
    if (requestedVersion != QLatin1String(MCP_PROTOCOL_VERSION)) {
        QJsonObject data;
        data[qsl("supported")] = QJsonArray{QString::fromLatin1(MCP_PROTOCOL_VERSION)};
        data[qsl("requested")] = requestedVersion;
        return error(id, UnsupportedProtocolVersion, tr("Unsupported protocol version"), data, QHttpServerResponse::StatusCode::BadRequest);
    }

    return dispatch(method, id, params);
}

MCPReply TMCPServer::dispatch(const QString& method, const QJsonValue& id, const QJsonObject& params)
{
    if (method == qsl("server/discover")) {
        return discover(id);
    }
    if (method == qsl("tools/list")) {
        return listTools(id);
    }
    if (method == qsl("tools/call")) {
        return callTool(id, params);
    }

    // The JSON-RPC body on this 404 is what tells a client it has reached a modern MCP
    // endpoint that lacks the method, rather than a server hosting no endpoint at all.
    return error(id, MethodNotFound, tr("Method not found: %1").arg(method), QJsonValue(), QHttpServerResponse::StatusCode::NotFound);
}

MCPReply TMCPServer::discover(const QJsonValue& id) const
{
    QJsonObject capabilities;
    // Deliberately no listChanged: the tool list is fixed, and announcing a change would
    // need a subscriptions/listen stream that there is nothing to send down.
    capabilities[qsl("tools")] = QJsonObject();

    QJsonObject payload;
    payload[qsl("supportedVersions")] = QJsonArray{QString::fromLatin1(MCP_PROTOCOL_VERSION)};
    payload[qsl("capabilities")] = capabilities;
    //: Shown to an AI model to explain what this MCP server is for. "Lua" and "MUD" are proper nouns.
    payload[qsl("instructions")] = tr("Runs Lua inside Mudlet, a MUD client. Use it to inspect and drive a player's "
                                      "session: read the map, windows, triggers, aliases and variables, and send "
                                      "commands to the game.");
    payload[qsl("ttlMs")] = csmCacheTtlMs;
    payload[qsl("cacheScope")] = qsl("private");
    return result(id, payload);
}

MCPReply TMCPServer::listTools(const QJsonValue& id) const
{
    QJsonObject payload;
    payload[qsl("tools")] = mpLuaBridge->getAvailableTools();
    payload[qsl("ttlMs")] = csmCacheTtlMs;
    payload[qsl("cacheScope")] = qsl("private");
    return result(id, payload);
}

MCPReply TMCPServer::callTool(const QJsonValue& id, const QJsonObject& params)
{
    const QString toolName = params.value(qsl("name")).toString();
    if (!mpLuaBridge->hasTool(toolName)) {
        return error(id, InvalidParams, tr("Unknown tool: %1").arg(toolName));
    }

    const MCPToolResult toolResult = mpLuaBridge->callTool(toolName, params.value(qsl("arguments")).toObject());

    QJsonObject textContent;
    textContent[qsl("type")] = qsl("text");
    textContent[qsl("text")] = toolResult.text;

    QJsonObject payload;
    payload[qsl("content")] = QJsonArray{textContent};
    // A failure here is the tool's, not the protocol's, so the model sees it and can retry.
    payload[qsl("isError")] = !toolResult.success;
    return result(id, payload);
}

QString TMCPServer::headerMismatch(const QHttpHeaders& headers, const QString& method, const QJsonObject& params, const QJsonObject& meta) const
{
    // These headers mirror body fields so that proxies can route without parsing the
    // body. If the two disagree, the two halves of the network are looking at different
    // requests, which the spec treats as a security problem rather than a mistake.
    const QByteArrayView versionHeader = headers.value("MCP-Protocol-Version");
    if (versionHeader.isEmpty()) {
        return tr("Missing required header: MCP-Protocol-Version");
    }
    const QString metaVersion = meta.value(QString::fromLatin1(META_PROTOCOL_VERSION)).toString();
    if (QString::fromUtf8(versionHeader) != metaVersion) {
        return tr("Header mismatch: MCP-Protocol-Version header value '%1' does not match body value '%2'").arg(QString::fromUtf8(versionHeader), metaVersion);
    }

    const QByteArrayView methodHeader = headers.value("Mcp-Method");
    if (methodHeader.isEmpty()) {
        return tr("Missing required header: Mcp-Method");
    }
    if (QString::fromUtf8(methodHeader) != method) {
        return tr("Header mismatch: Mcp-Method header value '%1' does not match body value '%2'").arg(QString::fromUtf8(methodHeader), method);
    }

    if (method == qsl("tools/call")) {
        const QByteArrayView nameHeader = headers.value("Mcp-Name");
        if (nameHeader.isEmpty()) {
            return tr("Missing required header: Mcp-Name");
        }
        const QString name = decodeHeaderValue(nameHeader);
        if (name != params.value(qsl("name")).toString()) {
            return tr("Header mismatch: Mcp-Name header value '%1' does not match body value '%2'").arg(name, params.value(qsl("name")).toString());
        }
    }

    return QString();
}

MCPReply TMCPServer::result(const QJsonValue& id, QJsonObject payload, QHttpServerResponse::StatusCode status) const
{
    payload[qsl("resultType")] = qsl("complete");

    QJsonObject meta = payload.value(qsl("_meta")).toObject();
    meta[QString::fromLatin1(META_SERVER_INFO)] = serverImplementation();
    payload[qsl("_meta")] = meta;

    QJsonObject body;
    body[qsl("jsonrpc")] = qsl("2.0");
    body[qsl("id")] = id;
    body[qsl("result")] = payload;
    return {body, status};
}

MCPReply TMCPServer::error(const QJsonValue& id, JsonRpcErrorCode code, const QString& message, const QJsonValue& data, QHttpServerResponse::StatusCode status) const
{
    QJsonObject errorObject;
    errorObject[qsl("code")] = code;
    errorObject[qsl("message")] = message;
    if (!data.isNull() && !data.isUndefined()) {
        errorObject[qsl("data")] = data;
    }

    QJsonObject body;
    body[qsl("jsonrpc")] = qsl("2.0");
    // A message too malformed to read an id from still gets a reply, with a null id.
    body[qsl("id")] = id.isUndefined() ? QJsonValue() : id;
    body[qsl("error")] = errorObject;
    return {body, status};
}

QJsonObject TMCPServer::serverImplementation() const
{
    QJsonObject implementation;
    implementation[qsl("name")] = QString::fromLatin1(MCP_SERVER_NAME);
    implementation[qsl("version")] = QString::fromLatin1(APP_VERSION);
    return implementation;
}

QString TMCPServer::decodeHeaderValue(QByteArrayView raw)
{
    // A value that would not survive an HTTP header - non-ASCII, control characters,
    // padding whitespace - arrives base64-wrapped in this sentinel instead.
    constexpr QByteArrayView prefix = "=?base64?";
    constexpr QByteArrayView suffix = "?=";
    if (raw.size() >= prefix.size() + suffix.size() && raw.startsWith(prefix) && raw.endsWith(suffix)) {
        const QByteArray payload = raw.sliced(prefix.size(), raw.size() - prefix.size() - suffix.size()).toByteArray();
        return QString::fromUtf8(QByteArray::fromBase64(payload));
    }
    return QString::fromUtf8(raw);
}

bool TMCPServer::originAllowed(const QString& origin)
{
    // Compare the parsed host, not the prefix: "http://localhost.example.com" starts with
    // "http://localhost" but is somebody else's domain.
    const QString host = QUrl(origin).host();
    return host == qsl("localhost") || host == qsl("127.0.0.1") || host == qsl("::1");
}
