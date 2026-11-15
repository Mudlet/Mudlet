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

/*
 * A game played through a connection the app embedding Mudlet makes itself (Options::supplyTransport), as a
 * browser must, with no socket anywhere: the test stands in for both the app and the game.
 *
 * Run with: ctest -R HeadlessSessionTransportTest -V
 */

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"
#include "TLuaInterpreter.h"

#include "GroupedTest.h"

namespace {
const QByteArray IAC_WILL_GMCP("\xff\xfb\xc9", 3);
const QByteArray IAC_DO_GMCP("\xff\xfd\xc9", 3);
const QByteArray IAC_SB_GMCP("\xff\xfa\xc9", 3);
const QByteArray IAC_SE("\xff\xf0", 2);
} // namespace

class HeadlessSessionTransportTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeProfile(const QString& name)
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host mEnableGMCP="yes"></Host></HostPackage>
</MudletPackage>
)");
        return MudletApp::writeProfileData(name, qsl("url"), qsl("game.example")).first && MudletApp::writeProfileData(name, qsl("port"), qsl("4000")).first;
    }

    static QByteArray written(const QSignalSpy& spy)
    {
        QByteArray result;
        for (const auto& arguments : spy) {
            result += arguments.at(1).toByteArray();
        }
        return result;
    }

    static QString printed(const QSignalSpy& spy)
    {
        QString result;
        for (const auto& arguments : spy) {
            result += arguments.at(1).toString();
        }
        return result;
    }

    static QString luaGlobal(THeadlessSession& session, const char* name)
    {
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
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

    void test_aGameIsPlayedThroughTheAppsConnection()
    {
        const QString name = qsl("Headless Transport");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QSignalSpy dials(&session, &THeadlessSession::transportDialRequested);
        QSignalSpy writes(&session, &THeadlessSession::transportWriteRequested);
        QSignalSpy closes(&session, &THeadlessSession::transportCloseRequested);
        QSignalSpy connected(&session, &THeadlessSession::connected);
        QSignalSpy disconnected(&session, &THeadlessSession::disconnected);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QCOMPARE(session.start({.profile = name, .connect = true, .supplyTransport = true}), THeadlessSession::StartResult::started);

        QTRY_COMPARE_WITH_TIMEOUT(dials.size(), 1, 10000);
        QCOMPARE(dials.at(0).at(1).toString(), qsl("game.example"));
        QCOMPARE(dials.at(0).at(2).toInt(), 4000);
        QCOMPARE(dials.at(0).at(3).toBool(), false);
        QCOMPARE(session.host()->mTelnet.getConnectionState(), QAbstractSocket::ConnectingState);
        QVERIFY(session.runLua(qsl(R"lua(
registerAnonymousEventHandler("sysConnectionEvent", function() connectionEvent = "yes" end)
registerAnonymousEventHandler("sysDisconnectionEvent", function() disconnectionEvent = "yes" end)
registerAnonymousEventHandler("gmcp.Char.Vitals", function() vitalsHp = gmcp.Char.Vitals.hp end)
tempTrigger("A dragon arrives", function() dragonSeen = "yes" send("kill dragon") end)
)lua")));

        session.transportConnected();
        QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 1, 10000);
        QCOMPARE(luaGlobal(session, "connectionEvent"), qsl("yes"));
        QCOMPARE(session.host()->mTelnet.getConnectionState(), QAbstractSocket::ConnectedState);

        session.transportReceived(IAC_WILL_GMCP + "Welcome to the game!\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(written(writes).contains(IAC_DO_GMCP), 10000);
        // A chunk split inside the subnegotiation, as a connection may split it
        const QByteArray gmcp = IAC_SB_GMCP + R"(Char.Vitals {"hp": 42})" + IAC_SE;
        session.transportReceived(gmcp.left(9));
        session.transportReceived(gmcp.mid(9) + "A dragon arrives.\r\n");
        QTRY_COMPARE_WITH_TIMEOUT(luaGlobal(session, "dragonSeen"), qsl("yes"), 10000);
        QCOMPARE(luaGlobal(session, "vitalsHp"), qsl("42"));
        QVERIFY(written(writes).contains("kill dragon\r\n"));
        QVERIFY(session.submit(qsl("look")));
        QVERIFY(written(writes).contains("look\r\n"));
        QVERIFY(!session.host()->mTelnet.currentlySecure());

        session.transportClosed(qsl("The game closed the connection"));
        QTRY_COMPARE_WITH_TIMEOUT(disconnected.size(), 1, 10000);
        QCOMPARE(luaGlobal(session, "disconnectionEvent"), qsl("yes"));
        QCOMPARE(session.host()->mTelnet.getConnectionState(), QAbstractSocket::UnconnectedState);
        // The game's close is already the app's news
        QCOMPARE(closes.size(), 0);
        QStringList lines;
        for (const auto& arguments : output) {
            lines << arguments.at(1).toString();
        }
        QVERIFY2(lines.contains(qsl("Welcome to the game!")) && lines.contains(qsl("A dragon arrives.")), qPrintable(lines.join(QChar::LineFeed)));
        QVERIFY(session.shutdown(false));
    }

    void test_theProfileEndsAndRedialsTheConnection()
    {
        const QString name = qsl("Headless Transport Redial");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QSignalSpy dials(&session, &THeadlessSession::transportDialRequested);
        QSignalSpy writes(&session, &THeadlessSession::transportWriteRequested);
        QSignalSpy closes(&session, &THeadlessSession::transportCloseRequested);
        QSignalSpy connected(&session, &THeadlessSession::connected);
        QSignalSpy disconnected(&session, &THeadlessSession::disconnected);
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy local(&session, &THeadlessSession::localText);
        QCOMPARE(session.start({.profile = name, .connect = true, .supplyTransport = true}), THeadlessSession::StartResult::started);
        QTRY_COMPARE_WITH_TIMEOUT(dials.size(), 1, 10000);
        session.transportConnected();
        QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 1, 10000);

        // Reported before the close, taken up after it: about a connection the profile has let go
        session.transportReceived("too late\r\n");
        QVERIFY(session.runLua(qsl("disconnect()")));
        QCOMPARE(closes.size(), 1);
        QTRY_COMPARE_WITH_TIMEOUT(disconnected.size(), 1, 10000);
        QTest::qWait(100);
        for (const auto& arguments : output) {
            QVERIFY(arguments.at(1).toString() != qsl("too late"));
        }
        QVERIFY(session.submit(qsl("nobody hears this")));
        QVERIFY(!written(writes).contains("nobody hears this"));

        QVERIFY(session.runLua(qsl("reconnect()")));
        QTRY_COMPARE_WITH_TIMEOUT(dials.size(), 2, 10000);
        // Made for the second dial, taken up while the third is outstanding
        session.transportConnected();
        QVERIFY(session.runLua(qsl("reconnect()")));
        QCOMPARE(dials.size(), 3);
        QCOMPARE(closes.size(), 2);
        QTest::qWait(100);
        QCOMPARE(connected.size(), 1);
        session.transportClosed(qsl("Connection refused"));
        QTRY_VERIFY2_WITH_TIMEOUT(printed(local).contains(qsl("Unable to connect to game.example:4000 - Connection refused")), qPrintable(printed(local)), 10000);
        QCOMPARE(connected.size(), 1);
        QCOMPARE(session.host()->mTelnet.getConnectionState(), QAbstractSocket::UnconnectedState);
        QVERIFY(session.shutdown(false));
    }

    void test_aSecureProfileIsNeverCalledSecure()
    {
        const QString name = qsl("Headless Transport Secure");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QSignalSpy dials(&session, &THeadlessSession::transportDialRequested);
        QSignalSpy connected(&session, &THeadlessSession::connected);
        QSignalSpy closes(&session, &THeadlessSession::transportCloseRequested);
        QSignalSpy local(&session, &THeadlessSession::localText);
        QCOMPARE(session.start({.profile = name, .connect = true, .tls = true, .supplyTransport = true}), THeadlessSession::StartResult::started);
        QTRY_COMPARE_WITH_TIMEOUT(dials.size(), 1, 10000);
        QCOMPARE(dials.at(0).at(3).toBool(), true);
        session.transportConnected();
        QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 1, 10000);
        QVERIFY(!session.host()->mTelnet.currentlySecure());
        QVERIFY2(printed(local).contains(qsl("Mudlet cannot tell whether it is encrypted")), qPrintable(printed(local)));
        // Closing the profile ends the app's connection too
        QVERIFY(session.shutdown(false));
        QCOMPARE(closes.size(), 1);
    }
};

#include "HeadlessSessionTransportTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionTransportTest)
