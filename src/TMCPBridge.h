#ifndef MUDLET_TMCPBRIDGE_H
#define MUDLET_TMCPBRIDGE_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QJsonObject>
#include <QObject>
#include <QString>

class QNetworkAccessManager;

struct MCPDiscovery
{
    quint16 port = 0;
    QString token;
    // Both halves are needed to reach the server, so either one missing means the file
    // held nothing usable.
    bool valid() const { return port != 0 && !token.isEmpty(); }
};

// The stdio side of Mudlet's MCP support: `mudlet --mcp-bridge` relays JSON-RPC lines
// between a stdio MCP client (Claude Desktop and friends can only launch a command, not
// call a URL) and the HTTP server inside a running Mudlet.
//
// The two ends speak different revisions of the protocol. Stdio clients hold a stateful
// 2025-06-18 style conversation - initialize handshake, pings, prompts/resources
// enquiries - while TMCPServer is stateless 2026-07-28. The bridge answers the stateful
// parts itself and forwards the rest with the newer revision's _meta envelope added.
//
// The running Mudlet advertises where to find it through a discovery file in its config
// directory, which the bridge re-reads on every request. That, not a baked-in address,
// is what lets Mudlet restart - new port, new token - without the user touching anything.
class TMCPBridge : public QObject
{
    Q_OBJECT

public:
    // Runs the bridge until stdin closes. Needs a QCoreApplication to already exist.
    static int exec(const QString& configDir);

    static QString discoveryFilePath(const QString& configDir);
    static bool writeDiscoveryFile(const QString& configDir, quint16 port, const QString& token);
    static void removeDiscoveryFile(const QString& configDir);
    // Removes the file only when no server is answering on the port it names: with
    // several Mudlets open the file belongs to whichever instance actually holds the
    // port, and an instance that never got it must not tear down another's signpost.
    static void removeDiscoveryFileIfStale(const QString& configDir);
    static MCPDiscovery readDiscoveryFile(const QString& configDir);

    // The protocol seam, free of process and socket state so tests can drive it directly.
    // Returns true when the message is the bridge's own to answer (reply may stay empty:
    // notifications get swallowed); initialize stores the client's declared capabilities
    // into clientCapabilities for forwardEnvelope to repeat on every later request.
    static bool replyLocally(const QJsonObject& message, QJsonObject& reply, QJsonObject& clientCapabilities);
    static QJsonObject forwardEnvelope(const QJsonObject& message, const QJsonObject& clientCapabilities);

    enum class ConnectOutcome { Written, NoClaudeDesktop, NoBinaryPath, ConfigUnreadable, WriteFailed };

    // Registers `mudlet --mcp-bridge` in Claude Desktop's settings file, keeping every
    // other setting - though the file is re-serialised, so its key order and spacing
    // are not preserved.
    static ConnectOutcome connectClaudeDesktop();
    // Repairs a previously registered entry whose binary path has gone stale - an
    // AppImage lands under a new name every release. Writes nothing unless the user
    // opted in earlier and the entry is still the bridge's own.
    static void refreshClaudeDesktopEntry();

    static QString claudeDesktopConfigDir();
    static QString claudeDesktopConfigFilePath();
    static QString mudletBinaryPath();
    static QJsonObject claudeDesktopEntry(const QString& command);
    // Returns the file contents to write; ok comes back false when existingConfig is
    // present but not a JSON object, in which case nothing must be written over it.
    static QByteArray mergeClaudeDesktopConfig(const QByteArray& existingConfig, const QJsonObject& entry, bool& ok);

private:
    explicit TMCPBridge(const QString& configDir);

    void startStdinReader();
    void handleLine(const QByteArray& line);
    void forward(const QJsonObject& message);
    static QJsonObject jsonRpcError(const QJsonValue& id, int code, const QString& message);
    static QJsonObject bridgeError(const QJsonValue& id, const QString& message);
    static void writeMessage(const QJsonObject& message);

    QString mConfigDir;
    QNetworkAccessManager* mpNetworkManager = nullptr;
    // What the client said in initialize; the 2026-07-28 revision wants it repeated in
    // every request's _meta.
    QJsonObject mClientCapabilities;
};

#endif // MUDLET_TMCPBRIDGE_H
