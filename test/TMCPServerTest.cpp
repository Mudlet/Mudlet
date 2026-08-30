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

#include <TMCPLuaBridge.h>
#include <TMCPServer.h>
#include <utils.h>

#include <QtTest/QtTest>

#include <QHostAddress>
#include <QElapsedTimer>
#include <QHttpHeaders>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkInterface>
#include <QNetworkRequest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

using StatusCode = QHttpServerResponse::StatusCode;

namespace {
const QString currentVersion = QString::fromLatin1(TMCPServer::MCP_PROTOCOL_VERSION);

// Set from the server under test in init(): every request has to carry its token, so the
// helper below adds it rather than each case remembering to.
QString authToken;

QHttpHeaders headersFor(const QString& method, const QString& name = QString(), const QString& version = currentVersion)
{
    QHttpHeaders headers;
    if (!version.isNull()) {
        headers.append("MCP-Protocol-Version", version);
    }
    if (!method.isNull()) {
        headers.append("Mcp-Method", method);
    }
    if (!name.isNull()) {
        headers.append("Mcp-Name", name);
    }
    if (!authToken.isEmpty()) {
        headers.append("Authorization", qsl("Bearer %1").arg(authToken).toUtf8());
    }
    return headers;
}

QByteArray bodyFor(const QString& method, const QJsonObject& params = QJsonObject(), const QJsonValue& id = QJsonValue(1), const QString& version = currentVersion)
{
    QJsonObject meta;
    meta[QString::fromLatin1(TMCPServer::META_PROTOCOL_VERSION)] = version;
    meta[QString::fromLatin1(TMCPServer::META_CLIENT_CAPABILITIES)] = QJsonObject();

    QJsonObject withMeta = params;
    withMeta[qsl("_meta")] = meta;

    QJsonObject message;
    message[qsl("jsonrpc")] = qsl("2.0");
    if (!id.isUndefined()) {
        message[qsl("id")] = id;
    }
    message[qsl("method")] = method;
    message[qsl("params")] = withMeta;
    return QJsonDocument(message).toJson(QJsonDocument::Compact);
}

QJsonObject resultOf(const MCPReply& reply)
{
    return reply.body.value(qsl("result")).toObject();
}

QJsonObject errorOf(const MCPReply& reply)
{
    return reply.body.value(qsl("error")).toObject();
}
} // namespace

class TMCPServerTest : public QObject
{
    Q_OBJECT

private:
    TMCPServer* server = nullptr;
    lua_State* L = nullptr;

    QStringList globalNames() const
    {
        QStringList names;
        lua_pushnil(L);
        while (lua_next(L, LUA_GLOBALSINDEX) != 0) {
            // Only read string keys: lua_tostring() on a number key rewrites it in place
            // and lua_next() then loses its place in the table.
            if (lua_type(L, -2) == LUA_TSTRING) {
                names << QString::fromUtf8(lua_tostring(L, -2));
            }
            lua_pop(L, 1);
        }
        names.sort();
        return names;
    }

    struct WalkTiming
    {
        bool ok = false;
        qint64 addedMs = 0;
    };

    // How long converting a table cost, over and above building the same table and not
    // returning it. Timing the snippet as a whole would mostly measure how fast this
    // machine runs a Lua loop of that many iterations, which says nothing about the walk.
    WalkTiming timeWalk(const QString& build) const
    {
        QElapsedTimer elapsed;
        elapsed.start();
        const bool builtOk = TMCPLuaBridge::runLua(L, build + qsl("return #t"), 60000).success;
        const qint64 withoutWalk = elapsed.restart();
        const bool walkedOk = TMCPLuaBridge::runLua(L, build + qsl("return t"), 60000).success;
        return {builtOk && walkedOk, elapsed.elapsed() - withoutWalk};
    }

    const void* globalPrint() const
    {
        lua_getglobal(L, "print");
        const void* address = lua_topointer(L, -1);
        lua_pop(L, 1);
        return address;
    }

private slots: // NOLINT(readability-redundant-access-specifiers)
    void init()
    {
        // Every case here is either protocol handling or a Lua snippet run against the
        // bare interpreter below, neither of which needs a profile.
        server = new TMCPServer(nullptr);
        authToken = server->authToken();
        L = luaL_newstate();
        luaL_openlibs(L);
    }

    void cleanup()
    {
        delete server;
        server = nullptr;
        authToken.clear();
        lua_close(L);
        L = nullptr;
    }

    // ---------- server/discover ----------

    void testDiscoverAdvertisesTheCurrentRevisionOnly()
    {
        const MCPReply reply = server->handleMessage(bodyFor(qsl("server/discover")), headersFor(qsl("server/discover")));

        QCOMPARE(reply.status, StatusCode::Ok);
        const QJsonObject result = resultOf(reply);
        QCOMPARE(result.value(qsl("supportedVersions")).toArray(), QJsonArray{currentVersion});
        QCOMPARE(result.value(qsl("resultType")).toString(), qsl("complete"));
        QVERIFY(result.value(qsl("capabilities")).toObject().contains(qsl("tools")));
        QVERIFY(!result.value(qsl("instructions")).toString().isEmpty());
    }

    void testEveryResultNamesTheServerInItsMeta()
    {
        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headersFor(qsl("tools/list")));

        const QJsonObject serverInfo = resultOf(reply).value(qsl("_meta")).toObject().value(QString::fromLatin1(TMCPServer::META_SERVER_INFO)).toObject();
        QCOMPARE(serverInfo.value(qsl("name")).toString(), QString::fromLatin1(TMCPServer::MCP_SERVER_NAME));
        QVERIFY(!serverInfo.value(qsl("version")).toString().isEmpty());
    }

    // ---------- tools ----------

    void testToolsListIsCacheableAndOffersTheLuaTool()
    {
        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headersFor(qsl("tools/list")));

        QCOMPARE(reply.status, StatusCode::Ok);
        const QJsonObject result = resultOf(reply);
        QCOMPARE(result.value(qsl("resultType")).toString(), qsl("complete"));

        // 2026-07-28 requires both on every list result.
        QVERIFY(result.value(qsl("ttlMs")).toInt() > 0);
        QCOMPARE(result.value(qsl("cacheScope")).toString(), qsl("private"));

        const QJsonArray tools = result.value(qsl("tools")).toArray();
        QCOMPARE(tools.size(), 1);
        const QJsonObject tool = tools.first().toObject();
        QCOMPARE(tool.value(qsl("name")).toString(), QString::fromLatin1(TMCPLuaBridge::MCP_LUA_TOOL));
        QCOMPARE(tool.value(qsl("inputSchema")).toObject().value(qsl("required")).toArray(), QJsonArray{qsl("code")});
    }

    void testUnknownToolIsAProtocolErrorNotAToolError()
    {
        QJsonObject params;
        params[qsl("name")] = qsl("nosuchtool");
        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/call"), params), headersFor(qsl("tools/call"), qsl("nosuchtool")));

        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::InvalidParams));
        QVERIFY(reply.body.value(qsl("result")).isUndefined());
    }

    void testAFailingToolCallIsReportedInTheResult()
    {
        QJsonObject arguments;
        arguments[qsl("code")] = QString();
        QJsonObject params;
        params[qsl("name")] = QString::fromLatin1(TMCPLuaBridge::MCP_LUA_TOOL);
        params[qsl("arguments")] = arguments;

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/call"), params), headersFor(qsl("tools/call"), QString::fromLatin1(TMCPLuaBridge::MCP_LUA_TOOL)));

        // A tool that fails still answers 200 with isError, so the model can correct itself.
        QCOMPARE(reply.status, StatusCode::Ok);
        const QJsonObject result = resultOf(reply);
        QCOMPARE(result.value(qsl("isError")).toBool(), true);
        QCOMPARE(result.value(qsl("resultType")).toString(), qsl("complete"));
        const QJsonObject content = result.value(qsl("content")).toArray().first().toObject();
        QCOMPARE(content.value(qsl("type")).toString(), qsl("text"));
        // The reason has to survive into the content block, otherwise the model is told
        // something went wrong and given nothing to correct.
        QVERIFY2(content.value(qsl("text")).toString().contains(qsl("code")), qPrintable(content.value(qsl("text")).toString()));
    }

    void testAnArgumentOfTheWrongTypeIsNamedAsSuch()
    {
        // QJsonValue::toString() answers "" for a number, so without a type check a numeric
        // profile reads as one that was never sent and the code runs in the wrong profile.
        QJsonObject arguments;
        arguments[qsl("code")] = qsl("return 1");
        arguments[qsl("profile")] = 3;
        QJsonObject params;
        params[qsl("name")] = QString::fromLatin1(TMCPLuaBridge::MCP_LUA_TOOL);
        params[qsl("arguments")] = arguments;

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/call"), params), headersFor(qsl("tools/call"), QString::fromLatin1(TMCPLuaBridge::MCP_LUA_TOOL)));

        const QJsonObject result = resultOf(reply);
        QCOMPARE(result.value(qsl("isError")).toBool(), true);
        // Not just "mentions the profile": without the type check this call still fails,
        // but with "no profile is open" - the same answer an honest empty argument gets.
        const QString text = result.value(qsl("content")).toArray().first().toObject().value(qsl("text")).toString();
        QVERIFY2(text.contains(qsl("'profile' argument must be a string")), qPrintable(text));
    }

    void testACodeArgumentOfTheWrongTypeIsNotReportedAsMissing()
    {
        QJsonObject arguments;
        arguments[qsl("code")] = 42;
        QJsonObject params;
        params[qsl("name")] = QString::fromLatin1(TMCPLuaBridge::MCP_LUA_TOOL);
        params[qsl("arguments")] = arguments;

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/call"), params), headersFor(qsl("tools/call"), QString::fromLatin1(TMCPLuaBridge::MCP_LUA_TOOL)));

        const QJsonObject result = resultOf(reply);
        QCOMPARE(result.value(qsl("isError")).toBool(), true);
        const QString text = result.value(qsl("content")).toArray().first().toObject().value(qsl("text")).toString();
        QVERIFY2(text.contains(qsl("must be a string")), qPrintable(text));
    }

    // ---------- transport-level validation ----------

    void testMissingProtocolVersionHeaderIsRejected()
    {
        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headersFor(qsl("tools/list"), QString(), QString()));

        QCOMPARE(reply.status, StatusCode::BadRequest);
        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::HeaderMismatch));
    }

    void testHeaderThatDisagreesWithTheBodyIsRejected()
    {
        // A proxy routing on the header and the server acting on the body would other-
        // wise be working from two different requests.
        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headersFor(qsl("tools/call")));

        QCOMPARE(reply.status, StatusCode::BadRequest);
        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::HeaderMismatch));
    }

    void testToolsCallWithoutTheNameHeaderIsRejected()
    {
        QJsonObject params;
        params[qsl("name")] = QString::fromLatin1(TMCPLuaBridge::MCP_LUA_TOOL);
        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/call"), params), headersFor(qsl("tools/call")));

        QCOMPARE(reply.status, StatusCode::BadRequest);
        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::HeaderMismatch));
    }

    void testAMalformedEncodedHeaderIsRejectedRatherThanDecodedToMojibake()
    {
        // "bHVh!" is "lua" with one character a base64 alphabet does not contain. A lenient
        // decode skips it, so a header that is not valid base64 at all is accepted as if the
        // client had spelled the tool name correctly.
        QJsonObject params;
        params[qsl("name")] = QString::fromLatin1(TMCPLuaBridge::MCP_LUA_TOOL);

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/call"), params), headersFor(qsl("tools/call"), qsl("=?base64?bHVh!?=")));

        QCOMPARE(reply.status, StatusCode::BadRequest);
        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::HeaderMismatch));
    }

    void testAnUnsupportedVersionSaysWhichOneWasAskedFor()
    {
        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list"), QJsonObject(), QJsonValue(1), qsl("2025-06-18")), headersFor(qsl("tools/list"), QString(), qsl("2025-06-18")));

        const QJsonObject data = errorOf(reply).value(qsl("data")).toObject();
        QCOMPARE(data.value(qsl("requested")).toString(), qsl("2025-06-18"));
        QCOMPARE(data.value(qsl("supported")).toArray(), QJsonArray{currentVersion});
    }

    void testAnUnsupportedVersionReportsNullWhenNoneWasNamed()
    {
        // An old client that opened with initialize and named no version must not read
        // alike to one that named "", or the error cannot tell it which mistake it made.
        QJsonObject message;
        message[qsl("jsonrpc")] = qsl("2.0");
        message[qsl("id")] = 1;
        message[qsl("method")] = qsl("initialize");
        message[qsl("params")] = QJsonObject();

        const MCPReply reply = server->handleMessage(QJsonDocument(message).toJson(QJsonDocument::Compact), headersFor(qsl("initialize")));

        const QJsonObject data = errorOf(reply).value(qsl("data")).toObject();
        QVERIFY2(data.contains(qsl("requested")), "the absence was left out rather than spelled out");
        QVERIFY(data.value(qsl("requested")).isNull());
    }

    void testToolCallArgumentsThatAreNotAnObjectAreAProtocolError()
    {
        // A string here reached the tool and came back as "the code argument is required",
        // which sends the model off correcting code it did supply.
        QJsonObject params;
        params[qsl("name")] = QString::fromLatin1(TMCPLuaBridge::MCP_LUA_TOOL);
        params[qsl("arguments")] = qsl("print('hello')");

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/call"), params), headersFor(qsl("tools/call"), QString::fromLatin1(TMCPLuaBridge::MCP_LUA_TOOL)));

        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::InvalidParams));
        QVERIFY(reply.body.value(qsl("result")).isUndefined());
    }

    void testBase64WrappedNameHeaderIsDecodedBeforeComparing()
    {
        const QString awkwardName = qsl("weißwurst");
        QJsonObject params;
        params[qsl("name")] = awkwardName;
        const QString encoded = qsl("=?base64?%1?=").arg(QString::fromLatin1(awkwardName.toUtf8().toBase64()));

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/call"), params), headersFor(qsl("tools/call"), encoded));

        // Past header validation, so it fails as an unknown tool rather than a mismatch.
        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::InvalidParams));
    }

    void testUnsupportedProtocolVersionListsWhatIsSupported()
    {
        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list"), QJsonObject(), QJsonValue(1), qsl("2025-06-18")), headersFor(qsl("tools/list"), QString(), qsl("2025-06-18")));

        QCOMPARE(reply.status, StatusCode::BadRequest);
        const QJsonObject error = errorOf(reply);
        QCOMPARE(error.value(qsl("code")).toInt(), static_cast<int>(TMCPServer::UnsupportedProtocolVersion));
        QCOMPARE(error.value(qsl("data")).toObject().value(qsl("supported")).toArray(), QJsonArray{currentVersion});
        QCOMPARE(error.value(qsl("data")).toObject().value(qsl("requested")).toString(), qsl("2025-06-18"));
    }

    void testInitializeIsAnsweredWithTheVersionsWeSpeak()
    {
        // A pre-2026 client has no way to fall forward, so this error is the only thing
        // it can put in front of a user - it has to name a version.
        QJsonObject params;
        params[qsl("protocolVersion")] = qsl("2025-06-18");
        QJsonObject message;
        message[qsl("jsonrpc")] = qsl("2.0");
        message[qsl("id")] = 1;
        message[qsl("method")] = qsl("initialize");
        message[qsl("params")] = params;

        const MCPReply reply = server->handleMessage(QJsonDocument(message).toJson(QJsonDocument::Compact), headersFor(qsl("initialize")));

        QCOMPARE(reply.status, StatusCode::BadRequest);
        const QJsonObject error = errorOf(reply);
        QCOMPARE(error.value(qsl("code")).toInt(), static_cast<int>(TMCPServer::UnsupportedProtocolVersion));
        QCOMPARE(error.value(qsl("data")).toObject().value(qsl("supported")).toArray(), QJsonArray{currentVersion});
    }

    void testMissingClientCapabilitiesIsRejected()
    {
        QJsonObject meta;
        meta[QString::fromLatin1(TMCPServer::META_PROTOCOL_VERSION)] = currentVersion;
        QJsonObject params;
        params[qsl("_meta")] = meta;
        QJsonObject message;
        message[qsl("jsonrpc")] = qsl("2.0");
        message[qsl("id")] = 1;
        message[qsl("method")] = qsl("tools/list");
        message[qsl("params")] = params;

        const MCPReply reply = server->handleMessage(QJsonDocument(message).toJson(QJsonDocument::Compact), headersFor(qsl("tools/list")));

        QCOMPARE(reply.status, StatusCode::BadRequest);
        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::InvalidParams));
    }

    void testUnknownMethodAnswers404WithAJsonRpcBody()
    {
        // The body is what tells a client this is a modern MCP endpoint missing the
        // method, rather than a server that hosts no endpoint at all.
        const MCPReply reply = server->handleMessage(bodyFor(qsl("resources/list")), headersFor(qsl("resources/list")));

        QCOMPARE(reply.status, StatusCode::NotFound);
        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::MethodNotFound));
    }

    void testMalformedJsonIsAnsweredWithAParseError()
    {
        const MCPReply reply = server->handleMessage(QByteArrayLiteral("{ not json"), headersFor(qsl("tools/list")));

        QCOMPARE(reply.status, StatusCode::BadRequest);
        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::ParseError));
        QVERIFY(reply.body.value(qsl("id")).isNull());
    }

    void testNonJsonRpc2MessageIsRejected()
    {
        QJsonObject message;
        message[qsl("jsonrpc")] = qsl("1.0");
        message[qsl("id")] = 1;
        message[qsl("method")] = qsl("tools/list");

        const MCPReply reply = server->handleMessage(QJsonDocument(message).toJson(QJsonDocument::Compact), headersFor(qsl("tools/list")));

        QCOMPARE(reply.status, StatusCode::BadRequest);
        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::InvalidRequest));
    }

    void testNotificationIsAcknowledgedWithoutABody()
    {
        const MCPReply reply = server->handleMessage(bodyFor(qsl("notifications/whatever"), QJsonObject(), QJsonValue()), headersFor(qsl("notifications/whatever")));

        QCOMPARE(reply.status, StatusCode::Accepted);
        QVERIFY(reply.body.isEmpty());
    }

    void testStringRequestIdSurvivesTheRoundTrip()
    {
        // A string id has to come back as the same string: anything that coerces it to a
        // number collapses every id to 0, and a client can no longer match reply to call.
        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list"), QJsonObject(), QJsonValue(qsl("discover-1"))), headersFor(qsl("tools/list")));

        QCOMPARE(reply.body.value(qsl("id")).toString(), qsl("discover-1"));
    }

    void testNonLocalOriginIsRefused()
    {
        QHttpHeaders headers = headersFor(qsl("tools/list"));
        headers.append("Origin", "https://evil.example.com");

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headers);

        QCOMPARE(reply.status, StatusCode::Forbidden);
    }

    void testLookalikeLocalhostOriginIsRefused()
    {
        // "http://localhost.example.com" passes a startsWith("http://localhost") test.
        QHttpHeaders headers = headersFor(qsl("tools/list"));
        headers.append("Origin", "http://localhost.example.com");

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headers);

        QCOMPARE(reply.status, StatusCode::Forbidden);
    }

    void testLocalhostOriginIsAllowed()
    {
        QHttpHeaders headers = headersFor(qsl("tools/list"));
        headers.append("Origin", "http://localhost:11235");

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headers);

        QCOMPARE(reply.status, StatusCode::Ok);
    }

    void testALeftOverSessionHeaderIsIgnoredRatherThanRejected()
    {
        // Sessions left the protocol in 2026-07-28. A client still sending the header has
        // to be served normally rather than rejected, so that it keeps working; there is
        // no session to resume and nothing in the reply should suggest otherwise.
        QHttpHeaders headers = headersFor(qsl("tools/list"));
        headers.append("Mcp-Session-Id", "left-over-from-2025");

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headers);

        QCOMPARE(reply.status, StatusCode::Ok);
        QVERIFY(!resultOf(reply).value(qsl("tools")).toArray().isEmpty());
    }

    // ---------- the access token ----------

    void testARequestWithoutTheTokenIsRefused()
    {
        // Loopback is not access control: anything already running on this computer can
        // reach the port, and the tool runs arbitrary Lua - os.execute() included.
        QHttpHeaders headers = headersFor(qsl("tools/list"));
        headers.removeAll("Authorization");

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headers);

        QCOMPARE(reply.status, StatusCode::Unauthorized);
        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::Unauthorized));
        QVERIFY(reply.body.value(qsl("result")).isUndefined());
    }

    void testARequestWithTheWrongTokenIsRefused()
    {
        QHttpHeaders headers = headersFor(qsl("tools/list"));
        headers.removeAll("Authorization");
        headers.append("Authorization", "Bearer not-the-token");

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headers);

        QCOMPARE(reply.status, StatusCode::Unauthorized);
    }

    void testTheTokenIsCheckedBeforeTheBodyIsParsed()
    {
        // An unauthorised caller must not be able to tell malformed JSON from valid JSON,
        // nor reach the parser at all.
        QHttpHeaders headers = headersFor(qsl("tools/list"));
        headers.removeAll("Authorization");

        const MCPReply reply = server->handleMessage(QByteArrayLiteral("{ not json"), headers);

        QCOMPARE(reply.status, StatusCode::Unauthorized);
        QCOMPARE(errorOf(reply).value(qsl("code")).toInt(), static_cast<int>(TMCPServer::Unauthorized));
    }

    void testTheTokenIsAlsoAcceptedFromTheUrlPath()
    {
        // The endpoint Mudlet shows carries the token in the path, because that is the one
        // place every MCP client can be pointed at without configuring a header.
        QHttpHeaders headers = headersFor(qsl("tools/list"));
        headers.removeAll("Authorization");

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headers, server->authToken());

        QCOMPARE(reply.status, StatusCode::Ok);
    }

    void testAWrongTokenInTheUrlPathIsRefused()
    {
        // The path is the route a client is pointed at, so it is also the one an intruder
        // would guess at; it has to be checked, not merely read.
        QHttpHeaders headers = headersFor(qsl("tools/list"));
        headers.removeAll("Authorization");

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headers, qsl("not-the-token"));

        QCOMPARE(reply.status, StatusCode::Unauthorized);
        QVERIFY(reply.body.value(qsl("result")).isUndefined());
    }

    void testTheShownEndpointCarriesTheToken()
    {
        QVERIFY(server->startServer(0).started);

        QVERIFY(server->getEndpoint().contains(server->authToken()));
        QVERIFY(!server->authToken().isEmpty());

        server->stopServer();
    }

    void testEachServerGetsItsOwnToken()
    {
        const QScopedPointer<TMCPServer> other(new TMCPServer(nullptr));

        QVERIFY(other->authToken() != server->authToken());
    }

    // ---------- listening ----------

    void testStartingAndStoppingCanBeRepeated()
    {
        // A restart has to release the old port and come back actually serving. Routes
        // cannot be removed from a QHttpServer, so a restart that reuses one stacks a
        // second copy of every route - which only a real request over the socket shows.
        QVERIFY(!server->running());
        QVERIFY(server->startServer(0).started);
        QVERIFY(server->running());
        const quint16 firstPort = server->getPort();
        QVERIFY(firstPort > 0);
        QVERIFY(server->getEndpoint().contains(QString::number(firstPort)));

        server->stopServer();
        QVERIFY(!server->running());
        QCOMPARE(server->getPort(), quint16(0));

        // Re-take the same port: if stopServer() leaked the socket this cannot bind.
        QVERIFY(server->startServer(firstPort).started);
        QVERIFY(server->running());
        QCOMPARE(server->getPort(), firstPort);

        QNetworkAccessManager manager;
        QNetworkRequest request{QUrl(server->getEndpoint())};
        request.setHeader(QNetworkRequest::ContentTypeHeader, qsl("application/json"));
        request.setRawHeader("MCP-Protocol-Version", currentVersion.toUtf8());
        request.setRawHeader("Mcp-Method", "server/discover");
        const QScopedPointer<QNetworkReply> reply(manager.post(request, bodyFor(qsl("server/discover"))));
        QTRY_VERIFY_WITH_TIMEOUT(reply->isFinished(), 5000);
        QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);

        server->stopServer();
    }

    void testStartingTwiceIsRefused()
    {
        QVERIFY(server->startServer(0).started);
        QVERIFY(!server->startServer(0).started);
        server->stopServer();
    }

    void testATakenPortIsReportedWithAReason()
    {
        // There is one server for the whole application, so the port is only ever taken by
        // something else on this computer - a second Mudlet, or an unrelated program. The reason has to survive back to the caller,
        // because it is the only thing that can be put in front of the user.
        QTcpServer squatter;
        QVERIFY(squatter.listen(QHostAddress::LocalHost, 0));
        const quint16 taken = squatter.serverPort();

        const MCPStartResult result = server->startServer(taken);

        QVERIFY(!result.started);
        QVERIFY2(!result.error.isEmpty(), "a failed start must say why");
        QVERIFY(!server->running());
        QCOMPARE(server->getPort(), quint16(0));
    }

    void testARealPostOverTheSocketIsAnswered()
    {
        // The cases above drive handleMessage() directly; this one goes over the socket,
        // so it covers the route, the binding and the QHttpServerResponse conversion.
        QVERIFY(server->startServer(0).started);

        QNetworkAccessManager manager;
        QNetworkRequest request{QUrl(server->getEndpoint())};
        request.setHeader(QNetworkRequest::ContentTypeHeader, qsl("application/json"));
        request.setRawHeader("Accept", "application/json, text/event-stream");
        request.setRawHeader("MCP-Protocol-Version", currentVersion.toUtf8());
        request.setRawHeader("Mcp-Method", "server/discover");

        const QScopedPointer<QNetworkReply> reply(manager.post(request, bodyFor(qsl("server/discover"))));
        QTRY_VERIFY_WITH_TIMEOUT(reply->isFinished(), 5000);

        QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
        const QJsonObject result = QJsonDocument::fromJson(reply->readAll()).object().value(qsl("result")).toObject();
        QCOMPARE(result.value(qsl("supportedVersions")).toArray(), QJsonArray{currentVersion});

        server->stopServer();
    }

    void testAGetIsTurnedAwayRatherThanOpeningAStream()
    {
        // GET was the standalone SSE endpoint until 2025-11-25. An old client has to be
        // told no, or it sits waiting for a stream that will never carry anything.
        QVERIFY(server->startServer(0).started);

        QNetworkAccessManager manager;
        const QScopedPointer<QNetworkReply> reply(manager.get(QNetworkRequest{QUrl(server->getEndpoint())}));
        QTRY_VERIFY_WITH_TIMEOUT(reply->isFinished(), 5000);

        QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 405);

        server->stopServer();
    }

    void testARetiredMethodIsTurnedAwayRatherThanNotFound()
    {
        // DELETE ended a session up to revision 2025-11-25. A 404 would read to an old
        // client as the wrong address rather than as a method that no longer exists, so
        // the route has to be registered - and its registration has to be checked.
        QVERIFY(server->startServer(0).started);

        QNetworkAccessManager manager;
        const QScopedPointer<QNetworkReply> reply(manager.deleteResource(QNetworkRequest{QUrl(server->getEndpoint())}));
        QTRY_VERIFY_WITH_TIMEOUT(reply->isFinished(), 5000);

        QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 405);

        server->stopServer();
    }

    void testTheServerIsNotReachableFromOffThisComputer()
    {
        // The tool runs arbitrary Lua, so binding to anything but loopback would hand the
        // whole local network a shell on this machine. Checked against a real address of
        // this host rather than by reading the bind call.
        QHostAddress routable;
        for (const QHostAddress& address : QNetworkInterface::allAddresses()) {
            if (!address.isLoopback() && !address.isLinkLocal() && address.protocol() == QAbstractSocket::IPv4Protocol) {
                routable = address;
                break;
            }
        }
        if (routable.isNull()) {
            QSKIP("this machine has no non-loopback IPv4 address to try");
        }

        QVERIFY(server->startServer(0).started);

        QTcpSocket socket;
        socket.connectToHost(routable, server->getPort());
        QVERIFY2(!socket.waitForConnected(2000), qPrintable(qsl("the server accepted a connection to %1").arg(routable.toString())));

        server->stopServer();
    }

    // ---------- the Lua runner ----------

    void testPrintedOutputComesBack()
    {
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("print('hello') print('world')"));

        QVERIFY(result.success);
        QCOMPARE(result.text, qsl("hello\nworld"));
    }

    void testReturnedValueComesBack()
    {
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("return 6 * 7"));

        QVERIFY(result.success);
        QCOMPARE(result.text, qsl("42"));
    }

    void testAReasonReturnedAlongsideNilIsNotLost()
    {
        // "return nil, reason" is the standard Mudlet API failure idiom. Collecting the
        // results into a table and unpacking it drops everything from the nil onwards,
        // because # stops at the hole - so the caller is told there was no output at all.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("return nil, 'no such room'"));

        QVERIFY(result.success);
        QVERIFY2(result.text.contains(qsl("no such room")), qPrintable(result.text));
    }

    void testPrintedOutputIsNotTruncatedAtAnEmbeddedNul()
    {
        // The runner joins every print() into one Lua string, so reading it as a C string
        // does not just clip one line - it discards every line after the NUL.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("print('a' .. string.char(0) .. 'b') print('after')"));

        QVERIFY(result.success);
        QVERIFY2(result.text.contains(qsl("after")), qPrintable(result.text));
    }

    void testATableWithAHoleKeepsItsNamedKeys()
    {
        // #t is any border of a table with a hole, so this table has three keys and a
        // length of three - the array shortcut fires and the named key is lost.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("return {'a', nil, 'c', x = 'd'}"));

        QVERIFY(result.success);
        QVERIFY2(result.text.contains(qsl("\"x\":\"d\"")), qPrintable(result.text));
        QVERIFY2(result.text.contains(qsl("\"a\"")), qPrintable(result.text));
        QVERIFY2(result.text.contains(qsl("\"c\"")), qPrintable(result.text));
    }

    void testATableKeyIsNotTruncatedAtAnEmbeddedNul()
    {
        // Reading a key as a C string stops at the NUL, so both keys become "a" and one of
        // the two values is silently dropped.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("return {[string.char(97,0,120)] = 1, [string.char(97,0,121)] = 2}"));

        QVERIFY(result.success);
        const QJsonObject table = QJsonDocument::fromJson(result.text.toUtf8()).object();
        QCOMPARE(table.size(), 2);
    }

    void testValuesAfterANilStillArrive()
    {
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("return 1, nil, 3"));

        QVERIFY(result.success);
        QVERIFY2(result.text.contains(QChar('3')), qPrintable(result.text));
    }

    void testReturningNilIsNotTheSameAsReturningNothing()
    {
        const MCPToolResult explicitNil = TMCPLuaBridge::runLua(L, qsl("return nil"));
        const MCPToolResult nothing = TMCPLuaBridge::runLua(L, qsl("local x = 1"));

        QVERIFY(explicitNil.success);
        QVERIFY(nothing.success);
        QVERIFY2(explicitNil.text != nothing.text, qPrintable(explicitNil.text));
    }

    void testALargeNumberKeepsAllItsDigits()
    {
        // QString::number(double) renders six significant digits, which turns a room id
        // or a timestamp into 1.23457e+06 - and the same value nested in a table goes
        // through QJsonDocument and stays exact, so the two disagree.
        const MCPToolResult bare = TMCPLuaBridge::runLua(L, qsl("return 1787506774"));
        const MCPToolResult nested = TMCPLuaBridge::runLua(L, qsl("return {1787506774}"));

        QCOMPARE(bare.text, qsl("1787506774"));
        QVERIFY2(nested.text.contains(qsl("1787506774")), qPrintable(nested.text));
    }

    void testALargeNumericTableKeyKeepsAllItsDigits()
    {
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("local t = {} t[1234567] = 'roomid' return t"));

        QVERIFY2(result.text.contains(qsl("1234567")), qPrintable(result.text));
    }

    void testANumericAndAStringKeyDoNotOverwriteEachOther()
    {
        // t[1] and t["1"] are different keys in Lua but the same key in JSON, so one
        // entry would silently vanish from the object the model is reasoning about.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("local t = {} t[1] = 'fromNumber' t['1'] = 'fromString' return t"));

        QVERIFY2(result.text.contains(qsl("fromNumber")), qPrintable(result.text));
        QVERIFY2(result.text.contains(qsl("fromString")), qPrintable(result.text));
    }

    void testCodeIsNotSilentlyTruncatedAtAnEmbeddedNul()
    {
        // Handing Lua a bare char* stops at the first NUL, so what actually runs is the
        // leading fragment - which can be a shorter but still valid program that does
        // something other than what was sent. Rejecting it is fine; running "return 1"
        // and reporting success is not.
        const QString code = qsl("return 1") + QChar(u'\0') + qsl("+ 41");
        const MCPToolResult result = TMCPLuaBridge::runLua(L, code);

        QVERIFY2(!(result.success && result.text == qsl("1")), qPrintable(result.text));
    }

    void testPrintIsPutBackAfterTheCallAndLeavesNoGlobals()
    {
        // Read _G by enumeration from outside the runner rather than checking for the
        // particular names the current implementation uses, so this still bites if the
        // runner is rewritten to leak a differently-named global.
        const QStringList before = globalNames();
        const void* printBefore = globalPrint();

        QVERIFY(TMCPLuaBridge::runLua(L, qsl("print('once')")).success);

        QCOMPARE(globalNames(), before);
        QCOMPARE(globalPrint(), printBefore);
    }

    void testASequenceIsRenderedAsAJsonArray()
    {
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("return {10, 20, 30}"));

        QVERIFY(result.success);
        QCOMPARE(result.text, qsl("[10,20,30]"));
    }

    void testAKeyedTableIsRenderedAsAJsonObject()
    {
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("return {name = 'Gandalf'}"));

        QVERIFY(result.success);
        QCOMPARE(result.text, qsl(R"({"name":"Gandalf"})"));
    }

    void testASelfReferencingTableDoesNotRecurseForever()
    {
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("local t = {} t.self = t return t"));

        QVERIFY(result.success);
        QVERIFY(result.text.contains(qsl("nested too deeply")));
    }

    void testARuntimeErrorIsReportedAsAToolFailure()
    {
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("error('deliberate')"));

        QVERIFY(!result.success);
        QVERIFY(result.text.contains(qsl("deliberate")));
    }

    void testAnErrorRaisedWithATableStillCarriesItsMessage()
    {
        // lua_tostring() answers null for a table, so reading the error that way loses
        // the message entirely.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("error({reason = 'structured'})"));

        QVERIFY(!result.success);
        QVERIFY(result.text.contains(qsl("structured")));
    }

    void testASyntaxErrorIsReportedAsAToolFailure()
    {
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("this is not lua"));

        QVERIFY(!result.success);
        QVERIFY(!result.text.isEmpty());
    }

    void testOutputPrintedBeforeAnErrorIsKept()
    {
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("print('got here') error('then failed')"));

        QVERIFY(!result.success);
        QVERIFY(result.text.contains(qsl("got here")));
        QVERIFY(result.text.contains(qsl("then failed")));
    }

    void testCodeWithNoOutputSaysSo()
    {
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("local x = 1"));

        QVERIFY(result.success);
        QVERIFY(!result.text.isEmpty());
    }

    void testASnippetThatNeverReturnsIsStopped()
    {
        // Mudlet is single threaded, so without a deadline this wedges the whole
        // application - every profile - with no way out but killing it.
        QElapsedTimer elapsed;
        elapsed.start();
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("while true do end"), 200);

        QVERIFY(!result.success);
        QVERIFY2(result.text.contains(qsl("stopped after running")), qPrintable(result.text));
        QVERIFY2(elapsed.elapsed() < 5000, "the deadline did not stop the snippet");
    }

    void testAnExplodingTableIsCutShortRatherThanExhaustingMemory()
    {
        // Four self-references are only one level deep per step, so the depth cap never
        // trips; unbounded this took 15 seconds, and five references reached 20GB
        // resident and had to be killed.
        QElapsedTimer elapsed;
        elapsed.start();
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("local t = {} for i = 1, 4 do t[i] = t end return t"));

        QVERIFY(result.success);
        QVERIFY2(elapsed.elapsed() < 5000, "the node budget did not cut the walk short");
    }

    void testAVeryLongStringIsCutShortRatherThanSentWhole()
    {
        // Counting nodes does not bound a reply on its own: one string is a single node
        // however long it is, and this one is 3MB.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("return string.rep('x', 3000000)"));

        QVERIFY(result.success);
        QVERIFY2(result.text.size() < 200000, qPrintable(qsl("the reply was %1 characters").arg(result.text.size())));
        // Saying so matters as much as doing it: a model handed a silently shortened
        // string would read it as the whole value and reason from a wrong answer.
        QVERIFY2(result.text.contains(qsl("cut short")), qPrintable(result.text.right(200)));
    }

    void testATableOfLargeStringsIsBoundedByHowMuchTextItCarries()
    {
        // Each string here is under the per-string cap, so only charging string length
        // against the node budget keeps 500 of them from adding up to the same blow-up.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("local t = {} for i = 1, 500 do t[i] = string.rep('x', 90000) end return t"));

        QVERIFY(result.success);
        QVERIFY2(result.text.size() < 5000000, qPrintable(qsl("the reply was %1 characters").arg(result.text.size())));
    }

    void testAWideKeyedTableIsCutShortQuickly()
    {
        // QJsonObject keeps its keys sorted, so every insert moves everything after it:
        // uncapped, this table took 4675ms to render, all of it on Mudlet's only thread.
        // Both of these are past the key cap, so the walk breaks off after the same 20000
        // keys either way and the wider one costs no more. That comparison is what makes
        // the case, not a millisecond budget: a sanitised CI runner spends longer on a
        // capped walk than this machine does on an uncapped one, so no one number is both
        // loose enough to pass there and tight enough to fail here.
        const WalkTiming capped = timeWalk(qsl("local t = {} for i = 1, 40000 do t['key'..i] = i end "));
        const WalkTiming tenTimesWider = timeWalk(qsl("local t = {} for i = 1, 400000 do t['key'..i] = i end "));

        QVERIFY(capped.ok);
        QVERIFY(tenTimesWider.ok);
        QVERIFY2(tenTimesWider.addedMs < 3 * capped.addedMs + 100, qPrintable(qsl("ten times the keys took %1ms to walk against %2ms").arg(tenTimesWider.addedMs).arg(capped.addedMs)));
    }

    void testALongArrayIsCutShortQuicklyAndIsStillAnArray()
    {
        // Both are past the node cap, so the walk stops after the same 200000 entries and
        // the longer one costs no more - see the wide-table case above for why the two are
        // weighed against each other rather than against a millisecond budget.
        const WalkTiming capped = timeWalk(qsl("local t = {} for i = 1, 400000 do t[i] = i end "));
        const WalkTiming fiveTimesLonger = timeWalk(qsl("local t = {} for i = 1, 2000000 do t[i] = i end "));

        QVERIFY(capped.ok);
        QVERIFY(fiveTimesLonger.ok);
        QVERIFY2(fiveTimesLonger.addedMs < 3 * capped.addedMs + 100, qPrintable(qsl("five times the entries took %1ms to walk against %2ms").arg(fiveTimesLonger.addedMs).arg(capped.addedMs)));

        // A table too big to walk through in full is still a list. Giving up on that and
        // rendering it as {"1":1, "10":10, "100":100} both reads wrong to a model and,
        // because those keys are then sorted, took 5114ms against this branch's 86ms.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("local t = {} for i = 1, 2000000 do t[i] = i end return t"), 60000);
        QVERIFY(result.success);
        QVERIFY2(result.text.startsWith(qsl("[1,2,3")), qPrintable(result.text.left(60)));
    }

    void testAWideTableSaysItsKeysWereLeftOut()
    {
        // Cutting a table off without saying so would read to a model as a table that
        // genuinely holds only those keys.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("local t = {} for i = 1, 50000 do t['key'..i] = i end return t"), 60000);

        QVERIFY(result.success);
        QVERIFY2(result.text.contains(qsl("left out")), qPrintable(result.text.left(200)));
    }

    void testTheHookIsPutBackAfterTheCall()
    {
        // A count hook left armed would charge this deadline to whatever the interpreter
        // runs next - every trigger and timer in the profile.
        TMCPLuaBridge::runLua(L, qsl("while true do end"), 200);

        QVERIFY(lua_gethook(L) == nullptr);
        QCOMPARE(lua_gethookmask(L), 0);
    }

    void testAHookThatWasAlreadyArmedIsPutBackWithItsMaskAndCount()
    {
        // Mudlet arms its own count hook while a script runs, so a snippet called from
        // there has one to restore rather than none - and restoring it with the wrong
        // count would change how often that hook fires for the rest of the session.
        const auto otherHook = [](lua_State*, lua_Debug*) {};
        lua_sethook(L, otherHook, LUA_MASKCOUNT | LUA_MASKLINE, 4242);

        TMCPLuaBridge::runLua(L, qsl("while true do end"), 200);

        QVERIFY(lua_gethook(L) == otherHook);
        QCOMPARE(lua_gethookmask(L), LUA_MASKCOUNT | LUA_MASKLINE);
        QCOMPARE(lua_gethookcount(L), 4242);
    }

    void testNonScalarKeysDoNotOverwriteEachOther()
    {
        // Naming every key after its type alone collapsed {[true] = 1, [false] = 2} into
        // a single entry, silently losing half the table.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("return {[true] = 'yes', [false] = 'no'}"));

        QVERIFY(result.success);
        QVERIFY2(result.text.contains(qsl("yes")), qPrintable(result.text));
        QVERIFY2(result.text.contains(qsl("no")), qPrintable(result.text));
    }

    void testANonFiniteNumberInsideATableIsNotLost()
    {
        // JSON has no nan or inf, so QJsonValue(double) turns both into null and the
        // model is told the value was nil.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("return {ratio = 0/0, limit = 1/0}"));

        QVERIFY(result.success);
        QVERIFY2(result.text.contains(qsl("nan")), qPrintable(result.text));
        QVERIFY2(result.text.contains(qsl("inf")), qPrintable(result.text));
    }

    void testTheNoOutputMessageSaysWhereEchoedTextWent()
    {
        // echo() and cecho() write to the profile window, not to the tool result, so a
        // model that used them is otherwise told its code did nothing at all.
        const MCPToolResult result = TMCPLuaBridge::runLua(L, qsl("local x = 1"));

        QVERIFY(result.success);
        QVERIFY2(result.text.contains(qsl("echo")), qPrintable(result.text));
    }

    void testTheLuaStackIsLeftAsItWasFound()
    {
        const int before = lua_gettop(L);
        TMCPLuaBridge::runLua(L, qsl("return 1, 2, 3"));
        TMCPLuaBridge::runLua(L, qsl("error('boom')"));
        TMCPLuaBridge::runLua(L, qsl("nonsense nonsense"));

        QCOMPARE(lua_gettop(L), before);
    }
};

QTEST_MAIN(TMCPServerTest) // NOLINT(misc-use-anonymous-namespace)
#include "TMCPServerTest.moc"
