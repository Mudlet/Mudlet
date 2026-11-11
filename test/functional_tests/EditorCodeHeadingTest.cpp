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
 * The code pane's heading carries the caret reading and the last save's compile note.
 * The strip is the splitter handle and transparent to the mouse so a drag on it still
 * resizes: the handle hears a click on the note, and a drag that came back is no click.
 *
 * Run with: ctest -R EditorCodeHeadingTest -V
 */

#include <QLabel>
#include <QMouseEvent>
#include <QScrollBar>
#include <QSplitterHandle>
#include <QStatusBar>
#include <QTreeWidget>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostDialogs.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "TScript.h"
#include "TLuaInterpreter.h"
#include "ScriptUnit.h"
#include "dlgSystemMessageArea.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggerPatternEdit.h"
#include "EditorLineMarginDelegate.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

class EditorCodeHeadingTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorCodeHeading-Test-Profile");

    // The item is line one of its chunk, and Lua counts lines from one, edbee from zero
    static constexpr int scmBrokenLine = 2;
    static constexpr int scmBrokenLineInEdbee = scmBrokenLine - 1;
    // The heading's own inset, and a pixel or two of rounding
    static constexpr int scmTrailingEdgeSlack = 8;
    // Lua's error quotes all of an unfinished string, so this sets the note's length too
    static constexpr int scmLongStringLength = 300;
    static constexpr int scmLongLineInEdbee = 1;
    static constexpr int scmInkTolerance = 24;
    static constexpr int scmMarkRounding = 3;
    // Past any platform's drag threshold
    static constexpr int scmDragTravel = 60;
    // Each step short of the drag threshold on its own, as a real drag's moves are
    static constexpr int scmDragStep = 3;

    // handle(1): the code pane is the second of the three panes splitter_right stacks
    QSplitterHandle* codeHeadingHandle() const { return mpEditor->splitter_right->handle(1); }
    // Found by object name, not member, so a build missing them fails a check rather than the compile
    QWidget* heading() const { return mpEditor->findChild<QWidget*>(qsl("editorCodeHeader")); }
    QLabel* caretReading() const { return mpEditor->findChild<QLabel*>(qsl("editorCodeCaret")); }
    QWidget* compileNote() const { return mpEditor->findChild<QWidget*>(qsl("editorCompileNote")); }
    QLabel* compileMessage() const { return mpEditor->findChild<QLabel*>(qsl("editorCompileMessage")); }
    // The note takes a second line before it is cut, so each line has to fit on its own
    static int widestLine(const QLabel* pLabel, const QString& text)
    {
        int widest = 0;
        for (const QString& line : text.split(QLatin1Char('\n'))) {
            widest = std::max(widest, pLabel->fontMetrics().horizontalAdvance(line));
        }
        return widest;
    }
    QLabel* compileMark() const { return mpEditor->findChild<QLabel*>(qsl("editorCompileMark")); }

    QString bannerReads() const { return mpEditor->mpSystemMessageArea->notificationAreaMessageBox->text(); }

    edbee::TextEditorController* controller() const { return mpEditor->mpSourceEditorEdbee->controller(); }

    int caretLine() const
    {
        const size_t caret = controller()->textSelection()->range(0).caret();
        return static_cast<int>(controller()->textDocument()->lineFromOffset(caret));
    }

    int caretOffset() const { return static_cast<int>(controller()->textSelection()->range(0).caret()); }

    // A note longer than any strip, on a line whose end takes more characters to name than its start
    static QString longBrokenScript() { return qsl("local a = 1\nlocal b = \"%1\n").arg(QString(scmLongStringLength, QLatin1Char('a'))); }

    // QTest::mouseMove reports no button down, and a splitter follows only a held one.
    // The global point leads: the handle moves out from under the pointer as panes resize.
    static void dragTo(QWidget* pHandle, const QPointF& global)
    {
        QMouseEvent move(QEvent::MouseMove, pHandle->mapFromGlobal(global), global, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(pHandle, &move);
    }

    QRect placeOnTheStrip(const QWidget* pPiece) const { return QRect(pPiece->mapTo(heading(), QPoint(0, 0)), pPiece->size()); }

    static QString describeSizes(const QList<int>& sizes)
    {
        QStringList words;
        for (const int size : sizes) {
            words << QString::number(size);
        }
        return words.join(QLatin1Char('/'));
    }

    // Rebuilt as fillout_form() does, for a script made after the tree. Signals held:
    // clear() reports a selection change while destroying rows, and the editor loads it
    void rebuildScriptRows()
    {
        {
            const QSignalBlocker quiet(mpEditor->treeWidget_scripts);
            mpEditor->treeWidget_scripts->clear();
            mpEditor->mpCurrentScriptItem = nullptr;
            mpEditor->mpScriptsBaseItem = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(qsl("Scripts")));
            mpEditor->treeWidget_scripts->insertTopLevelItem(0, mpEditor->mpScriptsBaseItem);
            mpEditor->mpSystemMessageArea->hide();
            mpEditor->populateScripts();
            mpEditor->mpScriptsBaseItem->setExpanded(true);
        }
        QCoreApplication::processEvents();
    }

    struct MarginInk
    {
        int inMark = 0;
        int inNumber = 0;
    };

    // Pixels of the error ink in one line's band of the margin, in the mark and past it where the number is
    MarginInk errorInkInMarginLine(const size_t line) const
    {
        edbee::TextMarginComponent* pMargin = mpEditor->mpSourceEditorEdbee->textMarginComponent();
        edbee::TextRenderer* pRenderer = pMargin->renderer();
        const QColor ink = mpEditor->mpEditorLineMargin->errorInk();
        const int top = pRenderer->yPosForLine(line) - mpEditor->mpSourceEditorEdbee->verticalScrollBar()->value();
        const QImage grab = TestPixels::shot(pMargin);
        const QRectF mark = mpEditor->mpEditorLineMargin->markRect();
        MarginInk found;
        for (int y = top; y < top + pRenderer->lineHeight(); ++y) {
            for (int x = 0; x < pMargin->width(); ++x) {
                if (TestPixels::readsAs(TestPixels::sampled(grab, QPoint(x, y)), ink, scmInkTolerance)) {
                    ++(x <= mark.right() ? found.inMark : found.inNumber);
                }
            }
        }
        return found;
    }

    void typeAndSave(const QString& script)
    {
        mpEditor->mpSourceEditorEdbee->textDocument()->setText(script);
        mpEditor->slot_saveEdits();
        QCoreApplication::processEvents();
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
        // A pattern, or the item cannot be saved clean whatever its Lua says: a trigger
        // with none reports that through the same channel a failed compile does
        mpEditor->mTriggerPatternEdit.at(0)->singleLineTextEdit_pattern->setPlainText(qsl("EditorCodeHeading"));
        mpEditor->mpSourceEditorEdbee->textDocument()->setText(qsl("-- somewhere for the caret to be\n"));
        QCoreApplication::processEvents();
    }

    void cleanup()
    {
        if (mpEditor) {
            mpEditor->resize(1200, 800);
            QCoreApplication::processEvents();
        }
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

    // Which handle shows is the splitter's business, so the pieces hanging off it are read
    void test_theHeadingGoesWithThePane()
    {
        QSplitterHandle* pHandle = codeHeadingHandle();
        QLabel* pCaret = caretReading();
        QWidget* pNote = compileNote();
        QVERIFY2(pCaret != nullptr, "the code pane's heading has no label named editorCodeCaret on it");
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QVERIFY2(pHandle->isAncestorOf(pCaret), "the caret reading does not hang off the handle over the code pane, so nothing takes it away with the pane");
        QVERIFY2(pHandle->isAncestorOf(pNote), "the compile note does not hang off the handle over the code pane, so nothing takes it away with the pane");

        QTRY_VERIFY2(pHandle->isVisible(), "the code pane's heading is not on show while a trigger is being edited");
        QTRY_VERIFY2(pCaret->isVisible(), "the caret reading is not on show while a trigger is being edited");

        // A note hidden anyway would pass whether or not it rides on the pane. The pane is
        // hidden by hand because clearing the form also clears the compile state.
        typeAndSave(qsl("local a = 1\nlocal b = 1 +* 2\n"));
        QTRY_VERIFY2(pNote->isVisible(), "a save that did not compile left no note on the heading");

        mpEditor->mpSourceEditorArea->hide();
        QCoreApplication::processEvents();
        QTRY_VERIFY2(!pHandle->isVisible(), "the heading stayed behind after the code pane it belongs to was taken away");
        QVERIFY2(!pCaret->isVisible(), "the caret reading stayed on screen after the code pane it belongs to was taken away");
        QVERIFY2(!pNote->isVisible(), "the compile note stayed on screen after the code pane it belongs to was taken away");

        mpEditor->mpSourceEditorArea->show();
        QCoreApplication::processEvents();
        QTRY_VERIFY2(pHandle->isVisible(), "the heading did not come back with the code pane");
        QVERIFY2(pCaret->isVisible(), "the caret reading did not come back with the code pane");
        QVERIFY2(pNote->isVisible(), "the compile note did not come back with the code pane, so the heading's pieces are being shown by hand rather than riding on it");

        mpEditor->clearTriggerForm();
        QTRY_VERIFY2(!pHandle->isVisible(), "clearing the form left the heading behind");
        QVERIFY2(!pCaret->isVisible(), "clearing the form left the caret reading on screen");

        mpEditor->addTrigger(false);
        QTRY_VERIFY2(pHandle->isVisible(), "the heading did not come back with the code pane");
        QVERIFY2(pCaret->isVisible(), "the caret reading did not come back with the code pane");
    }

    void test_aCleanSaveShowsNoNote()
    {
        typeAndSave(qsl("-- nothing wrong here\n"));

        QWidget* pNote = compileNote();
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QTRY_VERIFY2(!pNote->isVisible(), qPrintable(qsl("a save that compiled left a note on the heading reading \"%1\"").arg(compileMessage() ? compileMessage()->text() : QString())));
        QVERIFY2(codeHeadingHandle()->toolTip().isEmpty(), qPrintable(qsl("a save that compiled left a tooltip on the heading: \"%1\"").arg(codeHeadingHandle()->toolTip())));

        QLabel* pCaret = caretReading();
        QVERIFY2(pCaret != nullptr, "the code pane's heading has no label named editorCodeCaret on it");
        const QRect place = placeOnTheStrip(pCaret);
        QVERIFY2(
                place.right() >= heading()->width() - scmTrailingEdgeSlack,
                qPrintable(qsl("with no note on it the caret reading is at %1 on a %2px strip, which is not its trailing edge").arg(TestPixels::describe(place), QString::number(heading()->width()))));
    }

    // The chunk name only repeats the item the reader is looking at, so it goes to the tooltip
    void test_aFailedSaveNamesTheLine()
    {
        typeAndSave(qsl("local a = 1\nlocal b = 1 +* 2\n"));

        QWidget* pNote = compileNote();
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QTRY_VERIFY2(pNote->isVisible(), "a save that did not compile left no note on the heading");

        QLabel* pMessage = compileMessage();
        QVERIFY2(pMessage != nullptr, "the compile note has no label named editorCompileMessage in it");
        const QString said = pMessage->text();
        const QString tip = codeHeadingHandle()->toolTip();
        qInfo().noquote() << qsl("  the note reads \"%1\" and the tooltip \"%2\"").arg(said, tip);

        QVERIFY2(said.contains(qsl("%1:").arg(QString::number(scmBrokenLine))), qPrintable(qsl("the note does not name the line the compiler stopped on: \"%1\"").arg(said)));
        QVERIFY2(said.contains(qsl("unexpected symbol")), qPrintable(qsl("the note does not carry what the compiler said: \"%1\"").arg(said)));
        QVERIFY2(!said.contains(qsl("[string")) && !said.contains(qsl("Lua syntax error")), qPrintable(qsl("the note repeats the compiler's own preamble: \"%1\"").arg(said)));
        QVERIFY2(tip.contains(qsl("[string")), qPrintable(qsl("the whole of what the compiler said is not on the heading's tooltip: \"%1\"").arg(tip)));
    }

    void test_clickingTheNoteMovesTheCaretToTheLine()
    {
        typeAndSave(qsl("local a = 1\nlocal b = 1 +* 2\n"));

        QWidget* pNote = compileNote();
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QTRY_VERIFY2(pNote->isVisible(), "a save that did not compile left no note to click on");

        controller()->moveCaretToOffset(0, false);
        QCOMPARE(caretLine(), 0);

        QSplitterHandle* pHandle = codeHeadingHandle();
        const QList<int> before = mpEditor->splitter_right->sizes();
        QTest::mouseClick(pHandle, Qt::LeftButton, Qt::NoModifier, pNote->mapTo(pHandle, pNote->rect().center()));

        QTRY_COMPARE(caretLine(), scmBrokenLineInEdbee);
        QCOMPARE(mpEditor->splitter_right->sizes(), before);
    }

    // The note is a link even when the handle drags nothing, so the handle still has to hear the click
    void test_theNoteIsStillALinkOnAHeadingThatDragsNothing()
    {
        typeAndSave(qsl("local a = 1\nlocal b = 1 +* 2\n"));

        QWidget* pNote = compileNote();
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QTRY_VERIFY2(pNote->isVisible(), "a save that did not compile left no note to click on");

        controller()->moveCaretToOffset(0, false);
        QCOMPARE(caretLine(), 0);

        QSplitterHandle* pHandle = codeHeadingHandle();
        QVERIFY2(mpEditor->splitter_right->setHandleResizes(1, false), "the code pane's heading is not a handle this can be asked of");
        const QList<int> before = mpEditor->splitter_right->sizes();
        QTest::mouseClick(pHandle, Qt::LeftButton, Qt::NoModifier, pNote->mapTo(pHandle, pNote->rect().center()));

        QTRY_COMPARE(caretLine(), scmBrokenLineInEdbee);
        QCOMPARE(mpEditor->splitter_right->sizes(), before);
        QVERIFY(mpEditor->splitter_right->setHandleResizes(1, true));
    }

    void test_theNoteYieldsToTheCaretReading()
    {
        typeAndSave(qsl("local a = 1\nlocal b = 1 +* 2\n"));

        QLabel* pCaret = caretReading();
        QLabel* pMessage = compileMessage();
        QVERIFY2(pCaret != nullptr, "the code pane's heading has no label named editorCodeCaret on it");
        QVERIFY2(pMessage != nullptr, "the compile note has no label named editorCompileMessage in it");

        mpEditor->resize(mpEditor->minimumSizeHint());
        QCoreApplication::processEvents();

        QTRY_VERIFY2(pCaret->width() >= pCaret->sizeHint().width(), "the strip never made room for the caret reading's words");

        const QString said = pMessage->text();
        const bool elided = said.endsWith(QChar(0x2026));
        const int wanted = widestLine(pMessage, said);
        qInfo().noquote() << qsl("  on a %1px strip the note reads \"%2\" (%3px in a %4px label) and the caret reading \"%5\" (%6px)")
                                     .arg(QString::number(heading()->width()), said, QString::number(wanted), QString::number(pMessage->width()), pCaret->text(), QString::number(pCaret->width()));
        QVERIFY2(elided || wanted <= pMessage->width(),
                 qPrintable(qsl("the note reads \"%1\", which wants %2px of a %3px label without being cut to fit").arg(said, QString::number(wanted), QString::number(pMessage->width()))));

        const QRect caret = placeOnTheStrip(pCaret);
        const QRect note = placeOnTheStrip(pMessage);
        QVERIFY2(!caret.intersects(note), qPrintable(qsl("the note is at %1 and the caret reading at %2, which is the same place").arg(TestPixels::describe(note), TestPixels::describe(caret))));
    }

    // The strip itself is not resized, so eliding only on resize leaves the tail clipped mid-letter
    void test_theNoteIsCutAgainWhenTheCaretReadingGrows()
    {
        typeAndSave(longBrokenScript());

        QLabel* pMessage = compileMessage();
        QLabel* pCaret = caretReading();
        QVERIFY2(pMessage != nullptr, "the compile note has no label named editorCompileMessage in it");
        QVERIFY2(pCaret != nullptr, "the code pane's heading has no label named editorCodeCaret on it");
        QTRY_VERIFY2(compileNote() != nullptr && compileNote()->isVisible(), "a save that did not compile left no note on the heading");

        controller()->moveCaretToOffset(0, false);
        QTRY_VERIFY2(pMessage->text().endsWith(QChar(0x2026)),
                     qPrintable(qsl("the note reads \"%1\" in a %2px label, so it was never cut - there is nothing here to cut again").arg(pMessage->text(), QString::number(pMessage->width()))));
        const QString wide = pMessage->text();
        const int roomWhenWide = pMessage->width();
        const QString shortReading = pCaret->text();

        controller()->moveCaretTo(static_cast<size_t>(scmLongLineInEdbee), static_cast<size_t>(scmLongStringLength), false);
        QTRY_VERIFY2(pMessage->width() < roomWhenWide,
                     qPrintable(qsl("the caret reading went from \"%1\" to \"%2\" without taking any of the note's %3px").arg(shortReading, pCaret->text(), QString::number(roomWhenWide))));

        const QString narrow = pMessage->text();
        const int wanted = widestLine(pMessage, narrow);
        qInfo().noquote() << qsl("  the reading went from \"%1\" to \"%2\", the note's room from %3px to %4px, and the note now wants %5px of it")
                                     .arg(shortReading, pCaret->text(), QString::number(roomWhenWide), QString::number(pMessage->width()), QString::number(wanted));
        QVERIFY2(narrow != wide,
                 qPrintable(qsl("the note still reads what it was cut to for a %1px label in the %2px one it now has").arg(QString::number(roomWhenWide), QString::number(pMessage->width()))));
        QVERIFY2(narrow.endsWith(QChar(0x2026)), qPrintable(qsl("the note reads \"%1\", which does not say it was cut").arg(narrow)));
        QVERIFY2(wanted <= pMessage->width(),
                 qPrintable(qsl("the note wants %1px of a %2px label, so its last words are clipped rather than cut").arg(QString::number(wanted), QString::number(pMessage->width()))));
    }

    void test_theNoteCanBeFollowedFromTheKeyboard()
    {
        typeAndSave(qsl("local a = 1\nlocal b = 1 +* 2\n"));

        QWidget* pNote = compileNote();
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QTRY_VERIFY2(pNote->isVisible(), "a save that did not compile left no note to reach");

        mpEditor->activateWindow();
        pNote->setFocus(Qt::TabFocusReason);
        QTRY_VERIFY2(pNote->hasFocus(), "the note cannot hold the keyboard focus, so the mouse is the only way to the line it names");

        controller()->moveCaretToOffset(0, false);
        QCOMPARE(caretLine(), 0);

        QTest::keyClick(pNote, Qt::Key_Return);
        QTRY_COMPARE(caretLine(), scmBrokenLineInEdbee);
    }

    // Lua reports an unclosed block at the end of the wrapper the item is compiled in,
    // a line past the item's last; the jump lands at the start of the last line
    void test_anUnclosedBlockJumpsToTheLastLine()
    {
        typeAndSave(qsl("if true then\n  print('x')"));

        QWidget* pNote = compileNote();
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QTRY_VERIFY2(pNote->isVisible(), "an unclosed block was saved with no note on the heading");

        const int lines = static_cast<int>(controller()->textDocument()->lineCount());
        qInfo().noquote() << qsl("  the note reads \"%1\" and names line %2 of a document that has %3")
                                     .arg(compileMessage() ? compileMessage()->text() : QString(), QString::number(mpEditor->mEditorCompileErrorLine), QString::number(lines));
        QVERIFY2(mpEditor->mEditorCompileErrorLine > lines,
                 qPrintable(qsl("the compiler stopped on line %1 of a %2 line document, which is a line the reader can see - so this case is not the one past the end")
                                    .arg(QString::number(mpEditor->mEditorCompileErrorLine), QString::number(lines))));

        controller()->moveCaretToOffset(0, false);
        QCOMPARE(caretLine(), 0);

        QSplitterHandle* pHandle = codeHeadingHandle();
        QTest::mouseClick(pHandle, Qt::LeftButton, Qt::NoModifier, pNote->mapTo(pHandle, pNote->rect().center()));

        QTRY_COMPARE(caretLine(), lines - 1);
        const int startOfTheLastLine = static_cast<int>(controller()->textDocument()->offsetFromLine(static_cast<size_t>(lines - 1)));
        QVERIFY2(caretOffset() == startOfTheLastLine,
                 qPrintable(qsl("the caret is at offset %1 rather than %2, where the last line starts - the line asked for was past the end of the document and was followed there")
                                    .arg(QString::number(caretOffset()), QString::number(startOfTheLastLine))));
    }

    void test_aDragThatCameBackIsNotAClick()
    {
        typeAndSave(qsl("local a = 1\nlocal b = 1 +* 2\n"));

        QWidget* pNote = compileNote();
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QTRY_VERIFY2(pNote->isVisible(), "a save that did not compile left no note to press on");

        controller()->moveCaretToOffset(0, false);
        QCOMPARE(caretLine(), 0);

        QSplitterHandle* pHandle = codeHeadingHandle();
        const QList<int> before = mpEditor->splitter_right->sizes();
        const QPoint on = pNote->mapTo(pHandle, pNote->rect().center());
        const QPointF pressedAt = pHandle->mapToGlobal(QPointF(on));

        QTest::mousePress(pHandle, Qt::LeftButton, Qt::NoModifier, on);
        dragTo(pHandle, pressedAt + QPointF(0, scmDragTravel));
        dragTo(pHandle, pressedAt);
        QTest::mouseRelease(pHandle, Qt::LeftButton, Qt::NoModifier, pHandle->mapFromGlobal(pressedAt).toPoint());
        QCoreApplication::processEvents();

        QVERIFY2(caretLine() == 0, qPrintable(qsl("the caret moved to line %1, so a drag of the panes that came back to where it started was heard as a click").arg(QString::number(caretLine()))));
        QCOMPARE(mpEditor->splitter_right->sizes(), before);
    }

    // The handle follows each small move, so against the handle the pointer never
    // travels; only screen coordinates show the press was a drag
    void test_aDragOfSmallStepsIsNotAClickEither()
    {
        typeAndSave(qsl("local a = 1\nlocal b = 1 +* 2\n"));

        QWidget* pNote = compileNote();
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QTRY_VERIFY2(pNote->isVisible(), "a save that did not compile left no note to press on");

        controller()->moveCaretToOffset(0, false);
        QCOMPARE(caretLine(), 0);

        QSplitterHandle* pHandle = codeHeadingHandle();
        const QList<int> before = mpEditor->splitter_right->sizes();
        const QPoint on = pNote->mapTo(pHandle, pNote->rect().center());
        const QPointF pressedAt = pHandle->mapToGlobal(QPointF(on));

        QTest::mousePress(pHandle, Qt::LeftButton, Qt::NoModifier, on);
        QPointF at = pressedAt;
        for (int travelled = 0; travelled < scmDragTravel; travelled += scmDragStep) {
            at += QPointF(0, scmDragStep);
            dragTo(pHandle, at);
        }
        QTest::mouseRelease(pHandle, Qt::LeftButton, Qt::NoModifier, pHandle->mapFromGlobal(at).toPoint());
        QCoreApplication::processEvents();

        const QList<int> after = mpEditor->splitter_right->sizes();
        qInfo().noquote() << qsl("  the panes went from %1 to %2 over %3 steps of %4px")
                                     .arg(describeSizes(before), describeSizes(after), QString::number(scmDragTravel / scmDragStep), QString::number(scmDragStep));
        QVERIFY2(after != before, "the panes did not move, so the drag never reached the splitter and the case proves nothing");
        QVERIFY2(caretLine() == 0, qPrintable(qsl("the caret moved to line %1, so a drag of the panes made of small steps was heard as a click").arg(QString::number(caretLine()))));
    }

    void test_aFailedSaveRaisesNoBanner()
    {
        typeAndSave(qsl("local a = 1\nlocal b = 1 +* 2\n"));

        QWidget* pNote = compileNote();
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QTRY_VERIFY2(pNote->isVisible(), "a save that did not compile left no note on the heading");
        QTRY_VERIFY2(!mpEditor->mpSystemMessageArea->isVisible(), qPrintable(qsl("a save that did not compile raised the notice over the form as well, reading \"%1\"").arg(bannerReads())));
    }

    void test_theNoteComesBackWhenTheItemIsOpenedAgain()
    {
        typeAndSave(qsl("local a = 1\nlocal b = 1 +* 2\n"));

        QWidget* pNote = compileNote();
        QLabel* pMessage = compileMessage();
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QVERIFY2(pMessage != nullptr, "the compile note has no label named editorCompileMessage in it");
        QTRY_VERIFY2(pNote->isVisible(), "a save that did not compile left no note on the heading");
        const QString saidAfterTheSave = pMessage->text();
        QTreeWidgetItem* pBroken = mpEditor->mpCurrentTriggerItem;
        QVERIFY2(pBroken != nullptr, "no trigger is open, so there is nothing for this case to come back to");

        // Saved clean, so the open item rather than the switch itself quiets the heading
        mpEditor->addTrigger(false);
        mpEditor->mTriggerPatternEdit.at(0)->singleLineTextEdit_pattern->setPlainText(qsl("EditorCodeHeadingSecond"));
        typeAndSave(qsl("-- nothing wrong here\n"));
        QVERIFY2(mpEditor->mpCurrentTriggerItem != pBroken, "the second trigger was never opened, so nothing was switched away from");
        QTRY_VERIFY2(!pNote->isVisible(), qPrintable(qsl("the note followed the reader onto an item that compiled, reading \"%1\"").arg(pMessage->text())));

        mpEditor->slot_triggerSelected(pBroken);
        QTRY_VERIFY2(pNote->isVisible(), "opening the broken item again left the heading saying nothing about it");
        qInfo().noquote() << qsl("  the save said \"%1\" and opening the item again says \"%2\"").arg(saidAfterTheSave, pMessage->text());
        QCOMPARE(mpEditor->mEditorCompileErrorLine, scmBrokenLine);
        QVERIFY2(pMessage->text().contains(qsl("%1:").arg(QString::number(scmBrokenLine))),
                 qPrintable(qsl("the note the item was opened with does not name the line the save stopped on: \"%1\"").arg(pMessage->text())));
    }

    // A profile can arrive holding an item that never compiled, untouched by the editor
    void test_aBrokenItemOpensWithItsNoteAndNoBanner()
    {
        auto* pBroken = new TScript(qsl("A script that never compiled"), mpHost);
        pBroken->setScript(qsl("this is not lua ((\n"));
        pBroken->registerScript();
        QVERIFY2(!pBroken->state(), "the script this case meant to leave broken compiles after all");

        mpEditor->slot_showScripts();
        QCoreApplication::processEvents();
        rebuildScriptRows();

        QVERIFY2(!mpEditor->mpSystemMessageArea->isVisible(),
                 qPrintable(qsl("building the rows raised the notice over the form for a broken item nobody had opened, reading \"%1\"").arg(bannerReads())));

        QTreeWidgetItem* pRow = TestPixels::rowFor(mpEditor->mpScriptsBaseItem, pBroken->getID());
        QVERIFY2(pRow != nullptr, "the broken script has no row in the Scripts tree");
        mpEditor->slot_scriptsSelected(pRow);

        QWidget* pNote = compileNote();
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QTRY_VERIFY2(pNote->isVisible(), "opening an item that has never compiled left the heading saying nothing about it");
        QVERIFY2(!mpEditor->mpSystemMessageArea->isVisible(),
                 qPrintable(qsl("opening a broken item raised the notice over the form as well as the note on the heading, reading \"%1\"").arg(bannerReads())));
    }

    // A second line before the note is cut, so the compiler's whole sentence is read at a common window width
    void test_aNoteTooLongForOneLineGoesOntoASecond()
    {
        typeAndSave(qsl("if true then\nlocal a = 1\n"));
        QTRY_VERIFY2(compileNote() != nullptr && compileNote()->isVisible(), "a save that did not compile left no note on the heading");
        QLabel* pMessage = compileMessage();
        const QString whole = mpEditor->mEditorCompileNoteText;
        const QFontMetrics metrics = pMessage->fontMetrics();
        const int needed = metrics.horizontalAdvance(whole);
        // Room for a little over half of it, so it needs a second line but not a third
        mpEditor->resize(mpEditor->width() - pMessage->width() + needed * 6 / 10, mpEditor->height());
        QCoreApplication::processEvents();
        QTRY_VERIFY2(pMessage->width() < needed, qPrintable(qsl("SETUP: the note's %1px still hold its %2px").arg(QString::number(pMessage->width()), QString::number(needed))));
        QVERIFY2(2 * pMessage->width() > needed + 8 * metrics.averageCharWidth(), "SETUP: the note needs three lines here, so a cut is due anyway");

        const QString said = pMessage->text();
        QVERIFY2(!said.contains(QChar(0x2026)), qPrintable(qsl("the note was cut to \"%1\" where a second line holds it").arg(said)));
        QCOMPARE(said.simplified(), whole.simplified());
        QVERIFY2(widestLine(pMessage, said) <= pMessage->width(), qPrintable(qsl("a line of \"%1\" is wider than the %2px label").arg(said, QString::number(pMessage->width()))));
        QTRY_VERIFY2(pMessage->height() >= 2 * metrics.lineSpacing(),
                     qPrintable(qsl("the heading did not grow to hold the second line: the label is %1px tall").arg(QString::number(pMessage->height()))));
    }

    // setScript() replaces the code shown, so what was said about the old code goes with it
    void test_setScriptThatCompilesTakesTheNoteAndTheMarkAway()
    {
        const QString name = qsl("A script setScript repairs");
        auto* pBroken = new TScript(name, mpHost);
        pBroken->setScript(qsl("local a = 1\nlocal b = 1 +* 2\n"));
        pBroken->registerScript();
        QVERIFY2(!pBroken->state(), "SETUP: the script this case repairs compiles already");
        mpEditor->slot_showScripts();
        QCoreApplication::processEvents();
        rebuildScriptRows();
        mpEditor->slot_scriptsSelected(TestPixels::rowFor(mpEditor->mpScriptsBaseItem, pBroken->getID()));
        QTRY_VERIFY2(compileNote()->isVisible(), "SETUP: opening the broken script left no note");
        QCOMPARE(mpEditor->mpEditorLineMargin->errorLine(), scmBrokenLine);

        const QString repaired = qsl("local a = 1\nlocal b = 1 + 2\n");
        const auto [id, why] = mpHost->getLuaInterpreter()->setScriptCode(name, repaired, 0);
        QVERIFY2(id == pBroken->getID(), qPrintable(why));
        QCOMPARE(mpEditor->mpSourceEditorEdbeeDocument->text(), repaired);
        QVERIFY2(!compileNote()->isVisible(), qPrintable(qsl("the note still reads \"%1\" over code that compiles").arg(compileMessage()->text())));
        QCOMPARE(mpEditor->mpEditorLineMargin->errorLine(), 0);
    }

    // The heading describes the open item's own code, not a child painted along with it
    void test_aBrokenChildsErrorStaysOffItsOpenFolder()
    {
        const QString folderName = qsl("A folder holding a broken script");
        auto* pFolder = new TScript(folderName, mpHost);
        pFolder->setIsFolder(true);
        pFolder->registerScript();
        auto* pBroken = new TScript(pFolder, mpHost);
        pBroken->setName(qsl("The folder's broken script"));
        pBroken->setScript(qsl("local a = 1\nbroken broken or {}\n"));
        pBroken->registerScript();
        QVERIFY2(!pBroken->state() && pFolder->state(), "SETUP: the child compiles, or the folder does not");
        mpEditor->slot_showScripts();
        QCoreApplication::processEvents();
        rebuildScriptRows();
        QTreeWidgetItem* pFolderRow = TestPixels::rowFor(mpEditor->mpScriptsBaseItem, pFolder->getID());
        QVERIFY2(pFolderRow != nullptr && pFolderRow->childCount() == 1, "SETUP: the folder has no row, or not one child under it");
        mpEditor->slot_scriptsSelected(pFolderRow);
        QTRY_VERIFY2(!compileNote()->isVisible(), "SETUP: the open folder's heading already carries a note");

        const int flushes = mpEditor->mScriptIconFlushCount;
        // What Lua's disableScript() runs
        QVERIFY(mpHost->getScriptUnit()->disableScript(folderName));
        QTRY_COMPARE(mpEditor->mScriptIconFlushCount, flushes + 1);
        QVERIFY2(!compileNote()->isVisible(), qPrintable(qsl("the open folder's heading took its child's error: \"%1\"").arg(compileMessage()->text())));
        QCOMPARE(mpEditor->mpEditorLineMargin->errorLine(), 0);
    }

    // Regression: a body that failed as the profile loaded opened under a notice calling the error
    // fixed, and closing that notice forgot the error, so the item read as clean until opened again
    void test_aScriptThatFailedAsTheProfileLoadedOpensWithItsNote()
    {
        auto* pBroken = new TScript(qsl("A script that failed as the profile loaded"), mpHost);
        pBroken->setScript(qsl("broken broken or {}\n"));
        // What ScriptUnit::compileAll(true) keeps of the same failure while a profile loads
        pBroken->setLoadingError(pBroken->getError());
        pBroken->registerScript();
        QVERIFY2(!pBroken->state() && pBroken->getLoadingError(), "the script this case meant to leave failed as the profile loaded compiles after all");
        auto* pClean = new TScript(qsl("A script beside it that compiles"), mpHost);
        pClean->setScript(qsl("-- nothing wrong here\n"));
        pClean->registerScript();

        mpEditor->slot_showScripts();
        QCoreApplication::processEvents();
        rebuildScriptRows();
        QTreeWidgetItem* pBrokenRow = TestPixels::rowFor(mpEditor->mpScriptsBaseItem, pBroken->getID());
        QTreeWidgetItem* pCleanRow = TestPixels::rowFor(mpEditor->mpScriptsBaseItem, pClean->getID());
        QVERIFY2(pBrokenRow != nullptr && pCleanRow != nullptr, "the two scripts have no rows in the Scripts tree");

        QWidget* pNote = compileNote();
        QLabel* pMessage = compileMessage();
        QVERIFY2(pNote != nullptr && pMessage != nullptr, "the code pane's heading has no compile note on it");
        mpEditor->slot_scriptsSelected(pBrokenRow);
        QTRY_VERIFY2(pNote->isVisible(), "opening a script that failed as the profile loaded left the heading saying nothing about it");
        QVERIFY2(!mpEditor->mpSystemMessageArea->isVisible(), qPrintable(qsl("opening it raised the notice over the form as well, reading \"%1\"").arg(bannerReads())));
        const QString saidOnOpening = pMessage->text();

        mpEditor->showWarning(qsl("A notice about something else"));
        QVERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "the notice this case closes never came up");
        mpEditor->mpSystemMessageArea->messageAreaCloseButton->click();
        QVERIFY2(!mpEditor->mpSystemMessageArea->isVisible(), "the notice's close button left it up");
        QVERIFY2(pNote->isVisible(), "closing a notice about something else took the note off the heading");

        mpEditor->slot_scriptsSelected(pCleanRow);
        QTRY_VERIFY2(!pNote->isVisible(), qPrintable(qsl("the note followed the reader onto a script that compiles, reading \"%1\"").arg(pMessage->text())));
        mpEditor->slot_scriptsSelected(pBrokenRow);
        QTRY_VERIFY2(pNote->isVisible(), "after a notice was closed, opening the script again left the heading saying nothing about it");
        QCOMPARE(pMessage->text(), saidOnOpening);
        QVERIFY2(!mpEditor->mpSystemMessageArea->isVisible(), qPrintable(qsl("opening it again raised the notice over the form, reading \"%1\"").arg(bannerReads())));
    }

    // Regression: the package notice was given only to an item that compiled, from when the error shared its place
    void test_aBrokenItemInAPackageOpensWithTheNoteAndThePackageNotice()
    {
        auto* pPackage = new TScript(qsl("A package with a broken script"), mpHost);
        pPackage->setIsFolder(true);
        pPackage->mPackageName = qsl("EditorCodeHeadingBrokenPackage");
        pPackage->registerScript();
        auto* pBroken = new TScript(pPackage, mpHost);
        pBroken->setName(qsl("The package's broken script"));
        pBroken->setScript(qsl("broken broken or {}\n"));
        pBroken->registerScript();
        QVERIFY2(!pBroken->state(), "the script this case meant to leave broken compiles after all");
        QVERIFY2(!pBroken->packageName(pBroken).isEmpty(), "the broken script is not read as part of a package, so no package notice is due");

        mpEditor->slot_showScripts();
        QCoreApplication::processEvents();
        rebuildScriptRows();
        QTreeWidgetItem* pPackageRow = TestPixels::rowFor(mpEditor->mpScriptsBaseItem, pPackage->getID());
        QVERIFY2(pPackageRow != nullptr, "the package has no row in the Scripts tree");
        pPackageRow->setExpanded(true);
        QTreeWidgetItem* pBrokenRow = TestPixels::rowFor(pPackageRow, pBroken->getID());
        QVERIFY2(pBrokenRow != nullptr, "the broken script has no row under its package");
        mpEditor->slot_scriptsSelected(pBrokenRow);

        QWidget* pNote = compileNote();
        QVERIFY2(pNote != nullptr, "the code pane's heading has no widget named editorCompileNote on it");
        QTRY_VERIFY2(pNote->isVisible(), "opening a broken script in a package left the heading saying nothing about it");
        QTRY_VERIFY2(mpEditor->mpSystemMessageArea->isVisible(), "opening a broken script in a package raised no notice that it belongs to one");
        QVERIFY2(bannerReads().contains(qsl("part of a package")), qPrintable(qsl("the notice over the form reads \"%1\", not the package notice").arg(bannerReads())));
    }

    // The line the note names is found among the line numbers too, by the trees' error mark and the note's ink
    void test_theLineTheCompileStoppedOnIsMarkedAmongTheLineNumbers()
    {
        edbee::TextMarginComponent* pMargin = mpEditor->mpSourceEditorEdbee->textMarginComponent();
        QVERIFY2(mpEditor->mpEditorLineMargin != nullptr && pMargin->delegate() == mpEditor->mpEditorLineMargin, "the code pane's line numbers are not drawn by the editor's own margin delegate");
        typeAndSave(qsl("-- nothing wrong here\n-- nor here\n"));
        const int cleanWidth = pMargin->width();

        typeAndSave(qsl("local a = 1\nlocal b = 1 +* 2\n"));
        QTRY_VERIFY2(compileNote()->isVisible(), "a save that did not compile left no note on the heading, so there is no line to mark");
        QCOMPARE(mpEditor->mpEditorLineMargin->errorLine(), scmBrokenLine);
        const MarginInk marked = errorInkInMarginLine(scmBrokenLineInEdbee);
        const MarginInk unmarked = errorInkInMarginLine(0);
        qInfo().noquote()
                << qsl("  error ink in the margin: %1 pixels in the mark and %2 in the number on line %3, %4 and %5 on line 1")
                           .arg(QString::number(marked.inMark), QString::number(marked.inNumber), QString::number(scmBrokenLine), QString::number(unmarked.inMark), QString::number(unmarked.inNumber));
        QVERIFY2(marked.inMark > 0, "the line the compiler stopped on has no mark beside its number");
        QVERIFY2(marked.inNumber > 0, "the number of the line the compiler stopped on is not written in the error ink");
        QVERIFY2(unmarked.inMark == 0 && unmarked.inNumber == 0, "a line the compiler did not stop on is marked as well");
        QCOMPARE(pMargin->width(), cleanWidth);

        typeAndSave(qsl("local a = 1\nlocal b = 1 + 2\n"));
        QTRY_VERIFY2(!compileNote()->isVisible(), "a save that compiled left the note on the heading");
        const MarginInk cleared = errorInkInMarginLine(scmBrokenLineInEdbee);
        QVERIFY2(cleared.inMark == 0 && cleared.inNumber == 0, "a save that compiled left the line it once stopped on marked");
        QCOMPARE(pMargin->width(), cleanWidth);
    }

    // The raw state colour is a fill rather than an ink, a different red from the words
    void test_theNoteAndTheMarkAreWrittenInTheErrorInk()
    {
        typeAndSave(qsl("local a = 1\nlocal b = 1 +* 2\n"));

        QLabel* pMessage = compileMessage();
        QLabel* pMark = compileMark();
        QVERIFY2(pMessage != nullptr, "the compile note has no label named editorCompileMessage in it");
        QVERIFY2(pMark != nullptr, "the compile note has no label named editorCompileMark in it");
        QTRY_VERIFY2(compileNote() != nullptr && compileNote()->isVisible(), "a save that did not compile left no note on the heading");

        // Both appearances: on the one that needs no walk the raw state colour equals the ink
        const auto appearanceBefore = mudlet::self()->mAppearance;
        auto restore = qScopeGuard([appearanceBefore]() {
            mudlet::self()->setAppearance(appearanceBefore);
        });

        const QList<QPair<QString, enums::Appearance>> appearances{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}};
        QStringList misses;
        QStringList measured;
        for (const auto& appearance : appearances) {
            TestPixels::setAppearanceAndWait(appearance.second);

            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            const QColor red = uiDesign::errorInk(tokens);
            // QStyleSheetStyle::polish() puts a stylesheet's "color:" rule into the palette
            const QColor ink = pMessage->palette().color(pMessage->foregroundRole());
            const QImage markImage = pMark->pixmap().toImage().convertToFormat(QImage::Format_ARGB32);
            // Not QColor(), whose alpha reads as opaque
            QColor markInk = QColor::fromRgba(0);
            for (int y = 0; y < markImage.height(); ++y) {
                for (int x = 0; x < markImage.width(); ++x) {
                    if (markImage.pixelColor(x, y).alpha() > markInk.alpha()) {
                        markInk = markImage.pixelColor(x, y);
                    }
                }
            }
            measured << qsl("%1: the error ink is %2, the note is written in %3, and the mark in %4").arg(appearance.first, red.name(), ink.name(), TestPixels::inkOf(markImage));
            if (ink.name() != red.name()) {
                misses << qsl("%1: the note is written in %2 rather than in the error ink %3").arg(appearance.first, ink.name(), red.name());
            }
            // A thin stroke is never fully opaque, and un-premultiplying a pixel short of that rounds a channel by a step
            if (markInk.alpha() <= 0 || !TestPixels::readsAs(markInk, red, scmMarkRounding)) {
                misses << qsl("%1: the mark is drawn in %2 rather than in the error ink %3").arg(appearance.first, TestPixels::inkOf(markImage), red.name());
            }
        }
        qInfo().noquote() << qsl("  %1").arg(measured.join(qsl("\n  ")));
        QVERIFY2(misses.isEmpty(), qPrintable(qsl("\n  %1").arg(misses.join(qsl("\n  ")))));
    }
};

#include "EditorCodeHeadingTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorCodeHeadingTest)
