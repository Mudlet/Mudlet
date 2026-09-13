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

// Channel 102 carries two raw bytes to the game inside a subnegotiation, and only
// the payload may have its 0xFF doubled: a doubled framing byte turns the whole
// message into data the game never parses. The framing helper has unit tests of
// its own; this test reads what sendTelnetChannel102() actually puts on the
// socket, because the bug it pins lived between the helper and the Lua function.
//
// The game's offer arrives by loopback, the message is a real socket write,
// waited out by asking for a DO TIMING_MARK afterwards, which Mudlet always
// answers - so the capture is complete when the answer is in.

#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "RecordingTelnetServer.h"
#include "TLuaInterpreter.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TelnetChannel102WireTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    RecordingTelnetServer* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-Telnet-Channel-102");

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
                10000);
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

    void enableChannel102()
    {
        announce(TN_WILL, OPT_102);
        QVERIFY2(mpHost->mTelnet.isChannel102Enabled(), "the game's WILL 102 was not taken up, so nothing below could be sent");
        mpServer->forgetReceived();
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
        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, qsl("localhost"), QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");

        QSignalSpy connected(&mpHost->mTelnet, &cTelnet::signal_connected);
        if (connected.isEmpty()) {
            QVERIFY2(connected.wait(15s), "The test profile never connected to the recording server.");
        }
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_payloadIacIsDoubledAndTheFramingIsNot()
    {
        enableChannel102();
        QVERIFY2(luaSucceeds(qsl(R"(sendTelnetChannel102("\255b"))")), "sendTelnetChannel102() refused a two-byte message on an enabled channel");

        const QByteArray wire = sentBytes();
        const QList<Subnegotiation> messages = subnegotiationsFor(wire, static_cast<unsigned char>(OPT_102));
        QVERIFY2(messages.size() == 1, qPrintable(qsl("expected one channel 102 subnegotiation, wire was: %1").arg(QString::fromLatin1(wire.toHex(' ')))));
        QCOMPARE(messages.first().payload,
                 QByteArray("\xff"
                            "b",
                            2));
        // Spelled out so the test pins the wire: IAC SB 102 IAC IAC 'b' IAC SE
        QVERIFY2(wire.contains(QByteArray("\xff\xfa\x66\xff\xff\x62\xff\xf0", 8)), qPrintable(QString::fromLatin1(wire.toHex(' '))));
        QVERIFY2(!wire.contains(QByteArray("\xff\xff\xfa", 3)), "the opening IAC SB was doubled, which turns the message into data the game skips");
    }

    void test_plainPayloadIsFramedOnce()
    {
        enableChannel102();
        QVERIFY2(luaSucceeds(qsl(R"(sendTelnetChannel102("ab"))")), "sendTelnetChannel102() refused a two-byte message on an enabled channel");

        const QByteArray wire = sentBytes();
        const QList<Subnegotiation> messages = subnegotiationsFor(wire, static_cast<unsigned char>(OPT_102));
        QVERIFY2(messages.size() == 1, qPrintable(qsl("expected one channel 102 subnegotiation, wire was: %1").arg(QString::fromLatin1(wire.toHex(' ')))));
        QCOMPARE(messages.first().payload, QByteArray("ab"));
        QVERIFY2(wire.contains(QByteArray("\xff\xfa\x66\x61\x62\xff\xf0", 7)), qPrintable(QString::fromLatin1(wire.toHex(' '))));
    }
};

MUDLET_GROUPED_TEST_MAIN(TelnetChannel102WireTest)

#include "TelnetChannel102WireTest.moc"
