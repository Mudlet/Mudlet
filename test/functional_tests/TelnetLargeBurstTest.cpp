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
 * The stranding only happens where the host's socket receive buffer swallows the
 * whole burst, so the test only discriminates there - on Linux, which autotunes to
 * tens of MB. Elsewhere it passes without discriminating; see the note on the test
 * itself.
 *
 * Run with: ctest -R TelnetLargeBurstTest -V
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>
#include <memory>

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

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

    static QByteArray burstOf(const int lineCount, const int firstLine = 0)
    {
        QByteArray payload;
        payload.reserve(lineCount * 64);
        for (int i = firstLine; i < firstLine + lineCount; ++i) {
            payload += qsl("BURSTLINE %1 padded out so the burst clears one buffer read\r\n").arg(i, 6, 10, QLatin1Char('0')).toUtf8();
        }
        return payload;
    }

    // Only the tail is ever worth looking at: the line waited for is by
    // construction the last of the burst. Scanning the whole buffer instead
    // costs more the further the burst has got, so a run that falls behind
    // spends its wait scanning rather than letting the ingest catch up.
    bool tailContains(const QString& text, const int depth = 3) const
    {
        TMainConsole* console = mpHost->mpConsole;
        const int lastLine = console->buffer.getLastLineNumber();
        for (int i = lastLine; i >= std::max(0, lastLine - depth); --i) {
            if (console->buffer.line(i).contains(text)) {
                return true;
            }
        }
        return false;
    }

    // The first burst line that is missing, repeated or out of order, or -1
    // once all lineCount of them are in the buffer once each and in order
    int firstBurstLineOutOfPlace(const int lineCount) const
    {
        TBuffer& buffer = mpHost->mpConsole->buffer;
        const QString marker = qsl("BURSTLINE ");
        int expected = 0;
        for (int i = 0, last = buffer.getLastLineNumber(); i <= last; ++i) {
            const QString& line = buffer.line(i);
            if (!line.startsWith(marker)) {
                continue;
            }
            if (line.mid(marker.size(), 6).toInt() != expected) {
                return expected;
            }
            ++expected;
        }
        return expected == lineCount ? -1 : expected;
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

        QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(15s)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The regression test. One burst several times BUFFER_SIZE, then silence:
    // every line of it has to reach the console off that one burst alone.
    void aBurstLargerThanOneReadIsConsumedWithoutFurtherTraffic()
    {
        // 780 KB, near 8 buffer reads' worth. Measured against a build with the fix
        // reverted, this is the smallest burst that strands every time - 520 KB
        // strands too, 260 KB already completes unfixed - and it leaves the wait
        // below with several times the margin it needs. This only strands on a host
        // whose socket receive buffer swallows the whole burst, so that one
        // readyRead() covers it all - Linux autotunes to tens of MB and does. Where
        // the buffer is smaller the sender dribbles, readyRead() keeps firing and
        // even the unfixed code finishes, so there the test passes without
        // discriminating rather than failing spuriously.
        constexpr int lineCount = 12000;
        mpHost->mpConsole->buffer.clear();

        const QByteArray payload = burstOf(lineCount);
        QVERIFY(payload.size() > 100000);
        mpServer->sendRaw(payload);

        const QString lastLine = qsl("BURSTLINE %1").arg(lineCount - 1, 6, 10, QLatin1Char('0'));
        QElapsedTimer timer;
        timer.start();
        while (timer.durationElapsed() < 15s && !tailContains(lastLine)) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }

        QVERIFY2(tailContains(lastLine),
                 qPrintable(qsl("the burst stopped part-way through: %1 of %2 lines arrived and no more were coming. "
                                "slot_socketReadyToBeRead() read one BUFFER_SIZE chunk and left the rest unread, and "
                                "readyRead() only fires on fresh bytes.")
                                    .arg(mpHost->mpConsole->buffer.getLastLineNumber())
                                    .arg(lineCount)));
    }

    // One compressed read that inflates to more than the drain goes deep in one
    // go: every line of it has to arrive, once each and in order
    void aCompressedReadInflatingPastTheDrainDepthIsShownInFull()
    {
        // 1.3 MB of text from well under one read of compressed data, where
        // the drain stops at 8 output buffers of 100 KB each
        constexpr int lineCount = 20000;
        mpHost->mpConsole->buffer.clear();

        const QByteArray text = burstOf(lineCount) + "MCCP BURST END\r\n";
        // qCompress() prefixes the zlib stream with the source length
        const QByteArray compressed = qCompress(text, 9).mid(4);
        QVERIFY(compressed.size() < 100000);
        mpServer->sendRaw(QByteArray("\xFF\xFB\x56\xFF\xFA\x56\xFF\xF0", 8) + compressed);

        QElapsedTimer timer;
        timer.start();
        while (timer.durationElapsed() < 15s && !tailContains(qsl("MCCP BURST END"))) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        QVERIFY2(tailContains(qsl("MCCP BURST END")),
                 qPrintable(qsl("the compressed burst stopped part-way through, at buffer line %1 of %2.").arg(mpHost->mpConsole->buffer.getLastLineNumber()).arg(lineCount)));
        const int outOfPlace = firstBurstLineOutOfPlace(lineCount);
        QVERIFY2(outOfPlace < 0, qPrintable(qsl("burst line %1 is missing, repeated or out of order.").arg(outOfPlace)));
    }

    // Runs last, as it ends the connection the other cases share. A game that
    // sends its last words and closes straight away: most of them are still
    // waiting to be read when the close is noticed - see issue #10655
    void aBurstFollowedByACloseIsShownInFull()
    {
        // Compressed first, so that the close can also find input inflated
        // past the drain's depth still waiting ahead of what is unread on the
        // socket; the start sequence opens the send so no read splits it.
        constexpr int compressedLineCount = 20000;
        constexpr int plainLineCount = 12000;
        constexpr int lineCount = compressedLineCount + plainLineCount;
        mpHost->mpConsole->buffer.clear();
        QSignalSpy disconnected(&(mpHost->mTelnet), &cTelnet::signal_disconnected);

        // qCompress() prefixes the zlib stream with the source length
        const QByteArray compressed = qCompress(burstOf(compressedLineCount), 9).mid(4);
        mpServer->sendRaw(QByteArray("\xFF\xFB\x56\xFF\xFA\x56\xFF\xF0", 8) + compressed + burstOf(plainLineCount, compressedLineCount) + "GOODBYE FROM THE GAME\r\n");
        mpServer->closeClient();
        QVERIFY2(disconnected.wait(15s), "the game closing the connection was never noticed");
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        // the disconnect's own messages follow the game's text
        QVERIFY2(tailContains(qsl("GOODBYE FROM THE GAME"), 20),
                 qPrintable(qsl("the text sent before the close was cut short, at buffer line %1 of %2.").arg(mpHost->mpConsole->buffer.getLastLineNumber()).arg(lineCount)));
        const int outOfPlace = firstBurstLineOutOfPlace(lineCount);
        QVERIFY2(outOfPlace < 0, qPrintable(qsl("burst line %1 is missing, repeated or out of order.").arg(outOfPlace)));
    }
};

#include "TelnetLargeBurstTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetLargeBurstTest)
