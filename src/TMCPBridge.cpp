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

#include "TMCPBridge.h"

#include "TMCPServer.h"
#include "utils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTcpServer>

#include <cstdio>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

#if defined(Q_OS_WIN32)
#include <fcntl.h>
#include <io.h>
#endif

namespace {
// stderr is the one diagnostic channel a stdio MCP server has - stdout belongs to the
// protocol, and clients like Claude Desktop file stderr into a per-server log.
void noteOnStderr(const QString& note)
{
    std::cerr << note.toStdString() << std::endl;
}
} // namespace

TMCPBridge::TMCPBridge(const QString& configDir)
: mConfigDir(configDir)
, mpNetworkManager(new QNetworkAccessManager(this))
{
    // Until initialize says otherwise: what forwardEnvelope repeats in every request's
    // _meta then declares tool support rather than an empty capability set, even for a
    // client that skipped the handshake.
    mClientCapabilities[qsl("tools")] = QJsonObject();
}

int TMCPBridge::exec(const QString& configDir)
{
    TMCPBridge bridge(configDir);
    bridge.startStdinReader();
    return QCoreApplication::exec();
}

void TMCPBridge::startStdinReader()
{
#if defined(Q_OS_WIN32)
    // Text mode would translate line endings inside the JSON payloads.
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    // Detached because there is no portable way to interrupt a blocking getline. Once
    // the loop ends the thread touches only the application object, to post quit; the
    // bridge outlives every post to itself because exec() cannot return before that
    // quit runs, and queued calls tied to `this` as their context are dropped by Qt if
    // any are still in flight at destruction. Nothing else may call quit(), or the
    // application object could be gone while the reader still posts to it.
    std::thread([this]() {
        std::string line;
        while (std::getline(std::cin, line)) {
            const QByteArray bytes = QByteArray::fromStdString(line);
            QMetaObject::invokeMethod(
                    this,
                    [this, bytes]() {
                        handleLine(bytes);
                    },
                    Qt::QueuedConnection);
        }
        if (std::cin.bad()) {
            noteOnStderr(qsl("mudlet --mcp-bridge: stdin failed with a read error; shutting down"));
        }
        QMetaObject::invokeMethod(QCoreApplication::instance(), &QCoreApplication::quit, Qt::QueuedConnection);
    }).detach();
}

void TMCPBridge::handleLine(const QByteArray& line)
{
    if (line.trimmed().isEmpty()) {
        return;
    }
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        noteOnStderr(qsl("mudlet --mcp-bridge: dropping an unparseable line (%1): %2").arg(parseError.errorString(), QString::fromUtf8(line.left(80))));
        // A null id is what JSON-RPC prescribes for a request that never parsed.
        writeMessage(jsonRpcError(QJsonValue::Null, -32700, qsl("Parse error: %1").arg(parseError.errorString())));
        return;
    }
    if (!doc.isObject()) {
        // Initialize pins the 2025-06-18 revision, which retired JSON-RPC batching, so
        // an array (or any other non-object) is an invalid request rather than a format
        // the bridge quietly does not speak.
        noteOnStderr(qsl("mudlet --mcp-bridge: dropping a line that is not a JSON-RPC object: %1").arg(QString::fromUtf8(line.left(80))));
        writeMessage(jsonRpcError(QJsonValue::Null, -32600, qsl("Invalid Request: expected a single JSON-RPC object")));
        return;
    }
    const QJsonObject message = doc.object();
    QJsonObject reply;
    if (replyLocally(message, reply, mClientCapabilities)) {
        if (!reply.isEmpty()) {
            writeMessage(reply);
        }
        return;
    }
    forward(message);
}

bool TMCPBridge::replyLocally(const QJsonObject& message, QJsonObject& reply, QJsonObject& clientCapabilities)
{
    const QJsonValue id = message.value(qsl("id"));
    if (id.isUndefined() || id.isNull()) {
        // A notification - counting an explicit null id as one too, which is more
        // lenient than JSON-RPC 2.0 strictly allows but matches TMCPServer's reading.
        // The stateless server has no use for any of them, and a reply to a
        // notification would itself violate JSON-RPC.
        return true;
    }

    const QString method = message.value(qsl("method")).toString();
    auto result = [&id](const QJsonObject& payload) {
        QJsonObject out;
        out[qsl("jsonrpc")] = qsl("2.0");
        out[qsl("id")] = id;
        out[qsl("result")] = payload;
        return out;
    };

    if (method == qsl("initialize")) {
        const QJsonObject params = message.value(qsl("params")).toObject();
        if (params.value(qsl("capabilities")).isObject()) {
            clientCapabilities = params.value(qsl("capabilities")).toObject();
        }
        QJsonObject tools;
        tools[qsl("listChanged")] = false;
        QJsonObject capabilities;
        capabilities[qsl("tools")] = tools;
        QJsonObject serverInfo;
        serverInfo[qsl("name")] = qsl("Mudlet");
        serverInfo[qsl("version")] = QString::fromLatin1(APP_VERSION);
        QJsonObject payload;
        // The revision the bridge actually implements, not an echo of the client's:
        // agreeing to whatever was proposed would promise semantics handleLine does not
        // speak - 2025-03-26 batching, say. A client on another revision falls back or
        // disconnects, as the spec directs.
        payload[qsl("protocolVersion")] = qsl("2025-06-18");
        payload[qsl("capabilities")] = capabilities;
        payload[qsl("serverInfo")] = serverInfo;
        reply = result(payload);
        return true;
    }
    if (method == qsl("ping")) {
        reply = result(QJsonObject());
        return true;
    }
    if (method == qsl("prompts/list")) {
        QJsonObject payload;
        payload[qsl("prompts")] = QJsonArray();
        reply = result(payload);
        return true;
    }
    if (method == qsl("resources/list")) {
        QJsonObject payload;
        payload[qsl("resources")] = QJsonArray();
        reply = result(payload);
        return true;
    }
    if (method == qsl("resources/templates/list")) {
        QJsonObject payload;
        payload[qsl("resourceTemplates")] = QJsonArray();
        reply = result(payload);
        return true;
    }
    return false;
}

QJsonObject TMCPBridge::forwardEnvelope(const QJsonObject& message, const QJsonObject& clientCapabilities)
{
    QJsonObject params = message.value(qsl("params")).toObject();
    QJsonObject meta = params.value(qsl("_meta")).toObject();
    meta[QString::fromLatin1(TMCPServer::META_PROTOCOL_VERSION)] = QString::fromLatin1(TMCPServer::MCP_PROTOCOL_VERSION);
    meta[QString::fromLatin1(TMCPServer::META_CLIENT_CAPABILITIES)] = clientCapabilities;
    params[qsl("_meta")] = meta;
    QJsonObject envelope = message;
    envelope[qsl("params")] = params;
    return envelope;
}

QJsonObject TMCPBridge::jsonRpcError(const QJsonValue& id, const int code, const QString& message)
{
    QJsonObject error;
    error[qsl("code")] = code;
    error[qsl("message")] = message;
    QJsonObject out;
    out[qsl("jsonrpc")] = qsl("2.0");
    out[qsl("id")] = id;
    out[qsl("error")] = error;
    return out;
}

QJsonObject TMCPBridge::bridgeError(const QJsonValue& id, const QString& message)
{
    // -32000 sits in JSON-RPC 2.0's implementation-defined server-error range, the
    // closest thing there is to "the bridge itself could not manage this".
    return jsonRpcError(id, -32000, message);
}

void TMCPBridge::forward(const QJsonObject& message)
{
    const QJsonValue id = message.value(qsl("id"));
    // Read afresh on every request rather than cached: Mudlet rotates port and token
    // each launch, and this re-read is what lets the bridge survive a Mudlet restart.
    const MCPDiscovery discovery = readDiscoveryFile(mConfigDir);
    if (!discovery.valid()) {
        const QString path = discoveryFilePath(mConfigDir);
        if (QFileInfo::exists(path)) {
            // Present but unusable - unreadable, garbage, or half-written - which asks
            // for a different remedy than "start Mudlet".
            noteOnStderr(qsl("mudlet --mcp-bridge: the discovery file at %1 exists but could not be used").arg(path));
            writeMessage(bridgeError(id, qsl("Mudlet's discovery file at %1 could not be used - switch AI assistant access off and back on in Mudlet's preferences to rewrite it.").arg(path)));
            return;
        }
        noteOnStderr(qsl("mudlet --mcp-bridge: no discovery file at %1").arg(path));
        writeMessage(bridgeError(id, qsl("Cannot reach Mudlet. Is Mudlet running, with AI assistant access switched on in its preferences?")));
        return;
    }

    const QString method = message.value(qsl("method")).toString();
    QNetworkRequest request(QUrl(qsl("http://127.0.0.1:%1/mcp").arg(QString::number(discovery.port))));
    request.setHeader(QNetworkRequest::ContentTypeHeader, qsl("application/json"));
    request.setRawHeader("Authorization", qsl("Bearer %1").arg(discovery.token).toUtf8());
    // The mirror headers the 2026-07-28 revision wants alongside the body.
    request.setRawHeader("MCP-Protocol-Version", TMCPServer::MCP_PROTOCOL_VERSION);
    request.setRawHeader("Mcp-Method", method.toUtf8());
    if (method == qsl("tools/call")) {
        request.setRawHeader("Mcp-Name", message.value(qsl("params")).toObject().value(qsl("name")).toString().toUtf8());
    }
    // Three times the server's own Lua deadline (TMCPLuaBridge::csmDefaultDeadlineMs),
    // so a slow tool call still answers and a dead port does not hang the client.
    request.setTransferTimeout(30000);

    QNetworkReply* reply = mpNetworkManager->post(request, QJsonDocument(forwardEnvelope(message, mClientCapabilities)).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, id]() {
        reply->deleteLater();
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) {
            // The server frames its own failures as JSON-RPC errors, so what came back
            // goes to the client even on a non-2xx status - but rejections the server
            // makes before reading the body (a bad token, a refused Origin) carry a
            // null id, which a stdio client cannot match to any pending request, so the
            // request's own id is restored first.
            QJsonObject body = doc.object();
            if (body.value(qsl("id")) != id) {
                body[qsl("id")] = id;
            }
            writeMessage(body);
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            noteOnStderr(qsl("mudlet --mcp-bridge: could not reach Mudlet: %1").arg(reply->errorString()));
            writeMessage(bridgeError(id, qsl("Cannot reach Mudlet: %1. Is Mudlet running, with AI assistant access switched on in its preferences?").arg(reply->errorString())));
            return;
        }
        noteOnStderr(qsl("mudlet --mcp-bridge: Mudlet answered HTTP %1 with a body that is not JSON").arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()));
        writeMessage(bridgeError(id, qsl("Mudlet returned an unreadable response (HTTP %1)").arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())));
    });
}

void TMCPBridge::writeMessage(const QJsonObject& message)
{
    const QByteArray line = QJsonDocument(message).toJson(QJsonDocument::Compact).append('\n');
    const size_t written = std::fwrite(line.constData(), 1, static_cast<size_t>(line.size()), stdout);
    if (written != static_cast<size_t>(line.size()) || std::fflush(stdout) != 0) {
        // The client sees a truncated or missing reply; this note is the only record
        // that it was lost on the way out rather than never produced.
        noteOnStderr(qsl("mudlet --mcp-bridge: could not write a reply to stdout"));
    }
}

QString TMCPBridge::discoveryFilePath(const QString& configDir)
{
    return qsl("%1/mcp-server.json").arg(configDir);
}

bool TMCPBridge::writeDiscoveryFile(const QString& configDir, const quint16 port, const QString& token)
{
    QDir().mkpath(configDir);
    QSaveFile file(discoveryFilePath(configDir));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    // The token is the only thing keeping other local users off the server, so nobody
    // else may read this file. On a filesystem with no permission bits (FAT32 on a
    // portable install, say) the restriction cannot be applied; the file is written
    // anyway, since refusing would break the feature there entirely, but it is worth
    // a trace.
    if (!file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        qWarning() << "TMCPBridge::writeDiscoveryFile(...) WARNING - could not make" << file.fileName() << "private to this user; other users of this machine may be able to read the MCP access token";
    }
    QJsonObject contents;
    contents[qsl("port")] = static_cast<int>(port);
    contents[qsl("token")] = token;
    file.write(QJsonDocument(contents).toJson(QJsonDocument::Compact));
    return file.commit();
}

void TMCPBridge::removeDiscoveryFile(const QString& configDir)
{
    const QString path = discoveryFilePath(configDir);
    if (QFile::exists(path) && !QFile::remove(path)) {
        qWarning() << "TMCPBridge::removeDiscoveryFile(...) WARNING - could not remove" << path << "- `mudlet --mcp-bridge` may keep contacting a server that is gone";
    }
}

void TMCPBridge::removeDiscoveryFileIfStale(const QString& configDir)
{
    const MCPDiscovery discovery = readDiscoveryFile(configDir);
    if (discovery.valid()) {
        QTcpServer probe;
        if (!probe.listen(QHostAddress::LocalHost, discovery.port)) {
            // Something answers on that port - most likely another Mudlet instance
            // whose signpost this is. (It could also be an unrelated program squatting
            // a crashed session's port; erring towards keeping the file costs one
            // clear bridge error, removing a live instance's file cuts it off.)
            return;
        }
    }
    removeDiscoveryFile(configDir);
}

MCPDiscovery TMCPBridge::readDiscoveryFile(const QString& configDir)
{
    MCPDiscovery discovery;
    QFile file(discoveryFilePath(configDir));
    if (!file.open(QIODevice::ReadOnly)) {
        return discovery;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return discovery;
    }
    const QJsonObject contents = doc.object();
    const int port = contents.value(qsl("port")).toInt();
    const QString token = contents.value(qsl("token")).toString();
    if (port <= 0 || port > std::numeric_limits<quint16>::max() || token.isEmpty()) {
        return discovery;
    }
    discovery.port = static_cast<quint16>(port);
    discovery.token = token;
    return discovery;
}

QString TMCPBridge::claudeDesktopConfigDir()
{
#if defined(Q_OS_MACOS)
    return qsl("%1/Library/Application Support/Claude").arg(QDir::homePath());
#elif defined(Q_OS_WIN32)
    const QString appData = qEnvironmentVariable("APPDATA");
    return appData.isEmpty() ? QString() : qsl("%1/Claude").arg(appData);
#else
    // GenericConfigLocation honours XDG_CONFIG_HOME the way Claude Desktop's Linux
    // builds do. On macOS it would give ~/Library/Preferences - hence the branch above.
    return qsl("%1/Claude").arg(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
#endif
}

QString TMCPBridge::claudeDesktopConfigFilePath()
{
    const QString dir = claudeDesktopConfigDir();
    return dir.isEmpty() ? QString() : qsl("%1/claude_desktop_config.json").arg(dir);
}

QString TMCPBridge::mudletBinaryPath()
{
    // Inside an AppImage, applicationFilePath() names the transient mount the squashfs
    // was unpacked to, gone by the time Claude Desktop would run it. The AppImage
    // runtime exports the image's own path instead.
    const QString appImage = qEnvironmentVariable("APPIMAGE");
    if (!appImage.isEmpty()) {
        return appImage;
    }
    return QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
}

QJsonObject TMCPBridge::claudeDesktopEntry(const QString& command)
{
    // Only command, args and env may ever appear here: Claude Desktop (as of 2026-08)
    // silently discards its entire mcpServers block - everyone's servers, not just
    // this one - when an entry carries a key it does not expect, such as url or type.
    QJsonObject entry;
    entry[qsl("command")] = command;
    entry[qsl("args")] = QJsonArray{qsl("--mcp-bridge")};
    const QString xdgConfigHome = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (!xdgConfigHome.isEmpty()) {
        // Claude Desktop launches the bridge directly, not through a login shell, so a
        // variable this Mudlet was started with would otherwise be absent and the
        // bridge would resolve a different Mudlet config dir than the running Mudlet
        // and never find the discovery file.
        QJsonObject env;
        env[qsl("XDG_CONFIG_HOME")] = xdgConfigHome;
        entry[qsl("env")] = env;
    }
    return entry;
}

QByteArray TMCPBridge::mergeClaudeDesktopConfig(const QByteArray& existingConfig, const QJsonObject& entry, bool& ok)
{
    ok = false;
    QJsonObject root;
    if (!existingConfig.trimmed().isEmpty()) {
        QJsonParseError parseError{};
        const QJsonDocument doc = QJsonDocument::fromJson(existingConfig, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            // A file that cannot be read back must not be written over: it may hold
            // someone's hand-edited configuration with one stray comma.
            return {};
        }
        root = doc.object();
    }
    const QJsonValue serversValue = root.value(qsl("mcpServers"));
    if (!serversValue.isUndefined() && !serversValue.isObject()) {
        // Whatever put a non-object there, overwriting it wholesale would throw away
        // something this code does not understand.
        return {};
    }
    QJsonObject servers = serversValue.toObject();

    // The fresh entry replaces the old one outright rather than being merged into it:
    // an unexpected key poisons the whole block (see claudeDesktopEntry), so anything
    // an older Mudlet or a hand edit left there is a liability, not a setting to keep.
    // The one exception is env, where the user's own variables survive - only
    // XDG_CONFIG_HOME is Mudlet's to set or clear.
    QJsonObject merged = entry;
    QJsonObject env = servers.value(qsl("mudlet")).toObject().value(qsl("env")).toObject();
    const QJsonObject freshEnv = entry.value(qsl("env")).toObject();
    if (freshEnv.contains(qsl("XDG_CONFIG_HOME"))) {
        env[qsl("XDG_CONFIG_HOME")] = freshEnv.value(qsl("XDG_CONFIG_HOME"));
    } else {
        env.remove(qsl("XDG_CONFIG_HOME"));
    }
    if (env.isEmpty()) {
        merged.remove(qsl("env"));
    } else {
        merged[qsl("env")] = env;
    }
    servers[qsl("mudlet")] = merged;
    root[qsl("mcpServers")] = servers;
    ok = true;
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

TMCPBridge::ConnectOutcome TMCPBridge::connectClaudeDesktop()
{
    const QString command = mudletBinaryPath();
    if (command.isEmpty() || !QFileInfo(command).isExecutable()) {
        // However it came about, a path Claude Desktop cannot run would register a
        // bridge that never starts.
        return ConnectOutcome::NoBinaryPath;
    }
    const QString dir = claudeDesktopConfigDir();
    if (dir.isEmpty() || !QFileInfo::exists(dir)) {
        // The folder appears on Claude Desktop's first run, so its absence means the
        // app is not installed or has never been opened; a config it will never read
        // would only mislead.
        return ConnectOutcome::NoClaudeDesktop;
    }
    const QString path = claudeDesktopConfigFilePath();
    QByteArray existing;
    if (QFile file(path); file.exists()) {
        if (!file.open(QIODevice::ReadOnly)) {
            return ConnectOutcome::ConfigUnreadable;
        }
        existing = file.readAll();
    }
    bool ok = false;
    const QByteArray merged = mergeClaudeDesktopConfig(existing, claudeDesktopEntry(command), ok);
    if (!ok) {
        return ConnectOutcome::ConfigUnreadable;
    }
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly)) {
        return ConnectOutcome::WriteFailed;
    }
    out.write(merged);
    return out.commit() ? ConnectOutcome::Written : ConnectOutcome::WriteFailed;
}

void TMCPBridge::refreshClaudeDesktopEntry()
{
    const QString path = claudeDesktopConfigFilePath();
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "TMCPBridge::refreshClaudeDesktopEntry() WARNING - could not read" << path << "to check whether its Mudlet entry is current";
        return;
    }
    const QByteArray raw = file.readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject()) {
        qWarning() << "TMCPBridge::refreshClaudeDesktopEntry() WARNING -" << path << "does not hold a JSON object, so its Mudlet entry cannot be checked";
        return;
    }
    const QJsonObject entry = doc.object().value(qsl("mcpServers")).toObject().value(qsl("mudlet")).toObject();
    if (entry.isEmpty()) {
        // The user never pressed the connect button, or took the entry out; either way
        // registering Mudlet with Claude Desktop stays their call, not a startup side
        // effect.
        return;
    }
    if (!entry.value(qsl("args")).toArray().contains(QJsonValue(qsl("--mcp-bridge")))) {
        // Repurposed for something hand-rolled; leave it be.
        return;
    }
    bool ok = false;
    const QByteArray merged = mergeClaudeDesktopConfig(raw, claudeDesktopEntry(mudletBinaryPath()), ok);
    if (!ok || QJsonDocument::fromJson(merged).object() == doc.object()) {
        // Nothing would change - rewriting anyway would churn the file's formatting on
        // every startup.
        return;
    }
    // The recorded path goes stale whenever the binary moves - a downloaded AppImage
    // lands under a new name every release - so an opted-in entry is quietly brought
    // back in line with the Mudlet that is actually running.
    if (const ConnectOutcome outcome = connectClaudeDesktop(); outcome != ConnectOutcome::Written) {
        qWarning() << "TMCPBridge::refreshClaudeDesktopEntry() WARNING - could not bring the Claude Desktop entry up to date, so `mudlet --mcp-bridge` may be registered under a stale path";
    }
}
