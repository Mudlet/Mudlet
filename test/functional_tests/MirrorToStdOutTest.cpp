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

using namespace std::chrono_literals;

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
        // TConsoleModel::echo() those a script sends.
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
    // promises: one copied line per line shown, except that a game line a
    // trigger writes into is copied as sent and what the trigger wrote follows.
    QStringList shownLines() const
    {
        QStringList lines = mpHost->mainConsoleView()->buffer.lineBuffer;
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
        QTest::qWait(50ms);
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
        if (!connected.wait(3s)) {
            QFAIL("Could not connect with the host.");
        }

        // The stub sends its welcome message a tenth of a second after the
        // client connects, so wait for it here rather than let it land in the
        // middle of a case and be counted as a line that case fed.
        if (!QTest::qWaitFor(
                    [this]() {
                        return mpHost->mainConsoleView()->buffer.lineBuffer.contains(mWelcomeMessage);
                    },
                    5s)) {
            QFAIL("The server stub's welcome message never reached the console.");
        }

        mSavedMirrorToStdOut = MudletApp::smMirrorToStdOut;
        mSavedPostingTimeout = mpHost->mTelnet.getPostingTimeout();
    }

    void cleanupTestCase()
    {
        MudletApp::smMirrorToStdOut = mSavedMirrorToStdOut;
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
        QVERIFY(mpHost->mainConsoleView());
        // Settle with the option off: a posting timer the previous case armed
        // has to be allowed to fire before anything is collected for this one,
        // and cTelnet only picks the shortened timeout up once it has.
        MudletApp::smMirrorToStdOut = false;
        mpHost->mTelnet.setPostingTimeout(csmPostingTimeoutMs);
        QTest::qWait(350ms);
        mpHost->mBlankLineBehaviour = Host::BlankLineBehaviour::Show;
        mpHost->mainConsoleView()->buffer.clear();
        mCapturedOutput.clear();
        MudletApp::smMirrorToStdOut = true;
    }

    void cleanup() { MudletApp::smMirrorToStdOut = false; }

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

    // A game line lands below a line the print path left unfinished, so the
    // unfinished one is written out first, and client output after the game
    // line follows it.
    void test_anUnfinishedEchoIsMirroredAheadOfTheGameLineAfterIt()
    {
        QVERIFY(runLua(qsl("echo(\"left unfinished\")")));
        feedLineFromServer("game line below it");
        QVERIFY(runLua(qsl("echo(\"after the game line\\n\")")));

        QCOMPARE(mirroredLines(), QStringList({qsl("left unfinished"), qsl("game line below it"), qsl("after the game line")}));
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
        MudletApp::smMirrorToStdOut = false;

        feedLineFromServer("this run was not asked for");
        QVERIFY(runLua(qsl("print(\"neither was this\")")));

        QCOMPARE(timesMirroredContaining(qsl("this run was not asked for")), 0);
        QCOMPARE(timesMirroredContaining(qsl("neither was this")), 0);

        // ... and the feed itself was alive throughout, so the two counts above
        // are the option being off rather than nothing having been fed.
        MudletApp::smMirrorToStdOut = true;
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

    // The line is copied as the game sent it before the trigger runs, so what a
    // trigger writes into it follows as a line of its own once the pass is over.
    void test_textATriggerEchoesIsMirroredAfterTheLine()
    {
        QVERIFY(runLua(qsl("tempRegexTrigger([[^echo onto this line$]], [[echo(\" [added]\") cecho(\" <red>[cechoed]\")]])")));

        feedLineFromServer("echo onto this line");

        QCOMPARE(mirroredLines(), QStringList({qsl("echo onto this line"), qsl(" [added] [cechoed]")}));
        QCOMPARE(shownLines(), QStringList{qsl("echo onto this line [added] [cechoed]")});
    }

    // A line feed in a trigger's echo starts a line on screen, so it starts one
    // in the stream too, and text after the trigger is not run onto it.
    void test_linesATriggerEchoesAreMirroredOneLinePerLineShown()
    {
        QVERIFY(runLua(qsl("tempRegexTrigger([[^echo below this line$]], [[echo(\"\\nfirst below\\n\\nsecond below\")]])")));

        feedLineFromServer("echo below this line");
        QVERIFY(runLua(qsl("echo(\"after the trigger\\n\")")));

        const QStringList expected{qsl("echo below this line"), qsl("first below"), QString(), qsl("second below"), qsl("after the trigger")};
        QCOMPARE(mirroredLines(), expected);
        QCOMPARE(shownLines(), expected);
    }

    // Every way a trigger writes into its line is held and follows the line as
    // sent, in the order written and ahead of the next line of the same packet.
    // The inserts go in at the start of the line and the link at its end, so
    // each is a record of its own rather than run onto the others.
    void test_everyTriggerWriteIsMirroredInOrderBeforeTheNextLine()
    {
        QVERIFY(runLua(qsl("tempRegexTrigger([[^write into this line$]], "
                           "[[echoLink(\" [link]\", \"\", \"\") insertLink(\" [inserted link]\", \"\", \"\") "
                           "insertText(\" [inserted]\") send(\"a command\", true)]])")));

        feedLineFromServer("write into this line\r\nthe next line");

        QCOMPARE(mirroredLines(), QStringList({qsl("write into this line"), qsl(" [link]"), qsl(" [inserted link]"), qsl(" [inserted]"), qsl("a command"), qsl("the next line")}));
    }

    // Text put in ahead of the line and text added after it are apart on
    // screen, so they are not one record.
    void test_textATriggerPutsAtBothEndsOfItsLineIsMirroredApart()
    {
        QVERIFY(runLua(qsl("mirrorTrigger = tempRegexTrigger([[^A goblin is here$]], [[insertText(\"[!] \") echo(\" (hostile)\")]])")));

        feedLineFromServer("A goblin is here");
        QVERIFY(runLua(qsl("killTrigger(mirrorTrigger)")));

        QCOMPARE(mirroredLines(), QStringList({qsl("A goblin is here"), qsl("[!] "), qsl(" (hostile)")}));
        QCOMPARE(shownLines(), QStringList{qsl("[!] A goblin is here (hostile)")});
    }

    // The line is gone from the screen, so what the trigger wrote into it is
    // not copied, though the line itself was copied when it arrived.
    void test_textATriggerEchoesIntoALineItGagsIsNotMirrored()
    {
        QVERIFY(runLua(qsl("mirrorTrigger = tempRegexTrigger([[^echo into then gag$]], [[echo(\" [echoed]\") deleteLine()]])")));

        feedLineFromServer("before the gag\r\necho into then gag\r\nafter the gag");
        QVERIFY(runLua(qsl("killTrigger(mirrorTrigger)")));

        QCOMPARE(mirroredLines(), QStringList({qsl("before the gag"), qsl("echo into then gag"), qsl("after the gag")}));
        QCOMPARE(shownLines(), QStringList({qsl("before the gag"), qsl("after the gag")}));
    }

    // The held text goes out ahead of the sub-console's line in two parts, and
    // the blank row between them is still a row of the stream.
    void test_aBlankRowATriggerEchoesIsMirroredAcrossASubConsoleWrite()
    {
        QVERIFY(runLua(qsl("createMiniConsole(\"mirrorChat\", 0, 0, 200, 100) "
                           "mirrorTrigger = tempRegexTrigger([[^blank row around a write$]], [[echo(\"a\\n\") echo(\"mirrorChat\", \"elsewhere\\n\") echo(\"\\nb\")]])")));

        feedLineFromServer("blank row around a write");
        QVERIFY(runLua(qsl("killTrigger(mirrorTrigger)")));

        QCOMPARE(mirroredLines(), QStringList({qsl("blank row around a write"), qsl("a"), QString(), qsl("b")}));
        QCOMPARE(shownLines(), QStringList({qsl("blank row around a writea"), QString(), qsl("b")}));
    }

    void test_aBlankRowATriggerInsertsIsMirrored()
    {
        QVERIFY(runLua(qsl("mirrorTrigger = tempRegexTrigger([[^insert a row above$]], [[insertText(\"\\n\")]])")));

        feedLineFromServer("insert a row above");
        QVERIFY(runLua(qsl("killTrigger(mirrorTrigger)")));

        QCOMPARE(mirroredLines(), QStringList({qsl("insert a row above"), QString()}));
        QCOMPARE(shownLines(), QStringList({QString(), qsl("insert a row above")}));
    }

    // The console caps one write and drops a command it handles itself, and
    // the copy is of what it kept.
    void test_onlyWhatTheConsoleKeepsOfATriggerWriteIsMirrored()
    {
        QVERIFY(runLua(qsl("mirrorTrigger = tempRegexTrigger([[^echo too much$]], [[echo(string.rep(\"x\", 1000010))]])")));
        feedLineFromServer("echo too much");
        QVERIFY(runLua(qsl("killTrigger(mirrorTrigger)")));

        QCOMPARE(mirroredLines().size(), 2);
        QCOMPARE(mirroredLines().at(1), QString(1000000, QChar('x')));

        QVERIFY(runLua(qsl("mirrorTrigger = tempRegexTrigger([[^ask for the docs$]], [[send(\"!osc8-docs\", true)]])")));
        feedLineFromServer("ask for the docs");
        QVERIFY(runLua(qsl("killTrigger(mirrorTrigger)")));

        QVERIFY(!shownLines().contains(qsl("!osc8-docs")));
        QCOMPARE(timesMirrored(qsl("!osc8-docs")), 0);
    }

    void test_textATriggerReplacesOrCopiesIntoItsLineIsMirrored()
    {
        QVERIFY(runLua(qsl("mirrorTrigger = tempRegexTrigger([[^replace the end$]], [[selectSection(15, 0) replace(\" [tag]\")]])")));
        feedLineFromServer("replace the end");
        QVERIFY(runLua(qsl("killTrigger(mirrorTrigger)")));

        QCOMPARE(mirroredLines(), QStringList({qsl("replace the end"), qsl(" [tag]")}));
        QCOMPARE(shownLines(), QStringList{qsl("replace the end [tag]")});

        mpHost->mainConsoleView()->buffer.clear();
        mCapturedOutput.clear();
        QVERIFY(runLua(qsl("mirrorTrigger = tempRegexTrigger([[^append a copy$]], [[selectCurrentLine() copy() appendBuffer()]])")));
        feedLineFromServer("append a copy");
        QVERIFY(runLua(qsl("killTrigger(mirrorTrigger)")));

        QCOMPARE(mirroredLines(), QStringList({qsl("append a copy"), qsl("append a copy")}));
        QCOMPARE(shownLines(), QStringList{qsl("append a copyappend a copy")});

        mpHost->mainConsoleView()->buffer.clear();
        mCapturedOutput.clear();
        QVERIFY(runLua(qsl("mirrorTrigger = tempRegexTrigger([[^paste a copy above$]], [[selectCurrentLine() copy() moveCursor(0, getLineNumber() - 1) paste()]])")));
        feedLineFromServer("the line above\r\npaste a copy above");
        QVERIFY(runLua(qsl("killTrigger(mirrorTrigger)")));

        QCOMPARE(mirroredLines(), QStringList({qsl("the line above"), qsl("paste a copy above"), qsl("paste a copy above")}));
        QCOMPARE(shownLines(), QStringList({qsl("paste a copy abovethe line above"), qsl("paste a copy above")}));
    }

    // feedTriggers() commits its line in the middle of the trigger pass, below
    // the line the trigger wrote into, so the held text goes out ahead of it.
    void test_triggerTextIsMirroredAheadOfALineTheTriggerFeeds()
    {
        QVERIFY(runLua(qsl("mirrorTrigger = tempRegexTrigger([[^echo then feed$]], [[echo(\" [added]\") feedTriggers(\"a fed line\\n\")]])")));

        feedLineFromServer("echo then feed");
        QVERIFY(runLua(qsl("killTrigger(mirrorTrigger)")));

        QCOMPARE(mirroredLines(), QStringList({qsl("echo then feed"), qsl(" [added]"), qsl("a fed line")}));
        QCOMPARE(shownLines(), QStringList({qsl("echo then feed [added]"), qsl("a fed line")}));
    }

    void test_textATriggerEchoesBeforeClearingTheWindowIsNotMirrored()
    {
        QVERIFY(runLua(qsl("mirrorTrigger = tempRegexTrigger([[^echo then clear$]], [[echo(\" [cleared]\") clearWindow()]])")));

        feedLineFromServer("echo then clear");
        QVERIFY(runLua(qsl("killTrigger(mirrorTrigger)")));

        QCOMPARE(mirroredLines(), QStringList{qsl("echo then clear")});
        QCOMPARE(timesMirroredContaining(qsl("[cleared]")), 0);
    }

    void test_printInATriggerIsMirroredAfterTheLine()
    {
        QVERIFY(runLua(qsl("mirrorTrigger = tempRegexTrigger([[^print from a trigger$]], [[print(\"printed by the trigger\")]])")));

        feedLineFromServer("print from a trigger");
        QVERIFY(runLua(qsl("killTrigger(mirrorTrigger)")));

        QCOMPARE(mirroredLines().size(), 2);
        QCOMPARE(mirroredLines().at(0), qsl("print from a trigger"));
        QVERIFY(mirroredLines().at(1).startsWith(qsl("printed by the trigger")));
    }

    // A sub-console is not in trigger mode, so what a trigger writes to it is
    // copied at once; the main console's held text has to go out ahead of it.
    void test_triggerTextIsMirroredAheadOfASubConsoleLineWrittenAfterIt()
    {
        QVERIFY(runLua(qsl("createMiniConsole(\"mirrorChat\", 0, 0, 200, 100) "
                           "tempRegexTrigger([[^echo then write elsewhere$]], [[echo(\" [added]\") echo(\"mirrorChat\", \"elsewhere\\n\")]])")));

        feedLineFromServer("echo then write elsewhere");

        const QString main = qsl("%1.main| ").arg(mHostname);
        const QStringList expected{main + qsl("echo then write elsewhere"), main + qsl(" [added]"), qsl("%1.mirrorChat| elsewhere").arg(mHostname)};
        QCOMPARE(mCapturedOutput.filter(QRegularExpression(qsl("write elsewhere|\\[added\\]|\\| elsewhere$"))), expected);
    }

    // A sysBufferShrinkEvent handler runs in trigger mode but after the trigger
    // pass, so what it echoes must not wait for more game text to be copied.
    void test_textEchoedAfterTheTriggerPassIsMirroredWithoutMoreOutput()
    {
        TBuffer& buffer = mpHost->mainConsoleView()->buffer;
        const int savedLinesLimit = buffer.mLinesLimit;
        const int savedBatchDeleteSize = buffer.mBatchDeleteSize;
        auto restore = qScopeGuard([&buffer, savedLinesLimit, savedBatchDeleteSize]() {
            buffer.setBufferSize(savedLinesLimit, savedBatchDeleteSize);
        });
        buffer.setBufferSize(100, 10);
        QVERIFY(runLua(qsl("mirrorShrinkHandler = registerAnonymousEventHandler(\"sysBufferShrinkEvent\", function() echo(\"buffer trimmed\") end, true)")));

        QByteArray bulk;
        for (int i = 0; i < 90; ++i) {
            bulk.append(qsl("filler %1\r\n").arg(i).toLatin1());
        }
        feedFromServer(bulk, 50);
        for (int i = 0; i < 30 && !shownLines().contains(qsl("buffer trimmed")); ++i) {
            feedLineFromServer(qsl("one more %1").arg(i).toLatin1());
        }
        QVERIFY(runLua(qsl("killAnonymousEventHandler(mirrorShrinkHandler)")));

        QVERIFY(shownLines().contains(qsl("buffer trimmed")));
        QCOMPARE(timesMirrored(qsl("buffer trimmed")), 1);
    }

    // Once a write fails --mirror is off, and the rest of the same batch of
    // lines must neither be written nor warn again.
    void test_aFailedWriteTurnsMirroringOffWithOneWarning()
    {
        std::fflush(stdout);
        const int savedStdOut = dup(fileno(stdout));
        QVERIFY(savedStdOut != -1);
        const int fullFd = open("/dev/full", O_WRONLY);
        if (fullFd == -1) {
            close(savedStdOut);
            QSKIP("no /dev/full to make standard output fail");
        }
        const bool redirected = dup2(fullFd, fileno(stdout)) != -1;
        close(fullFd);
        if (!redirected) {
            close(savedStdOut);
            QFAIL("could not point standard output at /dev/full");
        }
        // Installed only once nothing can return early, as it would swallow the
        // warning for every case after this one
        static int failedWriteWarnings = 0;
        static QtMessageHandler previousHandler = nullptr;
        failedWriteWarnings = 0;
        previousHandler = qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& message) {
            if (message.startsWith(qsl("--mirror: could not write"))) {
                ++failedWriteWarnings;
                return;
            }
            previousHandler(type, context, message);
        });

        const bool succeeded = mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("echo(\"first\\nsecond\\nthird\\n\")"));

        std::fflush(stdout);
        dup2(savedStdOut, fileno(stdout));
        close(savedStdOut);
        clearerr(stdout);
        qInstallMessageHandler(previousHandler);
        QVERIFY(succeeded);
        QVERIFY(!MudletApp::smMirrorToStdOut);
        QCOMPARE(failedWriteWarnings, 1);
    }

    void test_echoLinkIsMirrored()
    {
        QVERIFY(runLua(qsl("echoLink(\"a link\", [[echo('clicked')]], \"hint\") echo(\" after it\\n\")")));

        QCOMPARE(mirroredLines(), QStringList{qsl("a link after it")});
        QCOMPARE(mirroredLines(), shownLines());
    }

    void test_echoLinkToASubConsoleIsMirroredOnce()
    {
        QVERIFY(runLua(qsl("createMiniConsole(\"mirrorChat\", 0, 0, 200, 100) echoLink(\"mirrorChat\", \"a sub-console link\", \"\", \"\") echo(\"mirrorChat\", \"\\n\")")));

        QCOMPARE(mCapturedOutput.filter(qsl("a sub-console link")), QStringList{qsl("%1.mirrorChat| a sub-console link").arg(mHostname)});
    }

    // Outside a trigger these append to the buffer just as echo() and echoLink() do
    void test_textInsertedAtTheEndOfTheBufferIsMirrored()
    {
        QVERIFY(runLua(qsl("moveCursorEnd(\"main\") insertText(\"inserted\") echo(\" and after\\n\") "
                           "moveCursorEnd(\"main\") insertLink(\"an inserted link\", \"\", \"\") echo(\" and after\\n\")")));

        QCOMPARE(mirroredLines(), QStringList({qsl("inserted and after"), qsl("an inserted link and after")}));
        QCOMPARE(mirroredLines(), shownLines());
    }

    void test_textAppendedFromTheClipboardIsMirrored()
    {
        QVERIFY(runLua(qsl("echo(\"copied line\\n\") moveCursor(0, getLineNumber() - 1) selectCurrentLine() copy() moveCursorEnd() appendBuffer()")));

        QCOMPARE(mirroredLines(), QStringList({qsl("copied line"), qsl("copied line")}));
        QCOMPARE(mirroredLines(), shownLines());
    }

    // The buffer is trimmed while print() appends, so what the shrink handler
    // echoes is on screen below the line print() ended and goes out after it.
    void test_textEchoedWhilePrintTrimsTheBufferIsMirroredAfterIt()
    {
        TBuffer& buffer = mpHost->mainConsoleView()->buffer;
        const int savedLinesLimit = buffer.mLinesLimit;
        const int savedBatchDeleteSize = buffer.mBatchDeleteSize;
        const bool savedEchoLuaErrors = mpHost->mEchoLuaErrors;
        auto restore = qScopeGuard([this, &buffer, savedLinesLimit, savedBatchDeleteSize, savedEchoLuaErrors]() {
            buffer.setBufferSize(savedLinesLimit, savedBatchDeleteSize);
            mpHost->mEchoLuaErrors = savedEchoLuaErrors;
        });
        mpHost->mEchoLuaErrors = true;
        buffer.setBufferSize(100, 10);
        for (int i = 0; i < 400 && static_cast<int>(buffer.size()) != buffer.mLinesLimit; ++i) {
            feedLineFromServer(qsl("filler %1").arg(i).toLatin1());
        }
        QCOMPARE(static_cast<int>(buffer.size()), buffer.mLinesLimit);
        QVERIFY(runLua(qsl("mirrorShrinkHandler = registerAnonymousEventHandler(\"sysBufferShrinkEvent\", function() echo(\"buffer trimmed\") end, true) "
                           "mirrorTrigger = tempRegexTrigger([[^print an error$]], [[printError(\"printed error\")]])")));
        mCapturedOutput.clear();

        feedLineFromServer("print an error");
        QVERIFY(runLua(qsl("killAnonymousEventHandler(mirrorShrinkHandler) killTrigger(mirrorTrigger)")));

        const QStringList mirrored = mirroredLines();
        const qsizetype trimmed = mirrored.indexOf(qsl("buffer trimmed"));
        QVERIFY2(trimmed > 1, qPrintable(mirrored.join(QChar::LineFeed)));
        QCOMPARE(mirrored.at(0), qsl("print an error"));
        // the line feed printError() writes first, which ends the line the handler then echoes onto
        QCOMPARE(mirrored.at(trimmed - 1), QString());
        QVERIFY(shownLines().filter(qsl("buffer trimmed")).constFirst().startsWith(qsl("buffer trimmed")));
    }

    // A prompt is ended by IAC GA rather than by a newline, which is a line
    // boundary of its own in the commit path. The cases with a GA come last,
    // because the first GA seen switches cTelnet over to posting on every read.
    void test_aPromptTerminatedByGoAheadIsMirrored()
    {
        QByteArray prompt("HP:100 MP:50 > ");
        prompt.append(static_cast<char>(0xFF)); // TN_IAC
        prompt.append(static_cast<char>(0xF9)); // TN_GA
        feedFromServer(prompt, 50);

        QCOMPARE(timesMirrored(qsl("HP:100 MP:50 > ")), 1);
        QCOMPARE(mirroredLines(), shownLines());
    }

    // A command typed at a prompt is written onto the prompt's line, which was
    // copied when it arrived, so the command is copied as a line of its own.
    void test_aCommandTypedAtAPromptIsMirrored()
    {
        QByteArray prompt("HP:90 MP:40 > ");
        prompt.append(static_cast<char>(0xFF)); // TN_IAC
        prompt.append(static_cast<char>(0xF9)); // TN_GA
        feedFromServer(prompt, 50);
        QVERIFY(mpHost->mainConsoleView()->buffer.promptBuffer.at(mpHost->mainConsoleView()->buffer.size() - 2));

        startCapture();
        mpHost->send(qsl("look at the prompt"), true, true);
        stopCapture();

        QCOMPARE(mirroredLines(), QStringList({qsl("HP:90 MP:40 > "), qsl("look at the prompt")}));
        QCOMPARE(shownLines(), QStringList{qsl("HP:90 MP:40 > look at the prompt")});
    }

    // Enter on an empty command line at a prompt adds nothing on screen, so it
    // adds nothing to the stream either.
    void test_anEmptyCommandAtAPromptIsNotMirrored()
    {
        QByteArray prompt("HP:80 MP:30 > ");
        prompt.append(static_cast<char>(0xFF)); // TN_IAC
        prompt.append(static_cast<char>(0xF9)); // TN_GA
        feedFromServer(prompt, 50);
        QVERIFY(mpHost->mainConsoleView()->buffer.promptBuffer.at(mpHost->mainConsoleView()->buffer.size() - 2));

        startCapture();
        mpHost->send(QString(), true, true);
        stopCapture();

        QCOMPARE(mirroredLines(), QStringList{qsl("HP:80 MP:30 > ")});
    }

    // Script text still open below the prompt stays below the command in the
    // stream, as it does on screen, and is copied once something ends it.
    void test_aCommandAtAPromptIsMirroredAheadOfOpenScriptText()
    {
        QByteArray prompt("HP:70 MP:20 > ");
        prompt.append(static_cast<char>(0xFF)); // TN_IAC
        prompt.append(static_cast<char>(0xF9)); // TN_GA
        feedFromServer(prompt, 50);
        QVERIFY(runLua(qsl("echo(\"still open\")")));

        startCapture();
        mpHost->send(qsl("look below"), true, true);
        stopCapture();
        QVERIFY(runLua(qsl("echo(\"\\n\")")));

        QCOMPARE(mirroredLines(), QStringList({qsl("HP:70 MP:20 > "), qsl("look below"), qsl("still open")}));
        QCOMPARE(shownLines(), QStringList({qsl("HP:70 MP:20 > look below"), qsl("still open")}));
    }
};

#include "MirrorToStdOutTest.moc"
MUDLET_GROUPED_TEST_MAIN(MirrorToStdOutTest)
