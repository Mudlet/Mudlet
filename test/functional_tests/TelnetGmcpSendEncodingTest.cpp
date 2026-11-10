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

// GMCP is UTF-8 JSON whatever encoding the game's text arrives in, and incoming
// GMCP is always read that way, so what sendGMCP() puts on the socket has to be
// UTF-8 too: a profile set to ISO 8859-1 must not hand the game Latin-1 bytes
// inside its JSON, or a substitute for what Latin-1 cannot hold.
//
// The message is a real socket write, waited out by asking for a DO TIMING_MARK
// afterwards, which Mudlet always answers - so the capture is complete when the
// answer is in.

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "RecordingTelnetServer.h"
#include "TLuaInterpreter.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TelnetGmcpSendEncodingTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    RecordingTelnetServer* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-Telnet-GMCP-Send-Encoding");

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    void announce(const char command, const char option)
    {
        QByteArray data;
        data.append(TN_IAC).append(command).append(option);
        mpHost->mTelnet.loopbackTest(data);
    }

    // Mudlet answers a DO TIMING_MARK with WONT TIMING_MARK whatever else is going
    // on (RFC 860), so that answer arriving means everything sent before it has
    // been recorded.
    QByteArray sentBytes()
    {
        announce(TN_DO, OPT_TIMING_MARK);
        QByteArray marker;
        marker.append(TN_IAC).append(TN_WONT).append(OPT_TIMING_MARK);
        const bool arrived = QTest::qWaitFor(
                [this, &marker]() {
                    return mpServer->received().contains(marker);
                },
                10s);
        if (!arrived) {
            QTest::qFail("the telnet marker never came back, so the capture is worthless", __FILE__, __LINE__);
            return {};
        }
        const QByteArray stream = mpServer->received();
        mpServer->forgetReceived();
        const int markerAt = stream.indexOf(marker);
        return markerAt >= 0 ? stream.left(markerAt) : stream;
    }

    bool luaSucceeds(const QString& call) { return mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(%1)").arg(call)); }

    QByteArray sentGmcpPayload(const QString& call)
    {
        if (!luaSucceeds(call)) {
            QTest::qFail("sendGMCP() refused a message on a game that negotiated GMCP", __FILE__, __LINE__);
            return {};
        }
        const QByteArray wire = sentBytes();
        const QList<Subnegotiation> messages = subnegotiationsFor(wire, static_cast<unsigned char>(OPT_GMCP));
        if (messages.size() != 1) {
            QTest::qFail(qPrintable(qsl("expected one GMCP subnegotiation, wire was: %1").arg(QString::fromLatin1(wire.toHex(' ')))), __FILE__, __LINE__);
            return {};
        }
        return messages.first().payload;
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

        mpServer = new RecordingTelnetServer(qApp);
        QVERIFY2(mpServer->start(), "RecordingTelnetServer failed to bind a loopback port");

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, qsl("localhost"), QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");

        QSignalSpy connected(&mpHost->mTelnet, &cTelnet::signal_connected);
        if (connected.isEmpty()) {
            QVERIFY2(connected.wait(15s), "The test profile never connected to the recording server.");
        }

        announce(TN_WILL, OPT_GMCP);
        QVERIFY2(mpHost->mTelnet.isGMCPEnabled(), "the game's WILL GMCP was not taken up, so nothing below could be sent");
        // Core.Hello and Core.Supports.Set, which are not under test
        sentBytes();
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void cleanup()
    {
        if (mpHost) {
            mpHost->mTelnet.setEncoding(QByteArrayLiteral("UTF-8"), false);
        }
    }

    void test_messageStaysUtf8OnAnIso8859_1Game()
    {
        QVERIFY(mpHost->mTelnet.setEncoding(QByteArrayLiteral("ISO 8859-1"), false).first);
        sentBytes();

        const QByteArray payload = sentGmcpPayload(qsl(R"(sendGMCP('Comm.Say {"text": "café ☕"}'))"));
        QCOMPARE(payload, QByteArray(R"(Comm.Say {"text": "café ☕"})"));
    }

    void test_dataArgumentStaysUtf8OnAGbkGame()
    {
        QVERIFY(mpHost->mTelnet.setEncoding(QByteArrayLiteral("GBK"), false).first);
        sentBytes();

        const QByteArray payload = sentGmcpPayload(qsl(R"(sendGMCP("Comm.Say", '{"text": "你好"}'))"));
        QCOMPARE(payload, QByteArray(R"(Comm.Say {"text": "你好"})"));
    }

    // An ASCII profile never transcoded, so a script's raw bytes still go out as they are
    void test_asciiProfilePassesRawBytesWithIacDoubled()
    {
        QVERIFY(mpHost->mTelnet.setEncoding(QByteArrayLiteral("ASCII"), false).first);
        sentBytes();

        QVERIFY(luaSucceeds(qsl(R"(sendGMCP("Test.Raw \233\255", "\255"))")));
        const QByteArray wire = sentBytes();
        QVERIFY2(wire.contains(QByteArray("Test.Raw \xE9\xFF\xFF \xFF\xFF\xFF\xF0")), qPrintable(QString::fromLatin1(wire.toHex(' '))));
    }

    // A packager's MUDLET_VERSION_BUILD can put any text in the build suffix
    void test_coreHelloVersionIsUtf8OnALatin1Game()
    {
        QVERIFY(mpHost->mTelnet.setEncoding(QByteArrayLiteral("ISO 8859-1"), false).first);

        const std::string hello = mpHost->mTelnet.coreHelloMessage(qsl("-tëst€"));
        QCOMPARE(QByteArray::fromStdString(hello), QByteArray(R"(Core.Hello { "client": "Mudlet", "version": ")") + APP_VERSION + QByteArray("-tëst€") + QByteArray(R"("})"));
    }

    void test_invalidUtf8StillBecomesAReplacementCharacter()
    {
        const QByteArray payload = sentGmcpPayload(qsl(R"(sendGMCP("Test.Raw \255", "\255"))"));
        QCOMPARE(payload, QByteArray("Test.Raw \xEF\xBF\xBD \xEF\xBF\xBD"));
    }
};

MUDLET_GROUPED_TEST_MAIN(TelnetGmcpSendEncodingTest)

#include "TelnetGmcpSendEncodingTest.moc"
