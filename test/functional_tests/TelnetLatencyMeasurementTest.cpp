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
 * Tests what getNetworkLatency() - the 'N:' figure in the status bar - is
 * allowed to report - https://github.com/Mudlet/Mudlet/issues/10106
 *
 * The reading is the wall time between writing a command and reading the
 * game's reply, so it only measures the network for as long as Mudlet is there
 * to notice the reply arriving. A client that freezes right across that
 * arrival reads the reply late and used to publish its own freeze as the ping:
 * on a link whose real round trip was 0.14s, three seconds of blocked Lua came
 * back as 2.999s. Two more faults sat in the same code: a reply that carried no
 * GA could stop every later command being measured, and a command the game
 * never answered left whatever it sent next timed as that command's reply.
 *
 * Both need a controlled socket round trip and a blocked event loop, which a
 * Lua spec cannot arrange, so this runs over a real TCP socket via
 * TelnetServerStub with the game's replies and Mudlet's stalls placed by hand.
 *
 * Run with: ctest -R TelnetLatencyMeasurementTest -V
 */

#include <QEventLoop>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QtTest/QtTest>
#include <chrono>
#include <functional>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

namespace {
const QByteArray csGoAhead = QByteArrayLiteral("\xff\xf9"); // IAC GA
const QByteArray csReply = QByteArrayLiteral("You are in a well lit room.\r\n");
// Attempts allowed at a round trip the runner stalled the reading away from - see takeReading()
constexpr int csReadingAttempts = 3;
} // namespace

class TelnetLatencyMeasurementTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Network-Latency-Measurement-Test-Host");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    double latency() const { return mpHost->mTelnet.networkLatencyTime; }

    // Runs the event loop for real, so the dispatcher parks in its poll and
    // delivers timers. QTest::qWait() would do neither cleanly: it alternates
    // processEvents() with a 10ms sleep, so the harness would be injecting
    // unresponsiveness of its own into the very thing being measured.
    static void runEventLoop(const std::chrono::milliseconds duration)
    {
        QEventLoop loop;
        QTimer::singleShot(duration, &loop, &QEventLoop::quit);
        loop.exec();
    }

    // A client stall: the thread is held without the event loop running, so
    // nothing is read from the socket and no timer is delivered - the same
    // shape as a script that blocks, without burning a runner's CPU to do it.
    static void freezeMudlet(const std::chrono::milliseconds duration) { QThread::sleep(duration); }

    void sendCommand()
    {
        QString command = qsl("look");
        mpHost->mTelnet.sendData(command, false, true);
    }

    void replyFromServer() { mpServer->sendRaw(csReply + csGoAhead); }

    // Sends a command whose reply comes back after 'flightTime' with the event
    // loop running throughout, which is the measurement working normally.
    void measureQuietRoundTrip(const std::chrono::milliseconds flightTime)
    {
        sendCommand();
        QTimer::singleShot(flightTime, this, [this]() {
            replyFromServer();
        });
        runEventLoop(flightTime + 400ms);
    }

    // Takes 'roundTrip' until it publishes a reading. One is only published when
    // Mudlet kept up with its event loop for the whole wait - see
    // NETWORK_LATENCY_MAX_STALL in ctelnet.cpp - so a runner that stalls anywhere
    // inside the window drops it and leaves the previous reading standing. That is
    // the measurement working as designed, but it leaves nothing to judge the round
    // trip by, so it is worth taking again rather than failing on.
    //
    // Only a missing reading is retried. One that arrives with the wrong value
    // is a real result and is left for the caller to assert on, since retrying
    // those would let a measurement that answers for the wrong command pass on
    // whichever attempt happened to look right.
    bool takeReading(const std::function<void()>& roundTrip)
    {
        for (int attempt = 0; attempt < csReadingAttempts; ++attempt) {
            mpHost->mTelnet.networkLatencyTime = 0.0;
            roundTrip();
            if (latency() > 0.0) {
                return true;
            }
        }
        return false;
    }

    // A quiet round trip, taken again if the runner stalled across it.
    bool takeQuietReading(const std::chrono::milliseconds flightTime)
    {
        return takeReading([this, flightTime]() {
            measureQuietRoundTrip(flightTime);
        });
    }

    // The same, for the round trip whose reply carries no GA.
    bool takeReadingWithoutGoAhead(const std::chrono::milliseconds flightTime)
    {
        return takeReading([this, flightTime]() {
            sendCommand();
            QTimer::singleShot(flightTime, this, [this]() {
                mpServer->sendRaw(QByteArrayLiteral("Nothing happens.\r\n"));
            });
            runEventLoop(flightTime + 400ms);
        });
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so a second copy of this test
        // running at the same time is not told the profile name it types is
        // already in use - see TelnetStringSequenceRecoveryTest.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        QVERIFY2(mpServer->isListening(), "TelnetServerStub failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connectedSpy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }

        // Latency is only measured for a game that drives GA, so the stub has
        // to have sent one before any of this means anything.
        mpServer->sendRaw(QByteArrayLiteral("Welcome.\r\n") + csGoAhead);
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mpHost->mTelnet.mGA_Driver;
                         },
                         5000),
                 "The stub's GA never reached cTelnet, so no latency would ever be measured");
    }

    // The reading persists across methods, and so does anything the stub still
    // has in flight - either would let a method pass on the method before it
    // rather than on what it does itself.
    void init()
    {
        runEventLoop(300ms);
        mpHost->mTelnet.networkLatencyTime = 0.0;
    }

    // A round trip Mudlet was awake for is reported. Without this the fix could
    // pass the rest of the file by never publishing a reading at all.
    void reportsRoundTripMeasuredWhileResponsive()
    {
        QVERIFY2(takeQuietReading(300ms), "A round trip taken with the event loop running published no reading at all");
        QVERIFY2(latency() > 0.2 && latency() < 0.6, qPrintable(qsl("A 0.3s round trip taken with the event loop running should be reported as roughly that, got %1s").arg(latency())));
    }

    // The reported bug: with the reply already sitting in the socket, the time
    // Mudlet then spends frozen before reading it is the client's, not the
    // network's, and must not be published as the ping.
    void dropsRoundTripMeasuredAcrossAStall()
    {
        QVERIFY2(takeQuietReading(300ms), "Setup failed: no reading to start from was published at all");
        const double beforeStall = latency();
        QVERIFY2(beforeStall > 0.2 && beforeStall < 0.6, qPrintable(qsl("Setup failed: expected a ~0.3s reading to start from, got %1s").arg(beforeStall)));

        sendCommand();
        runEventLoop(150ms);  // the command reaches the wire...
        replyFromServer();    // ...the game answers, so the reply waits in the socket...
        freezeMudlet(1200ms); // ...and Mudlet is frozen right across its arrival
        runEventLoop(400ms);  // only now is it read

        QVERIFY2(latency() < 1.0, qPrintable(qsl("The 1.2s freeze was reported as network latency: %1s").arg(latency())));
        QCOMPARE(latency(), beforeStall);

        // ...and dropping that reading did not put the measurement out of
        // action: the next quiet round trip is measured, and measured afresh.
        QVERIFY2(takeQuietReading(600ms), "The stall left the measurement out of action: no later round trip was reported at all");
        QVERIFY2(latency() > 0.5 && latency() < 0.9, qPrintable(qsl("A 0.6s round trip after the stall should have been reported as roughly that, got %1s").arg(latency())));
    }

    // A command the game never answers must not leave whatever it sends next -
    // however long afterwards - being timed as that command's reply.
    void abandonsAMeasurementTheGameNeverAnswers()
    {
        QVERIFY2(takeQuietReading(300ms), "Setup failed: no reading to start from was published at all");
        const double beforeSilence = latency();
        QVERIFY2(beforeSilence > 0.2 && beforeSilence < 0.6, qPrintable(qsl("Setup failed: expected a ~0.3s reading to start from, got %1s").arg(beforeSilence)));

        sendCommand();
        runEventLoop(11s); // past NETWORK_LATENCY_TIMEOUT, so the measurement is abandoned
        replyFromServer(); // so this is a line of its own, not that command's reply
        runEventLoop(400ms);

        QCOMPARE(latency(), beforeSilence);

        // ...and it was abandoned rather than merely left unpublished: an
        // abandoned measurement lets the next command start one of its own,
        // while one still outstanding would swallow this round trip too
        QVERIFY2(takeQuietReading(700ms), "The command sent after the unanswered one was not measured at all");
        QVERIFY2(latency() > 0.5 && latency() < 1.0, qPrintable(qsl("The command sent after the unanswered one was not measured on its own, got %1s").arg(latency())));
    }

    // A reply that carries no GA must not stop the commands after it being
    // measured, leaving one reading to stand in silently for all of them.
    void measuresCommandsAfterAReplyWithoutGoAhead()
    {
        QVERIFY2(takeReadingWithoutGoAhead(200ms), "Setup failed: a reply that carried no GA published no reading at all");
        const double afterUnprompted = latency();
        QVERIFY2(afterUnprompted > 0.1 && afterUnprompted < 0.5, qPrintable(qsl("Setup failed: expected the 0.2s round trip to be the standing reading, got %1s").arg(afterUnprompted)));

        QVERIFY2(takeQuietReading(700ms), "A round trip after a reply that carried no GA was not measured at all");
        QVERIFY2(latency() > 0.5 && latency() < 1.0, qPrintable(qsl("A 0.7s round trip was not measured on its own after a reply that carried no GA, got %1s").arg(latency())));
    }

    // A measurement outstanding when the connection goes away belongs to that
    // connection: the next one's first packet is not its reply.
    void abandonsAMeasurementWhenTheConnectionGoesAway()
    {
        QVERIFY2(takeQuietReading(300ms), "Setup failed: no reading to start from was published at all");
        const double beforeReconnect = latency();
        QVERIFY2(beforeReconnect > 0.2 && beforeReconnect < 0.6, qPrintable(qsl("Setup failed: expected a ~0.3s reading to start from, got %1s").arg(beforeReconnect)));

        sendCommand();
        runEventLoop(150ms);
        mpHost->mTelnet.disconnectIt();
        QVERIFY(QTest::qWaitFor(
                [this]() {
                    return mpHost->mTelnet.getConnectionState() == QAbstractSocket::UnconnectedState;
                },
                5000));

        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        mpHost->mTelnet.connectIt(mLocalhost, mPort.toInt());
        QVERIFY2(connectedSpy.wait(5000), "The profile did not reconnect to the stub");
        // Long enough that finishing the old connection's measurement here would
        // read as a round trip several times the one before it
        runEventLoop(800ms);
        replyFromServer();
        runEventLoop(400ms);

        QCOMPARE(latency(), beforeReconnect);
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
            QDir(path).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }
};

#include "TelnetLatencyMeasurementTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetLatencyMeasurementTest)
