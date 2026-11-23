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
 * The status bar carries only the item counts and the last save. A caret report sent
 * by showMessage() with no timeout was painted over the counts: hideOrShow() passes
 * over widgets not yet visible, and the first report arrives before the window shows.
 *
 * Run with: ctest -R EditorStatusBarTest -V
 */

#include <QFontInfo>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QStatusBar>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostDialogs.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

class EditorStatusBarTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorStatusBar-Test-Profile");
    // Summed over three channels: antialiasing lands a glyph's darkest pixel a shade off
    static constexpr int scmInkTolerance = 24;

    QStatusBar* statusBar() const { return mpEditor->statusBar(); }

    static QRect placeInTheBar(const QWidget* pLabel, const QWidget* pBar) { return QRect(pLabel->mapTo(pBar, QPoint(0, 0)), pLabel->size()); }

    // A palette can say one thing and the style paint another, so the paint is read
    static QColor paintedInk(const QImage& shot, const QRect& rect)
    {
        // grab() is in device pixels, the rectangle in logical ones
        const qreal shotRatio = shot.devicePixelRatio();
        const QRect inShot =
                QRect(QPoint(qRound(rect.left() * shotRatio), qRound(rect.top() * shotRatio)), QPoint(qRound(rect.right() * shotRatio), qRound(rect.bottom() * shotRatio))).intersected(shot.rect());
        const QColor ground = shot.pixelColor(inShot.topLeft());
        QColor ink = ground;
        int furthest = -1;
        for (int y = inShot.top(); y <= inShot.bottom(); ++y) {
            for (int x = inShot.left(); x <= inShot.right(); ++x) {
                const QColor candidate = shot.pixelColor(x, y);
                const int distance = TestPixels::distance(candidate, ground);
                if (distance > furthest) {
                    furthest = distance;
                    ink = candidate;
                }
            }
        }
        return ink;
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showTriggerDialog();
        QCoreApplication::processEvents();
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1100, 800);
        // None of the bar's widgets read as shown until the window is exposed
        QVERIFY2(QTest::qWaitForWindowExposed(mpEditor, 2000), "the editor window was never put on screen, so nothing in its status bar is showing");
        QVERIFY2(statusBar() != nullptr, "The editor window has no status bar");
    }

    void init()
    {
        // Something for the counts to count and a document for the caret to move in
        mpEditor->slot_showTriggers();
        mpEditor->addTrigger(false);
        mpEditor->mpSourceEditorEdbee->textDocument()->setText(qsl("-- somewhere for the caret to be\n"));
        QCoreApplication::processEvents();
    }

    void cleanup()
    {
        if (mpEditor) {
            statusBar()->clearMessage();
        }
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_theCaretReadingIsNotAMessageOnTheBar()
    {
        edbee::TextEditorController* pController = mpEditor->mpSourceEditorEdbee->controller();
        QVERIFY2(pController != nullptr, "The code pane has no controller, so it can report no caret position");

        QSignalSpy reports(pController, &edbee::TextEditorController::updateStatusTextSignal);
        pController->moveCaretToOffset(3, false);
        QTRY_VERIFY2(!reports.isEmpty(), "the code pane reported no caret position, so there is nothing to keep off the bar");

        const QString message = statusBar()->currentMessage();
        QVERIFY2(message.isEmpty(), qPrintable(qsl("the caret position went out as a temporary message, which is painted over the counts: \"%1\"").arg(message)));

        QLabel* pCounts = mpEditor->mpLabel_statusCounts;
        QVERIFY2(pCounts != nullptr, "The status bar has no label for the item counts");
        QTRY_VERIFY2(pCounts->isVisible(), "the counts label is not on show");
        // Filled behind a 200ms wait, so that a fillout is one walk of the tree
        QTRY_VERIFY2(!pCounts->text().isEmpty(), "the counts label never filled");
        qInfo().noquote() << qsl("  the counts read \"%1\" at %2, with the bar carrying no message").arg(pCounts->text(), TestPixels::describe(placeInTheBar(pCounts, statusBar())));
    }

    // The counts sit on the message side on purpose, so a timed message can borrow their place
    void test_timedMessageBorrowsTheCountsThenReturnsThem()
    {
        QLabel* pCounts = mpEditor->mpLabel_statusCounts;
        QLabel* pAutosave = mpEditor->mpLabel_statusAutosave;
        QVERIFY2(pCounts != nullptr, "The status bar has no label for the item counts");
        QVERIFY2(pAutosave != nullptr, "the status bar has no label for the last save");
        QTRY_VERIFY2(pCounts->isVisible(), "the counts label is not on show, so there is nothing for a message to borrow");

        statusBar()->showMessage(qsl("x"), 150);
        QTRY_VERIFY2(!pCounts->isVisible(), "a timed message did not take the counts label's place");
        QVERIFY2(pAutosave->isVisible(), "a timed message took the last save away as well, so it is not a permanent widget");

        QTRY_VERIFY2(pCounts->isVisible(), "the counts label never came back once the message had timed out");
    }

    void test_theBarIsWrittenLikeTheSidebar()
    {
        QListWidget* pSidebar = mpEditor->mpListWidget_editorSidebar;
        QVERIFY2(pSidebar != nullptr, "The editor has no sidebar list to take the hand from");
        const QFontInfo sidebarHand(pSidebar->font());

        const QList<QLabel*> readings{mpEditor->mpLabel_statusCounts, mpEditor->mpLabel_statusAutosave};
        for (QLabel* pLabel : readings) {
            QVERIFY2(pLabel != nullptr, "The status bar is missing one of its two labels");
            const QFontInfo hand(pLabel->font());
            qInfo().noquote() << qsl("  %1 is set in %2 at %3pt, weight %4; the sidebar in %5 at %6pt, weight %7")
                                         .arg(pLabel->objectName(),
                                              hand.family(),
                                              QString::number(hand.pointSizeF()),
                                              QString::number(hand.weight()),
                                              sidebarHand.family(),
                                              QString::number(sidebarHand.pointSizeF()),
                                              QString::number(sidebarHand.weight()));
            QVERIFY2(hand.family() == sidebarHand.family() && qFuzzyCompare(hand.pointSizeF(), sidebarHand.pointSizeF()) && hand.weight() == sidebarHand.weight(),
                     qPrintable(qsl("%1 is not written in the sidebar's hand").arg(pLabel->objectName())));
        }

        // An unchosen sidebar row: the chosen one is painted in the selection's colours
        QTRY_VERIFY2(!mpEditor->mpLabel_statusCounts->text().isEmpty(), "the counts label never filled, so there is no ink to read");
        QVERIFY2(pSidebar->count() > 1 && pSidebar->currentRow() != 1, "the sidebar has no unchosen second row to read the ink from");
        const QImage shot = mpEditor->grab().toImage();
        const QRect nameRect = QRect(pSidebar->viewport()->mapTo(mpEditor, pSidebar->visualItemRect(pSidebar->item(1)).topLeft()), pSidebar->visualItemRect(pSidebar->item(1)).size());
        const QRect countsRect = QRect(mpEditor->mpLabel_statusCounts->mapTo(mpEditor, QPoint(0, 0)), mpEditor->mpLabel_statusCounts->size());
        const QColor nameInk = paintedInk(shot, nameRect);
        const QColor countsInk = paintedInk(shot, countsRect);
        qInfo().noquote() << qsl("  the sidebar's \"%1\" is painted in %2 and the counts in %3, %4 apart")
                                     .arg(pSidebar->item(1)->text(), nameInk.name(), countsInk.name(), QString::number(TestPixels::distance(nameInk, countsInk)));
        QVERIFY2(TestPixels::distance(nameInk, countsInk) <= scmInkTolerance, qPrintable(qsl("the bar is written in %1 while the sidebar's names are in %2").arg(countsInk.name(), nameInk.name())));
    }
    void test_aSaveTurnedAwayLeavesTheLastSavedTime()
    {
        QLabel* pAutosave = mpEditor->mpLabel_statusAutosave;
        QVERIFY2(pAutosave != nullptr, "the status bar has no label for the last save");
        QTRY_VERIFY2(!mpHost->currentlySavingProfile(), "an earlier save never finished");
        const QString before = qsl("no save yet");
        pAutosave->setText(before);

        // saveProfile() turns a save away while the profile is still loading
        mpHost->mIsProfileLoadingSequence = true;
        mpEditor->autoSave();
        mpHost->mIsProfileLoadingSequence = false;
        QCOMPARE(pAutosave->text(), before);

        mpEditor->autoSave();
        QVERIFY2(pAutosave->text() != before, "a save that went through did not say when");
        QTRY_VERIFY2(!mpHost->currentlySavingProfile(), "the save never finished");
    }
};

#include "EditorStatusBarTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorStatusBarTest)
