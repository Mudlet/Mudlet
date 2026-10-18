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

#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>
#include <optional>
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
#include "PortableModeTestHelper.h"
#include "TAppFrontend.h"
#include "TGameDetails.h"
#include "THeadlessSession.h"
#include "TLuaInterpreter.h"
#include "TMap.h"
#include "TelnetServerStub.h"

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

    static bool writeProfile(const QString& name, const QString& script)
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
<HostPackage><Host></Host></HostPackage>
<ScriptPackage><Script isActive="yes" isFolder="no"><name>headlessSession</name><packageName></packageName><script>%1</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)")
                               .arg(script.toHtmlEscaped())
                               .toUtf8());
        return true;
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

    static qsizetype saveCount(const QString& name) { return QDir(MudletApp::getMudletPath(enums::profileXmlFilesPath, name)).entryList(QStringList{qsl("*.xml")}, QDir::Files).size(); }

    static QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    static QStringList outputTexts(const QSignalSpy& spy)
    {
        QStringList texts;
        for (const auto& arguments : spy) {
            texts << arguments.at(1).toString();
        }
        return texts;
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
        const auto keychain = answerTheKeychain(QKeychain::EntryNotFound, QKeychain::OtherError, qsl("synthetic: not removed"), jobs);

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
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        QVERIFY(QDir().mkpath(saveFolder));
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        QVERIFY(saveFile.open(QIODevice::WriteOnly));
        saveFile.write(qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>opener</name><packageName></packageName><script>loadProfile("%1", true)</script><eventHandlerList/></Script></ScriptPackage>
<HostPackage><Host)")
                               .arg(other)
                               .toUtf8());
        saveFile.close();

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::loadFailed);
        QVERIFY2(HostManager::self()->getHost(other), "The script read before the save broke off did not run.");
        QVERIFY(session.shutdown(true));
        QCOMPARE(saveCount(name), 1);
        QCOMPARE(saveCount(other), 2);
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
    // event loop until the connection is made: the GUI keeps it, but here it is dropped, as is that of a
    // profile whose session has gone
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
        QCOMPARE(luaGlobalString(pHost, "headlessConnected"), qsl("yes"));
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
        quint16 port = 0;
        {
            QTcpServer closed;
            QVERIFY(closed.listen(QHostAddress::LocalHost));
            port = closed.serverPort();
        }
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
        bool shutDown = true;
        QString refusal;
        QTimer::singleShot(0, &session, [&]() {
            askedDuringTheLoad = true;
            ranLua = session.runLua(qsl("headlessDuringLoad = 'yes'"));
            submitted = session.submit(qsl("look"));
            shutDown = session.shutdown(true);
            refusal = session.errorString();
        });
        for (int refusal = 0; refusal < 3; ++refusal) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        }
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(askedDuringTheLoad);
        QVERIFY(!ranLua);
        QVERIFY(!submitted);
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
        QObject context;
        QTimer::singleShot(0, &context, [&session, &nested, &ranLua, &submitted, save]() {
            nested = session.shutdown(!save);
            ranLua = session.runLua(qsl("headlessRan = 'yes'"));
            submitted = session.submit(qsl("north"));
        });
        for (int refusal = 0; refusal < 3; ++refusal) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        }
        QVERIFY(session.shutdown(save));
        QCOMPARE(nested, std::optional(false));
        QVERIFY(!ranLua);
        QVERIFY(!submitted);
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
        QString refusal;
        // One with no profiles of its own has nothing to refuse
        THeadlessSession unstarted;
        bool unstartedShutDown = false;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &ranLua, &submitted, &refusal, &unstarted, &unstartedShutDown](const QString&, const QString& text, bool) {
            if (text == qsl("outer line")) {
                ranLua = session.runLua(qsl(R"lua(feedTriggers("inner line\n"))lua"));
                submitted = session.submit(qsl("headless"));
                session.shutdown(true);
                refusal = session.errorString();
                unstartedShutDown = unstarted.shutdown(false);
            }
        });
        for (int refusal = 0; refusal < 3; ++refusal) {
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        }
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("outer line\n") feedTriggers("after\n"))lua")));
        QVERIFY(!ranLua);
        QVERIFY(!submitted);
        QCOMPARE(refusal, qsl("Refused from a handler of outputLine, localText or errorConsoleText; queue the call instead."));
        QVERIFY(unstartedShutDown);
        QVERIFY(HostManager::self());
        QCOMPARE(outputTexts(output), (QStringList{qsl("outer line"), qsl("after")}));
        QVERIFY(luaGlobalString(session.host(), "headlessAliasRan").isEmpty());
        QVERIFY(session.submit(qsl("headless")));
        QCOMPARE(luaGlobalString(session.host(), "headlessAliasRan"), qsl("yes"));
    }
};

#include "HeadlessSessionTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionTest)
