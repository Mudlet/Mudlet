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

#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "TUiTour.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QInputMethodEvent>
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
        QVERIFY(MudletPaths::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));

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
        delete mpTour.data();
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
};

#include "UiTourKeyboardTest.moc"
MUDLET_GROUPED_TEST_MAIN(UiTourKeyboardTest)
