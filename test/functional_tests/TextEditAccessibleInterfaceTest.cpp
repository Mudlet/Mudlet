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

// What a screen reader sees when it asks the main console for its text:
// TAccessibleTextEdit, Mudlet's QAccessibleTextInterface over the scrollback
// buffer.
//
// A Lua spec cannot reach any of it. The interface is handed to Qt's
// accessibility bridge by a factory that src/main.cpp installs, it is never
// exposed to the interpreter, and its whole job is to translate between the
// buffer's line/column coordinates and the flat character offsets the platform
// bridges speak - a translation with no Lua-visible consequence at all.
//
// The factory is installed here as well because MUDLET_GROUPED_TEST_MAIN (like
// QTEST_MAIN) never runs main(), so without this Qt would hand back the plain
// QAccessibleWidget and every case below would be measuring Qt rather than
// Mudlet.
//
// Offsets are derived from the buffer rather than written out, since the
// console starts with Mudlet's own startup messages ahead of anything this test
// prints.

#include <QAccessible>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFontInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "TAccessibleTextEdit.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TextEditAccessibleInterfaceTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-Accessible-TextEdit");
    const QString mLocalhost = qsl("localhost");
    // Content of its own, so a line can be found by name rather than by index
    const QString mFirstMarker = qsl("accessible line one");
    const QString mSecondMarker = qsl("accessible line two");
    const QString mThirdMarker = qsl("accessible line three");

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TTextEdit* pane() const { return mpHost->mpConsole->mUpperPane; }

    const QStringList& lines() const { return mpHost->mpConsole->buffer.lineBuffer; }

    QString wholeText() const { return lines().join(QChar::LineFeed); }

    int lineOf(const QString& text) const { return lines().indexOf(text); }

    // The offset of the first character of a line, counting the newline that
    // text() puts between lines.
    int offsetOfLine(const int line) const
    {
        int offset = 0;
        for (int i = 0; i < line; ++i) {
            offset += lines().at(i).length() + 1;
        }
        return offset;
    }

    QAccessibleTextInterface* textInterface() const
    {
        QAccessibleInterface* pInterface = QAccessible::queryAccessibleInterface(pane());
        return pInterface ? pInterface->textInterface() : nullptr;
    }

    bool lineIsOnScreen(const int line) const { return line >= pane()->imageTopLine() && line < pane()->imageTopLine() + pane()->getScreenHeight(); }

private slots:
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

#ifdef INCLUDE_FONTS
        // src/main.cpp extracts the bundled fonts into the config directory and
        // FontManager picks them up from there, but no test binary runs it, so
        // on a machine that has not run Mudlet before there is nothing on disk
        // to pick up and Qt quietly substitutes another family.
        for (const QString& file : {qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"), qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMoBd.ttf")}) {
            QVERIFY2(QFontDatabase::addApplicationFont(file) != -1, qPrintable(qsl("Could not register the bundled font %1").arg(file)));
        }
        const QFont bundled(qsl("Bitstream Vera Sans Mono"), 10);
        QCOMPARE(QFontInfo(bundled).family(), qsl("Bitstream Vera Sans Mono"));
#endif

        QAccessible::installFactory(TAccessibleTextEdit::textEditFactory);

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");

        QSignalSpy connected(&mpHost->mTelnet, &cTelnet::signal_connected);
        if (connected.isEmpty()) {
            QVERIFY2(connected.wait(15s), "The test profile never connected to the stub server.");
        }

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        // Enough lines that the first ones are scrolled off the top, which is
        // what the visibility cases need, followed by the three named ones so
        // they are on screen at the end.
        for (int line = 0; line < 200; ++line) {
            mpHost->mpConsole->print(qsl("filler %1\n").arg(line));
        }
        mpHost->mpConsole->print(qsl("%1\n%2\n%3\n").arg(mFirstMarker, mSecondMarker, mThirdMarker));
        QTest::qWait(100ms);

        QVERIFY2(textInterface(), "no QAccessibleTextInterface for the console - the factory never took");
        QVERIFY2(lineOf(mSecondMarker) > 0, "the marker lines never reached the buffer");
        QVERIFY2(pane()->imageTopLine() > 0, "the console never scrolled, so nothing here is off screen");
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

    void cleanup()
    {
        if (QAccessibleTextInterface* ti = textInterface()) {
            ti->removeSelection(0);
        }
        // Every case but one needs the marker lines on screen, and the one that
        // scrolls away from them leaves the view up in the buffer if it fails
        // part way through.
        pane()->scrollTo(mpHost->mpConsole->buffer.getLastLineNumber() + 1);
    }

    // The flat string every other case indexes into: the buffer's lines with a
    // newline between them, and a length that agrees with it.
    void test_theTextIsTheWholeBufferJoinedWithNewlines()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const QString all = wholeText();

        QCOMPARE(ti->characterCount(), all.length());
        QCOMPARE(ti->text(0, all.length()), all);
        QVERIFY2(all.contains(mSecondMarker), "the text handed out does not contain what was printed");
    }

    // A line boundary hands back the whole line the offset sits on, newline and
    // all, framed by the offsets of its first and last characters.
    void test_textAtOffsetReturnsTheLineTheOffsetIsOn()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const int line = lineOf(mSecondMarker);
        const int lineStart = offsetOfLine(line);
        int start = 0;
        int end = 0;

        const QString text = ti->textAtOffset(lineStart + 3, QAccessible::LineBoundary, &start, &end);

        QCOMPARE(text, mSecondMarker + QChar::LineFeed);
        QCOMPARE(start, lineStart);
        QCOMPARE(end, lineStart + mSecondMarker.length() + 1);
    }

    // ...and the before/after forms step a whole line either way.
    void test_textBeforeAndAfterOffsetStepToTheNeighbouringLines()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const int lineStart = offsetOfLine(lineOf(mSecondMarker));
        int start = 0;
        int end = 0;

        QCOMPARE(ti->textBeforeOffset(lineStart + 3, QAccessible::LineBoundary, &start, &end), mFirstMarker + QChar::LineFeed);
        QCOMPARE(ti->textAfterOffset(lineStart + 3, QAccessible::LineBoundary, &start, &end), mThirdMarker + QChar::LineFeed);
    }

    // There is nothing before the first line, or before the first character, or
    // after the last one, and asking for any of it has to come back empty
    // rather than walking off the end of the buffer.
    void test_steppingOffTheEndsOfTheTextComesBackEmpty()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        int start = -1;
        int end = -1;

        QCOMPARE(ti->textBeforeOffset(0, QAccessible::LineBoundary, &start, &end), QString());
        QCOMPARE(start, 0);
        QCOMPARE(end, 0);

        QCOMPARE(ti->textBeforeOffset(0, QAccessible::CharBoundary, &start, &end), QString());
        QCOMPARE(start, 0);
        QCOMPARE(end, 0);

        QCOMPARE(ti->textAfterOffset(ti->characterCount() - 1, QAccessible::CharBoundary, &start, &end), QString());
        QCOMPARE(start, 0);
        QCOMPARE(end, 0);

        QCOMPARE(ti->textBeforeOffset(0, QAccessible::WordBoundary, &start, &end), QString());
        QCOMPARE(start, 0);
        QCOMPARE(end, 0);
    }

    // A character boundary walks one code unit at a time, which is what a
    // screen reader's arrow keys ask for.
    void test_charBoundaryStepsOneCharacterAtATime()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const QString all = wholeText();
        const int offset = offsetOfLine(lineOf(mSecondMarker)) + 5;
        int start = -1;
        int end = -1;

        QCOMPARE(ti->textAtOffset(offset, QAccessible::CharBoundary, &start, &end), QString(all.at(offset)));
        QCOMPARE(start, offset);
        QCOMPARE(end, offset + 1);

        QCOMPARE(ti->textBeforeOffset(offset, QAccessible::CharBoundary, &start, &end), QString(all.at(offset - 1)));
        QCOMPARE(ti->textAfterOffset(offset, QAccessible::CharBoundary, &start, &end), QString(all.at(offset + 1)));
    }

    // A word boundary hands back the word the offset is inside.
    void test_wordBoundaryReturnsTheWordTheOffsetIsIn()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        // "accessible line two", two characters into the first word
        const int offset = offsetOfLine(lineOf(mSecondMarker)) + 2;
        int start = -1;
        int end = -1;

        QCOMPARE(ti->textAtOffset(offset, QAccessible::WordBoundary, &start, &end), qsl("accessible"));
        QCOMPARE(wholeText().mid(start, end - start), qsl("accessible"));
    }

    // Only four of Qt's boundary types are implemented; the rest have to be
    // refused rather than answered with something made up.
    void test_anUnsupportedBoundaryTypeIsRefused()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        int start = 0;
        int end = 0;

        QCOMPARE(ti->textAtOffset(offsetOfLine(lineOf(mSecondMarker)), QAccessible::ParagraphBoundary, &start, &end), QString());
        QCOMPARE(start, -1);
        QCOMPARE(end, -1);
    }

    // An offset past the end of the text is an error, not an empty line.
    void test_anOffsetPastTheEndOfTheTextIsRefused()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        int start = 0;
        int end = 0;

        QCOMPARE(ti->textAtOffset(ti->characterCount() + 1, QAccessible::LineBoundary, &start, &end), QString());
        QCOMPARE(start, -1);
        QCOMPARE(end, -1);
    }

    // NoBoundary means "everything from here", and each of the three methods
    // takes a different slice of the text for it.
    void test_noBoundaryHandsOutTheTextWholesale()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const QString all = wholeText();
        const int offset = offsetOfLine(lineOf(mSecondMarker));
        int start = -1;
        int end = -1;

        QCOMPARE(ti->textAfterOffset(offset, QAccessible::NoBoundary, &start, &end), all.mid(offset));
        QCOMPARE(start, offset);
        QCOMPARE(end, all.length());

        QCOMPARE(ti->textBeforeOffset(offset, QAccessible::NoBoundary, &start, &end), all.left(offset));
        QCOMPARE(ti->textAtOffset(offset, QAccessible::NoBoundary, &start, &end), all);
    }

    // The caret is stored as a line and a column but addressed as an offset, so
    // the two have to agree in both directions.
    void test_theCaretPositionRoundTripsThroughACharacterOffset()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const int line = lineOf(mSecondMarker);
        const int offset = offsetOfLine(line) + 4;
        pane()->setCaretPosition(0, 0);
        QVERIFY2(ti->cursorPosition() != offset, "the caret was already where this was going to put it");

        ti->setCursorPosition(offset);

        QCOMPARE(pane()->mCaretLine, line);
        QCOMPARE(pane()->mCaretColumn, 4);
        QCOMPARE(ti->cursorPosition(), offset);
    }

    // An offset that is not in the text leaves the caret alone rather than
    // moving it somewhere invented.
    void test_anInvalidCaretPositionIsIgnored()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const int offset = offsetOfLine(lineOf(mSecondMarker)) + 4;
        ti->setCursorPosition(offset);
        QCOMPARE(ti->cursorPosition(), offset);

        ti->setCursorPosition(-1);
        QCOMPARE(ti->cursorPosition(), offset);

        ti->setCursorPosition(ti->characterCount() + 1);
        QCOMPARE(ti->cursorPosition(), offset);
    }

    // Selecting from the accessibility side has to produce the console's own
    // selection, and dropping it has to take that away again.
    void test_addSelectionSelectsTheRangeAndRemoveSelectionClearsIt()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const int line = lineOf(mSecondMarker);
        QVERIFY2(lineIsOnScreen(line), "the line being selected is off screen, so no selection region could be built for it");
        const int start = offsetOfLine(line);
        QCOMPARE(ti->selectionCount(), 0);

        ti->addSelection(start, start + mSecondMarker.length());

        QCOMPARE(ti->selectionCount(), 1);
        int reportedStart = -1;
        int reportedEnd = -1;
        ti->selection(0, &reportedStart, &reportedEnd);
        QCOMPARE(reportedStart, start);
        QVERIFY2(reportedEnd > reportedStart, "the reported selection is empty although a range was selected");

        ti->removeSelection(0);

        QCOMPARE(ti->selectionCount(), 0);
    }

    // QAccessibleTextInterface's end offset is the first character that is NOT
    // selected, so text(start, end) has to hand back exactly what was selected -
    // which is what a screen reader reads out when it announces a selection.
    // Mudlet reports one less: addSelection() stores the last selected cell in
    // mPB and selection() returns that offset unchanged, so the final character
    // is dropped and a one-character selection comes back as an empty range
    // while selectionCount() still says there is one. A stock QTextEdit, which
    // implements the same interface, reports the range asked for.
    //
    // QEXPECT_FAIL rather than a weaker assertion, so this case goes red the day
    // the off-by-one is fixed and the markers can be taken back out.
    void test_theReportedSelectionRoundTripsThroughTheTextItSelected()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const int line = lineOf(mSecondMarker);
        QVERIFY2(lineIsOnScreen(line), "the line being selected is off screen, so no selection region could be built for it");
        const int start = offsetOfLine(line);
        int reportedStart = -1;
        int reportedEnd = -1;

        ti->addSelection(start, start + mSecondMarker.length());
        ti->selection(0, &reportedStart, &reportedEnd);

        QCOMPARE(reportedStart, start);
        QEXPECT_FAIL("", "#10411: selection() reports endOffset - 1, so the last selected character is lost", Continue);
        QCOMPARE(ti->text(reportedStart, reportedEnd), mSecondMarker);

        ti->removeSelection(0);
        ti->addSelection(start, start + 1);
        ti->selection(0, &reportedStart, &reportedEnd);

        QCOMPARE(ti->selectionCount(), 1);
        QEXPECT_FAIL("", "#10411: a one-character selection is reported as an empty range", Continue);
        QCOMPARE(reportedEnd - reportedStart, 1);
    }

    // setSelection is addSelection under another name, and both refuse an index
    // other than the single selection the console supports.
    void test_setSelectionOnlySupportsTheFirstSelection()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const int start = offsetOfLine(lineOf(mSecondMarker));
        QCOMPARE(ti->selectionCount(), 0);

        ti->setSelection(1, start, start + 4);
        QCOMPARE(ti->selectionCount(), 0);

        ti->setSelection(0, start, start + 4);
        QCOMPARE(ti->selectionCount(), 1);
    }

    // Offsets that are not in the text select nothing at all.
    void test_anInvalidSelectionRangeIsRefused()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        QCOMPARE(ti->selectionCount(), 0);

        ti->addSelection(-1, 5);
        QCOMPARE(ti->selectionCount(), 0);

        ti->addSelection(0, ti->characterCount() + 1);
        QCOMPARE(ti->selectionCount(), 0);
    }

    // The rectangle is what a screen magnifier follows, so it has to be one
    // character cell wide and sit one cell further right for the next offset.
    void test_characterRectCoversOneCellAndAdvancesWithTheOffset()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const int offset = offsetOfLine(lineOf(mSecondMarker)) + 2;

        const QRect first = ti->characterRect(offset);
        const QRect second = ti->characterRect(offset + 1);

        QVERIFY2(first.isValid() && first.width() > 0 && first.height() > 0, "no rectangle for a character that is on screen");
        QCOMPARE(second.size(), first.size());
        QCOMPARE(second.left() - first.left(), first.width());
        QCOMPARE(second.top(), first.top());
    }

    // A character that has scrolled off the top has no rectangle at all.
    void test_characterRectIsEmptyForALineThatIsScrolledOutOfView()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        QVERIFY2(!lineIsOnScreen(0), "the first line is still on screen, so this proves nothing");

        QCOMPARE(ti->characterRect(0), QRect());
    }

    // The inverse lookup: the point a character was reported at has to lead
    // back to that character.
    void test_offsetAtPointFindsTheCharacterUnderIt()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const int offset = offsetOfLine(lineOf(mSecondMarker)) + 6;
        const QRect rect = ti->characterRect(offset);
        QVERIFY(rect.isValid());

        QCOMPARE(ti->offsetAtPoint(rect.center()), offset);
    }

    // Scrolling on request is how a screen reader follows a caret it moved into
    // text that is no longer showing.
    void test_scrollToSubstringBringsAnOffScreenLineIntoView()
    {
        QAccessibleTextInterface* ti = textInterface();
        QVERIFY(ti);
        const int line = 5;
        QVERIFY2(!lineIsOnScreen(line), "the line is already on screen, so this proves nothing");

        ti->scrollToSubstring(offsetOfLine(line), offsetOfLine(line) + 3);

        QVERIFY2(lineIsOnScreen(line), qPrintable(qsl("line %1 is still not showing - the screen starts at line %2").arg(line).arg(pane()->imageTopLine())));
    }

    // The state a bridge reads to decide how to present the widget.
    void test_stateReportsSelectableMultiLineText()
    {
        QAccessibleInterface* pInterface = QAccessible::queryAccessibleInterface(pane());
        QVERIFY(pInterface);

        const QAccessible::State state = pInterface->state();

        QVERIFY2(state.selectableText, "the console does not report its text as selectable");
        QVERIFY2(state.multiLine, "the console does not report itself as multi-line");
        QVERIFY2(state.focusable, "the console does not report itself as focusable");
    }
};

#include "TextEditAccessibleInterfaceTest.moc"
MUDLET_GROUPED_TEST_MAIN(TextEditAccessibleInterfaceTest)
