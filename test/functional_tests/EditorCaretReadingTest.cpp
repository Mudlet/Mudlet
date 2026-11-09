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
 * The caret reading belongs on the code pane's heading: as a QStatusBar message with
 * no timeout it painted over the item counts, since hideOrShow() hides only widgets
 * already visible and the first caret report arrives before the window is shown.
 *
 * Run with: ctest -R EditorCaretReadingTest -V
 */

#include <QLabel>
#include <QSplitterHandle>
#include <QStatusBar>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggerPatternEdit.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorCaretReadingTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorCaretReading-Test-Profile");

    // handle(1): the code pane is the second of the three panes splitter_right stacks
    QSplitterHandle* codeHeadingHandle() const { return mpEditor->splitter_right->handle(1); }
    // Found by object name, not member, so a build missing them fails a check rather than the compile
    QWidget* heading() const { return mpEditor->findChild<QWidget*>(qsl("editorCodeHeader")); }
    QLabel* caretReading() const { return mpEditor->findChild<QLabel*>(qsl("editorCodeCaret")); }

    edbee::TextEditorController* controller() const { return mpEditor->mpSourceEditorEdbee->controller(); }

    QRect placeOnTheStrip(const QWidget* pPiece) const { return QRect(pPiece->mapTo(heading(), QPoint(0, 0)), pPiece->size()); }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showTriggerDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1200, 800);
        // Visibility and placement read nothing real until the window is exposed
        QVERIFY2(QTest::qWaitForWindowExposed(mpEditor, 2000), "the editor window was never put on screen, so nothing on its code pane's heading is showing");
        QVERIFY2(codeHeadingHandle() != nullptr, "The right hand splitter has no handle over the code pane");
        QVERIFY2(heading() != nullptr, "The handle over the code pane carries no heading");
    }

    void init()
    {
        mpEditor->slot_showTriggers();
        mpEditor->addTrigger(false);
        // A pattern, or the item cannot be saved clean whatever its Lua says
        mpEditor->mTriggerPatternEdit.at(0)->singleLineTextEdit_pattern->setPlainText(qsl("EditorCaretReading"));
        mpEditor->mpSourceEditorEdbee->textDocument()->setText(qsl("-- somewhere for the caret to be\n"));
        QTest::qWait(50ms);
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_theCaretReadingSitsOnTheHeading()
    {
        QVERIFY2(controller() != nullptr, "The code pane has no controller, so it can report no caret position");

        QSignalSpy reports(controller(), &edbee::TextEditorController::updateStatusTextSignal);
        controller()->moveCaretToOffset(3, false);
        QTRY_VERIFY2(!reports.isEmpty(), "the code pane reported no caret position, so there is nothing for the heading to show");

        QLabel* pCaret = caretReading();
        QVERIFY2(pCaret != nullptr, "the code pane's heading has no label named editorCodeCaret on it");
        QTRY_VERIFY2(pCaret->isVisible(), "the caret reading is not on show");

        const QString lastReport = reports.last().first().toString();
        const QString caretText = pCaret->text();
        qInfo().noquote() << qsl("  the pane reported \"%1\" and the heading reads \"%2\"").arg(lastReport, caretText);
        QVERIFY2(!caretText.isEmpty() && lastReport.contains(caretText) && !caretText.contains(qsl(" | ")),
                 qPrintable(qsl("the caret reading is \"%1\", which is not the front of the code pane's report \"%2\"").arg(caretText, lastReport)));
        // Cut the debugging tail: it names the last command run, which has "Offset" in it
        const QString reported = lastReport.section(qsl(" | "), 0, 0);
        QVERIFY2(!reported.contains(qsl("Offset")),
                 qPrintable(qsl("the code pane is still reporting a byte offset, which is a debugging reading rather than one a script author uses: \"%1\"").arg(reported)));

        const QRect place = placeOnTheStrip(pCaret);
        QVERIFY2(heading()->rect().contains(place),
                 qPrintable(qsl("the caret reading is at %1, which is not on the %2 strip").arg(TestPixels::describe(place), TestPixels::describe(heading()->rect()))));

        const QString message = mpEditor->statusBar()->currentMessage();
        QVERIFY2(message.isEmpty(), qPrintable(qsl("the caret position went out as a status bar message, which is painted over the counts: \"%1\"").arg(message)));
    }

    void test_theHeadingGoesWithThePane()
    {
        QSplitterHandle* pHandle = codeHeadingHandle();
        QTRY_VERIFY2(pHandle->isVisible(), "the code pane's heading is not on show while a trigger is being edited");

        mpEditor->clearTriggerForm();
        QTRY_VERIFY2(!pHandle->isVisible(), "the heading stayed behind after the code pane it belongs to was taken away");

        mpEditor->addTrigger(false);
        QTRY_VERIFY2(pHandle->isVisible(), "the heading did not come back with the code pane");
    }
};

#include "EditorCaretReadingTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorCaretReadingTest)
