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
 * tens of MB, and on Windows, where Mudlet asks for 1 MB. Elsewhere it passes
 * without discriminating; see the note on the test itself.
 *
 * Run with: ctest -R TelnetLargeBurstTest -V
 */

#include <QAbstractEventDispatcher>
#include <QDataStream>
#include <QRandomGenerator>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>
#include <memory>

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
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

    // Hardly compresses, so a compressed stream is still arriving while the first of it is shown
    static QByteArray hardToCompressBurstOf(const int lineCount)
    {
        QRandomGenerator generator(1);
        QByteArray payload;
        payload.reserve(lineCount * 64);
        for (int i = 0; i < lineCount; ++i) {
            payload += qsl("BURSTLINE %1 ").arg(i, 6, 10, QLatin1Char('0')).toUtf8();
            for (int word = 0; word < 3; ++word) {
                payload += QByteArray::number(generator.generate64(), 36);
            }
            payload += "\r\n";
        }
        return payload;
    }

    // Only the tail is ever worth looking at: the line waited for is by
    // construction the last of the burst, or within depth lines of the end once
    // the disconnect messages have followed it. Scanning the whole buffer instead
    // costs more the further the burst has got, so a run that falls behind
    // spends its wait scanning rather than letting the ingest catch up.
    bool tailContains(const QString& text, const int depth = 3) const
    {
        TMainConsole* console = mpHost->mainConsoleView();
        const int lastLine = console->buffer.getLastLineNumber();
        for (int i = lastLine; i >= std::max(0, lastLine - depth); --i) {
            if (console->buffer.line(i).contains(text)) {
                return true;
            }
        }
        return false;
    }

    // NULs are not displayed, and a run of them compresses to almost nothing, so
    // the first KB or so of this inflates past the depth one drain goes to: the
    // drain defers whatever size of reads the stream arrives in. 100000 is
    // BUFFER_SIZE in ctelnet.cpp.
    static QByteArray compressedBurst(const QByteArray& text)
    {
        const QByteArray leadIn((cTelnet::scmMaxDecompressionRecursion + 1) * 100000, '\0');
        // qCompress() prefixes the zlib stream with the source length
        return qCompress(leadIn + "\r\n" + text, 9).mid(4);
    }

    // The WILL in a read of its own, so Mudlet's answer to it is on the wire
    // before anything that follows
    void negotiateMccp()
    {
        mpServer->sendRaw(QByteArray("\xFF\xFB\x56", 3));
        QElapsedTimer timer;
        timer.start();
        while (timer.durationElapsed() < 500ms) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
    }

    bool bufferContains(const QString& text) const
    {
        TBuffer& buffer = mpHost->mainConsoleView()->buffer;
        for (int i = 0, last = buffer.getLastLineNumber(); i <= last; ++i) {
            if (buffer.line(i).contains(text)) {
                return true;
            }
        }
        return false;
    }

    // The number of the first burst line not where it should be (missing,
    // repeated or out of order), or -1 once all lineCount of them are in the
    // buffer once each and in order
    int firstBurstLineOutOfPlace(const int lineCount) const
    {
        TBuffer& buffer = mpHost->mainConsoleView()->buffer;
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

    // The number of the last buffer line containing text, or -1
    int lastLineContaining(const QString& text) const
    {
        TBuffer& buffer = mpHost->mainConsoleView()->buffer;
        for (int i = buffer.getLastLineNumber(); i >= 0; --i) {
            if (buffer.line(i).contains(text)) {
                return i;
            }
        }
        return -1;
    }

    // For the tests that start after one has ended the connection
    bool connectAgain()
    {
        if (mpHost->mTelnet.getConnectionState() == QAbstractSocket::ConnectedState) {
            return true;
        }
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QSignalSpy accepted(mpServer, &QTcpServer::newConnection);
        mpHost->mTelnet.connectIt(mLocalhost, mpServer->serverPort());
        // Mudlet's end can be connected before the stub has accepted it, and
        // the stub drops what it is given to send between sessions
        return connected.wait(15s) && (accepted.count() || accepted.wait(15s));
    }

    // Agreed ahead of the burst, as a game does at login: an answer reaching
    // the socket once the game has closed it is met with a reset, which
    // throws away whatever of the burst the client has yet to receive. The
    // LINEMODE refusal answers the request sent last, so once it is in, so
    // is everything the client had still to send.
    bool agreeProtocolsAheadOfTheLastWords()
    {
        const QByteArray lineModeRefused("\xFF\xFC\x22", 3);
        const qsizetype refusalsBefore = mpServer->receivedSoFar().count(lineModeRefused);
        mpServer->sendRaw(QByteArray("\xFF\xFB\x56\xFF\xFD\x22", 6));
        QElapsedTimer timer;
        timer.start();
        while (mpServer->receivedSoFar().count(lineModeRefused) == refusalsBefore) {
            if (timer.durationElapsed() > 15s) {
                return false;
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        return true;
    }

    // The game's last words: a compressed burst, as much again uncompressed and
    // a goodbye, with the connection closed as soon as they are sent. Compressed
    // first, so that the close can also find input inflated past the drain's
    // depth still waiting ahead of what is unread on the socket; the start
    // sequence opens the send so no read splits it.
    bool sendLastWordsAndClose()
    {
        if (!agreeProtocolsAheadOfTheLastWords()) {
            return false;
        }
        // qCompress() prefixes the zlib stream with the source length
        const QByteArray compressed = qCompress(burstOf(scmCompressedLastWords), 9).mid(4);
        mpServer->sendRaw(QByteArray("\xFF\xFA\x56\xFF\xF0", 5) + compressed + burstOf(scmPlainLastWords, scmCompressedLastWords) + "GOODBYE FROM THE GAME\r\n");
        mpServer->closeClient();
        return true;
    }

    // Plain text only: one read of it is a known number of lines, where a read
    // of a compressed stream can inflate to eight reads' worth
    bool sendPlainLastWordsAndClose()
    {
        if (!agreeProtocolsAheadOfTheLastWords()) {
            return false;
        }
        mpServer->sendRaw(burstOf(scmLastWordsLineCount) + "GOODBYE FROM THE GAME\r\n");
        mpServer->closeClient();
        return true;
    }
    static constexpr int scmCompressedLastWords = 20000;
    static constexpr int scmPlainLastWords = 12000;
    static constexpr int scmLastWordsLineCount = scmCompressedLastWords + scmPlainLastWords;

    inline static int smLinesAfterClose = 0;
    inline static int smLinesWhenPostedEventHandled = -1;
    static int countLineAfterClose(lua_State*)
    {
        if (++smLinesAfterClose == 1) {
            QMetaObject::invokeMethod(
                    qApp,
                    []() {
                        smLinesWhenPostedEventHandled = smLinesAfterClose;
                    },
                    Qt::QueuedConnection);
        }
        return 0;
    }

    // Posts an event from a flood's lines, one at a time, so that about one is handled
    // per turn. It notes how many lines went by first and how much text Qt held by then.
    inline static int smFloodLines = 0;
    inline static int smLastFloodLine = -1;
    inline static int smFloodLineOutOfPlace = -1;
    inline static int smFloodEventsHandled = 0;
    inline static int smMostFloodLinesAheadOfAnEvent = 0;
    inline static qint64 smMostTextHeldByQt = 0;
    inline static bool smFloodEventWaiting = false;
    inline static cTelnet* smpFloodTelnet = nullptr;
    static int countFloodLine(lua_State* L)
    {
        const int number = static_cast<int>(lua_tointeger(L, 1));
        if (number != smFloodLines && smFloodLineOutOfPlace < 0) {
            smFloodLineOutOfPlace = number;
        }
        ++smFloodLines;
        smLastFloodLine = number;
        if (!smFloodEventWaiting) {
            smFloodEventWaiting = true;
            QMetaObject::invokeMethod(
                    qApp,
                    [postedAt = smFloodLines]() {
                        smFloodEventWaiting = false;
                        ++smFloodEventsHandled;
                        smMostFloodLinesAheadOfAnEvent = std::max(smMostFloodLinesAheadOfAnEvent, smFloodLines - postedAt);
                        smMostTextHeldByQt = std::max(smMostTextHeldByQt, smpFloodTelnet->mpSocket->bytesAvailable());
                    },
                    Qt::QueuedConnection);
        }
        return 0;
    }

    // 100000 is BUFFER_SIZE in ctelnet.cpp; the line a read ends part-way through is finished by the next
    static int linesInOneRead() { return static_cast<int>(100000 / burstOf(1).size()) + 1; }

    // About 20 MB from a game that stays connected, far more than it takes one
    // read per turn to take in before the sender is done
    static constexpr int scmFloodLineCount = 300000;
    bool sendFloodAndWait(const bool compressed = false)
    {
        mpHost->mainConsoleView()->buffer.clear();
        smFloodLines = 0;
        smLastFloodLine = -1;
        smFloodLineOutOfPlace = -1;
        smFloodEventsHandled = 0;
        smMostFloodLinesAheadOfAnEvent = 0;
        smMostTextHeldByQt = 0;
        smFloodEventWaiting = false;
        smFloodLinesAtNestedLoopEnd = 0;
        smFloodEventsAtNestedLoopEnd = 0;
        smReadyReadsInNestedLoop = 0;
        smpFloodTelnet = &(mpHost->mTelnet);
        TLuaInterpreter* lua = mpHost->getLuaInterpreter();
        lua_register(lua->getLuaGlobalState(), "countFloodLine", countFloodLine);
        if (!lua->compileAndExecuteScript(qsl(R"(
            floodTrigger = tempRegexTrigger("^BURSTLINE (\\d+) ", function()
                countFloodLine(tonumber(matches[2]))
            end)
        )"))) {
            return false;
        }

        // More to a turn than one read takes, so the queued read never runs out of text
        mpServer->setSendBufferSize(4 * 1024 * 1024);
        // qCompress() prefixes the zlib stream with the source length
        mpServer->sendRaw(compressed ? QByteArray("\xFF\xFA\x56\xFF\xF0", 5) + qCompress(hardToCompressBurstOf(scmFloodLineCount), 1).mid(4) : burstOf(scmFloodLineCount));
        // Each line resets this, so a slow build gets all the time it needs and a stream that broke fails soon
        QElapsedTimer sinceLastLine;
        sinceLastLine.start();
        int linesSeen = 0;
        while (smLastFloodLine < scmFloodLineCount - 1 && sinceLastLine.durationElapsed() < 20s) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            if (smFloodLines != linesSeen) {
                linesSeen = smFloodLines;
                sinceLastLine.start();
            }
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        return lua->compileAndExecuteScript(qsl("killTrigger(floodTrigger)"));
    }

    // Averaged, as a read that empties Qt's buffer can put two reads in a turn. A second queued read
    // in every turn is half a read past this. Counted from the last nested event loop, whose turns hold no read.
    static int floodLinesPastNestedLoops() { return smFloodLines - smFloodLinesAtNestedLoopEnd; }
    static int floodTurnsPastNestedLoops() { return smFloodEventsHandled - smFloodEventsAtNestedLoopEnd; }
    static bool floodTookOneReadPerTurn() { return floodLinesPastNestedLoops() <= 1.5 * linesInOneRead() * floodTurnsPastNestedLoops(); }
    static QString floodTurnsReport()
    {
        return qsl("%1 lines of a flood took %2 turns, when one read is at most %3 lines.").arg(floodLinesPastNestedLoops()).arg(floodTurnsPastNestedLoops()).arg(linesInOneRead());
    }

    inline static bool smInNestedLoop = false;
    inline static int smReadyReadsInNestedLoop = 0;
    inline static int smFloodLinesAtNestedLoopEnd = 0;
    inline static int smFloodEventsAtNestedLoopEnd = 0;
    // Holds the event loop for the milliseconds given, as a modal dialog opened by a
    // trigger does
    static int spinNestedEventLoop(lua_State* L)
    {
        QEventLoop loop;
        QTimer::singleShot(static_cast<int>(lua_tointeger(L, 1)), &loop, &QEventLoop::quit);
        smInNestedLoop = true;
        loop.exec();
        smInNestedLoop = false;
        smFloodLinesAtNestedLoopEnd = smFloodLines;
        smFloodEventsAtNestedLoopEnd = smFloodEventsHandled;
        return 0;
    }

    inline static TelnetServerStub* smpGame = nullptr;
    // Has the game say something from inside a trigger, ahead of its nested event loop
    static int gameSends(lua_State* L)
    {
        smpGame->sendRaw(QByteArray(lua_tostring(L, 1)));
        return 0;
    }

    void registerNestedLoopFunctions()
    {
        smpGame = mpServer;
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_register(L, "spinNestedEventLoop", spinNestedEventLoop);
        lua_register(L, "gameSends", gameSends);
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
        // readyRead() covers it all - Linux autotunes to tens of MB and does, and
        // Windows is asked for 1 MB. Where the buffer is smaller the sender
        // dribbles, readyRead() keeps firing and even the unfixed code finishes,
        // so there the test passes without discriminating rather than failing
        // spuriously.
        constexpr int lineCount = 12000;
        mpHost->mainConsoleView()->buffer.clear();

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
                                    .arg(mpHost->mainConsoleView()->buffer.getLastLineNumber())
                                    .arg(lineCount)));
    }

    // One compressed read that inflates to more than the drain goes deep in one
    // go: every line of it has to arrive, once each and in order
    void aCompressedReadInflatingPastTheDrainDepthIsShownInFull()
    {
        // well under one read of compressed data
        constexpr int lineCount = 20000;
        mpHost->mainConsoleView()->buffer.clear();

        const QByteArray compressed = compressedBurst(burstOf(lineCount) + "MCCP BURST END\r\n");
        QVERIFY(compressed.size() < 100000);
        mpServer->sendRaw(QByteArray("\xFF\xFB\x56\xFF\xFA\x56\xFF\xF0", 8) + compressed);

        QElapsedTimer timer;
        timer.start();
        while (timer.durationElapsed() < 15s && !tailContains(qsl("MCCP BURST END"))) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        QVERIFY2(tailContains(qsl("MCCP BURST END")),
                 qPrintable(qsl("the compressed burst stopped part-way through, at buffer line %1 of %2.").arg(mpHost->mainConsoleView()->buffer.getLastLineNumber()).arg(lineCount)));
        const int outOfPlace = firstBurstLineOutOfPlace(lineCount);
        QVERIFY2(outOfPlace < 0, qPrintable(qsl("burst line %1 is missing, repeated or out of order.").arg(outOfPlace)));
    }

    // More compressed data than one read takes: what a drain deferred has to go
    // into zlib ahead of the rest of the stream still waiting in the socket
    void deferredInputIsProcessedAheadOfLaterReads()
    {
        constexpr int lineCount = 60000;
        mpHost->mainConsoleView()->buffer.clear();

        const QByteArray compressed = compressedBurst(burstOf(lineCount) + "DEFERRED BURST END\r\n");
        QVERIFY(compressed.size() > 100000);
        mpServer->sendRaw(QByteArray("\xFF\xFB\x56\xFF\xFA\x56\xFF\xF0", 8) + compressed);

        QElapsedTimer timer;
        timer.start();
        while (timer.durationElapsed() < 30s && !tailContains(qsl("DEFERRED BURST END"))) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        QVERIFY2(tailContains(qsl("DEFERRED BURST END")),
                 qPrintable(qsl("the compressed burst stopped part-way through, at buffer line %1 of %2.").arg(mpHost->mainConsoleView()->buffer.getLastLineNumber()).arg(lineCount)));
        const int outOfPlace = firstBurstLineOutOfPlace(lineCount);
        QVERIFY2(outOfPlace < 0, qPrintable(qsl("burst line %1 is missing, repeated or out of order.").arg(outOfPlace)));
    }

    // Closes the connection; the tests after it connect again.
    // The game closing straight after a burst: what a drain deferred was read
    // before the close, so it has to be shown even though no read follows it
    void aBurstTheGameClosesAfterIsShownInFull()
    {
        constexpr int lineCount = 10000;
        mpHost->mainConsoleView()->buffer.clear();

        // Some fifty drains' worth from one read, so the close is seen while
        // most of it is still deferred
        const QByteArray leadIn(50 * cTelnet::scmMaxDecompressionRecursion * 100000, '\0');
        const QByteArray compressed = qCompress(leadIn + "\r\n" + burstOf(lineCount) + "BURST BEFORE CLOSE END\r\n", 9).mid(4);
        QVERIFY(compressed.size() < 100000);
        negotiateMccp();
        QSignalSpy disconnected(&(mpHost->mTelnet), &cTelnet::signal_disconnected);
        mpServer->sendRaw(QByteArray("\xFF\xFA\x56\xFF\xF0", 5) + compressed);
        mpServer->closeClient();

        // Not the socket's state: that changes as the close is noticed, ahead
        // of what is still to be shown
        QVERIFY2(disconnected.wait(15s), "the game closing the connection was never noticed");
        QCOMPARE(mpHost->mTelnet.getConnectionState(), QAbstractSocket::UnconnectedState);
        QVERIFY2(bufferContains(qsl("BURST BEFORE CLOSE END")),
                 qPrintable(qsl("the burst stopped part-way through when the game closed, at burst line %1 of %2.").arg(firstBurstLineOutOfPlace(lineCount)).arg(lineCount)));
        const int outOfPlace = firstBurstLineOutOfPlace(lineCount);
        QVERIFY2(outOfPlace < 0, qPrintable(qsl("burst line %1 is missing, repeated or out of order.").arg(outOfPlace)));
    }

    // Each test from here on connects again, if the one before it closed the
    // connection, and closes it in turn. A game that sends its last words and
    // closes straight away: most of them are still waiting to be read when the
    // close is noticed.
    void aBurstFollowedByACloseIsShownInFull()
    {
        QVERIFY2(connectAgain(), "could not connect to the game again");
        mpHost->mainConsoleView()->buffer.clear();
        QSignalSpy disconnected(&(mpHost->mTelnet), &cTelnet::signal_disconnected);

        QVERIFY2(sendLastWordsAndClose(), "the client never answered the negotiation");
        QVERIFY2(disconnected.wait(15s), "the game closing the connection was never noticed");
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        QVERIFY2(tailContains(qsl("GOODBYE FROM THE GAME"), 20),
                 qPrintable(qsl("the text sent before the close was cut short, at buffer line %1 of %2.").arg(mpHost->mainConsoleView()->buffer.getLastLineNumber()).arg(scmLastWordsLineCount)));
        const int goodbye = lastLineContaining(qsl("GOODBYE FROM THE GAME"));
        const int alert = lastLineContaining(qsl("Socket got disconnected"));
        QVERIFY2(alert > goodbye, qPrintable(qsl("the disconnect message is at buffer line %1, ahead of the game's goodbye at %2.").arg(alert).arg(goodbye)));
        const int outOfPlace = firstBurstLineOutOfPlace(scmLastWordsLineCount);
        QVERIFY2(outOfPlace < 0, qPrintable(qsl("burst line %1 is missing, repeated or out of order.").arg(outOfPlace)));
    }

    // A failed GMCP login or sign-in turns off reconnecting while the game is
    // still connected; what the game says before closing is still to be shown
    void aCloseAfterAFailedLoginIsShownInFull()
    {
        QVERIFY2(connectAgain(), "could not connect to the game again");
        mpHost->mainConsoleView()->buffer.clear();
        QSignalSpy disconnected(&(mpHost->mTelnet), &cTelnet::signal_disconnected);

        mpHost->mTelnet.setDontReconnect(true);
        QVERIFY2(sendLastWordsAndClose(), "the client never answered the negotiation");
        QVERIFY2(disconnected.wait(15s), "the game closing the connection was never noticed");
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        QVERIFY2(bufferContains(qsl("GOODBYE FROM THE GAME")),
                 qPrintable(qsl("the text sent before the close was cut short, at burst line %1 of %2.").arg(firstBurstLineOutOfPlace(scmLastWordsLineCount)).arg(scmLastWordsLineCount)));
        const int outOfPlace = firstBurstLineOutOfPlace(scmLastWordsLineCount);
        QVERIFY2(outOfPlace < 0, qPrintable(qsl("burst line %1 is missing, repeated or out of order.").arg(outOfPlace)));
    }

    // Every other profile and the display keep going while the last words are
    // taken in, a read per event-loop turn: an event posted by the first of them
    // to come after the close has to be handled ahead of a second read
    void theEventLoopTurnsWhileTheLastWordsAreShown()
    {
        QVERIFY2(connectAgain(), "could not connect to the game again");
        mpHost->mainConsoleView()->buffer.clear();
        smLinesAfterClose = 0;
        smLinesWhenPostedEventHandled = -1;
        QSignalSpy disconnected(&(mpHost->mTelnet), &cTelnet::signal_disconnected);
        TLuaInterpreter* lua = mpHost->getLuaInterpreter();
        lua_register(lua->getLuaGlobalState(), "countLineAfterClose", countLineAfterClose);
        QVERIFY(lua->compileAndExecuteScript(qsl(R"(
            lastWordsTrigger = tempRegexTrigger("^BURSTLINE ", function()
                local _, _, connected = getConnectionInfo()
                if not connected then
                    countLineAfterClose()
                end
            end)
        )")));

        // All of it in the socket at once, so the close is there to notice while most of it is unread;
        // fed out by this thread's event loop, the reads can catch up with the close and leave nothing to test
        mpServer->setSendBufferSize(4 * 1024 * 1024);
        QVERIFY2(sendPlainLastWordsAndClose(), "the client never answered the negotiation");
        QVERIFY2(disconnected.wait(15s), "the game closing the connection was never noticed");
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QVERIFY(lua->compileAndExecuteScript(qsl("killTrigger(lastWordsTrigger)")));

        QVERIFY2(bufferContains(qsl("GOODBYE FROM THE GAME")), "the text sent before the close was cut short");
        if (smLinesAfterClose <= linesInOneRead()) {
            QSKIP(qPrintable(qsl("only %1 burst lines were left when the close was noticed, too few to need more than one read").arg(smLinesAfterClose)));
        }
        QVERIFY2(smLinesWhenPostedEventHandled > 0 && smLinesWhenPostedEventHandled <= linesInOneRead(),
                 qPrintable(qsl("the event loop stood still for %1 of the %2 burst lines left at the close, when one read is at most %3.")
                                    .arg(smLinesWhenPostedEventHandled)
                                    .arg(smLinesAfterClose)
                                    .arg(linesInOneRead())));
    }

    // A turn holds one read, or two when the first empties Qt's buffer and the notifier has
    // more. What is still unread waits in the socket, not in Qt.
    void theEventLoopTurnsWhileAFloodIsTakenIn()
    {
        QVERIFY2(connectAgain(), "could not connect to the game again");
        QVERIFY(sendFloodAndWait());

        QCOMPARE(smFloodLines, scmFloodLineCount);
        QVERIFY2(smFloodLineOutOfPlace < 0, qPrintable(qsl("burst line %1 is out of order.").arg(smFloodLineOutOfPlace)));
        QCOMPARE(mpHost->mTelnet.getConnectionState(), QAbstractSocket::ConnectedState);
        QVERIFY2(floodTookOneReadPerTurn(), qPrintable(floodTurnsReport()));
        QVERIFY2(smMostFloodLinesAheadOfAnEvent <= 2 * linesInOneRead(),
                 qPrintable(qsl("the event loop stood still for %1 lines of a flood, when one read is at most %2.").arg(smMostFloodLinesAheadOfAnEvent).arg(linesInOneRead())));
        // 1 MB is what slot_socketConnected() asks of Qt's read buffer
        QVERIFY2(smMostTextHeldByQt <= 1024 * 1024, qPrintable(qsl("Qt was holding %1 bytes of a flood.").arg(smMostTextHeldByQt)));
    }

    void aNestedEventLoopDuringAFloodKeepsEveryLineInOrder_data()
    {
        QTest::addColumn<bool>("compressed");
        QTest::newRow("plain") << false;
        QTest::newRow("compressed") << true;
    }

    // A read started inside a trigger's nested event loop lands ahead of the one it interrupts and gives zlib its stream out of
    // order. Nests in every read of the first third, as Qt blocks readyRead inside its own emission and while its buffer is full.
    void aNestedEventLoopDuringAFloodKeepsEveryLineInOrder()
    {
        QFETCH(bool, compressed);
        QVERIFY2(connectAgain(), "could not connect to the game again");
        if (compressed) {
            negotiateMccp();
        }
        TLuaInterpreter* lua = mpHost->getLuaInterpreter();
        lua_register(lua->getLuaGlobalState(), "spinNestedEventLoop", spinNestedEventLoop);
        QVERIFY(lua->compileAndExecuteScript(qsl(R"(nestedLoopTrigger = tempRegexTrigger("^BURSTLINE 0\\d\\d000 ", function() spinNestedEventLoop(5) end))")));
        const auto readyReads = connect(mpHost->mTelnet.mpSocket, &QIODevice::readyRead, this, []() {
            if (smInNestedLoop) {
                ++smReadyReadsInNestedLoop;
            }
        });
        const bool floodSent = sendFloodAndWait(compressed);
        disconnect(readyReads);
        QVERIFY(floodSent);
        QVERIFY(lua->compileAndExecuteScript(qsl("killTrigger(nestedLoopTrigger)")));
        // The compressed stream is not ended, and the tests after this connect again
        if (compressed) {
            QSignalSpy disconnected(&(mpHost->mTelnet), &cTelnet::signal_disconnected);
            mpHost->mTelnet.disconnectIt();
            QVERIFY2(disconnected.count() || disconnected.wait(15s), "the disconnect never finished");
        }

        QVERIFY2(smFloodLineOutOfPlace < 0, qPrintable(qsl("burst line %1 reached its trigger out of order.").arg(smFloodLineOutOfPlace)));
        QCOMPARE(smFloodLines, scmFloodLineCount);
        if (smReadyReadsInNestedLoop == 0) {
            QSKIP("no readyRead came inside a nested event loop, which is what this needs");
        }
        // One read of a compressed stream inflates to many times the lines
        if (!compressed) {
            QVERIFY2(floodTookOneReadPerTurn(), qPrintable(floodTurnsReport()));
        }
    }

    // feedTelnet() is refused while connected, so this is a trigger on fed text connecting. No read is running
    // to take in what the game sends during its nested event loop: that has to follow the fed text by itself
    void textFromTheGameDuringANestedEventLoopOnFedTextFollowsIt()
    {
        QVERIFY2(connectAgain(), "could not connect to the game again");
        QSignalSpy disconnected(&(mpHost->mTelnet), &cTelnet::signal_disconnected);
        mpHost->mTelnet.disconnectIt();
        QVERIFY2(disconnected.count() || disconnected.wait(15s), "the disconnect never finished");
        mpHost->mainConsoleView()->buffer.clear();
        mpServer->setWelcomeMessage(qsl("LINE FROM THE GAME"));
        const auto welcomeGuard = qScopeGuard([this] {
            mpServer->setWelcomeMessage(QString());
        });
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        TLuaInterpreter* lua = mpHost->getLuaInterpreter();
        lua_register(lua->getLuaGlobalState(), "spinNestedEventLoop", spinNestedEventLoop);
        QVERIFY(lua->compileAndExecuteScript(qsl(R"(
            tempRegexTrigger("^FIRST FED LINE", function()
                reconnect()
                spinNestedEventLoop(500)
            end, 1)
            feedTelnet("FIRST FED LINE\r\nLAST FED LINE\r\n")
        )")));
        if (connected.isEmpty()) {
            QSKIP("the connection took longer than the nested event loop, which is what this needs");
        }

        QElapsedTimer timer;
        timer.start();
        while (timer.durationElapsed() < 15s && !bufferContains(qsl("LINE FROM THE GAME"))) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        const int fromTheGame = lastLineContaining(qsl("LINE FROM THE GAME"));
        QVERIFY2(fromTheGame >= 0, "what the game sent was left unread");
        QVERIFY2(fromTheGame > lastLineContaining(qsl("LAST FED LINE")), "what the game sent landed in the middle of the fed text");
    }

    void textDuringANestedEventLoopOnAHeldLineIsKept_data()
    {
        QTest::addColumn<QString>("during");
        QTest::addColumn<QString>("lastText");
        QTest::newRow("a partial line") << qsl(R"(ONLY A PARTIAL)") << qsl("ONLY A PARTIAL");
        QTest::newRow("lines and a partial line") << qsl(R"(SECOND LINE\r\nTHIRD PART)") << qsl("THIRD PART");
    }

    // The posting timer shows a partial line after the game falls quiet, outside any read. What the game sends during
    // a trigger's nested event loop on it is kept. A loop shorter than that timeout, so the timer shows nothing during it.
    void textDuringANestedEventLoopOnAHeldLineIsKept()
    {
        QFETCH(QString, during);
        QFETCH(QString, lastText);
        QVERIFY2(connectAgain(), "could not connect to the game again");
        // The stub's welcome, 100 ms after it accepts, would complete the partial line
        QTRY_VERIFY(mpServer->clientConnected());
        QTest::qWait(200);
        mpServer->sendRaw("READY\r\n");
        QElapsedTimer timer;
        timer.start();
        while (timer.durationElapsed() < 15s && !bufferContains(qsl("READY"))) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        mpHost->mainConsoleView()->buffer.clear();
        registerNestedLoopFunctions();
        TLuaInterpreter* lua = mpHost->getLuaInterpreter();
        QVERIFY(lua->compileAndExecuteScript(qsl(R"(
            tempRegexTrigger("^HELD LINE", function()
                gameSends("%1")
                spinNestedEventLoop(100)
            end, 1)
        )")
                                                     .arg(during)));

        mpServer->sendRaw("HELD LINE");
        timer.start();
        while (timer.durationElapsed() < 15s && !bufferContains(lastText)) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        const int last = lastLineContaining(lastText);
        mpServer->sendRaw("\r\n");
        QVERIFY2(last >= 0, qPrintable(qsl("\"%1\" was lost.").arg(lastText)));
        QVERIFY2(last > lastLineContaining(qsl("HELD LINE")), "the text from the nested event loop came ahead of the line it followed");
    }

    void aPartialLineFedFromATriggerIsKept_data()
    {
        QTest::addColumn<QString>("fed");
        QTest::newRow("on a line the posting timer shows") << qsl("FED TO THE TRIGGER");
        QTest::newRow("on a whole line") << qsl(R"(FED TO THE TRIGGER\r\n)");
        QTest::newRow("on a whole line and a partial line") << qsl(R"(FED TO THE TRIGGER\r\nOUTER PARTIAL)");
    }

    // Offline, as feedTelnet() is refused on a connection
    void aPartialLineFedFromATriggerIsKept()
    {
        QFETCH(QString, fed);
        if (mpHost->mTelnet.getConnectionState() != QAbstractSocket::UnconnectedState) {
            QSignalSpy disconnected(&(mpHost->mTelnet), &cTelnet::signal_disconnected);
            mpHost->mTelnet.disconnectIt();
            QVERIFY2(disconnected.count() || disconnected.wait(15s), "the disconnect never finished");
        }
        mpHost->mainConsoleView()->buffer.clear();
        TLuaInterpreter* lua = mpHost->getLuaInterpreter();
        QVERIFY(lua->compileAndExecuteScript(qsl(R"(
            tempRegexTrigger("^FED TO THE TRIGGER", function() feedTelnet("LINE FROM THE TRIGGER\r\nPARTIAL FROM THE TRIGGER") end, 1)
            feedTelnet("%1")
        )")
                                                     .arg(fed)));

        QElapsedTimer timer;
        timer.start();
        while (timer.durationElapsed() < 15s && !bufferContains(qsl("PARTIAL FROM THE TRIGGER"))) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        const bool kept = bufferContains(qsl("PARTIAL FROM THE TRIGGER"));
        QVERIFY(lua->compileAndExecuteScript(qsl(R"(feedTelnet("\r\n"))")));
        QVERIFY2(kept, "the partial line the trigger fed was lost");
    }

    // A replay can run on a live connection, sharing the text held back from the game
    void textFromTheGameDuringANestedEventLoopInAReplayFollowsTheChunk()
    {
        QVERIFY2(connectAgain(), "could not connect to the game again");
        mpHost->mainConsoleView()->buffer.clear();
        registerNestedLoopFunctions();
        const QString fileName = mConfigDir.filePath(qsl("nested-loop-replay.dat"));
        {
            QFile file(fileName);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QDataStream stream(&file);
            stream.setVersion(QDataStream::Qt_5_12);
            const QByteArray chunk("REPLAY FIRST\r\nREPLAY LAST\r\n");
            stream << static_cast<qint32>(0) << static_cast<qint32>(chunk.size());
            stream.writeRawData(chunk.constData(), static_cast<int>(chunk.size()));
        }
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl(R"(
            tempRegexTrigger("^REPLAY FIRST", function()
                gameSends("LIVE LINE\r\nLIVE PARTIAL")
                spinNestedEventLoop(100)
            end, 1)
            assert(loadReplay("%1"))
        )")
                                                                             .arg(fileName)));

        QElapsedTimer timer;
        timer.start();
        while (timer.durationElapsed() < 15s && !bufferContains(qsl("LIVE PARTIAL"))) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        const int partial = lastLineContaining(qsl("LIVE PARTIAL"));
        mpServer->sendRaw("\r\n");
        QVERIFY2(partial >= 0, "the partial line the game sent was lost");
        QVERIFY2(lastLineContaining(qsl("LIVE LINE")) > lastLineContaining(qsl("REPLAY LAST")), "the game's text landed in the middle of the replay's chunk");
    }

    // A trigger on the last words that disconnects stops them there, as it
    // would on a connection still open: the goodbye is reads away
    void aTriggerDisconnectingDuringTheLastWordsEndsThem()
    {
        QVERIFY2(connectAgain(), "could not connect to the game again");
        mpHost->mainConsoleView()->buffer.clear();
        QSignalSpy disconnected(&(mpHost->mTelnet), &cTelnet::signal_disconnected);
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl(R"(tempRegexTrigger("^BURSTLINE 026000", function() disconnect() end, 1))")));

        QVERIFY2(sendLastWordsAndClose(), "the client never answered the negotiation");
        QVERIFY2(disconnected.wait(15s), "the connection ending was never noticed");
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        QVERIFY2(bufferContains(qsl("BURSTLINE 026000")), "the line the trigger disconnects on was never shown");
        QVERIFY2(!bufferContains(qsl("GOODBYE FROM THE GAME")), "the game's text went on being shown after a trigger disconnected");
    }

    // The reconnect follows the disconnect it answers, rather than starting in
    // the middle of tearing down the connection it replaces
    void aTriggerReconnectingOnTheLastWordsDoesSoOnceTheyAreIn()
    {
        QVERIFY2(connectAgain(), "could not connect to the game again");
        mpHost->mainConsoleView()->buffer.clear();
        QStringList order;
        const auto connecting = connect(&(mpHost->mTelnet), &cTelnet::signal_connecting, this, [&order]() {
            order << qsl("connecting");
        });
        const auto connected = connect(&(mpHost->mTelnet), &cTelnet::signal_connected, this, [&order]() {
            order << qsl("connected");
        });
        const auto disconnected = connect(&(mpHost->mTelnet), &cTelnet::signal_disconnected, this, [&order]() {
            order << qsl("disconnected");
        });
        QVERIFY(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl(R"(tempRegexTrigger("^GOODBYE FROM THE GAME", function() reconnect() end, 1))")));

        QVERIFY2(sendLastWordsAndClose(), "the client never answered the negotiation");
        QElapsedTimer timer;
        timer.start();
        while (timer.durationElapsed() < 15s && !order.contains(qsl("connected"))) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }
        disconnect(connecting);
        disconnect(connected);
        disconnect(disconnected);

        QVERIFY2(bufferContains(qsl("GOODBYE FROM THE GAME")), "the text sent before the close was cut short");
        QCOMPARE(order, QStringList({qsl("disconnected"), qsl("connecting"), qsl("connected")}));
        QCOMPARE(mpHost->mTelnet.getConnectionState(), QAbstractSocket::ConnectedState);
    }

    // The close is noticed while a trigger holds a nested event loop, in the
    // middle of a read: the rest has to wait for that read, not be dropped
    void aCloseSeenInsideANestedEventLoopIsShownInFull()
    {
        constexpr int lineCount = 30000;
        QVERIFY2(connectAgain(), "could not connect to the game again");
        mpHost->mainConsoleView()->buffer.clear();
        QSignalSpy disconnected(&(mpHost->mTelnet), &cTelnet::signal_disconnected);
        TLuaInterpreter* lua = mpHost->getLuaInterpreter();
        lua_register(lua->getLuaGlobalState(), "spinNestedEventLoop", spinNestedEventLoop);
        QVERIFY(lua->compileAndExecuteScript(qsl(R"(tempRegexTrigger("^BURSTLINE 000100", function() spinNestedEventLoop(500) end, 1))")));

        mpServer->sendRaw(burstOf(lineCount) + "GOODBYE FROM THE GAME\r\n");
        mpServer->closeClient();
        QVERIFY2(disconnected.wait(15s), "the game closing the connection was never noticed");
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        QVERIFY2(bufferContains(qsl("GOODBYE FROM THE GAME")),
                 qPrintable(qsl("the text sent before the close was cut short, at burst line %1 of %2.").arg(firstBurstLineOutOfPlace(lineCount)).arg(lineCount)));
        const int outOfPlace = firstBurstLineOutOfPlace(lineCount);
        QVERIFY2(outOfPlace < 0, qPrintable(qsl("burst line %1 is missing, repeated or out of order.").arg(outOfPlace)));
    }

    // A reconnect on the last words waits for a trigger's nested event loop, with no more than
    // a retry now and then in that time: a dialog can stay open for as long as the player likes
    void aReconnectWaitingForANestedEventLoopLeavesTheEventLoopIdle()
    {
        constexpr int loopMilliseconds = 300;
        QVERIFY2(connectAgain(), "could not connect to the game again");
        mpHost->mainConsoleView()->buffer.clear();
        registerNestedLoopFunctions();
        TLuaInterpreter* lua = mpHost->getLuaInterpreter();
        QVERIFY(lua->compileAndExecuteScript(qsl(R"(
            reconnectAsked = false
            reconnectingTrigger = tempRegexTrigger("^BURSTLINE ", function()
                local _, _, connected = getConnectionInfo()
                if not connected and not reconnectAsked then
                    reconnectAsked = true
                    reconnect()
                    spinNestedEventLoop(%1)
                end
            end)
        )")
                                                     .arg(loopMilliseconds)));
        int passes = 0;
        const auto awake = connect(QAbstractEventDispatcher::instance(), &QAbstractEventDispatcher::awake, this, [&passes]() {
            if (smInNestedLoop) {
                ++passes;
            }
        });
        QSignalSpy reconnected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QSignalSpy accepted(mpServer, &QTcpServer::newConnection);

        // All of it in the socket at once, so that the close is there to notice while most of it is unread
        mpServer->setSendBufferSize(4 * 1024 * 1024);
        const bool sent = sendPlainLastWordsAndClose();
        // The tests after this send to the stub, which drops what it is given ahead of accepting
        const bool connectedAgain = sent && reconnected.wait(15s) && (accepted.count() || accepted.wait(15s));
        disconnect(awake);
        QVERIFY(lua->compileAndExecuteScript(qsl("killTrigger(reconnectingTrigger)")));

        QVERIFY2(sent, "the client never answered the negotiation");
        QVERIFY2(connectedAgain, "the reconnect never connected");
        if (passes == 0) {
            QSKIP("no burst line was left when the close was noticed, which is what this needs");
        }
        QVERIFY2(passes < loopMilliseconds, qPrintable(qsl("the event loop went round %1 times in a nested loop of %2 ms.").arg(passes).arg(loopMilliseconds)));
    }

    // Reconnecting aborts the live connection, which empties its socket: there
    // is nothing left of it to read
    void aReconnectOnALiveConnectionReadsNothingFromItsClosedSocket()
    {
        QVERIFY2(connectAgain(), "could not connect to the game again");
        QTest::failOnWarning(QRegularExpression(qsl("device not open")));
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        mpHost->mTelnet.connectIt(mLocalhost, mpServer->serverPort());
        QVERIFY2(connected.wait(15s), "the reconnect never connected");
    }
};

#include "TelnetLargeBurstTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetLargeBurstTest)
