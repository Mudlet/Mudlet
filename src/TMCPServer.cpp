/***************************************************************************
 *   Copyright (C) 2025-2026 Vadim Peretokin - vadim.peretokin@mudlet.org  *
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

#include <QRandomGenerator>

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

// The tool list and the server's identity are fixed for the run, so clients are told they
// may cache both for an hour rather than re-ask every turn. The one thing that can change
// underneath that is the interface language, since the tool descriptions are translated.
static constexpr int csmCacheTtlMs = 60 * 60 * 1000;

// QRandomGenerator::system() rather than ::global(): the global one is seeded for speed
// rather than to be unguessable, and this value is what keeps other local processes out.
static QString generateAuthToken()
{
    quint32 words[4] = {};
    QRandomGenerator::system()->fillRange(words);
    return QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(words), static_cast<qsizetype>(sizeof(words))).toHex());
}

TMCPServer::TMCPServer(QObject* parent)
: QObject(parent)
, mpLuaBridge(new TMCPLuaBridge(this))
, mAuthToken(generateAuthToken())
{
}

TMCPServer::~TMCPServer()
{
    stopServer();
}

MCPStartResult TMCPServer::startServer(quint16 port)
{
    if (running()) {
        //: Reported when something asks the MCP server to start while it is already up.
        return {false, tr("The server is already running.")};
    }

    auto* httpServer = new QHttpServer(this);

    // Everything rides one POST endpoint in this revision. /mcp is the conventional
    // path, and / is accepted as well so a bare host:port still reaches the server.
    const auto onPost = [this](const QHttpServerRequest& request) {
        return respondToPost(request);
    };
    // Qt answers a Rule* here from 6.9 and a bool before that; both test the same way, and
    // an unregistered route would otherwise turn every POST into a silent 404.
    const auto rootRoute = httpServer->route(qsl("/"), QHttpServerRequest::Method::Post, onPost);
    const auto mcpRoute = httpServer->route(qsl("/mcp"), QHttpServerRequest::Method::Post, onPost);
    // The token is accepted as a trailing path segment as well as in an Authorization
    // header, because a good many MCP clients take only a URL and offer nowhere to put a
    // header - and the preferences dialog hands out one string to paste.
    const auto tokenRoute = httpServer->route(qsl("/mcp/<arg>"), QHttpServerRequest::Method::Post, [this](const QString& pathToken, const QHttpServerRequest& request) {
        return respondToPost(request, pathToken);
    });
    if (!rootRoute || !mcpRoute || !tokenRoute) {
        qWarning() << "TMCPServer: could not register the POST routes";
        delete httpServer;
        //: Reported when the MCP server could not set up the addresses it answers on.
        return {false, tr("The server could not register the addresses it answers on.")};
    }

    // GET opened the standalone SSE stream and DELETE ended a session in revisions up to
    // 2025-11-25. Neither exists now, and the spec asks a modern-only server to say so
    // with a 405 rather than let an old client hang waiting for a stream.
    const auto onRetiredMethod = [](const QHttpServerRequest&) {
        return QHttpServerResponse(QHttpServerResponse::StatusCode::MethodNotAllowed);
    };
    const QHttpServerRequest::Methods retiredMethods = QHttpServerRequest::Method::Get | QHttpServerRequest::Method::Delete;
    const auto rootRetired = httpServer->route(qsl("/"), retiredMethods, onRetiredMethod);
    const auto mcpRetired = httpServer->route(qsl("/mcp"), retiredMethods, onRetiredMethod);
    // The endpoint Mudlet hands out carries the token in the path, so that is the address
    // an old client would try these on; without this it gets a bare 404 instead.
    const auto tokenRetired = httpServer->route(qsl("/mcp/<arg>"), retiredMethods, [onRetiredMethod](const QString&, const QHttpServerRequest& request) {
        return onRetiredMethod(request);
    });
    // Checked for the same reason as the POST routes above: a registration that quietly
    // failed would answer 404 instead of 405, which is exactly the bare 404 that leaves an
    // old client hanging rather than telling it the stream is gone.
    if (!rootRetired || !mcpRetired || !tokenRetired) {
        qWarning() << "TMCPServer: could not register the routes that turn away retired methods";
        delete httpServer;
        //: Reported when the MCP server could not set up the addresses it answers on.
        return {false, tr("The server could not register the addresses it answers on.")};
    }

    auto* tcpServer = new QTcpServer(httpServer);
    if (!tcpServer->listen(QHostAddress::LocalHost, port)) {
        const QString reason = tcpServer->errorString();
        qWarning() << "TMCPServer: could not listen on port" << port << "-" << reason;
        delete httpServer;
        return {false, reason};
    }

    if (!httpServer->bind(tcpServer)) {
        qWarning() << "TMCPServer: could not bind the HTTP server to port" << tcpServer->serverPort();
        delete httpServer;
        //: Reported when the MCP server got a socket but could not serve HTTP on it.
        return {false, tr("The HTTP server could not take over the listening socket.")};
    }

    mPort = tcpServer->serverPort();
    mpHttpServer = httpServer;
    return {true, QString()};
}

void TMCPServer::stopServer()
{
    if (!running()) {
        return;
    }

    // This takes the listening socket with it: the QTcpServer is created as a child of the
    // HTTP server, so it is freed on the bind() failure path above too.
    delete mpHttpServer;
    mpHttpServer = nullptr;
    mPort = 0;
}

QString TMCPServer::getEndpoint() const
{
    if (!running()) {
        return QString();
    }
    return endpointFor(mPort, mAuthToken);
}

QString TMCPServer::endpointFor(quint16 port, const QString& token)
{
    if (token.isEmpty()) {
        return qsl("http://127.0.0.1:%1/mcp").arg(port);
    }
    return qsl("http://127.0.0.1:%1/mcp/%2").arg(QString::number(port), token);
}

QHttpServerResponse TMCPServer::respondToPost(const QHttpServerRequest& request, const QString& pathToken)
{
    const MCPReply reply = handleMessage(request.body(), request.headers(), pathToken);
    if (reply.body.isEmpty()) {
        return QHttpServerResponse(reply.status);
    }
    return QHttpServerResponse(reply.body, reply.status);
}

MCPReply TMCPServer::handleMessage(const QByteArray& requestBody, const QHttpHeaders& headers, const QString& pathToken)
{
    // Guards against DNS rebinding: a page in a browser tab always attaches Origin, so a
    // non-local one means somebody else's site is driving the request rather than a local
    // MCP client. Clients that do send a local Origin are still let through.
    const QByteArrayView origin = headers.value("Origin");
    if (!origin.isEmpty() && !originAllowed(QString::fromUtf8(origin))) {
        qWarning() << "TMCPServer: refused a request from origin" << origin;
        //: Sent back to a rejected MCP client. %1 is the web origin the request carried.
        return error(QJsonValue(), InvalidRequest, tr("Refused: origin '%1' is not local.").arg(QString::fromUtf8(origin)), QJsonValue(), QHttpServerResponse::StatusCode::Forbidden);
    }

    if (!authorised(headers, pathToken)) {
        qWarning() << "TMCPServer: refused an unauthenticated request";
        //: Sent back to an MCP client that did not present the server's access token.
        return error(QJsonValue(),
                     Unauthorized,
                     tr("Refused: this request did not carry Mudlet's MCP access token. Copy the address from Mudlet's preferences, which has the token in it."),
                     QJsonValue(),
                     QHttpServerResponse::StatusCode::Unauthorized);
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(requestBody, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        //: Sent to an MCP client whose request was not valid JSON. %1 is the parser's reason.
        return error(QJsonValue(), ParseError, tr("JSON parse error: %1").arg(parseError.errorString()), QJsonValue(), QHttpServerResponse::StatusCode::BadRequest);
    }

    const QJsonObject message = document.object();
    const QJsonValue id = message.value(qsl("id"));

    if (message.value(qsl("jsonrpc")).toString() != qsl("2.0")) {
        //: Sent to an MCP client whose request was not a JSON-RPC 2.0 message.
        return error(id, InvalidRequest, tr("Not a JSON-RPC 2.0 message"), QJsonValue(), QHttpServerResponse::StatusCode::BadRequest);
    }

    // A message without an id is a notification and is never answered. The core protocol
    // defines no client-to-server notification over HTTP, so there is nothing to act on.
    // An explicit null id is taken the same way, which is more lenient than JSON-RPC 2.0
    // strictly allows, because a client that sends one has no reply to correlate anyway.
    if (id.isUndefined() || id.isNull()) {
        return {QJsonObject(), QHttpServerResponse::StatusCode::Accepted};
    }

    const QString method = message.value(qsl("method")).toString();
    const QJsonObject params = message.value(qsl("params")).toObject();
    const QJsonObject meta = params.value(qsl("_meta")).toObject();

    // A pre-2026-07-28 client opens with initialize and has no way to fall forward, so
    // this error is the only diagnostic it can put in front of a user - name the version.
    if (method == qsl("initialize")) {
        //: Sent to an MCP client too old to talk to this server. %1 is a version like 2026-07-28.
        return unsupportedVersion(id, params.value(qsl("protocolVersion")), tr("This server speaks MCP %1 only, which has no initialize handshake.").arg(QString::fromLatin1(MCP_PROTOCOL_VERSION)));
    }

    if (!meta.contains(QString::fromLatin1(META_CLIENT_CAPABILITIES))) {
        //: Sent to an MCP client that left out a required _meta field. %1 is the field name.
        return error(id, InvalidParams, tr("Missing required _meta field: %1").arg(QString::fromLatin1(META_CLIENT_CAPABILITIES)), QJsonValue(), QHttpServerResponse::StatusCode::BadRequest);
    }

    const QString mismatch = headerMismatch(headers, method, params, meta);
    if (!mismatch.isEmpty()) {
        return error(id, HeaderMismatch, mismatch, QJsonValue(), QHttpServerResponse::StatusCode::BadRequest);
    }

    const QString requestedVersion = meta.value(QString::fromLatin1(META_PROTOCOL_VERSION)).toString();
    if (requestedVersion != QLatin1String(MCP_PROTOCOL_VERSION)) {
        //: Sent to an MCP client asking for a protocol revision this server does not speak.
        return unsupportedVersion(id, requestedVersion, tr("Unsupported protocol version"));
    }

    return dispatch(method, id, params);
}

MCPReply TMCPServer::unsupportedVersion(const QJsonValue& id, const QJsonValue& requested, const QString& message) const
{
    QJsonObject data;
    data[qsl("supported")] = QJsonArray{QString::fromLatin1(MCP_PROTOCOL_VERSION)};
    // Assigning an Undefined removes the key rather than writing null, so a client that
    // asked for no version at all would see "requested" missing and could not tell that
    // apart from a server that never reports it. Spell the absence out.
    data[qsl("requested")] = requested.isUndefined() ? QJsonValue(QJsonValue::Null) : requested;
    return error(id, UnsupportedProtocolVersion, message, data, QHttpServerResponse::StatusCode::BadRequest);
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
    //: Sent to an MCP client that called a method this server does not have. %1 is the method name.
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
        //: Sent to an MCP client that asked for a tool this server does not offer. %1 is the tool name it asked for.
        return error(id, InvalidParams, tr("Unknown tool: %1").arg(toolName));
    }

    // toObject() answers an empty object for a string, a number or an array, so arguments
    // sent in the wrong shape would reach the bridge as none at all and come back as
    // "the 'code' argument is required" - which sends the model off fixing the wrong thing.
    const QJsonValue argumentsValue = params.value(qsl("arguments"));
    if (!argumentsValue.isUndefined() && !argumentsValue.isNull() && !argumentsValue.isObject()) {
        //: Sent to an MCP client that put something other than an object in a tool call's "arguments". Keep 'arguments' as-is, it names a protocol field.
        return error(id, InvalidParams, tr("The 'arguments' field must be an object."));
    }

    const MCPToolResult toolResult = mpLuaBridge->callTool(toolName, argumentsValue.toObject());

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
        //: Sent to an MCP client that left out an HTTP header this protocol revision requires.
        return tr("Missing required header: MCP-Protocol-Version");
    }
    const QString metaVersion = meta.value(QString::fromLatin1(META_PROTOCOL_VERSION)).toString();
    if (QString::fromUtf8(versionHeader) != metaVersion) {
        //: Sent to an MCP client whose HTTP header disagrees with its request body. %1 is the header value, %2 the value in the body.
        return tr("Header mismatch: MCP-Protocol-Version header value '%1' does not match body value '%2'").arg(QString::fromUtf8(versionHeader), metaVersion);
    }

    const QByteArrayView methodHeader = headers.value("Mcp-Method");
    if (methodHeader.isEmpty()) {
        //: Sent to an MCP client that left out an HTTP header this protocol revision requires.
        return tr("Missing required header: Mcp-Method");
    }
    if (QString::fromUtf8(methodHeader) != method) {
        //: Sent to an MCP client whose HTTP header disagrees with its request body. %1 is the header value, %2 the value in the body.
        return tr("Header mismatch: Mcp-Method header value '%1' does not match body value '%2'").arg(QString::fromUtf8(methodHeader), method);
    }

    if (method == qsl("tools/call")) {
        const QByteArrayView nameHeader = headers.value("Mcp-Name");
        if (nameHeader.isEmpty()) {
            //: Sent to an MCP client that left out an HTTP header this protocol revision requires.
            return tr("Missing required header: Mcp-Name");
        }
        const QString name = decodeHeaderValue(nameHeader);
        if (name != params.value(qsl("name")).toString()) {
            //: Sent to an MCP client whose HTTP header disagrees with its request body. %1 is the header value, %2 the value in the body.
            return tr("Header mismatch: Mcp-Name header value '%1' does not match body value '%2'").arg(name, params.value(qsl("name")).toString());
        }
    }

    return QString();
}

MCPReply TMCPServer::result(const QJsonValue& id, QJsonObject payload, QHttpServerResponse::StatusCode status) const
{
    payload[qsl("resultType")] = qsl("complete");

    payload[qsl("_meta")] = QJsonObject{{QString::fromLatin1(META_SERVER_INFO), serverImplementation()}};

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
        // Strictly, so that a malformed wrapper is reported as bad encoding rather than
        // decoding to garbage and surfacing as a header mismatch against random bytes.
        const QByteArray::FromBase64Result decoded = QByteArray::fromBase64Encoding(payload, QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
        if (!decoded) {
            return QString();
        }
        return QString::fromUtf8(*decoded);
    }
    return QString::fromUtf8(raw);
}

bool TMCPServer::authorised(const QHttpHeaders& headers, const QString& pathToken) const
{
    QString presented = pathToken;
    if (presented.isEmpty()) {
        const QString authorization = QString::fromUtf8(headers.value(QHttpHeaders::WellKnownHeader::Authorization));
        constexpr QLatin1StringView bearer("Bearer ");
        if (authorization.startsWith(bearer, Qt::CaseInsensitive)) {
            presented = authorization.sliced(bearer.size()).trimmed();
        }
    }

    // Compare every character rather than stopping at the first difference, so that how
    // long the answer takes does not report how much of the token was guessed correctly.
    if (presented.size() != mAuthToken.size()) {
        return false;
    }
    int difference = 0;
    for (int i = 0; i < mAuthToken.size(); ++i) {
        difference |= presented.at(i).unicode() ^ mAuthToken.at(i).unicode();
    }
    return difference == 0;
}

bool TMCPServer::originAllowed(const QString& origin)
{
    // Compare the parsed host, not the prefix: "http://localhost.example.com" starts with
    // "http://localhost" but is somebody else's domain.
    const QString host = QUrl(origin).host();
    return host == qsl("localhost") || host == qsl("127.0.0.1") || host == qsl("::1");
}
