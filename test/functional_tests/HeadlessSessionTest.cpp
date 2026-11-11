/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

/*
 * THeadlessSession is what a frontend with no window - the mudlet-headless
 * runner, a phone app - drives a profile through. Each test makes a session of
 * its own, and with it the process's one profile pool, so none of them shares a
 * profile with another.
 *
 * Run with: ctest -R HeadlessSessionTest -V
 */

#include <QDataStream>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QRawFont>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QtTest/QtTest>

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#if defined(INCLUDE_OWN_QT6_KEYCHAIN)
#include <qtkeychain/keychain.h>
#else
#include <qt6keychain/keychain.h>
#endif
#include <zip.h>

#include "CredentialManager.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletMedia.h"
#include "MudletReplay.h"
#include "PortableModeTestHelper.h"
#include "RefusedPort.h"
#include "TAppFrontend.h"
#include "TConsoleModel.h"
#include "TDebug.h"
#include "TGameDetails.h"
#include "THeadlessSession.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TriggerUnit.h"

#include "GroupedTest.h"

class HeadlessSessionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeFile(const QString& path, const QByteArray& contents)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }

    static QByteArray packageXml(const QByteArray& script)
    {
        return R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>fromPackage</name><packageName></packageName><script>)"
               + script + R"(</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)";
    }

    static bool writeArchive(const QString& path, const QList<std::pair<QString, QByteArray>>& entries)
    {
        int errorCode = 0;
        zip* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
        if (!archive) {
            return false;
        }
        for (const auto& [name, contents] : entries) {
            zip_source* source = zip_source_buffer(archive, contents.constData(), contents.size(), 0);
            if (!source || zip_file_add(archive, name.toUtf8().constData(), source, ZIP_FL_ENC_UTF_8) < 0) {
                zip_source_free(source);
                zip_discard(archive);
                return false;
            }
        }
        return zip_close(archive) == 0;
    }

    static bool writeProfile(const QString& name, const QString& script, const QString& hostXml = QString())
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host>%2</Host></HostPackage>
<ScriptPackage><Script isActive="yes" isFolder="no"><name>headlessSession</name><packageName></packageName><script>%1</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)")
                               .arg(script.toHtmlEscaped(), hostXml)
                               .toUtf8());
        return true;
    }

    // A save that breaks off after a script that opens the other profile
    static bool writeOpenerThatFailsToLoad(const QString& name, const QString& other)
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        return QDir().mkpath(saveFolder)
               && writeFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder),
                            qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>opener</name><packageName></packageName><script>loadProfile("%1", true)</script><eventHandlerList/></Script></ScriptPackage>
<HostPackage><Host)")
                                    .arg(other)
                                    .toUtf8());
    }

    // Opened binary so Windows writes no \r
    static QString recordingScript(const QString& recordPath)
    {
        return qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  local file = io.open([[%1]], "ab")
  file:write("sysExitEvent\n")
  file:close()
end))lua")
                .arg(recordPath);
    }

    static QString readFile(const QString& path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    }

    // Each one the map load tries and turns down runs the event loop
    static bool writeUnreadableMaps(const QString& name, const int count)
    {
        const QString mapFolder = MudletApp::getMudletPath(enums::profileMapsPath, name);
        if (!QDir().mkpath(mapFolder)) {
            return false;
        }
        for (int map = 0; map < count; ++map) {
            QFile mapFile(qsl("%1/2020-01-0%2#00-00-00map.dat").arg(mapFolder).arg(map + 1));
            if (!mapFile.open(QIODevice::WriteOnly) || mapFile.write(QByteArray(64, 'x')) != 64) {
                return false;
            }
        }
        return true;
    }

    static QMap<QString, QByteArray> filesUnder(const QString& dir)
    {
        QMap<QString, QByteArray> files;
        QDirIterator it(dir, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            QFile file(path);
            files.insert(QDir(dir).relativeFilePath(path), file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray());
        }
        return files;
    }

    static bool storePassword(const QString& profile, const QString& key, const QString& password)
    {
        CredentialManager store;
        std::optional<bool> stored;
        store.storePassword(profile, key, password, [&stored](const bool success, const QString&) {
            stored = success;
        });
        return QTest::qWaitFor(
                       [&stored]() {
                           return stored.has_value();
                       },
                       10000)
               && *stored;
    }

    // Empty for none, or one that could not be read
    static QString retrievePassword(const QString& profile, const QString& key)
    {
        CredentialManager store;
        std::optional<QString> found;
        store.retrievePassword(profile, key, [&found](const bool success, const QString& password, const QString&) {
            found = success ? password : QString();
        });
        return QTest::qWaitFor(
                       [&found]() {
                           return found.has_value();
                       },
                       10000)
                       ? *found
                       : qsl("(no answer)");
    }

    // Read into a profile of a pool of its own, so the pool under test is left as it was
    static bool newestMapHoldsRooms(const QString& name, const QList<int>& roomIds)
    {
        const QDir folder(MudletApp::getMudletPath(enums::profileMapsPath, name));
        const QStringList saves = folder.entryList(QStringList{qsl("????-??-??#??-??-??map.dat")}, QDir::Files, QDir::Name | QDir::Reversed);
        HostManager pool;
        const QString readerName = qsl("Headless Map Reader");
        if (saves.isEmpty() || !pool.addHost(readerName, QString(), QString(), QString())) {
            return false;
        }
        TMap* map = pool.getHost(readerName)->mpMap.data();
        if (!map->restore(folder.filePath(saves.first()))) {
            return false;
        }
        return std::all_of(roomIds.cbegin(), roomIds.cend(), [map](const int roomId) {
            return map->mpRoomDB->getRoom(roomId) != nullptr;
        });
    }

    static qsizetype saveCount(const QString& name) { return QDir(MudletApp::getMudletPath(enums::profileXmlFilesPath, name)).entryList(QStringList{qsl("*.xml")}, QDir::Files).size(); }

    // pumpEvents() and waitForEvent() answer only in test mode
    static auto inTestMode()
    {
        const bool wasSet = qEnvironmentVariableIsSet("MUDLET_TEST_MODE");
        const QByteArray value = qgetenv("MUDLET_TEST_MODE");
        qputenv("MUDLET_TEST_MODE", "1");
        return qScopeGuard([wasSet, value]() {
            if (wasSet) {
                qputenv("MUDLET_TEST_MODE", value);
            } else {
                qunsetenv("MUDLET_TEST_MODE");
            }
        });
    }

    static QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    // Lua that feeds an OSC 8 link the game asks to show 0.2s later. Not a raw string: moc drops the class
    // over one holding these escapes.
    static QString feedDelayedLink(const QString& text)
    {
        const QString esc = QChar(0x1b);
        const QString link = qsl("%1]8;;send:look?config={\"visibility\":{\"action\":\"reveal\",\"delay\":200}}%1\\%2%1]8;;%1\\").arg(esc, text);
        return qsl("feedTriggers([==[(%1)\n]==])").arg(link);
    }

    // IAC SB MSSP MSSP_VAR "TLS" MSSP_VAL <port> IAC SE
    static QByteArray msspTlsPort(const QByteArray& port) { return QByteArray("\xff\xfa\x46\x01TLS\x02") + port + QByteArray("\xff\xf0"); }

    static QStringList outputTexts(const QSignalSpy& spy)
    {
        QStringList texts;
        for (const auto& arguments : spy) {
            texts << arguments.at(1).toString();
        }
        return texts;
    }

    // A replay file of chunks, each with the gap before it in milliseconds
    static bool writeReplay(const QString& path, const QList<std::pair<qint32, QByteArray>>& chunks)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        QDataStream stream(&file);
        stream.setVersion(QDataStream::Qt_5_12);
        for (const auto& [gap, bytes] : chunks) {
            stream << gap << static_cast<qint32>(bytes.size());
            stream.writeRawData(bytes.constData(), static_cast<int>(bytes.size()));
        }
        return stream.status() == QDataStream::Ok;
    }

    // Draws lineShown and localText as docs/headless.md tells an app to: a line of lineShown goes below a
    // line localText left open, and the line feed that starts the next piece then ends that line
    struct RebuiltConsole
    {
        QStringList lines;
        QStringList localPieces;
        bool localOpen = false;
        bool lineFeedOwed = false;
        bool ruleBroken = false;
        void shown(const QString& text)
        {
            if (localOpen) {
                localOpen = false;
                lineFeedOwed = true;
            }
            lines << text.split(QChar::LineFeed);
        }
        void local(QString text)
        {
            localPieces << text;
            if (std::exchange(lineFeedOwed, false)) {
                ruleBroken = ruleBroken || !text.startsWith(QChar::LineFeed);
                text.remove(0, 1);
            }
            if (text.isEmpty()) {
                return;
            }
            const QStringList parts = text.split(QChar::LineFeed);
            for (qsizetype i = 0; i < parts.size(); ++i) {
                if (i == parts.size() - 1 && parts.at(i).isEmpty()) {
                    break;
                }
                if (i == 0 && localOpen) {
                    lines.last().append(parts.at(i));
                } else {
                    lines << parts.at(i);
                }
            }
            localOpen = !text.endsWith(QChar::LineFeed);
        }
    };

    static void rebuildConsole(THeadlessSession& session, RebuiltConsole& console)
    {
        QObject::connect(&session, &THeadlessSession::lineShown, &session, [&console](const QString&, const QString& text) {
            console.shown(text);
        });
        QObject::connect(&session, &THeadlessSession::localText, &session, [&console](const QString&, const QString& text) {
            console.local(text);
        });
    }

    // The main console's lines from firstLine on, each wrapped line put back on the one it was split from
    static QStringList consoleLines(const TBuffer& buffer, const int firstLine)
    {
        QStringList lines;
        for (int y = firstLine; y < buffer.lineBuffer.size(); ++y) {
            const QString& text = buffer.lineBuffer.at(y);
            if (buffer.wrapsFromPreviousLine(y) && !lines.isEmpty()) {
                lines.last().append(QString(buffer.wrapGapBefore(y), QChar::Space) + text.mid(buffer.wrapIndentWidth(y)));
            } else {
                lines << text;
            }
        }
        while (!lines.isEmpty() && lines.last().isEmpty()) {
            lines.removeLast();
        }
        return lines;
    }

    // Stands in for the keychain of every credential store made until the guard it returns goes, answering
    // from the event loop: reads with readError and removals with removeError. The count is of the jobs seen.
    static auto answerTheKeychain(const QKeychain::Error readError, const QKeychain::Error removeError, const QString& message, std::shared_ptr<int> jobs)
    {
        const QByteArray testMode = qgetenv("MUDLET_TEST_MODE");
        // In test mode the credential store never asks the keychain
        qunsetenv("MUDLET_TEST_MODE");
        CredentialManager::jobStartHookForTesting() = [readError, removeError, message, jobs](QKeychain::Job* job) {
            ++*jobs;
            const QKeychain::Error error = qobject_cast<QKeychain::DeletePasswordJob*>(job) ? removeError : readError;
            QTimer::singleShot(0, job, [job, error, message]() {
                job->emitFinishedWithError(error, message);
            });
            return false;
        };
        return qScopeGuard([testMode]() {
            CredentialManager::jobStartHookForTesting() = nullptr;
            CredentialManager::forgetStoreRefusal();
            qputenv("MUDLET_TEST_MODE", testMode);
        });
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        QVERIFY2(!HostManager::self(), "A profile pool already exists, so a session cannot make its own.");
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    // A name the frontend got wrong can be asked for again
    void test_unknownProfileIsRefused()
    {
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("No Such Headless Profile"), .connect = false}), THeadlessSession::StartResult::noSuchProfile);
        QVERIFY(session.errorString().contains(qsl("No Such Headless Profile")));
        QVERIFY(!session.host());
        QVERIFY(!session.submit(qsl("look")));
        QVERIFY(!session.runLua(qsl("x = 1")));
        QCOMPARE(session.errorString(), qsl("No profile is open."));
        QVERIFY(!HostManager::self());
        QVERIFY(writeProfile(qsl("Headless Second Try"), QString()));
        QCOMPARE(session.start({.profile = qsl("Headless Second Try"), .connect = false}), THeadlessSession::StartResult::started);
    }

    // Mudlet.ini is read by the first pool alone, so the media mutes go from each pool to the next, as through
    // a name the frontend got wrong
    void test_theMediaMutesOutliveThePool()
    {
        const QString name = qsl("Headless Muted");
        QVERIFY(writeProfile(name, QString()));
        {
            const HostManager firstPool;
        }
        const auto unmute = qScopeGuard([]() {
            HostManager pool;
            pool.media().setApiMuted(false);
            pool.media().setGameMuted(false);
        });
        {
            HostManager pool;
            pool.media().setApiMuted(true);
            pool.media().setGameMuted(true);
        }
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Muted Typo"), .connect = false}), THeadlessSession::StartResult::noSuchProfile);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("headlessMutes = tostring(getConfig('muteMediaAPI')) .. ' ' .. tostring(getConfig('muteMediaGame'))")));
        QCOMPARE(luaGlobalString(session.host(), "headlessMutes"), qsl("true true"));
        QVERIFY(session.shutdown(false));
    }

    void test_waitsRunWithNoMainWindow()
    {
        const auto restore = inTestMode();
        const QString name = qsl("Headless Waits");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"(
local pumped, pumpError = pumpEvents(10)
tempTimer(0, function() raiseEvent("headlessWait", 7) end)
local event, arg = waitForEvent("headlessWait", 5000)
headlessWaits = tostring(pumped) .. " " .. tostring(pumpError) .. " " .. tostring(event) .. " " .. tostring(arg)
)")));
        QCOMPARE(luaGlobalString(session.host(), "headlessWaits"), qsl("true nil headlessWait 7"));
        QVERIFY(session.shutdown(false));
    }

    // As in the GUI, where the main window has gone by then: a wait there would pump a pool part-way destroyed
    void test_aWaitInAnExitHandlerIsToldOfTheShutdown()
    {
        const auto restore = inTestMode();
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString resultPath = dir.filePath(qsl("exit.txt"));
        const QString name = qsl("Headless Exit Wait");
        QVERIFY(writeProfile(name,
                             qsl(R"(
registerAnonymousEventHandler("sysExitEvent", function()
  local ok, err = pumpEvents(3000)
  local file = io.open([[%1]], "w")
  file:write(tostring(ok) .. " " .. tostring(err))
  file:close()
end)
)")
                                     .arg(resultPath)));
        QElapsedTimer elapsed;
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
            elapsed.start();
            QVERIFY(session.shutdown(false));
        }
        QCOMPARE(readFile(resultPath), qsl("nil pumpEvents: stopped early, Mudlet is shutting down"));
        QVERIFY2(elapsed.elapsed() < 2500, qPrintable(QString::number(elapsed.elapsed())));
    }

    void test_profileNameIsMatchedIgnoringCase()
    {
        QVERIFY(writeProfile(qsl("Headless Case"), QString()));
        THeadlessSession session;
        QSignalSpy loaded(&session, &THeadlessSession::profileLoaded);
        QCOMPARE(session.start({.profile = qsl("headless case"), .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(loaded.count(), 1);
        QCOMPARE(loaded.at(0).at(0).toString(), qsl("Headless Case"));
        QVERIFY(session.host());
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");
    }

    // As the dialog trims what is typed; with a host given, an untrimmed name would make a profile of its own
    void test_profileNameAndLoginAreTrimmed()
    {
        QVERIFY(writeProfile(qsl("Headless Trimmed"), QString()));
        const QStringList profilesBefore = QDir(MudletApp::getMudletPath(enums::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl(" headless trimmed "), .connect = false, .host = qsl("game.example"), .login = qsl(" bob ")}), THeadlessSession::StartResult::started);
        QCOMPARE(session.host()->getName(), qsl("Headless Trimmed"));
        QCOMPARE(QDir(MudletApp::getMudletPath(enums::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot), profilesBefore);
        QCOMPARE(MudletApp::readProfileData(qsl("Headless Trimmed"), qsl("login")), qsl("bob"));
        QCOMPARE(session.host()->getLogin(), qsl("bob"));
    }

    void test_aFirstLoadInstallsTheDefaultPackages_data()
    {
        QTest::addColumn<QString>("save");
        QTest::newRow("no save") << QString();
        QTest::newRow("settings copied") << qsl("Copied profile (settings only).xml");
    }

    // As the GUI's first load of a profile does; a later load leaves them be
    void test_aFirstLoadInstallsTheDefaultPackages()
    {
        QFETCH(QString, save);
        const QString name = save.isEmpty() ? qsl("Headless Never Saved") : qsl("Headless Settings Copied");
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        QVERIFY(QDir().mkpath(saveFolder));
        if (!save.isEmpty()) {
            QFile copied(qsl("%1/%2").arg(saveFolder, save));
            QVERIFY(copied.open(QIODevice::WriteOnly));
            copied.write(R"(<?xml version="1.0" encoding="UTF-8"?><!DOCTYPE MudletPackage><MudletPackage version="1.001"><HostPackage><Host></Host></HostPackage></MudletPackage>)");
        }
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.host());
        const QStringList installed = session.host()->mInstalledPackages;
        QVERIFY2(installed.contains(qsl("echo")) && installed.contains(qsl("generic_mapper")) && installed.contains(qsl("mudlet-base-ui")), qPrintable(installed.join(QLatin1Char(' '))));
        // One the player took out stays out
        QVERIFY(session.host()->uninstallPackage(qsl("echo"), enums::PackageModuleType::Package));
        QStringList kept = installed;
        kept.removeAll(qsl("echo"));
        QVERIFY(session.shutdown(true));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(session.host()->mInstalledPackages, kept);
    }

    // What the connection dialog stores as it is typed into, and Host reads as it is made; a new profile is
    // made for a host, its TLS box stored as it stands, an existing one keeps what is not given
    void test_connectionDetailsAreStored()
    {
        const QString name = qsl("Headless Details");
        QVERIFY(!QDir(MudletApp::getMudletPath(enums::profileHomePath, name)).exists());
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false, .host = qsl("game.example"), .port = 4000, .tls = true, .login = qsl("bob"), .password = qsl("hunter2")}),
                 THeadlessSession::StartResult::started);
        QCOMPARE(MudletApp::readProfileData(name, qsl("url")), qsl("game.example"));
        QCOMPARE(MudletApp::readProfileData(name, qsl("port")), qsl("4000"));
        QCOMPARE(MudletApp::readProfileData(name, qsl("ssl_tsl")), QString::number(Qt::Checked));
        QCOMPARE(MudletApp::readProfileData(name, qsl("login")), qsl("bob"));
        QVERIFY2(!QFileInfo::exists(MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("password"))), "The password for the run was stored.");
        QCOMPARE(session.host()->getUrl(), qsl("game.example"));
        QCOMPARE(session.host()->getPort(), 4000);
        QVERIFY(session.host()->mSslTsl);
        QCOMPARE(session.host()->getLogin(), qsl("bob"));
        QCOMPARE(session.host()->getPass(), qsl("hunter2"));
        QVERIFY(session.shutdown(true));

        QCOMPARE(session.start({.profile = name, .connect = false, .port = 4001}), THeadlessSession::StartResult::started);
        QCOMPARE(MudletApp::readProfileData(name, qsl("url")), qsl("game.example"));
        QCOMPARE(MudletApp::readProfileData(name, qsl("port")), qsl("4001"));
        QCOMPARE(MudletApp::readProfileData(name, qsl("ssl_tsl")), QString::number(Qt::Checked));
        QCOMPARE(session.host()->getLogin(), qsl("bob"));
        // The store is read before the empty password is believed, as the profile's own read of it is in flight
        CredentialManager store;
        std::optional<QString> storedPassword;
        store.retrievePassword(name, qsl("character"), [&storedPassword](const bool success, const QString& password, const QString&) {
            storedPassword = success ? password : QString();
        });
        QTRY_VERIFY_WITH_TIMEOUT(storedPassword.has_value(), 10000);
        QVERIFY2(storedPassword->isEmpty(), "The password for the run was stored.");
        QVERIFY(session.host()->getPass().isEmpty());
        QVERIFY(session.shutdown(true));
        QCOMPARE(session.start({.profile = name, .connect = false, .tls = false}), THeadlessSession::StartResult::started);
        QCOMPARE(MudletApp::readProfileData(name, qsl("ssl_tsl")), QString::number(Qt::Unchecked));
        QVERIFY(!session.host()->mSslTsl);
        QVERIFY(session.shutdown(true));
        QCOMPARE(session.start({.profile = name, .connect = false, .port = 70000}), THeadlessSession::StartResult::invalidConnectionDetails);
        QVERIFY2(session.errorString().contains(qsl("70000")), qPrintable(session.errorString()));
        QCOMPARE(MudletApp::readProfileData(name, qsl("port")), qsl("4001"));

        const QString plain = qsl("Headless Plain Details");
        QCOMPARE(session.start({.profile = plain, .connect = false, .host = qsl(" game.example ")}), THeadlessSession::StartResult::started);
        QCOMPARE(MudletApp::readProfileData(plain, qsl("url")), qsl("game.example"));
        QCOMPARE(MudletApp::readProfileData(plain, qsl("ssl_tsl")), QString::number(Qt::Unchecked));
        QVERIFY(!session.host()->mSslTsl);
        QCOMPARE(session.host()->getPort(), 23);
        QVERIFY(session.shutdown(true));

        // A predefined game's load sets its own address and TLS over the stored ones; what is given wins, as
        // the dialog's fields do, and its TLS box stands as the dialog ticks it for the game. The load's scripts
        // already see the given address, as the dialog's do.
        const bool gameTls = TGameDetails::findGame(qsl("Achaea"))->tlsEnabled;
        QCOMPARE(session.start({.profile = qsl("achaea"), .connect = false, .host = qsl("localhost"), .port = 4000}), THeadlessSession::StartResult::started);
        QCOMPARE(session.host()->getName(), qsl("Achaea"));
        QCOMPARE(session.host()->getUrl(), qsl("localhost"));
        QCOMPARE(session.host()->getPort(), 4000);
        QCOMPARE(session.host()->mSslTsl, gameTls);
        QCOMPARE(MudletApp::readProfileData(qsl("Achaea"), qsl("ssl_tsl")), QString::number(gameTls ? Qt::Checked : Qt::Unchecked));
        // Its packages chosen for the game, not for the address given
        QVERIFY2(session.host()->mInstalledPackages.contains(qsl("mudlet-mapper")) && !session.host()->mInstalledPackages.contains(qsl("generic_mapper")),
                 qPrintable(session.host()->mInstalledPackages.join(QLatin1Char(' '))));
        QVERIFY(session.shutdown(true));
        // Given nothing, what is stored is used over the game's own, as the dialog fills its fields with it
        QCOMPARE(session.start({.profile = qsl("Achaea"), .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(session.host()->getUrl(), qsl("localhost"));
        QCOMPARE(session.host()->getPort(), 4000);
        QCOMPARE(session.host()->mSslTsl, gameTls);
        QVERIFY(session.shutdown(true));
        QVERIFY(MudletApp::writeProfileData(qsl("Achaea"), qsl("url"), qsl("elsewhere.example")).first);
        QVERIFY(MudletApp::writeProfileData(qsl("Achaea"), qsl("ssl_tsl"), QString::number(gameTls ? Qt::Unchecked : Qt::Checked)).first);
        QCOMPARE(session.start({.profile = qsl("Achaea"), .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(session.host()->getUrl(), qsl("elsewhere.example"));
        QCOMPARE(session.host()->getPort(), 4000);
        QCOMPARE(session.host()->mSslTsl, !gameTls);
        QVERIFY(session.shutdown(true));
        QCOMPARE(session.start({.profile = qsl("Achaea"), .connect = false, .host = qsl("localhost"), .tls = gameTls}), THeadlessSession::StartResult::started);
        QCOMPARE(session.host()->getUrl(), qsl("localhost"));
        QCOMPARE(session.host()->mSslTsl, gameTls);
        QVERIFY(session.shutdown(true));

        // One that uses TLS, made new, has its box ticked as the dialog ticks it
        QVERIFY(TGameDetails::findGame(qsl("StickMUD"))->tlsEnabled);
        QCOMPARE(session.start({.profile = qsl("stickmud"), .connect = false, .host = qsl("localhost")}), THeadlessSession::StartResult::started);
        QVERIFY(session.host()->mSslTsl);
        QCOMPARE(MudletApp::readProfileData(qsl("StickMUD"), qsl("ssl_tsl")), QString::number(Qt::Checked));
        QVERIFY(session.shutdown(true));
        // A box given with no port is kept for the next run, as the dialog stores the port it shows with it
        QCOMPARE(session.start({.profile = qsl("StickMUD"), .connect = false, .tls = false}), THeadlessSession::StartResult::started);
        QVERIFY(!session.host()->mSslTsl);
        QVERIFY(session.shutdown(true));
        QCOMPARE(session.start({.profile = qsl("StickMUD"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(!session.host()->mSslTsl);
        QCOMPARE(session.host()->getPort(), TGameDetails::findGame(qsl("StickMUD"))->port);
        QVERIFY(session.shutdown(true));
        // Given only a port, too, as the dialog's box stands for the game while the port is changed
        QVERIFY(TGameDetails::findGame(qsl("Icesus"))->tlsEnabled);
        QCOMPARE(session.start({.profile = qsl("icesus"), .connect = false, .port = 4000}), THeadlessSession::StartResult::started);
        QCOMPARE(MudletApp::readProfileData(qsl("Icesus"), qsl("port")), qsl("4000"));
        QCOMPARE(MudletApp::readProfileData(qsl("Icesus"), qsl("ssl_tsl")), QString::number(Qt::Checked));
        QVERIFY(session.shutdown(true));
        // The box stands for the game while no port is stored, as the dialog's does, though a load came first
        QVERIFY(TGameDetails::findGame(qsl("MorgenGrauen"))->tlsEnabled);
        QVERIFY(writeProfile(qsl("MorgenGrauen"), QString()));
        QCOMPARE(session.start({.profile = qsl("MorgenGrauen"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.host()->mSslTsl);
        QVERIFY(session.shutdown(true));
        QCOMPARE(session.start({.profile = qsl("MorgenGrauen"), .connect = false, .host = qsl("localhost"), .port = 4000}), THeadlessSession::StartResult::started);
        QVERIFY(session.host()->mSslTsl);
        QCOMPARE(MudletApp::readProfileData(qsl("MorgenGrauen"), qsl("ssl_tsl")), QString::number(Qt::Checked));
        QVERIFY(session.shutdown(true));
        // Over the save's own, as the dialog puts its box over what the load set
        QVERIFY(TGameDetails::findGame(qsl("Accursed Lands"))->tlsEnabled);
        QVERIFY(writeProfile(qsl("Accursed Lands"), QString()));
        QCOMPARE(session.start({.profile = qsl("Accursed Lands"), .connect = false}), THeadlessSession::StartResult::started);
        session.host()->mSslTsl = false;
        QVERIFY(session.shutdown(true));
        QCOMPARE(session.start({.profile = qsl("Accursed Lands"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.host()->mSslTsl);
        QVERIFY(session.shutdown(true));
        // A save's scripts read the given address as the load runs them, not the game's own
        QVERIFY(writeProfile(qsl("Aardwolf"), qsl("headlessLoadHost = select(1, getConnectionInfo())")));
        QCOMPARE(session.start({.profile = qsl("aardwolf"), .connect = false, .host = qsl("localhost")}), THeadlessSession::StartResult::started);
        QCOMPARE(luaGlobalString(session.host(), "headlessLoadHost"), qsl("localhost"));
        QVERIFY(session.shutdown(true));

        const QString scripted = qsl("Headless Load Sees Details");
        QVERIFY(writeProfile(scripted, qsl("headlessLoadHost = select(1, getConnectionInfo())")));
        QCOMPARE(session.start({.profile = scripted, .connect = false, .host = qsl("game.example")}), THeadlessSession::StartResult::started);
        QCOMPARE(luaGlobalString(session.host(), "headlessLoadHost"), qsl("game.example"));
    }

    void test_aNewProfileNeedsAUsableName_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("host");
        QTest::addColumn<int>("result");
        QTest::newRow("no host") << qsl("Headless Unmade") << QString() << static_cast<int>(THeadlessSession::StartResult::noSuchProfile);
        QTest::newRow("a host of only spaces") << qsl("Headless Unmade") << qsl("  ") << static_cast<int>(THeadlessSession::StartResult::noSuchProfile);
        QTest::newRow("a character the dialog refuses") << qsl("Headless/Unmade") << qsl("game.example") << static_cast<int>(THeadlessSession::StartResult::invalidName);
        QTest::newRow("the profiles folder itself") << qsl(".") << qsl("game.example") << static_cast<int>(THeadlessSession::StartResult::invalidName);
    }

    void test_aNewProfileNeedsAUsableName()
    {
        QFETCH(QString, name);
        QFETCH(QString, host);
        QFETCH(int, result);
        const QStringList profilesBefore = QDir(MudletApp::getMudletPath(enums::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        THeadlessSession session;
        QCOMPARE(static_cast<int>(session.start({.profile = name, .connect = false, .host = host})), result);
        QVERIFY2(session.errorString().contains(name), qPrintable(session.errorString()));
        QVERIFY(!HostManager::self());
        QCOMPARE(QDir(MudletApp::getMudletPath(enums::profilesPath)).entryList(QDir::Dirs | QDir::NoDotAndDotDot), profilesBefore);
    }

    void test_detailsTheDialogRefusesAreRefused_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("host");
        QTest::addColumn<bool>("tls");
        QTest::addColumn<QString>("storedHost");
        QTest::addColumn<int>("port");
        QTest::addColumn<QString>("reason");
        QTest::newRow("not a host name") << qsl("Headless Bad Host") << qsl("bad host") << false << QString() << 0 << qsl("not the address of a game server");
        QTest::newRow("TLS to an IPv4 address") << qsl("Headless TLS IPv4") << qsl("127.0.0.1") << true << QString() << 0 << qsl("not an IP address");
        QTest::newRow("TLS to an IPv6 address") << qsl("Headless TLS IPv6") << qsl("::1") << true << QString() << 0 << qsl("not an IP address");
        QTest::newRow("TLS to the address stored") << qsl("Headless TLS Stored IP") << QString() << true << qsl("127.0.0.1") << 0 << qsl("not an IP address");
        QTest::newRow("a port above 65535") << qsl("Headless Port Too High") << qsl("game.example") << false << QString() << 70000 << qsl("1 to 65535");
        QTest::newRow("a port below 1") << qsl("Headless Port Too Low") << qsl("game.example") << false << QString() << -1 << qsl("1 to 65535");
    }

    // As the dialog checks what is typed into it, before anything is made or stored
    void test_detailsTheDialogRefusesAreRefused()
    {
        QFETCH(QString, name);
        QFETCH(QString, host);
        QFETCH(bool, tls);
        QFETCH(QString, storedHost);
        QFETCH(int, port);
        QFETCH(QString, reason);
        if (!storedHost.isEmpty()) {
            QVERIFY(writeProfile(name, QString()));
            QVERIFY(MudletApp::writeProfileData(name, qsl("url"), storedHost).first);
        }
        const QString profileFolder = MudletApp::getMudletPath(enums::profileHomePath, name);
        const QStringList filesBefore = QDir(profileFolder).entryList(QDir::Files);
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false, .host = host, .port = port, .tls = tls}), THeadlessSession::StartResult::invalidConnectionDetails);
        QVERIFY2(session.errorString().contains(reason), qPrintable(session.errorString()));
        QVERIFY(!HostManager::self());
        QCOMPARE(QDir(profileFolder).exists(), !storedHost.isEmpty());
        QCOMPARE(QDir(profileFolder).entryList(QDir::Files), filesBefore);
    }

    // A keychain entry left by a profile of the same name, deleted outside Mudlet, would be sent to the new game
    void test_aNewProfileTakesNoStalePassword()
    {
        const QString name = qsl("Headless Stale Secret");
        // The first pool of the process reads the setting from Mudlet.ini, over anything set before it
        {
            const HostManager firstPool;
        }
        const bool storedSecurely = MudletApp::storingPasswordsSecurely();
        MudletApp::setStorePasswordsSecurely(true);
        const auto restore = qScopeGuard([storedSecurely]() {
            MudletApp::setStorePasswordsSecurely(storedSecurely);
        });
        CredentialManager store;
        for (const QString& key : {qsl("character"), qsl("proxy")}) {
            std::optional<bool> stored;
            store.storePassword(name, key, qsl("stale"), [&stored](const bool success, const QString&) {
                stored = success;
            });
            QTRY_VERIFY_WITH_TIMEOUT(stored.has_value(), 10000);
            QVERIFY(*stored);
        }
        QVERIFY(!QDir(MudletApp::getMudletPath(enums::profileHomePath, name)).exists());

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false, .host = qsl("game.example")}), THeadlessSession::StartResult::started);
        for (const QString& key : {qsl("character"), qsl("proxy")}) {
            std::optional<QString> left;
            store.retrievePassword(name, key, [&left](const bool success, const QString& password, const QString&) {
                left = success ? password : QString();
            });
            QTRY_VERIFY_WITH_TIMEOUT(left.has_value(), 10000);
            QVERIFY2(left->isEmpty(), qPrintable(key));
        }
        QVERIFY(session.host()->getPass().isEmpty());
    }

    // The dialog clears only a name typed for a new profile; a game's, the keychain shares with every config root
    void test_aGameOpenedForTheFirstTimeKeepsItsStoredPassword()
    {
        const QString game = qsl("Slothmud");
        QVERIFY(!QDir(MudletApp::getMudletPath(enums::profileHomePath, game)).exists());
        // The first pool of the process reads the setting from Mudlet.ini, over anything set before it
        {
            const HostManager firstPool;
        }
        const bool storedSecurely = MudletApp::storingPasswordsSecurely();
        MudletApp::setStorePasswordsSecurely(true);
        const auto restore = qScopeGuard([storedSecurely]() {
            MudletApp::setStorePasswordsSecurely(storedSecurely);
        });
        CredentialManager store;
        std::optional<bool> stored;
        store.storePassword(game, qsl("character"), qsl("kept"), [&stored](const bool success, const QString&) {
            stored = success;
        });
        QTRY_VERIFY_WITH_TIMEOUT(stored.has_value(), 10000);
        QVERIFY(*stored);

        THeadlessSession session;
        QCOMPARE(session.start({.profile = game, .connect = false}), THeadlessSession::StartResult::started);
        std::optional<QString> left;
        store.retrievePassword(game, qsl("character"), [&left](const bool success, const QString& password, const QString&) {
            left = success ? password : QString();
        });
        QTRY_VERIFY_WITH_TIMEOUT(left.has_value(), 10000);
        QCOMPARE(*left, qsl("kept"));
    }

    // Sent to the new game, a password left behind is worse than no profile
    void test_aStalePasswordThatCannotBeRemovedIsNotOpened()
    {
        const QString name = qsl("Headless Stuck Secret");
        {
            const HostManager firstPool;
        }
        const bool storedSecurely = MudletApp::storingPasswordsSecurely();
        MudletApp::setStorePasswordsSecurely(true);
        const auto restore = qScopeGuard([storedSecurely, name]() {
            MudletApp::setStorePasswordsSecurely(storedSecurely);
            CredentialManager::removeCredential(name, qsl("character"));
        });
        QVERIFY(CredentialManager::storeCredential(name, qsl("character"), qsl("stale")));
        const auto jobs = std::make_shared<int>(0);
        const auto keychain = answerTheKeychain(QKeychain::EntryNotFound, QKeychain::AccessDenied, qsl("synthetic: not removed"), jobs);

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false, .host = qsl("game.example")}), THeadlessSession::StartResult::notOpened);
        QVERIFY2(session.errorString().contains(qsl("could not be removed")), qPrintable(session.errorString()));
        QVERIFY2(session.errorString().contains(qsl("synthetic: not removed")), qPrintable(session.errorString()));
        QVERIFY(!HostManager::self());
        QVERIFY(!QDir(MudletApp::getMudletPath(enums::profileHomePath, name)).exists());
    }

    // As the dialog: a keychain that cannot be asked, as with no D-Bus session, holds nothing to remove
    void test_aKeychainThatCannotBeReachedHoldsUpNoNewProfile()
    {
        const QString name = qsl("Headless Unreachable Secret");
        {
            const HostManager firstPool;
        }
        const bool storedSecurely = MudletApp::storingPasswordsSecurely();
        MudletApp::setStorePasswordsSecurely(true);
        const auto restore = qScopeGuard([storedSecurely]() {
            MudletApp::setStorePasswordsSecurely(storedSecurely);
        });
        const QByteArray testMode = qgetenv("MUDLET_TEST_MODE");
        const auto jobs = std::make_shared<int>(0);
        const auto keychain = answerTheKeychain(QKeychain::OtherError, QKeychain::OtherError, qsl("Cannot autolaunch D-Bus without X11 $DISPLAY"), jobs);
        // Once the proxy's lookup, the last, has begun, its stages are set, so the load is a test run's while a
        // removal of the character's before it would still have asked the keychain
        CredentialManager::jobStartHookForTesting() = [answer = CredentialManager::jobStartHookForTesting(), testMode](QKeychain::Job* job) {
            if (job->service().contains(qsl("-proxy-"))) {
                qputenv("MUDLET_TEST_MODE", testMode);
            }
            return answer(job);
        };

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false, .host = qsl("game.example")}), THeadlessSession::StartResult::started);
        QVERIFY2(qgetenv("MUDLET_TEST_MODE") == testMode, "The proxy's password was not looked up.");
        QVERIFY(session.shutdown(false));
    }

    // The keychain answers from the event loop, where a frontend's timer may run
    void test_shutdownWhileStalePasswordsAreRemovedIsRefused()
    {
        const QString name = qsl("Headless Secret Wait");
        {
            const HostManager firstPool;
        }
        const bool storedSecurely = MudletApp::storingPasswordsSecurely();
        MudletApp::setStorePasswordsSecurely(true);
        const auto restore = qScopeGuard([storedSecurely]() {
            MudletApp::setStorePasswordsSecurely(storedSecurely);
        });
        const QByteArray testMode = qgetenv("MUDLET_TEST_MODE");
        const auto jobs = std::make_shared<int>(0);
        const auto keychain = answerTheKeychain(QKeychain::EntryNotFound, QKeychain::NoError, QString(), jobs);

        THeadlessSession session;
        std::optional<bool> shutDown;
        QTimer::singleShot(0, &session, [&session, &shutDown, testMode]() {
            shutDown = session.shutdown(false);
            qputenv("MUDLET_TEST_MODE", testMode);
        });
        const THeadlessSession::StartResult result = session.start({.profile = name, .connect = false, .host = qsl("game.example")});
        QVERIFY2(shutDown.has_value(), "The credential store answered before the event loop ran.");
        QVERIFY(!*shutDown);
        QVERIFY(*jobs > 0);
        QCOMPARE(result, THeadlessSession::StartResult::started);
        QVERIFY(session.shutdown(false));
        QVERIFY(!HostManager::self());
    }

    // What the frontend does with its own copy while the keychain is asked is nothing to the run
    void test_thePasswordForTheRunIsTheOneGiven()
    {
        const QString name = qsl("Headless Given Secret Wait");
        {
            const HostManager firstPool;
        }
        const bool storedSecurely = MudletApp::storingPasswordsSecurely();
        MudletApp::setStorePasswordsSecurely(true);
        const auto restore = qScopeGuard([storedSecurely]() {
            MudletApp::setStorePasswordsSecurely(storedSecurely);
        });
        const QByteArray testMode = qgetenv("MUDLET_TEST_MODE");
        const auto jobs = std::make_shared<int>(0);
        const auto keychain = answerTheKeychain(QKeychain::EntryNotFound, QKeychain::NoError, QString(), jobs);

        THeadlessSession session;
        THeadlessSession::Options options{.profile = name, .connect = false, .host = qsl("game.example")};
        options.password = QString::fromLatin1("given");
        QTimer::singleShot(0, &session, [&options, testMode]() {
            options.password = QString::fromLatin1("changed");
            qputenv("MUDLET_TEST_MODE", testMode);
        });
        QCOMPARE(session.start(options), THeadlessSession::StartResult::started);
        QCOMPARE(options.password, qsl("changed"));
        QCOMPARE(session.host()->getPass(), qsl("given"));
        QVERIFY(session.shutdown(false));
    }

    // One a frontend gives with nothing in it is no password for the run, so the stored one is still taken
    void test_anEmptyPasswordForTheRunIsNone()
    {
        const QString name = qsl("Headless Empty Given Secret");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        session.host()->givePass(QString());
        session.host()->securedPasswordAnswered(true, qsl("stored"), QString(), false);
        QCOMPARE(session.host()->getPass(), qsl("stored"));
    }

    // The stored one answers from the credential store once the profile is made; one given for the run stands
    void test_aPasswordForTheRunOutlivesTheStoredOne()
    {
        const QString name = qsl("Headless Stored Secret");
        QVERIFY(writeProfile(name, QString()));
        // The first pool of the process reads the setting from Mudlet.ini, over anything set before it
        {
            const HostManager firstPool;
        }
        const bool storedSecurely = MudletApp::storingPasswordsSecurely();
        MudletApp::setStorePasswordsSecurely(true);
        const auto restore = qScopeGuard([storedSecurely]() {
            MudletApp::setStorePasswordsSecurely(storedSecurely);
        });
        CredentialManager store;
        std::optional<bool> stored;
        store.storePassword(name, qsl("character"), qsl("stored"), [&stored](const bool success, const QString&) {
            stored = success;
        });
        QTRY_VERIFY_WITH_TIMEOUT(stored.has_value(), 10000);
        QVERIFY(*stored);

        THeadlessSession session;
        THeadlessSession::Options options{.profile = name, .connect = false};
        options.password = QString::fromLatin1("given");
        QCOMPARE(session.start(options), THeadlessSession::StartResult::started);
        QCOMPARE(session.host()->getPass(), qsl("given"));
        // Its own copy, so that the caller's secureStringClear() wipes the caller's
        QVERIFY(session.host()->getPass().constData() != options.password.constData());
        // As the keychain answers once more after a deadline that the encrypted file's copy answered
        session.host()->securedPasswordAnswered(true, qsl("stored"), QString(), false);
        QCOMPARE(session.host()->getPass(), qsl("given"));
        QVERIFY(session.shutdown(false));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QTRY_COMPARE_WITH_TIMEOUT(session.host()->getPass(), qsl("stored"), 10000);
        // With none given for the run, the later answer is the one kept, as in the GUI
        session.host()->securedPasswordAnswered(true, qsl("from the keychain"), QString(), false);
        QCOMPARE(session.host()->getPass(), qsl("from the keychain"));
    }

    void test_aProfileWaitingForItsSaveIsKept_data()
    {
        QTest::addColumn<bool>("deleteSession");
        QTest::newRow("session deleted") << true;
        QTest::newRow("shut down") << false;
    }

    // installPackage() waits for a save under way, which runs the event loop, where the frontend's timer may
    // shut the session down or delete it; the profile's frame is still on the stack
    void test_aProfileWaitingForItsSaveIsKept()
    {
        QFETCH(bool, deleteSession);
        const QString name = deleteSession ? qsl("Headless Save Wait Deleted") : qsl("Headless Save Wait Shut Down");
        QVERIFY(writeProfile(name, QString()));
        const QString packagePath = mConfigDir.filePath(qsl("saveWait.xml"));
        QVERIFY(writeFile(packagePath, packageXml(R"(headlessSaveWait = "yes")")));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QPointer<Host> pHost(pSession->host());
        // Stands in for a save being written
        pHost->mWritingHostAndModules = true;
        std::optional<bool> keptThrough;
        bool shutDown = true;
        QTimer::singleShot(0, this, [pSession = QPointer(pSession), pHost, deleteSession, &keptThrough, &shutDown]() {
            if (deleteSession) {
                delete pSession.data();
            } else {
                shutDown = pSession->shutdown(false);
            }
            keptThrough = !pHost.isNull();
        });
        QTimer::singleShot(300, this, [pHost]() {
            if (pHost) {
                pHost->mWritingHostAndModules = false;
            }
        });
        const QPointer<THeadlessSession> pAlive(pSession);
        pSession->installPackage(packagePath);
        QVERIFY(keptThrough.has_value());
        QVERIFY2(*keptThrough, "The profile was deleted while it waited for its save.");
        if (!deleteSession) {
            QVERIFY(!shutDown);
            QVERIFY2(pSession->errorString().contains(qsl("waiting for its save")), qPrintable(pSession->errorString()));
        }
        delete pAlive.data();
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
    }

    void test_installPackage()
    {
        const QString name = qsl("Headless Package");
        QVERIFY(writeProfile(name, QString()));
        const QString packagePath = mConfigDir.filePath(qsl("headlessPackage.xml"));
        QVERIFY(writeFile(packagePath, packageXml(R"(headlessPackageRan = "yes")")));
        THeadlessSession session;
        QCOMPARE(session.installPackage(packagePath), THeadlessSession::PackageResult::failed);
        QCOMPARE(session.errorString(), qsl("No profile is open."));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY2(session.installPackage(packagePath) == THeadlessSession::PackageResult::installed, qPrintable(session.errorString()));
        QVERIFY(session.host()->mInstalledPackages.contains(qsl("headlessPackage")));
        QCOMPARE(luaGlobalString(session.host(), "headlessPackageRan"), qsl("yes"));
        const QString missing = mConfigDir.filePath(qsl("missing.mpackage"));
        QCOMPARE(session.installPackage(missing), THeadlessSession::PackageResult::failed);
        QVERIFY2(session.errorString().contains(missing), qPrintable(session.errorString()));

        // Left as it is, and said so
        QCOMPARE(session.installPackage(packagePath), THeadlessSession::PackageResult::alreadyInstalled);
        QVERIFY2(session.errorString().contains(qsl("\"headlessPackage\" is already installed")), qPrintable(session.errorString()));
        // Though one of its name is installed, a path with no file at it is a mistake
        const QString mistyped = mConfigDir.filePath(qsl("elsewhere/headlessPackage.xml"));
        QCOMPARE(session.installPackage(mistyped), THeadlessSession::PackageResult::failed);
        QVERIFY2(session.errorString().contains(mistyped), qPrintable(session.errorString()));

        // One whose config.lua names it otherwise than its file is still found to be installed when given again
        const QString renamedPath = mConfigDir.filePath(qsl("HeadlessRenamed-1.2.mpackage"));
        QVERIFY(writeArchive(renamedPath,
                             {{qsl("config.lua"), R"(mpackage = "HeadlessRenamed"
version = "1.2")"},
                              {qsl("HeadlessRenamed.xml"), packageXml(R"(headlessRenamedRan = "yes")")}}));
        QVERIFY2(session.installPackage(renamedPath) == THeadlessSession::PackageResult::installed, qPrintable(session.errorString()));
        QVERIFY(session.host()->mInstalledPackages.contains(qsl("HeadlessRenamed")));
        QCOMPARE(session.installPackage(renamedPath), THeadlessSession::PackageResult::alreadyInstalled);
        QVERIFY2(session.errorString().contains(qsl("\"HeadlessRenamed\" 1.2 is already installed")), qPrintable(session.errorString()));

        // A Lua error in one of its items is told apart from an install that went well, though the package stays
        // for fixing; given again, it is left as it is, as how its install went is not known now
        const QString brokenPath = mConfigDir.filePath(qsl("headlessBroken.xml"));
        QVERIFY(writeFile(brokenPath, packageXml(R"(error("broken on purpose"))")));
        QCOMPARE(session.installPackage(brokenPath), THeadlessSession::PackageResult::installedWithErrors);
        QVERIFY2(session.errorString().contains(qsl("broken on purpose")), qPrintable(session.errorString()));
        QVERIFY(session.host()->mInstalledPackages.contains(qsl("headlessBroken")));
        QCOMPARE(session.installPackage(brokenPath), THeadlessSession::PackageResult::alreadyInstalled);

        // An XML that stops part-way keeps what was read before it, as in the GUI
        const QString partialPath = mConfigDir.filePath(qsl("headlessPartial.xml"));
        QVERIFY(writeFile(partialPath, packageXml(R"(headlessPartialRan = "yes")").replace("</ScriptPackage>", R"(<Script isActive="yes" isFolder="no"><name>cutOff</na)")));
        QCOMPARE(session.installPackage(partialPath), THeadlessSession::PackageResult::installedWithErrors);
        QVERIFY2(session.errorString().contains(qsl("not all of it could be read")), qPrintable(session.errorString()));
        QVERIFY(session.host()->mInstalledPackages.contains(qsl("headlessPartial")));
        QCOMPARE(luaGlobalString(session.host(), "headlessPartialRan"), qsl("yes"));

        // One whose config.lua cannot be read lands under its file's name
        const QString badConfigPath = mConfigDir.filePath(qsl("HeadlessBadConfig.mpackage"));
        QVERIFY(writeArchive(badConfigPath, {{qsl("config.lua"), "this is not Lua ("}, {qsl("HeadlessBadConfig.xml"), packageXml(R"(headlessBadConfigRan = "yes")")}}));
        QCOMPARE(session.installPackage(badConfigPath), THeadlessSession::PackageResult::installedWithErrors);
        QVERIFY2(session.errorString().contains(qsl("config.lua")), qPrintable(session.errorString()));
        QVERIFY(session.host()->mInstalledPackages.contains(qsl("HeadlessBadConfig")));
        QCOMPARE(luaGlobalString(session.host(), "headlessBadConfigRan"), qsl("yes"));
    }

    // As the GUI's event loop would run them before the next thing it does
    void test_installHandlersRunBeforeInstallPackageReturns()
    {
        const QString name = qsl("Headless Install Handlers");
        QVERIFY(writeProfile(name, QString()));
        const QString packagePath = mConfigDir.filePath(qsl("headlessHandled.xml"));
        QVERIFY(writeFile(packagePath, packageXml(R"(registerAnonymousEventHandler("sysInstallPackage", function(_, name) headlessHandled = name end))")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY2(session.installPackage(packagePath) == THeadlessSession::PackageResult::installed, qPrintable(session.errorString()));
        QCOMPARE(luaGlobalString(session.host(), "headlessHandled"), qsl("headlessHandled"));
    }

    // A module's XML that stops part-way is told in problems, as a package's is
    void test_aModuleReadPartWayIsAProblem_data()
    {
        QTest::addColumn<bool>("archived");
        QTest::newRow("an XML file") << false;
        QTest::newRow("an archive") << true;
    }

    void test_aModuleReadPartWayIsAProblem()
    {
        QFETCH(bool, archived);
        const QString name = archived ? qsl("Headless Partial Archived Module") : qsl("Headless Partial Module");
        QVERIFY(writeProfile(name, QString()));
        const QByteArray cutOff = packageXml(R"(headlessModuleRan = "yes")").replace("</ScriptPackage>", R"(<Script isActive="yes" isFolder="no"><name>cutOff</na)");
        const QString modulePath = mConfigDir.filePath(archived ? qsl("headlessPartialArchivedModule.mpackage") : qsl("headlessPartialModule.xml"));
        QVERIFY(archived ? writeArchive(modulePath, {{qsl("headlessPartialArchivedModule.xml"), cutOff}}) : writeFile(modulePath, cutOff));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QStringList problems;
        QVERIFY(session.host()->installPackage(modulePath, enums::PackageModuleType::ModuleFromScript, true, &problems).first);
        QVERIFY2(problems.size() == 1 && problems.constFirst().startsWith(qsl("not all of it could be read")), qPrintable(problems.join(qsl("; "))));
    }

    void test_aSecondPoolIsRefused()
    {
        QVERIFY(writeProfile(qsl("Headless Second Pool"), QString()));
        auto pool = std::make_unique<HostManager>();
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Second Pool"), .connect = false}), THeadlessSession::StartResult::notOpened);
        QVERIFY(!session.host());
        QVERIFY2(HostManager::self() == pool.get(), "The refused session took the pool it did not make.");
    }

    void test_unreadableSaveIsLoadFailed()
    {
        const QString name = qsl("Headless Corrupt");
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        QVERIFY(QDir().mkpath(saveFolder));
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        saveFile.write("<?xml version=\"1.0\"?><MudletPackage version=\"1.001\"><HostPackage><Host");
        saveFile.close();
        TelnetServerStub server;
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy loaded(&session, &THeadlessSession::profileLoaded);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::loadFailed);
        // The XML reader's own reason says nothing of which profile it is
        const QString prefix = qsl("The profile \"Headless Corrupt\" could not be loaded: ");
        QVERIFY2(session.errorString().startsWith(prefix) && session.errorString().size() > prefix.size(), qPrintable(session.errorString()));
        QCOMPARE(loaded.count(), 0);
        // The GUI connects such a profile too, but there the player sees why it has no scripts
        QTest::qWait(1000);
        QVERIFY2(!server.hasPendingConnections() && !server.clientConnected(), "A profile whose save did not load was connected to its game.");
        // Its scripts never started, and what it holds now would be saved over its last good save and map
        QVERIFY(!session.runLua(qsl("headlessCorruptRan = true")));
        QVERIFY2(session.errorString().contains(qsl("takes no commands")), qPrintable(session.errorString()));
        QVERIFY(!session.submit(qsl("look")));
        const QString mapsFolder = MudletApp::getMudletPath(enums::profileMapsPath, name);
        const QStringList mapsBefore = QDir(mapsFolder).entryList(QDir::Files);
        QVERIFY(session.shutdown(true));
        QCOMPARE(saveCount(name), 1);
        QCOMPARE(QDir(mapsFolder).entryList(QDir::Files), mapsBefore);

        // The same session, given a profile that loads, takes commands and saves it
        const QString good = qsl("Headless After Corrupt");
        QVERIFY(writeProfile(good, QString()));
        QCOMPARE(session.start({.profile = good, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY2(session.runLua(qsl("headlessAfterCorruptRan = true")), qPrintable(session.errorString()));
        QVERIFY(session.shutdown(true));
        QCOMPARE(saveCount(good), 2);
    }

    // The scripts read before the save broke off still run; what they open is no part of the failed load
    void test_aProfileThatAFailedLoadOpenedIsSaved()
    {
        const QString name = qsl("Headless Corrupt Opener");
        const QString other = qsl("Headless Opened By The Corrupt");
        QVERIFY(writeProfile(other, QString()));
        QVERIFY(writeOpenerThatFailsToLoad(name, other));

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::loadFailed);
        QVERIFY2(HostManager::self()->getHost(other), "The script read before the save broke off did not run.");
        QVERIFY(session.shutdown(true));
        QCOMPARE(saveCount(name), 1);
        QCOMPARE(saveCount(other), 2);
    }

    // A profile with room 9 in its last good map, which it closes as it loads by the given script
    static bool writeProfileClosedAsItLoads(const QString& name, const QString& script, QByteArray& mapBefore)
    {
        const QString mapsFolder = MudletApp::getMudletPath(enums::profileMapsPath, name);
        {
            THeadlessSession session;
            if (!writeProfile(name, QString()) || session.start({.profile = name, .connect = false}) != THeadlessSession::StartResult::started || !session.host()->mpMap->addRoom(9)
                || !session.host()->saveMapFile(qsl("%1/2020-01-01#00-00-00map.dat").arg(mapsFolder)) || !session.shutdown(false)) {
                return false;
            }
        }
        mapBefore = filesUnder(mapsFolder).value(qsl("2020-01-01#00-00-00map.dat"));
        return !mapBefore.isEmpty() && writeProfile(name, script);
    }

    // Its scripts start before its map is loaded, so what it held then would be written over its last good map
    void test_aProfileClosedBeforeItsMapLoadsKeepsItsSaveAndMap()
    {
        const QString name = qsl("Headless Closed While Loading");
        const QString runner = qsl("Headless Closes While Loading Runner");
        QByteArray mapBefore;
        QVERIFY(writeProfileClosedAsItLoads(name, qsl("closeProfile()"), mapBefore));
        QVERIFY(writeProfile(runner, QString()));
        const QString mapsFolder = MudletApp::getMudletPath(enums::profileMapsPath, name);

        THeadlessSession session;
        QCOMPARE(session.start({.profile = runner, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY2(session.runLua(qsl("loadProfile([[%1]], true)").arg(name)), qPrintable(session.errorString()));
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self()->getHost(name), 5000);
        QCOMPARE(saveCount(name), 1);
        QCOMPARE(QDir(mapsFolder).entryList(QDir::Files), QStringList{qsl("2020-01-01#00-00-00map.dat")});
        QCOMPARE(filesUnder(mapsFolder).value(qsl("2020-01-01#00-00-00map.dat")), mapBefore);
        QVERIFY2(!session.saveFailed(), qPrintable(session.unsavedProfiles().join(qsl(", "))));
        QVERIFY(session.shutdown(true));
        QVERIFY(!session.saveFailed());
    }

    // Its sysLoadEvent handlers run with its map loaded but before its load ends, when saveProfile() still refuses
    // it, so the close waits for the load to end, as closeMudlet()'s does, and then saves
    void test_aProfileClosedFromItsSysLoadEventIsSavedOnceLoaded()
    {
        const QString name = qsl("Headless Closed From Its sysLoadEvent");
        const QString runner = qsl("Headless Closes From A sysLoadEvent Runner");
        QByteArray mapBefore;
        QVERIFY(writeProfileClosedAsItLoads(name, qsl(R"lua(registerAnonymousEventHandler("sysLoadEvent", function() addRoom(12) closeProfile() end))lua"), mapBefore));
        QVERIFY(writeProfile(runner, QString()));
        const QString mapsFolder = MudletApp::getMudletPath(enums::profileMapsPath, name);

        THeadlessSession session;
        QCOMPARE(session.start({.profile = runner, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY2(session.runLua(qsl("loadProfile([[%1]], true)").arg(name)), qPrintable(session.errorString()));
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self()->getHost(name), 5000);
        QCOMPARE(saveCount(name), 2);
        QCOMPARE(QDir(mapsFolder).entryList(QDir::Files).size(), 2);
        QCOMPARE(filesUnder(mapsFolder).value(qsl("2020-01-01#00-00-00map.dat")), mapBefore);
        QVERIFY2(newestMapHoldsRooms(name, {9, 12}), "The close did not save the map the profile loaded, with the room its handler added.");
        QVERIFY2(!session.saveFailed(), qPrintable(session.unsavedProfiles().join(qsl(", "))));
        QVERIFY(session.shutdown(true));
        QVERIFY(!session.saveFailed());
    }

    // As shutdown(true) and save() leave such a profile: its last good save and map are kept, and that is no failure
    void test_aScriptOpenedProfileWhoseSaveDidNotLoadClosesUnsaved()
    {
        const QString name = qsl("Headless Closes A Corrupt");
        const QString corrupt = qsl("Headless Corrupt Closed By A Script");
        QVERIFY(writeProfile(name, QString()));
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, corrupt);
        QVERIFY(QDir().mkpath(saveFolder));
        QVERIFY(writeFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder), "<?xml version=\"1.0\"?><MudletPackage version=\"1.001\"><HostPackage><Host"));

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY2(session.runLua(qsl("loadProfile([[%1]], true)").arg(corrupt)), qPrintable(session.errorString()));
        const QPointer<Host> pCorrupt(HostManager::self()->getHost(corrupt));
        QVERIFY(pCorrupt);
        QVERIFY(!pCorrupt->mLoadedOk);
        pCorrupt->mpMap->addRoom(5);
        QCoreApplication::processEvents();
        const QString home = MudletApp::getMudletPath(enums::profileHomePath, corrupt);
        const QStringList filesBefore = filesUnder(home).keys();
        QSignalSpy closeSaveFailed(pCorrupt.data(), &Host::closeSaveFailed);

        QVERIFY2(session.runLua(qsl("assert(closeProfile([[%1]]))").arg(corrupt)), qPrintable(session.errorString()));
        QTRY_VERIFY_WITH_TIMEOUT(!pCorrupt, 5000);
        QCOMPARE(closeSaveFailed.count(), 0);
        QCOMPARE(filesUnder(home).keys(), filesBefore);
        QVERIFY2(!session.saveFailed(), qPrintable(session.unsavedProfiles().join(qsl(", "))));
        QVERIFY(session.shutdown(true));
        QVERIFY(!session.saveFailed());
    }

    void test_anUnwantedConnectIsCalledOff_data()
    {
        QTest::addColumn<bool>("partlyLoaded");
        QTest::addColumn<bool>("deleteSession");
        QTest::newRow("save partly loaded") << true << false;
        QTest::newRow("save partly loaded, session deleted during the load") << true << true;
        QTest::newRow("session deleted during the load") << false << true;
    }

    // Some of a save can load before the part that does not, and a sysLoadEvent handler there can run the
    // event loop until the connection is made: the GUI keeps it, but here it is dropped. A profile whose
    // session has gone is not connected at all.
    void test_anUnwantedConnectIsCalledOff()
    {
        QFETCH(const bool, partlyLoaded);
        QFETCH(const bool, deleteSession);
        const QString name = qsl("Headless Unwanted Connect %1%2").arg(partlyLoaded).arg(deleteSession);
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        QVERIFY(QDir().mkpath(saveFolder));
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        const QString script = qsl(R"lua(registerAnonymousEventHandler("sysLoadEvent", function()
  for i = 1, 1000 do
    if select(3, getConnectionInfo()) then
      headlessConnected = "yes"
      return
    end
    loadMap()
  end
end))lua");
        saveFile.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>headlessSession</name><packageName></packageName><script>%1</script><eventHandlerList/></Script></ScriptPackage>
%2)")
                               .arg(script.toHtmlEscaped(), partlyLoaded ? qsl("<HostPackage><Host") : qsl("<HostPackage><Host></Host></HostPackage></MudletPackage>"))
                               .toUtf8());
        saveFile.close();
        QVERIFY(writeUnreadableMaps(name, 1));
        TelnetServerStub server;
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QObject context;
        if (deleteSession) {
            QTimer::singleShot(0, &context, [pSession]() {
                delete pSession;
            });
        }
        const auto started = pSession->start({.profile = name, .connect = true});
        QCOMPARE(started, deleteSession ? THeadlessSession::StartResult::notOpened : THeadlessSession::StartResult::loadFailed);
        // A deleted session's pool goes from a timer, so its profile is still here
        const QPointer<Host> pHost(HostManager::self()->getHost(name));
        QVERIFY(pHost);
        QCOMPARE(luaGlobalString(pHost, "headlessConnected"), deleteSession ? QString() : qsl("yes"));
        QVERIFY2(!std::get<2>(pHost->mTelnet.getConnectionInfo()), "An unwanted connection was left up.");
        if (deleteSession) {
            QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
            return;
        }
        QTRY_VERIFY_WITH_TIMEOUT(!server.clientConnected(), 5000);
        QTest::qWait(500);
        QVERIFY(!server.clientConnected());
        pSession->shutdown(false);
        delete pSession;
    }

    void test_gameLinesReachOutputLine()
    {
        const QString name = qsl("Headless Game");
        QVERIFY(writeProfile(name, QString()));
        TelnetServerStub server;
        server.setWelcomeMessage(qsl("Welcome to the headless session test."));
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);

        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Welcome to the headless session test.")), 10000);
        const auto welcome = output.at(outputTexts(output).indexOf(qsl("Welcome to the headless session test.")));
        QCOMPARE(welcome.at(0).toString(), name);
        QCOMPARE(welcome.at(2).toBool(), false);
    }

    void test_outputLineCarriesTheLinesColoursAndStyles()
    {
        const QString name = qsl("Headless Styled");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        Host* pHost = session.host();
        QVERIFY(pHost);
        // Not a raw string: moc misreads this one and drops the class
        QVERIFY(session.runLua(qsl("feedTriggers('\\27[31mred\\27[0m \\27[1mbold\\27[0m \\27[4;7mrev\\27[0m \\27[38;2;1;2;3;48;2;4;5;6mtrue\\27[0m\\nplain\\n\\n\\27[1;31mbright\\27[0m "
                                   "\\27[1;38;2;1;2;3mdeep\\27[0m\\n\\27[38;5;9mwarn\\27[39m after\\27[0m \\27[1;38;5;1mdark\\27[0m\\n')")));
        const QStringList texts = outputTexts(output);
        const qsizetype styled = texts.indexOf(qsl("red bold rev true"));
        QVERIFY2(styled >= 0, qPrintable(texts.join(QChar::LineFeed)));
        QCOMPARE(texts.mid(styled), (QStringList{qsl("red bold rev true"), qsl("plain"), QString(), qsl("bright deep"), qsl("warn after dark")}));

        using Style = THeadlessSession::TextStyle;
        const auto run = [pHost](int start, int length, const QColor& foreground, const QColor& background, THeadlessSession::TextStyles styles) {
            return THeadlessSession::TextRun{.start = start,
                                             .length = length,
                                             .foreground = foreground,
                                             .background = background,
                                             .defaultForeground = foreground == pHost->mFgColor,
                                             .defaultBackground = background == pHost->mBgColor,
                                             .styles = styles};
        };
        const QColor fg = pHost->mFgColor;
        const QColor bg = pHost->mBgColor;
        const QList<THeadlessSession::TextRun> expected{run(0, 3, pHost->mRed, bg, {}),
                                                        run(3, 1, fg, bg, {}),
                                                        run(4, 4, fg, bg, Style::Bold),
                                                        run(8, 1, fg, bg, {}),
                                                        run(9, 3, fg, bg, Style::Underline | Style::Reverse),
                                                        run(12, 1, fg, bg, {}),
                                                        run(13, 4, QColor(1, 2, 3), QColor(4, 5, 6), {})};
        const auto runs = output.at(styled).at(3).value<QList<THeadlessSession::TextRun>>();
        QCOMPARE(runs.size(), expected.size());
        for (qsizetype i = 0; i < runs.size(); ++i) {
            QVERIFY2(runs.at(i) == expected.at(i),
                     qPrintable(qsl("run %1: start %2 length %3 fg %4 bg %5 styles %6")
                                        .arg(QString::number(i), QString::number(runs.at(i).start), QString::number(runs.at(i).length), runs.at(i).foreground.name(), runs.at(i).background.name())
                                        .arg(runs.at(i).styles.toInt())));
        }
        QVERIFY(!expected.at(0).defaultForeground);
        QVERIFY(expected.at(1).defaultForeground && expected.at(1).defaultBackground);
        QVERIFY(!expected.at(6).defaultForeground && !expected.at(6).defaultBackground);

        const auto plain = output.at(styled + 1).at(3).value<QList<THeadlessSession::TextRun>>();
        QCOMPARE(plain.size(), 1);
        QVERIFY(plain.constFirst() == run(0, 5, fg, bg, {}));
        QVERIFY(output.at(styled + 2).at(3).value<QList<THeadlessSession::TextRun>>().isEmpty());

        // Bold with a colour: painted as the bright colour, or for 24-bit colour not at all, but bold all the same
        const auto coloured = output.at(styled + 3).at(3).value<QList<THeadlessSession::TextRun>>();
        QCOMPARE(coloured.size(), 3);
        QVERIFY2(coloured.at(0) == run(0, 6, pHost->mLightRed, bg, Style::Bold), qPrintable(coloured.at(0).foreground.name() + QChar::Space + QString::number(coloured.at(0).styles.toInt())));
        QVERIFY(coloured.at(1) == run(6, 1, fg, bg, {}));
        QVERIFY2(coloured.at(2) == run(7, 4, QColor(1, 2, 3), bg, Style::Bold), qPrintable(coloured.at(2).foreground.name() + QChar::Space + QString::number(coloured.at(2).styles.toInt())));

        // A bright colour index is not bold, nor what follows it in the default colour, which the GUI paints in bold
        const auto indexed = output.at(styled + 4).at(3).value<QList<THeadlessSession::TextRun>>();
        QCOMPARE(indexed.size(), 3);
        QVERIFY(indexed.at(0) == run(0, 4, pHost->mLightRed, bg, {}));
        QVERIFY(indexed.at(1) == run(4, 7, fg, bg, {}));
        QVERIFY(indexed.at(2) == run(11, 4, pHost->mRed, bg, Style::Bold));
        QVERIFY(session.shutdown(false));
    }

    // Only the main window makes a ShortcutsManager, and the GUI would open a save that had none with the defaults
    void test_aSaveKeepsTheKeyboardShortcuts()
    {
        const QString name = qsl("Headless Shortcuts");
        const QString shortcuts = qsl(R"(<profileShortcut key="Show Map">Ctrl+Shift+M</profileShortcut><profileShortcut key="Notepad"></profileShortcut>)");
        QVERIFY(writeProfile(name, QString(), shortcuts));
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
            QVERIFY(session.shutdown(true));
        }
        QDir saves(MudletApp::getMudletPath(enums::profileXmlFilesPath, name));
        const QStringList written = saves.entryList(QStringList{qsl("*.xml")}, QDir::Files, QDir::Time);
        QCOMPARE(written.size(), 2);
        // Read as XML, as pugixml builds differ on whether an empty element is written <a></a> or <a />
        QStringList saved;
        QXmlStreamReader reader(readFile(saves.filePath(written.first())));
        while (!reader.atEnd()) {
            if (reader.readNext() == QXmlStreamReader::StartElement && reader.name() == u"profileShortcut") {
                const QString key = reader.attributes().value(u"key").toString();
                saved << qsl("%1=%2").arg(key, reader.readElementText());
            }
        }
        QVERIFY2(!reader.hasError(), qPrintable(reader.errorString()));
        QCOMPARE(saved, (QStringList{qsl("Notepad="), qsl("Show Map=Ctrl+Shift+M")}));
    }

    void test_aReplayPlaysWithNoMainWindow()
    {
        const QString name = qsl("Headless Replay");
        QVERIFY(writeProfile(name, QString()));
        const QString logDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name);
        QVERIFY(QDir().mkpath(logDir));
        {
            QFile file(qsl("%1/headless.dat").arg(logDir));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QDataStream stream(&file);
            stream.setVersion(QDataStream::Qt_5_12);
            const QByteArray bytes = QByteArrayLiteral("Replayed with no main window.\r\n");
            stream << qint32{0} << static_cast<qint32>(bytes.size());
            stream.writeRawData(bytes.constData(), static_cast<int>(bytes.size()));
            QCOMPARE(stream.status(), QDataStream::Ok);
        }

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("local ok, err = loadReplay('headless.dat') headlessReplay = tostring(ok) .. (err and ' ' .. err or '')")));
        QCOMPARE(luaGlobalString(session.host(), "headlessReplay"), qsl("true"));
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Replayed with no main window.")), 10000);
        QVERIFY(session.shutdown(false));
    }

    // As the GUI's replay toolbar follows it, for a replay a script starts too, which ends with nothing said
    void test_aReplayIsFollowed()
    {
        const QString name = qsl("Headless Replay Followed");
        QVERIFY(writeProfile(name, QString()));
        const QString logDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name);
        QVERIFY(QDir().mkpath(logDir));
        {
            QFile file(qsl("%1/two.dat").arg(logDir));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QDataStream stream(&file);
            stream.setVersion(QDataStream::Qt_5_12);
            for (const auto& [gap, bytes] : {std::pair{0, QByteArrayLiteral("First replayed line.\r\n")}, std::pair{1000, QByteArrayLiteral("Second replayed line.\r\n")}}) {
                stream << qint32{gap} << static_cast<qint32>(bytes.size());
                stream.writeRawData(bytes.constData(), static_cast<int>(bytes.size()));
            }
            QCOMPARE(stream.status(), QDataStream::Ok);
        }

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy started(&session, &THeadlessSession::replayStarted);
        QSignalSpy speed(&session, &THeadlessSession::replaySpeedChanged);
        QSignalSpy finished(&session, &THeadlessSession::replayFinished);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        // Another profile's is not this one's
        const QString other = qsl("Headless Replay Elsewhere");
        QVERIFY(writeProfile(other, QString()));
        const QString otherLogDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, other);
        QVERIFY(QDir().mkpath(otherLogDir));
        QVERIFY(QFile::copy(qsl("%1/two.dat").arg(logDir), qsl("%1/two.dat").arg(otherLogDir)));
        QVERIFY(session.runLua(qsl("loadProfile('%1', true)").arg(other)));
        Host* pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl("loadReplay('two.dat')")));
        QVERIFY(MudletReplay::self()->running());
        MudletReplay::self()->speedUp();
        QTRY_VERIFY_WITH_TIMEOUT(!MudletReplay::self()->running(), 10000);
        QTest::qWait(50);
        QVERIFY(started.isEmpty());
        QVERIFY(speed.isEmpty());
        QVERIFY(finished.isEmpty());

        // One that never plays sends neither
        QVERIFY(writeFile(qsl("%1/corrupt.dat").arg(logDir), QByteArrayLiteral("\0\0\0")));
        QVERIFY(session.runLua(qsl("corruptReplay = tostring(loadReplay('corrupt.dat'))")));
        QCOMPARE(luaGlobalString(session.host(), "corruptReplay"), qsl("nil"));
        QTest::qWait(50);
        QVERIFY(started.isEmpty());
        QVERIFY(finished.isEmpty());

        QVERIFY(session.runLua(qsl("headlessReplay = tostring(loadReplay('two.dat'))")));
        QCOMPARE(luaGlobalString(session.host(), "headlessReplay"), qsl("true"));
        QTRY_COMPARE(started.size(), 1);
        QCOMPARE(started.constFirst().at(0).toString(), name);
        QVERIFY(session.runLua(qsl("secondReplay = tostring(loadReplay('two.dat'))")));
        QCOMPARE(luaGlobalString(session.host(), "secondReplay"), qsl("nil"));
        MudletReplay::self()->speedUp();
        QTRY_COMPARE(speed.size(), 1);
        QCOMPARE(speed.constFirst().at(0).toString(), name);
        QCOMPARE(speed.constFirst().at(1).toInt(), 2);
        QVERIFY(finished.isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Second replayed line.")), 10000);
        QTRY_COMPARE(finished.size(), 1);
        QCOMPARE(finished.constFirst().at(0).toString(), name);
        QCOMPARE(started.size(), 1);

        // Ended by the shutdown that lets the profile go, and not again by the replay's own end
        QVERIFY(session.runLua(qsl("loadReplay('two.dat')")));
        QTRY_COMPARE(started.size(), 2);
        QVERIFY(MudletReplay::self()->running());
        QVERIFY(session.shutdown(false));
        QCOMPARE(finished.size(), 2);
        QTest::qWait(100);
        QCOMPARE(finished.size(), 2);
        QCOMPARE(speed.size(), 1);
    }

    // The corrupt one played nothing, so only the one after it has a start and an end, and that end is its own
    void test_aReplayLoadedInPlaceOfACorruptOneIsTheOnlyOneFollowed()
    {
        const QString name = qsl("Headless Replay Fallback");
        QVERIFY(writeProfile(name, QString()));
        const QString logDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name);
        QVERIFY(QDir().mkpath(logDir));
        QVERIFY(writeFile(qsl("%1/corrupt.dat").arg(logDir), QByteArrayLiteral("\0\0\0")));
        {
            QFile file(qsl("%1/two.dat").arg(logDir));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QDataStream stream(&file);
            stream.setVersion(QDataStream::Qt_5_12);
            for (const auto& [gap, bytes] : {std::pair{0, QByteArrayLiteral("First fallback line.\r\n")}, std::pair{500, QByteArrayLiteral("Last fallback line.\r\n")}}) {
                stream << qint32{gap} << static_cast<qint32>(bytes.size());
                stream.writeRawData(bytes.constData(), static_cast<int>(bytes.size()));
            }
            QCOMPARE(stream.status(), QDataStream::Ok);
        }

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QStringList heard;
        connect(&session, &THeadlessSession::replayStarted, &session, [&]() {
            heard << qsl("started");
        });
        connect(&session, &THeadlessSession::replayFinished, &session, [&]() {
            heard << (outputTexts(output).contains(qsl("Last fallback line.")) ? qsl("finished after its last line") : qsl("finished early"));
        });
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("if not loadReplay('corrupt.dat') then fallbackReplay = tostring(loadReplay('two.dat')) end")));
        QCOMPARE(luaGlobalString(session.host(), "fallbackReplay"), qsl("true"));
        QTRY_VERIFY_WITH_TIMEOUT(!MudletReplay::self()->running(), 10000);
        QTest::qWait(50);
        QCOMPARE(heard, QStringList({qsl("started"), qsl("finished after its last line")}));
        QVERIFY(session.shutdown(false));
    }

    // loadReplay() accepts a recording that caught nothing, so an app waiting for its end gets one
    void test_anEmptyReplayStartsAndEnds()
    {
        const QString name = qsl("Headless Replay Empty");
        QVERIFY(writeProfile(name, QString()));
        const QString logDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name);
        QVERIFY(QDir().mkpath(logDir));
        QVERIFY(writeFile(qsl("%1/empty.dat").arg(logDir), QByteArray()));

        THeadlessSession session;
        QStringList heard;
        connect(&session, &THeadlessSession::replayStarted, &session, [&](const QString& profile) {
            heard << qsl("started %1").arg(profile);
        });
        connect(&session, &THeadlessSession::replayFinished, &session, [&](const QString& profile) {
            heard << qsl("finished %1").arg(profile);
        });
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("emptyReplay = tostring(loadReplay('empty.dat'))")));
        QCOMPARE(luaGlobalString(session.host(), "emptyReplay"), qsl("true"));
        QVERIFY(!MudletReplay::self()->running());
        QTRY_COMPARE(heard.size(), 2);
        QCOMPARE(heard, QStringList({qsl("started %1").arg(name), qsl("finished %1").arg(name)}));
        QTest::qWait(50);
        QCOMPARE(heard.size(), 2);
        QVERIFY(session.shutdown(false));
    }

    void test_aReplayEndsAsItsProfileCloses_data()
    {
        QTest::addColumn<bool>("byScript");
        QTest::newRow("shut down with a save") << false;
        QTest::newRow("closed by a script") << true;
    }

    // Once, and with nothing of it printed after, though the close runs the event loop
    void test_aReplayEndsAsItsProfileCloses()
    {
        QFETCH(const bool, byScript);
        const QString name = qsl("Headless Replay Closed");
        QVERIFY(writeProfile(name, QString()));
        const QString logDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name);
        QVERIFY(QDir().mkpath(logDir));
        {
            QFile file(qsl("%1/many.dat").arg(logDir));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QDataStream stream(&file);
            stream.setVersion(QDataStream::Qt_5_12);
            for (int i = 0; i < 2000; ++i) {
                const QByteArray bytes = QByteArrayLiteral("Replayed line ") + QByteArray::number(i) + QByteArrayLiteral(".\r\n");
                stream << qint32{i ? 2 : 0} << static_cast<qint32>(bytes.size());
                stream.writeRawData(bytes.constData(), static_cast<int>(bytes.size()));
            }
            QCOMPARE(stream.status(), QDataStream::Ok);
        }

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy started(&session, &THeadlessSession::replayStarted);
        QSignalSpy finished(&session, &THeadlessSession::replayFinished);
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        std::optional<qsizetype> linesAtTheEnd;
        connect(&session, &THeadlessSession::replayFinished, &session, [&]() {
            linesAtTheEnd = output.size();
            QVERIFY(quit.isEmpty());
        });
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        // As a save the close waits for runs the event loop
        QVERIFY(session.runLua(qsl("registerAnonymousEventHandler('sysExitEvent', function() pumpEvents(200) end) loadReplay('many.dat')")));
        QTRY_COMPARE(started.size(), 1);
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Replayed line 3.")), 10000);
        QVERIFY(MudletReplay::self()->running());

        if (byScript) {
            QVERIFY(session.runLua(qsl("closeProfile()")));
            QTRY_COMPARE_WITH_TIMEOUT(quit.size(), 1, 10000);
        } else {
            QVERIFY(session.shutdown(true));
        }
        QTest::qWait(200);
        QCOMPARE(finished.size(), 1);
        QCOMPARE(finished.constFirst().at(0).toString(), name);
        QVERIFY(linesAtTheEnd.has_value());
        QCOMPARE(output.size(), *linesAtTheEnd);
        QVERIFY(!outputTexts(output).contains(qsl("Replayed line 1999.")));
    }

    // As the GUI's command line hands the profile's key bindings its key events
    void test_aKeyPressedRunsItsKeyBinding()
    {
        using KeyResult = THeadlessSession::KeyResult;
        const QString name = qsl("Headless Keys");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession unstarted;
        QCOMPARE(unstarted.pressKey(Qt::Key_F1, Qt::ControlModifier), KeyResult::refused);

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
headlessKeys = ""
tempKey(mudlet.keymodifier.Control, mudlet.key.F1, [[headlessKeys = headlessKeys .. "Ctrl+F1 "]])
tempKey(mudlet.keymodifier.Keypad, mudlet.key["8"], [[headlessKeys = headlessKeys .. "Keypad 8 "]]))lua")));
        QCOMPARE(session.pressKey(Qt::Key_F1, Qt::ControlModifier), KeyResult::bindingRan);
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), qsl("Ctrl+F1 "));
        QCOMPARE(session.pressKey(Qt::Key_F1), KeyResult::noBinding);
        QCOMPARE(session.errorString(), qsl("No key binding of the profile takes that key."));
        QCOMPARE(session.pressKey(Qt::Key_8), KeyResult::noBinding);
        QCOMPARE(session.pressKey(Qt::Key_8, Qt::KeypadModifier), KeyResult::bindingRan);
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), qsl("Ctrl+F1 Keypad 8 "));

        std::optional<KeyResult> pressedMidWrite;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &pressedMidWrite](const QString&, const QString& text, bool) {
            if (text == qsl("headless key line")) {
                pressedMidWrite = session.pressKey(Qt::Key_F1, Qt::ControlModifier);
            }
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("headless key line\n"))lua")));
        QCOMPARE(pressedMidWrite, std::optional(KeyResult::refused));
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), qsl("Ctrl+F1 Keypad 8 "));

        QVERIFY(session.shutdown(false));
        QCOMPARE(session.pressKey(Qt::Key_F1, Qt::ControlModifier), KeyResult::refused);
        QCOMPARE(session.errorString(), qsl("No profile is open."));
    }

    // Bindings on them never fire in the GUI, as its command line acts on them first; on the keypad they do
    void test_keysTheCommandLineKeepsRunNoBinding()
    {
        using KeyResult = THeadlessSession::KeyResult;
        const QString name = qsl("Headless Kept Keys");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
headlessKeys = ""
local function bind(modifier, key, label)
  tempKey(modifier, key, function() headlessKeys = headlessKeys .. label .. " " end)
end
bind(mudlet.keymodifier.None, mudlet.key.Up, "Up")
bind(mudlet.keymodifier.Control, mudlet.key.Up, "Ctrl+Up")
bind(mudlet.keymodifier.Alt, mudlet.key.Up, "Alt+Up")
bind(mudlet.keymodifier.Keypad, mudlet.key.Up, "Keypad+Up")
bind(mudlet.keymodifier.None, mudlet.key.Return, "Return")
bind(mudlet.keymodifier.Alt, mudlet.key.Return, "Alt+Return")
bind(mudlet.keymodifier.None, mudlet.key.Escape, "Esc")
bind(mudlet.keymodifier.None, mudlet.key.Tab, "Tab")
bind(mudlet.keymodifier.Shift, mudlet.key.Space, "Shift+Space")
bind(mudlet.keymodifier.Control, mudlet.key.Space, "Ctrl+Space"))lua")));

        QCOMPARE(session.pressKey(Qt::Key_Return), KeyResult::keyKept);
        QCOMPARE(session.errorString(), qsl("The GUI's command line keeps that key for itself, offering it to no key binding; the app handles it."));
        QCOMPARE(session.pressKey(Qt::Key_Escape), KeyResult::keyKept);
        QCOMPARE(session.pressKey(Qt::Key_Tab), KeyResult::keyKept);
        QCOMPARE(session.pressKey(Qt::Key_Space, Qt::ShiftModifier), KeyResult::keyKept);
#if defined(Q_OS_MACOS)
        // The arrows carry the keypad flag there, so the line's own Up is Keypad+Up, which meets the bindings first
        QCOMPARE(session.pressKey(Qt::Key_Up), KeyResult::bindingRan);
        QCOMPARE(session.pressKey(Qt::Key_Up, Qt::ControlModifier), KeyResult::bindingRan);
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), qsl("Up Ctrl+Up "));
        QVERIFY(session.runLua(qsl("headlessKeys = ''")));
#else
        QCOMPARE(session.pressKey(Qt::Key_Up), KeyResult::keyKept);
        QCOMPARE(session.pressKey(Qt::Key_Up, Qt::ControlModifier), KeyResult::keyKept);
#endif
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), QString());

        QCOMPARE(session.pressKey(Qt::Key_Up, Qt::AltModifier), KeyResult::bindingRan);
        QCOMPARE(session.pressKey(Qt::Key_Up, Qt::KeypadModifier), KeyResult::bindingRan);
        QCOMPARE(session.pressKey(Qt::Key_Return, Qt::AltModifier), KeyResult::bindingRan);
        QCOMPARE(session.pressKey(Qt::Key_Space, Qt::ControlModifier), KeyResult::bindingRan);
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), qsl("Alt+Up Keypad+Up Alt+Return Ctrl+Space "));

        // A keypad key no binding takes goes on to the line, which sends with Enter, as a real Enter always is
        QVERIFY(session.runLua(qsl("headlessKeys = ''")));
        QCOMPARE(session.pressKey(Qt::Key_Enter, Qt::KeypadModifier), KeyResult::keyKept);
        QCOMPARE(session.pressKey(Qt::Key_Enter, Qt::ControlModifier | Qt::KeypadModifier), KeyResult::noBinding);
#if defined(Q_OS_MACOS)
        QCOMPARE(session.pressKey(Qt::Key_Down, Qt::KeypadModifier), KeyResult::keyKept);
        QCOMPARE(session.pressKey(Qt::Key_Down, Qt::ControlModifier | Qt::KeypadModifier), KeyResult::keyKept);
#else
        QCOMPARE(session.pressKey(Qt::Key_Down, Qt::KeypadModifier), KeyResult::noBinding);
        QCOMPARE(session.pressKey(Qt::Key_Down, Qt::ControlModifier | Qt::KeypadModifier), KeyResult::noBinding);
#endif
        QVERIFY(session.runLua(qsl(R"lua(tempKey(mudlet.keymodifier.Keypad, mudlet.key.Enter, function() headlessKeys = headlessKeys .. "Keypad+Enter " end))lua")));
        QCOMPARE(session.pressKey(Qt::Key_Enter, Qt::KeypadModifier), KeyResult::bindingRan);
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), qsl("Keypad+Enter "));
        QVERIFY(session.shutdown(false));
    }

    // As the GUI's hidden-input box types a printable key into the password before any binding may take it
    void test_aKeyTypedIntoHiddenInputRunsNoBinding()
    {
        using KeyResult = THeadlessSession::KeyResult;
        const QString name = qsl("Headless Hidden Keys");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy hidden(&session, &THeadlessSession::hiddenInputWanted);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QVERIFY(session.runLua(qsl(R"lua(
headlessKeys = ""
tempKey(mudlet.keymodifier.Keypad, mudlet.key["8"], [[send("north") headlessKeys = headlessKeys .. "Keypad 8 "]])
tempKey(mudlet.keymodifier.Shift, mudlet.key.A, [[headlessKeys = headlessKeys .. "Shift+A "]])
tempKey(mudlet.keymodifier.Control, mudlet.key.F1, [[headlessKeys = headlessKeys .. "Ctrl+F1 "]])
tempKey(mudlet.keymodifier.None, mudlet.key.Escape, [[headlessKeys = headlessKeys .. "Esc "]])
tempKey(mudlet.keymodifier.Control + mudlet.keymodifier.Alt, mudlet.key.S, [[headlessKeys = headlessKeys .. "Ctrl+Alt+S "]]))lua")));

        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Password:")), 10000);
        QCOMPARE(hidden.size(), 1);
        QCOMPARE(session.pressKey(Qt::Key_8, Qt::KeypadModifier), KeyResult::keyKept);
        QCOMPARE(session.errorString(), qsl("While the game hides what is typed, its hidden-input box keeps that key, typing it if printable; the app types it into its hidden input."));
        QCOMPARE(session.pressKey(Qt::Key_A, Qt::ShiftModifier), KeyResult::keyKept);
        QCOMPARE(session.pressKey(Qt::Key_Escape), KeyResult::keyKept);
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), QString());
        QCOMPARE(session.pressKey(Qt::Key_F1, Qt::ControlModifier), KeyResult::bindingRan);
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), qsl("Ctrl+F1 "));

        // The box goes by what the key typed: Ctrl+Alt is AltGr only where the layout made a character of it
        QCOMPARE(session.pressKey(Qt::Key_S, Qt::ControlModifier | Qt::AltModifier, qsl("ś")), KeyResult::keyKept);
        QCOMPARE(session.pressKey(Qt::Key_S, Qt::ControlModifier | Qt::AltModifier, QString(QChar(0x13))), KeyResult::bindingRan);
        QCOMPARE(session.pressKey(Qt::Key_S, Qt::ControlModifier | Qt::AltModifier, qsl("")), KeyResult::bindingRan);
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), qsl("Ctrl+F1 Ctrl+Alt+S Ctrl+Alt+S "));
        // Without the text, any printable key with Ctrl+Alt counts as AltGr
        QCOMPARE(session.pressKey(Qt::Key_S, Qt::ControlModifier | Qt::AltModifier), KeyResult::keyKept);
        QCOMPARE(session.pressKey(Qt::Key_8, Qt::KeypadModifier, qsl("8")), KeyResult::keyKept);
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), qsl("Ctrl+F1 Ctrl+Alt+S Ctrl+Alt+S "));
        QTest::qWait(100);
        QVERIFY(!game->readAll().contains("north"));
        QVERIFY(session.shutdown(false));
    }

    // As the GUI's replay toolbar holds, resumes and stops it, and only for a replay of the session's profile
    void test_aReplayIsPausedResumedAndStopped()
    {
        const QString name = qsl("Headless Replay Paused");
        QVERIFY(writeProfile(name, QString()));
        const QString logDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name);
        QVERIFY(QDir().mkpath(logDir));
        {
            QFile file(qsl("%1/three.dat").arg(logDir));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QDataStream stream(&file);
            stream.setVersion(QDataStream::Qt_5_12);
            for (const auto& [gap, bytes] : {std::pair{0, QByteArrayLiteral("First paused line.\r\n")},
                                             std::pair{1000, QByteArrayLiteral("Second paused line.\r\n")},
                                             std::pair{1000, QByteArrayLiteral("Third paused line.\r\n")}}) {
                stream << qint32{gap} << static_cast<qint32>(bytes.size());
                stream.writeRawData(bytes.constData(), static_cast<int>(bytes.size()));
            }
            QCOMPARE(stream.status(), QDataStream::Ok);
        }

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy local(&session, &THeadlessSession::localText);
        QSignalSpy started(&session, &THeadlessSession::replayStarted);
        QSignalSpy finished(&session, &THeadlessSession::replayFinished);
        QStringList heard;
        connect(&session, &THeadlessSession::replayStarted, this, [&heard]() {
            heard << qsl("started");
        });
        connect(&session, &THeadlessSession::replayFinished, this, [&heard]() {
            heard << qsl("finished");
        });
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(!session.replayPaused());
        QVERIFY(!session.pauseReplay());
        QCOMPARE(session.errorString(), qsl("No replay of the profile is playing."));
        QVERIFY(!session.resumeReplay());
        QVERIFY(!session.stopReplay());

        const QString other = qsl("Headless Replay Paused Elsewhere");
        QVERIFY(writeProfile(other, QString()));
        const QString otherLogDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, other);
        QVERIFY(QDir().mkpath(otherLogDir));
        QVERIFY(QFile::copy(qsl("%1/three.dat").arg(logDir), qsl("%1/three.dat").arg(otherLogDir)));
        QVERIFY(session.runLua(qsl("loadProfile('%1', true)").arg(other)));
        Host* pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl("loadReplay('three.dat')")));
        QVERIFY(MudletReplay::self()->running());
        QVERIFY(!session.pauseReplay());
        QCOMPARE(session.errorString(), qsl("No replay of the profile is playing."));
        QVERIFY(!pOther->mTelnet.replayPaused());
        pOther->mTelnet.stopReplay();
        QTRY_VERIFY_WITH_TIMEOUT(!MudletReplay::self()->running(), 10000);

        // Before replayStarted arrives, and before its first line plays; still held when it does arrive
        QVERIFY(session.runLua(qsl("loadReplay('three.dat')")));
        QVERIFY(session.pauseReplay());
        QVERIFY(session.replayPaused());
        QVERIFY(!session.pauseReplay());
        QCOMPARE(session.errorString(), qsl("The replay is already paused."));
        QTRY_COMPARE(started.size(), 1);
        QVERIFY(session.replayPaused());
        QTest::qWait(500);
        QVERIFY(!outputTexts(output).contains(qsl("First paused line.")));
        QVERIFY(session.resumeReplay());
        QVERIFY(!session.replayPaused());
        QVERIFY(!session.resumeReplay());
        QCOMPARE(session.errorString(), qsl("The replay is not paused."));
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("First paused line.")), 10000);

        QVERIFY(session.pauseReplay());
        QTest::qWait(1500);
        QVERIFY(!outputTexts(output).contains(qsl("Second paused line.")));
        QVERIFY(session.resumeReplay());
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Second paused line.")), 10000);

        // Stopped while held
        QVERIFY(session.pauseReplay());
        QVERIFY(session.stopReplay());
        QVERIFY(!session.replayPaused());
        QTRY_COMPARE(finished.size(), 1);
        QCOMPARE(finished.constFirst().at(0).toString(), name);
        QCOMPARE(heard, (QStringList{qsl("started"), qsl("finished")}));
        bool toldOfTheStop = false;
        for (const auto& arguments : std::as_const(local)) {
            toldOfTheStop = toldOfTheStop || arguments.at(1).toString().contains(qsl("The replay has been stopped."));
        }
        QVERIFY(toldOfTheStop);
        QVERIFY(!session.stopReplay());
        QTest::qWait(1500);
        QVERIFY(!outputTexts(output).contains(qsl("Third paused line.")));
        QVERIFY(session.shutdown(false));
    }

    // A trigger on a replayed line that runs the event loop is in the middle of that line's chunk, which the
    // replay reads on from where it was once the trigger is done
    void test_aReplayIsLeftAloneWhileAScriptRuns()
    {
        const auto testMode = inTestMode();
        const QString name = qsl("Headless Replay Busy");
        QVERIFY(writeProfile(name, QString()));
        const QString logDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name);
        QVERIFY(QDir().mkpath(logDir));
        {
            QFile file(qsl("%1/busy.dat").arg(logDir));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QDataStream stream(&file);
            stream.setVersion(QDataStream::Qt_5_12);
            for (const auto& [gap, bytes] : {std::pair{0, QByteArrayLiteral("First busy line.\r\nSecond busy line.\r\n")}, std::pair{200, QByteArrayLiteral("Third busy line.\r\n")}}) {
                stream << qint32{gap} << static_cast<qint32>(bytes.size());
                stream.writeRawData(bytes.constData(), static_cast<int>(bytes.size()));
            }
            QCOMPARE(stream.status(), QDataStream::Ok);
        }

        THeadlessSession session;
        QStringList heard;
        connect(&session, &THeadlessSession::outputLine, this, [&heard](const QString&, const QString& text) {
            heard << text;
        });
        connect(&session, &THeadlessSession::replayFinished, this, [&heard]() {
            heard << qsl("finished");
        });
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempTrigger("First busy line.", [[pumpEvents(300)]]))lua")));
        std::optional<bool> paused;
        std::optional<bool> stopped;
        QString why;
        QVERIFY(session.runLua(qsl("loadReplay('busy.dat')")));
        QTimer::singleShot(100, &session, [&]() {
            paused = session.pauseReplay();
            stopped = session.stopReplay();
            why = session.errorString();
        });
        QTRY_VERIFY_WITH_TIMEOUT(heard.contains(qsl("finished")), 10000);
        QTest::qWait(100);
        QCOMPARE(heard.join(QChar::LineFeed), qsl("First busy line.\nSecond busy line.\nThird busy line.\nfinished"));
        QCOMPARE(paused, std::optional(false));
        QCOMPARE(stopped, std::optional(false));
        QCOMPARE(why, qsl("The profile is running a script, so its replay cannot be changed until that has ended."));
        QVERIFY(session.shutdown(false));
    }

    // A held replay never reaches its end, so the profile's close is what ends it
    void test_aPausedReplayEndsWithTheProfile_data()
    {
        QTest::addColumn<bool>("byScript");
        QTest::newRow("shutdown()") << false;
        QTest::newRow("closeProfile()") << true;
    }

    void test_aPausedReplayEndsWithTheProfile()
    {
        QFETCH(const bool, byScript);
        const QString name = qsl("Headless Replay Held At Close");
        QVERIFY(writeProfile(name, QString()));
        const QString logDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name);
        QVERIFY(QDir().mkpath(logDir));
        {
            QFile file(qsl("%1/held.dat").arg(logDir));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QDataStream stream(&file);
            stream.setVersion(QDataStream::Qt_5_12);
            for (const auto& [gap, bytes] : {std::pair{0, QByteArrayLiteral("First held line.\r\n")}, std::pair{60000, QByteArrayLiteral("Second held line.\r\n")}}) {
                stream << qint32{gap} << static_cast<qint32>(bytes.size());
                stream.writeRawData(bytes.constData(), static_cast<int>(bytes.size()));
            }
            QCOMPARE(stream.status(), QDataStream::Ok);
        }

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy started(&session, &THeadlessSession::replayStarted);
        QSignalSpy finished(&session, &THeadlessSession::replayFinished);
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("loadReplay('held.dat')")));
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("First held line.")), 10000);
        QCOMPARE(started.size(), 1);
        QVERIFY(session.pauseReplay());
        if (byScript) {
            QVERIFY(session.runLua(qsl("closeProfile()")));
            QTRY_COMPARE_WITH_TIMEOUT(quit.size(), 1, 10000);
        } else {
            QVERIFY(session.shutdown(false));
        }
        QTRY_COMPARE(finished.size(), 1);
        QTest::qWait(200);
        QCOMPARE(finished.size(), 1);
        QCOMPARE(finished.constFirst().at(0).toString(), name);
        QVERIFY(!MudletReplay::self() || !MudletReplay::self()->running());
        if (byScript) {
            QVERIFY(session.shutdown(false));
        }
    }

    // A replay stopped before its replayStarted arrives still sends both, in that order, even if the session
    // shuts down before the event loop runs
    void test_aReplayStoppedBeforeItsStartIsPaired_data()
    {
        QTest::addColumn<bool>("shutDown");
        QTest::newRow("then the event loop") << false;
        QTest::newRow("then shutdown()") << true;
    }

    void test_aReplayStoppedBeforeItsStartIsPaired()
    {
        QFETCH(const bool, shutDown);
        const QString name = qsl("Headless Replay Stopped Early");
        QVERIFY(writeProfile(name, QString()));
        const QString logDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name);
        QVERIFY(QDir().mkpath(logDir));
        {
            QFile file(qsl("%1/early.dat").arg(logDir));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QDataStream stream(&file);
            stream.setVersion(QDataStream::Qt_5_12);
            const QByteArray bytes = QByteArrayLiteral("Early line.\r\n");
            stream << qint32{60000} << static_cast<qint32>(bytes.size());
            stream.writeRawData(bytes.constData(), static_cast<int>(bytes.size()));
            QCOMPARE(stream.status(), QDataStream::Ok);
        }

        THeadlessSession session;
        QStringList heard;
        connect(&session, &THeadlessSession::replayStarted, this, [&heard](const QString& profile) {
            heard << qsl("started %1").arg(profile);
        });
        connect(&session, &THeadlessSession::replayFinished, this, [&heard](const QString& profile) {
            heard << qsl("finished %1").arg(profile);
        });
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("loadReplay('early.dat')")));
        QVERIFY(session.stopReplay());
        if (shutDown) {
            QVERIFY(session.shutdown(false));
        }
        const QStringList paired{qsl("started %1").arg(name), qsl("finished %1").arg(name)};
        QTRY_COMPARE(heard, paired);
        QTest::qWait(200);
        QCOMPARE(heard, paired);
        QVERIFY(!MudletReplay::self() || !MudletReplay::self()->running());
        if (!shutDown) {
            QVERIFY(session.shutdown(false));
        }
    }

    // As the GUI's replay toolbar shows how far it has played and changes its speed
    void test_aReplaysSpeedIsChangedAndItsTimeRead()
    {
        const QString name = qsl("Headless Replay Speed");
        QVERIFY(writeProfile(name, QString()));
        const QString logDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name);
        QVERIFY(QDir().mkpath(logDir));
        // Gaps a speed change has to shorten for the replay to end within the test's wait
        QVERIFY(writeReplay(qsl("%1/three.dat").arg(logDir),
                            {{0, QByteArrayLiteral("First timed line.\r\n")}, {60000, QByteArrayLiteral("Second timed line.\r\n")}, {120000, QByteArrayLiteral("Third timed line.\r\n")}}));

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy started(&session, &THeadlessSession::replayStarted);
        QSignalSpy speed(&session, &THeadlessSession::replaySpeedChanged);
        QSignalSpy finished(&session, &THeadlessSession::replayFinished);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(!session.changeReplaySpeed(true));
        QCOMPARE(session.errorString(), qsl("No replay of the profile is playing."));
        QCOMPARE(session.replayElapsedMSecs(), qint64{-1});
        QCOMPARE(session.replaySpeed(), -1);

        const QString other = qsl("Headless Replay Speed Elsewhere");
        QVERIFY(writeProfile(other, QString()));
        const QString otherLogDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, other);
        QVERIFY(QDir().mkpath(otherLogDir));
        QVERIFY(QFile::copy(qsl("%1/three.dat").arg(logDir), qsl("%1/three.dat").arg(otherLogDir)));
        QVERIFY(session.runLua(qsl("loadProfile('%1', true)").arg(other)));
        Host* pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl("loadReplay('three.dat')")));
        QVERIFY(MudletReplay::self()->running());
        QVERIFY(!session.changeReplaySpeed(true));
        QCOMPARE(MudletReplay::self()->speed(), 1);
        QCOMPARE(session.replayElapsedMSecs(), qint64{-1});
        QCOMPARE(session.replaySpeed(), -1);
        pOther->mTelnet.stopReplay();
        QTRY_VERIFY_WITH_TIMEOUT(!MudletReplay::self()->running(), 10000);

        qint64 elapsedAtThirdLine = -1;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &elapsedAtThirdLine](const QString&, const QString& text) {
            if (text == qsl("Third timed line.")) {
                elapsedAtThirdLine = session.replayElapsedMSecs();
            }
        });

        // The time is that of the line waiting to be played, as the toolbar's is, so it is at the end of a gap
        // as soon as the gap begins and holds there, ahead of what has been shown, while paused
        QVERIFY(session.runLua(qsl("loadReplay('three.dat')")));
        QCOMPARE(session.replayElapsedMSecs(), qint64{0});
        QCOMPARE(session.replaySpeed(), 1);
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("First timed line.")), 10000);
        QTRY_COMPARE(session.replayElapsedMSecs(), qint64{60000});
        QVERIFY(session.pauseReplay());
        QTest::qWait(1000);
        QCOMPARE(session.replayElapsedMSecs(), qint64{60000});
        QVERIFY(!outputTexts(output).contains(qsl("Second timed line.")));

        QTRY_COMPARE(started.size(), 1);
        QVERIFY(session.changeReplaySpeed(true));
        QCOMPARE(session.replaySpeed(), 2);
        QVERIFY(session.resumeReplay());
        for (int i = 0; i < 9; ++i) {
            QVERIFY(session.changeReplaySpeed(true));
        }
        QCOMPARE(session.replaySpeed(), 1024);
        // At the fastest it stays there and its speed is sent again, as the toolbar's button does
        QVERIFY(session.changeReplaySpeed(true));
        QCOMPARE(session.replaySpeed(), 1024);
        QVERIFY(session.changeReplaySpeed(false));
        QCOMPARE(session.replaySpeed(), 512);
        QCOMPARE(session.replayElapsedMSecs(), qint64{60000});
        QTRY_COMPARE(speed.size(), 12);
        QList<int> speeds;
        for (const auto& arguments : std::as_const(speed)) {
            QCOMPARE(arguments.at(0).toString(), name);
            speeds << arguments.at(1).toInt();
        }
        QCOMPARE(speeds, QList<int>({2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 1024, 512}));

        // Three minutes of recording were left, which only the faster speed fits into the wait
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 5000);
        QVERIFY(outputTexts(output).contains(qsl("Third timed line.")));
        QCOMPARE(elapsedAtThirdLine, qint64{180000});
        QCOMPARE(session.replayElapsedMSecs(), qint64{-1});
        QCOMPARE(session.replaySpeed(), -1);
        QVERIFY(!session.changeReplaySpeed(false));
        QVERIFY(session.shutdown(false));
        QCOMPARE(session.replayElapsedMSecs(), qint64{-1});
    }

    // A recording's own time is counted past a day, where a clock face would go round
    void test_aReplaysTimeIsReadPastADay()
    {
        const QString name = qsl("Headless Replay Day");
        QVERIFY(writeProfile(name, QString()));
        const QString logDir = MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name);
        QVERIFY(QDir().mkpath(logDir));
        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy finished(&session, &THeadlessSession::replayFinished);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        // A millisecond short of a day wraps to just before the clock's start, which read as no replay at all
        int replays = 0;
        for (const qint32 gap : {86'399'999, 90'000'000}) {
            QVERIFY(writeReplay(qsl("%1/day.dat").arg(logDir), {{0, QByteArrayLiteral("Before the gap.\r\n")}, {gap, QByteArrayLiteral("After the gap.\r\n")}}));
            output.clear();
            QVERIFY(session.runLua(qsl("loadReplay('day.dat')")));
            QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Before the gap.")), 10000);
            QTRY_COMPARE(session.replayElapsedMSecs(), qint64{gap});
            QVERIFY(session.stopReplay());
            ++replays;
            QTRY_COMPARE(finished.size(), replays);
        }
        QVERIFY(session.shutdown(false));
    }

    // What the speech bridge, Discord and the MCP bridge take for the profile in front
    void test_setActiveProfileNamesTheActiveHost()
    {
        const QString name = qsl("Headless Active");
        QVERIFY(writeProfile(name, QString()));
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
            QVERIFY(!TAppFrontend::instance()->getActiveHost());
            QVERIFY(session.runLua(qsl("headlessActive = tostring(setActiveProfile('%1'))").arg(name)));
            QCOMPARE(luaGlobalString(session.host(), "headlessActive"), qsl("true"));
            QCOMPARE(TAppFrontend::instance()->getActiveHost(), session.host());
            QVERIFY(session.shutdown(false));
        }
        QVERIFY(!TAppFrontend::instance()->getActiveHost());
        // As with a main window, the profile in front went with it, so opening it again does not bring it back
        THeadlessSession reopened;
        QCOMPARE(reopened.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(!TAppFrontend::instance()->getActiveHost());
        QVERIFY(reopened.shutdown(false));
    }

    // A preload only fetches the file, which needs no window
    void test_whatOnlyAMainWindowCanShowSaysSo()
    {
        const QString name = qsl("Headless No Window");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
local function answer(f, ...)
  local ok, err = f(...)
  return tostring(ok) .. "|" .. tostring(err)
end
headlessNoWindow = table.concat({
  answer(showNotification, "headless title", "headless text"),
  answer(playVideoFile, {name = "headless-absent.mp4"}),
  answer(loadVideoFile, {name = "headless-absent.mp4"}),
}, ";")
)lua")));
        const QString refused = qsl("nil|the profile has no main window");
        QCOMPARE(luaGlobalString(session.host(), "headlessNoWindow"), QStringList({refused, refused, qsl("true|nil")}).join(qsl(";")));
        QVERIFY(session.shutdown(false));
    }

    // As GMCPAuthenticator reconnects after a rejected sign-in: from a timer, with no Lua running. The socket is
    // still connected, so connectIt() aborts it and runs the event loop, where the close closeMudlet() posted runs.
    void test_aProfileClosedWhileItReconnectsOutlivesTheReconnect()
    {
        const QString name = qsl("Headless Close During Reconnect");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("localhost")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QTRY_COMPARE_WITH_TIMEOUT(session.host()->mTelnet.getConnectionState(), QAbstractSocket::ConnectedState, 10000);

        int dialled = 0;
        connect(&server, &QTcpServer::newConnection, &server, [&server, &dialled]() {
            while (std::unique_ptr<QTcpSocket> socket{server.nextPendingConnection()}) {
                ++dialled;
            }
        });
        QPointer<Host> profile(session.host());
        std::optional<bool> outlivedReconnect;
        bool closedInReconnect = false;
        bool lookingUpOnceClosed = false;
        QTimer::singleShot(0, profile, [&]() {
            profile->mTelnet.reconnect();
            outlivedReconnect = !profile.isNull();
            if (profile) {
                closedInReconnect = profile->closeStarted();
                lookingUpOnceClosed = profile->mTelnet.getConnectionState() == QAbstractSocket::HostLookupState;
            }
        });
        QVERIFY(session.runLua(qsl("closeMudlet()")));
        QTRY_VERIFY_WITH_TIMEOUT(outlivedReconnect.has_value(), 10000);
        QVERIFY(*outlivedReconnect);
        QVERIFY2(closedInReconnect, "SETUP: the close did not run inside the reconnect");
        QVERIFY2(!lookingUpOnceClosed, "the reconnect went on to look the game up for a profile that had closed");
        QTRY_VERIFY_WITH_TIMEOUT(profile.isNull(), 10000);
        QTest::qWait(300);
        QCOMPARE(dialled, 0);
        // Takes the quit closeMudlet() asked for, which would otherwise end a later test's event loop
        QTimer watchdog;
        watchdog.setSingleShot(true);
        connect(&watchdog, &QTimer::timeout, &watchdog, []() {
            QCoreApplication::exit(2);
        });
        watchdog.start(std::chrono::seconds(10));
        QCOMPARE(QCoreApplication::exec(), 0);
    }

    // shutdown() from a handler the reconnect's event loop runs would close the profile under it; a save deletes
    // nothing, so it goes ahead
    void test_aSessionWaitsForAReconnectToReturn()
    {
        const QString name = qsl("Headless Shutdown During Reconnect");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("localhost")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QTRY_COMPARE_WITH_TIMEOUT(session.host()->mTelnet.getConnectionState(), QAbstractSocket::ConnectedState, 10000);

        std::optional<bool> saved;
        std::optional<bool> shutDown;
        QString saveError;
        QString shutdownError;
        connect(&session, &THeadlessSession::disconnected, &session, [&]() {
            if (!shutDown.has_value()) {
                saved = session.save();
                saveError = session.errorString();
                shutDown = session.shutdown(false);
                shutdownError = session.errorString();
            }
        });
        QPointer<Host> profile(session.host());
        std::optional<bool> handledInReconnect;
        QTimer::singleShot(0, profile, [&]() {
            profile->mTelnet.reconnect();
            handledInReconnect = shutDown.has_value();
        });
        QTRY_VERIFY_WITH_TIMEOUT(handledInReconnect.has_value(), 10000);
        QVERIFY2(*handledInReconnect, "SETUP: the disconnection was not handled inside the reconnect");
        QVERIFY2(saved == std::optional<bool>(true), qPrintable(saveError));
        QCOMPARE(shutDown, std::optional<bool>(false));
        QVERIFY2(shutdownError.contains(qsl("reconnecting")), qPrintable(shutdownError));
        QVERIFY(session.shutdown(false));
    }

    // The deleted session's pool waits for the reconnect, which must not then dial the game it is about to drop
    void test_aSessionDeletedWhileItReconnectsDialsNoMore()
    {
        const QString name = qsl("Headless Delete During Reconnect");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("localhost")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        const QPointer<THeadlessSession> session(pSession);
        QCOMPARE(pSession->start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QTRY_COMPARE_WITH_TIMEOUT(pSession->host()->mTelnet.getConnectionState(), QAbstractSocket::ConnectedState, 10000);

        int dialled = 0;
        connect(&server, &QTcpServer::newConnection, &server, [&server, &dialled]() {
            while (std::unique_ptr<QTcpSocket> socket{server.nextPendingConnection()}) {
                ++dialled;
            }
        });
        connect(pSession, &THeadlessSession::disconnected, pSession, [pSession]() {
            delete pSession;
        });
        QPointer<Host> profile(pSession->host());
        std::optional<bool> outlivedReconnect;
        bool deletedInReconnect = false;
        bool connectingOnceDeleted = false;
        QTimer::singleShot(0, profile, [&]() {
            profile->mTelnet.reconnect();
            outlivedReconnect = !profile.isNull();
            deletedInReconnect = session.isNull();
            if (profile) {
                connectingOnceDeleted = profile->mTelnet.getConnectionState() != QAbstractSocket::UnconnectedState;
            }
        });
        QTRY_VERIFY_WITH_TIMEOUT(outlivedReconnect.has_value(), 10000);
        QVERIFY(*outlivedReconnect);
        QVERIFY2(deletedInReconnect, "SETUP: the session was not deleted inside the reconnect");
        QVERIFY2(!connectingOnceDeleted, "the reconnect went on to connect a profile whose session had been deleted");
        QTRY_VERIFY_WITH_TIMEOUT(profile.isNull(), 10000);
        QTest::qWait(300);
        QCOMPARE(dialled, 0);
    }

    // With the game already gone, a script's reconnect only looks it up, and the pool, deleted from a handler the
    // same script set off, waits for the script: a lookup begun before is called off, and one asked for after
    // is never begun
    void test_aSessionDeletedWhileItLooksTheGameUpDialsNoMore_data()
    {
        QTest::addColumn<QString>("script");
        QTest::newRow("reconnect, then deleted") << qsl("reconnect(); echo('x')");
        QTest::newRow("deleted, then reconnect") << qsl("echo('x'); reconnect()");
    }

    void test_aSessionDeletedWhileItLooksTheGameUpDialsNoMore()
    {
        QFETCH(QString, script);
        const QString name = qsl("Headless Delete During Lookup");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("localhost")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        const QPointer<THeadlessSession> session(pSession);
        QCOMPARE(pSession->start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QPointer<Host> profile(pSession->host());
        QTRY_COMPARE_WITH_TIMEOUT(profile->mTelnet.getConnectionState(), QAbstractSocket::ConnectedState, 10000);
        game.reset();
        QTRY_COMPARE_WITH_TIMEOUT(profile->mTelnet.getConnectionState(), QAbstractSocket::UnconnectedState, 10000);

        int dialled = 0;
        connect(&server, &QTcpServer::newConnection, &server, [&server, &dialled]() {
            while (std::unique_ptr<QTcpSocket> socket{server.nextPendingConnection()}) {
                ++dialled;
            }
        });
        connect(pSession, &THeadlessSession::localText, pSession, [pSession]() {
            delete pSession;
        });
        pSession->runLua(script);
        QVERIFY2(session.isNull(), "SETUP: the script's echo did not delete the session");
        QVERIFY2(profile, "SETUP: the pool did not wait for the script");
        QCOMPARE(profile->mTelnet.getConnectionState(), QAbstractSocket::UnconnectedState);
        QTRY_VERIFY_WITH_TIMEOUT(profile.isNull(), 10000);
        QTest::qWait(300);
        QCOMPARE(dialled, 0);
    }

    // A profile opened once its pool is to go would only dial the game for the pool to drop it
    void test_aPoolToBeDeletedOpensNoProfile()
    {
        const QString name = qsl("Headless Delete Then Open");
        const QString other = qsl("Headless Opened Too Late");
        QVERIFY(writeProfile(name, QString()));
        QVERIFY(writeProfile(other, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(other, qsl("url"), qsl("localhost")).first);
        QVERIFY(MudletApp::writeProfileData(other, qsl("port"), QString::number(server.serverPort())).first);
        int dialled = 0;
        connect(&server, &QTcpServer::newConnection, &server, [&server, &dialled]() {
            while (std::unique_ptr<QTcpSocket> socket{server.nextPendingConnection()}) {
                ++dialled;
            }
        });

        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        const QPointer<THeadlessSession> session(pSession);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        connect(pSession, &THeadlessSession::localText, pSession, [pSession]() {
            delete pSession;
        });
        QPointer<Host> profile(pSession->host());
        pSession->runLua(qsl("echo('x'); openedTooLate = tostring(loadProfile('%1', false))").arg(other));
        QVERIFY2(session.isNull(), "SETUP: the script's echo did not delete the session");
        QVERIFY2(profile, "SETUP: the pool did not wait for the script");
        const QString openedTooLate = luaGlobalString(profile, "openedTooLate");
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QTest::qWait(300);
        QCOMPARE(dialled, 0);
        QCOMPARE(openedTooLate, qsl("nil"));
    }

    // shutdown() runs the event loop, here for a reset asked for first, where a dial already under way would connect,
    // run the profile's onConnect and sysConnectionEvent and start its login, just before the pool drops it. The
    // dial goes through a SOCKS5 proxy that holds it until its reply is sent: loopback's own handshake is over
    // before the test could step in on some platforms.
    void test_aShutdownCallsOffADialUnderWay()
    {
        const QString name = qsl("Headless Shutdown During Dial");
        QVERIFY(writeProfile(name, QString()));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("localhost")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), qsl("4000")).first);
        QTcpServer proxy;
        QVERIFY(proxy.listen(QHostAddress::LocalHost));
        QList<QPointer<QTcpSocket>> held;
        connect(&proxy, &QTcpServer::newConnection, &proxy, [&proxy, &held]() {
            while (QTcpSocket* client = proxy.nextPendingConnection()) {
                connect(client, &QIODevice::readyRead, client, [client, &held]() {
                    QByteArray in = client->property("in").toByteArray() + client->readAll();
                    if (!client->property("greeted").toBool()) {
                        if (in.size() < 2 || in.size() < 2 + static_cast<quint8>(in.at(1))) {
                            client->setProperty("in", in);
                            return;
                        }
                        in.remove(0, 2 + static_cast<quint8>(in.at(1)));
                        client->write(QByteArray::fromHex("0500"));
                        client->setProperty("greeted", true);
                    }
                    // A CONNECT to an IPv4, a name or an IPv6 address, then the port
                    if (in.size() < 5) {
                        client->setProperty("in", in);
                        return;
                    }
                    const quint8 type = static_cast<quint8>(in.at(3));
                    const int length = 4 + (type == 0x01 ? 4 : type == 0x04 ? 16 : 1 + static_cast<quint8>(in.at(4))) + 2;
                    if (in.size() < length) {
                        client->setProperty("in", in);
                        return;
                    }
                    client->setProperty("in", QByteArray());
                    held.append(client);
                });
            }
        });

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QPointer<Host> profile(session.host());
        profile->mUseProxy = true;
        profile->mProxyAddress = qsl("127.0.0.1");
        profile->mProxyPort = proxy.serverPort();
        int connections = 0;
        connect(&profile->mTelnet, &cTelnet::signal_connected, &proxy, [&connections]() {
            ++connections;
        });
        profile->mTelnet.reconnect();
        QTRY_VERIFY_WITH_TIMEOUT(!held.isEmpty(), 10000);
        QCOMPARE(profile->mTelnet.getConnectionState(), QAbstractSocket::ConnectingState);

        // Read into the proxy engine here, which then queues its notice of the connection, so that it is delivered
        // with the reset in shutdown()'s first pass of the event loop, rather than in passes it may not run
        for (const QPointer<QTcpSocket>& client : std::as_const(held)) {
            QVERIFY(client);
            client->write(QByteArray::fromHex("05000001000000000000"));
            QVERIFY(client->waitForBytesWritten(5000));
            const QList<QTcpSocket*> sockets = profile->mTelnet.mSocket_ipV4.findChildren<QTcpSocket*>() + profile->mTelnet.mSocket_ipV6.findChildren<QTcpSocket*>();
            const auto control = std::find_if(sockets.cbegin(), sockets.cend(), [&client](const QTcpSocket* socket) {
                return socket->localPort() == client->peerPort() && socket->peerPort() == client->localPort();
            });
            QVERIFY2(control != sockets.cend(), "SETUP: the proxy engine's own socket was not found");
            QVERIFY((*control)->waitForReadyRead(5000));
        }
        QCOMPARE(profile->mTelnet.getConnectionState(), QAbstractSocket::ConnectingState);
        QCOMPARE(connections, 0);
        QVERIFY(session.runLua(qsl("resetProfile()")));
        QVERIFY(session.shutdown(true));
        QVERIFY(profile.isNull());
        QCOMPARE(connections, 0);
    }

    void test_requestsForThePlayersAttentionReachTheSession()
    {
        const QString name = qsl("Headless Attention");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy notifications(&session, &THeadlessSession::notificationRequested);
        QSignalSpy attention(&session, &THeadlessSession::attentionRequested);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
headlessShown = tostring(showNotification("title", "body", 2)) .. "," .. tostring(showNotification("only a title"))
alert(1.5)
alert()
)lua")));
        QCOMPARE(luaGlobalString(session.host(), "headlessShown"), qsl("true,true"));
        QCOMPARE(notifications.count(), 2);
        QCOMPARE(notifications.at(0).at(0).toString(), name);
        QCOMPARE(notifications.at(0).at(1).toString(), qsl("title"));
        QCOMPARE(notifications.at(0).at(2).toString(), qsl("body"));
        QCOMPARE(notifications.at(0).at(3).value<std::optional<int>>(), std::optional<int>(2000));
        QCOMPARE(notifications.at(1).at(2).toString(), qsl("only a title"));
        QCOMPARE(notifications.at(1).at(3).value<std::optional<int>>(), std::nullopt);
        QCOMPARE(attention.count(), 2);
        QCOMPARE(attention.at(0).at(0).toString(), name);
        QCOMPARE(attention.at(0).at(1).toInt(), 1500);
        QCOMPARE(attention.at(1).at(1).toInt(), 0);

        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        game->write("Wake up\a\r\n");
        QTRY_COMPARE_WITH_TIMEOUT(attention.count(), 3, 10000);
        QCOMPARE(attention.at(2).at(0).toString(), name);
        QCOMPARE(attention.at(2).at(1).toInt(), 3000);
        QVERIFY(session.shutdown(false));
    }

    // Nobody hears a notification sent while the frontend blocks the session's signals
    void test_aBlockedSessionShowsNoNotification()
    {
        const QString name = qsl("Headless Blocked Notification");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy notifications(&session, &THeadlessSession::notificationRequested);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        {
            const QSignalBlocker blocker(&session);
            QVERIFY(session.runLua(qsl(R"lua(local shown, why = showNotification("title", "body") headlessBlocked = tostring(shown) .. "|" .. tostring(why))lua")));
        }
        QCOMPARE(luaGlobalString(session.host(), "headlessBlocked"), qsl("nil|the profile has no main window"));
        QCOMPARE(notifications.count(), 0);
        QVERIFY(session.shutdown(false));
    }

    // The exit handlers of a shutdown(false) run as the pool goes, where a frontend's function bound into Lua may delete
    // the session; a notification asked for after that has nobody to go to
    void test_aNotificationAfterTheSessionIsDeletedInAnExitHandler()
    {
        static THeadlessSession* spSession = nullptr;
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString resultPath = dir.filePath(qsl("exit.txt"));
        const QString name = qsl("Headless Exit Notification");
        QVERIFY(writeProfile(name, QString()));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        spSession = pSession;
        int heard = 0;
        connect(pSession, &THeadlessSession::notificationRequested, this, [&heard]() {
            ++heard;
        });
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        lua_register(pSession->host()->getLuaInterpreter()->getLuaGlobalState(), "deleteSession", [](lua_State*) -> int {
            delete spSession;
            return 0;
        });
        QVERIFY(pSession->runLua(qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  deleteSession()
  local shown, why = showNotification("bye")
  local file = io.open([[%1]], "w")
  file:write(tostring(shown) .. "|" .. tostring(why))
  file:close()
end))lua")
                                         .arg(resultPath)));
        const QPointer<THeadlessSession> session(pSession);
        QVERIFY(pSession->shutdown(false));
        QVERIFY(session.isNull());
        QCOMPARE(readFile(resultPath), qsl("nil|the profile has no main window"));
        QCOMPARE(heard, 0);
    }

    void test_announcementsReachTheSession()
    {
        const QString name = qsl("Headless Announcements");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy announcements(&session, &THeadlessSession::announcementRequested);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
announce("urgent", "importantall")
announce("latest", "importantmostrecent")
announce("calm")
announce("queued", "all")
)lua")));
        QCOMPARE(announcements.count(), 4);
        QCOMPARE(announcements.at(0).at(0).toString(), name);
        QCOMPARE(announcements.at(0).at(1).toString(), qsl("urgent"));
        QCOMPARE(announcements.at(0).at(2).toBool(), true);
        QCOMPARE(announcements.at(1).at(2).toBool(), true);
        QCOMPARE(announcements.at(2).at(1).toString(), qsl("calm"));
        QCOMPARE(announcements.at(2).at(2).toBool(), false);
        QCOMPARE(announcements.at(3).at(2).toBool(), false);
        QVERIFY(session.shutdown(false));
    }

    // The GUI tells the screen reader only while one is running, which it never is offscreen
    void test_hiddenAndRevealedLinksAreAnnounced()
    {
        const QString name = qsl("Headless Link Announcements");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy announcements(&session, &THeadlessSession::announcementRequested);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(feedDelayedLink(qsl("SECRETWORD"))));
        QTRY_COMPARE_WITH_TIMEOUT(announcements.count(), 1, 10000);
        QCOMPARE(announcements.at(0).at(0).toString(), name);
        QCOMPARE(announcements.at(0).at(1).toString(), qsl("Link revealed: SECRETWORD"));
        QCOMPARE(announcements.at(0).at(2).toBool(), false);

        // As a click on the link would; there is nothing to click here
        THyperlinkVisibilityManager& links = session.host()->mainConsoleModel().mHyperlinkVisibilityManager;
        QCOMPARE(links.trackedLinkIds().size(), 1);
        links.concealLink(*links.trackedLinkIds().cbegin());
        QTRY_COMPARE_WITH_TIMEOUT(announcements.count(), 2, 10000);
        QCOMPARE(announcements.at(1).at(1).toString(), qsl("Link hidden"));
        QVERIFY(session.shutdown(false));
    }

    void test_theGamesSecurePortIsOfferedAndAnswered()
    {
#if defined(QT_NO_SSL)
        QSKIP("Built without SSL support, so no secure port is offered");
#else
        const QString name = qsl("Headless Secure Port");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        // A name: a certificate seldom carries an address, so an address is never offered a secure port
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("localhost")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy offers(&session, &THeadlessSession::secureConnectionOffered);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QVERIFY(!session.answerSecureConnectionOffer(false));
        // The save writeProfile() makes leaves out the attribute, which loads as false
        session.host()->mAskTlsAvailable = true;

        game->write(msspTlsPort("4433"));
        QTRY_COMPARE_WITH_TIMEOUT(offers.count(), 1, 10000);
        QCOMPARE(offers.at(0).at(0).toString(), name);
        QVERIFY2(offers.at(0).at(2).toString().contains(qsl("4433")), qPrintable(offers.at(0).at(2).toString()));

        QVERIFY(session.answerSecureConnectionOffer(false));
        QVERIFY(!session.host()->mAskTlsAvailable);
        QVERIFY(!session.answerSecureConnectionOffer(false));
        // Declining reconnects where it was, which has to have happened before the game goes
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        QVERIFY(session.shutdown(false));
#endif
    }

    // Not left for the next connection, which the player was not asked about and which may offer its own
    void test_aSecurePortOfferLapsesWithItsConnection()
    {
#if defined(QT_NO_SSL)
        QSKIP("Built without SSL support, so no secure port is offered");
#else
        const QString name = qsl("Headless Secure Port Lapses");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("localhost")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy offers(&session, &THeadlessSession::secureConnectionOffered);
        QSignalSpy disconnections(&session, &THeadlessSession::disconnected);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        session.host()->mAskTlsAvailable = true;
        game->write(msspTlsPort("4433"));
        QTRY_COMPARE_WITH_TIMEOUT(offers.count(), 1, 10000);

        game->disconnectFromHost();
        QTRY_COMPARE_WITH_TIMEOUT(disconnections.count(), 1, 10000);
        QVERIFY(session.runLua(qsl("reconnect()")));
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        game.reset(server.nextPendingConnection());
        QTRY_COMPARE_WITH_TIMEOUT(session.host()->mTelnet.getConnectionState(), QAbstractSocket::ConnectedState, 10000);
        QVERIFY(!session.answerSecureConnectionOffer(false));
        QVERIFY(session.host()->mAskTlsAvailable);
        QCOMPARE(disconnections.count(), 1);

        game->write(msspTlsPort("4433"));
        QTRY_COMPARE_WITH_TIMEOUT(offers.count(), 2, 10000);
        // Taken back before the player says yes
        game->write(msspTlsPort("-1"));
        QTRY_COMPARE_WITH_TIMEOUT(session.host()->mMSSPTlsPort, 0, 10000);
        QVERIFY(!session.answerSecureConnectionOffer(true));
        QVERIFY(!session.errorString().isEmpty());
        QCOMPARE(session.host()->getPort(), server.serverPort());
        QVERIFY(session.shutdown(false));
#endif
    }

    // The answer reconnects, which runs the event loop while the profile's connection is still on the stack
    void test_aSecurePortAnswerKeepsTheProfileOpenUntilItReturns()
    {
#if defined(QT_NO_SSL)
        QSKIP("Built without SSL support, so no secure port is offered");
#else
        const QString name = qsl("Headless Secure Port Answer Busy");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("localhost")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy offers(&session, &THeadlessSession::secureConnectionOffered);
        std::optional<bool> shutDownFromHandler;
        connect(&session, &THeadlessSession::disconnected, &session, [&]() {
            if (!shutDownFromHandler.has_value()) {
                shutDownFromHandler = session.shutdown(false);
            }
        });
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        session.host()->mAskTlsAvailable = true;
        game->write(msspTlsPort("4433"));
        QTRY_COMPARE_WITH_TIMEOUT(offers.count(), 1, 10000);

        // Not yet written out, so the connection is still closing when the answer reconnects
        QVERIFY(session.submit(qsl("look")));
        QVERIFY(session.answerSecureConnectionOffer(false));
        QCOMPARE(shutDownFromHandler, std::optional<bool>(false));
        QVERIFY(session.shutdown(false));
#endif
    }

    void test_aGamesPackageDownloadIsFollowedAndCancelled()
    {
        // Sends part of what it promises, a little at a time, as Qt reports progress at most every 100ms
        QTcpServer packageServer;
        QVERIFY(packageServer.listen(QHostAddress::LocalHost));
        QPointer<QTcpSocket> packageClient;
        QTimer drip;
        drip.setInterval(150);
        connect(&packageServer, &QTcpServer::newConnection, &packageServer, [&]() {
            packageClient = packageServer.nextPendingConnection();
            connect(packageClient, &QTcpSocket::readyRead, packageClient, [&]() {
                if (!packageClient->readAll().isEmpty() && !drip.isActive()) {
                    packageClient->write("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 1000000\r\n\r\n");
                    drip.start();
                }
            });
        });
        connect(&drip, &QTimer::timeout, &drip, [&]() {
            if (packageClient) {
                packageClient->write(QByteArray(1000, 'x'));
            }
        });

        const QString name = qsl("Headless Package Download");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QStringList events;
        connect(&session, &THeadlessSession::packageDownloadStarted, &session, [&events](const QString&, const QString&) {
            events.append(qsl("started"));
        });
        connect(&session, &THeadlessSession::packageDownloadProgress, &session, [&events](const QString&, qint64 got) {
            if (got > 0 && (events.isEmpty() || events.last() != qsl("progress"))) {
                events.append(qsl("progress"));
            }
        });
        connect(&session, &THeadlessSession::packageDownloadFinished, &session, [&events](const QString&) {
            events.append(qsl("finished"));
        });
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        // Where the download has ended but the frontend has not yet been told
        std::optional<bool> cancelledOnceEnded;
        connect(&session.host()->mTelnet, &cTelnet::signal_packageDownloadFinished, &session, [&]() {
            cancelledOnceEnded = session.cancelPackageDownload();
        });
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QVERIFY(!session.cancelPackageDownload());
        // The save writeProfile() makes leaves out the attributes, which load as false
        session.host()->mEnableGMCP = true;
        session.host()->mAcceptServerGUI = true;

        // IAC WILL GMCP, then IAC SB GMCP Client.GUI {...} IAC SE
        game->write(QByteArray("\xff\xfb\xc9"));
        QByteArray fromClient;
        QTRY_VERIFY_WITH_TIMEOUT((fromClient += game->readAll()).contains(QByteArray("\xff\xfd\xc9")), 10000);
        const QString url = qsl("http://127.0.0.1:%1/headless-game-ui.mpackage").arg(packageServer.serverPort());
        game->write(QByteArray("\xff\xfa\xc9") + qsl("Client.GUI {\"version\": \"1\", \"url\": \"%1\"}").arg(url).toUtf8() + QByteArray("\xff\xf0"));

        QTRY_COMPARE_WITH_TIMEOUT(events, (QStringList{qsl("started"), qsl("progress")}), 10000);
        QVERIFY(session.cancelPackageDownload());
        QTRY_COMPARE_WITH_TIMEOUT(events, (QStringList{qsl("started"), qsl("progress"), qsl("finished")}), 10000);
        QCOMPARE(cancelledOnceEnded, std::optional<bool>(false));
        QVERIFY(!session.cancelPackageDownload());
        drip.stop();
        QVERIFY(session.shutdown(false));
    }

    // Its end, sent from the event loop, would come once the profile it belongs to has gone
    void test_aShutdownEndsAGamesPackageDownload()
    {
        QTcpServer packageServer;
        QVERIFY(packageServer.listen(QHostAddress::LocalHost));
        QPointer<QTcpSocket> packageClient;
        connect(&packageServer, &QTcpServer::newConnection, &packageServer, [&]() {
            packageClient = packageServer.nextPendingConnection();
            connect(packageClient, &QTcpSocket::readyRead, packageClient, [&]() {
                if (!packageClient->readAll().isEmpty()) {
                    packageClient->write("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: 1000000\r\n\r\n");
                    packageClient->write(QByteArray(1000, 'x'));
                }
            });
        });

        const QString name = qsl("Headless Package Download Shutdown");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QStringList events;
        connect(&session, &THeadlessSession::packageDownloadStarted, &session, [&events](const QString&, const QString&) {
            events.append(qsl("started"));
        });
        connect(&session, &THeadlessSession::packageDownloadFinished, &session, [&events, name](const QString& profile) {
            QCOMPARE(profile, name);
            events.append(qsl("finished"));
        });
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        session.host()->mEnableGMCP = true;
        session.host()->mAcceptServerGUI = true;
        game->write(QByteArray("\xff\xfb\xc9"));
        QByteArray fromClient;
        QTRY_VERIFY_WITH_TIMEOUT((fromClient += game->readAll()).contains(QByteArray("\xff\xfd\xc9")), 10000);
        const QString url = qsl("http://127.0.0.1:%1/headless-game-ui.mpackage").arg(packageServer.serverPort());
        game->write(QByteArray("\xff\xfa\xc9") + qsl("Client.GUI {\"version\": \"1\", \"url\": \"%1\"}").arg(url).toUtf8() + QByteArray("\xff\xf0"));
        QTRY_COMPARE_WITH_TIMEOUT(events, QStringList{qsl("started")}, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(session.host()->mTelnet.packageDownloadRunning(), 10000);

        QVERIFY(session.shutdown(false));
        QCOMPARE(events, (QStringList{qsl("started"), qsl("finished")}));
    }

    // Records started, each run of changes as one, and finished, with every maximum given
    static void followMapProgress(THeadlessSession& session, const QString& name, QStringList& events, QList<int>& maximums)
    {
        connect(&session, &THeadlessSession::mapProgressStarted, &session, [&events, &maximums, name](const QString& profile, const QString&, const QString&, const int maximum) {
            QCOMPARE(profile, name);
            events.append(qsl("started"));
            maximums.append(maximum);
        });
        connect(&session, &THeadlessSession::mapProgressChanged, &session, [&events, &maximums](const QString&, const QString& label, const int, const int maximum) {
            QVERIFY(!label.isEmpty());
            maximums.append(maximum);
            if (events.isEmpty() || events.last() != qsl("changed")) {
                events.append(qsl("changed"));
            }
        });
        connect(&session, &THeadlessSession::mapProgressFinished, &session, [&events, name](const QString& profile) {
            QCOMPARE(profile, name);
            events.append(qsl("finished"));
        });
    }

    static QString threeRoomsSavedTo(const QString& path)
    {
        // The map's Lua takes an open map
        return qsl(R"lua(
openMapWidget()
local area = addAreaName("Headless Progress")
for id = 1, 3 do
  addRoom(id)
  setRoomArea(id, area)
end
headlessMapSaved, headlessMapError = saveJsonMap([[%1]])
)lua")
                .arg(path);
    }

    void test_aMapExportAndImportAreFollowed()
    {
        const QString name = qsl("Headless Map Progress");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QStringList events;
        QList<int> maximums;
        followMapProgress(session, name, events, maximums);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QString path = mConfigDir.filePath(qsl("headless-map-progress.json"));
        QVERIFY(session.runLua(threeRoomsSavedTo(path)));
        QVERIFY2(session.runLua(qsl("assert(headlessMapSaved == true, tostring(headlessMapError))")), qPrintable(session.errorString()));
        QTRY_COMPARE_WITH_TIMEOUT(events, (QStringList{qsl("started"), qsl("changed"), qsl("finished")}), 10000);
        QVERIFY(std::all_of(maximums.cbegin(), maximums.cend(), [](const int maximum) {
            return maximum == 3;
        }));

        events.clear();
        maximums.clear();
        QVERIFY(session.runLua(qsl("headlessMapLoaded = loadJsonMap([[%1]])").arg(path)));
        QVERIFY(session.runLua(qsl("assert(headlessMapLoaded == true)")));
        QTRY_COMPARE_WITH_TIMEOUT(events, (QStringList{qsl("started"), qsl("changed"), qsl("finished")}), 10000);
        QVERIFY(std::all_of(maximums.cbegin(), maximums.cend(), [](const int maximum) {
            return maximum == 3;
        }));

        // Its end is sent from the event loop once the script has returned, which a shutdown() at once must not lose
        events.clear();
        QVERIFY(session.runLua(qsl("saveJsonMap([[%1]])").arg(path)));
        QCOMPARE(events, (QStringList{qsl("started"), qsl("changed")}));
        QVERIFY(session.shutdown(false));
        QCOMPARE(events, (QStringList{qsl("started"), qsl("changed"), qsl("finished")}));

        // As for a JSON map that start() loads
        const QString mapsFolder = MudletApp::getMudletPath(enums::profileMapsPath, name);
        QVERIFY(QDir().mkpath(mapsFolder));
        QVERIFY(QFile::copy(path, qsl("%1/headless-map-progress.json").arg(mapsFolder)));
        events.clear();
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.shutdown(false));
        QCOMPARE(events, (QStringList{qsl("started"), qsl("changed"), qsl("finished")}));
    }

    void test_aFailedMapExportEnds()
    {
        const QString name = qsl("Headless Map Export Fails");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QStringList events;
        QList<int> maximums;
        followMapProgress(session, name, events, maximums);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(threeRoomsSavedTo(mConfigDir.filePath(qsl("no such folder/headless-map-progress.json")))));
        QVERIFY2(session.runLua(qsl("assert(headlessMapSaved == nil and headlessMapError:find('could not open save file', 1, true), tostring(headlessMapError))")), qPrintable(session.errorString()));
        QTRY_COMPARE_WITH_TIMEOUT(events, (QStringList{qsl("started"), qsl("changed"), qsl("finished")}), 10000);
        QVERIFY(session.shutdown(false));
    }

    // The end a closed profile is owed comes before quitRequested, whose handler would otherwise close what the
    // end's own handler opened next
    void test_aHandlerOfAClosedProfilesEndMayStartAnother()
    {
        QTcpServer stalled;
        QVERIFY(stalled.listen(QHostAddress::LocalHost));
        std::vector<std::unique_ptr<QTcpSocket>> held;
        connect(&stalled, &QTcpServer::newConnection, &stalled, [&]() {
            held.emplace_back(stalled.nextPendingConnection());
        });
        const QString name = qsl("Headless Map Download Closed");
        const QString next = qsl("Headless Map Download Next");
        QVERIFY(writeProfile(name, QString()));
        QVERIFY(writeProfile(next, QString()));
        THeadlessSession session;
        QSignalSpy started(&session, &THeadlessSession::mapProgressStarted);
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        std::optional<THeadlessSession::StartResult> restarted;
        connect(&session, &THeadlessSession::mapProgressFinished, &session, [&](const QString& profile) {
            QCOMPARE(profile, name);
            QVERIFY(session.shutdown(false));
            restarted = session.start({.profile = next, .connect = false});
        });
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        session.host()->mpMap->downloadMap(qsl("http://127.0.0.1:%1/map.xml").arg(stalled.serverPort()), mConfigDir.filePath(qsl("headless-map-closed.xml")));
        QTRY_COMPARE_WITH_TIMEOUT(started.count(), 1, 10000);

        QVERIFY(session.runLua(qsl("closeProfile()")));
        QTRY_VERIFY_WITH_TIMEOUT(restarted.has_value(), 10000);
        QCOMPARE(*restarted, THeadlessSession::StartResult::started);
        QTest::qWait(100);
        QCOMPARE(quit.count(), 0);
        QVERIFY(session.host());
        QCOMPARE(session.host()->getName(), next);
        QVERIFY(session.shutdown(false));
    }

    // A transfer is started knowing nothing of how many rooms it goes through
    void test_anXmlMapImportIsFollowed()
    {
        const QString name = qsl("Headless Xml Map Progress");
        QVERIFY(writeProfile(name, QString()));
        const QString path = mConfigDir.filePath(qsl("headless-map-progress.xml"));
        QVERIFY(writeFile(path, R"(<?xml version="1.0" encoding="UTF-8"?>
<map>
  <areas><area id="5001" name="Headless Xml Area"/></areas>
  <rooms>
    <room id="5001" area="5001" title="One"><coord x="0" y="0" z="0"/></room>
    <room id="5002" area="5001" title="Two"><coord x="1" y="0" z="0"/></room>
  </rooms>
</map>
)"));
        THeadlessSession session;
        QStringList events;
        QList<int> maximums;
        followMapProgress(session, name, events, maximums);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("openMapWidget(); headlessMapLoaded = loadMap([[%1]])").arg(path)));
        QVERIFY(session.runLua(qsl("assert(headlessMapLoaded == true)")));
        QTRY_COMPARE_WITH_TIMEOUT(events, (QStringList{qsl("started"), qsl("changed"), qsl("finished")}), 10000);
        QCOMPARE(maximums.constFirst(), 0);
        QVERIFY(maximums.constLast() > 0);
        QVERIFY(session.shutdown(false));
    }

    void test_aMapExportIsCancelledButAnXmlImportIsNot()
    {
        const QString name = qsl("Headless Map Cancel");
        QVERIFY(writeProfile(name, QString()));
        const QString xmlPath = mConfigDir.filePath(qsl("headless-map-cancel.xml"));
        QVERIFY(writeFile(xmlPath, R"(<?xml version="1.0" encoding="UTF-8"?>
<map>
  <areas><area id="5001" name="Headless Cancel Area"/></areas>
  <rooms><room id="5001" area="5001" title="Room"><coord x="0" y="0" z="0"/></room></rooms>
</map>
)"));
        THeadlessSession session;
        bool armed = true;
        std::optional<bool> cancelled;
        connect(&session, &THeadlessSession::mapProgressStarted, &session, [&]() {
            if (std::exchange(armed, false)) {
                cancelled = session.cancelMapOperation();
            }
        });
        QSignalSpy finished(&session, &THeadlessSession::mapProgressFinished);
        QSignalSpy cancelDisabled(&session, &THeadlessSession::mapProgressCancelDisabled);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(!session.cancelMapOperation());

        const QString path = mConfigDir.filePath(qsl("headless-map-cancel.json"));
        QVERIFY(session.runLua(threeRoomsSavedTo(path)));
        QCOMPARE(cancelled, std::optional<bool>(true));
        QVERIFY2(session.runLua(qsl("assert(headlessMapSaved == nil and headlessMapError == 'aborted by user', tostring(headlessMapError))")), qPrintable(session.errorString()));
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 10000);
        QCOMPARE(cancelDisabled.count(), 0);
        QVERIFY(!QFile::exists(path));
        QVERIFY(!session.cancelMapOperation());

        // Writing its file, it can no longer be aborted
        QVERIFY(session.runLua(qsl("headlessMapSaved = saveJsonMap([[%1]])").arg(path)));
        QVERIFY(session.runLua(qsl("assert(headlessMapSaved == true)")));
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 2, 10000);
        QCOMPARE(cancelDisabled.count(), 1);

        armed = true;
        cancelled.reset();
        QVERIFY(session.runLua(qsl("headlessMapLoaded = loadMap([[%1]])").arg(xmlPath)));
        QVERIFY(session.runLua(qsl("assert(headlessMapLoaded == true)")));
        QCOMPARE(cancelled, std::optional<bool>(false));
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 3, 10000);
        QCOMPARE(cancelDisabled.count(), 2);
        QCoreApplication::processEvents();
        QCOMPARE(finished.count(), 3);
        QVERIFY(session.shutdown(false));
    }

    void test_aJsonMapImportIsCancelled()
    {
        const QString name = qsl("Headless Map Import Cancel");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        bool armed = false;
        std::optional<bool> cancelled;
        connect(&session, &THeadlessSession::mapProgressStarted, &session, [&]() {
            if (std::exchange(armed, false)) {
                cancelled = session.cancelMapOperation();
            }
        });
        QSignalSpy finished(&session, &THeadlessSession::mapProgressFinished);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QString path = mConfigDir.filePath(qsl("headless-map-import-cancel.json"));
        QVERIFY(session.runLua(threeRoomsSavedTo(path)));
        QVERIFY2(session.runLua(qsl("assert(headlessMapSaved == true, tostring(headlessMapError))")), qPrintable(session.errorString()));
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 10000);

        QVERIFY(session.runLua(qsl(R"lua(
function headlessMapNow()
  local names, rooms = {}, {}
  for areaName, id in pairs(getAreaTable()) do
    names[#names + 1] = areaName .. "=" .. id
  end
  for id in pairs(getRooms()) do
    rooms[#rooms + 1] = id
  end
  table.sort(names)
  table.sort(rooms)
  return table.concat(names, ",") .. ";" .. table.concat(rooms, ",") .. ";" .. getRoomAreaName(-1)
end
addRoom(4)
setRoomArea(4, addAreaName("Headless Added Later"))
headlessMapBefore = headlessMapNow()
)lua")));
        armed = true;
        QVERIFY(session.runLua(qsl("headlessMapLoaded, headlessMapError = loadJsonMap([[%1]])").arg(path)));
        QCOMPARE(cancelled, std::optional<bool>(true));
        QVERIFY2(session.runLua(qsl("assert(headlessMapLoaded == nil and headlessMapError == 'aborted by user', tostring(headlessMapError))")), qPrintable(session.errorString()));
        QVERIFY2(session.runLua(qsl("assert(headlessMapNow() == headlessMapBefore, headlessMapNow())")), qPrintable(session.errorString()));
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 2, 10000);
        QCoreApplication::processEvents();
        QCOMPARE(finished.count(), 2);
        QVERIFY(session.shutdown(false));
    }

    // From a handler of its start, while start() is still loading the profile
    void test_aJsonMapThatStartLoadsIsCancelled()
    {
        const QString name = qsl("Headless Map Load Cancel");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QString path = mConfigDir.filePath(qsl("headless-map-load-cancel.json"));
        QVERIFY(session.runLua(threeRoomsSavedTo(path)));
        QVERIFY2(session.runLua(qsl("assert(headlessMapSaved == true, tostring(headlessMapError))")), qPrintable(session.errorString()));
        QVERIFY(session.shutdown(false));
        const QString mapsFolder = MudletApp::getMudletPath(enums::profileMapsPath, name);
        QVERIFY(QDir().mkpath(mapsFolder));
        QVERIFY(QFile::copy(path, qsl("%1/headless-map-load-cancel.json").arg(mapsFolder)));

        std::optional<bool> cancelled;
        QString refusal;
        connect(&session, &THeadlessSession::mapProgressStarted, &session, [&]() {
            if (!cancelled) {
                cancelled = session.cancelMapOperation();
                refusal = session.errorString();
            }
        });
        QSignalSpy finished(&session, &THeadlessSession::mapProgressFinished);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY2(cancelled == std::optional<bool>(true), qPrintable(refusal));
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 10000);
        QVERIFY2(session.runLua(qsl("assert(next(getRooms()) == nil)")), qPrintable(session.errorString()));
        QVERIFY(session.shutdown(false));
    }

    // Once its request is sent, so from the frontend's own event loop rather than a script's
    void test_aMapDownloadIsCancelled()
    {
        QTcpServer stalled;
        QVERIFY(stalled.listen(QHostAddress::LocalHost));
        std::vector<std::unique_ptr<QTcpSocket>> held;
        connect(&stalled, &QTcpServer::newConnection, &stalled, [&]() {
            held.emplace_back(stalled.nextPendingConnection());
        });

        const QString name = qsl("Headless Map Download Cancel");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy started(&session, &THeadlessSession::mapProgressStarted);
        QSignalSpy finished(&session, &THeadlessSession::mapProgressFinished);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QString localFile = mConfigDir.filePath(qsl("headless-map-download.xml"));
        session.host()->mpMap->downloadMap(qsl("http://127.0.0.1:%1/map.xml").arg(stalled.serverPort()), localFile);
        QTRY_COMPARE_WITH_TIMEOUT(started.count(), 1, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(!held.empty(), 10000);

        QVERIFY2(session.cancelMapOperation(), qPrintable(session.errorString()));
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 10000);
        QVERIFY(!session.cancelMapOperation());
        QCoreApplication::processEvents();
        QCOMPARE(finished.count(), 1);
        QVERIFY(!QFile::exists(localFile));
        QVERIFY(session.shutdown(false));
    }

    void test_aMapOpenedWithNoMappingScriptIsReported()
    {
        const QString name = qsl("Headless No Mapping Script");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy missing(&session, &THeadlessSession::mappingScriptMissing);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        // A shutdown() before the event loop runs saves that it was asked, so must not lose it
        QVERIFY(session.runLua(qsl("openMapWidget()")));
        QVERIFY(session.shutdown(true));
        QTRY_COMPARE_WITH_TIMEOUT(missing.count(), 1, 10000);
        QCOMPARE(missing.at(0).at(0).toString(), name);

        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("openMapWidget()")));
        QTest::qWait(100);
        QCOMPARE(missing.count(), 1);
        QVERIFY(session.shutdown(false));

        const QString scripted = qsl("Headless Mapping Script");
        QVERIFY(writeProfile(scripted, qsl("mudlet = mudlet or {}; mudlet.mapper_script = true")));
        QCOMPARE(session.start({.profile = scripted, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("openMapWidget()")));
        QTest::qWait(100);
        QCOMPARE(missing.count(), 1);
        QVERIFY(session.shutdown(false));

        // So the two above are kept quiet by what they say, not by a session that no longer relays it
        const QString another = qsl("Headless No Mapping Script Either");
        QVERIFY(writeProfile(another, QString()));
        QCOMPARE(session.start({.profile = another, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("openMapWidget()")));
        QTRY_COMPARE_WITH_TIMEOUT(missing.count(), 2, 10000);
        QCOMPARE(missing.at(1).at(0).toString(), another);
        QVERIFY(session.shutdown(false));
    }

    // A prompt the game ends with GA, and commands as the command line sends them, a trailing line feed
    // making an empty one as there
    void test_promptsAndCommandsReachTheGame()
    {
        const QString name = qsl("Headless Prompt");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        game->write("Welcome\r\nhp 10> \xff\xf9");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("hp 10> ")), 10000);
        const auto prompt = output.at(outputTexts(output).indexOf(qsl("hp 10> ")));
        QCOMPARE(prompt.at(2).toBool(), true);
        QCOMPARE(output.at(outputTexts(output).indexOf(qsl("Welcome"))).at(2).toBool(), false);

        QVERIFY(session.submit(qsl("north\n")));
        QByteArray received;
        QTRY_VERIFY_WITH_TIMEOUT((received += game->readAll()).contains("north\r\n\r\n"), 10000);
    }

    // What the GUI writes into a trigger's line or a prompt's, which is not at the end as print() writes, keeps
    // the lines the GUI shows: an echo into a trigger's line ends with that line
    void test_commandEchoesAndTriggerEchoesReachLocalText()
    {
        const QString name = qsl("Headless Local Text");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempTrigger("Welcome", [[echo(" <tagged>") echo(" <twice>") send("from a trigger")]])
tempTrigger("Second", [[echo(" <second>")]])
tempTrigger("Third", [[echo(" <third>")]]))lua")));
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QSignalSpy local(&session, &THeadlessSession::localText);
        game->write("Welcome\r\nSecond\r\nThird\r\nhp 10> \xff\xf9");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("hp 10> ")), 10000);
        // Into the prompt's line, then on lines of their own
        QVERIFY(session.submit(qsl("look")));
        QVERIFY(session.runLua(qsl(R"lua(echo("later\n"))lua")));
        QVERIFY(session.submit(qsl("again")));
        QCOMPARE(outputTexts(local).join(QString()), qsl(" <tagged> <twice>\nfrom a trigger\n <second>\n <third>\nlook\nlater\nagain\n"));
        QCOMPARE(local.constFirst().at(0).toString(), name);
    }

    // As the GUI, which shows neither: the game asked for its echo to be off, as for a password
    void test_commandsSentWithTheEchoOffDoNotReachLocalText()
    {
        const QString name = qsl("Headless Echo Off");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempTrigger("Password", [[send("trigger secret")]]))lua")));
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QSignalSpy local(&session, &THeadlessSession::localText);
        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Password:")), 10000);
        QVERIFY(session.submit(qsl("typed secret")));
        QByteArray received;
        QTRY_VERIFY_WITH_TIMEOUT((received += game->readAll()).contains("typed secret"), 10000);
        QVERIFY2(received.contains("trigger secret"), received.constData());
        const QString shown = outputTexts(local).join(QString());
        QVERIFY2(!shown.contains(qsl("secret")), qPrintable(shown));
    }

    void test_theGameHidingWhatIsTypedReachesTheSession_data()
    {
        QTest::addColumn<bool>("passwordsShown");
        QTest::addColumn<QString>("endedBy");
        QTest::newRow("masked, game hangs up") << false << qsl("game");
        QTest::newRow("shown as typed") << true << qsl("game");
        QTest::newRow("masked, shut down") << false << qsl("shutdown");
        QTest::newRow("masked, script closes the profile") << false << qsl("script");
        // Read while the shutdown's close runs the event loop, as a save it waits for does
        QTest::newRow("masked, game stops hiding as it shuts down") << false << qsl("released while closing");
        QTest::newRow("masked, game hides again as it shuts down") << false << qsl("hidden while closing");
    }

    // Where the GUI opens and closes its hidden-input box, which a disconnect or the profile closing closes too
    void test_theGameHidingWhatIsTypedReachesTheSession()
    {
        QFETCH(const bool, passwordsShown);
        QFETCH(const QString, endedBy);
        const QString name = qsl("Headless Hidden Input");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy hidden(&session, &THeadlessSession::hiddenInputWanted);
        QSignalSpy disconnected(&session, &THeadlessSession::disconnected);
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        session.host()->setDisablePasswordMasking(passwordsShown);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QVERIFY(hidden.isEmpty());

        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Password:")), 10000);
        game->write("\xff\xfc\x01Welcome\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Welcome")), 10000);
        game->write("\xff\xfb\x01Password again:\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Password again:")), 10000);
        if (endedBy == qsl("game")) {
            game->close();
            QTRY_VERIFY_WITH_TIMEOUT(!disconnected.isEmpty(), 10000);
        } else if (endedBy.endsWith(qsl("while closing"))) {
            const auto restore = inTestMode();
            QSignalSpy errors(&session, &THeadlessSession::errorConsoleText);
            QVERIFY(session.runLua(qsl("registerAnonymousEventHandler('sysExitEvent', function() local ok, err = pumpEvents(200) if not ok then error(err) end end)")));
            if (endedBy == qsl("hidden while closing")) {
                game->write("\xff\xfc\x01Welcome back\r\n");
                QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Welcome back")), 10000);
                QCOMPARE(hidden.size(), 4);
                game->write("\xff\xfb\x01Read while closing\r\n");
            } else {
                game->write("\xff\xfc\x01Read while closing\r\n");
            }
            game->flush();
            QVERIFY(!outputTexts(output).contains(qsl("Read while closing")));
            QVERIFY(session.shutdown(true));
            QString errorText;
            for (const QList<QVariant>& error : errors) {
                errorText += error.at(1).toString();
            }
            QCOMPARE(errorText, QString());
            QVERIFY(outputTexts(output).contains(qsl("Read while closing")));
        } else if (endedBy == qsl("shutdown")) {
            QVERIFY(session.shutdown(false));
        } else {
            QVERIFY(session.runLua(qsl("closeProfile()")));
            QTRY_COMPARE_WITH_TIMEOUT(quit.count(), 1, 10000);
        }

        QList<bool> wanted;
        for (const QList<QVariant>& emitted : hidden) {
            QCOMPARE(emitted.at(0).toString(), name);
            wanted << emitted.at(1).toBool();
        }
        QCOMPARE(wanted, passwordsShown ? QList<bool>() : (QList<bool>{true, false, true, false}));
        if (session.host()) {
            QVERIFY(session.shutdown(false));
        }
        QCOMPARE(hidden.count(), wanted.size());
    }

    // As the GUI's hidden-input box sends it, and as its Esc on an empty box closes it
    void test_hiddenInputGoesStraightToTheGame()
    {
        const QString name = qsl("Headless Hidden Send");
        QVERIFY(writeProfile(name, QString()));
        const QString historyFile = MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("command_history_main"));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy hidden(&session, &THeadlessSession::hiddenInputWanted);
        QSignalSpy disconnected(&session, &THeadlessSession::disconnected);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempAlias("^typed", [[send("alias ran")]])
registerAnonymousEventHandler("sysDataSendRequest", function(_, command) hiddenSendSeen = command end))lua")));
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QSignalSpy local(&session, &THeadlessSession::localText);

        QVERIFY(!session.submitHiddenInput(qsl("too soon")));
        QCOMPARE(session.errorString(), qsl("The hidden input was not sent: the game is not hiding what is typed."));
        QVERIFY(!session.dismissHiddenInput());
        QCOMPARE(session.errorString(), qsl("Hidden input is not open: the game is not hiding what is typed."));
        QVERIFY(!session.hiddenInputEdited());

        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Password:")), 10000);
        QVERIFY2(session.submitHiddenInput(qsl("typed sec\r\nret")), qPrintable(session.errorString()));
        QByteArray received;
        QTRY_VERIFY_WITH_TIMEOUT((received += game->readAll()).contains("typed secret\r\n"), 10000);
        QTest::qWait(100);
        received += game->readAll();
        QVERIFY2(!received.contains("alias ran") && !received.contains("too soon"), received.constData());
        QVERIFY(luaGlobalString(session.host(), "hiddenSendSeen").isEmpty());
        const QString shown = outputTexts(local).join(QString());
        QVERIFY2(!shown.contains(qsl("secret")), qPrintable(shown));

        QVERIFY(session.host()->mTelnet.setEncoding("ISO 8859-1", false).first);
        QVERIFY(!session.submitHiddenInput(qsl("caf\u00e9 \u20ac")));
        QCOMPARE(session.errorString(), qsl("The hidden input was not sent: it has characters the game's encoding, ISO 8859-1, cannot carry."));
        QVERIFY2(session.submitHiddenInput(qsl("caf\u00e9")), qPrintable(session.errorString()));
        QTRY_VERIFY_WITH_TIMEOUT((received += game->readAll()).contains("caf\xe9\r\n"), 10000);
        QVERIFY2(!received.contains(" ?"), received.constData());

        QCOMPARE(hidden.size(), 1);
        QVERIFY(session.dismissHiddenInput());
        QCOMPARE(hidden.size(), 2);
        QCOMPARE(hidden.at(1).at(0).toString(), name);
        QCOMPARE(hidden.at(1).at(1).toBool(), false);
        QVERIFY(!session.dismissHiddenInput());
        QVERIFY(!session.submitHiddenInput(qsl("typed again")));
        QCOMPARE(session.errorString(),
                 qsl("The hidden input was not sent: the game hides what is typed, but hidden input was dismissed, waits on the automatic login, or is off as passwords are shown as typed."));
        // After Esc the player's next command goes the command line's way
        QVERIFY(session.submit(qsl("typed a command")));
        QTRY_VERIFY_WITH_TIMEOUT((received += game->readAll()).contains("alias ran"), 10000);
        QCOMPARE(luaGlobalString(session.host(), "hiddenSendSeen"), qsl("alias ran"));
        QVERIFY2(!received.contains("typed again"), received.constData());

        game->close();
        QTRY_VERIFY_WITH_TIMEOUT(!disconnected.isEmpty(), 10000);
        QVERIFY(!session.submitHiddenInput(qsl("too late")));
        QCOMPARE(session.errorString(), qsl("The hidden input was not sent: Mudlet is not connected to the game."));

        QVERIFY2(session.shutdown(true), qPrintable(session.errorString()));
        QVERIFY2(!readFile(historyFile).contains(qsl("secret")), qPrintable(readFile(historyFile)));
    }

    // As typing in the box does: a saved password the keychain gives up only now must not follow the player's own
    void test_editingHiddenInputStopsALateSavedPassword()
    {
        const QString name = qsl("Headless Hidden Edit");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy hidden(&session, &THeadlessSession::hiddenInputWanted);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        // Before the auto-login's first step, with the keychain still to answer
        session.host()->setLogin(qsl("player"));
        session.host()->setPass(QString());
        session.host()->securedPasswordAnswered(false, QString(), qsl("Operation timed out"), true);
        QByteArray received;
        QTRY_VERIFY_WITH_TIMEOUT((received += game->readAll()).contains("player\r\n"), 10000);

        game->write("Password:\r\n\xff\xfb\x01");
        QTRY_COMPARE_WITH_TIMEOUT(hidden.size(), 1, 10000);
        QVERIFY(hidden.at(0).at(1).toBool());
        QVERIFY2(session.hiddenInputEdited(), qPrintable(session.errorString()));
        session.host()->securedPasswordAnswered(true, qsl("stored"), QString(), false);
        QTest::qWait(100);
        received += game->readAll();
        QVERIFY2(!received.contains("stored"), received.constData());
        QVERIFY2(session.submitHiddenInput(qsl("typed")), qPrintable(session.errorString()));
        QTRY_VERIFY_WITH_TIMEOUT((received += game->readAll()).contains("typed\r\n"), 10000);
        QTest::qWait(100);
        received += game->readAll();
        QVERIFY2(!received.contains("stored"), received.constData());
        QVERIFY(session.shutdown(false));
    }

    // What the GUI's main command line shows of a script's text
    void test_whatAScriptPutsInTheCommandLineReachesTheSession()
    {
        const QString name = qsl("Headless Command Line Text");
        QVERIFY(writeProfile(name, qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() printCmdLine("on the way out") end))lua")));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy hidden(&session, &THeadlessSession::hiddenInputWanted);
        QSignalSpy requested(&session, &THeadlessSession::commandLineTextRequested);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        const auto texts = [&requested]() {
            QStringList result;
            for (const auto& arguments : std::as_const(requested)) {
                result << arguments.at(1).toString();
            }
            return result;
        };

        // A non-breaking space as the GUI's line gives it back, as a space
        QVERIFY(session.runLua(qsl(R"lua(printCmdLine("look") appendCmdLine(" north") clearCmdLine() sendCmdLine("say hi") printCmdLine("main", "a\194\160b"))lua")));
        QCOMPARE(texts(), (QStringList{qsl("look"), qsl("look north"), QString(), qsl("say hi"), qsl("a b")}));
        for (const auto& arguments : std::as_const(requested)) {
            QCOMPARE(arguments.at(0).toString(), name);
        }

        // Another command line is not the app's input line
        QVERIFY(session.runLua(qsl(R"lua(createCommandLine("headlessAux", 0, 0, 100, 20) printCmdLine("headlessAux", "x") appendCmdLine("headlessAux", "y") clearCmdLine("headlessAux"))lua")));
        QCOMPARE(requested.size(), 5);

        // Sent as it is set, so a handler is refused as those of the other ...Requested signals are
        std::optional<bool> submittedFromHandler;
        const auto connection = connect(&session, &THeadlessSession::commandLineTextRequested, this, [&session, &submittedFromHandler]() {
            submittedFromHandler = session.submit(qsl("from a handler"));
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl(R"lua(printCmdLine("handled"))lua")));
        disconnect(connection);
        QCOMPARE(submittedFromHandler, std::optional(false));
        QCOMPARE(requested.size(), 6);

        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Password:")), 10000);
        QCOMPARE(hidden.size(), 1);
        QVERIFY(session.runLua(qsl(R"lua(printCmdLine("into the box") appendCmdLine(" too"))lua")));
        QTest::qWait(100);
        QCOMPARE(requested.size(), 6);
        game->write("\xff\xfc\x01Welcome.\r\n");
        QTRY_COMPARE_WITH_TIMEOUT(hidden.size(), 2, 10000);

        // Nor what an exit handler sets as the profile closes
        QVERIFY(session.shutdown(false));
        QCOMPARE(requested.size(), 6);
    }

    // Set by a trigger on a line a go-ahead ends, which the game hides input after in the same read, it comes first
    void test_commandLineTextFromTheLineBeforeHiddenInputComesFirst()
    {
        const QString name = qsl("Headless Command Line Before Hidden");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QStringList heard;
        connect(&session, &THeadlessSession::commandLineTextRequested, this, [&heard](const QString&, const QString& text) {
            heard << qsl("text %1").arg(text);
        });
        connect(&session, &THeadlessSession::hiddenInputWanted, this, [&heard](const QString&, bool wanted) {
            heard << qsl("hidden %1").arg(wanted ? qsl("on") : qsl("off"));
        });
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QVERIFY(session.runLua(qsl(R"lua(tempTrigger("Welcome back.", [[printCmdLine("connect ")]]))lua")));
        game->write("Welcome back.\r\n\xff\xf9\xff\xfb\x01Password: ");
        QTRY_VERIFY_WITH_TIMEOUT(heard.contains(qsl("hidden on")), 10000);
        QTest::qWait(100);
        QCOMPARE(heard, (QStringList{qsl("text connect "), qsl("hidden on")}));
        QVERIFY(session.shutdown(false));
    }

    // closeProfile() raises sysExitEvent before the session hears of the close
    void test_aScriptsCloseSendsNoCommandLineTextFromTheExitHandler()
    {
        const QString name = qsl("Headless Command Line Script Close");
        QVERIFY(writeProfile(name, qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() printCmdLine("on the way out") end))lua")));
        THeadlessSession session;
        QSignalSpy requested(&session, &THeadlessSession::commandLineTextRequested);
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("closeProfile()")));
        QTRY_COMPARE_WITH_TIMEOUT(quit.count(), 1, 10000);
        QCOMPARE(requested.size(), 0);
    }

    // The GUI sends what is typed while the game hides input from its hidden-input box, leaving the main line be
    void test_aLineSubmittedWhileInputIsHiddenIsNotKeptAsTheCommandLine()
    {
        const QString name = qsl("Headless Command Line Hidden Submit");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy hidden(&session, &THeadlessSession::hiddenInputWanted);
        QSignalSpy requested(&session, &THeadlessSession::commandLineTextRequested);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        session.host()->mAutoClearCommandLineAfterSend = false;
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Password:")), 10000);
        QCOMPARE(hidden.size(), 1);
        QVERIFY(session.submit(qsl("secret")));
        QByteArray received;
        QTRY_VERIFY_WITH_TIMEOUT((received += game->readAll()).contains("secret"), 10000);
        game->write("\xff\xfc\x01Welcome.\r\n");
        QTRY_COMPARE_WITH_TIMEOUT(hidden.size(), 2, 10000);

        QVERIFY(session.runLua(qsl(R"lua(headlessLine = "[" .. getCmdLine() .. "]" appendCmdLine("x"))lua")));
        QCOMPARE(luaGlobalString(session.host(), "headlessLine"), qsl("[]"));
        QCOMPARE(requested.size(), 1);
        QCOMPARE(requested.at(0).at(1).toString(), qsl("x"));
        QVERIFY(session.shutdown(false));
    }

    // A paste sent while hidden input is wanted is no line of the GUI's command line, even once an alias of one of its
    // lines ends hidden input
    void test_aPasteSubmittedWhileInputIsHiddenIsNotKeptWhenAnAliasUnmasksIt()
    {
        const QString name = qsl("Headless Command Line Hidden Paste");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy hidden(&session, &THeadlessSession::hiddenInputWanted);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        session.host()->mAutoClearCommandLineAfterSend = false;
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        // As the profile's setting to show passwords as they are typed, which no script can reach
        static QPointer<Host> unmaskedHost;
        unmaskedHost = session.host();
        lua_register(session.host()->getLuaInterpreter()->getLuaGlobalState(), "headlessUnmask", [](lua_State*) -> int {
            if (unmaskedHost) {
                unmaskedHost->setDisablePasswordMasking(true);
            }
            return 0;
        });
        QVERIFY(session.runLua(qsl(R"lua(tempAlias("^unmask$", [[headlessUnmask()]]))lua")));
        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Password:")), 10000);
        QCOMPARE(hidden.size(), 1);
        QVERIFY(session.submit(qsl("unmask\nlook")));
        QVERIFY(!session.host()->passwordEntryWanted());
        QVERIFY(session.runLua(qsl(R"lua(headlessLine = "[" .. getCmdLine() .. "]")lua")));
        QCOMPARE(luaGlobalString(session.host(), "headlessLine"), qsl("[]"));
        QVERIFY(session.commandHistory().isEmpty());
        QVERIFY(session.shutdown(false));
    }

    // As the GUI's command line holds what it sends while its aliases run, then empties if the profile says so
    void test_aSubmittedLineIsClearedAsTheProfileSays_data()
    {
        QTest::addColumn<bool>("autoClear");
        QTest::newRow("cleared after a send") << true;
        QTest::newRow("kept after a send") << false;
    }

    void test_aSubmittedLineIsClearedAsTheProfileSays()
    {
        QFETCH(const bool, autoClear);
        const QString name = qsl("Headless Command Line Cleared");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy requested(&session, &THeadlessSession::commandLineTextRequested);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        session.host()->mAutoClearCommandLineAfterSend = autoClear;
        const auto texts = [&requested]() {
            QStringList result;
            for (const auto& arguments : std::as_const(requested)) {
                result << arguments.at(1).toString();
            }
            return result;
        };
        QVERIFY(session.runLua(qsl(R"lua(tempAlias("^headless alias$", [[headlessHeld = getCmdLine() printCmdLine("set by the alias")]]) printCmdLine("a"))lua")));
        QVERIFY(session.submit(qsl("x")));
        QVERIFY(session.runLua(qsl(R"lua(appendCmdLine("b"))lua")));
        QVERIFY(session.submit(qsl("headless alias")));
        QCOMPARE(luaGlobalString(session.host(), "headlessHeld"), qsl("headless alias"));
        QVERIFY(session.runLua(qsl("headlessAfter = '[' .. getCmdLine() .. ']'")));
        if (autoClear) {
            QCOMPARE(texts(), (QStringList{qsl("a"), QString(), qsl("b"), qsl("set by the alias"), QString()}));
            QCOMPARE(luaGlobalString(session.host(), "headlessAfter"), qsl("[]"));
        } else {
            QCOMPARE(texts(), (QStringList{qsl("a"), qsl("xb"), qsl("set by the alias")}));
            QCOMPARE(luaGlobalString(session.host(), "headlessAfter"), qsl("[set by the alias]"));
        }
        QVERIFY(session.shutdown(false));
    }

    // TCommandLine::enterCommand() empties the GUI's line behind a hidden-input box that an alias's event loop let
    // the game open, so the line the app set aside is empty once that box closes
    void test_aLineSentAsTheGameHidesInputIsClearedOnceItEnds_data()
    {
        QTest::addColumn<bool>("autoClear");
        QTest::addColumn<QStringList>("heard");
        const QStringList hiddenCycle{qsl("text say hi"), qsl("hidden on"), qsl("hidden off")};
        QTest::newRow("cleared after a send") << true << (QStringList(hiddenCycle) << qsl("text "));
        QTest::newRow("kept after a send") << false << hiddenCycle;
    }

    void test_aLineSentAsTheGameHidesInputIsClearedOnceItEnds()
    {
        QFETCH(const bool, autoClear);
        QFETCH(const QStringList, heard);
        const QString name = qsl("Headless Command Line Hidden By An Alias");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QStringList heardNow;
        connect(&session, &THeadlessSession::commandLineTextRequested, this, [&heardNow](const QString&, const QString& text) {
            heardNow << qsl("text %1").arg(text);
        });
        connect(&session, &THeadlessSession::hiddenInputWanted, this, [&heardNow](const QString&, bool wanted) {
            heardNow << qsl("hidden %1").arg(wanted ? qsl("on") : qsl("off"));
        });
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        session.host()->mAutoClearCommandLineAfterSend = autoClear;
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        // As saveJsonMap() and the like run the event loop, where the game's request to hide input can arrive
        static QPointer<Host> waitingHost;
        waitingHost = session.host();
        lua_register(session.host()->getLuaInterpreter()->getLuaGlobalState(), "headlessAwaitHiddenInput", [](lua_State*) -> int {
            QDeadlineTimer deadline(10000);
            while (waitingHost && !waitingHost->passwordEntryWanted() && !deadline.hasExpired()) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            }
            return 0;
        });
        QVERIFY(session.runLua(qsl(R"lua(tempAlias("^say hi$", [[headlessAwaitHiddenInput()]]) printCmdLine("say hi"))lua")));
        game->write("\xff\xfb\x01Password:\r\n");
        QVERIFY(session.submit(qsl("say hi")));
        QVERIFY(session.host()->passwordEntryWanted());
        QCOMPARE(heardNow, (QStringList{qsl("text say hi"), qsl("hidden on")}));
        game->write("\xff\xfc\x01Welcome.\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(heardNow.contains(qsl("hidden off")), 10000);
        QTest::qWait(100);
        QCOMPARE(heardNow, heard);
        QVERIFY(session.runLua(qsl("headlessAfter = '[' .. getCmdLine() .. ']'")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAfter"), autoClear ? qsl("[]") : qsl("[say hi]"));
        QVERIFY(session.shutdown(false));
    }

    // A send whose alias runs the event loop finishes after one an app's timer made meanwhile, once the game hides input
    void test_anOuterSendKeepsTheClearAnInnerSendLeftForHiddenInput()
    {
        const QString name = qsl("Headless Command Line Hidden By A Nested Send");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QStringList heardNow;
        connect(&session, &THeadlessSession::commandLineTextRequested, this, [&heardNow](const QString&, const QString& text) {
            heardNow << qsl("text %1").arg(text);
        });
        connect(&session, &THeadlessSession::hiddenInputWanted, this, [&heardNow](const QString&, bool wanted) {
            heardNow << qsl("hidden %1").arg(wanted ? qsl("on") : qsl("off"));
        });
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        session.host()->mAutoClearCommandLineAfterSend = true;
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        static QPointer<Host> waitingHost;
        static bool innerSent;
        waitingHost = session.host();
        innerSent = false;
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_register(L, "headlessAwaitHiddenInput", [](lua_State*) -> int {
            QDeadlineTimer deadline(10000);
            while (waitingHost && !waitingHost->passwordEntryWanted() && !deadline.hasExpired()) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            }
            return 0;
        });
        lua_register(L, "headlessAwaitInnerSend", [](lua_State*) -> int {
            QDeadlineTimer deadline(10000);
            while (!innerSent && !deadline.hasExpired()) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            }
            return 0;
        });
        QVERIFY(session.runLua(qsl(R"lua(tempAlias("^outer$", [[headlessAwaitInnerSend()]]) tempAlias("^inner$", [[headlessAwaitHiddenInput()]]))lua")));
        QTimer::singleShot(0, this, [&session, &game]() {
            game->write("\xff\xfb\x01Password:\r\n");
            QVERIFY(session.submit(qsl("inner")));
            innerSent = true;
        });
        QVERIFY(session.submit(qsl("outer")));
        QVERIFY(innerSent);
        QVERIFY(session.host()->passwordEntryWanted());
        game->write("\xff\xfc\x01Welcome.\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(heardNow.contains(qsl("hidden off")), 10000);
        QTest::qWait(100);
        QCOMPARE(heardNow, (QStringList{qsl("hidden on"), qsl("hidden off"), qsl("text ")}));
        QVERIFY(session.shutdown(false));
    }

    // As scripts read the GUI's command line, such as a key binding that acts on the line being typed
    void test_whatThePlayerHasTypedReachesTheScripts()
    {
        const QString name = qsl("Headless Typed So Far");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession unstarted;
        QVERIFY(!unstarted.setCommandLineText(qsl("kill rat")));
        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy hidden(&session, &THeadlessSession::hiddenInputWanted);
        QSignalSpy requested(&session, &THeadlessSession::commandLineTextRequested);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QVERIFY(session.runLua(qsl(R"lua(tempKey(mudlet.key.F2, [[send(getCmdLine())]]))lua")));

        QVERIFY(session.setCommandLineText(qsl("kill rat")));
        QCOMPARE(session.pressKey(Qt::Key_F2), THeadlessSession::KeyResult::bindingRan);
        QByteArray received;
        QTRY_VERIFY_WITH_TIMEOUT((received += game->readAll()).contains("kill rat\r\n"), 10000);
        QVERIFY(session.setCommandLineText(QString()));
        QVERIFY(session.runLua(qsl("headlessTyped = '[' .. getCmdLine() .. ']'")));
        QCOMPARE(luaGlobalString(session.host(), "headlessTyped"), qsl("[]"));
        QTest::qWait(50);
        QVERIFY(requested.isEmpty());

        // As the GUI's line gives it back, a non-breaking space is a space
        QVERIFY(session.setCommandLineText(qsl("a\u00a0b")));
        QVERIFY(session.runLua(qsl("headlessTyped = getCmdLine()")));
        QCOMPARE(luaGlobalString(session.host(), "headlessTyped"), qsl("a b"));
        QVERIFY(session.setCommandLineText(QString()));

        // An app that shows what a script sets, and hands its line back once the signal is over, is not sent it again
        QVERIFY(session.runLua(qsl(R"lua(printCmdLine("from a script"))lua")));
        QCOMPARE(requested.size(), 1);
        QVERIFY(session.setCommandLineText(requested.constLast().at(1).toString()));
        QTest::qWait(100);
        QCOMPARE(requested.size(), 1);
        QVERIFY(session.runLua(qsl("headlessTyped = getCmdLine()")));
        QCOMPARE(luaGlobalString(session.host(), "headlessTyped"), qsl("from a script"));

        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Password:")), 10000);
        QCOMPARE(hidden.size(), 1);
        QVERIFY(!session.setCommandLineText(qsl("secret")));
        QCOMPARE(session.errorString(), qsl("The command line takes no text while the game hides what is typed; send it with submitHiddenInput()."));
        // A line a script set is no password, so it stays as the GUI's does
        QVERIFY(session.runLua(qsl("headlessTyped = getCmdLine()")));
        QCOMPARE(luaGlobalString(session.host(), "headlessTyped"), qsl("from a script"));
        // An app that empties its line meanwhile says so
        QVERIFY(session.setCommandLineText(QString()));
        QVERIFY(session.runLua(qsl("headlessTyped = '[' .. getCmdLine() .. ']'")));
        QCOMPARE(luaGlobalString(session.host(), "headlessTyped"), qsl("[]"));
        QVERIFY(session.shutdown(false));
    }

    // As the GUI's hidden-input box takes what was typed ahead of the prompt that opens it, from the command line,
    // typing into a line that is all selected counting as typing on an empty one
    void test_aLineTypedAheadOfAPasswordPromptIsForNoScript_data()
    {
        QTest::addColumn<QString>("before");
        QTest::addColumn<bool>("sendFirst");
        QTest::addColumn<bool>("escape");
        QTest::addColumn<QString>("typed");
        QTest::addColumn<bool>("playerTyped");
        QTest::addColumn<QString>("atPrompt");
        QTest::addColumn<QStringList>("heard");
        const QString moved = qsl("moved");
        const QString hiddenOn = qsl("hidden on");
        QTest::newRow("on an empty line") << QString() << false << false << qsl("hunter2") << true << qsl("[]") << QStringList{moved, hiddenOn};
        QTest::newRow("over the line a send kept") << QString() << true << false << qsl("hunter2") << true << qsl("[]") << QStringList{moved, hiddenOn};
        QTest::newRow("on into the line a send kept") << QString() << true << false << qsl("bobby") << true << qsl("[]") << QStringList{moved, hiddenOn};
        QTest::newRow("over a line a script selected") << qsl(R"lua(printCmdLine("bob") selectCmdLineText())lua") << false << false << qsl("bobcat") << true << qsl("[]")
                                                       << QStringList{qsl("text bob"), moved, hiddenOn};
        QTest::newRow("over a line sendCmdLine set") << qsl(R"lua(sendCmdLine("bob"))lua") << false << false << qsl("bobcat") << true << qsl("[]") << QStringList{qsl("text bob"), moved, hiddenOn};
        QTest::newRow("over a line Escape selected") << qsl(R"lua(printCmdLine("bob"))lua") << false << true << qsl("bobcat") << true << qsl("[]") << QStringList{qsl("text bob"), moved, hiddenOn};
        QTest::newRow("over a line the app selected") << qsl(R"lua(printCmdLine("bob"))lua") << false << false << qsl("hunter2") << true << qsl("[]") << QStringList{qsl("text bob"), moved, hiddenOn};
        QTest::newRow("on after a script's text") << qsl(R"lua(printCmdLine("say "))lua") << false << false << qsl("say hi") << true << qsl("[say hi]") << QStringList{qsl("text say "), hiddenOn};
        QTest::newRow("recalled from the history") << QString() << false << false << qsl("look") << false << qsl("[look]") << QStringList{hiddenOn};
        QTest::newRow("over the line a send kept, recalled") << QString() << true << false << qsl("look") << false << qsl("[look]") << QStringList{hiddenOn};
    }

    void test_aLineTypedAheadOfAPasswordPromptIsForNoScript()
    {
        QFETCH(const QString, before);
        QFETCH(const bool, sendFirst);
        QFETCH(const bool, escape);
        QFETCH(const QString, typed);
        QFETCH(const bool, playerTyped);
        QFETCH(const QString, atPrompt);
        QFETCH(const QStringList, heard);
        const QString name = qsl("Headless Typed Ahead");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QStringList heardNow;
        connect(&session, &THeadlessSession::commandLineTextRequested, this, [&heardNow](const QString&, const QString& text) {
            heardNow << qsl("text %1").arg(text);
        });
        connect(&session, &THeadlessSession::typedLineMoveRequested, this, [&heardNow](const QString&) {
            heardNow << qsl("moved");
        });
        connect(&session, &THeadlessSession::hiddenInputWanted, this, [&heardNow](const QString&, bool wanted) {
            heardNow << qsl("hidden %1").arg(wanted ? qsl("on") : qsl("off"));
        });
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        session.host()->mAutoClearCommandLineAfterSend = false;
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QVERIFY(session.runLua(qsl(R"lua(tempTrigger("Password:", [[headlessAtPrompt = "[" .. getCmdLine() .. "]"]]))lua")));
        if (!before.isEmpty()) {
            QVERIFY(session.runLua(before));
        }
        if (sendFirst) {
            QVERIFY(session.setCommandLineText(qsl("bob")));
            QVERIFY(session.submit(qsl("bob")));
            QVERIFY(session.setCommandLineText(qsl("bob")));
        }
        if (escape) {
            QCOMPARE(session.pressKey(Qt::Key_Escape), THeadlessSession::KeyResult::keyKept);
        }
        QVERIFY(session.setCommandLineText(typed, playerTyped ? THeadlessSession::CommandLineEdit::typed : THeadlessSession::CommandLineEdit::notTyped));
        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_COMPARE_WITH_TIMEOUT(luaGlobalString(session.host(), "headlessAtPrompt"), atPrompt, 10000);
        QCOMPARE(heardNow, heard);
        QVERIFY(session.runLua(qsl("headlessAfter = '[' .. getCmdLine() .. ']'")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAfter"), atPrompt);
        QVERIFY(session.shutdown(false));
    }

    // As the GUI's command line stays behind its hidden-input box, a script's text the app set aside meanwhile is
    // still the script's once it is back
    void test_scriptTextPutBackAfterHiddenInputIsNoTypedLine()
    {
        const QString name = qsl("Headless Typed Put Back");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy hidden(&session, &THeadlessSession::hiddenInputWanted);
        QSignalSpy moved(&session, &THeadlessSession::typedLineMoveRequested);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QVERIFY(session.runLua(qsl(R"lua(tempTrigger("Again:", [[headlessAtPrompt = "[" .. getCmdLine() .. "]"]]))lua")));
        QVERIFY(session.runLua(qsl(R"lua(printCmdLine("from a script"))lua")));
        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_COMPARE_WITH_TIMEOUT(hidden.size(), 1, 10000);
        QVERIFY(session.setCommandLineText(QString()));
        game->write("\xff\xfc\x01Wrong.\r\n");
        QTRY_COMPARE_WITH_TIMEOUT(hidden.size(), 2, 10000);
        QVERIFY(session.setCommandLineText(qsl("from a script"), THeadlessSession::CommandLineEdit::notTyped));
        game->write("\xff\xfb\x01"
                    "Again:\r\n");
        QTRY_COMPARE_WITH_TIMEOUT(luaGlobalString(session.host(), "headlessAtPrompt"), qsl("[from a script]"), 10000);
        QCOMPARE(hidden.size(), 3);
        QVERIFY(moved.isEmpty());
        QVERIFY(session.shutdown(false));
    }

    // Deleting the session is how an app may tear down on the move, which comes from inside the game's read
    void test_theSessionDeletedWhenTheTypedLineMoves()
    {
        const QString name = qsl("Headless Typed Move Delete");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        const QPointer<THeadlessSession> session(pSession);
        int hiddenHeard = 0;
        connect(pSession, &THeadlessSession::hiddenInputWanted, this, [&hiddenHeard]() {
            ++hiddenHeard;
        });
        QCOMPARE(pSession->start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QPointer<Host> profile(pSession->host());
        connect(pSession, &THeadlessSession::typedLineMoveRequested, pSession, [pSession]() {
            delete pSession;
        });
        QVERIFY(pSession->setCommandLineText(qsl("hunter2")));
        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(session.isNull(), 10000);
        QCOMPARE(hiddenHeard, 0);
        QTRY_VERIFY_WITH_TIMEOUT(profile.isNull(), 10000);
    }

    // The app's line needs no emptying after a submit(), as Mudlet says when it empties it
    void test_theTypedLineAfterASubmitIsAsTheProfileLeavesIt_data()
    {
        QTest::addColumn<bool>("autoClear");
        QTest::newRow("cleared after a send") << true;
        QTest::newRow("kept after a send") << false;
    }

    void test_theTypedLineAfterASubmitIsAsTheProfileLeavesIt()
    {
        QFETCH(const bool, autoClear);
        const QString name = qsl("Headless Typed Then Sent");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy requested(&session, &THeadlessSession::commandLineTextRequested);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        session.host()->mAutoClearCommandLineAfterSend = autoClear;
        QVERIFY(session.setCommandLineText(qsl("look")));
        QVERIFY(session.submit(qsl("look")));
        QVERIFY(session.runLua(qsl("headlessAfter = '[' .. getCmdLine() .. ']'")));
        if (autoClear) {
            QCOMPARE(requested.size(), 1);
            QCOMPARE(requested.constFirst().at(1).toString(), QString());
            QCOMPARE(luaGlobalString(session.host(), "headlessAfter"), qsl("[]"));
        } else {
            QVERIFY(requested.isEmpty());
            QCOMPARE(luaGlobalString(session.host(), "headlessAfter"), qsl("[look]"));
        }
        QVERIFY(session.shutdown(false));
    }

    // In the file the GUI's main command line reads, newest first behind the line being typed, and as there
    // without what was typed while the game hid it
    void test_submittedCommandsAreKeptInTheCommandHistory()
    {
        const QString name = qsl("Headless History");
        QVERIFY(writeProfile(name, QString()));
        const QString historyFile = MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("command_history_main"));
        QVERIFY(writeFile(historyFile, "\nnewest from the GUI\noldest from the GUI"));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        {
            THeadlessSession session;
            QSignalSpy output(&session, &THeadlessSession::outputLine);
            QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
            QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
            std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
            QVERIFY(session.submit(qsl("look")));
            QVERIFY(session.submit(qsl("oldest from the GUI")));
            QVERIFY(session.submit(QString()));
            game->write("\xff\xfb\x01Password:\r\n");
            QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Password:")), 10000);
            QVERIFY(session.submit(qsl("typed secret")));
            QByteArray received;
            QTRY_VERIFY_WITH_TIMEOUT((received += game->readAll()).contains("typed secret"), 10000);
            QVERIFY2(session.shutdown(true), qPrintable(session.errorString()));
        }
        QCOMPARE(readFile(historyFile), qsl("\noldest from the GUI\nlook\nnewest from the GUI"));
    }

    // For the app's input line to walk with its up arrow, as the GUI's command line walks the same history
    void test_theCommandHistoryIsReadBack()
    {
        const QString name = qsl("Headless History Read");
        QVERIFY(writeProfile(name, QString()));
        const QString historyFile = MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("command_history_main"));
        QVERIFY(writeFile(historyFile, "\nfrom the GUI"));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession unstarted;
        QVERIFY(unstarted.commandHistory().isEmpty());
        {
            THeadlessSession session;
            QSignalSpy output(&session, &THeadlessSession::outputLine);
            QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
            QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
            std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
            QCOMPARE(session.commandHistory(), QStringList{qsl("from the GUI")});
            QVERIFY(session.submit(qsl("a")));
            QVERIFY(session.submit(qsl("b")));
            QVERIFY(session.submit(qsl("a")));
            QCOMPARE(session.commandHistory(), (QStringList{qsl("a"), qsl("b"), qsl("from the GUI")}));

            game->write("\xff\xfb\x01Password:\r\n");
            QTRY_VERIFY_WITH_TIMEOUT(outputTexts(output).contains(qsl("Password:")), 10000);
            QVERIFY(session.submit(qsl("typed secret")));
            QByteArray received;
            QTRY_VERIFY_WITH_TIMEOUT((received += game->readAll()).contains("typed secret"), 10000);
            QCOMPARE(session.commandHistory(), (QStringList{qsl("a"), qsl("b"), qsl("from the GUI")}));
            QVERIFY2(session.shutdown(true), qPrintable(session.errorString()));
            QVERIFY(session.commandHistory().isEmpty());
        }

        THeadlessSession again;
        QCOMPARE(again.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(again.commandHistory(), (QStringList{qsl("a"), qsl("b"), qsl("from the GUI")}));
        QVERIFY(again.shutdown(false));
    }

    // TCommandLine::enterCommand() keeps the line as the aliases left it, and a paste is one entry that the
    // history file keeps a line at a time
    void test_theCommandHistoryKeepsTheLineAsTheAliasesLeftIt()
    {
        const QString name = qsl("Headless History Aliases");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession unstarted;
        QVERIFY(unstarted.commandHistory().isEmpty());
        QVERIFY(!unstarted.host());
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
            QVERIFY(session.commandHistory().isEmpty());
            QVERIFY(session.host());
            QVERIFY(session.runLua(qsl(R"lua(tempAlias("^go$", [[clearCmdLine()]]) tempAlias("^t ([a-z]+) (.*)$", [[printCmdLine("t " .. matches[2] .. " ")]]))lua")));
            QVERIFY(session.submit(qsl("look")));
            QVERIFY(session.submit(qsl("go")));
            QVERIFY(session.submit(qsl("t bob hi there")));
            QCOMPARE(session.commandHistory(), (QStringList{qsl("t bob "), qsl("look")}));
            QVERIFY(session.submit(qsl("x\ny")));
            QCOMPARE(session.commandHistory(), (QStringList{qsl("x\ny"), qsl("t bob "), qsl("look")}));
            QVERIFY2(session.shutdown(true), qPrintable(session.errorString()));
        }
        THeadlessSession again;
        QCOMPARE(again.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(again.commandHistory(), (QStringList{qsl("x"), qsl("y"), qsl("t bob "), qsl("look")}));
        QVERIFY(again.shutdown(false));
    }

    void test_aCommandHistoryThatCannotBeReadIsNotWrittenOver_data()
    {
        QTest::addColumn<bool>("directory");
        QTest::newRow("a write-only file") << false;
        // As one with the right to read any file cannot be kept from reading a file
        QTest::newRow("a directory") << true;
    }

    // Writing it would lose the history it holds, and the app is told. Write-only, as the save refuses a file it
    // cannot write
    void test_aCommandHistoryThatCannotBeReadIsNotWrittenOver()
    {
        QFETCH(bool, directory);
        const QString name = directory ? qsl("Headless History Directory") : qsl("Headless Unreadable History");
        QVERIFY(writeProfile(name, QString()));
        const QString historyFile = MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("command_history_main"));
        const auto restoreRights = [historyFile]() {
            QFile::setPermissions(historyFile, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        };
        const auto restore = qScopeGuard(restoreRights);
        if (directory) {
            QVERIFY(QDir().mkpath(historyFile));
        } else {
            QVERIFY(writeFile(historyFile, "\nfrom the GUI"));
            QVERIFY(QFile::setPermissions(historyFile, QFileDevice::WriteOwner));
            if (QFile probe(historyFile); probe.open(QIODevice::ReadOnly)) {
                QSKIP("Running with the right to read any file, so the history cannot be made unreadable");
            }
        }

        {
            THeadlessSession session;
            QSignalSpy errors(&session, &THeadlessSession::errorConsoleText);
            QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
            QCOMPARE(errors.size(), 1);
            QCOMPARE(errors.at(0).at(0).toString(), name);
            QVERIFY2(errors.at(0).at(1).toString().contains(qsl("command history")), qPrintable(errors.at(0).at(1).toString()));
            QVERIFY(session.commandHistory().isEmpty());
            QVERIFY(session.submit(qsl("look")));
            QCOMPARE(session.commandHistory(), QStringList{qsl("look")});
            QVERIFY2(session.shutdown(true), qPrintable(session.errorString()));
        }
        if (directory) {
            QVERIFY(QFileInfo(historyFile).isDir());
            QVERIFY(QDir(historyFile).removeRecursively());
            return;
        }
        restoreRights();
        QCOMPARE(readFile(historyFile), qsl("\nfrom the GUI"));
    }

    // The warning comes from inside start(), where an app that takes any error as fatal may delete the session
    void test_theSessionDeletedOnTheCommandHistoryWarning()
    {
        const QString name = qsl("Headless History Warning Delete");
        QVERIFY(writeProfile(name, QString()));
        const QString historyFile = MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("command_history_main"));
        QVERIFY(QDir().mkpath(historyFile));
        const auto removeDirectory = qScopeGuard([historyFile]() {
            QDir(historyFile).removeRecursively();
        });
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        const QPointer<THeadlessSession> session(pSession);
        QPointer<Host> profile;
        connect(pSession, &THeadlessSession::errorConsoleText, pSession, [pSession, &profile, name](const QString&, const QString& text) {
            if (text.contains(qsl("command history"))) {
                profile = HostManager::self()->getHost(name);
                delete pSession;
            }
        });
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::notOpened);
        QVERIFY(session.isNull());
        QVERIFY2(profile, "SETUP: the warning did not reach the handler while the profile was open");
        QTRY_VERIFY_WITH_TIMEOUT(profile.isNull(), 10000);
    }

    // What a profile sends on its way out, as from sysExitEvent, reaches the game before the connection goes
    void test_whatIsSentAsTheProfileClosesReachesTheGame()
    {
        const QString name = qsl("Headless Last Words");
        QVERIFY(writeProfile(name, qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() send("last words", false) end))lua")));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);
        THeadlessSession session;
        QSignalSpy connected(&session, &THeadlessSession::connected);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QTRY_COMPARE_WITH_TIMEOUT(connected.count(), 1, 10000);
        QVERIFY(session.shutdown(false));
        QByteArray received;
        QTRY_VERIFY2_WITH_TIMEOUT((received += game->readAll()).contains("last words"), received.constData(), 10000);
    }

    // Links, popups and inserted text, which the GUI shows as it does echo(), at the end, into a trigger's line
    // and where the cursor was moved
    void test_linksAndInsertedTextReachLocalText()
    {
        QVERIFY(writeProfile(qsl("Headless Local Links"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Local Links"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempTrigger("Linked", [[echoLink(" <link>", "", "hint") insertText(" <inserted>") insertLink(" <inserted link>", "", "hint")]]))lua")));
        QSignalSpy local(&session, &THeadlessSession::localText);
        QVERIFY(session.runLua(qsl(R"lua(echoLink("a link", "", "hint") echo("\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(cechoLink("<red>a coloured link", "", "hint") echo("\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(echoPopup("a popup", {""}, {"hint"}) echo("\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(moveCursorEnd() insertText("inserted\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(moveCursorEnd() insertLink("an inserted link", "", "hint") echo("\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("Linked\n"))lua")));
        QCOMPARE(outputTexts(local).join(QString()), qsl("a link\na coloured link\na popup\ninserted\nan inserted link\n <link> <inserted> <inserted link>"));
        QSignalSpy moved(&session, &THeadlessSession::localText);
        QVERIFY(session.runLua(qsl(R"lua(moveCursor(0, 1) insertText("<moved>") moveCursor(0, 2) insertLink("<moved link>", "", "hint"))lua")));
        QCOMPARE(outputTexts(moved).join(QString()), qsl("\n<moved>\n<moved link>"));
    }

    // An open piece ends with its line: the next game line, a deleted line's successor, and the trigger's own
    // line, where echo() and print() from a trigger both land
    void test_localTextEndsAPieceWithItsLine()
    {
        QVERIFY(writeProfile(qsl("Headless Local Lines"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Local Lines"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempTrigger("Alpha", [[echo(" <a>")]])
tempTrigger("Beta", [[echo(" <b>") deleteLine()]])
tempTrigger("Gamma", [[echo(" <c>")]]))lua")));
        QSignalSpy local(&session, &THeadlessSession::localText);
        QVERIFY(session.runLua(qsl(R"lua(echo("HP: "))lua")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("Welcome\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(echo("ok\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("Alpha\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("Beta\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("Gamma\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(echo("cleared") clearWindow() echo("end\n"))lua")));
        QCOMPARE(outputTexts(local).join(QString()), qsl("HP: \nok\n <a>\n <b>\n <c>\ncleared\nend\n"));
    }

    void test_errorConsoleTextIsSent()
    {
        const QString name = qsl("Headless Error Console");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy errors(&session, &THeadlessSession::errorConsoleText);
        QVERIFY(session.runLua(qsl(R"lua(registerAnonymousEventHandler("headlessBoom", function() error("headless boom") end) raiseEvent("headlessBoom"))lua")));
        const QString text = outputTexts(errors).join(QString());
        QVERIFY2(text.contains(qsl("headless boom")), qPrintable(text));
        QCOMPARE(errors.constFirst().at(0).toString(), name);
    }

    void test_aConnectionThatCannotBeMadeIsDisconnected()
    {
        const QString name = qsl("Headless Refused");
        QVERIFY(writeProfile(name, QString()));
        RefusedPort refused;
        QVERIFY(refused.reserve());
        const quint16 port = refused.port();
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(port)).first);

        THeadlessSession session;
        QSignalSpy connected(&session, &THeadlessSession::connected);
        QSignalSpy disconnected(&session, &THeadlessSession::disconnected);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_COMPARE_WITH_TIMEOUT(disconnected.count(), 1, 10000);
        QCOMPARE(disconnected.constFirst().at(0).toString(), name);
        QCOMPARE(connected.count(), 0);
        session.shutdown(false);
    }

    void test_aConnectionMadeIsConnected()
    {
        const QString name = qsl("Headless Connected");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy connected(&session, &THeadlessSession::connected);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_COMPARE_WITH_TIMEOUT(connected.count(), 1, 10000);
        QCOMPARE(connected.constFirst().at(0).toString(), name);
        session.shutdown(false);
    }

    // As a resize of the GUI's main window tells the game
    void test_theScreenSizeReachesTheGame()
    {
        const QString name = qsl("Headless Screen Size");
        QVERIFY(writeProfile(name, QString()));
        TelnetServerStub server;
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QVERIFY(!session.setScreenSize(60, 40));
        QCOMPARE(session.errorString(), qsl("No profile is open."));
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(registerAnonymousEventHandler("sysWindowResizeEvent", function(_, w, h, window)
  headlessResizes = (headlessResizes or "") .. w .. "x" .. h .. " " .. window .. ";"
end))lua")));
        // Before the game asks, so it is told when it does
        QVERIFY(session.setScreenSize(70, 30));
        QTRY_VERIFY_WITH_TIMEOUT(server.clientConnected(), 10000);
        server.sendRaw(QByteArray::fromHex("fffd1f"));
        QTRY_COMPARE_WITH_TIMEOUT(server.nawsUpdates(), (QVector<QSize>{QSize(70, 30)}), 10000);
        server.clearNawsUpdates();
        QVERIFY(session.runLua(qsl("headlessResizes = ''")));

        QVERIFY(session.setScreenSize(60, 40));
        QTRY_COMPARE_WITH_TIMEOUT(server.nawsUpdates(), (QVector<QSize>{QSize(60, 40)}), 10000);
        server.clearNawsUpdates();
        QVERIFY(session.runLua(qsl("headlessGrid = getColumnCount() .. 'x' .. getRowCount()")));
        QCOMPARE(luaGlobalString(session.host(), "headlessGrid"), qsl("60x40"));
        // As the GUI raises it for its main window, in pixels
        QVERIFY(session.runLua(qsl("local w, h = getMainWindowSize(); headlessResized = w .. 'x' .. h .. ' main;'")));
        QCOMPARE(luaGlobalString(session.host(), "headlessResizes"), luaGlobalString(session.host(), "headlessResized"));
        QVERIFY(session.setScreenSize(60, 40));
        QTest::qWait(1000);
        QVERIFY(server.nawsUpdates().isEmpty());
        QCOMPARE(luaGlobalString(session.host(), "headlessResizes"), luaGlobalString(session.host(), "headlessResized"));

        // As a frontend that reports its size on every layout pass would
        QVERIFY(session.setScreenSize(50, 40));
        for (int i = 0; i < 10; ++i) {
            QTest::qWait(100);
            QVERIFY(session.setScreenSize(50, 40));
        }
        QTest::qWait(100);
        QCOMPARE(server.nawsUpdates(), (QVector<QSize>{QSize(50, 40)}));
        server.clearNawsUpdates();

        // As the GUI's main console reports it
        QVERIFY(session.setScreenSize(20, 40));
        QTRY_COMPARE_WITH_TIMEOUT(server.nawsUpdates(), (QVector<QSize>{QSize(40, 40)}), 10000);
        server.clearNawsUpdates();
        QVERIFY(session.setScreenSize(300, 40));
        QTRY_COMPARE_WITH_TIMEOUT(server.nawsUpdates(), (QVector<QSize>{QSize(std::min(300, session.host()->mWrapAt), 40)}), 10000);
        server.clearNawsUpdates();
        // A byte of 255 goes out doubled, as IAC
        QVERIFY(session.setScreenSize(60, 255));
        QTRY_COMPARE_WITH_TIMEOUT(server.nawsUpdates(), (QVector<QSize>{QSize(60, 255)}), 10000);
        server.clearNawsUpdates();
        QVERIFY(session.setScreenSize(60, 65535));
        QTRY_COMPARE_WITH_TIMEOUT(server.nawsUpdates(), (QVector<QSize>{QSize(60, 65535)}), 10000);
        server.clearNawsUpdates();

        for (const auto& [columns, rows] : {std::pair(0, 40), std::pair(60, 0), std::pair(65536, 40), std::pair(60, 65536), std::pair(-1, -1)}) {
            QVERIFY(!session.setScreenSize(columns, rows));
            QVERIFY2(session.errorString().contains(qsl("from 1 to 65535")), qPrintable(session.errorString()));
        }
        QTest::qWait(500);
        QVERIFY(server.nawsUpdates().isEmpty());
        session.shutdown(false);
    }

    // As the GUI's main console takes a border from its text, and tells the game and scripts
    void test_aBorderTakesItsRoomFromTheGrid()
    {
        const QString name = qsl("Headless Border Columns");
        QVERIFY(writeProfile(name, QString()));
        TelnetServerStub server;
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QVERIFY(session.setScreenSize(100, 30));
        QTRY_VERIFY_WITH_TIMEOUT(server.clientConnected(), 10000);
        server.sendRaw(QByteArray::fromHex("fffd1f"));
        QTRY_COMPARE_WITH_TIMEOUT(server.nawsUpdates(), (QVector<QSize>{QSize(100, 30)}), 10000);
        server.clearNawsUpdates();
        QVERIFY(session.runLua(qsl(R"lua(cellWidth, cellHeight = calcFontSize()
headlessResizes = ""
registerAnonymousEventHandler("sysWindowResizeEvent", function(_, w, h, window)
  headlessResizes = headlessResizes .. w .. "x" .. h .. " " .. window .. ";"
end))lua")));
        const int cellWidth = luaGlobalString(session.host(), "cellWidth").toInt();
        const int cellHeight = luaGlobalString(session.host(), "cellHeight").toInt();
        QVERIFY(cellWidth > 0 && cellHeight > 0);
        const QString mainWindow = qsl("%1x%2").arg(100 * cellWidth).arg(30 * cellHeight);

        QVERIFY(session.runLua(qsl(R"lua(setBorderLeft(10 * cellWidth)
headlessGrid = getColumnCount() .. "x" .. getRowCount()
local w, h = getMainWindowSize()
headlessMainWindow = w .. "x" .. h)lua")));
        QCOMPARE(luaGlobalString(session.host(), "headlessGrid"), qsl("90x30"));
        QCOMPARE(luaGlobalString(session.host(), "headlessMainWindow"), mainWindow);
        QCOMPARE(luaGlobalString(session.host(), "headlessResizes"), qsl("%1x%2 main;").arg(90 * cellWidth).arg(30 * cellHeight));
        QTRY_COMPARE_WITH_TIMEOUT(server.nawsUpdates(), (QVector<QSize>{QSize(90, 30)}), 10000);
        server.clearNawsUpdates();

        // The area stays the frontend's, so a new size is less the border too
        QVERIFY(session.setScreenSize(120, 30));
        QVERIFY(session.runLua(qsl("headlessGrid = getColumnCount() .. 'x' .. getRowCount()")));
        QCOMPARE(luaGlobalString(session.host(), "headlessGrid"), qsl("110x30"));
        QTRY_COMPARE_WITH_TIMEOUT(server.nawsUpdates(), (QVector<QSize>{QSize(std::min(110, session.host()->mWrapAt), 30)}), 10000);
        server.clearNawsUpdates();
        QVERIFY(session.setScreenSize(100, 30));
        QTRY_COMPARE_WITH_TIMEOUT(server.nawsUpdates(), (QVector<QSize>{QSize(90, 30)}), 10000);
        server.clearNawsUpdates();

        // A handler that sets a border as it is told of one is told once more, and that ends it
        QVERIFY(session.runLua(qsl(R"lua(headlessResizes = ""
local handler
handler = registerAnonymousEventHandler("sysWindowResizeEvent", function()
  setBorderTop(2 * cellHeight)
end)
setBorderLeft(0)
killAnonymousEventHandler(handler)
headlessGrid = getColumnCount() .. "x" .. getRowCount()
local w, h = getMainWindowSize()
headlessMainWindow = w .. "x" .. h)lua")));
        QCOMPARE(luaGlobalString(session.host(), "headlessGrid"), qsl("100x28"));
        QCOMPARE(luaGlobalString(session.host(), "headlessMainWindow"), mainWindow);
        QCOMPARE(luaGlobalString(session.host(), "headlessResizes"), qsl("%1x%2 main;%1x%3 main;").arg(100 * cellWidth).arg(30 * cellHeight).arg(28 * cellHeight));
        QTRY_COMPARE_WITH_TIMEOUT(server.nawsUpdates(), (QVector<QSize>{QSize(100, 28)}), 10000);
        server.clearNawsUpdates();

        QVERIFY(session.runLua(qsl("setBorderTop(0) headlessGrid = getColumnCount() .. 'x' .. getRowCount()")));
        QCOMPARE(luaGlobalString(session.host(), "headlessGrid"), qsl("100x30"));
        QTRY_COMPARE_WITH_TIMEOUT(server.nawsUpdates(), (QVector<QSize>{QSize(100, 30)}), 10000);

        // A border handleWindowResizeEvent() sets is off what the resize raises, as the GUI takes them off as it raises
        QVERIFY(session.runLua(qsl(R"lua(headlessResizes = ""
headlessOwnHandler = handleWindowResizeEvent
function handleWindowResizeEvent()
  setBorderLeft(20 * cellWidth)
end)lua")));
        QVERIFY(session.setScreenSize(120, 30));
        QVERIFY(session.runLua(qsl("handleWindowResizeEvent = headlessOwnHandler headlessGrid = getColumnCount() .. 'x' .. getRowCount()")));
        QCOMPARE(luaGlobalString(session.host(), "headlessGrid"), qsl("100x30"));
        QCOMPARE(luaGlobalString(session.host(), "headlessResizes"), qsl("%1x%2 main;%1x%2 main;").arg(100 * cellWidth).arg(30 * cellHeight));

        // Borders wider than the area leave the GUI's floor of 40 columns, and no room for the text
        QVERIFY(session.runLua(qsl(R"lua(headlessResizes = ""
setBorderLeft(200 * cellWidth)
headlessGrid = getColumnCount() .. "x" .. getRowCount())lua")));
        QCOMPARE(luaGlobalString(session.host(), "headlessGrid"), qsl("40x30"));
        QCOMPARE(luaGlobalString(session.host(), "headlessResizes"), qsl("0x%1 main;").arg(30 * cellHeight));
        session.shutdown(false);
    }

    // The GUI loads a save before it makes the main console, which lays out the borders the save left; with no view
    // they are laid out once the load has settled the font, before any script runs
    void test_aSavedBorderIsLaidOutOnceTheSaveHasLoaded()
    {
        const QString name = qsl("Headless Saved Border");
        QVERIFY(writeProfile(name, qsl(R"lua(headlessGridAtStart = getColumnCount() .. "x" .. getRowCount())lua"), qsl("<borderLeftWidth>300</borderLeftWidth>")));
        QSize unlaid;
        {
            QVERIFY(!HostManager::self());
            auto pool = std::make_unique<HostManager>();
            QVERIFY(pool->addHost(name, QString(), QString(), QString()));
            Host* pHost = pool->getHost(name);
            QVERIFY(pHost);
            unlaid = QSize(pHost->mScreenWidth, pHost->mScreenHeight);
            pHost->loadProfileSave(QString());
            QCOMPARE(pHost->borders(), QMargins(300, 0, 0, 0));
            QCOMPARE(QSize(pHost->mScreenWidth, pHost->mScreenHeight), unlaid);
        }

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("cellWidth = calcFontSize()")));
        const int cellWidth = luaGlobalString(session.host(), "cellWidth").toInt();
        QVERIFY(cellWidth > 0);
        QCOMPARE(luaGlobalString(session.host(), "headlessGridAtStart"), qsl("%1x%2").arg(std::max(40, (unlaid.width() * cellWidth - 300) / cellWidth)).arg(unlaid.height()));
        QVERIFY(session.shutdown(false));
    }

    void test_aResizeHandlerThatClosesTheProfile_data()
    {
        QTest::addColumn<bool>("deleteSession");
        QTest::newRow("session kept") << false;
        QTest::newRow("session deleted") << true;
    }

    // A close from the resize handlers runs there and then; as a frontend's function bound into Lua would, they
    // can delete the session too
    void test_aResizeHandlerThatClosesTheProfile()
    {
        static THeadlessSession* spSession = nullptr;
        QFETCH(const bool, deleteSession);
        const QString name = deleteSession ? qsl("Headless Resize Closes Deleted") : qsl("Headless Resize Closes");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name, recordingScript(recordPath)));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        spSession = pSession;
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        lua_register(pSession->host()->getLuaInterpreter()->getLuaGlobalState(), "deleteSession", [](lua_State*) -> int {
            delete spSession;
            return 0;
        });
        QVERIFY(pSession->runLua(qsl(R"lua(function handleWindowResizeEvent() closeProfile()%1 end
registerAnonymousEventHandler("sysWindowResizeEvent", function()
  local file = io.open([[%2]], "ab")
  file:write("sysWindowResizeEvent\n")
  file:close()
end))lua")
                                         .arg(deleteSession ? qsl("; deleteSession()") : QString(), recordPath)));
        const QPointer<THeadlessSession> session(pSession);

        QVERIFY(pSession->setScreenSize(60, 40));
        QCOMPARE(session.isNull(), deleteSession);
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self() || !HostManager::self()->getHost(name), 10000);
        if (deleteSession) {
            // A pool deleted with a script running closes from a timer, and turns away a start() till then
            QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        }
    }

    // A frontend that opens the profile again asks about that run's close alone
    void test_aStartForgetsAFailedSave()
    {
        const QString name = qsl("Headless Save Failed");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function()
  local current = getMudletHomeDir() .. "/current"
  os.rename(current, current .. ".moved")
  local file = io.open(current, "wb")
  file:write("not a folder")
  file:close()
end))lua")));
        session.shutdown(true);
        QVERIFY(session.saveFailed());
        const QString current = qsl("%1/current").arg(MudletApp::getMudletPath(enums::profileHomePath, name));
        QVERIFY(QFile::remove(current));
        QVERIFY(QDir().rename(current + qsl(".moved"), current));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(!session.saveFailed());
        session.shutdown(true);
        QVERIFY(!session.saveFailed());
    }

    // Lines a trigger gags still go out as the game sent them, as --mirror copies them
    void test_fedLinesReachOutputLineInOrder()
    {
        QVERIFY(writeProfile(qsl("Headless Fed"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Fed"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(session.runLua(qsl(R"lua(tempTrigger("second", [[deleteLine()]]) feedTriggers("first fed\nsecond fed\nthird fed\n"))lua")));
        QCOMPARE(outputTexts(output), (QStringList{qsl("first fed"), qsl("second fed"), qsl("third fed")}));
    }

    void test_linesReachLineShownAsTheirTriggersLeftThem()
    {
        QVERIFY(writeProfile(qsl("Headless Shown"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown"), .connect = false}), THeadlessSession::StartResult::started);
        Host* pHost = session.host();
        QVERIFY(pHost);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^gag me$", [[deleteLine()]])
tempRegexTrigger("^long form$", [[selectString("long form", 1) replace("short")]])
tempRegexTrigger("^paint me$", [[selectString("paint", 1) fg("red") resetFormat()]])
tempRegexTrigger("^echo here$", [[echo(" and more\nsecond row")]])
tempRegexTrigger("^feeder$", [[feedTriggers("fed one\n") feedTriggers("fed two\n")]])
tempRegexTrigger("^fed one$", [[feedTriggers("fed deeper\n")]])
tempRegexTrigger("^gagged feeder$", [[deleteLine() feedTriggers("fed under a gag\n")]])
feedTriggers("plain\ngag me\nlong form\npaint me\necho here\nfeeder\ngagged feeder\nlast\n"))lua")));
        QCOMPARE(outputTexts(output),
                 (QStringList{qsl("plain"),
                              qsl("gag me"),
                              qsl("long form"),
                              qsl("paint me"),
                              qsl("echo here"),
                              qsl("feeder"),
                              qsl("fed one"),
                              qsl("fed deeper"),
                              qsl("fed two"),
                              qsl("gagged feeder"),
                              qsl("fed under a gag"),
                              qsl("last")}));
        QCOMPARE(outputTexts(shown),
                 (QStringList{qsl("plain"),
                              qsl("short"),
                              qsl("paint me"),
                              qsl("echo here and more\nsecond row"),
                              qsl("feeder"),
                              qsl("fed one"),
                              qsl("fed deeper"),
                              qsl("fed two"),
                              qsl("fed under a gag"),
                              qsl("last")}));
        QCOMPARE(shown.constFirst().at(0).toString(), qsl("Headless Shown"));
        QCOMPARE(shown.constFirst().at(2).toBool(), false);
        const auto runs = shown.at(2).at(3).value<QList<THeadlessSession::TextRun>>();
        QCOMPARE(runs.size(), 2);
        QCOMPARE(runs.at(0).start, 0);
        QCOMPARE(runs.at(0).length, 5);
        QCOMPARE(runs.at(0).foreground, QColor(255, 0, 0));
        QVERIFY(!runs.at(0).defaultForeground);
        QCOMPARE(runs.at(1).foreground, pHost->mFgColor);
        QCOMPARE(output.at(3).at(3).value<QList<THeadlessSession::TextRun>>().size(), 1);
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    void test_whatTriggersWriteElsewhereReachesLineShown()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Elsewhere"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Elsewhere"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^gag and echo$", [[deleteLine() echo(" echoed") echo(" twice")]])
tempRegexTrigger("^gag and cecho$", [[deleteLine() cecho("\n<red>OUCH")]])
tempRegexTrigger("^feed and echo$", [[feedTriggers("fed\n") echo("echoed after the feed")]])
tempRegexTrigger("^send and feed$", [[send("a command") feedTriggers("fed after the command\n")]])
tempRegexTrigger("^tag above$", [[local line = getLineNumber() moveCursor(0, line - 1) insertText("tag\n") wrapLine("main", line - 1)]])
feedTriggers("before\ngag and echo\nbefore cecho\ngag and cecho\nfeed and echo\nsend and feed\nunder tag\ntag above\nlast\n"))lua")));
        QCOMPARE(outputTexts(shown),
                 (QStringList{qsl("before"),
                              qsl(" echoed twice"),
                              qsl("before cecho"),
                              qsl("OUCH"),
                              qsl("feed and echo"),
                              qsl("fed"),
                              qsl("echoed after the feed"),
                              qsl("send and feed"),
                              qsl("a command"),
                              qsl("fed after the command"),
                              qsl("under tag"),
                              qsl("tag above"),
                              qsl("tag"),
                              qsl("last")}));
        const auto runs = shown.at(3).at(3).value<QList<THeadlessSession::TextRun>>();
        QCOMPARE(runs.size(), 1);
        QCOMPARE(runs.at(0).length, 4);
        QCOMPARE(runs.at(0).foreground, QColor(255, 0, 0));
        QVERIFY(session.host()->mainConsoleModel().mTriggerWritesForLineShown.isEmpty());
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    // A fed line that fills the line its feeder emptied is shown once, in its place
    void test_aLineFedIntoAnEmptiedLineIsShownOnce()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Emptied"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Emptied"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^empty me$", [[selectCurrentLine() replace("") feedTriggers("into the emptied line\n")]])
tempRegexTrigger("^$", [[feedTriggers("into the blank line\n")]])
feedTriggers("empty me\nbetween\n\nlast\n"))lua")));
        QCOMPARE(outputTexts(shown), (QStringList{qsl("into the emptied line"), qsl("between"), qsl("into the blank line"), qsl("last")}));
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    void test_aLineLongerThanTheWrapIsShownWhole()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Long"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Long"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        const QString feeder = qsl("feeder %1").arg(QStringList(40, qsl("word")).join(QChar::Space));
        const QString wrapper = qsl("wrapper %1").arg(QStringList(40, qsl("word")).join(QChar::Space));
        QVERIFY(feeder.size() > session.host()->mWrapAt);
        const QString inserted = qsl("%1 ").arg(QStringList(40, qsl("inserted")).join(QChar::Space));
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^feeder ", [[feedTriggers("gagged\n")]])
tempRegexTrigger("^gagged$", [[deleteLine()]])
tempRegexTrigger("^wrapper ", [[wrapLine("main", getLineNumber())]])
tempRegexTrigger("^lengthen the line above$", [[local line = getLineNumber() moveCursor(0, line - 1) insertText("%3") wrapLine("main", line - 1)]])
tempRegexTrigger("^split at a line feed$", [[echo(" and on\n<after the line feed>") wrapLine("main", getLineNumber())]])
feedTriggers("%1\n%2\nabove\nlengthen the line above\nsplit at a line feed\nlast\n"))lua")
                                       .arg(feeder, wrapper, inserted)));
        QCOMPARE(outputTexts(shown), (QStringList{feeder, wrapper, qsl("above"), qsl("lengthen the line above"), inserted, qsl("split at a line feed and on\n<after the line feed>"), qsl("last")}));
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    // localText leaves out what lineShown sends, so the two draw the console with nothing twice or missing,
    // a gagged line's echoes included
    void test_lineShownAndLocalTextRebuildTheConsole()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Rebuilt"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Rebuilt"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^Spam$", [[deleteLine()]])
tempRegexTrigger("^tagged$", [[echo(" <in a trigger>")]])
tempRegexTrigger("^sender$", [[send("from a trigger")]])
tempAlias("^look$", [[echo("the alias echoes\n")]]))lua")));
        const TBuffer& buffer = session.host()->mainConsoleModel().buffer;
        QVERIFY(buffer.lineBuffer.constLast().isEmpty());
        const int firstLine = static_cast<int>(buffer.lineBuffer.size()) - 1;
        RebuiltConsole console;
        rebuildConsole(session, console);
        QVERIFY(session.runLua(qsl(R"lua(echo("HP: "))lua")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("Spam\n"))lua")));
        QVERIFY(session.submit(qsl("look")));
        QVERIFY(session.runLua(qsl(R"lua(echo("open "))lua")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("tagged\nsender\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(echo("ok\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("Spam\n"))lua")));
        QVERIFY(session.submit(qsl("look")));
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY2(!console.localPieces.join(QString()).contains(qsl("trigger")), qPrintable(console.localPieces.join(QChar::Space)));
        QCOMPARE(console.localPieces.join(QString()).count(qsl("the alias echoes")), 2);
        QVERIFY(!console.ruleBroken);
        QCOMPARE(console.lines, consoleLines(buffer, firstLine));
        QVERIFY(session.shutdown(false));
    }

    // A Lua error the console echoes while a trigger runs is shown after the trigger's line, and leaves it whole
    void test_whatATriggerPrintsReachesLineShown()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Printed"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Printed"), .connect = false}), THeadlessSession::StartResult::started);
        session.host()->mEchoLuaErrors = true;
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^erring$", [[echo(" <before>") error("headless trigger boom")]]))lua")));
        const TBuffer& buffer = session.host()->mainConsoleModel().buffer;
        QVERIFY(buffer.lineBuffer.constLast().isEmpty());
        const int firstLine = static_cast<int>(buffer.lineBuffer.size()) - 1;
        RebuiltConsole console;
        rebuildConsole(session, console);
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("erring\nlast\n"))lua")));
        const QStringList texts = outputTexts(shown);
        QVERIFY2(texts.size() > 2, qPrintable(texts.join(QChar::LineFeed)));
        QCOMPARE(texts.constFirst(), qsl("erring <before>"));
        QVERIFY2(texts.mid(1, texts.size() - 2).join(QChar::LineFeed).contains(qsl("headless trigger boom")), qPrintable(texts.join(QChar::LineFeed)));
        QCOMPARE(texts.constLast(), qsl("last"));
        QVERIFY2(!console.localPieces.join(QString()).contains(qsl("headless trigger boom")), qPrintable(console.localPieces.join(QChar::Space)));
        QVERIFY(!console.ruleBroken);
        QCOMPARE(console.lines, consoleLines(buffer, firstLine));
        QVERIFY(session.shutdown(false));
    }

    // Printed as the GUI prints it, not written past the console's notice of what is printed
    void test_theNoticeOfLoggingResumedReachesLocalText()
    {
        const QString name = qsl("Headless Logging Notice");
        QVERIFY(writeProfile(name, QString()));
        QVERIFY(writeFile(MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("autolog")), QByteArray()));
        THeadlessSession session;
        QSignalSpy local(&session, &THeadlessSession::localText);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(session.host()->mainConsoleModel().mLogToLogFile, 10000);
        QVERIFY2(outputTexts(local).join(QString()).contains(qsl("System Message: Logging has started.")), qPrintable(outputTexts(local).join(QString())));
        QVERIFY(session.runLua(qsl(R"lua(startLogging(false))lua")));
        QVERIFY(session.shutdown(false));
    }

    // A fed line deleted in its own pass leaves the long line it was fed from to be wrapped there, as it always has
    // been, so the triggers after the feeder see only its first part; lineShown still sends it whole
    void test_aLineAFedLineLeftLastIsWrappedForItsLaterTriggers()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Wrapped Mid Pass"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Wrapped Mid Pass"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        const QString feeder = qsl("gagfeeder %1").arg(QStringList(40, qsl("word")).join(QChar::Space));
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^gagfeeder ", [[feedTriggers("gagme\n")]])
tempRegexTrigger("^gagme$", [[deleteLine()]])
tempRegexTrigger("^gagfeeder ", [[seenByALaterTrigger = getCurrentLine()]])
feedTriggers("%1\nlast\n"))lua")
                                       .arg(feeder)));
        QVERIFY(session.runLua(qsl(R"lua(assert(#seenByALaterTrigger > 0 and #seenByALaterTrigger < %1, #seenByALaterTrigger))lua").arg(feeder.size())));
        QVERIFY(session.runLua(qsl(R"lua(assert(("%1"):sub(1, #seenByALaterTrigger) == seenByALaterTrigger))lua").arg(feeder)));
        QCOMPARE(outputTexts(shown), (QStringList{feeder, qsl("last")}));
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    // The console still shows the rest of a line split in its pass once a trigger deletes its first part
    void test_whatADeleteLeavesOfASplitLineIsShown()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Split Then Gagged"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Split Then Gagged"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        const QString gagged = qsl("splitgag %1").arg(QStringList(40, qsl("word")).join(QChar::Space));
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^splitgag ", [[wrapLine("main", getLineNumber()) deleteLine()]])
feedTriggers("before\n%1\nlast\n"))lua")
                                       .arg(gagged)));
        const QStringList texts = outputTexts(shown);
        QCOMPARE(texts.size(), 3);
        QCOMPARE(texts.at(0), qsl("before"));
        QVERIFY2(texts.at(1).size() > 100 && gagged.endsWith(texts.at(1)) && !texts.at(1).startsWith(QChar::Space), qPrintable(texts.at(1)));
        QCOMPARE(texts.at(2), qsl("last"));
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    // Connected from a handler while a line's triggers run, lineShown starts with the next line the game sends, and
    // localText keeps what that pass writes
    void test_lineShownConnectedMidPassStartsWithTheNextLine()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Connected Mid Pass"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Connected Mid Pass"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy local(&session, &THeadlessSession::localText);
        QStringList shownTexts;
        bool connected = false;
        connect(&session, &THeadlessSession::outputLine, this, [&](const QString&, const QString& text) {
            if (text == qsl("inner") && !std::exchange(connected, true)) {
                connect(&session, &THeadlessSession::lineShown, this, [&shownTexts](const QString&, const QString& shownText) {
                    shownTexts << shownText;
                });
            }
        });
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^outer$", [[echo(" <early>") feedTriggers("inner\n")]])
feedTriggers("outer\nafter\n"))lua")));
        QVERIFY(connected);
        QCOMPARE(shownTexts, QStringList{qsl("after")});
        QCOMPARE(outputTexts(local).join(QString()).count(qsl("<early>")), 1);
        QVERIFY(session.shutdown(false));
    }

    // Lines trimmed off the top while a pass runs move the line's parts and what a trigger prints with them
    void test_aPassThatTrimsTheConsoleIsShownWhole()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Trimmed"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Trimmed"), .connect = false}), THeadlessSession::StartResult::started);
        session.host()->mEchoLuaErrors = true;
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        const QString trimmer = qsl("trimmer %1").arg(QStringList(40, qsl("word")).join(QChar::Space));
        QStringList fed;
        for (int i = 1; i <= 30; ++i) {
            fed << qsl("fed %1").arg(i);
        }
        QStringList printed;
        for (int i = 1; i <= 15; ++i) {
            printed << qsl("trimline%1").arg(i, 2, 10, QLatin1Char('0'));
        }
        QVERIFY(session.runLua(qsl(R"lua(setConsoleBufferSize("main", 100, 10)
local filler = {}
for i = 1, 120 do filler[#filler + 1] = "filler " .. i end
feedTriggers(table.concat(filler, "\n") .. "\n")
tempRegexTrigger("^trimmer ", [[wrapLine("main", getLineNumber()) feedTriggers("%1\n") error("%2")]]))lua")
                                       .arg(fed.join(qsl("\\n")), printed.join(qsl("\\n")))));
        const TBuffer& buffer = session.host()->mainConsoleModel().buffer;
        QVERIFY(buffer.lineBuffer.size() >= 90);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("%1\nlast\n"))lua").arg(trimmer)));
        // Some 50 lines written, so trimmed as they came
        QVERIFY(buffer.lineBuffer.size() <= 110);
        const QStringList texts = outputTexts(shown);
        QVERIFY2(texts.size() > 32, qPrintable(texts.join(QChar::LineFeed)));
        QCOMPARE(texts.constFirst(), trimmer);
        QCOMPARE(texts.mid(1, 30), fed);
        QCOMPARE(texts.constLast(), qsl("last"));
        const QString printedShown = texts.mid(31, texts.size() - 32).join(QChar::LineFeed);
        for (const QString& line : std::as_const(printed)) {
            QVERIFY2(printedShown.count(line) == 1, qPrintable(printedShown));
        }
        QVERIFY(outputTexts(triggerErrors).join(QString()).contains(qsl("trimline15")));
        QVERIFY(session.shutdown(false));
    }

    // What a trigger wrote away from its own line and then replaced is shown as the console keeps it, once
    void test_aWriteATriggerReplacesIsShownOnce()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Replaced"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Replaced"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^gag and rewrite$", [[deleteLine() echo(" old") moveCursorEnd() selectString("old", 1) replace("new")]])
tempRegexTrigger("^gag and widen$", [[deleteLine() echo(" abc") moveCursorEnd() selectString("b", 1) replace("BBB")]])
tempRegexTrigger("^gag and drop$", [[deleteLine() echo(" gone") moveCursorEnd() selectString(" gone", 1) replace("")]])
feedTriggers("before\ngag and rewrite\nmiddle\ngag and widen\nthen\ngag and drop\nlast\n"))lua")));
        QCOMPARE(outputTexts(shown), (QStringList{qsl("before"), qsl(" new"), qsl("middle"), qsl(" aBBBc"), qsl("then"), qsl("last")}));
        const TBuffer& buffer = session.host()->mainConsoleModel().buffer;
        QVERIFY(buffer.lineBuffer.contains(qsl("before new")));
        QVERIFY(buffer.lineBuffer.contains(qsl("middle aBBBc")));
        QVERIFY(buffer.lineBuffer.contains(qsl("then")));
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    // A sysBufferShrinkEvent handler run by the print of a trigger's Lua error echoes below the error, once
    void test_whatAShrinkHandlerEchoesUnderATriggersErrorIsShownOnce()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Shrink Echo"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Shrink Echo"), .connect = false}), THeadlessSession::StartResult::started);
        session.host()->mEchoLuaErrors = true;
        QStringList errorLines;
        for (int i = 1; i <= 40; ++i) {
            errorLines << qsl("shrinkerror%1").arg(i, 2, 10, QLatin1Char('0'));
        }
        QVERIFY(session.runLua(qsl(R"lua(setConsoleBufferSize("main", 100, 10)
local filler = {}
for i = 1, 120 do filler[#filler + 1] = "filler " .. i end
feedTriggers(table.concat(filler, "\n") .. "\n")
headlessShrinks = 0
registerAnonymousEventHandler("sysBufferShrinkEvent", function() headlessShrinks = headlessShrinks + 1 echo("shrunk " .. headlessShrinks .. "!") end)
tempRegexTrigger("^erring$", [[error("%1")]]))lua")
                                       .arg(errorLines.join(qsl("\\n")))));
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy local(&session, &THeadlessSession::localText);
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("erring\nlast\n"))lua")));
        const QStringList texts = outputTexts(shown);
        const QString shownText = texts.join(QChar::LineFeed);
        const QString localText = outputTexts(local).join(QString());
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "headlessShrinks");
        const int shrinks = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        QVERIFY(shrinks > 0);
        bool shownInThePass = false;
        for (int i = 1; i <= shrinks; ++i) {
            const QString echoed = qsl("shrunk %1!").arg(i);
            QVERIFY2(shownText.count(echoed) + localText.count(echoed) == 1, qPrintable(qsl("%1\n---\n%2").arg(shownText, localText)));
            shownInThePass = shownInThePass || shownText.contains(echoed);
        }
        QVERIFY2(shownInThePass, qPrintable(shownText));
        qsizetype firstError = -1;
        qsizetype firstEcho = -1;
        for (qsizetype i = 0; i < texts.size(); ++i) {
            if (firstError < 0 && texts.at(i).contains(qsl("shrinkerror"))) {
                firstError = i;
            }
            if (firstEcho < 0 && texts.at(i).contains(qsl("shrunk "))) {
                firstEcho = i;
            }
        }
        QVERIFY2(firstError >= 0 && firstEcho > firstError, qPrintable(shownText));
        QCOMPARE(texts.constLast(), qsl("last"));
        QVERIFY(session.shutdown(false));
    }

    // A colour a trigger puts on what it wrote itself is shown with that text
    void test_aColourATriggerPutsOnItsOwnWriteIsShown()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Recoloured"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Recoloured"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^gag and colour$", [[deleteLine() echo(" old") moveCursorEnd() selectString("old", 1) setFgColor(255, 0, 0) deselect() resetFormat()]])
feedTriggers("before\ngag and colour\nlast\n"))lua")));
        QCOMPARE(outputTexts(shown), (QStringList{qsl("before"), qsl(" old"), qsl("last")}));
        const auto runs = shown.at(1).at(3).value<QList<THeadlessSession::TextRun>>();
        QStringList described;
        bool red = false;
        for (const THeadlessSession::TextRun& run : runs) {
            described << qsl("%1+%2 %3").arg(QString::number(run.start), QString::number(run.length), run.foreground.name());
            red = red || (run.start == 1 && run.length == 3 && run.foreground == QColor(255, 0, 0));
        }
        QVERIFY2(red, qPrintable(described.join(qsl(", "))));
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    // What a trigger puts in beside what it wrote, or on a line above it, is shown where the lines have it
    void test_textATriggerPutsInBesideItsOwnWriteIsShownInPlace()
    {
        QVERIFY(writeProfile(qsl("Headless Shown In Place"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown In Place"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^gag and prefix$", [[deleteLine() echo(" (gagged)") moveCursorEnd() prefix("[X] ")]])
tempRegexTrigger("^stack inserts$", [[deleteLine() echo(" tail") moveCursorEnd() moveCursor(#getCurrentLine() - #" tail", getLineNumber()) insertText("A") insertText("B")]])
tempRegexTrigger("^mark both$", [[local y = getLineNumber()
moveCursor(3, y - 2) insertText("1")
moveCursor(3, y - 1) insertText("2")
moveCursor(4, y - 2) insertText("3")]])
feedTriggers("before\ngag and prefix\nthen\nstack inserts\none\ntwo\nmark both\nlast\n"))lua")));
        QCOMPARE(outputTexts(shown),
                 (QStringList{qsl("before"), qsl("[X] "), qsl(" (gagged)"), qsl("then"), qsl("BA tail"), qsl("one"), qsl("two"), qsl("mark both"), qsl("13"), qsl("2"), qsl("last")}));
        const TBuffer& buffer = session.host()->mainConsoleModel().buffer;
        QVERIFY(buffer.lineBuffer.contains(qsl("[X] before (gagged)")));
        QVERIFY(buffer.lineBuffer.contains(qsl("thenBA tail")));
        QVERIFY(buffer.lineBuffer.contains(qsl("one13")));
        QVERIFY(buffer.lineBuffer.contains(qsl("two2")));
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    // Held text keeps its place when a print in the same pass trims the top of the buffer
    void test_aWriteATriggerReplacesAfterATrimIsShownOnce()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Trimmed Replace"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Trimmed Replace"), .connect = false}), THeadlessSession::StartResult::started);
        session.host()->mEchoLuaErrors = true;
        QStringList errorLines;
        for (int i = 1; i <= 40; ++i) {
            errorLines << qsl("trimerror%1").arg(i, 2, 10, QLatin1Char('0'));
        }
        QVERIFY(session.runLua(qsl(R"lua(setConsoleBufferSize("main", 100, 10)
local filler = {}
for i = 1, 95 do filler[#filler + 1] = "filler " .. i end
feedTriggers(table.concat(filler, "\n") .. "\n")
headlessTrimmed = 0
registerAnonymousEventHandler("sysBufferShrinkEvent", function(_, _, trimmed) headlessTrimmed = headlessTrimmed + trimmed end)
tempRegexTrigger("^gag and rewrite$", [[deleteLine() echo(" old") moveCursorEnd() headlessOldAt = getLineNumber() headlessTrimmed = 0]])
tempRegexTrigger("^gag and rewrite$", [[error("%1")]])
tempRegexTrigger("^gag and rewrite$", [[moveCursor(0, headlessOldAt - headlessTrimmed) selectString("old", 1) replace("new")]]))lua")
                                       .arg(errorLines.join(qsl("\\n")))));
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("before\ngag and rewrite\nlast\n"))lua")));
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "headlessTrimmed");
        const int trimmed = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        QVERIFY(trimmed > 0);
        const QStringList texts = outputTexts(shown);
        const QString shownText = texts.join(QChar::LineFeed);
        QVERIFY2(texts.count(qsl(" new")) == 1 && !texts.contains(qsl(" old")) && !texts.contains(qsl("new")), qPrintable(shownText));
        QVERIFY(session.host()->mainConsoleModel().buffer.lineBuffer.contains(qsl("before new")));
        QCOMPARE(texts.constLast(), qsl("last"));
        QVERIFY(session.shutdown(false));
    }

    // Held text a script's wrapLine() splits is shown whole, as a wrapped line is, and an edit after the wrap finds it
    void test_aWriteATriggerWrapsThenEditsIsShownOnce()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Wrapped"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Wrapped"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        QVERIFY(session.runLua(
                qsl(R"lua(tempRegexTrigger("^gag and wrap$", [[deleteLine() echo(" a\nb") wrapLine("main", getLastLineNumber("main")) moveCursorEnd() selectString("b", 1) replace("B")]])
feedTriggers("before\ngag and wrap\nlast\n"))lua")));
        QCOMPARE(outputTexts(shown), (QStringList{qsl("before"), qsl(" a\nB"), qsl("last")}));
        const TBuffer& buffer = session.host()->mainConsoleModel().buffer;
        QVERIFY(buffer.lineBuffer.contains(qsl("before a")));
        QVERIFY(buffer.lineBuffer.contains(qsl("B")));
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    // What is left of held text a wrap split, once a script deletes one of its lines, is what is shown
    void test_aSplitWriteWithALineDeletedShowsWhatIsLeft()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Split Deleted"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Split Deleted"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        QVERIFY(session.runLua(qsl(R"lua(local split = [[deleteLine() echo(" a\nmid\nend") local y = getLastLineNumber("main") wrapLine("main", y) ]]
tempRegexTrigger("^split drop first$", split .. [[moveCursor(0, y) deleteLine()]])
tempRegexTrigger("^split drop middle$", split .. [[moveCursor(0, y + 1) deleteLine()]])
tempRegexTrigger("^split drop last$", split .. [[moveCursor(0, y + 2) deleteLine() moveCursor(3, y + 1) insertText("!")]])
feedTriggers("one\nsplit drop first\ntwo\nsplit drop middle\nthree\nsplit drop last\nlast\n"))lua")));
        QCOMPARE(outputTexts(shown), (QStringList{qsl("one"), qsl("mid\nend"), qsl("two"), qsl(" a\nend"), qsl("three"), qsl(" a\nmid!"), qsl("last")}));
        const TBuffer& buffer = session.host()->mainConsoleModel().buffer;
        QVERIFY(buffer.lineBuffer.contains(qsl("mid!")));
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    // What is left of held text a wrap split, once a trim takes its first lines, is what is shown
    void test_aSplitWriteATrimCutsShowsWhatIsLeft()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Split Trimmed"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Split Trimmed"), .connect = false}), THeadlessSession::StartResult::started);
        session.host()->mEchoLuaErrors = true;
        QStringList errorLines;
        for (int i = 1; i <= 95; ++i) {
            errorLines << qsl("spliterror%1").arg(i, 2, 10, QLatin1Char('0'));
        }
        // The split lands on lines 8 to 10, so the trim of the top 10 the error's print makes cuts it after "mid"
        QVERIFY(session.runLua(qsl(R"lua(setConsoleBufferSize("main", 100, 10)
clearWindow("main")
while getLastLineNumber("main") < 9 do feedTriggers("filler\n") end
headlessTrimmed = 0
registerAnonymousEventHandler("sysBufferShrinkEvent", function(_, _, trimmed) headlessTrimmed = headlessTrimmed + trimmed end)
tempRegexTrigger("^split trim$", [[deleteLine() headlessSplitAt = getLastLineNumber("main") echo(" a\nmid\nend") wrapLine("main", headlessSplitAt) error("%1")]]))lua")
                                       .arg(errorLines.join(qsl("\\n")))));
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("split trim\nlast\n"))lua")));
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "headlessTrimmed");
        const int trimmed = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        QCOMPARE(trimmed, 10);
        lua_getglobal(L, "headlessSplitAt");
        const int splitAt = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        QCOMPARE(splitAt, 8);
        const QStringList texts = outputTexts(shown);
        const QString shownText = texts.join(QChar::LineFeed);
        const qsizetype end = texts.indexOf(qsl("end"));
        qsizetype firstError = -1;
        for (qsizetype i = 0; i < texts.size() && firstError < 0; ++i) {
            if (texts.at(i).contains(qsl("spliterror"))) {
                firstError = i;
            }
        }
        QVERIFY2(end >= 0 && firstError > end, qPrintable(shownText));
        QVERIFY2(!shownText.contains(qsl("mid")) && !shownText.contains(qsl("filler")), qPrintable(shownText));
        QCOMPARE(texts.constLast(), qsl("last"));
        QVERIFY(session.shutdown(false));
    }

    // A sysBufferShrinkEvent handler that deletes held text before it echoes has its echo shown once
    void test_whatAShrinkHandlerEchoesAfterDeletingHeldTextIsShownOnce()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Shrink Delete"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Shrink Delete"), .connect = false}), THeadlessSession::StartResult::started);
        session.host()->mEchoLuaErrors = true;
        QStringList errorLines;
        for (int i = 1; i <= 40; ++i) {
            errorLines << qsl("droperror%1").arg(i, 2, 10, QLatin1Char('0'));
        }
        QVERIFY(session.runLua(qsl(R"lua(setConsoleBufferSize("main", 100, 10)
local filler = {}
for i = 1, 95 do filler[#filler + 1] = "filler " .. i end
feedTriggers(table.concat(filler, "\n") .. "\n")
registerAnonymousEventHandler("sysBufferShrinkEvent", function(_, _, trimmed)
  if headlessOldAt then
    moveCursor(0, headlessOldAt - trimmed)
    deleteLine()
    headlessOldAt = nil
    echo("dropped!")
  end
end)
tempRegexTrigger("^gag and erring$", [[deleteLine() echo(" old") moveCursorEnd() headlessOldAt = getLineNumber() error("%1")]]))lua")
                                       .arg(errorLines.join(qsl("\\n")))));
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy local(&session, &THeadlessSession::localText);
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("before\ngag and erring\nlast\n"))lua")));
        const QString shownText = outputTexts(shown).join(QChar::LineFeed);
        const QString localText = outputTexts(local).join(QString());
        QVERIFY2(shownText.count(qsl("dropped!")) + localText.count(qsl("dropped!")) == 1, qPrintable(qsl("%1\n---\n%2").arg(shownText, localText)));
        QVERIFY2(!shownText.contains(qsl(" old")), qPrintable(shownText));
        QCOMPARE(outputTexts(shown).constLast(), qsl("last"));
        QVERIFY(session.shutdown(false));
    }

    // Until an app connects lineShown, what triggers write is not kept for it
    void test_lineShownWithNoReceiverHoldsNothing()
    {
        QVERIFY(writeProfile(qsl("Headless Shown Unwatched"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Shown Unwatched"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        // Sent just after the echo is written, while the pass that would take what was held for it still runs
        qsizetype heldMidPass = 0;
        const auto heldWatch = connect(&session, &THeadlessSession::localText, this, [&session, &heldMidPass]() {
            heldMidPass = std::max(heldMidPass, session.host()->mainConsoleModel().mTriggerWritesForLineShown.size());
        });
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^gag and echo$", [[deleteLine() echo("unwatched echo")]])
tempRegexTrigger("^feeder$", [[feedTriggers("fed\n") echo("unwatched after the feed")]])
feedTriggers("gag and echo\nfeeder\n"))lua")));
        QCOMPARE(outputTexts(output), (QStringList{qsl("gag and echo"), qsl("feeder"), qsl("fed")}));
        QVERIFY(session.host()->mainConsoleModel().mTriggerWritesForLineShown.isEmpty());
        disconnect(heldWatch);
        QCOMPARE(heldMidPass, 0);
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("watched\n"))lua")));
        QCOMPARE(outputTexts(shown), (QStringList{qsl("watched")}));
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    void test_clearingTheMainConsoleReachesTheSession()
    {
        QVERIFY(writeProfile(qsl("Headless Cleared"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Cleared"), .connect = false}), THeadlessSession::StartResult::started);
        QStringList events;
        connect(&session, &THeadlessSession::lineShown, this, [&events](const QString&, const QString& text) {
            events << text;
        });
        connect(&session, &THeadlessSession::mainConsoleCleared, this, [&events](const QString& profile) {
            events << qsl("(cleared %1)").arg(profile);
        });
        QVERIFY(session.runLua(qsl(R"lua(createMiniConsole("headlessMini", 0, 0, 100, 100)
tempRegexTrigger("^wipe$", [[feedTriggers("fed then wiped\n") clearWindow()]])
tempRegexTrigger("^wipe and echo$", [[clearWindow() echo("echoed after the clear")]])
feedTriggers("before\n") clearWindow() feedTriggers("between\n") clearWindow("main") feedTriggers("wipe\n")
echo("headlessMini", "mini\n") clearWindow("headlessMini") feedTriggers("after\n") feedTriggers("wipe and echo\n"))lua")));
        QCOMPARE(events,
                 (QStringList{qsl("before"),
                              qsl("(cleared Headless Cleared)"),
                              qsl("between"),
                              qsl("(cleared Headless Cleared)"),
                              qsl("(cleared Headless Cleared)"),
                              qsl("after"),
                              qsl("(cleared Headless Cleared)"),
                              qsl("echoed after the clear")}));
        QVERIFY(session.shutdown(false));
    }

    void test_aClearEndsTheLineLocalTextLeftOpen()
    {
        QVERIFY(writeProfile(qsl("Headless Cleared Open"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Cleared Open"), .connect = false}), THeadlessSession::StartResult::started);
        QStringList events;
        connect(&session, &THeadlessSession::localText, this, [&events](const QString&, const QString& text) {
            events << text;
        });
        connect(&session, &THeadlessSession::mainConsoleCleared, this, [&events](const QString&) {
            events << qsl("(cleared)");
        });
        QVERIFY(session.runLua(qsl(R"lua(echo("abc") clearWindow() echo("def\n"))lua")));
        QCOMPARE(events, (QStringList{qsl("abc"), qsl("(cleared)"), qsl("def\n")}));
        QVERIFY(session.shutdown(false));
    }

    void test_loggingReachesTheSession()
    {
        const QString name = qsl("Headless Logging");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QSignalSpy logging(&session, &THeadlessSession::loggingChanged);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(startLogging(true))lua")));
        QTRY_COMPARE_WITH_TIMEOUT(logging.size(), 1, 10000);
        QCOMPARE(logging.at(0).at(0).toString(), name);
        QCOMPARE(logging.at(0).at(1).toBool(), true);
        const QString fileName = logging.at(0).at(2).toString();
        QCOMPARE(QFileInfo(fileName).absolutePath(), QDir(MudletApp::getMudletPath(enums::profileReplayAndLogFilesPath, name)).absolutePath());
        QVERIFY2(QFileInfo::exists(fileName), qPrintable(fileName));
        QVERIFY(session.runLua(qsl(R"lua(startLogging(false))lua")));
        QTRY_COMPARE_WITH_TIMEOUT(logging.size(), 2, 10000);
        QCOMPARE(logging.at(1).at(1).toBool(), false);
        QVERIFY(logging.at(1).at(2).toString().isEmpty());
        QVERIFY(session.shutdown(false));
        QCOMPARE(logging.size(), 2);

        // Left on, as the GUI's autolog resumes it on the next load
        QVERIFY(writeFile(MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("autolog")), QByteArray()));
        logging.clear();
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QTRY_COMPARE_WITH_TIMEOUT(logging.size(), 1, 10000);
        QCOMPARE(logging.at(0).at(1).toBool(), true);
        QVERIFY(QFileInfo::exists(logging.at(0).at(2).toString()));
        QTest::qWait(100);
        QCOMPARE(logging.size(), 1);
        // The profile stops logging only as it goes, after the session has let it go
        QVERIFY(session.shutdown(false));
        QCOMPARE(logging.size(), 2);
        QCOMPARE(logging.at(1).at(0).toString(), name);
        QCOMPARE(logging.at(1).at(1).toBool(), false);
        QVERIFY(logging.at(1).at(2).toString().isEmpty());
        QTest::qWait(100);
        QCOMPARE(logging.size(), 2);

        // The false still queued is dropped by the shutdown(), which sends its own
        logging.clear();
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QTRY_COMPARE_WITH_TIMEOUT(logging.size(), 1, 10000);
        QVERIFY(session.runLua(qsl(R"lua(startLogging(false))lua")));
        QVERIFY(session.shutdown(false));
        QTest::qWait(100);
        QCOMPARE(logging.size(), 2);
        QCOMPARE(logging.at(1).at(1).toBool(), false);

        QVERIFY(writeFile(MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("autolog")), QByteArray()));
        logging.clear();
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QTRY_COMPARE_WITH_TIMEOUT(logging.size(), 1, 10000);
        QVERIFY(session.runLua(qsl("closeProfile()")));
        QTRY_COMPARE_WITH_TIMEOUT(quit.size(), 1, 10000);
        QCOMPARE(logging.size(), 2);
        QCOMPARE(logging.at(1).at(1).toBool(), false);
        QVERIFY(session.shutdown(false));
        QTest::qWait(100);
        QCOMPARE(logging.size(), 2);
    }

    void test_aLogThatCannotStartIsReported()
    {
        const QString name = qsl("Headless Logging Refused");
        QVERIFY(writeProfile(name, QString()));
        // A directory under a file, which cannot be made even by root
        const QString blocker = mConfigDir.filePath(qsl("headless-log-blocker"));
        QVERIFY(writeFile(blocker, QByteArray("not a directory")));
        THeadlessSession session;
        QSignalSpy logging(&session, &THeadlessSession::loggingChanged);
        QSignalSpy errors(&session, &THeadlessSession::errorConsoleText);
        QSignalSpy local(&session, &THeadlessSession::localText);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        session.host()->mLogDir = qsl("%1/logs").arg(blocker);
        QVERIFY(session.runLua(qsl(R"lua(local ok, message = startLogging(true) failedToLog = message)lua")));
        QTRY_COMPARE_WITH_TIMEOUT(logging.size(), 1, 10000);
        QCOMPARE(logging.at(0).at(1).toBool(), false);
        QVERIFY(logging.at(0).at(2).toString().isEmpty());
        // Printed on the main console, as the GUI shows it, and not to the error console. The reason is the
        // OS's own wording, which differs between platforms, so only its presence is checked
        const QString logDirAndQuote = qsl("\"%1/logs/").arg(blocker);
        QVERIFY2(std::any_of(local.cbegin(),
                             local.cend(),
                             [&logDirAndQuote](const QList<QVariant>& args) {
                                 const QString text = args.at(1).toString();
                                 return text.startsWith(qsl("Could not start logging to")) && text.contains(logDirAndQuote) && !text.section(qsl("\": "), 1).trimmed().isEmpty();
                             }),
                 qPrintable(qsl("no report naming the log file and the reason in %1 localText message(s)").arg(local.size())));
        QCOMPARE(errors.size(), 0);
        QVERIFY(session.runLua(qsl(R"lua(assert(failedToLog:find("could not be logged to file", 1, true)))lua")));
        QTest::qWait(100);
        QCOMPARE(logging.size(), 1);
        QVERIFY(session.shutdown(false));
        QCOMPARE(logging.size(), 1);
    }

    // Two families this machine has installed that Mudlet does not bundle, fixed-pitch first, so that a build
    // with WITH_FONTS=NO changes the font too
    static QStringList installedFontFamilies()
    {
        QSet<QString> bundled{Host::scmDefaultFontFamily};
        QDirIterator fonts(qsl(":/fonts"), {qsl("*.ttf"), qsl("*.otf")}, QDir::Files, QDirIterator::Subdirectories);
        while (fonts.hasNext()) {
            QFile file(fonts.next());
            if (file.open(QIODevice::ReadOnly)) {
                bundled.insert(QRawFont(file.readAll(), 12).familyName());
            }
        }
        QStringList fixedPitch;
        QStringList others;
        for (const QString& family : QFontDatabase::families()) {
            if (fixedPitch.size() >= 2) {
                break;
            }
            if (bundled.contains(family) || family.contains(QLatin1Char('[')) || family.contains(qsl("Emoji"), Qt::CaseInsensitive) || QFontDatabase::isPrivateFamily(family)
                || QFontMetrics(QFont(family)).averageCharWidth() == 0) {
                continue;
            }
            (QFontDatabase::isFixedPitch(family) ? fixedPitch : others) << family;
        }
        return (fixedPitch + others).mid(0, 2);
    }

    void test_aScriptsFontChangeReachesTheSession()
    {
        const QString name = qsl("Headless Font");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QList<QFont> fonts;
        connect(&session, &THeadlessSession::consoleFontChanged, this, [&](const QString& profile, const QFont& font) {
            QCOMPARE(profile, name);
            fonts << font;
        });
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QTest::qWait(100);
        fonts.clear();
        QCOMPARE(TFontAttributes(session.consoleFont()), TFontAttributes(session.host()->displayFontWithNoView()));
        const QStringList families = installedFontFamilies();
        QCOMPARE(families.size(), 2);

        QVERIFY(session.runLua(qsl(R"lua(setFontSize("main", 20))lua")));
        QTRY_COMPARE_WITH_TIMEOUT(fonts.size(), 1, 10000);
        QTest::qWait(100);
        QCOMPARE(fonts.size(), 1);
        QCOMPARE(fonts.at(0).pointSize(), 20);
        QCOMPARE(TFontAttributes(fonts.at(0)), TFontAttributes(session.host()->displayFontWithNoView()));
        QCOMPARE(TFontAttributes(session.consoleFont()), TFontAttributes(fonts.at(0)));

        QVERIFY(session.runLua(qsl(R"lua(setFont("main", [==[%1]==]))lua").arg(families.at(0))));
        QTRY_COMPARE_WITH_TIMEOUT(fonts.size(), 2, 10000);
        QTest::qWait(100);
        QCOMPARE(fonts.size(), 2);
        QCOMPARE(fonts.at(1).family(), families.at(0));
        QCOMPARE(fonts.at(1).pointSize(), 20);
        QCOMPARE(TFontAttributes(fonts.at(1)), TFontAttributes(session.host()->displayFontWithNoView()));

        QVERIFY(session.runLua(qsl(R"lua(setFontSize("main", 20) setFont("main", [==[%1]==]))lua").arg(families.at(0))));
        QTest::qWait(100);
        QCOMPARE(fonts.size(), 2);

        // What a package's font coming or going, or a save installed as a package, changes it through
        QVERIFY(session.host()->setDisplayFont(QFont(families.at(1), 13)).first);
        QTRY_COMPARE_WITH_TIMEOUT(fonts.size(), 3, 10000);
        QCOMPARE(fonts.at(2).family(), families.at(1));
        QCOMPARE(fonts.at(2).pointSize(), 13);
        QCOMPARE(TFontAttributes(fonts.at(2)), TFontAttributes(session.host()->displayFontWithNoView()));
        session.host()->setDisplayFontFromString(QFont(families.at(0), 11).toString());
        QTRY_COMPARE_WITH_TIMEOUT(fonts.size(), 4, 10000);
        QCOMPARE(fonts.at(3).family(), families.at(0));
        QCOMPARE(fonts.at(3).pointSize(), 11);
        session.host()->setDisplayFontSize(11);
        QTest::qWait(100);
        QCOMPARE(fonts.size(), 4);
        QVERIFY(session.shutdown(false));
        QCOMPARE(session.consoleFont(), QFont());
    }

    // Read once the profile has loaded; what the load itself changed may follow, as consoleFontChanged
    void test_theFontAProfileLoadsWithIsKnownAtProfileLoaded()
    {
        const QString name = qsl("Headless Saved Font");
        const QStringList families = installedFontFamilies();
        QVERIFY(!families.isEmpty());
        const QString& family = families.constFirst();
        QVERIFY(writeProfile(name, QString(), qsl("<mDisplayFont>%1</mDisplayFont>").arg(QFont(family, 15).toString())));
        const QString missing = qsl("Headless Missing Font");
        QVERIFY(writeProfile(missing, QString(), qsl("<mDisplayFont>%1</mDisplayFont>").arg(QFont(qsl("No Such Headless Family"), 17).toString())));
        THeadlessSession session;
        QFont loadedWith;
        connect(&session, &THeadlessSession::profileLoaded, this, [&]() {
            loadedWith = session.consoleFont();
        });
        QList<QFont> fonts;
        connect(&session, &THeadlessSession::consoleFontChanged, this, [&](const QString&, const QFont& font) {
            fonts << font;
        });
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(loadedWith.family(), family);
        QCOMPARE(loadedWith.pointSize(), 15);
        QTest::qWait(100);
        QCOMPARE(fonts.size(), 0);
        QVERIFY(session.shutdown(false));

        // The bundled default stands in for a family that is not installed, and the missing family the load set on
        // the way there is never sent
        QCOMPARE(session.start({.profile = missing, .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(loadedWith.family(), Host::scmDefaultFontFamily);
        QCOMPARE(loadedWith.pointSize(), 17);
        QCOMPARE(TFontAttributes(session.host()->displayFontWithNoView()), TFontAttributes(loadedWith));
        QTest::qWait(100);
        QVERIFY2(fonts.isEmpty(), qPrintable(fonts.isEmpty() ? QString() : fonts.constFirst().family()));
        QVERIFY(session.runLua(qsl(R"lua(setFontSize("main", 18))lua")));
        QTRY_COMPARE_WITH_TIMEOUT(fonts.size(), 1, 10000);
        QCOMPARE(fonts.constFirst().family(), Host::scmDefaultFontFamily);
        QCOMPARE(fonts.constFirst().pointSize(), 18);
        QVERIFY(session.shutdown(false));
    }

    static QStringList debugTexts(const QSignalSpy& debug)
    {
        QStringList texts;
        for (const auto& arguments : debug) {
            texts << arguments.at(0).toString();
        }
        return texts;
    }

    static qsizetype indexContaining(const QStringList& texts, const QString& part)
    {
        for (qsizetype i = 0; i < texts.size(); ++i) {
            if (texts.at(i).contains(part)) {
                return i;
            }
        }
        return -1;
    }

    void test_theCentralDebugConsoleCanBeShown()
    {
        QVERIFY(writeProfile(qsl("Headless Debug"), QString()));
        auto session = std::make_unique<THeadlessSession>();
        QSignalSpy debug(session.get(), &THeadlessSession::debugText);
        QCOMPARE(session->start({.profile = qsl("Headless Debug"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session->runLua(qsl(R"lua(tempTrigger("headlessDebugged", [[]]))lua")));
        const auto matches = [&debug]() {
            return debugTexts(debug).filter(qsl("matched.")).size();
        };
        QVERIFY(session->runLua(qsl(R"lua(feedTriggers("headlessDebugged\n"))lua")));
        const qint64 shownAt = QDateTime::currentMSecsSinceEpoch();
        QTest::qWait(20);

        QVERIFY(session->setDebugConsoleShown(true));
        QVERIFY(session->debugConsoleShown());
        QVERIFY(TDebug::smDebugMode);
        // What the profile's start wrote is kept for the console, and comes before what the console says as it opens
        const qsizetype started = indexContaining(debugTexts(debug), qsl("Profile 'Headless Debug' started."));
        const qsizetype hidden = indexContaining(debugTexts(debug), qsl("kind(s) of message are hidden"));
        QVERIFY2(started >= 0, qPrintable(debugTexts(debug).join(QString())));
        QVERIFY(hidden > started);
        QVERIFY(debug.at(started).at(3).toLongLong() < shownAt);
        QVERIFY(debug.at(hidden).at(3).toLongLong() >= shownAt);
        QCOMPARE(indexContaining(debugTexts(debug), qsl("controls below")), -1);
        QVERIFY(session->runLua(qsl(R"lua(feedTriggers("headlessDebugged\n"))lua")));
        QCOMPARE(matches(), 1);
        QVERIFY(debug.constLast().at(1).value<QColor>().isValid());
        QVERIFY(debug.constLast().at(3).toLongLong() >= shownAt);
        // Shown already, so nothing is said again
        const qsizetype sizeBefore = debug.size();
        QVERIFY(session->setDebugConsoleShown(true));
        QCOMPARE(debug.size(), sizeBefore);

        QVERIFY(session->setDebugConsoleShown(false));
        QVERIFY(!session->debugConsoleShown());
        QVERIFY(!TDebug::smDebugMode);
        QVERIFY(session->runLua(qsl(R"lua(feedTriggers("headlessDebugged\n"))lua")));
        QCOMPARE(matches(), 1);

        // One for the process, owned by whichever showed it last
        QVERIFY(session->setDebugConsoleShown(true));
        THeadlessSession other;
        QVERIFY(other.setDebugConsoleShown(true));
        QVERIFY(!session->debugConsoleShown());
        QVERIFY(other.debugConsoleShown());
        QVERIFY(!session->setDebugConsoleShown(false));
        QVERIFY(session->errorString().contains(qsl("Another session")));
        QVERIFY(other.debugConsoleShown());
        QVERIFY(TDebug::smDebugMode);
        {
            THeadlessSession third;
            QVERIFY(third.setDebugConsoleShown(true));
            QVERIFY(other.setDebugConsoleShown(true));
        }
        QVERIFY(other.debugConsoleShown());
        QVERIFY(TDebug::smDebugMode);
        QVERIFY(session->setDebugConsoleShown(true));
        QVERIFY(!other.debugConsoleShown());

        // The Sink's destructor clears the pointer by itself, so debug mode is what shows the close
        session.reset();
        QVERIFY(!TDebug::sink());
        QVERIFY(!TDebug::smDebugMode);
    }

    // With no profile open: before a start() that fails, and with a session that never had one
    void test_theDebugConsoleClosesWithASessionThatHasNoProfile()
    {
        THeadlessSession session;
        QVERIFY(session.setDebugConsoleShown(true));
        QCOMPARE(session.start({.profile = qsl("Headless No Such Debug Profile"), .connect = false}), THeadlessSession::StartResult::noSuchProfile);
        QVERIFY(session.debugConsoleShown());
        QVERIFY(session.shutdown(false));
        QVERIFY(!session.debugConsoleShown());
        QVERIFY(!TDebug::sink());
        QVERIFY(!TDebug::smDebugMode);

        {
            THeadlessSession unstarted;
            QVERIFY(unstarted.setDebugConsoleShown(true));
            QVERIFY(TDebug::smDebugMode);
        }
        QVERIFY(!TDebug::sink());
        QVERIFY(!TDebug::smDebugMode);
    }

    void test_debugCategoriesPickWhatTheConsolePrints()
    {
        const TDebug::Categories before = TDebug::enabledCategories();
        const auto restore = qScopeGuard([before]() {
            TDebug::setEnabledCategories(before);
        });
        QVERIFY(writeProfile(qsl("Headless Debug Categories"), QString()));
        THeadlessSession session;
        QSignalSpy debug(&session, &THeadlessSession::debugText);
        QCOMPARE(session.start({.profile = qsl("Headless Debug Categories"), .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(session.debugCategories(), TDebug::csmAllCategories & ~TDebug::csmNoisyCategories);
        QVERIFY(session.setDebugConsoleShown(true));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("headlessUnseenLine\n"))lua")));
        QCOMPARE(indexContaining(debugTexts(debug), qsl("headlessUnseenLine")), -1);

        session.setDebugCategories(session.debugCategories() | TDebug::Category::GameLine);
        QVERIFY(session.debugCategories().testFlag(TDebug::Category::GameLine));
        debug.clear();
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("headlessSeenLine\n"))lua")));
        // A message in pieces, each to append to the last: the head ends no line
        const QStringList texts = debugTexts(debug);
        const qsizetype head = indexContaining(texts, qsl("new line arrived:"));
        QVERIFY2(head >= 0, qPrintable(texts.join(QString())));
        QVERIFY(!texts.at(head).endsWith(QLatin1Char('\n')));
        QVERIFY(head + 1 < texts.size());
        QVERIFY(texts.at(head + 1).endsWith(qsl("headlessSeenLine\n")));
        QVERIFY(debug.at(head).at(1).value<QColor>() != debug.at(head + 1).at(1).value<QColor>());

        session.setDebugCategories(session.debugCategories() & ~TDebug::Categories(TDebug::Category::GameLine));
        debug.clear();
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("headlessUnseenAgain\n"))lua")));
        QCOMPARE(indexContaining(debugTexts(debug), qsl("headlessUnseenAgain")), -1);

        // Closed by the shutdown(), once the profile has said it ended, and not left to the destructor
        QVERIFY(session.shutdown(false));
        QVERIFY(!session.debugConsoleShown());
        QVERIFY(!TDebug::sink());
        QVERIFY(!TDebug::smDebugMode);
        QVERIFY2(indexContaining(debugTexts(debug), qsl("Profile 'Headless Debug Categories' ended.")) >= 0, qPrintable(debugTexts(debug).join(QString())));
    }

    // Shown before start(), so the profile's load is seen as it runs
    void test_theDebugConsoleShowsAProfileLoading()
    {
        QVERIFY(writeProfile(qsl("Headless Debug Load"), qsl(R"lua(tempTrigger("headlessLoadMatch", [[]]) feedTriggers("headlessLoadMatch\n"))lua")));
        THeadlessSession session;
        QSignalSpy debug(&session, &THeadlessSession::debugText);
        QVERIFY(session.setDebugConsoleShown(true));
        const qint64 shownAt = QDateTime::currentMSecsSinceEpoch();
        QCOMPARE(session.start({.profile = qsl("Headless Debug Load"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.debugConsoleShown());
        QTRY_VERIFY_WITH_TIMEOUT(indexContaining(debugTexts(debug), qsl("Profile 'Headless Debug Load' started.")) >= 0, 10000);
        const qsizetype started = indexContaining(debugTexts(debug), qsl("Profile 'Headless Debug Load' started."));
        QVERIFY(debug.at(started).at(3).toLongLong() >= shownAt);
        QVERIFY2(!debugTexts(debug).filter(qsl("matched.")).isEmpty(), qPrintable(debugTexts(debug).join(QString())));
        QVERIFY(session.shutdown(false));
        QVERIFY(!TDebug::smDebugMode);
    }

    // A regex that does not compile: why, then the pattern, in two pieces of different colours
    void test_aTriggerErrorReachesTheDebugConsoleInPieces()
    {
        QVERIFY(writeProfile(qsl("Headless Debug Error"), QString()));
        THeadlessSession session;
        QSignalSpy debug(&session, &THeadlessSession::debugText);
        QCOMPARE(session.start({.profile = qsl("Headless Debug Error"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(headlessBrokenTrigger = tempRegexTrigger("^headlessBroken$", [[]]))lua")));
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "headlessBrokenTrigger");
        const int triggerId = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        TTrigger* trigger = session.host()->getTriggerUnit()->getTrigger(triggerId);
        QVERIFY(trigger);
        QVERIFY(session.setDebugConsoleShown(true));
        debug.clear();
        QVERIFY(!trigger->setRegexCodeList({qsl("^headlessBroken[$")}, {REGEX_PERL}));
        const QStringList texts = debugTexts(debug);
        const qsizetype head = indexContaining(texts, qsl("REGEX ERROR: failed to compile, reason:\n"));
        QVERIFY2(head >= 0, qPrintable(texts.join(QString())));
        QVERIFY(head + 1 < texts.size());
        QVERIFY2(texts.at(head + 1).startsWith(qsl("in: \"^headlessBroken[$\"")), qPrintable(texts.at(head + 1)));
        QVERIFY(debug.at(head).at(1).value<QColor>() != debug.at(head + 1).at(1).value<QColor>());
        QVERIFY(session.setDebugConsoleShown(false));
        QVERIFY(session.shutdown(false));
    }

    // The console is shown before start(), so the profile's own construction sends debugText, before the load has begun
    void test_aSessionDeletedByDebugTextAsItsProfileStartsLetsItGo()
    {
        const QString name = qsl("Headless Debug Deleted");
        QVERIFY(writeProfile(name, QString()));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        const QPointer<THeadlessSession> pAlive(pSession);
        QVERIFY(pSession->setDebugConsoleShown(true));
        connect(pSession, &THeadlessSession::debugText, this, [pAlive, name](const QString& text) {
            if (pAlive && text.contains(qsl("Profile '%1' started.").arg(name))) {
                delete pAlive.data();
            }
        });
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::notOpened);
        QVERIFY(!pAlive);
        QVERIFY(!TDebug::sink());
        QVERIFY(!TDebug::smDebugMode);
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
    }

    // Sent mid-write, as outputLine is
    void test_aDebugTextHandlerIsRefusedACallBack()
    {
        QVERIFY(writeProfile(qsl("Headless Debug Refused"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Debug Refused"), .connect = false}), THeadlessSession::StartResult::started);
        std::optional<bool> ran;
        QString error;
        connect(&session, &THeadlessSession::debugText, this, [&]() {
            if (!ran) {
                ran = session.runLua(qsl("headlessFromDebugText = true"));
                error = session.errorString();
            }
        });
        QVERIFY(session.setDebugConsoleShown(true));
        QVERIFY(ran.has_value());
        QVERIFY(!*ran);
        QVERIFY2(error.contains(qsl("debugText")), qPrintable(error));
        QVERIFY(session.setDebugConsoleShown(false));
        QVERIFY(session.shutdown(false));
    }

    void test_aPromptATriggerRewritesIsShownAsAPrompt()
    {
        const QString name = qsl("Headless Shown Prompt");
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);

        THeadlessSession session;
        QSignalSpy shown(&session, &THeadlessSession::lineShown);
        QSignalSpy triggerErrors(&session, &THeadlessSession::errorConsoleText);
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempRegexTrigger("^hp ", [[selectString("hp", 1) replace("HP")]])
tempRegexTrigger("^Gagged$", [[deleteLine()]]))lua")));
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        game->write("Welcome\r\nGagged\r\nhp 10> \xff\xf9");
        QTRY_VERIFY_WITH_TIMEOUT(outputTexts(shown).contains(qsl("HP 10> ")), 10000);
        QCOMPARE(outputTexts(shown), (QStringList{qsl("Welcome"), qsl("HP 10> ")}));
        QCOMPARE(shown.at(0).at(2).toBool(), false);
        QCOMPARE(shown.at(1).at(2).toBool(), true);
        QVERIFY2(triggerErrors.isEmpty(), qPrintable(outputTexts(triggerErrors).join(QString())));
        QVERIFY(session.shutdown(false));
    }

    void test_linesFromTheLoadReachOutputLine()
    {
        QVERIFY(writeProfile(qsl("Headless Load Lines"), qsl(R"lua(feedTriggers("fed by the script\n")
registerAnonymousEventHandler("sysLoadEvent", function() feedTriggers("fed by sysLoadEvent\n") end))lua")));
        THeadlessSession session;
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = qsl("Headless Load Lines"), .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(outputTexts(output), (QStringList{qsl("fed by the script"), qsl("fed by sysLoadEvent")}));
    }

    // Another profile a script opens, and what echo() adds, are not the game's lines of the session's profile
    void test_onlyTheProfilesGameLinesReachOutputLine()
    {
        QVERIFY(writeProfile(qsl("Headless Own"), QString()));
        QVERIFY(writeProfile(qsl("Headless Other"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Own"), .connect = false}), THeadlessSession::StartResult::started);
        Host* pOther = HostManager::self()->openProfile(qsl("Headless Other"), false);
        QVERIFY(pOther);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(feedTriggers("fed to the other\n"))lua")));
        QVERIFY(session.runLua(qsl(R"lua(echo("echoed\n") feedTriggers("fed to its own\n"))lua")));
        QCOMPARE(outputTexts(output), (QStringList{qsl("fed to its own")}));
    }

    void test_submitRunsAliases()
    {
        QVERIFY(writeProfile(qsl("Headless Submit"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Submit"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempAlias("^headless (\\w+)$", [[headlessAliasHit = matches[2]; headlessAliasCount = (headlessAliasCount or 0) + 1]]))lua")));
        QVERIFY(session.submit(qsl("headless ping")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasHit"), qsl("ping"));
        // As a paste into the GUI's command line, each line its own command
        QVERIFY(session.submit(qsl("headless one\nheadless two")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasHit"), qsl("two"));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasCount"), qsl("3"));
        // As a Windows paste, which the GUI's command line never hands over
        QVERIFY(session.submit(qsl("headless three\r\nheadless four")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasCount"), qsl("5"));
    }

    void test_runLuaReportsErrors()
    {
        QVERIFY(writeProfile(qsl("Headless Lua"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Lua"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("headlessLua = 'ran'")));
        QCOMPARE(luaGlobalString(session.host(), "headlessLua"), qsl("ran"));
        QVERIFY(!session.runLua(qsl("this is not lua")));
        QVERIFY2(session.errorString().contains(qsl("near")), qPrintable(session.errorString()));
        QVERIFY(!session.runLua(qsl("error('headless failure')")));
        QCOMPARE(session.errorString(), qsl(R"lua([string "error('headless failure')"]:1: headless failure)lua"));
        QVERIFY(!session.runLua(qsl("error({})")));
        QCOMPARE(session.errorString(), qsl("no error message available from Lua"));
        QVERIFY(!session.runLua(QString()));
        QVERIFY(!session.errorString().contains(qsl("headless failure")));
    }

    void test_closeProfileRequestsQuit()
    {
        QVERIFY(writeProfile(qsl("Headless Close"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Close"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        QVERIFY(session.runLua(qsl("closeProfile()")));
        QTRY_COMPARE_WITH_TIMEOUT(quit.count(), 1, 10000);
        QVERIFY(!session.host());
        QVERIFY(!session.submit(qsl("look")));
    }

    // The app shut the session down before the close's own end came out of the event loop
    void test_aShutdownAfterAScriptClosedTheProfileSendsNoQuit()
    {
        QVERIFY(writeProfile(qsl("Headless Closed Then Shut Down"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Closed Then Shut Down"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        std::optional<bool> hostWhenShutDown;
        QVERIFY(session.runLua(qsl("closeProfile()")));
        QMetaObject::invokeMethod(
                &session,
                [&]() {
                    hostWhenShutDown = static_cast<bool>(session.host());
                    QVERIFY(session.shutdown(false));
                },
                Qt::QueuedConnection);
        QTRY_VERIFY_WITH_TIMEOUT(hostWhenShutDown.has_value(), 10000);
        QVERIFY(!*hostWhenShutDown);
        QTest::qWait(200);
        QCOMPARE(quit.count(), 0);
    }

    // The shutdown's wait for another profile's reset runs the event loop, which delivers the close's end
    void test_aShutdownWaitingOnAResetSendsNoQuitForAProfileAScriptClosed()
    {
        const QString name = qsl("Headless Closed While Another Resets");
        const QString other = qsl("Headless Resetting Elsewhere");
        QVERIFY(writeProfile(name, QString()));
        QVERIFY(writeProfile(other, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("loadProfile('%1', true)").arg(other)));
        Host* pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        QVERIFY(session.runLua(qsl("closeProfile()")));
        std::optional<bool> shutDown;
        // Behind the close, ahead of its end
        QMetaObject::invokeMethod(
                &session,
                [&]() {
                    QVERIFY(!session.host());
                    QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl("resetProfile()")));
                    QVERIFY(pOther->profileResetInProgress());
                    shutDown = session.shutdown(false);
                    QCOMPARE(quit.count(), 0);
                },
                Qt::QueuedConnection);
        QTRY_VERIFY_WITH_TIMEOUT(shutDown.has_value(), 10000);
        QVERIFY(*shutDown);
        QTest::qWait(200);
        QCOMPARE(quit.count(), 0);
    }

    void test_shutdownSavesOnlyWhenAsked_data()
    {
        QTest::addColumn<bool>("save");
        QTest::newRow("save") << true;
        QTest::newRow("no save") << false;
    }

    void test_shutdownSavesOnlyWhenAsked()
    {
        QFETCH(bool, save);
        const QString name = save ? qsl("Headless Shutdown Save") : qsl("Headless Shutdown No Save");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name, recordingScript(recordPath)));

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        session.shutdown(save);
        QVERIFY(!HostManager::self());
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), save ? 2 : 1);
    }

    // As a phone app sent to the background, where the system may stop it without warning
    void test_saveLeavesTheProfileRunning()
    {
        const QString name = qsl("Headless Save");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name, recordingScript(recordPath)));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempAlias("^headless$", [[headlessAliasRan = "yes"]]))lua")));
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        const QString mapsFolder = MudletApp::getMudletPath(enums::profileMapsPath, name);
        const qsizetype mapsBefore = QDir(mapsFolder).entryList(QDir::Files).size();

        QVERIFY2(session.save(), qPrintable(session.errorString()));
        QCOMPARE(saveCount(name), 2);
        QCOMPARE(QDir(mapsFolder).entryList(QDir::Files).size(), mapsBefore + 1);
        QVERIFY(readFile(recordPath).isEmpty());
        QVERIFY(session.host());
        QVERIFY(session.submit(qsl("headless")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasRan"), qsl("yes"));
        QVERIFY(session.runLua(qsl("headlessAfterSave = 'yes'")));
        QTest::qWait(100);
        QCOMPARE(quit.count(), 0);
        QVERIFY(session.shutdown(false));
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
    }

    // A frontend may be stopped as soon as save() returns
    void test_theSaveIsOnDiskWhenSaveReturns()
    {
        const QString name = qsl("Headless Save On Disk");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(permAlias("headlessSavedAlias", "", "^xyzzy$", ""))lua")));
        QVERIFY2(session.save(), qPrintable(session.errorString()));
        // Before anything runs the event loop, which would let a write still running finish
        QVERIFY(!session.host()->currentlySavingProfile());
        const QDir saveFolder(MudletApp::getMudletPath(enums::profileXmlFilesPath, name));
        const QStringList saves = saveFolder.entryList(QStringList{qsl("*.xml")}, QDir::Files, QDir::Name);
        QCOMPARE(saves.size(), 2);
        const QString newest = readFile(saveFolder.filePath(saves.constLast()));
        QVERIFY2(newest.contains(qsl("headlessSavedAlias")) && newest.trimmed().endsWith(qsl("</MudletPackage>")), qPrintable(newest.right(200)));
        // The command lines' history settings the save wrote
        const QString profileIni = readFile(MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("profile.ini")));
        QVERIFY2(profileIni.contains(qsl("SaveHistory")), qPrintable(profileIni));
        QVERIFY(session.shutdown(false));
    }

    void test_saveWritesTheWindowLayout()
    {
        const QString name = qsl("Headless Save Layout");
        QVERIFY(writeProfile(name, QString()));
        const QString layoutPath = MudletApp::getMudletPath(enums::mainDataItemPath, qsl("windowLayoutHeadless.dat"));
        QFile::remove(layoutPath);
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("openUserWindow('headlessSaveWindow', false, false, 'f'); resizeWindow('headlessSaveWindow', 250, 160)")));

        QVERIFY2(session.save(), qPrintable(session.errorString()));
        QFile layoutFile(layoutPath);
        QVERIFY2(layoutFile.open(QIODevice::ReadOnly), "save() wrote no window layout.");
        // As QDataStream writes a QString: UTF-16, big-endian
        QByteArray windowName;
        for (const QChar c : qsl("headlessSaveWindow")) {
            windowName.append('\0').append(c.toLatin1());
        }
        QVERIFY(layoutFile.readAll().contains(windowName));
        layoutFile.close();
        QVERIFY(session.shutdown(false));
        QFile::remove(layoutPath);
    }

    // As a script that ran while another profile's save waited for its writes could
    void test_aProfileThatCloseFailedToSaveDuringASaveIsNamed()
    {
        const QString name = qsl("Headless Save Opener");
        const QString other = qsl("Headless Save Quit Meanwhile");
        QVERIFY(writeProfile(other, QString()));
        QVERIFY(writeProfile(name, qsl("loadProfile(\"%1\", true)").arg(other)));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(HostManager::self()->getHost(other));
        QCOMPARE(HostManager::self()->hostList().constFirst()->getName(), name);
        const QString current = qsl("%1/current").arg(MudletApp::getMudletPath(enums::profileHomePath, other));
        QVERIFY(QDir().rename(current, current + qsl(".moved")));
        QVERIFY(writeFile(current, "not a folder"));
        bool closeAsked = false;
        connect(session.host(), &Host::profileSaveFinished, this, [&closeAsked, other]() {
            if (!std::exchange(closeAsked, true)) {
                HostManager::self()->requestProfileClose(other);
            }
        });

        QVERIFY(!session.save());
        QVERIFY(closeAsked);
        QCOMPARE(session.errorString(), qsl("These profiles could not be saved: Headless Save Quit Meanwhile"));
        QCOMPARE(saveCount(name), 2);
        QVERIFY(QFile::remove(current));
        QVERIFY(QDir().rename(current + qsl(".moved"), current));
        QVERIFY(session.shutdown(false));
    }

    void test_saveLeavesAFailedLoadAsItWas()
    {
        const QString name = qsl("Headless Save Corrupt Opener");
        const QString other = qsl("Headless Saved By The Corrupt");
        QVERIFY(writeProfile(other, QString()));
        QVERIFY(writeOpenerThatFailsToLoad(name, other));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::loadFailed);
        const QString mapsFolder = MudletApp::getMudletPath(enums::profileMapsPath, name);
        const QStringList mapsBefore = QDir(mapsFolder).entryList(QDir::Files);

        QVERIFY2(session.save(), qPrintable(session.errorString()));
        QCOMPARE(saveCount(name), 1);
        QCOMPARE(QDir(mapsFolder).entryList(QDir::Files), mapsBefore);
        QCOMPARE(saveCount(other), 2);
        QVERIFY(HostManager::self()->getHost(other));
        QVERIFY(session.shutdown(false));
    }

    void test_aSaveThatFailsNamesTheProfile()
    {
        const QString name = qsl("Headless Save Fails");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QString current = qsl("%1/current").arg(MudletApp::getMudletPath(enums::profileHomePath, name));
        QVERIFY(QDir().rename(current, current + qsl(".moved")));
        QVERIFY(writeFile(current, "not a folder"));

        QVERIFY(!session.save());
        QCOMPARE(session.errorString(), qsl("These profiles could not be saved: Headless Save Fails"));
        QVERIFY(session.host());
        QVERIFY(QFile::remove(current));
        QVERIFY(QDir().rename(current + qsl(".moved"), current));
        QVERIFY2(session.save(), qPrintable(session.errorString()));
        QCOMPARE(saveCount(name), 2);
        QVERIFY(session.shutdown(false));
    }

    // As a full disk that is then cleared would: a QSettings keeps the first error it meets
    void test_aSaveOnceProfileIniCanBeWrittenAgainSucceeds()
    {
        const QString name = qsl("Headless Save Profile Ini");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QString profileIni = MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("profile.ini"));
        QFile::remove(profileIni);
        QVERIFY(QDir().mkpath(profileIni));
        session.host()->writeProfileIniData(qsl("Headless/first"), qsl("1"));

        QVERIFY(!session.save());
        QCOMPARE(session.errorString(), qsl("These profiles could not be saved: Headless Save Profile Ini"));
        QVERIFY(QDir(profileIni).removeRecursively());
        session.host()->writeProfileIniData(qsl("Headless/second"), qsl("2"));
        QVERIFY2(session.save(), qPrintable(session.errorString()));
        const QString written = readFile(profileIni);
        QVERIFY2(written.contains(qsl("first=1")) && written.contains(qsl("second=2")), qPrintable(written));
        QVERIFY(session.shutdown(false));
    }

    // A QSettings that found its file damaged keeps that parse error, which would hide every write that fails after it
    void test_aProfileIniThatCannotBeWrittenAfterItWasDamagedIsAFailedSave()
    {
        const QString name = qsl("Headless Save Damaged Profile Ini");
        QVERIFY(writeProfile(name, QString()));
        const QString profileIni = MudletApp::getMudletPath(enums::profileDataItemPath, name, qsl("profile.ini"));
        QVERIFY(writeFile(profileIni, "[CommandLines]\nUsedIndexes=1\nthis line was truncated mid-write\n"));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        session.host()->writeProfileIniData(qsl("Headless/first"), qsl("1"));
        QVERIFY2(session.save(), qPrintable(session.errorString()));
        QVERIFY(QFile::remove(profileIni));
        QVERIFY(QDir().mkpath(profileIni));
        session.host()->writeProfileIniData(qsl("Headless/second"), qsl("2"));

        QVERIFY(!session.save());
        QCOMPARE(session.errorString(), qsl("These profiles could not be saved: Headless Save Damaged Profile Ini"));
        QVERIFY(QDir(profileIni).removeRecursively());
        QVERIFY2(session.save(), qPrintable(session.errorString()));
        const QString written = readFile(profileIni);
        QVERIFY2(written.contains(qsl("first=1")) && written.contains(qsl("second=2")), qPrintable(written));
        QVERIFY(session.shutdown(false));
    }

    // The close's save writes the profile and map; profile.ini would otherwise only be written as the Host is deleted
    void test_aProfileWhoseCloseCouldNotWriteProfileIniDuringASaveIsNamed()
    {
        const QString name = qsl("Headless Save Ini Opener");
        const QString other = qsl("Headless Save Ini Quit Meanwhile");
        QVERIFY(writeProfile(other, QString()));
        QVERIFY(writeProfile(name, qsl("loadProfile(\"%1\", true)").arg(other)));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        auto pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        QCOMPARE(HostManager::self()->hostList().constFirst()->getName(), name);
        pOther->writeProfileIniData(qsl("Headless/first"), qsl("1"));
        const QString profileIni = MudletApp::getMudletPath(enums::profileDataItemPath, other, qsl("profile.ini"));
        QFile::remove(profileIni);
        QVERIFY(QDir().mkpath(profileIni));
        bool closeAsked = false;
        connect(session.host(), &Host::profileSaveFinished, this, [&closeAsked, other]() {
            if (!std::exchange(closeAsked, true)) {
                HostManager::self()->requestProfileClose(other);
            }
        });

        QVERIFY(!session.save());
        QVERIFY(closeAsked);
        QCOMPARE(session.errorString(), qsl("These profiles could not be saved: Headless Save Ini Quit Meanwhile"));
        QCOMPARE(saveCount(name), 2);
        QVERIFY(QDir(profileIni).removeRecursively());
        QVERIFY(session.shutdown(false));
    }

    // The profile's own save leaves a synced module's items out, so its file is the only place they are kept
    void test_aSyncedModuleThatCannotBeWrittenIsAFailedSave()
    {
        const QString name = qsl("Headless Save Synced Module");
        QVERIFY(writeProfile(name, QString()));
        const QString modulePath = mConfigDir.filePath(qsl("headlessSyncedModule.xml"));
        QVERIFY(writeFile(modulePath, packageXml(R"(headlessModuleRan = "yes")")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("installModule([[%1]]); enableModuleSync('headlessSyncedModule')").arg(modulePath)));
        QVERIFY2(session.save(), qPrintable(session.errorString()));
        QVERIFY(QFile::remove(modulePath));
        QVERIFY(QDir().mkpath(modulePath));

        QVERIFY(!session.save());
        QCOMPARE(session.errorString(), qsl("These profiles could not be saved: Headless Save Synced Module"));
        QVERIFY(QDir(modulePath).removeRecursively());
        QVERIFY2(session.save(), qPrintable(session.errorString()));
        QVERIFY(readFile(modulePath).contains(qsl("headlessModuleRan")));
        QVERIFY(session.shutdown(false));
    }

    // As a frontend's function bound into Lua would: the profile's Lua state is still running
    void test_saveFromUnderAScriptIsRefused()
    {
        static THeadlessSession* spSession = nullptr;
        static bool sSaved = true;
        const QString name = qsl("Headless Lua Save");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        spSession = &session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        lua_register(session.host()->getLuaInterpreter()->getLuaGlobalState(), "saveSession", [](lua_State*) -> int {
            sSaved = spSession->save();
            return 0;
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl("saveSession()")));
        QVERIFY(!sSaved);
        QCOMPARE(saveCount(name), 1);
        QVERIFY2(session.save(), qPrintable(session.errorString()));
        QCOMPARE(saveCount(name), 2);
    }

    void test_shutdownDoesNotRequestQuit_data()
    {
        QTest::addColumn<bool>("save");
        QTest::newRow("save") << true;
        QTest::newRow("no save") << false;
    }

    // The quit is queued, so one owed to the shut down profile would end the next start()'s
    void test_shutdownDoesNotRequestQuit()
    {
        QFETCH(bool, save);
        QVERIFY(writeProfile(qsl("Headless Shutdown Quiet"), QString()));
        THeadlessSession session;
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        QCOMPARE(session.start({.profile = qsl("Headless Shutdown Quiet"), .connect = false}), THeadlessSession::StartResult::started);
        session.shutdown(save);
        QTest::qWait(200);
        QCOMPARE(quit.count(), 0);
        QCOMPARE(session.start({.profile = qsl("Headless Shutdown Quiet"), .connect = false}), THeadlessSession::StartResult::started);
        QTest::qWait(200);
        QCOMPARE(quit.count(), 0);
        session.shutdown(false);
    }

    // As a frontend that runs the event loop itself would, opening the profile again once it has closed
    void test_aQuitFromAnEarlierRunIsDropped()
    {
        QVERIFY(writeProfile(qsl("Headless Close Restart"), QString()));
        THeadlessSession session;
        QSignalSpy quit(&session, &THeadlessSession::quitRequested);
        QCOMPARE(session.start({.profile = qsl("Headless Close Restart"), .connect = false}), THeadlessSession::StartResult::started);
        const QPointer<Host> pFirst(session.host());
        QVERIFY(session.runLua(qsl("closeProfile()")));
        QDeadlineTimer deadline(10000);
        while (pFirst && !deadline.hasExpired()) {
            QCoreApplication::processEvents();
        }
        QVERIFY(!pFirst);
        QCOMPARE(quit.count(), 0);
        QVERIFY(session.shutdown(true));
        QCOMPARE(session.start({.profile = qsl("Headless Close Restart"), .connect = false}), THeadlessSession::StartResult::started);
        QTest::qWait(200);
        QCOMPARE(quit.count(), 0);
        QVERIFY(session.host());
        QVERIFY(session.runLua(qsl("closeProfile()")));
        QTRY_COMPARE_WITH_TIMEOUT(quit.count(), 1, 10000);
        session.shutdown(false);
    }

    void test_exitLinesReachOutputLine_data()
    {
        QTest::addColumn<bool>("save");
        QTest::newRow("save") << true;
        QTest::newRow("no save") << false;
    }

    void test_exitLinesReachOutputLine()
    {
        QFETCH(bool, save);
        QVERIFY(writeProfile(qsl("Headless Exit Lines"), qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() feedTriggers("fed by sysExitEvent\n") end))lua")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Exit Lines"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        session.shutdown(save);
        QCOMPARE(outputTexts(output), (QStringList{qsl("fed by sysExitEvent")}));
    }

    // Nothing runs the event loop after the runner's shutdown, where the pool lets such a profile go
    void test_shutdownDeletesAProfileTakenOutOfThePool()
    {
        QVERIFY(writeProfile(qsl("Headless Exit Deletes"), QString()));
        // Named to come after the session's, so the closing pass meets it after the handler deleted it
        QVERIFY(writeProfile(qsl("Headless Exit Removed"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Exit Deletes"), .connect = false}), THeadlessSession::StartResult::started);
        const QPointer<Host> pRemoved(HostManager::self()->openProfile(qsl("Headless Exit Removed"), false));
        QVERIFY(pRemoved);
        // Stands in for a frontend's slot that a sysExitEvent handler reaches
        lua_register(session.host()->getLuaInterpreter()->getLuaGlobalState(), "deleteFromPool", [](lua_State* L) -> int {
            HostManager::self()->deleteHost(QString::fromUtf8(lua_tostring(L, 1)));
            return 0;
        });
        QVERIFY(session.runLua(qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() deleteFromPool("Headless Exit Removed") end))lua")));
        session.shutdown(true);
        QVERIFY2(!pRemoved, "The profile taken out of the pool outlived the shutdown.");
    }

    void test_thePoolOutlivesAProfileBeingLetGo_data()
    {
        QTest::addColumn<bool>("ownProfile");
        QTest::addColumn<bool>("deleteSession");
        QTest::newRow("another profile, shut down unsaved") << false << false;
        QTest::newRow("the session's profile, shut down unsaved") << true << false;
        QTest::newRow("another profile, session deleted") << false << true;
    }

    // One a sysExitEvent handler takes out of the pool is let go from a timer once the handler has returned,
    // and its deletion still reaches the pool
    void test_thePoolOutlivesAProfileBeingLetGo()
    {
        QFETCH(bool, ownProfile);
        QFETCH(bool, deleteSession);
        const QString name = qsl("Headless Let Go %1 %2").arg(ownProfile).arg(deleteSession);
        const QString other = qsl("Headless Let Go Other %1 %2").arg(ownProfile).arg(deleteSession);
        QVERIFY(writeProfile(name, QString()));
        QVERIFY(writeProfile(other, QString()));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QPointer<Host> pOther(HostManager::self()->openProfile(other, false));
        QVERIFY(pOther);
        const QString letGo = ownProfile ? name : other;
        Host* pLetGoHost = ownProfile ? pSession->host() : pOther.data();
        const QPointer<Host> pLetGo(pLetGoHost);
        lua_register(pLetGoHost->getLuaInterpreter()->getLuaGlobalState(), "deleteFromPool", [](lua_State* L) -> int {
            HostManager::self()->deleteHost(QString::fromUtf8(lua_tostring(L, 1)));
            return 0;
        });
        QVERIFY(pLetGoHost->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() deleteFromPool("%1") end))lua").arg(letGo)));
        QVERIFY(HostManager::self()->requestProfileClose(letGo));
        QVERIFY(HostManager::releasingProfile());
        if (deleteSession) {
            delete pSession;
            QVERIFY2(HostManager::self() || !pLetGo, "The pool went before the profile it was letting go.");
            QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
            return;
        }
        QVERIFY(pSession->shutdown(false));
        QVERIFY2(!pLetGo, "A profile being let go outlived the shutdown.");
        QVERIFY(!pSession->host());
        delete pSession;
    }

    // echo() of this phrase commits the OSC 8 examples as game lines, with no trigger pass
    void test_linesWithNoTriggerPassReachOutputLine()
    {
        QVERIFY(writeProfile(qsl("Headless No Pass"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless No Pass"), .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QVERIFY(session.runLua(qsl(R"lua(echo("!osc8-" .. "docs"))lua")));
        QVERIFY2(outputTexts(output).contains(qsl("Docs: https://wiki.mudlet.org/w/Area_51#OSC_8:_Hyperlink_Protocol")), qPrintable(outputTexts(output).join(QLatin1Char('\n'))));
    }

    // A map that will not read takes a turn of the event loop, where a frontend's timer could run a script
    // before the profile's own have started, or delete the profile under the load
    void test_callsDuringTheLoadAreRefused()
    {
        const QString name = qsl("Headless Load Shutdown");
        QVERIFY(writeProfile(name, QString()));
        QVERIFY(writeUnreadableMaps(name, 1));
        THeadlessSession session;
        bool askedDuringTheLoad = false;
        bool ranLua = true;
        bool submitted = true;
        bool saved = true;
        bool shutDown = true;
        QString refusal;
        QTimer::singleShot(0, &session, [&]() {
            askedDuringTheLoad = true;
            ranLua = session.runLua(qsl("headlessDuringLoad = 'yes'"));
            submitted = session.submit(qsl("look"));
            saved = session.save();
            shutDown = session.shutdown(true);
            refusal = session.errorString();
        });
        for (int refusal = 0; refusal < 4; ++refusal) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        }
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(askedDuringTheLoad);
        QVERIFY(!ranLua);
        QVERIFY(!submitted);
        QVERIFY(!saved);
        QVERIFY(!shutDown);
        QCOMPARE(refusal, qsl("Refused while a profile loads; call it once that has returned."));
        QVERIFY(session.host());
        QVERIFY(HostManager::self());
        QVERIFY(session.runLua(qsl("headlessAfterLoad = 'yes'")));
    }

    // As an app's screen closing while it loads
    void test_deletingTheSessionDuringTheLoad()
    {
        const QString name = qsl("Headless Load Delete");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name, recordingScript(recordPath)));
        QVERIFY(writeUnreadableMaps(name, 1));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QTimer::singleShot(0, this, [pSession = QPointer(pSession)]() {
            delete pSession.data();
        });
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::notOpened);
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), 1);
    }

    // As one a frontend gives its map: nothing of the profile's own is on the stack, but the pump is
    void test_shutdownDuringAMapOperationIsRefused()
    {
        const QString name = qsl("Headless Map Shutdown");
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(writeUnreadableMaps(name, 2));
        bool shutDown = true;
        QTimer::singleShot(0, &session, [&session, &shutDown]() {
            shutDown = session.shutdown(true);
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        session.host()->mpMap->restore(QString());
        QVERIFY(!shutDown);
        QVERIFY(HostManager::self());
        QVERIFY(session.shutdown(true));
        QVERIFY(!HostManager::self());
    }

    // As a frontend's function bound into Lua would: the profile's Lua state is still running
    void test_shutdownFromUnderAScriptIsRefused()
    {
        static THeadlessSession* spSession = nullptr;
        QVERIFY(writeProfile(qsl("Headless Lua Shutdown"), QString()));
        THeadlessSession session;
        spSession = &session;
        QCOMPARE(session.start({.profile = qsl("Headless Lua Shutdown"), .connect = false}), THeadlessSession::StartResult::started);
        lua_register(session.host()->getLuaInterpreter()->getLuaGlobalState(), "shutdownSession", [](lua_State*) -> int {
            spSession->shutdown(true);
            return 0;
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl("shutdownSession() headlessAfterShutdown = string.rep('x', 3)")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAfterShutdown"), qsl("xxx"));
        QVERIFY(HostManager::self());
        QVERIFY(session.shutdown(true));
        QVERIFY(!HostManager::self());
    }

    // As a profile's close does while its save waits, which a frontend's timer can run in the middle of
    void test_shutdownDuringANestedEventLoopIsRefused()
    {
        QVERIFY(writeProfile(qsl("Headless Nested Loop"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Nested Loop"), .connect = false}), THeadlessSession::StartResult::started);
        session.host()->getLuaInterpreter()->enterNestedEventLoop();
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(!session.shutdown(true));
        QVERIFY(!session.errorString().isEmpty());
        QVERIFY(HostManager::self());
        session.host()->getLuaInterpreter()->leaveNestedEventLoop();
        QVERIFY(session.shutdown(true));
        QVERIFY(!HostManager::self());
    }

    // Run in the middle of the close, the reset would rebuild the Lua state its exit handlers and save are using
    void test_aResetAskedForFirstEndsBeforeTheClose()
    {
        const QString name = qsl("Headless Reset Shutdown");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name,
                             qsl(R"lua(local file = io.open([[%1]], "ab") file:write("loaded\n") file:close()
%2)lua")
                                     .arg(recordPath, recordingScript(recordPath))));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("resetProfile()")));
        session.shutdown(true);
        QVERIFY(!HostManager::self());
        QCOMPARE(readFile(recordPath), qsl("loaded\nloaded\nsysExitEvent\n"));
    }

    // The profile closes with its reset still asked for, which it then never runs
    void test_shutdownAfterAResetAndACloseFromTheSameScript()
    {
        const QString name = qsl("Headless Reset Close");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name,
                             qsl(R"lua(local file = io.open([[%1]], "ab") file:write("loaded\n") file:close()
%2)lua")
                                     .arg(recordPath, recordingScript(recordPath))));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("resetProfile() closeProfile()")));
        QVERIFY(session.shutdown(true));
        QVERIFY(!HostManager::self());
        QCOMPARE(readFile(recordPath), qsl("loaded\nsysExitEvent\n"));
    }

    void test_deletingTheSessionWithAResetAskedForLetsTheResetEnd()
    {
        const QString name = qsl("Headless Reset Delete");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name,
                             qsl(R"lua(local file = io.open([[%1]], "ab") file:write("loaded\n") file:close()
%2)lua")
                                     .arg(recordPath, recordingScript(recordPath))));
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
            QVERIFY(session.runLua(qsl("resetProfile()")));
        }
        THeadlessSession next;
        QCOMPARE(next.start({.profile = name, .connect = false}), THeadlessSession::StartResult::poolClosing);
        QVERIFY(!next.errorString().isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QCOMPARE(readFile(recordPath), qsl("loaded\nloaded\nsysExitEvent\n"));
        QCOMPARE(saveCount(name), 1);
        QCOMPARE(next.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
    }

    // As an app's screen closing while it waits: the profiles close unsaved
    void test_deletingTheSessionWhileShutdownWaitsForAReset()
    {
        const QString name = qsl("Headless Reset Wait Delete");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name,
                             qsl(R"lua(local file = io.open([[%1]], "ab") file:write("loaded\n") file:close()
%2)lua")
                                     .arg(recordPath, recordingScript(recordPath))));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(pSession->runLua(qsl("resetProfile()")));
        QTimer::singleShot(0, this, [pSession = QPointer(pSession)]() {
            delete pSession.data();
        });
        QVERIFY(!pSession->shutdown(true));
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QCOMPARE(readFile(recordPath), qsl("loaded\nloaded\nsysExitEvent\n"));
        QCOMPARE(saveCount(name), 1);
    }

    void test_deletingTheSessionUnderShutdownsExitHandlers_data()
    {
        QTest::addColumn<bool>("save");
        QTest::newRow("save") << true;
        QTest::newRow("no save") << false;
    }

    // As above, while an exit handler runs the map's pump: the save, if asked for, goes ahead
    void test_deletingTheSessionUnderShutdownsExitHandlers()
    {
        QFETCH(const bool, save);
        const QString name = save ? qsl("Headless Save Delete") : qsl("Headless No Save Delete");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name,
                             qsl(R"lua(%1
registerAnonymousEventHandler("sysExitEvent", function() loadMap() end))lua")
                                     .arg(recordingScript(recordPath))));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(writeUnreadableMaps(name, 2));
        QTimer::singleShot(0, this, [pSession = QPointer(pSession)]() {
            delete pSession.data();
        });
        QVERIFY(pSession->shutdown(save));
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QCOMPARE(readFile(recordPath), qsl("sysExitEvent\n"));
        QCOMPARE(saveCount(name), save ? 2 : 1);
    }

    void test_aStartWhileTheProfilesCloseIsPoolClosing_data()
    {
        QTest::addColumn<int>("close");
        QTest::newRow("deleted") << 0;
        QTest::newRow("shut down, saved") << 1;
        QTest::newRow("shut down, unsaved") << 2;
        QTest::newRow("shut down after a reset") << 3;
    }

    // Until the last one has gone, as its exit handlers, and a shutdown's wait for a reset, may run the event loop
    void test_aStartWhileTheProfilesCloseIsPoolClosing()
    {
        QFETCH(const int, close);
        const QString name = qsl("Headless Closing Start %1").arg(close);
        QVERIFY(writeProfile(name, close == 3 ? QString() : qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() loadMap() end))lua")));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(writeUnreadableMaps(name, 2));
        if (close == 3) {
            QVERIFY(pSession->runLua(qsl("resetProfile()")));
        }
        THeadlessSession next;
        std::optional<THeadlessSession::StartResult> started;
        bool ranLua = true;
        bool shutDown = true;
        QObject context;
        QTimer::singleShot(0, &context, [pSession, &next, &started, &ranLua, &shutDown, name]() {
            started = next.start({.profile = name, .connect = false});
            ranLua = pSession->runLua(qsl("headlessRan = 'yes'"));
            shutDown = pSession->shutdown(true);
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        if (close) {
            QVERIFY(pSession->shutdown(close != 2));
        }
        delete pSession;
        QVERIFY(!HostManager::self());
        QCOMPARE(started, std::optional(THeadlessSession::StartResult::poolClosing));
        QVERIFY(!ranLua);
        QVERIFY(!shutDown);
    }

    void test_callsDuringAShutdownAreRefused_data()
    {
        QTest::addColumn<bool>("save");
        QTest::newRow("while it waits for a reset") << true;
        QTest::newRow("under its exit handlers") << false;
    }

    // As a frontend's timer closing the profiles the other way, or sending to them, while they close
    void test_callsDuringAShutdownAreRefused()
    {
        QFETCH(const bool, save);
        const QString name = save ? qsl("Headless Nested Shutdown") : qsl("Headless Nested Shutdown Exit");
        QVERIFY(writeProfile(name, save ? QString() : qsl(R"lua(registerAnonymousEventHandler("sysExitEvent", function() loadMap() end))lua")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        if (save) {
            QVERIFY(session.runLua(qsl("resetProfile()")));
        } else {
            QVERIFY(writeUnreadableMaps(name, 2));
        }
        std::optional<bool> nested;
        bool ranLua = true;
        bool submitted = true;
        bool saved = true;
        QObject context;
        QTimer::singleShot(0, &context, [&session, &nested, &ranLua, &submitted, &saved, save]() {
            nested = session.shutdown(!save);
            ranLua = session.runLua(qsl("headlessRan = 'yes'"));
            submitted = session.submit(qsl("north"));
            saved = session.save();
        });
        for (int refusal = 0; refusal < 4; ++refusal) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        }
        QVERIFY(session.shutdown(save));
        QCOMPARE(nested, std::optional(false));
        QVERIFY(!ranLua);
        QVERIFY(!submitted);
        QVERIFY(!saved);
        QVERIFY(!HostManager::self());
        QCOMPARE(saveCount(name), save ? 2 : 1);
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.shutdown(false));
        QVERIFY(!HostManager::self());
    }

    // As a frontend's timer closing its screen while a script runs the map's pump
    void test_deletingTheSessionWhileRunLuaPumpsEvents()
    {
        const QString name = qsl("Headless RunLua Delete");
        QVERIFY(writeProfile(name, QString()));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(writeUnreadableMaps(name, 2));
        const QPointer<Host> pHost(pSession->host());
        QTimer::singleShot(0, this, [pSession = QPointer(pSession)]() {
            delete pSession.data();
        });
        QVERIFY(!pSession->runLua(qsl("loadMap() headlessAfterDelete = 'yes' error('after the delete')")));
        QCOMPARE(luaGlobalString(pHost, "headlessAfterDelete"), qsl("yes"));
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QCOMPARE(saveCount(name), 1);
    }

    // As above, from an alias
    void test_deletingTheSessionWhileSubmitPumpsEvents()
    {
        const QString name = qsl("Headless Submit Delete");
        QVERIFY(writeProfile(name, QString()));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(pSession->runLua(qsl(R"lua(tempAlias("^pump$", [[loadMap()]]) tempAlias("^after$", [[headlessAfterDelete = "yes"]]))lua")));
        QVERIFY(writeUnreadableMaps(name, 2));
        const QPointer<Host> pHost(pSession->host());
        QTimer::singleShot(0, this, [pSession = QPointer(pSession)]() {
            delete pSession.data();
        });
        QVERIFY(pSession->submit(qsl("pump\nafter")));
        QVERIFY(pHost);
        QVERIFY(luaGlobalString(pHost, "headlessAfterDelete").isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QCOMPARE(saveCount(name), 1);
    }

    void test_aClosedProfileTakesNoMoreCommands_data()
    {
        QTest::addColumn<int>("closedBy");
        QTest::newRow("a script, before the call") << 0;
        QTest::newRow("the first line's alias") << 1;
        QTest::newRow("the frontend, in the event loop the first line's alias ran") << 2;
    }

    // Its aliases and connection stop once it has closed, before it is deleted, so what is left of a paste goes nowhere
    void test_aClosedProfileTakesNoMoreCommands()
    {
        QFETCH(const int, closedBy);
        const QString name = qsl("Headless Submit Closing %1").arg(closedBy);
        QVERIFY(writeProfile(name, QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempAlias("^first$", [[%1]]) tempAlias("^after$", [[headlessAfter = "yes"]]))lua").arg(closedBy == 1 ? qsl("closeProfile()") : qsl("loadMap()"))));
        QVERIFY(writeUnreadableMaps(name, 2));
        const QPointer<Host> pHost(session.host());
        if (closedBy == 0) {
            QVERIFY(session.runLua(qsl("closeProfile()")));
        } else if (closedBy == 2) {
            QTimer::singleShot(0, this, [name]() {
                if (HostManager* pPool = HostManager::self()) {
                    pPool->requestProfileClose(name);
                }
            });
        }
        QVERIFY(!session.submit(qsl("first\nafter")));
        QCOMPARE(session.errorString(), qsl("The profile is closing, so it takes no more commands."));
        QVERIFY(pHost);
        QVERIFY(pHost->isClosingDown());
        QVERIFY(luaGlobalString(pHost, "headlessAfter").isEmpty());
        QVERIFY(!session.runLua(qsl("headlessAfter = 'yes'")));
        QCOMPARE(session.errorString(), qsl("The profile is closing, so it takes no more commands."));
        QTRY_VERIFY_WITH_TIMEOUT(!session.host(), 10000);
        QVERIFY(session.shutdown(false));
    }

    // Deleting the pool would close the Lua state the script is still running on
    void test_deletingTheSessionFromUnderAScriptClosesTheProfileOnceItEnds()
    {
        static THeadlessSession* spSession = nullptr;
        QVERIFY(writeProfile(qsl("Headless Lua Delete"), QString()));
        QObject owner;
        spSession = new THeadlessSession(&owner);
        QCOMPARE(spSession->start({.profile = qsl("Headless Lua Delete"), .connect = false}), THeadlessSession::StartResult::started);
        const QPointer<Host> pHost(spSession->host());
        lua_register(pHost->getLuaInterpreter()->getLuaGlobalState(), "deleteSession", [](lua_State*) -> int {
            delete spSession;
            spSession = nullptr;
            return 0;
        });
        QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("deleteSession() headlessAfterDelete = string.rep('x', 3)")));
        QVERIFY(pHost);
        QCOMPARE(luaGlobalString(pHost, "headlessAfterDelete"), qsl("xxx"));
        // Then it closes, so that another session can start
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QVERIFY(!pHost);
        THeadlessSession next;
        QCOMPARE(next.start({.profile = qsl("Headless Lua Delete"), .connect = false}), THeadlessSession::StartResult::started);
    }

    // The buffer is still committing the line the handler was handed, with no script running
    void test_deletingTheSessionFromAGameLineHandler()
    {
        const QString name = qsl("Headless Game Line Delete");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        // The trigger runs once the line is handed out, so it shows the commit finished
        QVERIFY(writeProfile(name,
                             qsl(R"lua(%1
tempTrigger("outer line", function()
  local file = io.open([[%2]], "ab")
  file:write("trigger\n")
  file:close()
end))lua")
                                     .arg(recordingScript(recordPath), recordPath)));
        TelnetServerStub server;
        server.setWelcomeMessage(qsl("outer line"));
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        const QPointer<Host> pHost(pSession->host());
        connect(pSession, &THeadlessSession::outputLine, this, [pSession](const QString&, const QString& text, bool) {
            if (text == qsl("outer line")) {
                delete pSession;
            }
        });
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QVERIFY(!pHost);
        QCOMPARE(readFile(recordPath), qsl("trigger\nsysExitEvent\n"));
        QCOMPARE(saveCount(name), 1);
    }

    // Not what a handler may do, but the session's own part holds: deleted from a timer in the loop, its pool
    // waits for the line's commit to end
    void test_deletingTheSessionInAnOutputLineHandlersEventLoop()
    {
        const QString name = qsl("Headless Nested Loop Delete");
        const QString recordPath = qsl("%1/%2.record").arg(mConfigDir.path(), name);
        QVERIFY(writeProfile(name,
                             qsl(R"lua(%1
tempTrigger("outer line", function()
  local file = io.open([[%2]], "ab")
  file:write("trigger\n")
  file:close()
end))lua")
                                     .arg(recordingScript(recordPath), recordPath)));
        TelnetServerStub server;
        server.setWelcomeMessage(qsl("outer line"));
        server.start(qsl("127.0.0.1"));
        QVERIFY(server.isListening());
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        const QPointer<Host> pHost(pSession->host());
        bool sessionGoneInTheLoop = false;
        bool poolKeptThroughTheLoop = false;
        connect(pSession,
                &THeadlessSession::outputLine,
                this,
                [this, pSession, pAlive = QPointer(pSession), &sessionGoneInTheLoop, &poolKeptThroughTheLoop](const QString&, const QString& text, bool) {
                    if (text != qsl("outer line")) {
                        return;
                    }
                    QTimer::singleShot(0, this, [pSession]() {
                        delete pSession;
                    });
                    QEventLoop loop;
                    QTimer::singleShot(200, &loop, &QEventLoop::quit);
                    loop.exec();
                    sessionGoneInTheLoop = !pAlive;
                    poolKeptThroughTheLoop = HostManager::self() != nullptr;
                });
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QVERIFY(sessionGoneInTheLoop);
        QVERIFY(poolKeptThroughTheLoop);
        QVERIFY(!pHost);
        QCOMPARE(readFile(recordPath), qsl("trigger\nsysExitEvent\n"));
    }

    void test_deletingTheSessionFromAProfileLoadedHandler()
    {
        const QString name = qsl("Headless Loaded Delete");
        QVERIFY(writeProfile(name, QString()));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QString profileAfterDelete;
        connect(pSession, &THeadlessSession::profileLoaded, this, [pSession, &profileAfterDelete](const QString& profile) {
            delete pSession;
            profileAfterDelete = profile;
        });
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QCOMPARE(profileAfterDelete, name);
        QVERIFY(!HostManager::self());
    }

    void test_deletingTheSessionFromAnOutputLineHandler_data()
    {
        QTest::addColumn<bool>("throughSession");
        QTest::newRow("runLua()") << true;
        QTest::newRow("a script of the profile's own") << false;
    }

    // As above, from under the script that fed the line and, if the session ran it, that call
    void test_deletingTheSessionFromAnOutputLineHandler()
    {
        QFETCH(const bool, throughSession);
        const QString name = throughSession ? qsl("Headless Handler Delete") : qsl("Headless Handler Delete Own");
        QVERIFY(writeProfile(name, QString()));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QPointer<Host> pHost(pSession->host());
        QString profileAfterDelete;
        connect(pSession, &THeadlessSession::outputLine, this, [pSession, &profileAfterDelete](const QString& profile, const QString& text, bool) {
            if (text == qsl("outer line")) {
                delete pSession;
                profileAfterDelete = profile;
            }
        });
        const QString script = qsl(R"lua(feedTriggers("outer line\n") headlessAfterDelete = "yes")lua");
        QVERIFY(throughSession ? pSession->runLua(script) : pHost->getLuaInterpreter()->compileAndExecuteScript(script));
        QCOMPARE(luaGlobalString(pHost, "headlessAfterDelete"), qsl("yes"));
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QVERIFY(!pHost);
        QCOMPARE(profileAfterDelete, name);
    }

    void test_deletingTheSessionFromALocalTextOrErrorConsoleHandler_data()
    {
        QTest::addColumn<QString>("way");
        QTest::newRow("a command's echo") << qsl("submit");
        QTest::newRow("Mudlet's messages") << qsl("connect");
        QTest::newRow("a Lua error") << qsl("error");
    }

    // Sent from the middle of what the profile is doing, which goes on once the handler returns
    void test_deletingTheSessionFromALocalTextOrErrorConsoleHandler()
    {
        QFETCH(const QString, way);
        const QString name = qsl("Headless Local Delete %1").arg(way);
        QVERIFY(writeProfile(name, QString()));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        QCOMPARE(pSession->start({.profile = name, .connect = way == qsl("connect")}), THeadlessSession::StartResult::started);
        const QPointer<Host> pHost(pSession->host());
        bool deleted = false;
        bool hostOutlivedTheHandler = false;
        QString profileAfterDelete;
        const auto deleteSession = [&](const QString& profile) {
            if (!deleted) {
                deleted = true;
                delete pSession;
                hostOutlivedTheHandler = !pHost.isNull();
                profileAfterDelete = profile;
            }
        };
        connect(pSession, &THeadlessSession::localText, this, [&](const QString& profile, const QString& text) {
            if (way != qsl("error") && (way != qsl("connect") || text.contains(qsl("connection made")))) {
                deleteSession(profile);
            }
        });
        connect(pSession, &THeadlessSession::errorConsoleText, this, [&](const QString& profile, const QString&) {
            deleteSession(profile);
        });
        if (way == qsl("submit")) {
            QVERIFY(pSession->submit(qsl("look")));
        } else if (way == qsl("error")) {
            QVERIFY(!pSession->runLua(qsl("this is not Lua")));
        }
        QTRY_VERIFY_WITH_TIMEOUT(deleted, 10000);
        QVERIFY(hostOutlivedTheHandler);
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self(), 10000);
        QVERIFY(!pHost);
        QCOMPARE(profileAfterDelete, name);
    }

    // A line fed from a handler would be committed in the middle of the one it is handed, and a shutdown
    // there would delete the buffer that is committing it
    void test_outputLineHandlersCannotReenterTheProfile()
    {
        QVERIFY(writeProfile(qsl("Headless Reentry"), QString()));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Reentry"), .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(tempAlias("^headless", [[headlessAliasRan = "yes"]]))lua")));
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        bool ranLua = true;
        bool submitted = true;
        bool saved = true;
        QString refusal;
        // One with no profiles of its own has nothing to refuse
        THeadlessSession unstarted;
        bool unstartedShutDown = false;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &ranLua, &submitted, &saved, &refusal, &unstarted, &unstartedShutDown](const QString&, const QString& text, bool) {
            if (text == qsl("outer line")) {
                ranLua = session.runLua(qsl(R"lua(feedTriggers("inner line\n"))lua"));
                submitted = session.submit(qsl("headless"));
                saved = session.save();
                session.shutdown(true);
                refusal = session.errorString();
                unstartedShutDown = unstarted.shutdown(false);
            }
        });
        for (int refusal = 0; refusal < 4; ++refusal) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        }
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("outer line\n") feedTriggers("after\n"))lua")));
        QVERIFY(!ranLua);
        QVERIFY(!submitted);
        QVERIFY(!saved);
        QCOMPARE(refusal,
                 qsl("Refused from a handler of outputLine, lineShown, mainConsoleCleared, localText, errorConsoleText, debugText, hiddenInputWanted or a ...Requested signal; queue the call "
                     "instead."));
        QVERIFY(unstartedShutDown);
        QVERIFY(HostManager::self());
        QCOMPARE(outputTexts(output), (QStringList{qsl("outer line"), qsl("after")}));
        QVERIFY(luaGlobalString(session.host(), "headlessAliasRan").isEmpty());
        QVERIFY(session.submit(qsl("headless")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasRan"), qsl("yes"));
    }

    void test_aChosenConfigDirKeepsTheProfilesAndSettings()
    {
        QVERIFY(!HostManager::self());
        // The root is settled once a process, so the other tests' one is forgotten, and brought back after
        MudletApp::setConfigPath(QString());
        const auto forget = qScopeGuard([]() {
            MudletApp::setConfigPath(QString());
        });
        QTemporaryDir parent;
        QVERIFY(parent.isValid());
        const QString chosen = parent.filePath(qsl("chosen"));
        QVERIFY(QDir().mkpath(chosen));
        QVERIFY(writeFile(qsl("%1/Mudlet.ini").arg(chosen), "[General]\nenableMuteAPI=true\n"));
        const QString name = qsl("Headless Own Folder");
        const QString profileFolder = qsl("%1/profiles/%2").arg(chosen, name);
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false, .host = qsl("game.example"), .configDir = chosen + qsl("/./")}), THeadlessSession::StartResult::started);
            QVERIFY(MudletApp::portableRootInUse());
            QVERIFY(QDir(profileFolder).exists());
            QVERIFY(!QDir(qsl("%1/mudlet/profiles/%2").arg(mConfigDir.path(), name)).exists());
            QVERIFY(session.runLua(qsl("headlessHome = getMudletHomeDir() headlessMuted = tostring(getConfig('muteMediaAPI'))")));
            QCOMPARE(luaGlobalString(session.host(), "headlessHome"), profileFolder);
            QCOMPARE(luaGlobalString(session.host(), "headlessMuted"), qsl("true"));
            HostManager::self()->media().setApiMuted(false);
            QVERIFY(session.shutdown(true));
        }
        QVERIFY(readFile(qsl("%1/Mudlet.ini").arg(chosen)).contains(qsl("enableMuteAPI=false")));
        QVERIFY(!QDir(MudletApp::getMudletPath(enums::profileXmlFilesPath, name)).isEmpty());

        // Settled now, so it can be named again but no other can
        THeadlessSession again;
        QCOMPARE(again.start({.profile = name, .connect = false, .configDir = chosen}), THeadlessSession::StartResult::started);
        QVERIFY(again.shutdown(false));
        THeadlessSession elsewhere;
        QCOMPARE(elsewhere.start({.profile = name, .connect = false, .configDir = parent.path()}), THeadlessSession::StartResult::invalidConfigDir);
        QVERIFY(elsewhere.errorString().contains(chosen));
        // Nor is the GUI's folder, which an empty one names
        THeadlessSession unnamed;
        QCOMPARE(unnamed.start({.profile = name, .connect = false}), THeadlessSession::StartResult::invalidConfigDir);
        QVERIFY(unnamed.errorString().contains(chosen));
        QVERIFY(!HostManager::self());
    }

    // A profile of the same name in the GUI keeps its passwords, and the chosen folder's are its own
    void test_aChosenConfigDirKeepsItsPasswordsApart()
    {
        QVERIFY(!HostManager::self());
        const bool storedSecurely = MudletApp::storingPasswordsSecurely();
        const QString name = qsl("Headless Shared Name");
        const QString desktopStore = qsl("%1/profiles/%2").arg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation), name);
        const auto restore = qScopeGuard([storedSecurely, desktopStore]() {
            MudletApp::setConfigPath(QString());
            MudletApp::setStorePasswordsSecurely(storedSecurely);
            QDir(desktopStore).removeRecursively();
        });
        // In test mode the GUI's store is the file one, as when Preferences say to keep passwords in the profile
        MudletApp::setConfigPath(QString());
        {
            const HostManager desktopPool;
        }
        MudletApp::setStorePasswordsSecurely(true);
        for (const QString& key : {qsl("character"), qsl("proxy")}) {
            QVERIFY(storePassword(name, key, qsl("desktop")));
        }
        const QMap<QString, QByteArray> desktopFiles = filesUnder(desktopStore);
        QVERIFY(!desktopFiles.isEmpty());

        MudletApp::setConfigPath(QString());
        QTemporaryDir parent;
        QVERIFY(parent.isValid());
        const QString chosen = parent.filePath(qsl("chosen"));
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = name, .connect = false, .host = qsl("game.example"), .configDir = chosen}), THeadlessSession::StartResult::started);
            QVERIFY(MudletApp::storingPasswordsSecurely());
            QCOMPARE(retrievePassword(name, qsl("character")), QString());
            QVERIFY(storePassword(name, qsl("character"), qsl("chosen")));
            QCOMPARE(retrievePassword(name, qsl("character")), qsl("chosen"));
            QVERIFY(session.shutdown(false));
        }
        QCOMPARE(filesUnder(desktopStore), desktopFiles);
        const QStringList chosenFiles = filesUnder(chosen).keys();
        const QString credentials = qsl("credentials/profiles/%1/").arg(name);
        const QString profile = qsl("profiles/%1/").arg(name);
        QVERIFY2(chosenFiles.contains(credentials + qsl("encryption_key")), qPrintable(chosenFiles.join(qsl(", "))));
        QVERIFY2(std::any_of(chosenFiles.cbegin(),
                             chosenFiles.cend(),
                             [&credentials](const QString& file) {
                                 return file.startsWith(credentials + qsl("passwords/"));
                             }),
                 qPrintable(chosenFiles.join(qsl(", "))));
        QVERIFY2(std::none_of(chosenFiles.cbegin(),
                              chosenFiles.cend(),
                              [&profile](const QString& file) {
                                  return file.startsWith(profile) && (file.endsWith(qsl("/encryption_key")) || file.contains(qsl("/passwords/")));
                              }),
                 qPrintable(chosenFiles.join(qsl(", "))));

        MudletApp::setConfigPath(QString());
        {
            const HostManager desktopAgain;
        }
        MudletApp::setStorePasswordsSecurely(true);
        QCOMPARE(retrievePassword(name, qsl("character")), qsl("desktop"));
    }

    void test_aConfigDirThatCannotBeUsedIsRefused()
    {
        QVERIFY(!HostManager::self());
        MudletApp::setConfigPath(QString());
        const auto forget = qScopeGuard([]() {
            MudletApp::setConfigPath(QString());
        });
        QTemporaryDir parent;
        QVERIFY(parent.isValid());
        const QString file = parent.filePath(qsl("a file"));
        QVERIFY(writeFile(file, "not a folder"));
        const QString underFile = qsl("%1/below").arg(file);
        const QString relative = qsl("headless-relative-config-dir");
        for (const QString& configDir : {file, underFile, relative}) {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = qsl("Headless Refused Folder"), .connect = false, .host = qsl("game.example"), .configDir = configDir}),
                     THeadlessSession::StartResult::invalidConfigDir);
            QVERIFY2(session.errorString().contains(configDir), qPrintable(session.errorString()));
            QVERIFY(!HostManager::self());
            QVERIFY(!MudletApp::configRootSettled());
        }
        QVERIFY(!QFileInfo::exists(relative));

        const QString made = parent.filePath(qsl("made/here"));
        {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = qsl("Headless Made Folder"), .connect = false, .host = qsl("game.example"), .configDir = made}), THeadlessSession::StartResult::started);
            QVERIFY(QDir(qsl("%1/profiles/Headless Made Folder").arg(made)).exists());
            QVERIFY(session.shutdown(false));
        }
        MudletApp::setConfigPath(QString());

        // One settled by the usual means is not swapped for a folder named afterwards
        {
            const HostManager pool;
        }
        QVERIFY(MudletApp::configRootSettled());
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless Refused Folder"), .connect = false, .host = qsl("game.example"), .configDir = parent.path()}),
                 THeadlessSession::StartResult::invalidConfigDir);
        QVERIFY(!QDir(parent.filePath(qsl("profiles"))).exists());
        const QString settled = MudletApp::getMudletPath(enums::mainPath);
        QCOMPARE(session.start({.profile = qsl("Headless Refused Folder"), .connect = false, .host = qsl("game.example"), .configDir = settled}), THeadlessSession::StartResult::invalidConfigDir);
        QVERIFY2(session.errorString().contains(qsl("the GUI's")), qPrintable(session.errorString()));
        QVERIFY(!HostManager::self());
    }

    // Its profiles' passwords are in the keychain or AppConfigLocation, which a chosen folder would not look in
    void test_theGUIsFolderCannotBeChosen()
    {
        QVERIFY(!HostManager::self());
        MudletApp::setConfigPath(QString());
        const auto forget = qScopeGuard([]() {
            MudletApp::setConfigPath(QString());
        });
        const QString guiFolder = MudletApp::resolveConfigRoot(MudletApp::executableDir()).path;
        QVERIFY(QDir().mkpath(guiFolder));
        QTemporaryDir parent;
        QVERIFY(parent.isValid());
        QStringList spellings{guiFolder, qsl("%1/profiles/..").arg(guiFolder)};
#if !defined(Q_OS_WINDOWS)
        const QString alias = parent.filePath(qsl("alias"));
        QVERIFY(QFile::link(guiFolder, alias));
        spellings << alias;
#endif
        for (const QString& configDir : spellings) {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = qsl("Headless GUI Folder"), .connect = false, .host = qsl("game.example"), .configDir = configDir}), THeadlessSession::StartResult::invalidConfigDir);
            QVERIFY2(session.errorString().contains(qsl("the GUI's")), qPrintable(session.errorString()));
            QVERIFY(!HostManager::self());
            QVERIFY(!MudletApp::configRootSettled());
        }
#if !defined(Q_OS_WINDOWS)
        // Nor by another name once the GUI's is settled
        {
            const HostManager pool;
        }
        QVERIFY(MudletApp::configRootSettled());
        THeadlessSession session;
        QCOMPARE(session.start({.profile = qsl("Headless GUI Folder"), .connect = false, .host = qsl("game.example"), .configDir = alias}), THeadlessSession::StartResult::invalidConfigDir);
        QVERIFY2(session.errorString().contains(qsl("the GUI's")), qPrintable(session.errorString()));
#endif
    }

    // The GUI picks again at every launch, and a chosen folder that fills with profiles can win that pick
    void test_noFolderTheGUICouldPickCanBeChosen()
    {
#if defined(Q_OS_WINDOWS)
        QSKIP("redirects HOME and makes symlinks");
#else
        QVERIFY(!HostManager::self());
        MudletApp::setConfigPath(QString());
        QTemporaryDir sandbox;
        QVERIFY(sandbox.isValid());
        const QByteArray savedHome = qgetenv("HOME");
        const QByteArray savedXdg = qgetenv("XDG_CONFIG_HOME");
        const auto restore = qScopeGuard([savedHome, savedXdg]() {
            MudletApp::setConfigPath(QString());
            qputenv("HOME", savedHome);
            qputenv("XDG_CONFIG_HOME", savedXdg);
        });
        qputenv("HOME", sandbox.path().toUtf8());
        const QString xdgHome = sandbox.filePath(qsl("xdg"));
        qputenv("XDG_CONFIG_HOME", xdgHome.toUtf8());
        const QString dotConfig = sandbox.filePath(qsl(".config"));
        for (const QString& folder : {xdgHome, dotConfig}) {
            QVERIFY(QDir().mkpath(folder));
        }
        QVERIFY(QFile::link(xdgHome, sandbox.filePath(qsl("xdg-link"))));
        QVERIFY(QFile::link(dotConfig, sandbox.filePath(qsl("config-link"))));
        const QString legacy = MudletApp::legacyConfigDir();
        const QString xdg = qsl("%1/mudlet").arg(xdgHome);
        QCOMPARE(legacy, qsl("%1/mudlet").arg(dotConfig));
        const auto refused = [&legacy, &xdg](const QString& configDir) {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = qsl("Headless GUI Folder"), .connect = false, .host = qsl("game.example"), .configDir = configDir}), THeadlessSession::StartResult::invalidConfigDir);
            QVERIFY2(session.errorString().contains(qsl("the GUI's")), qPrintable(session.errorString()));
            QVERIFY(!HostManager::self());
            QVERIFY(!MudletApp::configRootSettled());
            QVERIFY(!QFileInfo::exists(qsl("%1/profiles").arg(legacy)));
            QVERIFY(!QFileInfo::exists(qsl("%1/profiles").arg(xdg)));
        };

        // Neither exists, so the GUI would take the XDG one; either can still become its pick
        QCOMPARE(MudletApp::resolveConfigRoot(MudletApp::executableDir()).path, xdg);
        for (const QString& configDir : {legacy, sandbox.filePath(qsl("config-link/mudlet")), sandbox.filePath(qsl("xdg-link/mudlet"))}) {
            refused(configDir);
            if (QTest::currentTestFailed()) {
                return;
            }
            QVERIFY(!QFileInfo::exists(legacy));
            QVERIFY(!QFileInfo::exists(xdg));
        }

        // Only the legacy one holds profiles, so the GUI is on it; the XDG one would win once it holds some
        QVERIFY(QDir().mkpath(qsl("%1/profiles/GUI Profile").arg(legacy)));
        QCOMPARE(MudletApp::resolveConfigRoot(MudletApp::executableDir()).path, legacy);
        const auto withLegacyProfiles = [&legacy](const QString& configDir) {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = qsl("Headless GUI Folder"), .connect = false, .host = qsl("game.example"), .configDir = configDir}), THeadlessSession::StartResult::invalidConfigDir);
            QVERIFY2(session.errorString().contains(qsl("the GUI's")), qPrintable(session.errorString()));
            QVERIFY(!MudletApp::configRootSettled());
            QCOMPARE(QDir(qsl("%1/profiles").arg(legacy)).entryList(QDir::Dirs | QDir::NoDotAndDotDot), QStringList{qsl("GUI Profile")});
        };
        for (const QString& configDir : {xdg, sandbox.filePath(qsl("xdg-link/mudlet"))}) {
            withLegacyProfiles(configDir);
            if (QTest::currentTestFailed()) {
                return;
            }
            QVERIFY(!QFileInfo::exists(xdg));
        }
#endif
    }

    // Reached through a link to nowhere, or named by a portable.txt the GUI refuses today, it is still the GUI's
    void test_theGUIsFoldersAreRefusedHoweverTheyAreReached()
    {
#if defined(Q_OS_WINDOWS)
        QSKIP("redirects HOME and makes symlinks");
#else
        QVERIFY(!HostManager::self());
        MudletApp::setConfigPath(QString());
        QTemporaryDir sandbox;
        QVERIFY(sandbox.isValid());
        const QByteArray savedHome = qgetenv("HOME");
        const QByteArray savedXdg = qgetenv("XDG_CONFIG_HOME");
        const auto restore = qScopeGuard([savedHome, savedXdg]() {
            MudletApp::setConfigPath(QString());
            qputenv("HOME", savedHome);
            qputenv("XDG_CONFIG_HOME", savedXdg);
        });
        qputenv("HOME", sandbox.path().toUtf8());
        qunsetenv("XDG_CONFIG_HOME");
        const auto refusedSaying = [](const QString& configDir, const QString& why) {
            THeadlessSession session;
            QCOMPARE(session.start({.profile = qsl("Headless Other Mudlet"), .connect = false, .host = qsl("game.example"), .configDir = configDir}), THeadlessSession::StartResult::invalidConfigDir);
            QVERIFY2(session.errorString().contains(why), qPrintable(session.errorString()));
            QVERIFY(!HostManager::self());
            QVERIFY(!MudletApp::configRootSettled());
        };

        // ~/.config linked to a folder not there yet: making its target would make the GUI's folder
        const QString dots = sandbox.filePath(qsl("dots/config"));
        const QString dotConfig = sandbox.filePath(qsl(".config"));
        QVERIFY(QFile::link(dots, dotConfig));
        refusedSaying(qsl("%1/mudlet").arg(dots), qsl("the GUI's"));
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY(!QFileInfo::exists(sandbox.filePath(qsl("dots"))));
        QVERIFY(QFile::remove(dotConfig));

        // A portable.txt the GUI refuses today, as the folder it names has no parent yet
        const QString legacy = MudletApp::legacyConfigDir();
        QVERIFY(QDir().mkpath(legacy));
        const QString portableRoot = sandbox.filePath(qsl("missing/data"));
        QVERIFY(writeFile(qsl("%1/portable.txt").arg(legacy), portableRoot.toUtf8()));
        refusedSaying(portableRoot, qsl("the GUI's"));
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY(!QFileInfo::exists(sandbox.filePath(qsl("missing"))));
#if defined(Q_OS_MACOS)
        refusedSaying(sandbox.filePath(qsl(".config/MUDLET")), qsl("the GUI's"));
        if (QTest::currentTestFailed()) {
            return;
        }
#endif
        QVERIFY(QFile::remove(qsl("%1/portable.txt").arg(legacy)));

        // The Flatpak GUI's, whose XDG_CONFIG_HOME is its own under HOME, made or not
        const QString flatpak = sandbox.filePath(qsl(".var/app/org.mudlet.mudlet/config/mudlet"));
        refusedSaying(flatpak, qsl("the GUI's"));
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY(!QFileInfo::exists(sandbox.filePath(qsl(".var"))));
        QVERIFY(QDir().mkpath(qsl("%1/profiles/Flatpak Profile").arg(flatpak)));
        refusedSaying(flatpak, qsl("the GUI's"));
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY(!QFileInfo::exists(qsl("%1/profiles/Headless Other Mudlet").arg(flatpak)));

        // A folder that is none of the GUI's is the app's, profiles it put there before its first start
        // included, and again in a later run that has nothing of the first one's but its profiles
        const QString seeded = sandbox.filePath(qsl("app data"));
        QVERIFY(QDir().mkpath(qsl("%1/profiles/Seeded Game").arg(seeded)));
        for (int run = 0; run < 2; ++run) {
            THeadlessSession session;
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("\"%1\" already holds profiles").arg(QRegularExpression::escape(seeded))));
            QCOMPARE(session.start({.profile = qsl("Headless Other Mudlet"), .connect = false, .host = qsl("game.example"), .configDir = seeded}), THeadlessSession::StartResult::started);
            QVERIFY(QDir(qsl("%1/profiles/Headless Other Mudlet").arg(seeded)).exists());
            QVERIFY(QDir(qsl("%1/profiles/Seeded Game").arg(seeded)).exists());
            QVERIFY(session.shutdown(false));
            MudletApp::setConfigPath(QString());
            QVERIFY(QDir(qsl("%1/credentials").arg(seeded)).removeRecursively());
        }
#endif
    }
};

#include "HeadlessSessionTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionTest)
