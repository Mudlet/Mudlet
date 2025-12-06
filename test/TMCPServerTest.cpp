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

#include <TMCPServer.h>
#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUuid>

class TMCPServerTest : public QObject
{
    Q_OBJECT

private:
    TMCPServer* server = nullptr;

private slots:
    void init()
    {
        server = new TMCPServer(nullptr, nullptr);
    }

    void cleanup()
    {
        delete server;
        server = nullptr;
    }

    // ========== createJsonRpcError tests ==========

    void testCreateJsonRpcError_basicError()
    {
        QJsonObject error = server->createJsonRpcError(1, -32600, "Invalid Request");

        QCOMPARE(error["jsonrpc"].toString(), QString("2.0"));
        QCOMPARE(error["id"].toInt(), 1);
        QVERIFY(error.contains("error"));

        QJsonObject errorObj = error["error"].toObject();
        QCOMPARE(errorObj["code"].toInt(), -32600);
        QCOMPARE(errorObj["message"].toString(), QString("Invalid Request"));
        QVERIFY(!errorObj.contains("data"));
    }

    void testCreateJsonRpcError_withData()
    {
        QJsonObject data;
        data["detail"] = "additional info";
        QJsonObject error = server->createJsonRpcError(42, -32602, "Invalid params", QJsonValue(data));

        QCOMPARE(error["id"].toInt(), 42);
        QJsonObject errorObj = error["error"].toObject();
        QCOMPARE(errorObj["code"].toInt(), -32602);
        QVERIFY(errorObj.contains("data"));
        QCOMPARE(errorObj["data"].toObject()["detail"].toString(), QString("additional info"));
    }

    void testCreateJsonRpcError_parseError()
    {
        QJsonObject error = server->createJsonRpcError(-1, -32700, "Parse error");
        QJsonObject errorObj = error["error"].toObject();
        QCOMPARE(errorObj["code"].toInt(), -32700);
    }

    void testCreateJsonRpcError_invalidRequest()
    {
        QJsonObject error = server->createJsonRpcError(1, -32600, "Invalid Request");
        QJsonObject errorObj = error["error"].toObject();
        QCOMPARE(errorObj["code"].toInt(), -32600);
    }

    void testCreateJsonRpcError_methodNotFound()
    {
        QJsonObject error = server->createJsonRpcError(1, -32601, "Method not found");
        QJsonObject errorObj = error["error"].toObject();
        QCOMPARE(errorObj["code"].toInt(), -32601);
    }

    void testCreateJsonRpcError_invalidParams()
    {
        QJsonObject error = server->createJsonRpcError(1, -32602, "Invalid params");
        QJsonObject errorObj = error["error"].toObject();
        QCOMPARE(errorObj["code"].toInt(), -32602);
    }

    void testCreateJsonRpcError_internalError()
    {
        QJsonObject error = server->createJsonRpcError(1, -32603, "Internal error");
        QJsonObject errorObj = error["error"].toObject();
        QCOMPARE(errorObj["code"].toInt(), -32603);
    }

    void testCreateJsonRpcError_serverError()
    {
        QJsonObject error = server->createJsonRpcError(1, -32000, "Server error");
        QJsonObject errorObj = error["error"].toObject();
        QCOMPARE(errorObj["code"].toInt(), -32000);
    }

    // ========== generateSessionId tests ==========

    void testGenerateSessionId_format()
    {
        QString sessionId = server->generateSessionId();

        QVERIFY(!sessionId.isEmpty());
        QVERIFY(!sessionId.contains("{"));
        QVERIFY(!sessionId.contains("}"));
        QCOMPARE(sessionId.length(), 36);
    }

    void testGenerateSessionId_uniqueness()
    {
        QSet<QString> sessionIds;
        for (int i = 0; i < 100; ++i) {
            QString sessionId = server->generateSessionId();
            QVERIFY(!sessionIds.contains(sessionId));
            sessionIds.insert(sessionId);
        }
    }

    void testGenerateSessionId_validUuid()
    {
        QString sessionId = server->generateSessionId();
        QUuid uuid = QUuid::fromString(sessionId);
        QVERIFY(!uuid.isNull());
    }

    // ========== getOrCreateSession tests ==========

    void testGetOrCreateSession_newSession()
    {
        QString sessionId = "test-session-id";
        auto* session = server->getOrCreateSession(sessionId);

        QVERIFY(session != nullptr);
        QCOMPARE(session->sessionId, sessionId);
        QVERIFY(!session->initialized);
        QCOMPARE(session->requestId, 1);
    }

    void testGetOrCreateSession_existingSession()
    {
        QString sessionId = "test-session-id";
        auto* session1 = server->getOrCreateSession(sessionId);
        session1->initialized = true;
        session1->clientName = "TestClient";

        auto* session2 = server->getOrCreateSession(sessionId);

        QCOMPARE(session1, session2);
        QVERIFY(session2->initialized);
        QCOMPARE(session2->clientName, QString("TestClient"));
    }

    void testGetOrCreateSession_multipleSessions()
    {
        auto* session1 = server->getOrCreateSession("session-1");
        auto* session2 = server->getOrCreateSession("session-2");

        QVERIFY(session1 != session2);
        QCOMPARE(session1->sessionId, QString("session-1"));
        QCOMPARE(session2->sessionId, QString("session-2"));
    }

    // ========== createInitializeResult tests ==========

    void testCreateInitializeResult_structure()
    {
        QJsonObject result = server->createInitializeResult("2025-06-18");

        QVERIFY(result.contains("protocolVersion"));
        QVERIFY(result.contains("serverInfo"));
        QVERIFY(result.contains("capabilities"));
    }

    void testCreateInitializeResult_protocolVersion()
    {
        QJsonObject result = server->createInitializeResult("2025-06-18");
        QCOMPARE(result["protocolVersion"].toString(), QString("2025-06-18"));
    }

    void testCreateInitializeResult_serverInfo()
    {
        QJsonObject result = server->createInitializeResult("2025-06-18");
        QJsonObject serverInfo = result["serverInfo"].toObject();

        QVERIFY(serverInfo.contains("name"));
        QVERIFY(serverInfo.contains("version"));
        QCOMPARE(serverInfo["name"].toString(), QString("mudlet"));
        QCOMPARE(serverInfo["version"].toString(), QString("1.0.0"));
    }

    void testCreateInitializeResult_capabilities()
    {
        QJsonObject result = server->createInitializeResult("2025-06-18");
        QJsonObject capabilities = result["capabilities"].toObject();

        QVERIFY(capabilities.contains("tools"));
        QJsonObject tools = capabilities["tools"].toObject();
        QVERIFY(tools.contains("listChanged"));
        QCOMPARE(tools["listChanged"].toBool(), true);
    }

    // ========== handleInitializeRequest tests ==========

    void testHandleInitializeRequest_success()
    {
        QJsonObject request;
        request["jsonrpc"] = "2.0";
        request["id"] = 1;
        request["method"] = "initialize";

        QJsonObject params;
        params["protocolVersion"] = TMCPServer::MCP_PROTOCOL_VERSION;
        QJsonObject clientInfo;
        clientInfo["name"] = "TestClient";
        clientInfo["version"] = "1.0.0";
        params["clientInfo"] = clientInfo;
        request["params"] = params;

        QJsonObject response;
        auto* session = server->getOrCreateSession("test-session");
        server->handleInitializeRequest(request, response, session);

        QCOMPARE(response["jsonrpc"].toString(), QString("2.0"));
        QCOMPARE(response["id"].toInt(), 1);
        QVERIFY(response.contains("result"));
        QVERIFY(!response.contains("error"));

        QVERIFY(session->initialized);
        QCOMPARE(session->clientName, QString("TestClient"));
        QCOMPARE(session->clientVersion, QString("1.0.0"));
    }

    void testHandleInitializeRequest_missingProtocolVersion()
    {
        QJsonObject request;
        request["jsonrpc"] = "2.0";
        request["id"] = 1;

        QJsonObject params;
        QJsonObject clientInfo;
        clientInfo["name"] = "TestClient";
        params["clientInfo"] = clientInfo;
        request["params"] = params;

        QJsonObject response;
        auto* session = server->getOrCreateSession("test-session");
        server->handleInitializeRequest(request, response, session);

        QVERIFY(response.contains("error"));
        QJsonObject error = response["error"].toObject();
        QCOMPARE(error["code"].toInt(), -32602);
        QVERIFY(error["message"].toString().contains("protocolVersion"));
    }

    void testHandleInitializeRequest_missingClientInfo()
    {
        QJsonObject request;
        request["jsonrpc"] = "2.0";
        request["id"] = 1;

        QJsonObject params;
        params["protocolVersion"] = TMCPServer::MCP_PROTOCOL_VERSION;
        request["params"] = params;

        QJsonObject response;
        auto* session = server->getOrCreateSession("test-session");
        server->handleInitializeRequest(request, response, session);

        QVERIFY(response.contains("error"));
        QJsonObject error = response["error"].toObject();
        QCOMPARE(error["code"].toInt(), -32602);
        QVERIFY(error["message"].toString().contains("clientInfo"));
    }

    void testHandleInitializeRequest_missingClientName()
    {
        QJsonObject request;
        request["jsonrpc"] = "2.0";
        request["id"] = 1;

        QJsonObject params;
        params["protocolVersion"] = TMCPServer::MCP_PROTOCOL_VERSION;
        QJsonObject clientInfo;
        clientInfo["version"] = "1.0.0";
        params["clientInfo"] = clientInfo;
        request["params"] = params;

        QJsonObject response;
        auto* session = server->getOrCreateSession("test-session");
        server->handleInitializeRequest(request, response, session);

        QVERIFY(response.contains("error"));
        QJsonObject error = response["error"].toObject();
        QCOMPARE(error["code"].toInt(), -32602);
        QVERIFY(error["message"].toString().contains("name"));
    }

    void testHandleInitializeRequest_versionMismatch()
    {
        QJsonObject request;
        request["jsonrpc"] = "2.0";
        request["id"] = 1;

        QJsonObject params;
        params["protocolVersion"] = "1999-01-01";
        QJsonObject clientInfo;
        clientInfo["name"] = "TestClient";
        params["clientInfo"] = clientInfo;
        request["params"] = params;

        QJsonObject response;
        auto* session = server->getOrCreateSession("test-session");
        server->handleInitializeRequest(request, response, session);

        QVERIFY(response.contains("error"));
        QJsonObject error = response["error"].toObject();
        QCOMPARE(error["code"].toInt(), -32602);
        QVERIFY(error.contains("data"));
        QJsonObject errorData = error["data"].toObject();
        QVERIFY(errorData.contains("supported"));
        QVERIFY(errorData.contains("requested"));
    }

    // ========== handleListToolsRequest tests ==========

    void testHandleListToolsRequest_structure()
    {
        QJsonObject request;
        request["jsonrpc"] = "2.0";
        request["id"] = 1;
        request["method"] = "tools/list";

        QJsonObject response;
        server->handleListToolsRequest(request, response);

        QCOMPARE(response["jsonrpc"].toString(), QString("2.0"));
        QCOMPARE(response["id"].toInt(), 1);
        QVERIFY(response.contains("result"));

        QJsonObject result = response["result"].toObject();
        QVERIFY(result.contains("tools"));
        QVERIFY(result["tools"].isArray());
    }

    // ========== handlePingRequest tests ==========

    void testHandlePingRequest_emptyResult()
    {
        QJsonObject request;
        request["jsonrpc"] = "2.0";
        request["id"] = 42;
        request["method"] = "ping";

        QJsonObject response;
        server->handlePingRequest(request, response);

        QCOMPARE(response["jsonrpc"].toString(), QString("2.0"));
        QCOMPARE(response["id"].toInt(), 42);
        QVERIFY(response.contains("result"));
        QVERIFY(response["result"].isObject());
        QVERIFY(response["result"].toObject().isEmpty());
    }

    // ========== handleCallToolRequest tests ==========

    void testHandleCallToolRequest_missingName()
    {
        QJsonObject request;
        request["jsonrpc"] = "2.0";
        request["id"] = 1;
        request["method"] = "tools/call";

        QJsonObject params;
        request["params"] = params;

        QJsonObject response;
        server->handleCallToolRequest(request, response);

        QVERIFY(response.contains("error"));
        QJsonObject error = response["error"].toObject();
        QCOMPARE(error["code"].toInt(), -32602);
        QVERIFY(error["message"].toString().contains("name"));
    }

    void testHandleCallToolRequest_emptyName()
    {
        QJsonObject request;
        request["jsonrpc"] = "2.0";
        request["id"] = 1;
        request["method"] = "tools/call";

        QJsonObject params;
        params["name"] = "";
        request["params"] = params;

        QJsonObject response;
        server->handleCallToolRequest(request, response);

        QVERIFY(response.contains("error"));
        QJsonObject error = response["error"].toObject();
        QCOMPARE(error["code"].toInt(), -32602);
        QVERIFY(error["message"].toString().contains("empty"));
    }

    void testHandleCallToolRequest_unknownTool()
    {
        QJsonObject request;
        request["jsonrpc"] = "2.0";
        request["id"] = 1;
        request["method"] = "tools/call";

        QJsonObject params;
        params["name"] = "nonexistent_tool";
        request["params"] = params;

        QJsonObject response;
        server->handleCallToolRequest(request, response);

        QVERIFY(response.contains("error"));
        QJsonObject error = response["error"].toObject();
        QCOMPARE(error["code"].toInt(), -32601);
    }

    // ========== Server state tests ==========

    void testIsRunning_initial()
    {
        QVERIFY(!server->isRunning());
    }

    void testGetPort_initial()
    {
        QCOMPARE(server->getPort(), 0);
    }

    void testGetServerInfo_notRunning()
    {
        QString info = server->getServerInfo();
        QVERIFY(info.contains("Not running"));
    }

    // ========== Protocol constants tests ==========

    void testProtocolVersion()
    {
        QCOMPARE(QString::fromLatin1(TMCPServer::MCP_PROTOCOL_VERSION), QString("2025-06-18"));
    }

    void testServerName()
    {
        QCOMPARE(QString::fromLatin1(TMCPServer::MCP_SERVER_NAME), QString("mudlet"));
    }

    void testServerVersion()
    {
        QCOMPARE(QString::fromLatin1(TMCPServer::MCP_SERVER_VERSION), QString("1.0.0"));
    }
};

#include "TMCPServerTest.moc"
QTEST_MAIN(TMCPServerTest)
