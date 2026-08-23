#ifndef MUDLET_TMCPSERVER_H
#define MUDLET_TMCPSERVER_H

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

#include <QHttpServerResponse>
#include <QJsonObject>
#include <QObject>
#include <QString>

class TMCPLuaBridge;
class QHttpHeaders;
class QHttpServer;
class QHttpServerRequest;

// The answer to one MCP message, before it becomes a QHttpServerResponse. Split out
// because a QHttpServerRequest can be default-constructed but not populated outside
// QtHttpServer, so this is the only seam at which the protocol can be driven without
// opening a real socket.
struct MCPReply
{
    QJsonObject body;
    QHttpServerResponse::StatusCode status = QHttpServerResponse::StatusCode::Ok;
};

// Whether the listener came up, and why not if it did not. The reason has to travel back
// to the caller: a bind clash is the one failure a user can actually do something about.
struct MCPStartResult
{
    bool started = false;
    QString error;
};

// A Model Context Protocol server speaking revision 2026-07-28 over Streamable HTTP.
// Specification: https://modelcontextprotocol.io/specification/2026-07-28
//
// That revision is stateless: there is no initialize handshake, no Mcp-Session-Id and no
// ping, and every request repeats its protocol version and capabilities in _meta. No
// per-request or per-session protocol state may therefore be held here.
//
// One server serves the whole application rather than one per profile. An MCP client is
// configured with a single URL, so a per-profile endpoint would move about as profiles are
// opened and closed, and the servers would all contend for the same port anyway.
class TMCPServer : public QObject
{
    Q_OBJECT

public:
    explicit TMCPServer(QObject* parent = nullptr);
    ~TMCPServer();

    // Port 0 asks the OS for a free one, which getPort() then reports back.
    MCPStartResult startServer(quint16 port = 0);
    void stopServer();
    bool running() const { return mpHttpServer != nullptr; }
    quint16 getPort() const { return mPort; }
    QString getEndpoint() const;
    // The address to paste into an MCP client for a given port. Shared with the preferences
    // dialog, which compares the address it would offer against the one that is live.
    static QString endpointFor(quint16 port, const QString& token = QString());

    // The shared secret a client must present. Binding to loopback is not an access
    // control: every other process on the machine can reach loopback too, and this tool
    // runs arbitrary Lua, which reaches os.execute() - so any local process, or any other
    // user with a session on a shared machine, would otherwise have code execution. Fresh
    // on every launch, so a token that leaks into a log or a screenshot stops working.
    QString authToken() const { return mAuthToken; }

    // Answers one JSON-RPC message. Public because it is the protocol seam - see MCPReply.
    // pathToken carries the secret when it arrived in the URL rather than in a header.
    MCPReply handleMessage(const QByteArray& requestBody, const QHttpHeaders& headers, const QString& pathToken = QString());

    static constexpr const char* MCP_PROTOCOL_VERSION = "2026-07-28";
    static constexpr const char* MCP_SERVER_NAME = "mudlet";

    // _meta keys the specification reserves, on the way in from the client...
    static constexpr const char* META_PROTOCOL_VERSION = "io.modelcontextprotocol/protocolVersion";
    static constexpr const char* META_CLIENT_CAPABILITIES = "io.modelcontextprotocol/clientCapabilities";
    // ...and on the way back out with every result.
    static constexpr const char* META_SERVER_INFO = "io.modelcontextprotocol/serverInfo";

    enum JsonRpcErrorCode {
        ParseError = -32700,
        InvalidRequest = -32600,
        MethodNotFound = -32601,
        InvalidParams = -32602,
        InternalError = -32603,
        // -32020 to -32099 is the range the specification reserves for itself; the older
        // -32000 to -32019 range must not be used by new implementations.
        HeaderMismatch = -32020,
        UnsupportedProtocolVersion = -32022,
        Unauthorized = -32024
    };

private:
    MCPReply dispatch(const QString& method, const QJsonValue& id, const QJsonObject& params);
    MCPReply discover(const QJsonValue& id) const;
    MCPReply listTools(const QJsonValue& id) const;
    MCPReply callTool(const QJsonValue& id, const QJsonObject& params);

    // Returns an empty string when the required mirror headers are present and agree
    // with the body, and the complaint to send back otherwise.
    QString headerMismatch(const QHttpHeaders& headers, const QString& method, const QJsonObject& params, const QJsonObject& meta) const;

    MCPReply unsupportedVersion(const QJsonValue& id, const QJsonValue& requested, const QString& message) const;

    MCPReply result(const QJsonValue& id, QJsonObject payload, QHttpServerResponse::StatusCode status = QHttpServerResponse::StatusCode::Ok) const;
    MCPReply error(const QJsonValue& id,
                   JsonRpcErrorCode code,
                   const QString& message,
                   const QJsonValue& data = QJsonValue(),
                   QHttpServerResponse::StatusCode status = QHttpServerResponse::StatusCode::Ok) const;

    QJsonObject serverImplementation() const;
    static QString decodeHeaderValue(QByteArrayView raw);
    static bool originAllowed(const QString& origin);
    bool authorised(const QHttpHeaders& headers, const QString& pathToken) const;

    QHttpServerResponse respondToPost(const QHttpServerRequest& request, const QString& pathToken = QString());

    TMCPLuaBridge* mpLuaBridge;
    QString mAuthToken;
    // Recreated on every start: QHttpServer has no way to drop a route or unbind a
    // socket, so a restart would otherwise stack a second copy of each route.
    QHttpServer* mpHttpServer = nullptr;
    quint16 mPort = 0;
};

#endif // MUDLET_TMCPSERVER_H
