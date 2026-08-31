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
 * slot_socketReadyToBeRead() reads at most BUFFER_SIZE (100 KB) per readyRead(),
 * and Qt only emits readyRead() when fresh bytes reach the socket. A server that
 * pushes more than that in one burst and then waits for input therefore left the
 * remainder sitting unread in the socket's own buffer, for as long as it took the
 * game to send something else - the display stopping part-way through the burst.
 *
 * Run with: ctest -R TelnetLargeBurstTest -V
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

class TelnetLargeBurstTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("LargeBurst-Host");
    const QString mLocalhost = qsl("localhost");

    static QByteArray burstOf(const int lineCount)
    {
        QByteArray payload;
        payload.reserve(lineCount * 64);
        for (int i = 0; i < lineCount; ++i) {
            payload += qsl("BURSTLINE %1 padded out so the burst clears one buffer read\r\n").arg(i, 6, 10, QLatin1Char('0')).toUtf8();
        }
        return payload;
    }

    bool bufferContains(const QString& text) const
    {
        TMainConsole* console = mpHost->mpConsole;
        // backwards: the line waited for is by construction the last of the burst,
        // and this runs on every poll of a multi-megabyte buffer
        for (int i = console->buffer.getLastLineNumber(); i >= 0; --i) {
            if (console->buffer.line(i).contains(text)) {
                return true;
            }
        }
        return false;
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(15000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The regression test. One burst several times BUFFER_SIZE, then silence:
    // every line of it has to reach the console off that one burst alone.
    void aBurstLargerThanOneReadIsConsumedWithoutFurtherTraffic()
    {
        // ~2.6 MB, 26 buffer reads' worth. This only strands on a host whose socket
        // receive buffer swallows the whole burst, so that one readyRead() covers it
        // all - Linux autotunes to tens of MB and does. Where the buffer is smaller
        // the sender dribbles, readyRead() keeps firing and even the unfixed code
        // finishes, so there the test passes without discriminating rather than
        // failing spuriously.
        constexpr int lineCount = 40000;
        mpHost->mpConsole->buffer.clear();

        const QByteArray payload = burstOf(lineCount);
        QVERIFY(payload.size() > 100000);
        mpServer->sendRaw(payload);

        const QString lastLine = qsl("BURSTLINE %1").arg(lineCount - 1, 6, 10, QLatin1Char('0'));
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 15000 && !bufferContains(lastLine)) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }

        QVERIFY2(bufferContains(lastLine),
                 qPrintable(qsl("the burst stopped part-way through: %1 of %2 lines arrived and no more were coming. "
                                "slot_socketReadyToBeRead() read one BUFFER_SIZE chunk and left the rest unread, and "
                                "readyRead() only fires on fresh bytes.")
                                    .arg(mpHost->mpConsole->buffer.getLastLineNumber())
                                    .arg(lineCount)));
    }

    // A latency reading is the wall time between a write and the next read. The
    // leftovers of a burst are not a reply to anything: they were already in the
    // socket before a trigger fired mid-burst and sent a command, so timing that
    // command against them reports a round trip of nearly nothing.
    void drainingABurstDoesNotPublishALatencyReading()
    {
        // Writes are only timed once the game has shown a Go-Ahead.
        mpServer->sendRaw(QByteArray("Welcome.\r\n\xff\xf9", 12));
        QElapsedTimer settle;
        settle.start();
        while (settle.elapsed() < 5000 && !mpHost->mTelnet.mGA_Driver) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        QVERIFY2(mpHost->mTelnet.mGA_Driver, "the Go-Ahead did not put the connection into GA-driven mode, so no write gets timed");

        // Settle any measurement the connection setup left running, so the burst
        // below starts with none outstanding: reading the socket ends whichever
        // one is in flight, and that has to happen here rather than mid-test.
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("send('sync')")));
        mpServer->sendRaw(QByteArray("SYNCMARK\r\n"));
        settle.restart();
        while (settle.elapsed() < 5000 && !bufferContains(qsl("SYNCMARK"))) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        QVERIFY2(bufferContains(qsl("SYNCMARK")), "the sync marker never arrived");

        // Two buffer reads' worth and no more: the stub flushes synchronously, so
        // a payload this size is wholly in the socket before the event loop next
        // runs, and the second read can only come from the queued drain. A burst
        // past what the socket buffers hold would dribble out over later flushes,
        // and those later arrivals are genuine readyRead()s that may legitimately
        // end a measurement - which would say nothing about the drain.
        constexpr int lineCount = 2400;
        mpHost->mpConsole->buffer.clear();
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("tempTrigger('BURSTLINE 000000', function() send('probe') end)")));

        // Nothing answers 'probe', so a reading that appears at all is a wrong one.
        constexpr double sentinel = 0.5;
        mpHost->mTelnet.networkLatencyTime = sentinel;

        mpServer->sendRaw(burstOf(lineCount));
        const QString lastLine = qsl("BURSTLINE %1").arg(lineCount - 1, 6, 10, QLatin1Char('0'));
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 15000 && !bufferContains(lastLine)) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        QVERIFY2(bufferContains(lastLine), "the burst stopped part-way, so the drain under test never ran");

        QVERIFY2(qFuzzyCompare(mpHost->mTelnet.networkLatencyTime, sentinel),
                 qPrintable(qsl("the drain of the burst's remainder was taken for the reply to the command the trigger sent: "
                                "latency went from %1s to %2s off bytes that were buffered before that command was written.")
                                    .arg(sentinel)
                                    .arg(mpHost->mTelnet.networkLatencyTime)));
    }
};

#include "TelnetLargeBurstTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetLargeBurstTest)
