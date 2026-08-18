/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org     *
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
 * Tests that an unterminated ANSI string sequence cannot silence the game -
 * https://github.com/Mudlet/Mudlet/issues/9757
 *
 * ESC followed by 'P', 'X', '^', '_' or ']' starts a DCS, SOS, PM, APC or OSC
 * sequence, whose payload is consumed rather than displayed. A game that emits
 * one of those introducers by accident must not blacken the output that
 * follows it: the sequence ends at the first BEL, ESC \ or line ending, and a
 * connection boundary drops it outright. Well-formed sequences must still be
 * consumed in full, including ones that are larger than the buffering cap or
 * split over several packets.
 *
 * Everything is driven over a real TCP socket via TelnetServerStub so that the
 * bytes travel the same path as a game's output.
 *
 * Run with: ctest -R TelnetStringSequenceRecoveryTest -V
 */

#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

namespace {
const QByteArray csBel = QByteArrayLiteral("\x07");
const QByteArray csStringTerminator = QByteArrayLiteral("\x1b") + QByteArrayLiteral("\\");
const QByteArray csCrLf = QByteArrayLiteral("\r\n");
// Distinctive enough that finding it anywhere in the buffer means a payload
// that should have been consumed was displayed instead:
const QByteArray csPayload = QByteArrayLiteral("0;SEQPAYLOAD");
} // namespace

class TelnetStringSequenceRecoveryTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("String-Sequence-Recovery-Test-Host");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    // Every introducer that starts a sequence whose payload is consumed
    // instead of displayed.
    static void addIntroducerColumn()
    {
        QTest::addColumn<QByteArray>("introducer");
        QTest::newRow("DCS (ESC P)") << QByteArrayLiteral("\x1b") + QByteArrayLiteral("P");
        QTest::newRow("SOS (ESC X)") << QByteArrayLiteral("\x1b") + QByteArrayLiteral("X");
        QTest::newRow("PM (ESC ^)") << QByteArrayLiteral("\x1b") + QByteArrayLiteral("^");
        QTest::newRow("APC (ESC _)") << QByteArrayLiteral("\x1b") + QByteArrayLiteral("_");
        QTest::newRow("OSC (ESC ])") << QByteArrayLiteral("\x1b") + QByteArrayLiteral("]");
    }

    void sendFromServer(const QByteArray& data) { mpServer->sendRaw(data); }

    // Sends data that does not end a line and waits out cTelnet's 300ms idle
    // flush, so the parser has seen every byte of it - including the carriage
    // return that flush puts through the parser - by the time this returns.
    void sendFromServerAndSettle(const QByteArray& data)
    {
        mpServer->sendRaw(data);
        QTest::qWait(500ms);
    }

    QString joinedBuffer() const
    {
        QStringList lines;
        for (int i = 0; i <= mpHost->mpConsole->buffer.getLastLineNumber(); ++i) {
            lines << mpHost->mpConsole->buffer.line(i);
        }
        return lines.join(QChar::LineFeed);
    }

    bool waitForBufferText(const QString& needle, int timeoutMs = 5000)
    {
        return QTest::qWaitFor(
                [this, &needle]() {
                    return joinedBuffer().contains(needle);
                },
                timeoutMs);
    }

    // Several times MAX_OSC_SEQUENCE_LENGTH, and position-dependent so that a
    // failure says where the payload started leaking.
    static QByteArray oversizedPayload()
    {
        QByteArray payload;
        for (int i = 0; payload.size() < 20000; ++i) {
            payload += QByteArrayLiteral("payload-") + QByteArray::number(i).rightJustified(4, '0') + QByteArrayLiteral(" ");
        }
        return payload;
    }

    // Takes the profile offline and back on with the stub greeting the new
    // connection with welcomeMessage, leaving an empty display behind so that
    // greeting is the first thing in it.
    bool reconnectWithWelcomeMessage(const QString& welcomeMessage)
    {
        mpServer->setWelcomeMessage(welcomeMessage);
        mpHost->mTelnet.disconnectIt();
        const bool wentOffline = QTest::qWaitFor(
                [this]() {
                    return mpHost->mTelnet.getConnectionState() == QAbstractSocket::UnconnectedState;
                },
                5000);
        mpHost->mpConsole->buffer.clear();
        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        mpHost->mTelnet.connectIt(mLocalhost, mPort.toInt());
        const bool reconnected = connectedSpy.wait(5000);
        const bool arrived = waitForBufferText(welcomeMessage);
        mpServer->setWelcomeMessage(QString());
        return wentOffline && reconnected && arrived;
    }

    bool bufferHasLine(const QString& text) const
    {
        for (int i = 0; i <= mpHost->mpConsole->buffer.getLastLineNumber(); ++i) {
            if (mpHost->mpConsole->buffer.line(i) == text) {
                return true;
            }
        }
        return false;
    }

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    // Start mudlet and create a profile once for all tests.
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
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

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    // Each test starts from an empty display and a parser that is not part way
    // through a sequence, so a failure cannot cascade into the tests after it.
    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        mpHost->mpConsole->buffer.clear();
        mpHost->mpConsole->buffer.resetSequenceParserState();
    }

    // A BEL ends the sequence: the payload is consumed and the text around it
    // is displayed.
    void recoveryByBel_data() { addIntroducerColumn(); }

    void recoveryByBel()
    {
        QFETCH(QByteArray, introducer);

        sendFromServer(QByteArrayLiteral("PRE") + introducer + csPayload + csBel + QByteArrayLiteral("POST") + csCrLf);

        QVERIFY2(waitForBufferText(qsl("PREPOST")), qPrintable(qsl("Text around the sequence went missing, buffer holds: '%1'").arg(joinedBuffer())));
        QVERIFY2(!joinedBuffer().contains(qsl("SEQPAYLOAD")), qPrintable(qsl("Sequence payload was displayed: '%1'").arg(joinedBuffer())));
    }

    // ESC \ (the String Terminator) ends the sequence just as a BEL does.
    void recoveryByStringTerminator_data() { addIntroducerColumn(); }

    void recoveryByStringTerminator()
    {
        QFETCH(QByteArray, introducer);

        sendFromServer(QByteArrayLiteral("PRE") + introducer + csPayload + csStringTerminator + QByteArrayLiteral("POST") + csCrLf);

        QVERIFY2(waitForBufferText(qsl("PREPOST")), qPrintable(qsl("Text around the sequence went missing, buffer holds: '%1'").arg(joinedBuffer())));
        QVERIFY2(!joinedBuffer().contains(qsl("SEQPAYLOAD")), qPrintable(qsl("Sequence payload was displayed: '%1'").arg(joinedBuffer())));
    }

    // The reported bug: with no terminator at all the sequence must end with
    // the line it started on, so the lines after it are still displayed.
    void recoveryByEndOfLine_data() { addIntroducerColumn(); }

    void recoveryByEndOfLine()
    {
        QFETCH(QByteArray, introducer);

        sendFromServer(QByteArrayLiteral("PRE") + introducer + csPayload + csCrLf + QByteArrayLiteral("RESUMED-ONE") + csCrLf + QByteArrayLiteral("RESUMED-TWO") + csCrLf
                       + QByteArrayLiteral("RESUMED-THREE") + csCrLf);

        QVERIFY2(waitForBufferText(qsl("RESUMED-THREE")), qPrintable(qsl("Output stayed dark after an unterminated sequence, buffer holds: '%1'").arg(joinedBuffer())));
        const QString displayed = joinedBuffer();
        QVERIFY2(displayed.contains(qsl("RESUMED-ONE")), qPrintable(qsl("First line after the sequence went missing: '%1'").arg(displayed)));
        QVERIFY2(displayed.contains(qsl("RESUMED-TWO")), qPrintable(qsl("Second line after the sequence went missing: '%1'").arg(displayed)));
        QVERIFY2(bufferHasLine(qsl("PRE")), qPrintable(qsl("Text before the introducer went missing: '%1'").arg(displayed)));
        QVERIFY2(!displayed.contains(qsl("SEQPAYLOAD")), qPrintable(qsl("Sequence payload was displayed: '%1'").arg(displayed)));
    }

    // A game that keeps talking after an unterminated sequence has every line
    // of it displayed, not just the first.
    void volumeAfterUnterminatedSequence_data() { addIntroducerColumn(); }

    void volumeAfterUnterminatedSequence()
    {
        QFETCH(QByteArray, introducer);

        QByteArray data = QByteArrayLiteral("The chest is locked.") + introducer + csPayload + csCrLf;
        for (int i = 0; i < 50; ++i) {
            data += QByteArrayLiteral("bulk line ") + QByteArray::number(i) + csCrLf;
        }
        sendFromServer(data);

        QVERIFY2(waitForBufferText(qsl("bulk line 49")), qPrintable(qsl("Bulk output after an unterminated sequence was lost, buffer holds: '%1'").arg(joinedBuffer())));
        for (int i = 0; i < 50; ++i) {
            QVERIFY2(bufferHasLine(qsl("bulk line %1").arg(i)), qPrintable(qsl("Line %1 was lost, buffer holds: '%2'").arg(QString::number(i), joinedBuffer())));
        }
    }

    // A sequence still waiting for its terminator when the display is cleared
    // must not blacken the output that follows the clear.
    void recoveryAfterClear_data() { addIntroducerColumn(); }

    void recoveryAfterClear()
    {
        QFETCH(QByteArray, introducer);

        // No line ending, so the parser is legitimately still inside the
        // sequence when the display is cleared:
        sendFromServerAndSettle(QByteArrayLiteral("BEFORE-CLEAR") + introducer + csPayload);
        mpHost->mpConsole->buffer.clear();

        // clear() deliberately leaves the parser alone, so the sequence runs
        // on to the next line ending the game sends - which costs the first of
        // these lines, unless the game happened to end a line in between:
        sendFromServer(QByteArrayLiteral("AFTER-CLEAR-ONE") + csCrLf + QByteArrayLiteral("AFTER-CLEAR-TWO") + csCrLf);

        QVERIFY2(waitForBufferText(qsl("AFTER-CLEAR-TWO")), qPrintable(qsl("Output stayed dark after clear(), buffer holds: '%1'").arg(joinedBuffer())));
    }

    // Bytes that a dead connection never terminated cannot be completed by the
    // next one, so the very first line of a new session is displayed.
    void recoveryAfterReconnect_data() { addIntroducerColumn(); }

    void recoveryAfterReconnect()
    {
        QFETCH(QByteArray, introducer);

        sendFromServerAndSettle(QByteArrayLiteral("BEFORE-RECONNECT") + introducer + csPayload);

        QVERIFY2(reconnectWithWelcomeMessage(qsl("NEW-SESSION-FIRST-LINE")),
                 qPrintable(qsl("The new connection's first line was eaten by the old one's sequence, buffer holds: '%1'").arg(joinedBuffer())));
    }

    // The other parser latches are dropped at the same boundary: half of a CSI
    // would otherwise swallow the head of the new session's first line.
    void halfReceivedCsiDoesNotSurviveReconnect()
    {
        sendFromServerAndSettle(QByteArrayLiteral("BEFORE-RECONNECT") + QByteArrayLiteral("\x1b") + QByteArrayLiteral("[3"));

        // A carried-over "CSI 3" would swallow the 'C' as its final byte:
        QVERIFY2(reconnectWithWelcomeMessage(qsl("CSI-RESET-CHECK")), qPrintable(qsl("A half-received CSI ate the head of the new session's first line, buffer holds: '%1'").arg(joinedBuffer())));
    }

    // A Sixel image is a well-formed DCS: its payload must be consumed in full
    // rather than spilled onto the screen.
    void wellFormedSixelIsConsumed()
    {
        sendFromServer(QByteArrayLiteral("BEFORE-SIXEL") + QByteArrayLiteral("\x1b") + QByteArrayLiteral("P0;1;0q\"1;1;6;6#0;2;0;0;0#0!6~-") + csStringTerminator + QByteArrayLiteral("AFTER-SIXEL")
                       + csCrLf);

        QVERIFY2(waitForBufferText(qsl("BEFORE-SIXELAFTER-SIXEL")), qPrintable(qsl("Text around the Sixel image went missing, buffer holds: '%1'").arg(joinedBuffer())));
        QVERIFY2(!joinedBuffer().contains(qsl("#0;2;0;0;0")), qPrintable(qsl("Sixel payload was displayed: '%1'").arg(joinedBuffer())));
    }

    // The Kitty graphics protocol's APC payloads are consumed the same way.
    void wellFormedKittyGraphicIsConsumed()
    {
        sendFromServer(QByteArrayLiteral("BEFORE-KITTY") + QByteArrayLiteral("\x1b") + QByteArrayLiteral("_Ga=T,f=100,s=1,v=1;iVBORw0KGgoAAAANSUhEUg==") + csStringTerminator
                       + QByteArrayLiteral("AFTER-KITTY") + csCrLf);

        QVERIFY2(waitForBufferText(qsl("BEFORE-KITTYAFTER-KITTY")), qPrintable(qsl("Text around the Kitty graphic went missing, buffer holds: '%1'").arg(joinedBuffer())));
        QVERIFY2(!joinedBuffer().contains(qsl("iVBORw0KGgo")), qPrintable(qsl("Kitty payload was displayed: '%1'").arg(joinedBuffer())));
    }

    // Real images run to far more than the buffering cap, so a well-formed
    // sequence that is longer than it still has to be consumed in full.
    void wellFormedSequenceLargerThanTheCapIsConsumed()
    {
        sendFromServer(QByteArrayLiteral("BEFORE-HUGE") + QByteArrayLiteral("\x1b") + QByteArrayLiteral("P") + oversizedPayload() + csStringTerminator + QByteArrayLiteral("AFTER-HUGE") + csCrLf);

        QVERIFY2(waitForBufferText(qsl("AFTER-HUGE")), qPrintable(qsl("Text after an oversized sequence went missing, buffer holds: '%1'").arg(joinedBuffer())));
        QVERIFY2(joinedBuffer().contains(qsl("BEFORE-HUGE")), qPrintable(qsl("Text before an oversized sequence went missing, buffer holds: '%1'").arg(joinedBuffer())));
        QVERIFY2(!joinedBuffer().contains(qsl("payload-0000")), qPrintable(qsl("Payload of the oversized sequence was displayed: '%1'").arg(joinedBuffer())));
        QVERIFY2(!joinedBuffer().contains(qsl("payload-1000")), qPrintable(qsl("Payload of the oversized sequence was displayed past the cap: '%1'").arg(joinedBuffer())));
    }

    // An OSC is the one string sequence whose payload Mudlet acts on, so one
    // that outgrows the cap loses its command - which has to be reported, not
    // swallowed, and must not take the rest of the line with it.
    void oversizedOscIsReportedAndItsCommandIgnored()
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("an OSC sequence passed 4096 bytes, so its command will be ignored")));

        sendFromServer(QByteArrayLiteral("\x1b") + QByteArrayLiteral("]") + oversizedPayload() + csBel + QByteArrayLiteral("AFTER-OVERSIZED-OSC") + csCrLf);

        QVERIFY2(waitForBufferText(qsl("AFTER-OVERSIZED-OSC")), qPrintable(qsl("Text after an oversized OSC went missing, buffer holds: '%1'").arg(joinedBuffer())));
        QVERIFY2(!joinedBuffer().contains(qsl("payload-1000")), qPrintable(qsl("Payload of the oversized OSC was displayed: '%1'").arg(joinedBuffer())));
    }

    // Giving up on one sequence must not leave the parser refusing to decode
    // every OSC that follows it.
    void oscStillDecodesAfterAnUnterminatedSequence()
    {
        const bool savedMayRedefine = mpHost->getMayRedefineColors();
        const QColor savedRed = mpHost->mRed;
        mpHost->setMayRedefineColors(true);

        sendFromServer(QByteArrayLiteral("BEFORE-ABORT") + QByteArrayLiteral("\x1b") + QByteArrayLiteral("]") + csPayload + csCrLf);
        const bool aborted = waitForBufferText(qsl("BEFORE-ABORT"));
        sendFromServer(QByteArrayLiteral("\x1b") + QByteArrayLiteral("]P1223344") + csBel + QByteArrayLiteral("OSC-AFTER-ABORT") + csCrLf);
        const bool arrived = waitForBufferText(qsl("OSC-AFTER-ABORT"));
        const QColor redAfter = mpHost->mRed;

        mpHost->mRed = savedRed;
        mpHost->setMayRedefineColors(savedMayRedefine);

        QVERIFY2(aborted, "The line carrying the unterminated sequence never arrived");
        QVERIFY2(arrived, qPrintable(qsl("Text after the second OSC went missing, buffer holds: '%1'").arg(joinedBuffer())));
        QVERIFY2(redAfter == QColor(0x22, 0x33, 0x44), qPrintable(qsl("An OSC sent after an abandoned sequence was consumed but not decoded, red is %1").arg(redAfter.name())));
    }

    // Bounding the sequence at the end of a line must not break a well-formed
    // one that arrives in several packets - including one that pauses for
    // longer than cTelnet's 300ms idle flush, which puts a carriage return
    // through the parser part way through the payload.
    void wellFormedSequenceSplitAcrossReads()
    {
        sendFromServerAndSettle(QByteArrayLiteral("PRE-SPLIT") + QByteArrayLiteral("\x1b") + QByteArrayLiteral("P") + QByteArrayLiteral("SEQ"));

        sendFromServer(QByteArrayLiteral("PAYLOAD") + csStringTerminator + QByteArrayLiteral("POST-SPLIT") + csCrLf);

        QVERIFY2(waitForBufferText(qsl("PRE-SPLITPOST-SPLIT")), qPrintable(qsl("A sequence split across two reads lost the text around it: '%1'").arg(joinedBuffer())));
        QVERIFY2(!joinedBuffer().contains(qsl("SEQ")), qPrintable(qsl("Payload of the split sequence was displayed: '%1'").arg(joinedBuffer())));
    }

    // ...and an unterminated one that is split still ends at the line ending
    // that arrives in the later packet.
    void unterminatedSequenceSplitAcrossReads()
    {
        sendFromServerAndSettle(QByteArrayLiteral("PRE-SPLIT-BOUND") + QByteArrayLiteral("\x1b") + QByteArrayLiteral("X") + csPayload);

        sendFromServer(csCrLf + QByteArrayLiteral("RESUMED-AFTER-SPLIT") + csCrLf);

        QVERIFY2(waitForBufferText(qsl("RESUMED-AFTER-SPLIT")), qPrintable(qsl("Output stayed dark after a split unterminated sequence: '%1'").arg(joinedBuffer())));
    }

    // The worst place for the idle flush's carriage return to land is between
    // the ESC and the backslash of a String Terminator: taken as payload it
    // would hide the terminator and cost the rest of that line.
    void stringTerminatorSplitByTheIdleFlush()
    {
        sendFromServerAndSettle(QByteArrayLiteral("PRE-ST-SPLIT") + QByteArrayLiteral("\x1b") + QByteArrayLiteral("P") + QByteArrayLiteral("PAYLOAD") + QByteArrayLiteral("\x1b"));

        sendFromServer(QByteArrayLiteral("\\") + QByteArrayLiteral("AFTER-ST-SPLIT") + csCrLf);

        QVERIFY2(waitForBufferText(qsl("AFTER-ST-SPLIT")), qPrintable(qsl("The String Terminator was missed and the rest of the line was discarded: '%1'").arg(joinedBuffer())));
        QVERIFY2(!joinedBuffer().contains(qsl("PAYLOAD")), qPrintable(qsl("Payload of the split sequence was displayed: '%1'").arg(joinedBuffer())));
    }

    // The line endings that are not a line feed: a prompt marker (the game's
    // IAC GA, or an escaped 0xff byte) and an end of transmission.
    void recoveryByOtherLineEndings_data()
    {
        QTest::addColumn<QByteArray>("lineEnding");
        QTest::newRow("end of transmission (0x04)") << QByteArrayLiteral("\x04");
        // Doubled so that the telnet layer delivers one literal 0xff byte
        // rather than reading it as the start of a command:
        QTest::newRow("prompt marker (0xff)") << QByteArrayLiteral("\xff\xff");
    }

    void recoveryByOtherLineEndings()
    {
        QFETCH(QByteArray, lineEnding);

        sendFromServer(QByteArrayLiteral("\x1b") + QByteArrayLiteral("P") + csPayload + lineEnding + QByteArrayLiteral("RESUMED-AFTER-ENDING") + csCrLf);

        QVERIFY2(waitForBufferText(qsl("RESUMED-AFTER-ENDING")), qPrintable(qsl("Output stayed dark after the line ending, buffer holds: '%1'").arg(joinedBuffer())));
        QVERIFY2(!joinedBuffer().contains(qsl("SEQPAYLOAD")), qPrintable(qsl("Sequence payload was displayed: '%1'").arg(joinedBuffer())));
    }

    // Locally generated text (feedTriggers(), MMCP chat, MXP insertions) runs
    // through the same parser but keeps its own copy of the latches, so it
    // needs the same bound.
    void unterminatedSequenceInALocalFeedIsBounded()
    {
        std::string stuck{"BEFORE-LOCAL\x1bP0;SEQPAYLOAD"};
        mpHost->mpConsole->printOnDisplay(stuck, false);
        // As on the Game Server channel, the line ending of this one is what
        // ends the sequence, so this feed is the one that is lost:
        std::string eaten{"EATEN-LOCAL\n"};
        mpHost->mpConsole->printOnDisplay(eaten, false);
        std::string resumed{"VISIBLE-LOCAL\n"};
        mpHost->mpConsole->printOnDisplay(resumed, false);

        QVERIFY2(waitForBufferText(qsl("VISIBLE-LOCAL")), qPrintable(qsl("Locally fed text stayed dark after an unterminated sequence: '%1'").arg(joinedBuffer())));
        QVERIFY2(!joinedBuffer().contains(qsl("SEQPAYLOAD")), qPrintable(qsl("Sequence payload was displayed: '%1'").arg(joinedBuffer())));
    }

    // Bounding the sequence must not stop a complete OSC from being acted on.
    void wellFormedOscIsStillDecoded()
    {
        const bool savedMayRedefine = mpHost->getMayRedefineColors();
        const QColor savedRed = mpHost->mRed;
        mpHost->setMayRedefineColors(true);

        sendFromServer(QByteArrayLiteral("\x1b") + QByteArrayLiteral("]P1223344") + csBel + QByteArrayLiteral("OSC-DECODED") + csCrLf);
        const bool arrived = waitForBufferText(qsl("OSC-DECODED"));
        const QColor redAfter = mpHost->mRed;

        mpHost->mRed = savedRed;
        mpHost->setMayRedefineColors(savedMayRedefine);

        QVERIFY2(arrived, qPrintable(qsl("Text after the OSC went missing, buffer holds: '%1'").arg(joinedBuffer())));
        QVERIFY2(redAfter == QColor(0x22, 0x33, 0x44), qPrintable(qsl("The OSC colour redefinition was not decoded, red is %1").arg(redAfter.name())));
    }

    // The open question from the bug report: text the parser discards never
    // reaches the buffer, so it is missing from the log as well - but the
    // output that resumes after the sequence is logged as usual.
    void logFollowsTheDisplayThroughAnUnterminatedSequence()
    {
        const QString savedLogDir = mpHost->mLogDir;
        const QString savedLogFileName = mpHost->mLogFileName;
        const QString savedLogFileNameFormat = mpHost->mLogFileNameFormat;
        const bool savedHtmlFormat = mpHost->mIsNextLogFileInHtmlFormat;
        auto restoreLoggingSettings = qScopeGuard([&]() {
            mpHost->mLogDir = savedLogDir;
            mpHost->mLogFileName = savedLogFileName;
            mpHost->mLogFileNameFormat = savedLogFileNameFormat;
            mpHost->mIsNextLogFileInHtmlFormat = savedHtmlFormat;
        });

        mpHost->mLogDir.clear();
        mpHost->mLogFileNameFormat.clear();
        mpHost->mLogFileName = qsl("string-sequence-recovery-test");
        mpHost->mIsNextLogFileInHtmlFormat = false;
        mpHost->mpConsole->toggleLogging(false);
        QVERIFY(mpHost->mpConsole->mLogToLogFile);
        const QString logFileName = mpHost->mpConsole->mLogFileName;

        sendFromServer(QByteArrayLiteral("PRE-LOG") + QByteArrayLiteral("\x1b") + QByteArrayLiteral("P") + csPayload + csCrLf + QByteArrayLiteral("LOGGED-AFTER-BLACKOUT") + csCrLf
                       + QByteArrayLiteral("LOGGED-LAST-LINE") + csCrLf);
        QVERIFY2(waitForBufferText(qsl("LOGGED-LAST-LINE")), qPrintable(qsl("Output stayed dark, buffer holds: '%1'").arg(joinedBuffer())));

        mpHost->mpConsole->toggleLogging(false);
        QFile logFile(logFileName);
        QVERIFY2(logFile.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(qsl("Could not read back the log at %1").arg(logFileName)));
        const QString log = QString::fromUtf8(logFile.readAll());
        logFile.close();
        logFile.remove();

        QVERIFY2(log.contains(qsl("PRE-LOG")), qPrintable(qsl("Text before the introducer is missing from the log: '%1'").arg(log)));
        QVERIFY2(log.contains(qsl("LOGGED-AFTER-BLACKOUT")), qPrintable(qsl("The line after the unterminated sequence is missing from the log: '%1'").arg(log)));
        QVERIFY2(log.contains(qsl("LOGGED-LAST-LINE")), qPrintable(qsl("The last line is missing from the log: '%1'").arg(log)));
        QVERIFY2(!log.contains(qsl("SEQPAYLOAD")), qPrintable(qsl("Sequence payload reached the log: '%1'").arg(log)));
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

#include "TelnetStringSequenceRecoveryTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetStringSequenceRecoveryTest)
