/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                                  *
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
 * The key routing dlgSourceEditorArea installs on edbee's autocomplete popup,
 * driven through the popup's own QWindow so the keys travel the way Qt
 * delivers them to a user.
 *
 * While a Qt::Popup is open, QWidgetWindow::handleKeyEvent() hands every key
 * to that popup, and QApplication::notify() then passes an *ignored* key on
 * to the ignored widget's parent - which for edbee's suggestion list is the
 * popup QMenu itself, because edbee parents the list to the menu. So a
 * navigation key the list cannot act on comes back up to the menu the routing
 * filter is installed on, and routing it to the list again is a loop.
 *
 * Reaching the end of a test is most of what the cases below assert: the loop
 * ends the process with a stack overflow rather than a failed comparison.
 *
 * QTest::keyClick() on the popup's windowHandle() is what exercises the path.
 * QApplication::sendEvent() straight to a widget skips popup routing
 * altogether, so a test built on it passes whether or not the bug is there.
 *
 * Run with: ctest -R EditorAutoCompleteFocusTest -V
 */

#include <QListWidget>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "dlgSourceEditorArea.h"
#include "mudlet.h"

#include "edbee/edbee.h"
#include "edbee/models/textautocompleteprovider.h"
#include "edbee/models/textdocument.h"
#include "edbee/models/texteditorconfig.h"
#include "edbee/texteditorwidget.h"
#include "edbee/views/components/texteditorautocompletecomponent.h"
#include "edbee/views/components/texteditorcomponent.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorAutoCompleteFocusTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    dlgSourceEditorArea* mpArea = nullptr;
    edbee::TextEditorWidget* mpEditor = nullptr;
    edbee::TextEditorComponent* mpEditorComponent = nullptr;
    QListWidget* mpList = nullptr;
    QWidget* mpMenu = nullptr;

    // A prefix no Lua function can collide with, so the popup's rows are
    // exactly the ones planted below. Three of the words match it, one does.
    static QString manyWordPrefix() { return qsl("qazauto"); }
    static QString oneWordPrefix() { return qsl("qazautoo"); }

    static bool popupIsOpen() { return QApplication::activePopupWidget() != nullptr; }

    void plantWords()
    {
        auto* provider = new edbee::StringTextAutoCompleteProvider();
        provider->add(qsl("qazautoone"), 3, qsl("qazautoone()"));
        provider->add(qsl("qazautotwo"), 3, qsl("qazautotwo()"));
        provider->add(qsl("qazautothree"), 3, qsl("qazautothree()"));
        edbee::Edbee::instance()->autoCompleteProviderList()->giveProvider(provider);
    }

    void closePopup()
    {
        if (auto* popup = QApplication::activePopupWidget()) {
            popup->close();
        }
        QTest::qWait(10ms);
    }

    // Types a prefix, which is what makes edbee refill the list and pop the
    // menu up. Reports rather than QVERIFYing, so the calling test can say
    // what state it got instead of leaving the rest of the case to measure
    // nothing.
    bool openPopupFor(const QString& prefix, int expectedRows)
    {
        closePopup();
        mpEditor->textDocument()->setText(QString());
        QTest::qWait(10ms);
        mpEditorComponent->setFocus();
        QTest::keyClicks(mpEditorComponent, prefix);

        if (!QTest::qWaitFor(&popupIsOpen, 2s)) {
            return false;
        }
        if (QApplication::activePopupWidget() != mpMenu) {
            return false;
        }
        return mpList->count() == expectedRows;
    }

    // The popup's own window, so Qt's popup routing is what delivers the key.
    // Reports rather than QVERIFYing: a popup that is not there would leave
    // the rest of the case measuring nothing.
    bool pressInPopup(Qt::Key key)
    {
        auto* popup = QApplication::activePopupWidget();
        if (!popup || popup != mpMenu || !popup->isVisible()) {
            return false;
        }
        auto* window = popup->windowHandle();
        if (!window) {
            return false;
        }
        QTest::keyClick(window, key);
        QTest::qWait(10ms);
        return true;
    }

    // The list the editor holds the keyboard for, which is the whole point of
    // the change: a suggestion popup must not take the keyboard away from the
    // text being edited.
    bool focusStayedWithEditor() const { return QApplication::focusWidget() == mpEditorComponent; }

    // The state a navigation key the list cannot act on should leave behind:
    // the current row unmoved, the popup still up, the editor still holding
    // the keyboard.
    bool endsWhereItStarted(int expectedRow) const { return mpList->currentRow() == expectedRow && mpMenu->isVisible() && focusStayedWithEditor(); }

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

        // The mudlet constructor is what primes the edbee singleton with the
        // Lua grammar and the Mudlet theme, so it has to run before an editor
        // can be built.
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->init();
        plantWords();

        mpArea = new dlgSourceEditorArea(nullptr);
        mpEditor = mpArea->edbeeEditorWidget;
        QVERIFY(mpEditor);
        mpEditorComponent = mpEditor->textEditorComponent();
        QVERIFY(mpEditorComponent);
        mpList = mpEditor->autoCompleteComponent()->listWidget();
        QVERIFY(mpList);
        // edbee parents the list to the menu, which is what makes an ignored
        // key travel from one to the other
        mpMenu = mpList->parentWidget();
        QVERIFY(mpMenu);

        mpEditor->config()->setAutocompleteAutoShow(true);
        mpArea->resize(640, 400);
        mpArea->show();
        QTRY_VERIFY(mpArea->isVisible());
        QTest::qWait(100ms);
    }

    void cleanupTestCase()
    {
        closePopup();
        delete mpArea;
        mpArea = nullptr;
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void cleanup() { closePopup(); }

    // edbee makes row 0 current every time it refills the list, so this is the
    // state a user is in the moment a prefix with more than one match has been
    // typed - no deliberate navigation needed to reach it.
    void test_upOnTheFirstSuggestion()
    {
        QVERIFY2(openPopupFor(manyWordPrefix(), 3), "the autocomplete popup never came up with the three planted suggestions");
        QCOMPARE(mpList->currentRow(), 0);
        QVERIFY2(focusStayedWithEditor(), "the popup took the keyboard focus off the editor before any key was pressed");

        QVERIFY2(pressInPopup(Qt::Key_Up), "the popup was not there to take the Up key");
        QVERIFY2(endsWhereItStarted(0), "Up on the first suggestion moved the selection, closed the popup or moved the focus");
    }

    void test_downOnTheLastSuggestion()
    {
        QVERIFY2(openPopupFor(manyWordPrefix(), 3), "the autocomplete popup never came up with the three planted suggestions");

        // walk to the end the way a user would, so the key under test is the
        // one that cannot move any further
        for (int i = 0, last = mpList->count() - 1; i < last; ++i) {
            QVERIFY2(pressInPopup(Qt::Key_Down), "the popup stopped taking Down keys before the last suggestion");
        }
        QCOMPARE(mpList->currentRow(), mpList->count() - 1);

        QVERIFY2(pressInPopup(Qt::Key_Down), "the popup was not there to take the second Down key");
        QVERIFY2(endsWhereItStarted(mpList->count() - 1), "Down on the last suggestion moved the selection, closed the popup or moved the focus");
    }

    void test_anArrowKeyWithOnlyOneSuggestion()
    {
        QVERIFY2(openPopupFor(oneWordPrefix(), 1), "the autocomplete popup never came up with the single planted suggestion");
        QCOMPARE(mpList->currentRow(), 0);

        QVERIFY2(pressInPopup(Qt::Key_Down), "the popup was not there to take the Down key");
        QVERIFY2(endsWhereItStarted(0), "Down on a one-suggestion list moved the selection, closed the popup or moved the focus");
    }

    void test_pageUpOnTheFirstSuggestion()
    {
        QVERIFY2(openPopupFor(manyWordPrefix(), 3), "the autocomplete popup never came up with the three planted suggestions");
        QCOMPARE(mpList->currentRow(), 0);

        QVERIFY2(pressInPopup(Qt::Key_PageUp), "the popup was not there to take the PageUp key");
        QVERIFY2(endsWhereItStarted(0), "PageUp on the first suggestion moved the selection, closed the popup or moved the focus");
    }

    // The counterpart to the four cases above: a key the list *can* act on
    // still reaches it. Dropping every key that comes back up from the list
    // would pass those four and leave the popup unnavigable.
    void test_navigationStillMovesTheSelection()
    {
        QVERIFY2(openPopupFor(manyWordPrefix(), 3), "the autocomplete popup never came up with the three planted suggestions");

        QVERIFY2(pressInPopup(Qt::Key_Down), "the popup was not there to take the first Down key");
        QCOMPARE(mpList->currentRow(), 1);
        QVERIFY2(focusStayedWithEditor(), "navigating the list moved the keyboard focus off the editor");

        QVERIFY2(pressInPopup(Qt::Key_Up), "the popup was not there to take the Up key");
        QCOMPARE(mpList->currentRow(), 0);
        QVERIFY2(focusStayedWithEditor(), "navigating the list moved the keyboard focus off the editor");
    }

    // The reason the routing exists: with the popup up, ordinary typing still
    // reaches the editor and the popup keeps tracking the word.
    void test_typingReachesTheEditorWhileThePopupIsOpen()
    {
        QVERIFY2(openPopupFor(manyWordPrefix(), 3), "the autocomplete popup never came up with the three planted suggestions");

        QVERIFY2(pressInPopup(Qt::Key_Down), "the popup was not there to take the Down key");
        QCOMPARE(mpList->currentRow(), 1);

        // a letter, delivered the way Qt delivers it while a popup is open
        auto* window = QApplication::activePopupWidget()->windowHandle();
        QVERIFY(window);
        QTest::keyClick(window, 't');
        QTest::qWait(10ms);

        QCOMPARE(mpEditor->textDocument()->text(), qsl("%1t").arg(manyWordPrefix()));
        QVERIFY2(QApplication::activePopupWidget() == mpMenu, "typing a letter closed the popup instead of narrowing it");
        QVERIFY2(focusStayedWithEditor(), "typing a letter moved the keyboard focus off the editor");
    }
};

#include "EditorAutoCompleteFocusTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorAutoCompleteFocusTest)
