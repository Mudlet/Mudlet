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

#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResources();

// TBuffer::wrapLine() rebuilds every line from its start line to the end of the
// buffer, and its callers rely on more than the text coming back out intact:
// the count it returns positions the user cursor and the repaint range, and the
// timestamps and prompt flags it carries over decide what the timestamp column
// and TConsole::printCommand() do next. These lock all of that down.
class WrapLineRewrapTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Test-WrapLineRewrap");
    QString mPort; // assigned the stub's actual loopback port in init()
    const QString mLocalhost = qsl("localhost");
    const QString mMiniConsole = qsl("rewrapTest");
    // U+6F22 U+5B57 - East Asian Wide, so two columns are needed per glyph
    const QString mWideText = QString(QChar(0x6F22)) + QChar(0x5B57);

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        // Port 0 asks the OS for an ephemeral port so parallel test runs do
        // not collide on a hardcoded one
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), "TelnetServerStub failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    void cleanup()
    {
        const QString profilePath = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }

    // The count is not "lines added": it is every line in the rewrapped range
    // bar one, whether that line was split or not. TConsole::insertLink() moves
    // the user cursor down by it, so a line that needed no wrapping still has
    // to be counted.
    void test_rewrapCountsEveryLineInItsRange()
    {
        auto* console = consoleWithWrapWidth(200);
        QVERIFY(console);
        echo(qsl("first line\\n"));
        echo(qsl("second line\\n"));
        echo(qsl("third line\\n"));

        // three lines of text plus the empty one the buffer keeps ready
        QCOMPARE(console->buffer.lineBuffer.size(), 4);
        QVERIFY(console->buffer.lineBuffer.constLast().isEmpty());
        QCOMPARE(console->buffer.wrapLine(0, 200, 0, 0), 3);
        QCOMPARE(joinedText(console), qsl("first linesecond linethird line"));

        // and again with text in that last line, so the range ends on a line
        // that needs no wrapping either
        echo(qsl("fourth line"));
        QVERIFY(!console->buffer.lineBuffer.constLast().isEmpty());
        QCOMPARE(console->buffer.wrapLine(0, 200, 0, 0), 3);
        QCOMPARE(joinedText(console), qsl("first linesecond linethird linefourth line"));
    }

    // A line that fits has to come back with its own timestamp and prompt flag,
    // not a freshly stamped and unflagged replacement.
    void test_aLineThatFitsKeepsItsTimestampAndPromptFlag()
    {
        auto* console = consoleWithWrapWidth(200);
        QVERIFY(console);
        echo(qsl("a line that fits\\n"));
        QCOMPARE(console->buffer.lineBuffer.size(), 2);

        console->buffer.timeBuffer[0] = qsl("11:22:33.444");
        console->buffer.promptBuffer[0] = true;

        console->buffer.wrapLine(0, 200, 0, 0);

        QCOMPARE(console->buffer.line(0), qsl("a line that fits"));
        QCOMPARE(console->buffer.timeBuffer.at(0), qsl("11:22:33.444"));
        QVERIFY(console->buffer.promptBuffer.at(0));
    }

    // Continuation lines are marked by a blank timestamp, which is what tells a
    // later rewrap not to indent them as though they started a line.
    void test_aWrappedLineIsSplitAndItsContinuationsAreBlankStamped()
    {
        auto* console = consoleWithWrapWidth(10);
        QVERIFY(console);
        echo(qsl("abcdefghijklmnopqrstuvwxyz\\n"));

        QCOMPARE(joinedText(console), qsl("abcdefghijklmnopqrstuvwxyz"));
        QCOMPARE(nonEmptyLineCount(console), 3);
        QCOMPARE(console->buffer.line(0), qsl("abcdefghij"));
        QVERIFY2(console->buffer.timeBuffer.at(0) != mudlet::smBlankTimeStamp, "the line that starts the paragraph lost its timestamp");
        QCOMPARE(console->buffer.timeBuffer.at(1), mudlet::smBlankTimeStamp);
        QCOMPARE(console->buffer.timeBuffer.at(2), mudlet::smBlankTimeStamp);
    }

    // The shortcut stops at a blank line rather than keeping it: the rewrap
    // replaces that one through appendEmptyLine(), so it comes back with a
    // fresh timestamp and no prompt flag while its neighbours stay put.
    void test_blankLinesBetweenTextSurviveARewrap()
    {
        auto* console = consoleWithWrapWidth(200);
        QVERIFY(console);
        echo(qsl("above\\n\\nbelow\\n"));

        QCOMPARE(console->buffer.line(0), qsl("above"));
        QVERIFY(console->buffer.line(1).isEmpty());
        QCOMPARE(console->buffer.line(2), qsl("below"));

        console->buffer.promptBuffer[1] = true;
        console->buffer.timeBuffer[1] = qsl("sentinel");

        QCOMPARE(console->buffer.wrapLine(0, 200, 0, 0), 3);

        QCOMPARE(console->buffer.line(0), qsl("above"));
        QVERIFY(console->buffer.line(1).isEmpty());
        QCOMPARE(console->buffer.line(2), qsl("below"));
        QVERIFY2(!console->buffer.promptBuffer.at(1), "the replacement blank line kept the old one's prompt flag");
        QVERIFY2(console->buffer.timeBuffer.at(1) != qsl("sentinel"), "the replacement blank line kept the old one's timestamp");
    }

    // A line whose text has gone but whose formatting has not is replaced by an
    // empty one, orphaned TChars and all. It is built by hand here because no
    // caller was found that produces it, but the shortcut for lines that need
    // no wrapping has to agree with the rewrapping loop about it either way.
    void test_aLineWithFormattingButNoTextIsReplaced()
    {
        auto* console = consoleWithWrapWidth(200);
        QVERIFY(console);
        echo(qsl("keep me\\n"));
        echo(qsl("second line\\n"));

        console->buffer.lineBuffer[1] = QString();
        QVERIFY(!console->buffer.buffer.at(1).empty());
        console->buffer.promptBuffer[1] = true;

        console->buffer.wrapLine(0, 200, 0, 0);

        QCOMPARE(console->buffer.line(0), qsl("keep me"));
        QVERIFY2(console->buffer.buffer.at(1).empty(), "the orphaned formatting was left behind on a line with no text");
        QVERIFY2(!console->buffer.promptBuffer.at(1), "the replacement line kept the old one's prompt flag");
    }

    // The indent goes on the line that starts a paragraph and the hanging
    // indent on every continuation of it, so the two have to be told apart -
    // including on the second pass, over lines that are already wrapped.
    void test_indentAndHangingIndentAreAppliedToTheRightLines()
    {
        auto* console = consoleWithWrapWidth(12);
        QVERIFY(console);
        runLua(qsl("setWindowWrapIndent('%1', 2)").arg(mMiniConsole));
        runLua(qsl("setWindowWrapHangingIndent('%1', 4)").arg(mMiniConsole));
        echo(qsl("abcdefghijklmnopqrstuvwxyz\\n"));

        QCOMPARE(textIgnoringIndentation(console), qsl("abcdefghijklmnopqrstuvwxyz"));
        const QString firstLine = console->buffer.line(0);
        const QString secondLine = console->buffer.line(1);
        QVERIFY2(firstLine.startsWith(qsl("  a")), qPrintable(qsl("the first line was not indented by two: '%1'").arg(firstLine)));
        QVERIFY2(secondLine.startsWith(qsl("    ")), qPrintable(qsl("the second line did not get the hanging indent: '%1'").arg(secondLine)));

        // A second pass sees the indentation it already added as part of the
        // text and lays more on top, so the split points move. That is how it
        // has always behaved; what must not change is that no text is lost.
        console->buffer.wrapLine(0, 12, 2, 4);

        QCOMPARE(textIgnoringIndentation(console), qsl("abcdefghijklmnopqrstuvwxyz"));
    }

    // Narrowing the width rewraps a scrollback that is already wrapped, so no
    // line in the range is skipped by the shortcut.
    void test_narrowingTheWidthRewrapsTheWholeScrollback()
    {
        auto* console = consoleWithWrapWidth(30);
        QVERIFY(console);
        echo(qsl("abcdefghijklmnopqrstuvwxyz0123456789\\n"));
        echo(qsl("ABCDEFGHIJKLMNOPQRSTUVWXYZ9876543210\\n"));
        const int linesAt30 = nonEmptyLineCount(console);

        console->setWrapAt(8);
        // TConsole::luaWrapLine() is the only production path that rewraps a
        // scrollback at a new width - resizing the window does not
        runLua(qsl("wrapLine('%1', 0)").arg(mMiniConsole));

        QCOMPARE(joinedText(console), qsl("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ9876543210"));
        QVERIFY2(nonEmptyLineCount(console) > linesAt30, "narrowing the wrap width did not produce more lines");
        for (int i = 0, total = console->buffer.getLastLineNumber(); i <= total; ++i) {
            QVERIFY2(console->buffer.line(i).size() <= 8, qPrintable(qsl("line %1 is wider than the wrap width: '%2'").arg(QString::number(i), console->buffer.line(i))));
        }
    }

    // East Asian Wide glyphs take two columns each, so the split points come
    // from the column count rather than the character count.
    void test_wideGlyphsWrapOnTheirColumnWidth()
    {
        auto* console = consoleWithWrapWidth(4);
        QVERIFY(console);
        echo(qsl("%1%1%1\\n").arg(mWideText));

        QCOMPARE(joinedText(console), mWideText.repeated(3));
        QCOMPARE(nonEmptyLineCount(console), 3);
    }

    // A newline inserted into a line before the last one splits it in two, so
    // the lines under it move down and are rewrapped along with it.
    void test_anEmbeddedNewlineSplitsALineOtherThanTheLast()
    {
        auto* console = consoleWithWrapWidth(200);
        QVERIFY(console);
        echo(qsl("alpha\\n"));
        echo(qsl("beta\\n"));
        echo(qsl("gamma\\n"));

        QPoint target(2, 0);
        QVERIFY(console->buffer.insertInLine(target, qsl("XX\nYY"), TChar()));
        QCOMPARE(console->buffer.wrapLine(0, 200, 0, 0), 4);

        QCOMPARE(console->buffer.line(0), qsl("alXX"));
        QCOMPARE(console->buffer.line(1), qsl("YYpha"));
        QCOMPARE(console->buffer.line(2), qsl("beta"));
        QCOMPARE(console->buffer.line(3), qsl("gamma"));
    }

    // The arithmetic that adds the lines left in place to the lines the loop
    // rewrapped only has anything to prove when both are non-zero, which needs
    // a short line ahead of one that splits.
    void test_linesLeftInPlaceAndLinesSplitAreCountedTogether()
    {
        auto* console = consoleWithWrapWidth(30);
        QVERIFY(console);
        echo(qsl("abc\\n"));
        echo(qsl("abcdefghijklmnopqrstuvwxyz0123456789\\n"));

        console->setWrapAt(8);
        const int reported = console->buffer.wrapLine(0, 8, 0, 0);

        QCOMPARE(reported, static_cast<int>(console->buffer.buffer.size()) - 1);
        QCOMPARE(console->buffer.line(0), qsl("abc"));
        QCOMPARE(joinedText(console), qsl("abcabcdefghijklmnopqrstuvwxyz0123456789"));
        for (int i = 0, total = console->buffer.getLastLineNumber(); i <= total; ++i) {
            QVERIFY2(console->buffer.line(i).size() <= 8, qPrintable(qsl("line %1 is wider than the wrap width: '%2'").arg(QString::number(i), console->buffer.line(i))));
        }
    }

    // Logging is the one side effect wrapLine() has, and it only ever logs all
    // but the last line of what it produced - commitLineData() logs that one.
    // Only the main console logs, so this cannot use a miniconsole.
    void test_aWrappedLineReachesTheLogWhole()
    {
        auto* host = startLoggingProfile();
        QVERIFY(host);
        host->mpConsole->setWrapAt(20);

        // one unbroken token, so no wrap point can swallow a space, and long
        // enough to be split into more than two lines
        const QString token = qsl("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        runLua(qsl("feedTelnet('%1\\n')").arg(token));
        runLua(qsl("feedTelnet('and a short one\\n')"));

        QString log = stopLoggingAndReadLog(host);
        log.remove(QChar::LineFeed);
        QCOMPARE(log.count(token), 1);
        QVERIFY2(log.contains(qsl("and a short one")), "the line after the wrapped one is missing from the log");
    }

    // The bounds check lets startLine == buffer.size() through, so the empty
    // range has to answer 0 and leave the buffer alone.
    void test_anEmptyRangeChangesNothing()
    {
        auto* console = consoleWithWrapWidth(200);
        QVERIFY(console);
        echo(qsl("only line\\n"));
        const int lines = static_cast<int>(console->buffer.buffer.size());

        QCOMPARE(console->buffer.wrapLine(lines, 200, 0, 0), 0);

        QCOMPARE(static_cast<int>(console->buffer.buffer.size()), lines);
        QCOMPARE(console->buffer.line(0), qsl("only line"));
    }

    // TConsole::printCommand() echoes what was typed onto the prompt line and
    // rewraps from there, so its range is the prompt line plus the empty line
    // the buffer keeps ready - a mix of one line to leave alone and one to
    // replace.
    void test_aCommandEchoedOntoThePromptRewrapsFromThePromptLine()
    {
        auto* console = consoleWithWrapWidth(200);
        QVERIFY(console);
        echo(qsl("You say, \\\"hello\\\"\\n"));
        QCOMPARE(console->buffer.lineBuffer.size(), 2);
        console->buffer.promptBuffer[0] = true;

        QString command = qsl(" and wave");
        console->printCommand(command);

        QCOMPARE(console->buffer.line(0), qsl("You say, \"hello\" and wave"));
        QVERIFY2(!console->buffer.promptBuffer.at(0), "the prompt flag was not cleared once the command was echoed onto it");
        QCOMPARE(console->buffer.lineBuffer.size(), 2);
        QVERIFY(console->buffer.lineBuffer.constLast().isEmpty());
    }

private:
    void runLua(const QString& script)
    {
        auto host = mudlet::self()->getActiveHost();
        host->getLuaInterpreter()->compileAndExecuteScript(script);
    }

    void echo(const QString& text) { runLua(qsl("echo('%1', '%2')").arg(mMiniConsole, text)); }

    // a miniconsole keeps the assertions free of the main console's connection
    // messages, and wraps its text through exactly the same code
    TConsole* consoleWithWrapWidth(int width)
    {
        startProfile();
        runLua(qsl("createMiniConsole('%1', 0, 0, 600, 600)").arg(mMiniConsole));
        auto* console = mudlet::self()->getActiveHost()->mpConsole->mSubConsoleMap.value(mMiniConsole);
        if (console) {
            console->setWrapAt(width);
        }
        return console;
    }

    void startProfile()
    {
        QTimer::singleShot(0, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mPort);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectedSpy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connectedSpy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    // the buffer carries empty lines of its own (one is always kept ready for
    // the next text), so only the lines with something in them are counted
    static int nonEmptyLineCount(TConsole* console)
    {
        int count = 0;
        for (int i = 0, total = console->buffer.getLastLineNumber(); i <= total; ++i) {
            if (!console->buffer.line(i).isEmpty()) {
                ++count;
            }
        }
        return count;
    }

    // every line of the console joined back together - with no indentation
    // configured the wrapping only breaks lines, so this has to come back out
    // exactly as it went in
    static QString joinedText(TConsole* console)
    {
        QString text;
        for (int i = 0, total = console->buffer.getLastLineNumber(); i <= total; ++i) {
            text.append(console->buffer.line(i));
        }
        return text;
    }

    // as joinedText(), but with every space dropped, for the cases where the
    // wrapping pads lines out with indentation
    static QString textIgnoringIndentation(TConsole* console) { return joinedText(console).remove(QChar::Space); }

    // Takes the profile offline (feedTelnet() needs that) and turns on
    // plain-text logging to a known file name, without timestamps so the log
    // holds nothing but the wrapped text.
    Host* startLoggingProfile()
    {
        startProfile();
        auto* host = mudlet::self()->getActiveHost();
        if (!host) {
            return nullptr;
        }
        host->mTelnet.disconnectIt();
        if (!QTest::qWaitFor(
                    [host]() {
                        return host->mTelnet.getConnectionState() == QAbstractSocket::UnconnectedState;
                    },
                    5000)) {
            qWarning() << "Profile did not go offline in time; feedTelnet() calls will fail";
        }

        host->mLogDir.clear();
        host->mLogFileNameFormat.clear();
        host->mLogFileName = qsl("wrapline-rewrap-test");
        host->mIsNextLogFileInHtmlFormat = false;
        host->mIsLoggingTimestamps = false;
        host->mpConsole->toggleLogging(false);
        return host;
    }

    QString stopLoggingAndReadLog(Host* host)
    {
        const QString logFileName = host->mpConsole->mLogFileName;
        host->mpConsole->toggleLogging(false);

        QFile logFile(logFileName);
        if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        return QString::fromUtf8(logFile.readAll());
    }

    void deleteProfileDirectory(const QString& profileName) { deleteDirectory(mudlet::getMudletPath(enums::profileHomePath, profileName)); }

    void deleteDirectory(const QString& path)
    {
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

void initializeQRCResources()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "WrapLineRewrapTest.moc"
QTEST_MAIN(WrapLineRewrapTest)
