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

#include <TMCPLuaBridge.h>
#include <TMCPServer.h>
#include <utils.h>

#include <QtTest/QtTest>

#include <QHttpHeaders>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>

using StatusCode = QHttpServerResponse::StatusCode;

namespace {
const QString currentVersion = QString::fromLatin1(TMCPServer::MCP_PROTOCOL_VERSION);

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
    return headers;
}

QByteArray bodyFor(const QString& method, const QJsonObject& params = QJsonObject(), const QJsonValue& id = QJsonValue(1), const QString& version = currentVersion)
{
    QJsonObject meta;
    meta[QString::fromLatin1(TMCPServer::META_PROTOCOL_VERSION)] = version;
    meta[QString::fromLatin1(TMCPServer::META_CLIENT_CAPABILITIES)] = QJsonObject();

    QJsonObject withMeta = params;
    withMeta[QStringLiteral("_meta")] = meta;

    QJsonObject message;
    message[QStringLiteral("jsonrpc")] = QStringLiteral("2.0");
    if (!id.isUndefined()) {
        message[QStringLiteral("id")] = id;
    }
    message[QStringLiteral("method")] = method;
    message[QStringLiteral("params")] = withMeta;
    return QJsonDocument(message).toJson(QJsonDocument::Compact);
}

QJsonObject resultOf(const MCPReply& reply)
{
    return reply.body.value(QStringLiteral("result")).toObject();
}

QJsonObject errorOf(const MCPReply& reply)
{
    return reply.body.value(QStringLiteral("error")).toObject();
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
        // No Host: every case here is either protocol handling or a Lua snippet run
        // against the bare interpreter below, neither of which needs a profile.
        server = new TMCPServer(nullptr, nullptr);
        L = luaL_newstate();
        luaL_openlibs(L);
    }

    void cleanup()
    {
        delete server;
        server = nullptr;
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
        QCOMPARE(result.value(qsl("content")).toArray().first().toObject().value(qsl("type")).toString(), qsl("text"));
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
        // An earlier revision of this server read the id with toInt(), which turned every
        // string id into 0 and broke correlation for clients that use them.
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

    void testSessionHeaderIsIgnoredAndNeverMinted()
    {
        // Sessions left the protocol in 2026-07-28; an old client sending one must not
        // be handed a session id back, or it will keep talking the old dialect.
        QHttpHeaders headers = headersFor(qsl("tools/list"));
        headers.append("Mcp-Session-Id", "left-over-from-2025");

        const MCPReply reply = server->handleMessage(bodyFor(qsl("tools/list")), headers);

        QCOMPARE(reply.status, StatusCode::Ok);
        QVERIFY(!resultOf(reply).contains(qsl("sessionId")));
    }

    // ---------- listening ----------

    void testStartingAndStoppingCanBeRepeated()
    {
        // Routes used to be registered onto a long-lived QHttpServer on every start, so
        // a second start stacked a duplicate of each one.
        QVERIFY(!server->running());
        QVERIFY(server->startServer(0));
        QVERIFY(server->running());
        const int firstPort = server->getPort();
        QVERIFY(firstPort > 0);
        QVERIFY(server->getEndpoint().contains(QString::number(firstPort)));

        server->stopServer();
        QVERIFY(!server->running());
        QCOMPARE(server->getPort(), 0);

        QVERIFY(server->startServer(0));
        QVERIFY(server->running());
        server->stopServer();
    }

    void testStartingTwiceIsRefused()
    {
        QVERIFY(server->startServer(0));
        QVERIFY(!server->startServer(0));
        server->stopServer();
    }

    void testARealPostOverTheSocketIsAnswered()
    {
        // Everything above drives handleMessage() directly; this is the only case that
        // proves the route, the binding and the QHttpServerResponse conversion as well.
        QVERIFY(server->startServer(0));

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
        QVERIFY(server->startServer(0));

        QNetworkAccessManager manager;
        const QScopedPointer<QNetworkReply> reply(manager.get(QNetworkRequest{QUrl(server->getEndpoint())}));
        QTRY_VERIFY_WITH_TIMEOUT(reply->isFinished(), 5000);

        QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 405);

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

    void testPrintIsPutBackAfterTheCallAndLeavesNoGlobals()
    {
        // The capture used to park its saved print and its collected lines in _G, where
        // they outlived the call and collided with any script using those names. Read _G
        // from outside the runner and by name rather than checking for the two names that
        // version happened to use, so this still bites if the runner is rewritten.
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
        // lua_tostring() answers null for a table, which used to drop the message.
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
