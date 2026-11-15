/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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
 * Guards the notepad's design-language shell: its two sheets, the send strip's
 * three readings, the tab chips and their crosses, the glyphs, and the question
 * before a note with text in it is destroyed.
 *
 * Run with: ctest -R NotepadShellTest -V
 */

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProgressBar>
#include <QTabBar>
#include <QTextCursor>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QtTest/QtTest>


#include "Host.h"
#include "HostDialogs.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgNotepad.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class NotepadShellTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    Host* mpHost = nullptr;
    dlgNotepad* mpNotepad = nullptr;
    const QString mProfileName = qsl("NotepadShell-Test-Profile");

    // Per channel: a styled fill is composited rather than blitted
    static constexpr int scmFillTolerance = 6;

    QString centralSheet() const { return mpNotepad->centralwidget->styleSheet(); }
    QString toolBarSheet() const { return mpNotepad->toolBar->styleSheet(); }

    QPlainTextEdit* currentNote() const { return qobject_cast<QPlainTextEdit*>(mpNotepad->tabWidget->currentWidget()); }

    // None empty: the counts on the two buttons count non-empty lines
    void writeNote(const int lines)
    {
        QStringList text;
        for (int line = 1; line <= lines; ++line) {
            text << qsl("line %1").arg(QString::number(line));
        }
        currentNote()->setPlainText(text.join(QChar::LineFeed));
        QCoreApplication::processEvents();
    }

    void selectFromTheTop(const int lines)
    {
        QTextCursor cursor = currentNote()->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, lines);
        currentNote()->setTextCursor(cursor);
        QCoreApplication::processEvents();
    }

    void clearTheSelection()
    {
        QTextCursor cursor = currentNote()->textCursor();
        cursor.movePosition(QTextCursor::Start);
        currentNote()->setTextCursor(cursor);
        QCoreApplication::processEvents();
    }

    QString wordsOn(const QString& labelName) const
    {
        auto* pLabel = mpNotepad->findChild<QLabel*>(labelName);
        return pLabel ? pLabel->text() : QString();
    }

    // The action, not its toolbar widget, says whether the bar carries it
    static bool onShow(const QAction* pAction) { return pAction && pAction->isVisible(); }

    // Found on the field rather than through the member, so a cue never added
    // to the field fails
    QAction* fieldCue() const
    {
        const QList<QAction*> onTheField = mpNotepad->lineEdit_prependText->actions();
        for (QAction* pAction : onTheField) {
            if (pAction->objectName() == qsl("notepadPrefixWarning")) {
                return pAction;
            }
        }
        return nullptr;
    }

    void nothingIsAFragment() const
    {
        const QString words = wordsOn(qsl("notepadNoPrefixText"));
        QVERIFY2(!words.contains(QChar(0x2026)) && !words.endsWith(qsl("...")), qPrintable(qsl("the warning is cut to a fragment rather than shown whole or not at all: \"%1\"").arg(words)));
    }

    // Settled, so the strip has measured again for the new width
    void atWidth(const int width) const
    {
        mpNotepad->resize(width, 520);
        TestPixels::settle();
    }

    void theCrossOnTabIsTheRecipes(const int index) const
    {
        QTabBar* pTabBar = mpNotepad->tabWidget->tabBar();
        QWidget* pTrailing = pTabBar->tabButton(index, QTabBar::RightSide);
        QWidget* pCross = pTrailing ? pTrailing : pTabBar->tabButton(index, QTabBar::LeftSide);
        QVERIFY2(pCross, qPrintable(qsl("tab %1 carries no cross on either side of its word").arg(QString::number(index))));

        const QSize wanted(uiDesign::scmTabCloseBoxSize, uiDesign::scmTabCloseBoxSize);
        QVERIFY2(pCross->size() == wanted,
                 qPrintable(qsl("tab %1: a cross drawn into a %2px box the platform sized is stretched past the %3px mark the recipe draws")
                                    .arg(QString::number(index), QString::number(pCross->size().width()), QString::number(uiDesign::scmTabCloseGlyphSize))));

        // The gap between chips comes off the trailing side of the tab's rect
        const QRect tab = pTabBar->tabRect(index);
        const QRect chip = tab.adjusted(0, 0, -uiDesign::scmTabGap, 0);
        const int padding = uiDesign::scmInputBorderWidth + uiDesign::scmTabPaddingHorizontal;
        const int stands = pTrailing ? chip.right() - pCross->geometry().right() : pCross->geometry().left() - chip.left();
        QVERIFY2(stands == padding,
                 qPrintable(qsl("the cross on tab %1 sits %2px from the chip's edge where the word has %3px of padding")
                                    .arg(QString::number(index), QString::number(stands), QString::number(uiDesign::scmTabPaddingHorizontal))));

        const int offCentre = std::abs(pCross->geometry().center().y() - tab.center().y());
        QVERIFY2(offCentre <= 1, qPrintable(qsl("the cross on tab %1 sits %2px off the chip's centre line, where the word is").arg(QString::number(index), QString::number(offCentre))));
    }

    // 20ms ticks the answerer waits for a box, and puts up with one still shown
    // after it was answered. It must give up: a box nothing presses would hang
    // the case inside the modal loop until ctest kills it
    static constexpr int scmBoxAnswerTicks = 100;

    enum class TheAnswer { Keep, Discard };

    QTimer* mpBoxAnswerer = nullptr;
    QPointer<QMessageBox> mpAnsweredBox;
    int mBoxesSeen = 0;
    QString mBoxComplaint;

    // activeModalWidget() is Qt's bookkeeping, not the platform plugin's, so it
    // works offscreen; the sweep over the top levels is a fallback
    static QMessageBox* visibleMessageBox()
    {
        if (auto* pModal = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
            return pModal;
        }
        const QList<QWidget*> topLevels = QApplication::topLevelWidgets();
        for (QWidget* pWidget : topLevels) {
            if (auto* pBox = qobject_cast<QMessageBox*>(pWidget); pBox && pBox->isVisible()) {
                return pBox;
            }
        }
        return nullptr;
    }

    // QMessageBox::exec() spins its own loop, so the answer is armed before the
    // close. A QPointer empties when the answered box (a local of the call under
    // test) goes, so a second box at the same address is still counted
    void armTheAnswer(const TheAnswer answer)
    {
        mBoxesSeen = 0;
        mBoxComplaint.clear();
        mpAnsweredBox.clear();
        delete mpBoxAnswerer;
        mpBoxAnswerer = new QTimer(this);
        mpBoxAnswerer->setInterval(20ms);
        auto ticks = std::make_shared<int>(0);
        connect(mpBoxAnswerer, &QTimer::timeout, this, [this, answer, ticks]() {
            QMessageBox* pBox = visibleMessageBox();
            if (!pBox) {
                if (++*ticks > scmBoxAnswerTicks) {
                    mpBoxAnswerer->stop();
                }
                return;
            }

            if (pBox == mpAnsweredBox) {
                if (++*ticks > scmBoxAnswerTicks) {
                    mBoxComplaint = qsl("the box stayed up after it was answered");
                    mpBoxAnswerer->stop();
                    pBox->close();
                }
                return;
            }

            *ticks = 0;
            ++mBoxesSeen;
            mpAnsweredBox = pBox;

            QAbstractButton* pKeep = pBox->button(QMessageBox::Cancel);
            if (!pKeep) {
                mBoxComplaint = qsl("the box carries no Cancel, so there is no way to keep the note");
                pBox->close();
                return;
            }
            if (pBox->defaultButton() != pKeep || pBox->escapeButton() != pKeep) {
                mBoxComplaint = qsl("Cancel is not both the default and the escape button, so Return or Escape destroys the note");
            }

            if (answer == TheAnswer::Keep) {
                pKeep->click();
                return;
            }

            const QList<QAbstractButton*> onTheBox = pBox->buttons();
            for (QAbstractButton* pButton : onTheBox) {
                if (pBox->buttonRole(pButton) == QMessageBox::DestructiveRole) {
                    pButton->click();
                    return;
                }
            }
            mBoxComplaint = qsl("the box carries no button in the destructive role, so nothing on it closes the note");
            pBox->close();
        });
        mpBoxAnswerer->start();
    }

    void disarmTheAnswer()
    {
        if (mpBoxAnswerer) {
            mpBoxAnswerer->stop();
        }
    }

private slots:
    // A failed case must not leave the answerer pressing the next case's dialogs
    void cleanup() { disarmTheAnswer(); }

    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));
        // Some assertions match English text
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));

        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        // The way mudlet's Notes button opens it
        mudlet::self()->slot_notes();
        mpNotepad = HostDialogs::of(mpHost).mpNotePad;
        QVERIFY2(mpNotepad, "the notepad did not open at all");
        mpNotepad->resize(760, 520);
        QVERIFY(QTest::qWaitForWindowExposed(mpNotepad));

        // A second note, so the strip has a chosen tab and one that is not
        if (mpNotepad->tabWidget->count() < 2) {
            mpNotepad->addTab(qsl("Second"));
        }
        QCOMPARE(mpNotepad->tabWidget->count(), 2);
        mpNotepad->tabWidget->setCurrentIndex(0);
        QCoreApplication::processEvents();
    }

    void cleanupTestCase()
    {
        mpNotepad = nullptr;
        mpHost = nullptr;
        if (mudlet::self()) {
            mudlet::self()->setAppearance(enums::Appearance::systemSetting);
        }
        mSession.stop();
    }

    // A profile's Lua stylesheet is assigned to the window on every show,
    // replacing any sheet set there
    void test_theShellsTwoSheetsAreSetOnWidgetsRatherThanOnTheWindow()
    {
        const QString central = centralSheet();
        QVERIFY2(central.contains(qsl("QTabBar::tab")), qPrintable(qsl("the central widget's sheet does not draw the strip of tabs at all: %1").arg(central.left(200))));
        QVERIFY2(central.contains(uiDesign::themeTokens().field.name()),
                 qPrintable(qsl("the central widget's sheet never names the field tone %1, so the notes are not drawn as the fields they are typed into").arg(uiDesign::themeTokens().field.name())));

        const QString bar = toolBarSheet();
        QVERIFY2(bar.contains(qsl("QToolBar#toolBar {")), qPrintable(qsl("the bar's sheet does not draw the bar itself: %1").arg(bar.left(200))));
        QVERIFY2(bar.contains(qsl("notepadFieldState")), "the bar's sheet draws no hairline on a field the strip has warned about");
        QVERIFY2(bar.contains(qsl("notepadSendingBar")), "the bar's sheet says nothing about the thread a send's progress is drawn as");
    }

    void test_theStripShowsItsReachAndNoStopWhileIdle()
    {
        QVERIFY2(!mpNotepad->findChild<QToolButton*>(qsl("notepadOptionsToggle")), "the bar still carries the Options button the send controls were folded behind");

        const QList<QPair<QAction*, QString>> sends{
                {mpNotepad->action_sendLine, qsl("Send line")}, {mpNotepad->action_sendSelection, qsl("Send selection")}, {mpNotepad->action_sendAll, qsl("Send all")}};
        for (const auto& send : sends) {
            QVERIFY2(onShow(send.first), qPrintable(qsl("'%1' is not on the strip at all").arg(send.second)));
            QVERIFY2(!send.first->icon().isNull(), qPrintable(qsl("'%1' carries no glyph, so it is a word with a gap beside it").arg(send.second)));
            QVERIFY2(!send.first->toolTip().isEmpty(), qPrintable(qsl("'%1' says nothing about what it will do").arg(send.second)));
        }

        writeNote(12);
        selectFromTheTop(3);
        QCOMPARE(mpNotepad->action_sendAll->text(), qsl("Send all (12 lines)"));
        QCOMPARE(mpNotepad->action_sendSelection->text(), qsl("Send selection (3 lines)"));
        QVERIFY2(mpNotepad->action_sendSelection->isEnabled(), "Send selection is unavailable with three lines selected");
        QCOMPARE(mpNotepad->action_sendLine->text(), qsl("Send line"));

        clearTheSelection();
        QCOMPARE(mpNotepad->action_sendSelection->text(), qsl("Send selection"));
        QVERIFY2(!mpNotepad->action_sendSelection->isEnabled(), "Send selection is available with nothing selected");

        // The count is kept between edits, so it has to follow the note on show and ignore the others
        mpNotepad->tabWidget->setCurrentIndex(1);
        QCoreApplication::processEvents();
        currentNote()->setPlainText(qsl("one\ntwo"));
        QCoreApplication::processEvents();
        QCOMPARE(mpNotepad->action_sendAll->text(), qsl("Send all (2 lines)"));
        QPlainTextEdit* pSecond = currentNote();
        mpNotepad->tabWidget->setCurrentIndex(0);
        QCoreApplication::processEvents();
        QCOMPARE(mpNotepad->action_sendAll->text(), qsl("Send all (12 lines)"));
        pSecond->clear();
        QCoreApplication::processEvents();
        QCOMPARE(mpNotepad->action_sendAll->text(), qsl("Send all (12 lines)"));

        QVERIFY2(!onShow(mpNotepad->action_stop), "Stop is on the strip with nothing being sent");

        // On macOS the lead word and the field otherwise sit a size larger than
        // the buttons beside them and off their line
        const QWidget* pSendLine = mpNotepad->toolBar->widgetForAction(mpNotepad->action_sendLine);
        QVERIFY2(pSendLine, "the bar made no button for Send line");
        // Rounded: some platforms' tool button font is fractional, where a
        // sheet can only name a whole point
        const int buttonSize = qRound(pSendLine->font().pointSizeF());
        for (const QWidget* pWord : {static_cast<QWidget*>(mpNotepad->label_prependText),
                                     static_cast<QWidget*>(mpNotepad->lineEdit_prependText),
                                     static_cast<QWidget*>(mpNotepad->mpLabel_noPrefixText),
                                     static_cast<QWidget*>(mpNotepad->mpLabel_sendingText)}) {
            QVERIFY2(qRound(pWord->font().pointSizeF()) == buttonSize,
                     qPrintable(qsl("%1 is set at %2pt beside buttons set at %3pt").arg(pWord->objectName(), QString::number(pWord->font().pointSizeF()), QString::number(buttonSize))));
        }
    }

    // A tool button holds its word in by the bar's padding while a label's word
    // starts at its edge, so equal margins alone leave the seam off-centre
    void test_theSeamOnTheStripIsCentredBetweenTheTwoWordsItParts()
    {
        atWidth(1400);
        writeNote(12);
        clearTheSelection();

        const QWidget* pSendAll = mpNotepad->toolBar->widgetForAction(mpNotepad->action_sendAll);
        QVERIFY2(pSendAll, "the bar made no button for Send all");
        const QWidget* pSeam = nullptr;
        for (QAction* pAction : mpNotepad->toolBar->actions()) {
            if (pAction->isSeparator()) {
                pSeam = mpNotepad->toolBar->widgetForAction(pAction);
                break;
            }
        }
        QVERIFY2(pSeam, "the bar carries no seam between the send buttons and the prefix");
        const QLabel* pWord = mpNotepad->label_prependText;
        QVERIFY2(pWord && pWord->isVisible(), "the word leading the prefix field is not on the strip");
        QVERIFY2(pSendAll->x() < pSeam->x() && pSeam->x() < pWord->x(),
                 qPrintable(qsl("the strip is not laid out button, seam, word: they start at %1, %2 and %3")
                                    .arg(QString::number(pSendAll->x()), QString::number(pSeam->x()), QString::number(pWord->x()))));

        const int beforeTheSeam = pSeam->x() - (pSendAll->x() + pSendAll->width());
        const int afterTheSeam = pWord->x() - (pSeam->x() + pSeam->width());
        QVERIFY2(beforeTheSeam == afterTheSeam, qPrintable(qsl("the bar leaves %1px before the seam and %2px after it").arg(QString::number(beforeTheSeam), QString::number(afterTheSeam))));

        // Read off the ink: the button's slack after its word is what the
        // margins alone cannot account for
        const QImage shot = mpNotepad->toolBar->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const qreal ratio = shot.width() / qreal(mpNotepad->toolBar->width());
        const QRgb bar = shot.pixel(qRound(2 * ratio), qRound(2 * ratio));
        const auto inkIn = [&](const QWidget* pWidget, int& first, int& last) {
            first = -1;
            last = -1;
            const QRect area = QRect(pWidget->mapTo(mpNotepad->toolBar, QPoint(0, 0)), pWidget->size());
            for (int x = area.left(); x <= area.right(); ++x) {
                for (int y = area.top() + 2; y < area.bottom() - 2; ++y) {
                    const QRgb px = shot.pixel(qRound(x * ratio), qRound(y * ratio));
                    if (qAbs(qRed(px) - qRed(bar)) + qAbs(qGreen(px) - qGreen(bar)) + qAbs(qBlue(px) - qBlue(bar)) > 90) {
                        if (first < 0) {
                            first = x;
                        }
                        last = x;
                        break;
                    }
                }
            }
        };
        int buttonFirst = 0, buttonLast = 0, wordFirst = 0, wordLast = 0;
        inkIn(pSendAll, buttonFirst, buttonLast);
        inkIn(pWord, wordFirst, wordLast);
        QVERIFY2(buttonLast > 0 && wordFirst > 0, "no ink was found in the Send all button or in the lead word");
        const int seamLine = pSeam->x() + pSeam->width() / 2;
        const int leftOfTheLine = seamLine - buttonLast;
        const int rightOfTheLine = wordFirst - seamLine;
        qInfo().noquote() << qsl("  the seam stands %1px after Send all's last ink and %2px before the lead word's first").arg(QString::number(leftOfTheLine), QString::number(rightOfTheLine));
        // <= 2: the tool button's one transparent border pixel, plus rounding at the grab's ratio
        QVERIFY2(qAbs(leftOfTheLine - rightOfTheLine) <= 2,
                 qPrintable(qsl("the seam is %1px from the word on its left and %2px from the word on its right").arg(QString::number(leftOfTheLine), QString::number(rightOfTheLine))));
    }

    void test_aSendShowsProgressAndStopThenReturns()
    {
        writeNote(12);
        clearTheSelection();
        mpNotepad->lineEdit_prependText->setText(qsl("say"));
        QCoreApplication::processEvents();

        mpNotepad->action_sendAll->trigger();
        QCoreApplication::processEvents();

        QVERIFY2(!onShow(mpNotepad->action_sendAll), "Send all is still on the strip while a send runs");
        QVERIFY2(!onShow(mpNotepad->action_sendLine) && !onShow(mpNotepad->action_sendSelection), "the other two send buttons stayed on the strip while a send runs");
        QVERIFY2(!onShow(mpNotepad->action_prependText) && !onShow(mpNotepad->action_prependTextLabel), "the prefix and the word leading it stayed on the strip while a send runs");
        QVERIFY2(onShow(mpNotepad->action_stop), "there is no way to stop a send that is running");
        QCOMPARE(wordsOn(qsl("notepadSendingText")), qsl("Sending 0 of 12"));

        auto* pBar = mpNotepad->findChild<QProgressBar*>(qsl("notepadSendingBar"));
        QVERIFY2(pBar, "the strip carries no thread of progress");
        QCOMPARE(pBar->maximum(), 12);
        QCOMPARE(pBar->value(), 0);

        QTRY_VERIFY_WITH_TIMEOUT(wordsOn(qsl("notepadSendingText")) == qsl("Sending 1 of 12"), 2000);
        QCOMPARE(pBar->value(), 1);
        QTRY_VERIFY_WITH_TIMEOUT(wordsOn(qsl("notepadSendingText")) == qsl("Sending 2 of 12"), 2000);
        QCOMPARE(pBar->value(), 2);

        mpNotepad->slot_stopSending();
        QCoreApplication::processEvents();
        QTRY_VERIFY_WITH_TIMEOUT(onShow(mpNotepad->action_sendAll), 1000);
        QVERIFY2(!onShow(mpNotepad->action_stop), "Stop is still on the strip after the send was stopped");
        QVERIFY2(onShow(mpNotepad->action_prependText), "the prefix field did not come back after the send was stopped");

        mpNotepad->lineEdit_prependText->clear();
        QCoreApplication::processEvents();
    }

    void test_anEmptyPrefixOverALongNoteWarns()
    {
        // Wide enough for the whole sentence; the narrower forms are the next case
        mpNotepad->resize(1400, 520);
        QCoreApplication::processEvents();
        mpNotepad->lineEdit_prependText->clear();
        writeNote(12);
        clearTheSelection();

        QWidget* pNote = mpNotepad->findChild<QWidget*>(qsl("notepadNoPrefixNote"));
        QVERIFY2(pNote, "the strip has no 'notepadNoPrefixNote' at all");
        QVERIFY2(pNote->isVisible(), "a 12 line note with no prefix is sent without a word about it");
        QVERIFY2(wordsOn(qsl("notepadNoPrefixText")).contains(qsl("12")), qPrintable(qsl("the warning does not say how many lines would go: \"%1\"").arg(wordsOn(qsl("notepadNoPrefixText")))));
        QCOMPARE(mpNotepad->lineEdit_prependText->property("notepadFieldState").toString(), qsl("warning"));
        QVERIFY2(mpNotepad->action_sendAll->isEnabled(), "the warning took the send away rather than saying what it would do");

        QAction* pCue = fieldCue();
        QVERIFY2(pCue, "the prefix field carries no 'notepadPrefixWarning' at all");
        QVERIFY2(pCue->isVisible(), "the field carries no cue while the strip is warning about what a send would do");
        QVERIFY2(!pCue->icon().isNull(), "the field's cue carries no picture, so it is a gap at the end of the field");
        QCOMPARE(pCue->toolTip(), mpNotepad->mNoPrefixNoteText);
        QVERIFY2(pCue->toolTip().contains(qsl("12")), qPrintable(qsl("the cue does not say how many lines would go: \"%1\"").arg(pCue->toolTip())));

        mpNotepad->lineEdit_prependText->setText(qsl("say"));
        QCoreApplication::processEvents();
        QVERIFY2(!pNote->isVisible(), "the warning stayed up after a prefix was typed");
        QVERIFY2(!pCue->isVisible(), "the field kept its cue after a prefix was typed");
        QVERIFY2(mpNotepad->lineEdit_prependText->property("notepadFieldState").toString().isEmpty(), "the field kept its warning hairline after a prefix was typed");

        mpNotepad->lineEdit_prependText->clear();
        writeNote(3);
        QCoreApplication::processEvents();
        QVERIFY2(!pNote->isVisible(), "a three line note with no prefix is warned about, which is a warning about nothing");
        QVERIFY2(!pCue->isVisible(), "a three line note put the cue in the field");
        QVERIFY2(mpNotepad->lineEdit_prependText->property("notepadFieldState").toString().isEmpty(), "a three line note put the warning hairline on the field");

        writeNote(12);
        atWidth(760);
        QVERIFY2(pCue->isVisible(), "the field's cue went with the words the narrower strip had no room for");
        QVERIFY2(pNote->toolTip().contains(qsl("12")), qPrintable(qsl("the whole sentence is nowhere to be read: \"%1\"").arg(pNote->toolTip())));
        QVERIFY2(mpNotepad->lineEdit_prependText->toolTip().contains(qsl("12")),
                 qPrintable(qsl("the field, which is what a reader is looking at, does not carry the sentence: \"%1\"").arg(mpNotepad->lineEdit_prependText->toolTip())));
        // The action rather than the widget: a bar that has moved its tail into
        // its extension menu leaves the widget saying it is visible
        if (mpNotepad->action_noPrefixNote->isVisible()) {
            QVERIFY2(pNote->x() + pNote->width() <= mpNotepad->toolBar->width(), "the warning is drawn off the end of the strip");
        }
    }

    // Widths pinned to what this window measures on the platform these run on:
    // the strip without the words asks 712px, the sentence 298px and the short
    // form 54px, so the sentence needs a 1010px window and the short form 766px
    void test_theWarningIsShownWholeOrShortOrNotAtAll()
    {
        mpNotepad->lineEdit_prependText->clear();
        writeNote(12);
        clearTheSelection();

        QWidget* pNote = mpNotepad->findChild<QWidget*>(qsl("notepadNoPrefixNote"));
        QAction* pCue = fieldCue();
        QVERIFY2(pNote && pCue, "the strip is missing the warning's reading or the field's cue");
        const QString whole = mpNotepad->mNoPrefixNoteText;
        QVERIFY2(whole.contains(qsl("12")), qPrintable(qsl("the sentence to be fitted is not the warning: \"%1\"").arg(whole)));

        atWidth(1400);
        QTRY_COMPARE(wordsOn(qsl("notepadNoPrefixText")), whole);
        QVERIFY2(pNote->isVisible(), "the warning is not on the strip on a window with room for the whole sentence");
        QVERIFY2(pCue->isVisible(), "the field's cue is gone at the width where everything fits");
        nothingIsAFragment();

        atWidth(800);
        QTRY_COMPARE(wordsOn(qsl("notepadNoPrefixText")), qsl("No prefix"));
        QVERIFY2(pNote->isVisible(), "the short form is not on the strip at the width it is for");
        QVERIFY2(pCue->isVisible(), "the field's cue went with the sentence the strip had no room for");
        nothingIsAFragment();

        // Read off the action: at this width the bar also moves its tail into
        // the extension menu, which leaves the widget's isVisible() stale
        atWidth(520);
        QTRY_VERIFY2(!onShow(mpNotepad->action_noPrefixNote), "the reading is still on the strip at a width neither form fits in");
        QVERIFY2(pNote->isHidden(), "the warning's widget was left on the strip, so it is drawn as a dot with nothing after it");
        QVERIFY2(wordsOn(qsl("notepadNoPrefixText")).isEmpty(), qPrintable(qsl("words were left on the label at the narrowest width: \"%1\"").arg(wordsOn(qsl("notepadNoPrefixText")))));
        QVERIFY2(pCue->isVisible(), "with no words on the strip the field's cue is gone as well, so nothing warns at all");
        QVERIFY2(pNote->toolTip().contains(qsl("12")) && pCue->toolTip().contains(qsl("12")), "the whole sentence is nowhere to be read at the narrowest width");
        nothingIsAFragment();

        // Widened again: a fit that only ever narrows would pass the steps above
        atWidth(1400);
        QTRY_COMPARE(wordsOn(qsl("notepadNoPrefixText")), whole);
        QVERIFY2(pNote->isVisible(), "the warning did not come back when the window was widened again");

        mpNotepad->resize(760, 520);
        QCoreApplication::processEvents();
    }

    void test_theStripsInksAreTheDesigns()
    {
        auto* pDot = mpNotepad->findChild<QLabel*>(qsl("notepadSendingDot"));
        auto* pWords = mpNotepad->findChild<QLabel*>(qsl("notepadNoPrefixText"));
        QVERIFY2(pDot && pWords, "the strip is missing one of its two readings");

        mpNotepad->lineEdit_prependText->clear();
        writeNote(12);
        clearTheSelection();

        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            TestPixels::setAppearanceAndWait(appearance.second, mpNotepad);
            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

            QVERIFY2(pDot->styleSheet().contains(tokens.accent.name()),
                     qPrintable(qsl("on the %1 appearance the sending dot is not drawn in the accent %2: %3").arg(appearance.first, tokens.accent.name(), pDot->styleSheet())));

            const QColor ink = pWords->palette().color(QPalette::Active, QPalette::WindowText);
            const qreal ratio = uiDesign::contrastRatio(ink, tokens.page);
            qInfo().noquote() << qsl("  %1: the warning reads %2 on the page %3 = %4:1").arg(appearance.first, ink.name(), tokens.page.name(), QString::number(ratio, 'f', 2));
            QVERIFY2(ratio >= uiDesign::scmTextMinimumRatio,
                     qPrintable(qsl("on the %1 appearance the warning reads %2 on the page %3 = %4:1, under the %5:1 every word of this design is walked to")
                                        .arg(appearance.first, ink.name(), tokens.page.name(), QString::number(ratio, 'f', 2), QString::number(uiDesign::scmTextMinimumRatio, 'f', 1))));
        }

        // The cases after this one read pixels against the machine's appearance
        TestPixels::setAppearanceAndWait(enums::Appearance::systemSetting, mpNotepad);
    }

    void test_everyPictureInTheWindowComesFromTheGlyphs()
    {
        const QStringList pictureButtons{qsl("notepadAddTab"), qsl("notepadFindPrevious"), qsl("notepadFindNext"), qsl("notepadFindClose")};
        for (const QString& name : pictureButtons) {
            auto* pButton = mpNotepad->findChild<QToolButton*>(name);
            QVERIFY2(pButton, qPrintable(qsl("there is no '%1'").arg(name)));
            QVERIFY2(!pButton->icon().isNull(), qPrintable(qsl("'%1' carries no glyph, so it is an empty square").arg(name)));
            QVERIFY2(pButton->text().isEmpty(), qPrintable(qsl("'%1' still carries a word beside its glyph").arg(name)));
        }

        const QString central = centralSheet();
        QVERIFY2(central.contains(qsl("close-button")), "the strip of tabs draws no cross of its own, so a tab keeps the platform's");
        const int at = central.indexOf(qsl("::close-button"));
        QVERIFY2(central.mid(at, 200).contains(qsl("image:")), "the tab's close-button rule points at no picture");
    }

    void test_theChosenTabIsLitAndTheRestAreThePage()
    {
        QTabBar* pTabBar = mpNotepad->tabWidget->tabBar();
        QVERIFY2(pTabBar->count() == 2, "the strip does not hold the two tabs this reads");
        mpNotepad->tabWidget->setCurrentIndex(0);
        QCoreApplication::processEvents();

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor lit = uiDesign::blend(tokens.page, tokens.accent, uiDesign::scmSoftWashStrength);
        const QImage shot = mpNotepad->grab().toImage();

        // Inside the fill and clear of the word, in the tab's leading padding
        const auto insideTab = [pTabBar, this](const int index) {
            const QRect tab = pTabBar->tabRect(index);
            return pTabBar->mapTo(mpNotepad, QPoint(tab.left() + 4, tab.center().y()));
        };

        const QColor chosen = shot.pixelColor(insideTab(0));
        const QColor other = shot.pixelColor(insideTab(1));
        qInfo().noquote() << qsl("  chosen tab %1, the other %2; the accent wash on the page is %3 and the page is %4").arg(chosen.name(), other.name(), lit.name(), tokens.page.name());

        QVERIFY2(TestPixels::readsAs(chosen, lit, scmFillTolerance),
                 qPrintable(qsl("the chosen tab reads %1 rather than the accent wash %2 - it is not drawn as a lit chip").arg(chosen.name(), lit.name())));
        QVERIFY2(TestPixels::readsAs(other, tokens.page, scmFillTolerance),
                 qPrintable(qsl("the tab that is not chosen reads %1 rather than the page %2 - the strip is still drawing folder tabs").arg(other.name(), tokens.page.name())));
    }

    // Nothing else restyles this window: it is not a QDialog, and the profile
    // stylesheet it is handed on show says nothing about either sheet
    void test_bothSheetsAreRebuiltWhenTheAppearanceMoves()
    {
        QMap<QString, QPair<QString, QString>> sheets;
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            TestPixels::setAppearanceAndWait(appearance.second, mpNotepad);
            const QString field = uiDesign::themeTokens().field.name();
            QVERIFY2(centralSheet().contains(field),
                     qPrintable(qsl("on the %1 appearance the field tone is %2, which the central widget's sheet does not name - the shell did not restyle").arg(appearance.first, field)));
            QVERIFY2(toolBarSheet().contains(uiDesign::themeTokens().page.name()),
                     qPrintable(qsl("on the %1 appearance the bar's sheet does not name the page tone %2").arg(appearance.first, uiDesign::themeTokens().page.name())));
            sheets.insert(appearance.first, {centralSheet(), toolBarSheet()});
        }

        QVERIFY2(sheets.value(qsl("dark")).first != sheets.value(qsl("light")).first, "both appearances built the same central sheet, so the reading above would hold whatever it said");
        QVERIFY2(sheets.value(qsl("dark")).second != sheets.value(qsl("light")).second, "both appearances built the same toolbar sheet");
    }

    void test_theRetiredFoldKeyIsClearedOnSave()
    {
        const QString key = qsl("Notepad/SendControlsVisible");
        mpHost->writeProfileIniData(key, qsl("true"));
        QCOMPARE(mpHost->readProfileIniData(key), qsl("true"));

        mpNotepad->saveSettings();

        QVERIFY2(mpHost->readProfileIniData(key).isEmpty(), qPrintable(qsl("the profile still carries %1 after the notepad saved its settings").arg(key)));
        QVERIFY2(!mpHost->readProfileIniData(qsl("Notepad/WindowState")).isEmpty(), "the save that was meant to clear the old key did not write the window state either, so it did not run");
    }

    // A closed note is gone: save() writes only the tabs that are left
    void test_aNoteWithTextAsksBeforeItIsClosed()
    {
        QCOMPARE(mpNotepad->tabWidget->count(), 2);
        auto* pSecond = qobject_cast<QPlainTextEdit*>(mpNotepad->tabWidget->widget(1));
        QVERIFY2(pSecond, "the second note is not a field at all");
        pSecond->setPlainText(qsl("kept"));
        QCoreApplication::processEvents();

        armTheAnswer(TheAnswer::Keep);
        mpNotepad->closeTab(1);
        disarmTheAnswer();
        QVERIFY2(mBoxComplaint.isEmpty(), qPrintable(mBoxComplaint));
        QVERIFY2(mBoxesSeen == 1, qPrintable(qsl("closing a note with text in it put %1 questions to the reader rather than one").arg(QString::number(mBoxesSeen))));
        QCOMPARE(mpNotepad->tabWidget->count(), 2);
        QCOMPARE(qobject_cast<QPlainTextEdit*>(mpNotepad->tabWidget->widget(1))->toPlainText(), qsl("kept"));

        armTheAnswer(TheAnswer::Discard);
        mpNotepad->closeTab(1);
        disarmTheAnswer();
        QVERIFY2(mBoxComplaint.isEmpty(), qPrintable(mBoxComplaint));
        QCOMPARE(mBoxesSeen, 1);
        QCOMPARE(mpNotepad->tabWidget->count(), 1);

        // Armed anyway: a wrong question is answered rather than left to hang
        // the case in the box's loop
        mpNotepad->addTab(qsl("Second"));
        QCoreApplication::processEvents();
        QCOMPARE(mpNotepad->tabWidget->count(), 2);
        armTheAnswer(TheAnswer::Keep);
        mpNotepad->closeTab(1);
        disarmTheAnswer();
        QVERIFY2(!mBoxesSeen, "closing an empty note asked the reader about text that was not there");
        QCOMPARE(mpNotepad->tabWidget->count(), 1);

        mpNotepad->addTab(qsl("Second"));
        mpNotepad->tabWidget->setCurrentIndex(0);
        QCoreApplication::processEvents();
        QCOMPARE(mpNotepad->tabWidget->count(), 2);
    }

    void test_closingTheOtherNotesAsksOnceForAllOfThem()
    {
        QCOMPARE(mpNotepad->tabWidget->count(), 2);
        qobject_cast<QPlainTextEdit*>(mpNotepad->tabWidget->widget(1))->setPlainText(qsl("kept"));
        const int third = mpNotepad->addTab(qsl("Third"), qsl("also kept"));
        QCoreApplication::processEvents();
        QCOMPARE(third, 2);
        QCOMPARE(mpNotepad->tabWidget->count(), 3);

        armTheAnswer(TheAnswer::Keep);
        mpNotepad->closeOtherTabs(0);
        disarmTheAnswer();
        QVERIFY2(mBoxComplaint.isEmpty(), qPrintable(mBoxComplaint));
        QVERIFY2(mBoxesSeen == 1, qPrintable(qsl("closing the other notes put %1 questions to the reader rather than one").arg(QString::number(mBoxesSeen))));
        QVERIFY2(mpNotepad->tabWidget->count() == 3, qPrintable(qsl("Cancel left %1 of the 3 notes standing").arg(QString::number(mpNotepad->tabWidget->count()))));

        armTheAnswer(TheAnswer::Discard);
        mpNotepad->closeOtherTabs(0);
        disarmTheAnswer();
        QVERIFY2(mBoxComplaint.isEmpty(), qPrintable(mBoxComplaint));
        QCOMPARE(mBoxesSeen, 1);
        QCOMPARE(mpNotepad->tabWidget->count(), 1);

        mpNotepad->addTab(qsl("Second"));
        mpNotepad->tabWidget->setCurrentIndex(0);
        QCoreApplication::processEvents();
        QCOMPARE(mpNotepad->tabWidget->count(), 2);
    }

    void test_theLastTabCarriesNoCrossSinceItCannotClose()
    {
        QTabBar* pTabBar = mpNotepad->tabWidget->tabBar();
        const auto crossOn = [pTabBar](const int index) {
            QWidget* pTrailing = pTabBar->tabButton(index, QTabBar::RightSide);
            return pTrailing ? pTrailing : pTabBar->tabButton(index, QTabBar::LeftSide);
        };

        QCOMPARE(mpNotepad->tabWidget->count(), 2);
        for (int i = 0; i < 2; ++i) {
            QVERIFY2(crossOn(i), qPrintable(qsl("tab %1 carries no cross while there are two notes to close one of").arg(QString::number(i))));
        }

        mpNotepad->closeTab(1);
        QCoreApplication::processEvents();
        QCOMPARE(mpNotepad->tabWidget->count(), 1);
        QVERIFY2(!crossOn(0), "the sole tab still carries a cross, though closeTab() will not close it");

        mpNotepad->closeTab(0);
        QCoreApplication::processEvents();
        QCOMPARE(mpNotepad->tabWidget->count(), 1);

        const int second = mpNotepad->addTab(qsl("Second"));
        QCoreApplication::processEvents();
        QCOMPARE(mpNotepad->tabWidget->count(), 2);
        for (int i = 0; i < 2; ++i) {
            QVERIFY2(crossOn(i), qPrintable(qsl("tab %1 carries no cross after a second note brought the crosses back").arg(QString::number(i))));
        }

        // Recreated crosses are caught by the filter on the bar, not by the
        // styling pass over the tabs that existed then
        theCrossOnTabIsTheRecipes(0);
        if (QTest::currentTestFailed()) {
            return;
        }
        theCrossOnTabIsTheRecipes(second);

        mpNotepad->tabWidget->setCurrentIndex(0);
        QCoreApplication::processEvents();
    }

    // Qt's tab close button takes its size from the style in its constructor,
    // never from the ::close-button rule, and is placed from the tab's raw rect
    // rather than from the padding box the rule lays the word in
    void test_everyCrossIsTheRecipesBoxInsideTheChipsPadding()
    {
        for (int i = 0; i < mpNotepad->tabWidget->tabBar()->count(); ++i) {
            theCrossOnTabIsTheRecipes(i);
            if (QTest::currentTestFailed()) {
                return;
            }
        }

        // The dark appearance's base style is Fusion, which answers a different
        // size for a tab's cross than the platform style; a new tab is caught by
        // the filter on the bar
        TestPixels::setAppearanceAndWait(enums::Appearance::dark, mpNotepad);
        const int third = mpNotepad->addTab(qsl("Third"));
        QCoreApplication::processEvents();
        theCrossOnTabIsTheRecipes(third);
    }
};

#include "NotepadShellTest.moc"
MUDLET_GROUPED_TEST_MAIN(NotepadShellTest)
