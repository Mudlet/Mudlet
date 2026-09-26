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

// The command line's spell checking and its right-click menu: the underline a
// word gets as it is typed, the spelling suggestions and user dictionary
// entries the menu offers, and the entries scripts add to it with
// addCommandLineMenuEvent().
//
// Lua can reach the spell checker itself but not the command line's use of
// it: what a word looks like and what a right-click offers are only visible
// from the widget.
//
// Every word checked here is made up, so no system dictionary can know it -
// the test binaries do not find Mudlet's bundled ones, and a machine that has
// one installed must not change the outcome.

#include <QMenu>
#include <QMouseEvent>
#include <QPointer>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QtTest/QtTest>
#include <algorithm>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "TCommandLine.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TUiTour.h"
#include "TelnetServerStub.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

class CommandLineSpellCheckTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-CommandLine-SpellCheck");
    const QString mLocalhost = qsl("localhost");
    int mLineCounter = 0;
    QStringList mAddedWords;

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    // A command line of this test's own, so no test inherits another's text or
    // menu entries. Lua cannot delete one again, but it is parented into the
    // console so the profile's teardown takes it with it.
    QString freshCommandLineName() { return qsl("spellCheckLine%1").arg(++mLineCounter); }

    TCommandLine* freshCommandLine(const QString& name)
    {
        auto [created, message] = mpHost->mpConsole->createCommandLine(QString(), name, 0, 0, 300, 30);
        if (!created) {
            qWarning() << "CommandLineSpellCheckTest - could not create a command line:" << message;
            return nullptr;
        }
        TCommandLine* pCommandLine = mpHost->mpConsole->subCommandLineWidget(name);
        if (pCommandLine) {
            pCommandLine->mSaveCommands = false;
        }
        return pCommandLine;
    }

    TCommandLine* freshCommandLine() { return freshCommandLine(freshCommandLineName()); }

    bool runLua(const QString& script) { return mpHost->mLuaInterpreter.compileAndExecuteScript(script); }

    QString luaGlobal(const char* name) const
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    void addToUserDictionary(const QString& word)
    {
        mpHost->spellChecker().addWord(word);
        mAddedWords << word;
    }

    // The format of the character just inside the start of the word, which is
    // what the underline was put on
    static QTextCharFormat formatOf(const TCommandLine* pCommandLine, const QString& word)
    {
        const int start = pCommandLine->toPlainText().indexOf(word);
        if (start < 0) {
            return {};
        }
        QTextCursor cursor(pCommandLine->document());
        cursor.setPosition(start + 1);
        return cursor.charFormat();
    }

    static bool markedMisspelt(const TCommandLine* pCommandLine, const QString& word)
    {
        const QTextCharFormat format = formatOf(pCommandLine, word);
        return format.underlineStyle() == QTextCharFormat::SpellCheckUnderline && format.underlineColor() == QColor(Qt::red);
    }

    static bool markedAsTheUsersOwn(const TCommandLine* pCommandLine, const QString& word)
    {
        const QTextCharFormat format = formatOf(pCommandLine, word);
        return format.underlineStyle() == QTextCharFormat::DashUnderline && format.underlineColor() == QColor(Qt::cyan);
    }

    static bool unmarked(const TCommandLine* pCommandLine, const QString& word) { return formatOf(pCommandLine, word).underlineStyle() == QTextCharFormat::NoUnderline; }

    // Right-clicks the middle of the word and returns the menu that opened.
    // The menu is parented on the command line, so any left over from an
    // earlier right-click are cleared out first to make the find unambiguous.
    static QMenu* rightClick(TCommandLine* pCommandLine, const QString& word)
    {
        for (QMenu* stale : pCommandLine->findChildren<QMenu*>()) {
            delete stale;
        }
        return rightClickLeavingOtherMenus(pCommandLine, word);
    }

    static QMenu* rightClickLeavingOtherMenus(TCommandLine* pCommandLine, const QString& word)
    {
        const QList<QMenu*> menusBefore = pCommandLine->findChildren<QMenu*>();
        const int start = pCommandLine->toPlainText().indexOf(word);
        if (start < 0) {
            return nullptr;
        }
        QTextCursor cursor(pCommandLine->document());
        cursor.setPosition(start + word.size() / 2);
        const QPoint position = pCommandLine->cursorRect(cursor).center();
        QWidget* pViewport = pCommandLine->viewport();
        QMouseEvent press(QEvent::MouseButtonPress, position, pViewport->mapToGlobal(position), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
        QApplication::sendEvent(pViewport, &press);
        QMouseEvent release(QEvent::MouseButtonRelease, position, pViewport->mapToGlobal(position), Qt::RightButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(pViewport, &release);
        for (QMenu* pMenu : pCommandLine->findChildren<QMenu*>()) {
            if (!menusBefore.contains(pMenu)) {
                return pMenu;
            }
        }
        return nullptr;
    }

    static QAction* findAction(const QMenu* pMenu, const QString& text)
    {
        for (QAction* pAction : pMenu->actions()) {
            if (pAction->text() == text) {
                return pAction;
            }
        }
        return nullptr;
    }

    static QStringList actionTexts(const QMenu* pMenu)
    {
        QStringList texts;
        for (const QAction* pAction : pMenu->actions()) {
            texts << pAction->text();
        }
        return texts;
    }

    static void closeMenu(QMenu* pMenu)
    {
        pMenu->close();
        delete pMenu;
    }

    // Puts the text in and the caret at its end, then has the word the caret is
    // on checked the way a key press has it checked
    static void enterText(TCommandLine* pCommandLine, const QString& text)
    {
        pCommandLine->setPlainText(text);
        pCommandLine->moveCursor(QTextCursor::End);
        QTest::keyClick(pCommandLine, Qt::Key_End);
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
        // the first-run tour's application-wide event filter would swallow the
        // key presses below
        TUiTour::rememberShown();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost);
        mpHost->setEnableSpellCheck(true);
        mpHost->setUserDictionaryOptions(true, false);
        QVERIFY2(mpHost->spellChecker().systemHandle(), "the profile has no system dictionary handle, so nothing below gets checked at all");
        QVERIFY2(mpHost->spellChecker().userHandle(), "the profile has no user dictionary to add words to");
    }

    void cleanup()
    {
        for (const QString& word : std::as_const(mAddedWords)) {
            mpHost->spellChecker().removeWord(word);
        }
        mAddedWords.clear();
    }

    void test_aMisspeltWordIsUnderlinedAsItIsTyped()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        QTest::keyClicks(pCommandLine, qsl("qzxfrob"));

        QVERIFY2(markedMisspelt(pCommandLine, qsl("qzxfrob")), "a word no dictionary knows was not marked as misspelt");
    }

    // The control for the test above
    void test_nothingIsUnderlinedWithSpellCheckOff()
    {
        mpHost->setEnableSpellCheck(false);
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        QTest::keyClicks(pCommandLine, qsl("qzxfrob"));

        QVERIFY2(unmarked(pCommandLine, qsl("qzxfrob")), "a word was marked with spell check off");
    }

    // A word the player added themselves is still not in the system dictionary,
    // and gets a mark of its own to say where it was found
    void test_aWordFromTheUserDictionaryGetsADashedUnderline()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);
        addToUserDictionary(qsl("qzxmyword"));

        QTest::keyClicks(pCommandLine, qsl("qzxmyword"));

        QVERIFY2(markedAsTheUsersOwn(pCommandLine, qsl("qzxmyword")), "a word from the user dictionary was not marked as the player's own");
    }

    // Switching spell check in the preferences reaches text already sitting in
    // the main command line: on marks every word in it, off takes the marks away
    void test_switchingSpellCheckRechecksTheWholeMainCommandLine()
    {
        TCommandLine* pCommandLine = mpHost->mpConsole->mpCommandLine;
        QVERIFY(pCommandLine);
        const auto clearTheLine = qScopeGuard([pCommandLine]() {
            pCommandLine->clear();
        });
        mpHost->setEnableSpellCheck(false);
        mpHost->setUserDictionaryOptions(true, false);
        pCommandLine->setPlainText(qsl("qzxfirst qzxsecond qzxthird"));
        QVERIFY(unmarked(pCommandLine, qsl("qzxfirst")));
        QVERIFY(unmarked(pCommandLine, qsl("qzxthird")));

        // the order dlgProfilePreferences applies them in
        mpHost->setEnableSpellCheck(true);
        mpHost->setUserDictionaryOptions(true, false);

        QVERIFY2(markedMisspelt(pCommandLine, qsl("qzxfirst")), "turning spell check on did not check the first word already in the line");
        QVERIFY2(markedMisspelt(pCommandLine, qsl("qzxsecond")), "turning spell check on did not check a word in the middle of the line");
        QVERIFY2(markedMisspelt(pCommandLine, qsl("qzxthird")), "turning spell check on did not check the last word already in the line");

        mpHost->setEnableSpellCheck(false);
        mpHost->setUserDictionaryOptions(true, false);

        QVERIFY2(unmarked(pCommandLine, qsl("qzxfirst")), "turning spell check off left a word marked");
        QVERIFY2(unmarked(pCommandLine, qsl("qzxthird")), "turning spell check off left a word marked");
    }

    void test_rightClickingAMisspeltWordOffersToAddIt()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);
        enterText(pCommandLine, qsl("qzxnewword"));
        QVERIFY(!mpHost->spellChecker().wordSet().contains(qsl("qzxnewword")));

        QMenu* pMenu = rightClick(pCommandLine, qsl("qzxnewword"));
        QVERIFY2(pMenu, "right-clicking the command line opened no menu");
        QAction* pAdd = findAction(pMenu, TCommandLine::tr("Add to user dictionary"));
        QAction* pRemove = findAction(pMenu, TCommandLine::tr("Remove from user dictionary"));
        QVERIFY2(pAdd && pRemove, qPrintable(actionTexts(pMenu).join(qsl(" | "))));
        QVERIFY2(pAdd->isEnabled(), "a word in neither dictionary could not be added");
        QVERIFY2(!pRemove->isEnabled(), "a word that is not in the user dictionary was offered for removal");
        // the profile's own dictionary has no suggestions for a word it has never seen
        QVERIFY2(findAction(pMenu, TCommandLine::tr("no suggestions (profile)")), qPrintable(actionTexts(pMenu).join(qsl(" | "))));

        pAdd->trigger();
        mAddedWords << qsl("qzxnewword");
        closeMenu(pMenu);

        QVERIFY2(mpHost->spellChecker().wordSet().contains(qsl("qzxnewword")), "choosing to add the word did not put it in the user dictionary");
        QVERIFY2(markedAsTheUsersOwn(pCommandLine, qsl("qzxnewword")), "the added word was not rechecked");
    }

    void test_rightClickingAUserDictionaryWordOffersToRemoveIt()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);
        addToUserDictionary(qsl("qzxoldword"));
        enterText(pCommandLine, qsl("qzxoldword"));
        QVERIFY(markedAsTheUsersOwn(pCommandLine, qsl("qzxoldword")));

        QMenu* pMenu = rightClick(pCommandLine, qsl("qzxoldword"));
        QVERIFY2(pMenu, "right-clicking the command line opened no menu");
        QAction* pAdd = findAction(pMenu, TCommandLine::tr("Add to user dictionary"));
        QAction* pRemove = findAction(pMenu, TCommandLine::tr("Remove from user dictionary"));
        QVERIFY2(pAdd && pRemove, qPrintable(actionTexts(pMenu).join(qsl(" | "))));
        QVERIFY2(!pAdd->isEnabled(), "a word already in the user dictionary was offered to be added again");
        QVERIFY2(pRemove->isEnabled(), "a word in the user dictionary could not be removed");

        pRemove->trigger();
        closeMenu(pMenu);

        QVERIFY2(!mpHost->spellChecker().wordSet().contains(qsl("qzxoldword")), "choosing to remove the word left it in the user dictionary");
        QVERIFY2(markedMisspelt(pCommandLine, qsl("qzxoldword")), "the removed word was not rechecked");
    }

    // A suggestion from the user dictionary replaces the word that was
    // right-clicked, and only that word
    void test_choosingASuggestionReplacesTheWord()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);
        // two letters swapped, which hunspell always tries
        addToUserDictionary(qsl("qzxforb"));
        enterText(pCommandLine, qsl("say qzxfrob now"));

        QMenu* pMenu = rightClick(pCommandLine, qsl("qzxfrob"));
        QVERIFY2(pMenu, "right-clicking the command line opened no menu");
        QAction* pSuggestion = findAction(pMenu, qsl("qzxforb"));
        QVERIFY2(pSuggestion, qPrintable(actionTexts(pMenu).join(qsl(" | "))));
        QVERIFY(pSuggestion->isEnabled());

        pSuggestion->trigger();
        closeMenu(pMenu);

        QCOMPARE(pCommandLine->toPlainText(), qsl("say qzxforb now"));
    }

    // Short words are not checked, so neither are they offered anything
    void test_rightClickingAShortWordOffersNoSpellings()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);
        enterText(pCommandLine, qsl("qz"));

        QMenu* pMenu = rightClick(pCommandLine, qsl("qz"));
        QVERIFY2(pMenu, "right-clicking the command line opened no menu");
        QVERIFY2(!findAction(pMenu, TCommandLine::tr("Add to user dictionary")), qPrintable(actionTexts(pMenu).join(qsl(" | "))));
        closeMenu(pMenu);
    }

    // Scripts add their own entries to the menu, which raise the event they
    // were given when chosen, and can take them away again
    void test_aMenuEntryFromAScriptRaisesItsEvent()
    {
        mpHost->setEnableSpellCheck(false);
        const QString name = freshCommandLineName();
        TCommandLine* pCommandLine = freshCommandLine(name);
        QVERIFY(pCommandLine);
        enterText(pCommandLine, qsl("look"));
        QVERIFY(runLua(qsl("cmdLineMenuEventSeen = ''\n"
                           "registerAnonymousEventHandler('cmdLineSpellTestMenuEvent', function(event) cmdLineMenuEventSeen = event end)\n"
                           "addCommandLineMenuEvent('%1', 'Do the thing', 'cmdLineSpellTestMenuEvent')")
                               .arg(name)));

        QMenu* pMenu = rightClick(pCommandLine, qsl("look"));
        QVERIFY2(pMenu, "right-clicking the command line opened no menu");
        QAction* pEntry = findAction(pMenu, qsl("Do the thing"));
        QVERIFY2(pEntry, qPrintable(actionTexts(pMenu).join(qsl(" | "))));
        QCOMPARE(luaGlobal("cmdLineMenuEventSeen"), QString());

        pEntry->trigger();
        closeMenu(pMenu);

        QCOMPARE(luaGlobal("cmdLineMenuEventSeen"), qsl("cmdLineSpellTestMenuEvent"));

        QVERIFY(runLua(qsl("removeCommandLineMenuEvent('%1', 'Do the thing')").arg(name)));
        pMenu = rightClick(pCommandLine, qsl("look"));
        QVERIFY2(pMenu, "right-clicking the command line opened no menu");
        QVERIFY2(!findAction(pMenu, qsl("Do the thing")), "a removed menu entry was still offered");
        closeMenu(pMenu);
    }

    // Every right-click builds a new menu, so one the user dismisses has to go
    // away together with its entries instead of piling up on the command line
    void test_aDismissedMenuIsDeletedWithItsEntries_data()
    {
        QTest::addColumn<bool>("chooseAnEntry");
        QTest::newRow("dismissed with Escape") << false;
        QTest::newRow("an entry chosen with Return") << true;
    }

    void test_aDismissedMenuIsDeletedWithItsEntries()
    {
        QFETCH(bool, chooseAnEntry);
        const QString name = freshCommandLineName();
        TCommandLine* pCommandLine = freshCommandLine(name);
        QVERIFY(pCommandLine);
        enterText(pCommandLine, qsl("qzxleaky"));
        QVERIFY(markedMisspelt(pCommandLine, qsl("qzxleaky")));
        QVERIFY(runLua(qsl("addCommandLineMenuEvent('%1', 'Leak check', 'cmdLineSpellTestLeakEvent')").arg(name)));
        const qsizetype actionsBefore = pCommandLine->findChildren<QAction*>().size();

        QList<QPointer<QObject>> built;
        for (int i = 0; i < 3; ++i) {
            QMenu* pMenu = rightClickLeavingOtherMenus(pCommandLine, qsl("qzxleaky"));
            QVERIFY2(pMenu, "right-clicking the command line opened no menu");
            QVERIFY2(findAction(pMenu, qsl("Add to user dictionary")), qPrintable(actionTexts(pMenu).join(qsl(" | "))));
            QAction* pEntry = findAction(pMenu, qsl("Leak check"));
            QVERIFY2(pEntry, qPrintable(actionTexts(pMenu).join(qsl(" | "))));
            built << pMenu;
            for (QAction* pAction : pMenu->actions()) {
                built << pAction;
            }
            if (chooseAnEntry) {
                pMenu->setActiveAction(pEntry);
                QTest::keyClick(pMenu, Qt::Key_Return);
            } else {
                QTest::keyClick(pMenu, Qt::Key_Escape);
            }
        }

        const auto stillAlive = [&built]() {
            return std::count_if(built.cbegin(), built.cend(), [](const QPointer<QObject>& pObject) {
                return !pObject.isNull();
            });
        };
        QTRY_COMPARE_WITH_TIMEOUT(stillAlive(), 0, 2000);
        QCOMPARE(pCommandLine->findChildren<QMenu*>().size(), 0);
        QCOMPARE(pCommandLine->findChildren<QAction*>().size(), actionsBefore);
    }
};

#include "CommandLineSpellCheckTest.moc"
MUDLET_GROUPED_TEST_MAIN(CommandLineSpellCheckTest)
