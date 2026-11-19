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

// A game restarts the MTTS terminal type cycle by sending DONT then DO TTYPE,
// most often around a copyover, and the first SEND after that expects the
// client name again. https://tintin.mudhalla.net/protocols/mtts/
//
// The game's requests arrive by loopback; the answers are real socket writes,
// waited out with a DO TIMING_MARK, which Mudlet always answers.

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "RecordingTelnetServer.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TelnetTerminalTypeCycleTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    RecordingTelnetServer* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-Telnet-Terminal-Type-Cycle");

    void announce(const char command, const char option)
    {
        QByteArray data;
        data.append(TN_IAC).append(command).append(option);
        mpHost->mTelnet.loopbackTest(data);
    }

    void requestTerminalType()
    {
        QByteArray data;
        data.append(TN_IAC).append(TN_SB).append(OPT_TERMINAL_TYPE).append(TNSB_SEND).append(TN_IAC).append(TN_SE);
        mpHost->mTelnet.loopbackTest(data);
    }

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

    // The terminal type in each TTYPE IS sent since the last capture, in order
    QList<QByteArray> terminalTypesSent()
    {
        QList<QByteArray> types;
        for (const Subnegotiation& reply : subnegotiationsFor(sentBytes(), static_cast<unsigned char>(OPT_TERMINAL_TYPE))) {
            if (!reply.payload.isEmpty() && reply.payload.front() == TNSB_IS) {
                types.append(reply.payload.mid(1));
            }
        }
        return types;
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
        if (mpHost->mTelnet.getConnectionState() != QAbstractSocket::ConnectedState) {
            QVERIFY2(connected.wait(15s), "The test profile never connected to the recording server.");
        }
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

    void test_dontTerminalTypeRestartsTheCycle()
    {
        QVERIFY2(mpHost->mEnableMTTS, "MTTS is off in this profile, so there is no cycle to restart");
        mpHost->mVersionInTTYPE = false;

        announce(TN_DO, OPT_TERMINAL_TYPE);
        sentBytes();

        // The three requests KaVir's snippet sends: name, terminal type, MTTS bitvector
        requestTerminalType();
        requestTerminalType();
        requestTerminalType();
        const QList<QByteArray> firstCycle = terminalTypesSent();
        QCOMPARE(firstCycle.size(), 3);
        QCOMPARE(firstCycle.at(0), QByteArray("MUDLET"));
        QVERIFY2(firstCycle.at(2).startsWith("MTTS "), firstCycle.at(2).constData());

        // A copyover: the game turns TTYPE off, then on again, and asks afresh
        announce(TN_DONT, OPT_TERMINAL_TYPE);
        announce(TN_DO, OPT_TERMINAL_TYPE);
        sentBytes();
        requestTerminalType();
        const QList<QByteArray> afterRestart = terminalTypesSent();
        QCOMPARE(afterRestart.size(), 1);
        QCOMPARE(afterRestart.first(), QByteArray("MUDLET"));
    }
};

MUDLET_GROUPED_TEST_MAIN(TelnetTerminalTypeCycleTest)

#include "TelnetTerminalTypeCycleTest.moc"
