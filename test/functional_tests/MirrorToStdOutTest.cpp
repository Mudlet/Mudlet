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
 * The --mirror command line option copies console output to standard output so
 * that a CI run (and Mudlet's own Lua spec runner) can read it back. That copy
 * was only made in TConsole::print()/printFormatted(), which game text never
 * reaches, so these cases pin that a line arriving from the server is copied
 * too - exactly once, as the console shows it, and with the lines the print
 * path copies left alone.
 *
 * The copy is collected by redirecting this process's standard output into a
 * file for the length of each feed, so that what is asserted on is the bytes a
 * shell would capture rather than the call that produced them. Assertions run
 * with standard output restored, so QTest's own reporting is never swallowed.
 * The option itself is set directly here; the one line that connects --mirror
 * to it is exercised end to end by .claude/scripts/run-lua-tests.sh, which runs
 * the real binary with the option and reads the stream back.
 *
 * Run with: ctest -R MirrorToStdOutTest -V
 */

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TBuffer.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include <fcntl.h>
#include <unistd.h>

#include <cstdio>

#include "GroupedTest.h"

class MirrorToStdOutTest : public QObject
{
    Q_OBJECT

private:
    // Short enough that cTelnet's posting timer fires inside a feed's own wait,
    // so the cases that turn on it do not have to wait out the 300ms default.
    static constexpr int csmPostingTimeoutMs = 30;
    // Something to wait for before the cases start - see initTestCase()
    const QString mWelcomeMessage = qsl("the server stub says hello");

    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Mirror-To-StdOut-Test-Host");
    const QString mLocalhost = qsl("localhost");
    QString mPort;
    bool mSavedMirrorToStdOut = false;
    int mSavedPostingTimeout = 0;

    QString mCapturePath;
    int mSavedStdOut = -1;
    QStringList mCapturedOutput;

    // Redirects this process's standard output into a file of its own. Only the
    // feeds run inside such a window: an assertion made while it is open would
    // have its failure report captured instead of shown.
    void startCapture()
    {
        std::fflush(stdout);
        mSavedStdOut = dup(fileno(stdout));
        QVERIFY(mSavedStdOut != -1);
        const int captureFd = open(mCapturePath.toLocal8Bit().constData(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        QVERIFY(captureFd != -1);
        QVERIFY(dup2(captureFd, fileno(stdout)) != -1);
        close(captureFd);
    }

    void stopCapture()
    {
        std::fflush(stdout);
        QVERIFY(dup2(mSavedStdOut, fileno(stdout)) != -1);
        close(mSavedStdOut);
        mSavedStdOut = -1;

        QFile captured(mCapturePath);
        QVERIFY(captured.open(QIODevice::ReadOnly));
        const QString text = QString::fromUtf8(captured.readAll());
        captured.close();
        if (text.isEmpty()) {
            return;
        }
        QStringList lines = text.split(QChar::LineFeed);
        if (lines.constLast().isEmpty()) {
            lines.removeLast();
        }
        // Windows opens standard output in text mode, so the newline ending
        // each copied line reaches the file as a carriage return and a line
        // feed - the line ending a reader on that platform expects. What the
        // cases are about is what is on the line, so the separator comes off
        // here rather than being asserted on. Nothing else can leave a carriage
        // return at the end of one: cTelnet strips those the game sends and
        // TConsole::echo() those a script sends.
        for (QString& line : lines) {
            if (line.endsWith(QChar::CarriageReturn)) {
                line.chop(1);
            }
        }
        mCapturedOutput.append(lines);
    }

    // What --mirror wrote for this profile's main console, with the prefix taken
    // off, in the order it was written. Anything else the process may put on
    // standard output is not this test's business.
    QStringList mirroredLines() const
    {
        const QString prefix = qsl("%1.main| ").arg(mHostname);
        QStringList lines;
        for (const QString& line : mCapturedOutput) {
            if (line.startsWith(prefix)) {
                lines.append(line.mid(prefix.size()));
            }
        }
        return lines;
    }

    int timesMirrored(const QString& text) const { return static_cast<int>(mirroredLines().count(text)); }

    // Lua print() pads what it writes, so its line is matched loosely
    int timesMirroredContaining(const QString& text) const { return static_cast<int>(mirroredLines().filter(text).size()); }

    // What the console holds, minus the empty line a commit leaves ready for the
    // next one. Comparing this with mirroredLines() is the relation --mirror
    // promises: one copied line per line shown.
    QStringList shownLines() const
    {
        QStringList lines = mpHost->mpConsole->buffer.lineBuffer;
        if (!lines.isEmpty() && lines.constLast().isEmpty()) {
            lines.removeLast();
        }
        return lines;
    }

    void feedFromServer(const QByteArray& payload, const int waitMs)
    {
        startCapture();
        QByteArray data = payload;
        mpHost->mTelnet.loopbackTest(data);
        if (waitMs > 0) {
            QTest::qWait(waitMs);
        }
        stopCapture();
    }

    // cTelnet strips every carriage return the game sends, so the "\r\n" here
    // is not a second line terminator: what reaches TBuffer is "<payload>\n".
    void feedLineFromServer(const QByteArray& payload) { feedFromServer(payload + "\r\n", 50); }

    void waitWhileCapturing(const int waitMs)
    {
        startCapture();
        QTest::qWait(waitMs);
        stopCapture();
    }

    bool runLua(const QString& script)
    {
        startCapture();
        const bool succeeded = mpHost->getLuaInterpreter()->compileAndExecuteScript(script);
        QTest::qWait(50);
        stopCapture();
        return succeeded;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so a second copy of this test
        // running at the same time does not share the profile list.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mCapturePath = qsl("%1/mirrored-stdout.txt").arg(mConfigDir.path());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->setWelcomeMessage(mWelcomeMessage);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(3000)) {
            QFAIL("Could not connect with the host.");
        }

        // The stub sends its welcome message a tenth of a second after the
        // client connects, so wait for it here rather than let it land in the
        // middle of a case and be counted as a line that case fed.
        if (!QTest::qWaitFor(
                    [this]() {
                        return mpHost->mpConsole->buffer.lineBuffer.contains(mWelcomeMessage);
                    },
                    5000)) {
            QFAIL("The server stub's welcome message never reached the console.");
        }

        mSavedMirrorToStdOut = mudlet::smMirrorToStdOut;
        mSavedPostingTimeout = mpHost->mTelnet.getPostingTimeout();
    }

    void cleanupTestCase()
    {
        mudlet::smMirrorToStdOut = mSavedMirrorToStdOut;
        if (mpHost) {
            mpHost->mTelnet.setPostingTimeout(mSavedPostingTimeout);
        }
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        // Settle with the option off: a posting timer the previous case armed
        // has to be allowed to fire before anything is collected for this one,
        // and cTelnet only picks the shortened timeout up once it has.
        mudlet::smMirrorToStdOut = false;
        mpHost->mTelnet.setPostingTimeout(csmPostingTimeoutMs);
        QTest::qWait(350);
        mpHost->mBlankLineBehaviour = Host::BlankLineBehaviour::Show;
        mpHost->mpConsole->buffer.clear();
        mCapturedOutput.clear();
        mudlet::smMirrorToStdOut = true;
    }

    void cleanup() { mudlet::smMirrorToStdOut = false; }

    void test_gameTextIsMirrored()
    {
        feedLineFromServer("hello from the game");

        QCOMPARE(mirroredLines(), QStringList{qsl("hello from the game")});
        QCOMPARE(mirroredLines(), shownLines());
    }

    void test_everyGameLineOfOnePacketIsMirrored()
    {
        feedLineFromServer("first line\r\nsecond line\r\nthird line");

        QCOMPARE(mirroredLines(), QStringList({qsl("first line"), qsl("second line"), qsl("third line")}));
        QCOMPARE(mirroredLines(), shownLines());
    }

    // Half a line and the rest of it are one line, not two. No wait between the
    // two reads: cTelnet's posting timer would otherwise post the first half on
    // its own, which is a line boundary in its own right - see the case below.
    void test_aGameLineSplitAcrossReadsIsMirroredOnce()
    {
        feedFromServer("split over ", 0);
        feedLineFromServer("two reads");

        QCOMPARE(mirroredLines(), QStringList{qsl("split over two reads")});
        QCOMPARE(mirroredLines(), shownLines());
    }

    // A packet that ends mid-line and is then followed by silence: cTelnet's
    // posting timer commits what it has with a '\r', which is a line of its own
    // on screen and so one copied line, not a copy of a half line and then of
    // the whole of it.
    void test_aTimerFlushedFragmentIsMirroredOnceAsItsOwnLine()
    {
        feedFromServer("You are standing in a field. >", 6 * csmPostingTimeoutMs);

        QCOMPARE(mirroredLines(), QStringList{qsl("You are standing in a field. >")});

        feedLineFromServer("and now a new line");

        QCOMPARE(mirroredLines(), QStringList({qsl("You are standing in a field. >"), qsl("and now a new line")}));
        QCOMPARE(mirroredLines(), shownLines());
    }

    // Once a burst of text has been posted the timer still fires, with nothing
    // left to post; commitLineData() returns before the copy is made for that
    // lone '\r', so no empty line appears in the stream.
    void test_theEmptyTimerPostingIsNotMirrored()
    {
        feedLineFromServer("a complete line");
        waitWhileCapturing(6 * csmPostingTimeoutMs);

        QCOMPARE(mirroredLines(), QStringList{qsl("a complete line")});
        QCOMPARE(mirroredLines(), shownLines());
    }

    // A blank line the console shows is a blank line in the stream, which is
    // what keeps the copy line-for-line with the screen.
    void test_aBlankLineIsMirroredAsTheConsoleShowsIt()
    {
        feedLineFromServer("before the gap\r\n\r\nafter the gap");

        QCOMPARE(mirroredLines(), QStringList({qsl("before the gap"), QString(), qsl("after the gap")}));
        QCOMPARE(mirroredLines(), shownLines());
    }

    // ... and a blank line the profile is set to hide is shown by neither.
    void test_aHiddenBlankLineIsNotMirrored()
    {
        mpHost->mBlankLineBehaviour = Host::BlankLineBehaviour::Hide;

        feedLineFromServer("before the gap\r\n\r\nafter the gap");

        QCOMPARE(mirroredLines(), QStringList({qsl("before the gap"), qsl("after the gap")}));
        QCOMPARE(mirroredLines(), shownLines());
    }

    // The point of the stream is that it can be read: what is copied is the
    // line after the escape sequences have been taken out of it, not the bytes
    // the game sent.
    void test_colouredGameTextIsMirroredAsPlainText()
    {
        feedLineFromServer("\033[1;31mred from the game\033[0m");

        QCOMPARE(mirroredLines(), QStringList{qsl("red from the game")});
        QCOMPARE(timesMirroredContaining(QChar(0x1B)), 0);
        QCOMPARE(mirroredLines(), shownLines());
    }

    // print() output is copied by the print path and never reaches the commit
    // path, so the count stays at one whatever the commit path does. Kept as
    // the guard for a future move of the copy to after runTriggers(), where the
    // committed line could pick up text the print path has already copied.
    void test_scriptOutputIsNotMirroredTwice()
    {
        QVERIFY(runLua(qsl("print(\"printed by a script\")")));
        feedLineFromServer("game text after the script");

        QCOMPARE(timesMirroredContaining(qsl("printed by a script")), 1);
        QCOMPARE(timesMirrored(qsl("game text after the script")), 1);
    }

    // print() hands the console its text and its newline as two separate
    // writes, and can hand over several lines at once; the stream gets one
    // prefixed line per line shown either way.
    void test_printOutputIsMirroredOneLinePerLineShown()
    {
        QVERIFY(runLua(qsl("print(\"first printed line\\nsecond printed line\")")));

        QCOMPARE(mirroredLines().size(), 2);
        QCOMPARE(mirroredLines().at(0), qsl("first printed line"));
        QVERIFY(mirroredLines().at(1).startsWith(qsl("second printed line")));
    }

    // echo() can end a line and leave blank ones behind it. They are lines on
    // the screen, so they are lines in the stream: a copy that stopped at the
    // last line with text on it would no longer be line for line with it.
    void test_trailingBlankLinesFromAScriptAreMirrored()
    {
        QVERIFY(runLua(qsl("echo(\"text and then a gap\\n\\n\")")));

        QCOMPARE(mirroredLines(), QStringList({qsl("text and then a gap"), QString()}));
        QCOMPARE(mirroredLines(), shownLines());
    }

    // A line the print path has left unfinished is one line on the screen until
    // something ends it, so it is one line in the stream as well - which is how
    // Mudlet's own startup message, written in two pieces, reaches it.
    void test_aLineEchoedInPiecesIsMirroredOnce()
    {
        QVERIFY(runLua(qsl("echo(\"first half, \") echo(\"second half\\n\")")));

        QCOMPARE(mirroredLines(), QStringList{qsl("first half, second half")});
        QCOMPARE(mirroredLines(), shownLines());
    }

    // Console names come from Lua, which takes any string at all. A line feed
    // in one would split the record it prefixes over two lines and leave a
    // reader unable to say which console the second half came from.
    void test_aConsoleNameWithALineFeedDoesNotSplitARecord()
    {
        QVERIFY(runLua(qsl("createMiniConsole(\"split\\nme\", 0, 0, 200, 100) echo(\"split\\nme\", \"in the oddly named console\\n\")")));

        const QStringList carryingTheText = mCapturedOutput.filter(qsl("in the oddly named console"));
        QCOMPARE(carryingTheText, QStringList{qsl("%1.split\uFFFDme| in the oddly named console").arg(mHostname)});
    }

    // feedTriggers() puts text on the same commit path without a server behind
    // it, so it is copied too.
    void test_locallyFedTextIsMirrored()
    {
        QVERIFY(runLua(qsl("feedTriggers(\"fed to the triggers\\n\")")));

        QCOMPARE(timesMirrored(qsl("fed to the triggers")), 1);
    }

    void test_nothingIsMirroredWithoutTheOption()
    {
        mudlet::smMirrorToStdOut = false;

        feedLineFromServer("this run was not asked for");
        QVERIFY(runLua(qsl("print(\"neither was this\")")));

        QCOMPARE(timesMirroredContaining(qsl("this run was not asked for")), 0);
        QCOMPARE(timesMirroredContaining(qsl("neither was this")), 0);

        // ... and the feed itself was alive throughout, so the two counts above
        // are the option being off rather than nothing having been fed.
        mudlet::smMirrorToStdOut = true;
        feedLineFromServer("this run was asked for");

        QCOMPARE(timesMirrored(qsl("this run was asked for")), 1);
    }

    // The third blank-line setting turns the line into a single space, and the
    // copy follows what the console was given. That setting also turns the
    // posting timer's empty flush into a line of its own, which is why the two
    // sides are compared rather than a line count being asserted.
    void test_aBlankLineReplacedWithASpaceIsMirroredAsOne()
    {
        mpHost->mBlankLineBehaviour = Host::BlankLineBehaviour::ReplaceWithSpace;

        feedLineFromServer("before the gap\r\n\r\nafter the gap");

        QCOMPARE(timesMirrored(qsl("before the gap")), 1);
        QCOMPARE(timesMirrored(qsl("after the gap")), 1);
        QVERIFY(mirroredLines().contains(qsl(" ")));
        QCOMPARE(mirroredLines(), shownLines());
    }

    // The stream carries UTF-8 whatever the terminal it lands in is set to.
    void test_nonAsciiGameTextIsMirroredIntact()
    {
        const QPair<bool, QString> encodingSet = mpHost->mTelnet.setEncoding("UTF-8", false);
        QVERIFY2(encodingSet.first, qPrintable(encodingSet.second));

        feedLineFromServer(qsl("caf\u00E9 \u65E5\u672C\u8A9E").toUtf8());

        QCOMPARE(mirroredLines(), QStringList{qsl("caf\u00E9 \u65E5\u672C\u8A9E")});
        QCOMPARE(mirroredLines(), shownLines());
    }

    // Whatever the buffer makes of a control character the game sent, the copy
    // says the same as the console rather than something of its own.
    void test_aControlCharacterIsMirroredAsTheConsoleHoldsIt()
    {
        feedLineFromServer("bell\aand on");

        QCOMPARE(mirroredLines().size(), 1);
        QCOMPARE(mirroredLines(), shownLines());
    }

    // A line a trigger gags is copied all the same: the copy is made when the
    // line arrives, before the trigger runs. Pins the trade-off that placement
    // makes, so that moving the copy cannot change it unnoticed.
    void test_aGaggedLineIsStillMirrored()
    {
        QVERIFY(runLua(qsl("tempRegexTrigger([[^gag this line$]], [[deleteLine()]])")));

        feedLineFromServer("gag this line");

        QCOMPARE(timesMirrored(qsl("gag this line")), 1);
        QVERIFY(!shownLines().contains(qsl("gag this line")));
    }

    // Text a trigger adds with echo() is put into the line by
    // TBuffer::insertInLine(), which no copy is made from, so the line is
    // copied as the game sent it. Pins the one hole left in the option.
    void test_textATriggerEchoesIsNotMirrored()
    {
        QVERIFY(runLua(qsl("tempRegexTrigger([[^echo onto this line$]], [[echo(\" [added]\")]])")));

        feedLineFromServer("echo onto this line");

        QCOMPARE(timesMirrored(qsl("echo onto this line")), 1);
        QCOMPARE(timesMirroredContaining(qsl("[added]")), 0);
        QCOMPARE(shownLines().filter(qsl("[added]")).size(), 1);
    }

    // A prompt is ended by IAC GA rather than by a newline, which is a line
    // boundary of its own in the commit path. Last, because the first GA seen
    // switches cTelnet over to posting on every read.
    void test_aPromptTerminatedByGoAheadIsMirrored()
    {
        QByteArray prompt("HP:100 MP:50 > ");
        prompt.append(static_cast<char>(0xFF)); // TN_IAC
        prompt.append(static_cast<char>(0xF9)); // TN_GA
        feedFromServer(prompt, 50);

        QCOMPARE(timesMirrored(qsl("HP:100 MP:50 > ")), 1);
        QCOMPARE(mirroredLines(), shownLines());
    }
};

#include "MirrorToStdOutTest.moc"
MUDLET_GROUPED_TEST_MAIN(MirrorToStdOutTest)
