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

#include <TMCPBridge.h>
#include <TMCPServer.h>
#include <utils.h>

#include <QtTest/QtTest>

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTcpServer>
#include <QTemporaryDir>

#if !defined(Q_OS_WIN32)
#include <unistd.h>
#endif

namespace {
QJsonObject parse(const QByteArray& json)
{
    return QJsonDocument::fromJson(json).object();
}
} // namespace

class TMCPBridgeTest : public QObject
{
    Q_OBJECT

private slots:
    void discoveryRoundTrip();
    void discoveryFileIsPrivate();
    void discoveryMissingFile();
    void discoveryGarbage_data();
    void discoveryGarbage();
    void discoveryRemove();
    void discoveryStaleRemoval();
    void discoveryWriteFailure();

    void notificationIsSwallowed();
    void initializeAnsweredLocally();
    void initializeDefaults();
    void pingAnsweredLocally();
    void emptyListsAnsweredLocally_data();
    void emptyListsAnsweredLocally();
    void toolMethodsAreForwarded();

    void envelopeCarriesMeta();
    void envelopeKeepsForeignMeta();

    void entryHasOnlySafeKeys();
    void entryFollowsXdgConfigHome();
    void mergeIntoEmptyConfig();
    void mergeKeepsOtherServersAndKeys();
    void mergeReplacesOwnEntry();
    void mergeKeepsUserEnvVars();
    void mergeRefusesGarbage();
    void mergeRefusesNonObjectServers();

    void connectWithoutClaudeDir();
    void connectRefusesGarbageConfig();
    void refreshUntouchedWithoutOptIn();
    void refreshLeavesRepurposedEntry();
    void refreshRepointsStaleEntry();
    void refreshLeavesCurrentEntryAlone();

    void bridgeAnswersOverPipes();
    void bridgeReportsMudletGone();
    void wrongTokenSurfacesAsError();
};

void TMCPBridgeTest::discoveryRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(TMCPBridge::writeDiscoveryFile(dir.path(), 12345, qsl("sekrit")));
    const MCPDiscovery discovery = TMCPBridge::readDiscoveryFile(dir.path());
    QVERIFY(discovery.valid());
    QCOMPARE(discovery.port, quint16(12345));
    QCOMPARE(discovery.token, qsl("sekrit"));
}

void TMCPBridgeTest::discoveryFileIsPrivate()
{
#if defined(Q_OS_WIN32)
    QSKIP("POSIX permission bits do not carry the access control on Windows");
#endif
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(TMCPBridge::writeDiscoveryFile(dir.path(), 1, qsl("t")));
    const QFileDevice::Permissions permissions = QFileInfo(TMCPBridge::discoveryFilePath(dir.path())).permissions();
    QVERIFY2(!(permissions & (QFileDevice::ReadGroup | QFileDevice::ReadOther)), "the token in the discovery file must not be readable by other users");
}

void TMCPBridgeTest::discoveryMissingFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(!TMCPBridge::readDiscoveryFile(dir.path()).valid());
}

void TMCPBridgeTest::discoveryGarbage_data()
{
    QTest::addColumn<QByteArray>("contents");
    QTest::newRow("not json") << QByteArray("who goes there");
    QTest::newRow("not an object") << QByteArray("[1,2]");
    QTest::newRow("port missing") << QByteArray(R"({"token":"t"})");
    QTest::newRow("port zero") << QByteArray(R"({"port":0,"token":"t"})");
    QTest::newRow("port too big") << QByteArray(R"({"port":65536,"token":"t"})");
    QTest::newRow("token missing") << QByteArray(R"({"port":1})");
    QTest::newRow("token empty") << QByteArray(R"({"port":1,"token":""})");
}

void TMCPBridgeTest::discoveryGarbage()
{
    QFETCH(QByteArray, contents);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile file(TMCPBridge::discoveryFilePath(dir.path()));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(contents);
    file.close();
    QVERIFY(!TMCPBridge::readDiscoveryFile(dir.path()).valid());
}

void TMCPBridgeTest::discoveryRemove()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(TMCPBridge::writeDiscoveryFile(dir.path(), 1, qsl("t")));
    TMCPBridge::removeDiscoveryFile(dir.path());
    QVERIFY(!QFileInfo::exists(TMCPBridge::discoveryFilePath(dir.path())));
}

void TMCPBridgeTest::discoveryStaleRemoval()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = TMCPBridge::discoveryFilePath(dir.path());

    // Nothing answers on the named port, so the file is last session's leftovers.
    quint16 deadPort = 0;
    {
        QTcpServer placeholder;
        QVERIFY(placeholder.listen(QHostAddress::LocalHost, 0));
        deadPort = placeholder.serverPort();
    }
    QVERIFY(TMCPBridge::writeDiscoveryFile(dir.path(), deadPort, qsl("t")));
    TMCPBridge::removeDiscoveryFileIfStale(dir.path());
    QVERIFY(!QFileInfo::exists(path));

    // Something listening on the port may be the Mudlet instance the file describes.
    QTcpServer live;
    QVERIFY(live.listen(QHostAddress::LocalHost, 0));
    QVERIFY(TMCPBridge::writeDiscoveryFile(dir.path(), live.serverPort(), qsl("t")));
    TMCPBridge::removeDiscoveryFileIfStale(dir.path());
    QVERIFY(QFileInfo::exists(path));

    // A file no bridge could ever use points at nothing worth keeping.
    QFile garbage(path);
    QVERIFY(garbage.open(QIODevice::WriteOnly | QIODevice::Truncate));
    garbage.write("not json");
    garbage.close();
    TMCPBridge::removeDiscoveryFileIfStale(dir.path());
    QVERIFY(!QFileInfo::exists(path));
}

void TMCPBridgeTest::discoveryWriteFailure()
{
#if defined(Q_OS_WIN32)
    QSKIP("POSIX permission bits do not make a directory unwritable on Windows");
#else
    if (geteuid() == 0) {
        QSKIP("root writes into read-only directories regardless of their permissions");
    }
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QFile::setPermissions(dir.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner));
    QVERIFY(!TMCPBridge::writeDiscoveryFile(dir.path(), 1, qsl("t")));
    QVERIFY(QFile::setPermissions(dir.path(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));
#endif
}

void TMCPBridgeTest::notificationIsSwallowed()
{
    QJsonObject reply;
    QJsonObject capabilities;
    QVERIFY(TMCPBridge::replyLocally(parse(R"({"jsonrpc":"2.0","method":"notifications/initialized"})"), reply, capabilities));
    QVERIFY(reply.isEmpty());
}

void TMCPBridgeTest::initializeAnsweredLocally()
{
    QJsonObject reply;
    QJsonObject capabilities;
    const QJsonObject message = parse(R"({"jsonrpc":"2.0","id":1,"method":"initialize",
        "params":{"protocolVersion":"2025-03-26","capabilities":{"roots":{}},"clientInfo":{"name":"c","version":"1"}}})");
    QVERIFY(TMCPBridge::replyLocally(message, reply, capabilities));

    const QJsonObject result = reply.value(qsl("result")).toObject();
    // Not an echo of the client's proposal: agreeing to 2025-03-26 would promise its
    // batching, which the bridge does not speak. The client falls back or disconnects,
    // as the spec directs.
    QCOMPARE(result.value(qsl("protocolVersion")).toString(), qsl("2025-06-18"));
    QCOMPARE(result.value(qsl("serverInfo")).toObject().value(qsl("name")).toString(), qsl("Mudlet"));
    QCOMPARE(result.value(qsl("capabilities")).toObject().value(qsl("tools")).toObject().value(qsl("listChanged")).toBool(true), false);
    // What the client declared has to be remembered, to be repeated in every forward.
    QVERIFY(capabilities.contains(qsl("roots")));
}

void TMCPBridgeTest::initializeDefaults()
{
    QJsonObject reply;
    QJsonObject capabilities;
    capabilities[qsl("tools")] = QJsonObject();
    QVERIFY(TMCPBridge::replyLocally(parse(R"({"jsonrpc":"2.0","id":1,"method":"initialize"})"), reply, capabilities));
    QCOMPARE(reply.value(qsl("result")).toObject().value(qsl("protocolVersion")).toString(), qsl("2025-06-18"));
    // No capabilities offered, so the starting assumption stays.
    QVERIFY(capabilities.contains(qsl("tools")));
}

void TMCPBridgeTest::pingAnsweredLocally()
{
    QJsonObject reply;
    QJsonObject capabilities;
    QVERIFY(TMCPBridge::replyLocally(parse(R"({"jsonrpc":"2.0","id":7,"method":"ping"})"), reply, capabilities));
    QCOMPARE(reply.value(qsl("id")).toInt(), 7);
    QVERIFY(reply.value(qsl("result")).toObject().isEmpty());
    QVERIFY(!reply.contains(qsl("error")));
}

void TMCPBridgeTest::emptyListsAnsweredLocally_data()
{
    QTest::addColumn<QString>("method");
    QTest::addColumn<QString>("field");
    QTest::newRow("prompts") << qsl("prompts/list") << qsl("prompts");
    QTest::newRow("resources") << qsl("resources/list") << qsl("resources");
    QTest::newRow("resource templates") << qsl("resources/templates/list") << qsl("resourceTemplates");
}

void TMCPBridgeTest::emptyListsAnsweredLocally()
{
    QFETCH(QString, method);
    QFETCH(QString, field);
    QJsonObject message;
    message[qsl("jsonrpc")] = qsl("2.0");
    message[qsl("id")] = 3;
    message[qsl("method")] = method;
    QJsonObject reply;
    QJsonObject capabilities;
    QVERIFY(TMCPBridge::replyLocally(message, reply, capabilities));
    const QJsonValue list = reply.value(qsl("result")).toObject().value(field);
    QVERIFY(list.isArray());
    QVERIFY(list.toArray().isEmpty());
}

void TMCPBridgeTest::toolMethodsAreForwarded()
{
    QJsonObject reply;
    QJsonObject capabilities;
    QVERIFY(!TMCPBridge::replyLocally(parse(R"({"jsonrpc":"2.0","id":1,"method":"tools/list"})"), reply, capabilities));
    reply = QJsonObject();
    capabilities = QJsonObject();
    QVERIFY(!TMCPBridge::replyLocally(parse(R"({"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"run_lua"}})"), reply, capabilities));
}

void TMCPBridgeTest::envelopeCarriesMeta()
{
    QJsonObject capabilities;
    capabilities[qsl("tools")] = QJsonObject();
    const QJsonObject envelope = TMCPBridge::forwardEnvelope(parse(R"({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"run_lua","arguments":{"code":"return 1"}}})"), capabilities);

    const QJsonObject params = envelope.value(qsl("params")).toObject();
    QCOMPARE(params.value(qsl("name")).toString(), qsl("run_lua"));
    const QJsonObject meta = params.value(qsl("_meta")).toObject();
    QCOMPARE(meta.value(QString::fromLatin1(TMCPServer::META_PROTOCOL_VERSION)).toString(), QString::fromLatin1(TMCPServer::MCP_PROTOCOL_VERSION));
    QVERIFY(meta.value(QString::fromLatin1(TMCPServer::META_CLIENT_CAPABILITIES)).toObject().contains(qsl("tools")));
    QCOMPARE(envelope.value(qsl("id")).toInt(), 1);
    QCOMPARE(envelope.value(qsl("method")).toString(), qsl("tools/call"));
}

void TMCPBridgeTest::envelopeKeepsForeignMeta()
{
    const QJsonObject envelope = TMCPBridge::forwardEnvelope(parse(R"({"jsonrpc":"2.0","id":1,"method":"tools/list","params":{"_meta":{"traceId":"abc"}}})"), QJsonObject());
    const QJsonObject meta = envelope.value(qsl("params")).toObject().value(qsl("_meta")).toObject();
    QCOMPARE(meta.value(qsl("traceId")).toString(), qsl("abc"));
    QVERIFY(meta.contains(QString::fromLatin1(TMCPServer::META_PROTOCOL_VERSION)));
}

void TMCPBridgeTest::entryHasOnlySafeKeys()
{
    const QJsonObject entry = TMCPBridge::claudeDesktopEntry(qsl("/opt/mudlet"));
    QCOMPARE(entry.value(qsl("command")).toString(), qsl("/opt/mudlet"));
    const QJsonArray args = entry.value(qsl("args")).toArray();
    QCOMPARE(args.size(), 1);
    QCOMPARE(args.first().toString(), qsl("--mcp-bridge"));
    // Pins the constraint documented on claudeDesktopEntry in TMCPBridge.cpp: one
    // unexpected key and Claude Desktop throws away its whole mcpServers block.
    for (auto it = entry.constBegin(); it != entry.constEnd(); ++it) {
        QVERIFY2(it.key() == qsl("command") || it.key() == qsl("args") || it.key() == qsl("env"), qPrintable(qsl("unexpected key in the Claude Desktop entry: %1").arg(it.key())));
    }
}

void TMCPBridgeTest::entryFollowsXdgConfigHome()
{
    const QByteArray saved = qgetenv("XDG_CONFIG_HOME");

    qputenv("XDG_CONFIG_HOME", "/somewhere/config");
    const QJsonObject withXdg = TMCPBridge::claudeDesktopEntry(qsl("/opt/mudlet"));
    qunsetenv("XDG_CONFIG_HOME");
    const QJsonObject withoutXdg = TMCPBridge::claudeDesktopEntry(qsl("/opt/mudlet"));

    if (saved.isEmpty()) {
        qunsetenv("XDG_CONFIG_HOME");
    } else {
        qputenv("XDG_CONFIG_HOME", saved);
    }

    // Claude Desktop launches the bridge without the login shell's environment, so
    // the entry has to carry the variable itself - and only when it is actually set,
    // as an empty env object is one more thing to go wrong.
    QCOMPARE(withXdg.value(qsl("env")).toObject().value(qsl("XDG_CONFIG_HOME")).toString(), qsl("/somewhere/config"));
    QVERIFY(!withoutXdg.contains(qsl("env")));
}

void TMCPBridgeTest::mergeIntoEmptyConfig()
{
    bool ok = false;
    const QByteArray merged = TMCPBridge::mergeClaudeDesktopConfig(QByteArray(), TMCPBridge::claudeDesktopEntry(qsl("/opt/mudlet")), ok);
    QVERIFY(ok);
    const QJsonObject entry = parse(merged).value(qsl("mcpServers")).toObject().value(qsl("mudlet")).toObject();
    QCOMPARE(entry.value(qsl("command")).toString(), qsl("/opt/mudlet"));
}

void TMCPBridgeTest::mergeKeepsOtherServersAndKeys()
{
    const QByteArray existing = R"({"globalShortcut":"Ctrl+Space","mcpServers":{"filesystem":{"command":"npx","args":["fs"]}}})";
    bool ok = false;
    const QJsonObject merged = parse(TMCPBridge::mergeClaudeDesktopConfig(existing, TMCPBridge::claudeDesktopEntry(qsl("/opt/mudlet")), ok));
    QVERIFY(ok);
    QCOMPARE(merged.value(qsl("globalShortcut")).toString(), qsl("Ctrl+Space"));
    const QJsonObject servers = merged.value(qsl("mcpServers")).toObject();
    QCOMPARE(servers.value(qsl("filesystem")).toObject().value(qsl("command")).toString(), qsl("npx"));
    QVERIFY(servers.contains(qsl("mudlet")));
}

void TMCPBridgeTest::mergeReplacesOwnEntry()
{
    const QByteArray existing = R"({"mcpServers":{"mudlet":{"command":"/old/gone/mudlet","args":["--mcp-bridge"]}}})";
    bool ok = false;
    const QJsonObject merged = parse(TMCPBridge::mergeClaudeDesktopConfig(existing, TMCPBridge::claudeDesktopEntry(qsl("/new/mudlet")), ok));
    QVERIFY(ok);
    QCOMPARE(merged.value(qsl("mcpServers")).toObject().value(qsl("mudlet")).toObject().value(qsl("command")).toString(), qsl("/new/mudlet"));
}

void TMCPBridgeTest::mergeKeepsUserEnvVars()
{
    const QByteArray existing = R"({"mcpServers":{"mudlet":{"command":"/old/mudlet","args":["--mcp-bridge"],"env":{"MY_VAR":"kept","XDG_CONFIG_HOME":"/stale"},"type":"stdio"}}})";

    // Built by hand rather than through claudeDesktopEntry, so the case does not
    // depend on what XDG_CONFIG_HOME holds in the environment running the tests.
    QJsonObject freshEnv;
    freshEnv[qsl("XDG_CONFIG_HOME")] = qsl("/fresh");
    QJsonObject fresh;
    fresh[qsl("command")] = qsl("/new/mudlet");
    fresh[qsl("args")] = QJsonArray{qsl("--mcp-bridge")};
    fresh[qsl("env")] = freshEnv;

    bool ok = false;
    const QJsonObject entry = parse(TMCPBridge::mergeClaudeDesktopConfig(existing, fresh, ok)).value(qsl("mcpServers")).toObject().value(qsl("mudlet")).toObject();
    QVERIFY(ok);
    // The user's own variable survives, Mudlet's is refreshed, and the stray key
    // Claude Desktop would choke on is gone.
    QCOMPARE(entry.value(qsl("env")).toObject().value(qsl("MY_VAR")).toString(), qsl("kept"));
    QCOMPARE(entry.value(qsl("env")).toObject().value(qsl("XDG_CONFIG_HOME")).toString(), qsl("/fresh"));
    QVERIFY(!entry.contains(qsl("type")));

    // A fresh entry without env clears Mudlet's variable but not the user's.
    fresh.remove(qsl("env"));
    const QJsonObject cleared = parse(TMCPBridge::mergeClaudeDesktopConfig(existing, fresh, ok)).value(qsl("mcpServers")).toObject().value(qsl("mudlet")).toObject();
    QVERIFY(ok);
    QCOMPARE(cleared.value(qsl("env")).toObject().value(qsl("MY_VAR")).toString(), qsl("kept"));
    QVERIFY(!cleared.value(qsl("env")).toObject().contains(qsl("XDG_CONFIG_HOME")));
}

void TMCPBridgeTest::mergeRefusesGarbage()
{
    bool ok = true;
    const QByteArray merged = TMCPBridge::mergeClaudeDesktopConfig(QByteArray("{not json"), TMCPBridge::claudeDesktopEntry(qsl("/opt/mudlet")), ok);
    // A config that cannot be read back must never be overwritten.
    QVERIFY(!ok);
    QVERIFY(merged.isEmpty());
}

void TMCPBridgeTest::mergeRefusesNonObjectServers()
{
    bool ok = true;
    const QByteArray merged = TMCPBridge::mergeClaudeDesktopConfig(QByteArray(R"({"mcpServers":[1,2]})"), TMCPBridge::claudeDesktopEntry(qsl("/opt/mudlet")), ok);
    QVERIFY(!ok);
    QVERIFY(merged.isEmpty());
}

namespace {
// Points every path the Claude Desktop config functions resolve through - XDG on
// Linux, HOME on macOS, APPDATA on Windows - into a temp dir, so no test can touch
// the real user's Claude Desktop settings.
struct ScopedClaudeConfigDirRedirect
{
    QTemporaryDir tempDir;
    QByteArray savedXdg = qgetenv("XDG_CONFIG_HOME");
    QByteArray savedHome = qgetenv("HOME");
    QByteArray savedAppData = qgetenv("APPDATA");
    QByteArray savedAppImage = qgetenv("APPIMAGE");

    ScopedClaudeConfigDirRedirect()
    {
        qputenv("XDG_CONFIG_HOME", tempDir.path().toUtf8());
        qputenv("HOME", tempDir.path().toUtf8());
        qputenv("APPDATA", tempDir.path().toUtf8());
        // With it set, mudletBinaryPath would name the AppImage instead of this test
        // binary.
        qunsetenv("APPIMAGE");
    }

    ~ScopedClaudeConfigDirRedirect()
    {
        restore("XDG_CONFIG_HOME", savedXdg);
        restore("HOME", savedHome);
        restore("APPDATA", savedAppData);
        restore("APPIMAGE", savedAppImage);
    }

    static void restore(const char* name, const QByteArray& value)
    {
        if (value.isEmpty()) {
            qunsetenv(name);
        } else {
            qputenv(name, value);
        }
    }

    QString configFile() const { return TMCPBridge::claudeDesktopConfigFilePath(); }

    bool createClaudeDir() const { return QDir().mkpath(TMCPBridge::claudeDesktopConfigDir()); }

    bool writeConfig(const QByteArray& contents) const
    {
        QFile file(configFile());
        return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(contents) == contents.size();
    }

    QByteArray readConfig() const
    {
        QFile file(configFile());
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        return file.readAll();
    }
};
} // namespace

void TMCPBridgeTest::connectWithoutClaudeDir()
{
    ScopedClaudeConfigDirRedirect redirect;
    QVERIFY(redirect.tempDir.isValid());
    QCOMPARE(TMCPBridge::connectClaudeDesktop(), TMCPBridge::ConnectOutcome::NoClaudeDesktop);
    // Creating the folder would make the next attempt claim the app is installed.
    QVERIFY(!QFileInfo::exists(TMCPBridge::claudeDesktopConfigDir()));
}

void TMCPBridgeTest::connectRefusesGarbageConfig()
{
    ScopedClaudeConfigDirRedirect redirect;
    QVERIFY(redirect.tempDir.isValid());
    QVERIFY(redirect.createClaudeDir());
    QVERIFY(redirect.writeConfig("{broken"));
    QCOMPARE(TMCPBridge::connectClaudeDesktop(), TMCPBridge::ConnectOutcome::ConfigUnreadable);
    QCOMPARE(redirect.readConfig(), QByteArray("{broken"));
}

void TMCPBridgeTest::refreshUntouchedWithoutOptIn()
{
    ScopedClaudeConfigDirRedirect redirect;
    QVERIFY(redirect.tempDir.isValid());

    // No config file at all: refresh must not conjure one up.
    TMCPBridge::refreshClaudeDesktopEntry();
    QVERIFY(!QFileInfo::exists(redirect.configFile()));

    // A config without a mudlet entry belongs to the user's other servers alone.
    QVERIFY(redirect.createClaudeDir());
    const QByteArray original = R"({"mcpServers":{"filesystem":{"command":"npx"}}})";
    QVERIFY(redirect.writeConfig(original));
    TMCPBridge::refreshClaudeDesktopEntry();
    QCOMPARE(redirect.readConfig(), original);
}

void TMCPBridgeTest::refreshLeavesRepurposedEntry()
{
    ScopedClaudeConfigDirRedirect redirect;
    QVERIFY(redirect.tempDir.isValid());
    QVERIFY(redirect.createClaudeDir());
    const QByteArray original = R"({"mcpServers":{"mudlet":{"command":"/opt/something-else","args":["--other"]}}})";
    QVERIFY(redirect.writeConfig(original));
    TMCPBridge::refreshClaudeDesktopEntry();
    QCOMPARE(redirect.readConfig(), original);
}

void TMCPBridgeTest::refreshRepointsStaleEntry()
{
    ScopedClaudeConfigDirRedirect redirect;
    QVERIFY(redirect.tempDir.isValid());
    QVERIFY(redirect.createClaudeDir());
    QVERIFY(redirect.writeConfig(R"({"mcpServers":{"filesystem":{"command":"npx"},"mudlet":{"command":"/old/gone/mudlet","args":["--mcp-bridge"],"env":{"MY_VAR":"kept"}}}})"));
    TMCPBridge::refreshClaudeDesktopEntry();
    const QJsonObject servers = parse(redirect.readConfig()).value(qsl("mcpServers")).toObject();
    QCOMPARE(servers.value(qsl("mudlet")).toObject().value(qsl("command")).toString(), TMCPBridge::mudletBinaryPath());
    QCOMPARE(servers.value(qsl("mudlet")).toObject().value(qsl("env")).toObject().value(qsl("MY_VAR")).toString(), qsl("kept"));
    QCOMPARE(servers.value(qsl("filesystem")).toObject().value(qsl("command")).toString(), qsl("npx"));
}

void TMCPBridgeTest::refreshLeavesCurrentEntryAlone()
{
    ScopedClaudeConfigDirRedirect redirect;
    QVERIFY(redirect.tempDir.isValid());
    QVERIFY(redirect.createClaudeDir());
    bool ok = false;
    const QByteArray current = TMCPBridge::mergeClaudeDesktopConfig(QByteArray(), TMCPBridge::claudeDesktopEntry(TMCPBridge::mudletBinaryPath()), ok);
    QVERIFY(ok);
    QVERIFY(redirect.writeConfig(current));
    TMCPBridge::refreshClaudeDesktopEntry();
    // Byte-identical: a semantic no-op must not churn the file on every startup.
    QCOMPARE(redirect.readConfig(), current);
}

// The process cases below run the real `mudlet --mcp-bridge`, so they also cover
// main.cpp's argv handling and config dir resolution. HOME and XDG_CONFIG_HOME each
// point into the temp dir; either alone isolates the config lookup (the profiles/ dir
// gives the XDG root the tiebreak in utils::xdgConfigDir), but with both redirected no
// resolution path can reach the user's real configuration.
namespace {
struct BridgeProcess
{
    QTemporaryDir tempDir;
    QProcess process;

    QString configDir() const { return qsl("%1/config/mudlet").arg(tempDir.path()); }

    bool start()
    {
        if (!tempDir.isValid() || !QDir().mkpath(qsl("%1/profiles").arg(configDir())) || !QDir().mkpath(qsl("%1/home").arg(tempDir.path()))) {
            return false;
        }
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(qsl("XDG_CONFIG_HOME"), qsl("%1/config").arg(tempDir.path()));
        env.insert(qsl("HOME"), qsl("%1/home").arg(tempDir.path()));
        env.remove(qsl("APPIMAGE"));
        process.setProcessEnvironment(env);
        process.start(qsl(MUDLET_BINARY_PATH), {qsl("--mcp-bridge")});
        return process.waitForStarted(10000);
    }

    // qWait rather than waitForReadyRead: an in-process TMCPServer answers on this
    // test's own event loop, which waitForReadyRead would not run - the server would
    // never see the request and both sides would sit waiting on each other.
    QJsonObject request(const QByteArray& line)
    {
        process.write(line + '\n');
        QElapsedTimer timer;
        timer.start();
        while (!process.canReadLine() && timer.elapsed() < 15000) {
            QTest::qWait(25);
        }
        if (!process.canReadLine()) {
            // Distinguishes a hung bridge from an empty reply in the failure output.
            return QJsonObject{{qsl("bridgeTestError"), qsl("timed out waiting for a reply")}};
        }
        return QJsonDocument::fromJson(process.readLine()).object();
    }
};
} // namespace

void TMCPBridgeTest::bridgeAnswersOverPipes()
{
    TMCPServer server;
    const MCPStartResult start = server.startServer(0);
    QVERIFY2(start.started, qPrintable(start.error));

    BridgeProcess bridge;
    QVERIFY(bridge.start());
    QVERIFY(TMCPBridge::writeDiscoveryFile(bridge.configDir(), server.getPort(), server.authToken()));

    const QJsonObject initReply =
            bridge.request(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{"tools":{}},"clientInfo":{"name":"test","version":"0"}}})");
    QCOMPARE(initReply.value(qsl("id")).toInt(), 1);
    QCOMPARE(initReply.value(qsl("result")).toObject().value(qsl("serverInfo")).toObject().value(qsl("name")).toString(), qsl("Mudlet"));

    // No reply may come back for this; the next request's answer proves it did not.
    bridge.process.write("{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}\n");

    const QJsonObject toolsReply = bridge.request(R"({"jsonrpc":"2.0","id":2,"method":"tools/list"})");
    QCOMPARE(toolsReply.value(qsl("id")).toInt(), 2);
    QVERIFY2(!toolsReply.contains(qsl("error")), qPrintable(QString::fromUtf8(QJsonDocument(toolsReply).toJson())));
    // A populated list proves the whole HTTP leg: token auth, _meta injection and the
    // mirror headers all passed the real server's checks.
    QVERIFY(!toolsReply.value(qsl("result")).toObject().value(qsl("tools")).toArray().isEmpty());

    // A line that never parses gets JSON-RPC's prescribed null-id parse error.
    const QJsonObject parseErrorReply = bridge.request("who goes there");
    QVERIFY(parseErrorReply.value(qsl("id")).isNull());
    QCOMPARE(parseErrorReply.value(qsl("error")).toObject().value(qsl("code")).toInt(), -32700);

    // Batching was retired by the revision the bridge pins, so an array is refused.
    const QJsonObject batchReply = bridge.request("[1,2,3]");
    QCOMPARE(batchReply.value(qsl("error")).toObject().value(qsl("code")).toInt(), -32600);

    // A blank line is keepalive noise, not a message; the next reply proves it was
    // skipped rather than answered.
    bridge.process.write("\n");

    // An "Unknown tool" error can only come from TMCPServer's own dispatch, which runs
    // after the Mcp-Name mirror header arrived intact - a missing header fails earlier
    // with a different message.
    const QJsonObject unknownTool = bridge.request(R"({"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"no_such_tool","arguments":{}}})");
    QCOMPARE(unknownTool.value(qsl("id")).toInt(), 3);
    QVERIFY2(unknownTool.value(qsl("error")).toObject().value(qsl("message")).toString().contains(qsl("Unknown tool")), qPrintable(QString::fromUtf8(QJsonDocument(unknownTool).toJson())));

    bridge.process.closeWriteChannel();
    QVERIFY(bridge.process.waitForFinished(10000));
    QCOMPARE(bridge.process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(bridge.process.exitCode(), 0);
}

void TMCPBridgeTest::bridgeReportsMudletGone()
{
    BridgeProcess bridge;
    QVERIFY(bridge.start());
    // No discovery file: the answer has to say Mudlet is not there, not hang or die.
    const QJsonObject reply = bridge.request(R"({"jsonrpc":"2.0","id":9,"method":"tools/list"})");
    QCOMPARE(reply.value(qsl("id")).toInt(), 9);
    QVERIFY(reply.value(qsl("error")).toObject().value(qsl("message")).toString().contains(qsl("Is Mudlet running")));

    // A discovery file naming a port nothing listens on: the connection failure has to
    // come back under the request's own id - and getting any discovery-based answer at
    // all here proves the file is re-read per request, not once at startup.
    quint16 deadPort = 0;
    {
        QTcpServer placeholder;
        QVERIFY(placeholder.listen(QHostAddress::LocalHost, 0));
        deadPort = placeholder.serverPort();
    }
    QVERIFY(TMCPBridge::writeDiscoveryFile(bridge.configDir(), deadPort, qsl("t")));
    const QJsonObject refused = bridge.request(R"({"jsonrpc":"2.0","id":10,"method":"tools/list"})");
    QCOMPARE(refused.value(qsl("id")).toInt(), 10);
    QVERIFY2(refused.value(qsl("error")).toObject().value(qsl("message")).toString().contains(qsl("Cannot reach Mudlet:")), qPrintable(QString::fromUtf8(QJsonDocument(refused).toJson())));

    bridge.process.closeWriteChannel();
    QVERIFY(bridge.process.waitForFinished(10000));
    QCOMPARE(bridge.process.exitCode(), 0);
}

void TMCPBridgeTest::wrongTokenSurfacesAsError()
{
    TMCPServer server;
    const MCPStartResult start = server.startServer(0);
    QVERIFY2(start.started, qPrintable(start.error));

    BridgeProcess bridge;
    QVERIFY(bridge.start());
    QVERIFY(TMCPBridge::writeDiscoveryFile(bridge.configDir(), server.getPort(), qsl("wrong-token")));

    // The server rejects a bad token before reading the body, so its reply carries a
    // null id; the bridge has to restore the request's own id or a stdio client can
    // never match the reply and waits forever.
    const QJsonObject reply = bridge.request(R"({"jsonrpc":"2.0","id":4,"method":"tools/list"})");
    QCOMPARE(reply.value(qsl("id")).toInt(), 4);
    QVERIFY2(reply.contains(qsl("error")), qPrintable(QString::fromUtf8(QJsonDocument(reply).toJson())));

    bridge.process.closeWriteChannel();
    QVERIFY(bridge.process.waitForFinished(10000));
    QCOMPARE(bridge.process.exitCode(), 0);
}

QTEST_MAIN(TMCPBridgeTest) // NOLINT(misc-use-anonymous-namespace)
#include "TMCPBridgeTest.moc"
