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

// Caret mode: the keyboard-only way around the scrollback that Mudlet offers
// screen-reader users. TTextEdit::keyPressEvent() is the whole of it - arrow
// and paging keys walking a caret through the buffer, Tab and Ctrl+bracket
// walking between links, Return activating the link under the caret, the Menu
// key offering the choices of a link that carries several commands, and any
// printable key handing both the character and the focus to the command line.
//
// A Lua spec cannot press a key in a console. It can read nothing of this
// either: the caret's line and column are private C++ state on TTextEdit with
// no scripting accessor, so even the outcome is invisible from Lua.
//
// Deliberately not covered: the Shift and Ctrl variants of the arrow keys.
// Those branches read QGuiApplication::keyboardModifiers(), which is live
// window-system state that QTest's widget-level key events do not set, so
// sending Shift+Left here would exercise the plain Left branch and quietly
// claim to have tested selection.

#include <QFileInfo>
#include <QFontDatabase>
#include <QFontInfo>
#include <QMenu>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "TBuffer.h"
#include "TCommandLine.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class CaretNavigationTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-Caret-Navigation");
    const QString mLocalhost = qsl("localhost");
    // Three lines of known and deliberately unequal lengths, so a column can be
    // clamped by one and restored by the next
    const QString mLongLine = qsl("alpha bravo charlie");
    const QString mShortLine = qsl("short");
    const QString mLastLine = qsl("delta echo foxtrot golf");
    const QString mLinkLine = qsl("LINKONE LINKTWO");
    const QString mPopupLine = qsl("POPUPLINK");
    int mLongLineNumber = -1;
    int mShortLineNumber = -1;
    int mLastLineNumber = -1;
    int mLinkLineNumber = -1;
    int mPopupLineNumber = -1;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TTextEdit* pane() const { return mpHost->mpConsole->mUpperPane; }

    TBuffer& consoleBuffer() const { return mpHost->mpConsole->buffer; }

    static void press(TTextEdit* pPane, const Qt::Key key, const Qt::KeyboardModifiers modifiers = Qt::NoModifier) { QTest::keyClick(pPane, key, modifiers); }

    bool runLua(const QString& script) { return mpHost->mLuaInterpreter.compileAndExecuteScript(script); }

    // Handing focus to the command line is done on a chain of single-shot
    // timers up to 50ms long. Left to fire in a later case, the console's
    // focus-out handler turns caret mode off in the middle of it.
    static void drainDeferredFocusChange() { QTest::qWait(80ms); }

    // The menu is parented on the pane and deletes itself when it closes, but
    // only once the deferred deletion is delivered, so an earlier case's menu
    // can still be a child when the next one looks.
    QMenu* openLinkMenu()
    {
        for (QMenu* stale : pane()->findChildren<QMenu*>()) {
            delete stale;
        }
        QTest::keyClick(pane(), Qt::Key_Menu);
        return pane()->findChild<QMenu*>();
    }

    QString luaGlobal(const char* name) const
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

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

        // More lines than fit on the screen, so paging has somewhere to go
        for (int line = 0; line < 100; ++line) {
            mpHost->mpConsole->print(qsl("caret filler %1\n").arg(line));
        }
        mpHost->mpConsole->print(qsl("%1\n%2\n%3\n").arg(mLongLine, mShortLine, mLastLine));
        // Two links on one line, with a plain character between them, so
        // stepping between links has to land on each one's first column
        QVERIFY(runLua(qsl("caretLinkOne = ''\n"
                           "caretLinkTwo = ''\n"
                           "echoLink('LINKONE', [[caretLinkOne = 'ran']], 'the first link')\n"
                           "echo(' ')\n"
                           "echoLink('LINKTWO', [[caretLinkTwo = 'ran']], 'the second link')\n"
                           "echo('\\n')\n"
                           "caretPopupA = ''\n"
                           "caretPopupB = ''\n"
                           "echoPopup('POPUPLINK', {[[caretPopupA = 'a']], [[caretPopupB = 'b']]}, {'first choice', 'second choice'})\n"
                           "echo('\\n')")));
        QTest::qWait(100ms);

        mLongLineNumber = consoleBuffer().lineBuffer.indexOf(mLongLine);
        mShortLineNumber = consoleBuffer().lineBuffer.indexOf(mShortLine);
        mLastLineNumber = consoleBuffer().lineBuffer.indexOf(mLastLine);
        mLinkLineNumber = consoleBuffer().lineBuffer.indexOf(mLinkLine);
        mPopupLineNumber = consoleBuffer().lineBuffer.indexOf(mPopupLine);
        QVERIFY2(mLongLineNumber > 0, "the marker lines never reached the buffer");
        QCOMPARE(mShortLineNumber, mLongLineNumber + 1);
        QCOMPARE(mLastLineNumber, mLongLineNumber + 2);
        QCOMPARE(mLinkLineNumber, mLongLineNumber + 3);
        QCOMPARE(mPopupLineNumber, mLongLineNumber + 4);
        QVERIFY2(consoleBuffer().getLinkIndexAt(mLinkLineNumber, 0) > 0, "echoLink() put no link on the line it printed");
        QVERIFY2(consoleBuffer().getLinkIndexAt(mLastLineNumber, 0) == 0, "the line before the links carries a link of its own, so stepping forward would find the wrong one");
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

    void init()
    {
        mpHost->setCaretEnabled(true);
        // Remembered across key presses rather than reset by them, so a column
        // banked by one test would follow the caret into the next one
        pane()->mOldCaretColumn = 0;
        consoleBuffer().setFocusedLink(0);
    }

    void cleanup()
    {
        // Guarded: TConsole::setCaretMode(false) asserts if it is called when
        // the pane already has its focus proxy back, which a second call does.
        if (mpHost->caretEnabled()) {
            mpHost->setCaretEnabled(false);
        }
        // A case that moved focus can leave the change undelivered, and it would
        // otherwise land during the next case - after its init() turned caret
        // mode back on, so the pane's focusOutEvent turns it straight off again.
        drainDeferredFocusChange();
    }

    // Nothing below runs at all unless caret mode is on: without it
    // keyPressEvent() hands every key straight back to QWidget.
    void test_withoutCaretModeTheArrowKeysDoNotMoveTheCaret()
    {
        mpHost->setCaretEnabled(false);
        QVERIFY(!mpHost->caretEnabled());
        pane()->setCaretPosition(mLongLineNumber, 4);

        press(pane(), Qt::Key_Right);

        QCOMPARE(pane()->mCaretColumn, 4);

        // The same key press with caret mode back on, so a press() that never
        // reached the pane could not be what kept the column still above.
        // Turning caret mode on parks the caret at the end of the buffer, so
        // the position has to be set again after it rather than before.
        mpHost->setCaretEnabled(true);
        pane()->setCaretPosition(mLongLineNumber, 4);
        press(pane(), Qt::Key_Right);
        QCOMPARE(pane()->mCaretColumn, 5);
    }

    void test_leftAndRightWalkTheCaretAlongALine()
    {
        pane()->setCaretPosition(mLongLineNumber, 4);

        press(pane(), Qt::Key_Right);
        QCOMPARE(pane()->mCaretLine, mLongLineNumber);
        QCOMPARE(pane()->mCaretColumn, 5);

        press(pane(), Qt::Key_Left);
        QCOMPARE(pane()->mCaretLine, mLongLineNumber);
        QCOMPARE(pane()->mCaretColumn, 4);
    }

    // Off the right-hand end of a line and onto the start of the next.
    void test_rightAtTheEndOfALineMovesToTheStartOfTheNext()
    {
        pane()->setCaretPosition(mLongLineNumber, mLongLine.length() - 1);

        press(pane(), Qt::Key_Right);

        QCOMPARE(pane()->mCaretLine, mShortLineNumber);
        QCOMPARE(pane()->mCaretColumn, 0);
    }

    // ...and back off the left-hand end onto the end of the previous one.
    void test_leftAtTheStartOfALineMovesToTheEndOfThePrevious()
    {
        pane()->setCaretPosition(mShortLineNumber, 0);

        press(pane(), Qt::Key_Left);

        QCOMPARE(pane()->mCaretLine, mLongLineNumber);
        QCOMPARE(pane()->mCaretColumn, mLongLine.length() - 1);
    }

    void test_upAndDownWalkTheCaretBetweenLines()
    {
        pane()->setCaretPosition(mLongLineNumber, 3);

        press(pane(), Qt::Key_Down);
        QCOMPARE(pane()->mCaretLine, mShortLineNumber);
        QCOMPARE(pane()->mCaretColumn, 3);

        press(pane(), Qt::Key_Up);
        QCOMPARE(pane()->mCaretLine, mLongLineNumber);
        QCOMPARE(pane()->mCaretColumn, 3);
    }

    // Walking down past a short line must not silently lose the column the user
    // was in: it is banked, clamped for the short line, and given back once a
    // line is long enough to hold it again.
    void test_aColumnLostToAShortLineComesBackOnTheNextLongOne()
    {
        pane()->setCaretPosition(mLongLineNumber, 10);
        QVERIFY2(mShortLine.length() < 10, "the short line is not short enough for the column to be clamped");

        press(pane(), Qt::Key_Down);
        QCOMPARE(pane()->mCaretLine, mShortLineNumber);
        QCOMPARE(pane()->mCaretColumn, mShortLine.length() - 1);

        press(pane(), Qt::Key_Down);
        QCOMPARE(pane()->mCaretLine, mLastLineNumber);
        QCOMPARE(pane()->mCaretColumn, 10);
    }

    void test_homeAndEndJumpToTheEndsOfTheCurrentLine()
    {
        pane()->setCaretPosition(mLongLineNumber, 7);

        press(pane(), Qt::Key_Home);
        QCOMPARE(pane()->mCaretLine, mLongLineNumber);
        QCOMPARE(pane()->mCaretColumn, 0);

        press(pane(), Qt::Key_End);
        QCOMPARE(pane()->mCaretLine, mLongLineNumber);
        QCOMPARE(pane()->mCaretColumn, mLongLine.length() - 1);
    }

    void test_pageUpAndPageDownMoveByAScreenful()
    {
        const int screenful = pane()->getScreenHeight();
        QVERIFY2(screenful > 1 && screenful < mLongLineNumber, "the screen is not a usable size for paging");
        pane()->setCaretPosition(mLongLineNumber, 0);

        press(pane(), Qt::Key_PageUp);
        QCOMPARE(pane()->mCaretLine, mLongLineNumber - screenful);

        press(pane(), Qt::Key_PageDown);
        QCOMPARE(pane()->mCaretLine, mLongLineNumber);
    }

    // The ends of the buffer are walls, not places to fall off.
    // A step onto the last line either way first, so a key press that did
    // nothing at all could not pass for the wall being there.
    void test_theCaretStopsAtBothEndsOfTheBuffer()
    {
        pane()->setCaretPosition(1, 3);
        press(pane(), Qt::Key_Up);
        QCOMPARE(pane()->mCaretLine, 0);
        press(pane(), Qt::Key_Up);
        QCOMPARE(pane()->mCaretLine, 0);

        const int lastLineWithText = consoleBuffer().lineBuffer.length() - 1 - (consoleBuffer().lineBuffer.last().isEmpty() ? 1 : 0);
        pane()->setCaretPosition(lastLineWithText - 1, 0);
        press(pane(), Qt::Key_Down);
        QCOMPARE(pane()->mCaretLine, lastLineWithText);
        press(pane(), Qt::Key_Down);
        QCOMPARE(pane()->mCaretLine, lastLineWithText);

        press(pane(), Qt::Key_PageDown);
        QCOMPARE(pane()->mCaretLine, lastLineWithText);
    }

    // #7933: a printable key pressed while reading the output is meant to be
    // the start of a command, so caret mode gets out of the way and the
    // character is delivered to the command line rather than dropped.
    void test_aPrintableKeyHandsTheCharacterToTheCommandLine()
    {
        TCommandLine* pCommandLine = mpHost->mpConsole->mpCommandLine;
        QVERIFY(pCommandLine);
        pCommandLine->clear();
        pane()->setCaretPosition(mLongLineNumber, 0);
        QVERIFY(mpHost->caretEnabled());

        QTest::keyClick(pane(), 'z');

        QVERIFY2(!mpHost->caretEnabled(), "caret mode is still on, so the console kept the keyboard");
        QCOMPARE(pCommandLine->toPlainText(), qsl("z"));
        pCommandLine->clear();
        drainDeferredFocusChange();
    }

    // ...but a key that cannot start a command stays with the caret.
    void test_aCaretKeyIsNotSentToTheCommandLine()
    {
        TCommandLine* pCommandLine = mpHost->mpConsole->mpCommandLine;
        QVERIFY(pCommandLine);
        pCommandLine->clear();
        pane()->setCaretPosition(mLongLineNumber, 4);

        press(pane(), Qt::Key_Left);

        QVERIFY2(mpHost->caretEnabled(), "an arrow key turned caret mode off");
        QCOMPARE(pCommandLine->toPlainText(), QString());
        QCOMPARE(pane()->mCaretColumn, 3);
    }

    // Tab and Shift+Tab are how a screen-reader user finds the links in a page
    // of output without a mouse.
    void test_tabAndBacktabStepBetweenTheLinksInTheBuffer()
    {
        pane()->setCaretPosition(mLastLineNumber, 0);
        const int firstLinkColumn = mLinkLine.indexOf(qsl("LINKONE"));
        const int secondLinkColumn = mLinkLine.indexOf(qsl("LINKTWO"));

        press(pane(), Qt::Key_Tab);
        QCOMPARE(pane()->mCaretLine, mLinkLineNumber);
        QCOMPARE(pane()->mCaretColumn, firstLinkColumn);

        press(pane(), Qt::Key_Tab);
        QCOMPARE(pane()->mCaretLine, mLinkLineNumber);
        QCOMPARE(pane()->mCaretColumn, secondLinkColumn);

        press(pane(), Qt::Key_Backtab);
        QCOMPARE(pane()->mCaretLine, mLinkLineNumber);
        QCOMPARE(pane()->mCaretColumn, firstLinkColumn);
    }

    // Ctrl+] and Ctrl+[ do the same, for the case where the user has given Tab
    // away to the caret-mode toggle.
    void test_ctrlBracketAlsoStepsBetweenLinks()
    {
        pane()->setCaretPosition(mLastLineNumber, 0);
        const int firstLinkColumn = mLinkLine.indexOf(qsl("LINKONE"));
        const int secondLinkColumn = mLinkLine.indexOf(qsl("LINKTWO"));

        press(pane(), Qt::Key_BracketRight, Qt::ControlModifier);
        QCOMPARE(pane()->mCaretColumn, firstLinkColumn);

        press(pane(), Qt::Key_BracketRight, Qt::ControlModifier);
        QCOMPARE(pane()->mCaretColumn, secondLinkColumn);

        press(pane(), Qt::Key_BracketLeft, Qt::ControlModifier);
        QCOMPARE(pane()->mCaretColumn, firstLinkColumn);
    }

    // Without the modifier the same keys are ordinary characters, and go to the
    // command line like any other printable key.
    void test_aBracketWithoutControlIsJustACharacter()
    {
        pane()->setCaretPosition(mLastLineNumber, 0);

        press(pane(), Qt::Key_BracketRight);

        QCOMPARE(pane()->mCaretLine, mLastLineNumber);
        QCOMPARE(pane()->mCaretColumn, 0);
        QVERIFY2(!mpHost->caretEnabled(), "a printable key left caret mode on");
        mpHost->mpConsole->mpCommandLine->clear();
        drainDeferredFocusChange();
    }

    // Return on the link under the caret runs it, which is the entire point of
    // being able to reach it with the keyboard.
    void test_returnActivatesTheLinkUnderTheCaret()
    {
        QVERIFY(runLua(qsl("caretLinkOne = ''")));
        pane()->setCaretPosition(mLastLineNumber, 0);
        press(pane(), Qt::Key_Tab);
        QVERIFY2(consoleBuffer().getFocusedLink() > 0, "no link is focused, so Return has nothing to activate");

        press(pane(), Qt::Key_Return);

        QCOMPARE(luaGlobal("caretLinkOne"), qsl("ran"));
    }

    // With no link under it, Return is not an activation and must not run the
    // link the caret visited last.
    void test_returnOffALinkRunsNothing()
    {
        QVERIFY(runLua(qsl("caretLinkOne = ''")));
        pane()->setCaretPosition(mLinkLineNumber, mLinkLine.indexOf(qsl("LINKONE")));
        QVERIFY(consoleBuffer().getFocusedLink() > 0);
        pane()->setCaretPosition(mLongLineNumber, 0);
        QCOMPARE(consoleBuffer().getFocusedLink(), 0);

        press(pane(), Qt::Key_Return);

        QCOMPARE(luaGlobal("caretLinkOne"), QString());
    }

    // A link carrying more than one command cannot be activated outright, so
    // the Menu key offers the choice the mouse would get from a right-click.
    void test_theMenuKeyOffersEachOfALinksCommands()
    {
        pane()->setCaretPosition(mPopupLineNumber, 0);
        QVERIFY2(consoleBuffer().getFocusedLink() > 0, "no link is focused, so there is no menu to open");

        QMenu* pMenu = openLinkMenu();

        QVERIFY2(pMenu, "no menu appeared for a link with more than one command");
        QStringList offered;
        for (const QAction* pAction : pMenu->actions()) {
            offered << pAction->text();
        }
        QCOMPARE(offered, QStringList({qsl("first choice"), qsl("second choice")}));
        pMenu->close();
    }

    void test_choosingFromTheMenuRunsThatCommand()
    {
        QVERIFY(runLua(qsl("caretPopupA = ''\ncaretPopupB = ''")));
        pane()->setCaretPosition(mPopupLineNumber, 0);
        QMenu* pMenu = openLinkMenu();
        QVERIFY(pMenu);
        QCOMPARE(pMenu->actions().size(), 2);

        pMenu->actions().at(1)->trigger();

        QCOMPARE(luaGlobal("caretPopupB"), qsl("b"));
        QCOMPARE(luaGlobal("caretPopupA"), QString());
        pMenu->close();
    }

    // With only one command there is nothing to choose between, so the same key
    // runs it rather than opening a menu of one.
    void test_theMenuKeyActivatesASingleCommandLinkOutright()
    {
        QVERIFY(runLua(qsl("caretLinkOne = ''")));
        pane()->setCaretPosition(mLinkLineNumber, mLinkLine.indexOf(qsl("LINKONE")));
        QVERIFY(consoleBuffer().getFocusedLink() > 0);

        QMenu* pMenu = openLinkMenu();

        QVERIFY2(!pMenu, "a link with a single command still opened a menu");
        QCOMPARE(luaGlobal("caretLinkOne"), qsl("ran"));
    }

    // Activating a link marks it visited, which is what the announcement and
    // the styling both read. A link of this case's own, because the ones above
    // have been activated already and would satisfy the assertion without this
    // key press doing anything.
    void test_activatingALinkMarksItVisited()
    {
        QVERIFY(runLua(qsl("echoLink('LINKTHREE', [[caretLinkThree = 'ran']], 'the third link')\necho('\\n')")));
        QTest::qWait(100ms);
        const int line = consoleBuffer().lineBuffer.indexOf(qsl("LINKTHREE"));
        QVERIFY2(line > 0, "the third link never reached the buffer");
        // Anything that takes focus off the pane while that wait runs turns caret
        // mode off through focusOutEvent, and setCaretPosition() then returns
        // before it focuses a link. Linux and Windows hold the focus with a
        // keyboard grab that setCaretMode() does not do on macOS, which is why
        // this only ever failed there. The caret is this case's precondition
        // rather than its subject, so re-establish it instead of racing it.
        if (!mpHost->caretEnabled()) {
            mpHost->setCaretEnabled(true);
        }
        pane()->setCaretPosition(line, 0);
        const int linkIndex = consoleBuffer().getFocusedLink();
        // Reported in full because this has failed on macOS x86_64 alone and could
        // not be reproduced elsewhere: the focused link is read out of the TChar
        // grid, so which of the text and the grid disagreed is the whole diagnosis.
        QVERIFY2(linkIndex > 0,
                 qPrintable(qsl("no link focused at line %1 col 0; caret mode %2, text '%3' (%4 chars), grid row %5 cells, lineBuffer %6 lines, buffer %7 lines")
                                    .arg(line)
                                    .arg(mpHost->caretEnabled() ? qsl("on") : qsl("off"))
                                    .arg(consoleBuffer().lineBuffer.at(line))
                                    .arg(consoleBuffer().lineBuffer.at(line).length())
                                    .arg(line < static_cast<int>(consoleBuffer().buffer.size()) ? QString::number(consoleBuffer().buffer.at(line).size()) : qsl("out of range"))
                                    .arg(consoleBuffer().lineBuffer.size())
                                    .arg(static_cast<int>(consoleBuffer().buffer.size()))));
        QVERIFY2(!consoleBuffer().isLinkVisited(linkIndex), "the link counts as visited before anything activated it");

        press(pane(), Qt::Key_Return);

        QVERIFY2(consoleBuffer().isLinkVisited(linkIndex), "the activated link was not marked as visited");
    }
};

#include "CaretNavigationTest.moc"
MUDLET_GROUPED_TEST_MAIN(CaretNavigationTest)
