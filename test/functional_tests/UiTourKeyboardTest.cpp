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
 * The interface tour is an overlay over the main window, and the card it draws
 * carries Skip/Back/Next buttons. Clicking one of them hands the keyboard
 * focus to that button. Keys the button ignores still reached the tour, by
 * propagating up the parent chain, but a QPushButton takes the arrow keys as
 * focus navigation and hands the focus to a widget the overlay covers - and
 * from there nothing reaches the tour at all, while PageUp splits the console
 * underneath. See issue #10876.
 *
 * Run with: ctest -R UiTourKeyboardTest -V
 */

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TLuaInterpreter.h"
#include "TUiTour.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QInputMethodEvent>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>

#include "GroupedTest.h"

class UiTourKeyboardTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;
    QPointer<TUiTour> mpTour;
    QPlainTextEdit* mpBehindOverlay = nullptr;

    QPushButton* cardButton(const QString& name) const { return mpTour->findChild<QPushButton*>(name); }

    QPushButton* skipButton() const { return cardButton(qsl("uiTourSkipButton")); }

    // Back is disabled on the first step and enabled on every other one, so it
    // says "first step or not" without depending on the wording of a step - it
    // does not say how far the tour has moved, which is why every case below
    // asserts where the tour started from as well
    QPushButton* backButton() const { return cardButton(qsl("uiTourBackButton")); }

    QPushButton* nextButton() const { return cardButton(qsl("uiTourNextButton")); }

    bool tourHasTheFocus() const
    {
        QWidget* pFocused = QApplication::focusWidget();
        return pFocused && mpTour->isAncestorOf(pFocused);
    }

    // A closed tour lingers until its deferred delete, so only a visible one counts as open
    static TUiTour* openTour()
    {
        for (auto* pTour : mudlet::self()->findChildren<TUiTour*>()) {
            if (pTour->isVisible()) {
                return pTour;
            }
        }
        return nullptr;
    }

    static int openTourCount()
    {
        int count = 0;
        for (auto* pTour : mudlet::self()->findChildren<TUiTour*>()) {
            if (pTour->isVisible()) {
                ++count;
            }
        }
        return count;
    }

    // The card's "3 of 6" label, which says which step the tour is on
    QString progress() const
    {
        static const QRegularExpression progressText(qsl("^\\d+ of \\d+$"));
        for (auto* pLabel : mpTour->findChildren<QLabel*>()) {
            if (progressText.match(pLabel->text()).hasMatch()) {
                return pLabel->text();
            }
        }
        return {};
    }

    // Closes init()'s tour first: openTour() would find it before the menu's
    // one, and the menu tour's filter, installed later, would take the keys
    // meant for it
    bool openTheTourFromTheHelpMenu()
    {
        delete mpTour.data();
        if (openTour()) {
            return false;
        }
        mudlet::self()->dactionUiTour->trigger();
        mpTour = openTour();
        return mpTour;
    }

    // Through the window rather than straight to a widget, and from a widget
    // the overlay covers, so each key takes the route a real key press does
    // and only the tour's application-wide filter can pick it out
    QStringList stepsShownPressingOnlyTheRightArrow()
    {
        QStringList stepsShown;
        while (mpTour && mpTour->isVisible() && stepsShown.size() < 100) {
            stepsShown << progress();
            mpBehindOverlay->setFocus();
            QTest::keyClick(mudlet::self()->windowHandle(), Qt::Key_Right);
        }
        return stepsShown;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }

        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(mXdgDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mXdgDir.path()))); // profiles/ = XDG opt-in
        qputenv("XDG_CONFIG_HOME", mXdgDir.path().toUtf8());

        // The tour reads the arrow keys the way the layout runs, so pin the
        // direction rather than inherit the environment's
        QGuiApplication::setLayoutDirection(Qt::LeftToRight);

        mudlet::start();
        mudlet::self()->setupConfig();
        QVERIFY(MudletApp::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        // activateProfile() needs the coordinator, and the last case activates one
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        // Builds the menu bar the tour's later steps point at, and connects
        // Help > Take a UI tour
        mudlet::self()->init();

        mudlet::self()->show();
        QVERIFY(QTest::qWaitForWindowExposed(mudlet::self()));
        // Not just exposed: setFocus() only reaches QApplication::focusWidget()
        // once the window is active, and every case here turns on where the
        // focus is
        QVERIFY(QTest::qWaitForWindowActive(mudlet::self()));
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mudlet::self();
    }

    void init()
    {
        // Stands in for the console for text entry: something under the
        // overlay that takes a key once the focus reaches it
        mpBehindOverlay = new QPlainTextEdit(mudlet::self());
        mpBehindOverlay->show();

        mpTour = new TUiTour(mudlet::self());
        mpTour->start();
        QVERIFY(mpTour->isVisible());
        QVERIFY2(!backButton()->isEnabled(), "the tour did not open on its first step");
        QCOMPARE(QApplication::focusWidget(), mpTour.data());
    }

    void cleanup()
    {
        // Every tour, not just mpTour: a case that fails before taking hold of
        // the tour the Help menu opened would otherwise leave it covering the
        // window for the next case
        qDeleteAll(mudlet::self()->findChildren<TUiTour*>(Qt::FindDirectChildrenOnly));
        delete mpBehindOverlay;
        mpBehindOverlay = nullptr;
    }

    void test_theTourAnswersTheArrowKeysBeforeAnythingIsClicked()
    {
        QTest::keyClick(mpTour, Qt::Key_Right);

        QVERIFY2(backButton()->isEnabled(), "the tour did not answer an arrow key on its first step");
    }

    void test_arrowKeysStillMoveTheTourAfterItsNextButtonIsClicked()
    {
        QTest::mouseClick(nextButton(), Qt::LeftButton);
        QVERIFY2(backButton()->isEnabled(), "clicking Next did not advance the tour");
        QCOMPARE(QApplication::focusWidget(), nextButton());

        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Left);

        QVERIFY2(!backButton()->isEnabled(), "the tour stopped answering the arrow keys once its Next button had been clicked");
    }

    void test_theArrowKeysFollowTheLayoutDirection()
    {
        QTest::mouseClick(nextButton(), Qt::LeftButton);
        QVERIFY2(backButton()->isEnabled(), "clicking Next did not advance the tour");

        QGuiApplication::setLayoutDirection(Qt::RightToLeft);
        QTest::keyClick(mpTour, Qt::Key_Right);
        QGuiApplication::setLayoutDirection(Qt::LeftToRight);

        QVERIFY2(!backButton()->isEnabled(), "a right to left layout did not turn the arrow keys around");
    }

    void test_escapeStillClosesTheTourOnceTheFocusHasLeftTheOverlay()
    {
        QTest::mouseClick(nextButton(), Qt::LeftButton);
        QVERIFY2(backButton()->isEnabled(), "clicking Next did not advance the tour");
        // Move the focus out of the overlay by hand, as the first arrow key
        // does in the running application. Without this the case passes
        // without the fix as well: with the focus still on a card button,
        // Escape propagates up the parent chain to the overlay
        mpBehindOverlay->setFocus();
        QCOMPARE(QApplication::focusWidget(), mpBehindOverlay);
        QSignalSpy finished(mpTour.data(), &TUiTour::signal_tourFinished);

        QTest::keyClick(mpBehindOverlay, Qt::Key_Escape);

        QVERIFY2(!mpTour->isVisible(), "Escape stopped closing the tour once the focus had left the overlay");
        // Once, not twice: the key is answered on the press, and the release
        // that follows it must not finish the tour a second time
        QCOMPARE(finished.count(), 1);
    }

    // An arrow key on a focused card button used to hand the focus on to the
    // widget behind the overlay, which is how PageUp ended up splitting the
    // console the tour was pointing at
    void test_keysDoNotReachTheWidgetsTheOverlayCovers()
    {
        QTest::mouseClick(nextButton(), Qt::LeftButton);
        QVERIFY2(backButton()->isEnabled(), "clicking Next did not advance the tour");
        mpBehindOverlay->setPlainText(QStringList(200, qsl("line")).join(QChar::LineFeed));
        mpBehindOverlay->moveCursor(QTextCursor::End);
        const int cursorAtTheEnd = mpBehindOverlay->textCursor().position();
        mpBehindOverlay->setFocus();
        QCOMPARE(QApplication::focusWidget(), mpBehindOverlay);

        QTest::keyClick(mpBehindOverlay, Qt::Key_X);
        QVERIFY2(!mpBehindOverlay->toPlainText().contains(QChar('x')), "a key press reached a widget behind the tour overlay");

        QTest::keyClick(mpBehindOverlay, Qt::Key_PageUp);
        QCOMPARE(mpBehindOverlay->textCursor().position(), cursorAtTheEnd);
        QVERIFY2(!backButton()->isEnabled(), "PageUp went past the tour instead of taking it back a step");

        QTest::keyClick(mpBehindOverlay, Qt::Key_PageDown);
        QCOMPARE(mpBehindOverlay->textCursor().position(), cursorAtTheEnd);
        QVERIFY2(backButton()->isEnabled(), "PageDown went past the tour instead of taking it on a step");
        // Taking the key is half of it: a screen reader would otherwise be
        // left announcing a covered widget while the tour moves
        QVERIFY2(tourHasTheFocus(), "the tour took a key from a covered widget but left the focus there");
    }

    void test_textFromAnInputMethodDoesNotReachTheWidgetsTheOverlayCovers()
    {
        mpBehindOverlay->setFocus();
        QCOMPARE(QApplication::focusWidget(), mpBehindOverlay);

        QInputMethodEvent commit;
        commit.setCommitString(qsl("x"));
        QApplication::sendEvent(mpBehindOverlay, &commit);

        QVERIFY2(mpBehindOverlay->toPlainText().isEmpty(), "input method text reached a widget behind the tour overlay");
    }

    void test_tabBringsTheFocusBackToTheTour()
    {
        mpBehindOverlay->setFocus();
        QCOMPARE(QApplication::focusWidget(), mpBehindOverlay);

        QTest::keyClick(mpBehindOverlay, Qt::Key_Tab);

        QVERIFY2(tourHasTheFocus(), "Tab left the focus stranded on a widget the overlay covers");
    }

    void test_spaceOnTheFocusedBackButtonGoesBackRatherThanForward()
    {
        QTest::mouseClick(nextButton(), Qt::LeftButton);
        QVERIFY2(backButton()->isEnabled(), "clicking Next did not advance the tour");
        backButton()->setFocus();
        QCOMPARE(QApplication::focusWidget(), backButton());

        QTest::keyClick(backButton(), Qt::Key_Space);

        QVERIFY2(!backButton()->isEnabled(), "Space on the focused Back button did not take the tour back");
    }

    void test_enterOnTheFocusedSkipButtonClosesTheTour()
    {
        skipButton()->setFocus();
        QCOMPARE(QApplication::focusWidget(), skipButton());

        QTest::keyClick(skipButton(), Qt::Key_Return);

        QVERIFY2(!mpTour->isVisible(), "Enter on the focused Skip button did not close the tour");
    }

    void test_anotherWindowKeepsItsKeyboardWhileTheTourIsUp()
    {
        // Unparented, so it is a window of its own - as the script editor and
        // a detached profile window are
        QPlainTextEdit anotherWindow;

        QTest::keyClick(&anotherWindow, Qt::Key_X);
        QCOMPARE(anotherWindow.toPlainText(), qsl("x"));

        QTest::keyClick(&anotherWindow, Qt::Key_Right);
        QVERIFY2(!backButton()->isEnabled(), "an arrow key in another window moved the tour");
    }

    void test_theKeyboardComesBackOnceTheTourIsGone()
    {
        mpBehindOverlay->setFocus();
        QCOMPARE(QApplication::focusWidget(), mpBehindOverlay);
        QTest::keyClick(mpBehindOverlay, Qt::Key_Escape);
        QVERIFY(!mpTour->isVisible());

        QTest::keyClick(mpBehindOverlay, Qt::Key_X);

        QVERIFY2(mpBehindOverlay->toPlainText() == qsl("x"), "the overlay went on eating keys after the tour closed");
    }

    void test_theHelpMenuActionOpensATourEveryTime()
    {
        QVERIFY2(openTheTourFromTheHelpMenu(), "Help > Take a UI tour opened no tour");
        QVERIFY2(!backButton()->isEnabled(), "Help > Take a UI tour did not open on the welcome step");
        QPointer<TUiTour> firstTour = mpTour;
        QTest::keyClick(firstTour, Qt::Key_Escape);
        QVERIFY2(!firstTour->isVisible(), "Escape did not close the tour the Help menu opened");
        QTRY_VERIFY2(firstTour.isNull(), "the closed tour was never deleted");

        mudlet::self()->dactionUiTour->trigger();
        mpTour = openTour();
        QVERIFY2(mpTour, "Help > Take a UI tour opened nothing the second time");
    }

    void test_theHelpMenuActionBringsBackTheOpenTourRatherThanOpeningAnother()
    {
        QVERIFY2(openTheTourFromTheHelpMenu(), "Help > Take a UI tour opened no tour");
        mpBehindOverlay->setFocus();
        QCOMPARE(QApplication::focusWidget(), mpBehindOverlay);

        mudlet::self()->dactionUiTour->trigger();

        QCOMPARE(openTourCount(), 1);
        QVERIFY2(tourHasTheFocus(), "Help > Take a UI tour left the focus on a widget the open tour covers");
    }

    void test_theRightArrowAloneTakesTheTourToItsEnd()
    {
        QVERIFY2(openTheTourFromTheHelpMenu(), "Help > Take a UI tour opened no tour");
        QSignalSpy finished(mpTour.data(), &TUiTour::signal_tourFinished);

        // No profile is loaded yet, so the game window and input line steps
        // have nothing to point at and are passed over
        QCOMPARE(stepsShownPressingOnlyTheRightArrow(), (QStringList{qsl("1 of 6"), qsl("4 of 6"), qsl("5 of 6"), qsl("6 of 6")}));
        QCOMPARE(finished.count(), 1);
        QTRY_VERIFY2(!openTour(), "a tour was still open after the last step");
    }

    // Last, as the profile it loads stays for the rest of the run
    void test_withAProfileTheTourVisitsEveryStepAndTellsTheProfileItFinished()
    {
        mudlet::self()->setStorePasswordsSecurely(false);
        const QString profileName = qsl("UiTourKeyboard-Test");
        QVERIFY2(HostManager::self()->addHost(profileName, QString(), QString(), QString()), "failed to put a profile in the pool");
        Host* pHost = HostManager::self()->getHost(profileName);
        QVERIFY(pHost);
        mudlet::self()->addConsoleForNewHost(pHost);
        mudlet::self()->activateProfile(pHost);
        QCOMPARE(mudlet::self()->getActiveHost(), pHost);
        auto* pLua = pHost->getLuaInterpreter();
        QVERIFY(pLua->compileAndExecuteScript(qsl("mudlet = mudlet or {} mudlet.uiTourPending = true")));
        QVERIFY2(openTheTourFromTheHelpMenu(), "Help > Take a UI tour opened no tour");

        QCOMPARE(stepsShownPressingOnlyTheRightArrow(), (QStringList{qsl("1 of 6"), qsl("2 of 6"), qsl("3 of 6"), qsl("4 of 6"), qsl("5 of 6"), qsl("6 of 6")}));
        QVERIFY2(!openTour(), "the right arrow key did not take the tour to its end");
        QVERIFY2(pLua->compileAndExecuteScript(qsl("assert(mudlet.uiTourPending == false)")), "finishing the tour did not tell the profile");
    }
};

#include "UiTourKeyboardTest.moc"
MUDLET_GROUPED_TEST_MAIN(UiTourKeyboardTest)
