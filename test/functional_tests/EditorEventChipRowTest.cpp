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
 * A script's events drawn as a row of chips. The first case also holds the old
 * list, field and +/- buttons gone: a form still carrying them would pass every
 * other case while offering two ways to do the same thing.
 *
 * Run with: ctest -R EditorEventChipRowTest -V
 */

#include <QAbstractItemView>
#include <QCompleter>
#include <QFontInfo>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QtTest/QtTest>
#include <chrono>

#include "ChipRow.h"
#include "Host.h"
#include "HostDialogs.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "ScriptUnit.h"
#include "TEvent.h"
#include "TScript.h"
#include "TTrigger.h"
#include "TriggerUnit.h"
#include "dlgScriptsMainArea.h"
#include "dlgSourceEditorArea.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorEventChipRowTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    QTreeWidgetItem* mpScriptItem = nullptr;
    const QString mProfileName = qsl("EditorEventChipRow-Test-Profile");

    static QStringList startingEvents() { return {qsl("sysConnectionEvent"), qsl("MyEvent"), qsl("OneMoreEvent")}; }

    uiDesign::ChipRow* row() const { return mpEditor->mpChipRow_scriptEvents; }
    // Null means closed: the field stays in the row, hidden, between names
    QLineEdit* field() const
    {
        QLineEdit* pField = row()->findChild<QLineEdit*>(qsl("editorChipEditor"));
        return pField && !pField->isHidden() ? pField : nullptr;
    }
    QToolButton* addButton() const { return row()->findChild<QToolButton*>(qsl("editorChipAdd")); }
    QLabel* note() const
    {
        QLabel* pNote = row()->findChild<QLabel*>(qsl("editorChipNote"));
        return pNote && !pNote->isHidden() ? pNote : nullptr;
    }

    QStringList savedEvents() const
    {
        if (!mpScriptItem) {
            return {};
        }
        TScript* pScript = mpHost->getScriptUnit()->getScript(mpScriptItem->data(0, Qt::UserRole).toInt());
        return pScript ? pScript->getEventHandlerList() : QStringList{};
    }

    // Read off the chip widgets rather than items(), so it checks what is drawn
    QStringList shownEvents() const
    {
        QStringList shown;
        for (int i = 0; i < row()->count(); ++i) {
            QWidget* pChip = row()->chipAt(i);
            if (!pChip) {
                continue;
            }
            if (QLabel* pLabel = pChip->findChild<QLabel*>(); pLabel) {
                shown << pLabel->text();
            }
        }
        return shown;
    }

    void typeIntoField(const QString& text)
    {
        QLineEdit* pField = field();
        if (!pField) {
            QTest::qFail("the field is not open, so there is nothing to type into", __FILE__, __LINE__);
            return;
        }
        pField->setText(text);
        QTest::keyClick(pField, Qt::Key_Return);
        QCoreApplication::processEvents();
    }

    QCompleter* completer() const
    {
        QLineEdit* pField = row()->findChild<QLineEdit*>(qsl("editorChipEditor"));
        return pField ? pField->completer() : nullptr;
    }

    QAbstractItemView* suggestionList() const
    {
        QCompleter* pCompleter = completer();
        return pCompleter ? pCompleter->popup() : nullptr;
    }

    // The popup's model is the completer's filtered one, not everything on offer
    QStringList offered() const
    {
        QAbstractItemView* pList = suggestionList();
        if (!pList || !pList->model()) {
            return {};
        }
        QStringList rows;
        for (int i = 0; i < pList->model()->rowCount(); ++i) {
            rows << pList->model()->index(i, 0).data(Qt::DisplayRole).toString();
        }
        return rows;
    }

    QString noteFor(const QString& name) const
    {
        QAbstractItemView* pList = suggestionList();
        if (!pList || !pList->model()) {
            return {};
        }
        for (int i = 0; i < pList->model()->rowCount(); ++i) {
            const QModelIndex row = pList->model()->index(i, 0);
            if (row.data(Qt::DisplayRole).toString() == name) {
                return row.data(Qt::UserRole).toString();
            }
        }
        return {};
    }

    // Key input, not setText(): the completer only reacts to typed text
    void keyIntoField(const QString& text)
    {
        QLineEdit* pField = field();
        if (!pField) {
            QTest::qFail("the field is not open, so there is nothing to type into", __FILE__, __LINE__);
            return;
        }
        QTest::keyClicks(pField, text);
        QCoreApplication::processEvents();
    }

    void retypeField(const QString& text)
    {
        QLineEdit* pField = field();
        if (!pField) {
            QTest::qFail("the field is not open, so there is nothing to type into", __FILE__, __LINE__);
            return;
        }
        pField->selectAll();
        QTest::keyClicks(pField, text);
        QCoreApplication::processEvents();
    }

    // With a payload argument, the way a protocol event arrives
    void raiseInProfile(const QString& name)
    {
        TEvent event{};
        event.mArgumentList.append(name);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        event.mArgumentList.append(qsl("{}"));
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        mpHost->raiseEvent(event);
    }

    static QString originOn(const QWidget* pChip)
    {
        const QLabel* pWord = pChip ? pChip->findChild<QLabel*>(qsl("editorChipLabel")) : nullptr;
        return pWord ? pWord->toolTip() : QString();
    }

    // The pixel of the word furthest from the chip's own fill is the ink it is written in
    static QColor wordInk(QWidget* pChip, const QColor& fill)
    {
        const QLabel* pWord = pChip->findChild<QLabel*>(qsl("editorChipLabel"));
        if (!pWord) {
            return QColor();
        }
        const QImage shot = TestPixels::shot(pChip);
        const qreal ratio = shot.devicePixelRatio();
        const QRect word = pWord->geometry();
        const QRect painted = QRect(QPoint(qRound(word.x() * ratio), qRound(word.y() * ratio)), QSize(qRound(word.width() * ratio), qRound(word.height() * ratio))).intersected(shot.rect());
        QColor ink = fill;
        for (int y = painted.top(); y <= painted.bottom(); ++y) {
            for (int x = painted.left(); x <= painted.right(); ++x) {
                const QColor pixel = shot.pixelColor(x, y);
                if (TestPixels::distance(pixel, fill) > TestPixels::distance(ink, fill)) {
                    ink = pixel;
                }
            }
        }
        return ink;
    }

    // Each case leaves the last one's chips behind
    void reopenFieldOn(const QStringList& chips)
    {
        row()->setItems(chips);
        QCoreApplication::processEvents();
        row()->beginAdd();
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

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1100, 900);
        mpEditor->slot_showScripts();
        mpEditor->addScript(false);
        QTest::qWait(100ms);

        mpScriptItem = mpEditor->mpCurrentScriptItem;
        QVERIFY2(mpScriptItem != nullptr, "addScript() left no current script item");

        // Filled by reselecting, the path a script loaded with the profile takes
        TScript* pScript = mpHost->getScriptUnit()->getScript(mpScriptItem->data(0, Qt::UserRole).toInt());
        QVERIFY2(pScript != nullptr, "the new script is not in the script unit");
        pScript->setEventHandlerList(startingEvents());
        mpEditor->slot_scriptsSelected(mpScriptItem);
        QTest::qWait(50ms);

        // After the reselect, which refills the form and would undo the name; the
        // search cases find the script by its tree row's name
        mpEditor->mpScriptsMainArea->lineEdit_script_name->setText(qsl("ChipRowScript"));
        mpEditor->slot_saveSelectedItem();
        QTest::qWait(50ms);
        QCOMPARE(mpScriptItem->text(0), qsl("ChipRowScript"));
    }

    void cleanupTestCase()
    {
        mpScriptItem = nullptr;
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_theScriptsEventsAreItsChips()
    {
        QVERIFY2(row() != nullptr, "the scripts form has no row of event chips");
        QCOMPARE(row()->count(), 3);
        QCOMPARE(shownEvents(), startingEvents());
        QCOMPARE(row()->items(), startingEvents());

        for (const QString& gone : {qsl("listWidget_script_registered_event_handlers"),
                                    qsl("lineEdit_script_event_handler_entry"),
                                    qsl("toolButton_script_add_event_handler"),
                                    qsl("toolButton_script_remove_event_handler"),
                                    qsl("label_script_event_handler_entry")}) {
            QVERIFY2(mpEditor->mpScriptsMainArea->findChild<QWidget*>(gone) == nullptr, qPrintable(qsl("the scripts form still carries %1").arg(gone)));
        }
        QVERIFY2(addButton() != nullptr, "the row has no button to add an event with");
        QVERIFY2(field() == nullptr, "the row opens with its field already showing");

        // Checked at first show, before any font change: lineHeight() is only
        // known once the form's sheet has reached the field
        const QLabel* pLead = mpEditor->mpScriptsMainArea->label_script_registered_event_handlers;
        QCOMPARE(pLead->height(), row()->lineHeight());
        QCOMPARE(addButton()->height(), row()->lineHeight());
    }

    void test_typingANameAddsAChip()
    {
        row()->beginAdd();
        QCoreApplication::processEvents();
        QVERIFY2(field() != nullptr, "beginAdd() did not open the field");
        QVERIFY2(addButton()->isHidden(), "the add button is still showing beside the field it opened");

        typeIntoField(qsl("FourthEvent"));

        QCOMPARE(row()->count(), 4);
        QCOMPARE(row()->items().at(3), qsl("FourthEvent"));

        mpEditor->saveScript();
        QTest::qWait(50ms);
        QCOMPARE(savedEvents(), startingEvents() << qsl("FourthEvent"));
    }

    void test_aNameAlreadyListedIsRefused()
    {
        QVERIFY2(field() != nullptr, "the field closed after the last name was taken, so there is nothing to type the duplicate into");
        QSignalSpy refusals(row(), &uiDesign::ChipRow::duplicateRefused);

        typeIntoField(qsl("MyEvent"));

        QCOMPARE(row()->count(), 4);
        QCOMPARE(refusals.count(), 1);
        QLabel* pNote = note();
        QVERIFY2(pNote != nullptr, "nothing said why the name was not taken");
        QVERIFY2(pNote->text().contains(qsl("MyEvent")), qPrintable(qsl("the note does not name what was refused: %1").arg(pNote->text())));
        QVERIFY2(field() != nullptr, "the field was closed on a name it refused, so the typing is lost");
    }

    void test_escapeClosesTheField()
    {
        QVERIFY2(field() != nullptr, "the field is not open, so there is nothing for Escape to close");
        const int before = row()->count();

        QTest::keyClick(field(), Qt::Key_Escape);
        QCoreApplication::processEvents();

        QVERIFY2(field() == nullptr, "Escape left the field open");
        QVERIFY2(!addButton()->isHidden(), "Escape closed the field without putting the add button back");
        QCOMPARE(row()->count(), before);
    }

    void test_theCrossTakesAnEventAway()
    {
        QCOMPARE(row()->count(), 4);
        QWidget* pChip = row()->chipAt(1);
        QVERIFY2(pChip != nullptr, "the row has no second chip to take away");
        QCOMPARE(pChip->findChild<QLabel*>()->text(), qsl("MyEvent"));

        QToolButton* pCross = pChip->findChild<QToolButton*>(qsl("editorChipRemove"));
        QVERIFY2(pCross != nullptr, "a chip has no cross to take it away with");
        pCross->click();
        QCoreApplication::processEvents();

        QCOMPARE(row()->count(), 3);
        QVERIFY2(!row()->items().contains(qsl("MyEvent")), "the event the cross was clicked on is still listed");
        QCOMPARE(row()->items(), QStringList({qsl("sysConnectionEvent"), qsl("OneMoreEvent"), qsl("FourthEvent")}));
    }

    void test_theSearchJumpPointsAtTheChip()
    {
        // Focus elsewhere first, or the chip the last case focused would pass this
        // without any jump
        row()->focusItem(0);
        QCoreApplication::processEvents();
        QCOMPARE(row()->focusWidget(), row()->chipAt(0));

        QTreeWidgetItem* pResult = new QTreeWidgetItem(QStringList{qsl("ChipRowScript")});
        pResult->setData(0, dlgTriggerEditor::ItemRole, static_cast<int>(EditorViewType::cmScriptView));
        pResult->setData(0, dlgTriggerEditor::TypeRole, dlgTriggerEditor::SearchResultIsEventHandler);
        pResult->setData(0, dlgTriggerEditor::NameRole, qsl("ChipRowScript"));
        pResult->setData(0, dlgTriggerEditor::IdRole, mpScriptItem->data(0, Qt::UserRole).toInt());
        pResult->setData(0, dlgTriggerEditor::PatternOrLineRole, 1);
        mpEditor->treeWidget_searchResults->addTopLevelItem(pResult);

        mpEditor->slot_itemSelectedInSearchResults(pResult);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        // focusWidget(), not hasFocus(): hasFocus() also needs the window active,
        // which a headless run cannot promise. The jump refills the row from the
        // saved script, so the event removed above is back.
        QVERIFY2(row()->count() > 1, "the jump left the row with fewer chips than the result pointed into");
        QCOMPARE(row()->focusWidget(), row()->chipAt(1));
    }

    void test_wrappingGrowsTheFormColumn()
    {
        mpEditor->resize(1100, 900);
        QCoreApplication::processEvents();
        QTest::qWait(100ms);
        const int oneLine = mpEditor->mpNonCodeWidgets->height();
        const int codeOneLine = mpEditor->mpSourceEditorArea->height();
        const int lineHeight = row()->lineHeight();
        QVERIFY2(row()->height() < 2 * lineHeight,
                 qPrintable(qsl("the chips already take more than one line at 1100px, so this case has nothing to widen from - the row is %1 tall").arg(row()->height())));

        mpEditor->resize(620, 900);
        QCoreApplication::processEvents();
        QTest::qWait(100ms);

        const int wrapped = mpEditor->mpNonCodeWidgets->height();
        qInfo().noquote() << qsl("  the column is %1 on one line of chips and %2 on two, a chip line being %3").arg(QString::number(oneLine), QString::number(wrapped), QString::number(lineHeight));
        QVERIFY2(row()->height() >= 2 * lineHeight,
                 qPrintable(qsl("the chips did not wrap at 620px - the row is %1 tall and a line of it is %2").arg(QString::number(row()->height()), QString::number(lineHeight))));
        QVERIFY2(wrapped >= oneLine + lineHeight,
                 qPrintable(qsl("the form column stayed at %1 while the chips wrapped onto a second line, up from %2").arg(QString::number(wrapped), QString::number(oneLine))));
        QVERIFY2(mpEditor->mpSourceEditorArea->height() < codeOneLine,
                 qPrintable(qsl("the code pane is still %1 tall, so the room the second line of chips took came from somewhere else").arg(QString::number(mpEditor->mpSourceEditorArea->height()))));

        mpEditor->resize(1100, 900);
        QCoreApplication::processEvents();
        QTest::qWait(100ms);
    }

    void test_fewerEventsGiveTheRoomBack()
    {
        mpEditor->resize(620, 900);
        QCoreApplication::processEvents();
        QTest::qWait(100ms);
        mpEditor->hideSystemMessageArea();
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QVERIFY2(row()->count() >= 3, "the script under test has lost the events this case needs it to be showing");
        const int withChips = mpEditor->mpNonCodeWidgets->height();

        mpEditor->addScript(false);
        QCoreApplication::processEvents();
        QTest::qWait(100ms);
        mpEditor->hideSystemMessageArea();
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QCOMPARE(row()->count(), 0);
        const int withNone = mpEditor->mpNonCodeWidgets->height();
        qInfo().noquote() << qsl("  the column is %1 with the chips and %2 without them").arg(QString::number(withChips), QString::number(withNone));
        QVERIFY2(withNone < withChips, qPrintable(qsl("the column stayed at %1 for a script with no events, which is what the last one's chips needed").arg(QString::number(withNone))));

        mpEditor->treeWidget_scripts->setCurrentItem(mpScriptItem);
        QCoreApplication::processEvents();
        QTest::qWait(100ms);
        mpEditor->hideSystemMessageArea();
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QVERIFY2(row()->count() >= 3, "the script's events did not come back with it");
        QCOMPARE(mpEditor->mpNonCodeWidgets->height(), withChips);

        mpEditor->resize(1100, 900);
        QCoreApplication::processEvents();
        QTest::qWait(100ms);
    }

    // The cross takes no focus, so the removal itself must commit a half-typed
    // name; with one chip there is no focus-out at all once the row empties
    void test_removingAChipSettlesTheOpenFieldFirst()
    {
        row()->setItems({qsl("AlphaEvent"), qsl("BetaEvent")});
        QCoreApplication::processEvents();
        QSignalSpy changes(row(), &uiDesign::ChipRow::itemsChanged);

        row()->beginAdd();
        QCoreApplication::processEvents();
        QVERIFY2(field() != nullptr, "beginAdd() did not open the field");
        field()->setText(qsl("GammaEvent"));

        QWidget* pChip = row()->chipAt(1);
        QVERIFY2(pChip != nullptr, "the row has no second chip to take away");
        QToolButton* pCross = pChip->findChild<QToolButton*>(qsl("editorChipRemove"));
        QVERIFY2(pCross != nullptr, "a chip has no cross to take it away with");
        pCross->click();
        QCoreApplication::processEvents();

        QCOMPARE(row()->items(), QStringList({qsl("AlphaEvent"), qsl("GammaEvent")}));
        QCOMPARE(changes.count(), 2);
        QVERIFY2(field() == nullptr, "the field is still open after a chip was taken away");
        QVERIFY2(!addButton()->isHidden(), "the add button did not come back when the field closed");

        row()->setItems({qsl("AlphaEvent")});
        QCoreApplication::processEvents();
        changes.clear();

        row()->beginAdd();
        QCoreApplication::processEvents();
        QVERIFY2(field() != nullptr, "beginAdd() did not open the field");
        field()->setText(qsl("DeltaEvent"));

        QToolButton* pOnlyCross = row()->chipAt(0)->findChild<QToolButton*>(qsl("editorChipRemove"));
        QVERIFY2(pOnlyCross != nullptr, "the one chip has no cross to take it away with");
        pOnlyCross->click();
        QCoreApplication::processEvents();

        QCOMPARE(row()->items(), QStringList{qsl("DeltaEvent")});
        QCOMPARE(changes.count(), 2);
        QVERIFY2(field() == nullptr, "the field is still open after the last chip was taken away");
        QVERIFY2(!addButton()->isHidden(), "the add button did not come back when the field closed");
    }

    void test_aRefusedNameOnTheWayOutClosesTheField()
    {
        row()->setItems({qsl("sysLoadEvent")});
        QCoreApplication::processEvents();
        const int before = row()->count();
        QSignalSpy refusals(row(), &uiDesign::ChipRow::duplicateRefused);

        row()->beginAdd();
        QCoreApplication::processEvents();
        QVERIFY2(field() != nullptr, "beginAdd() did not open the field");
        field()->setText(qsl("sysLoadEvent"));

        mpEditor->mpScriptsMainArea->lineEdit_script_name->setFocus(Qt::OtherFocusReason);
        QCoreApplication::processEvents();

        QCOMPARE(refusals.count(), 1);
        QVERIFY2(field() == nullptr, "the field was left open on a name that was refused, with nothing left to type over it");
        QVERIFY2(!addButton()->isHidden(), "the add button is still hidden behind a field that has been refused");
        QVERIFY2(note() != nullptr, "nothing is left saying why the name was not taken");
        QCOMPARE(row()->count(), before);
    }

    // The script's list can hold a name twice while the row draws it once, so
    // the search's index is not the chip's index
    void test_theSearchJumpFindsTheChipByName()
    {
        mpEditor->addScript(false);
        QCoreApplication::processEvents();
        QTest::qWait(100ms);
        QTreeWidgetItem* pOtherItem = mpEditor->mpCurrentScriptItem;
        QVERIFY2(pOtherItem != nullptr && pOtherItem != mpScriptItem, "addScript() left no second script to search into");
        mpEditor->mpScriptsMainArea->lineEdit_script_name->setText(qsl("SearchJumpScript"));
        mpEditor->slot_saveSelectedItem();
        QTest::qWait(50ms);
        QCOMPARE(pOtherItem->text(0), qsl("SearchJumpScript"));
        const int otherID = pOtherItem->data(0, Qt::UserRole).toInt();

        // Leave the second script before writing its list, or leaving it saves
        // the row's chips over it
        mpEditor->slot_scriptsSelected(mpScriptItem);
        QTest::qWait(50ms);
        TScript* pOther = mpHost->getScriptUnit()->getScript(otherID);
        QVERIFY2(pOther != nullptr, "the second script is not in the script unit");
        pOther->setEventHandlerList({qsl("MyEvent"), qsl("MyEvent"), qsl("OtherEvent")});
        QCOMPARE(pOther->getEventHandlerList().count(), 3);

        // Focus inside the row first, or a jump that pointed at nothing could pass
        addButton()->setFocus(Qt::OtherFocusReason);
        QCoreApplication::processEvents();
        QCOMPARE(row()->focusWidget(), addButton());

        auto* pResult = new QTreeWidgetItem(QStringList{qsl("SearchJumpScript")});
        pResult->setData(0, dlgTriggerEditor::ItemRole, static_cast<int>(EditorViewType::cmScriptView));
        pResult->setData(0, dlgTriggerEditor::TypeRole, dlgTriggerEditor::SearchResultIsEventHandler);
        pResult->setData(0, dlgTriggerEditor::NameRole, qsl("SearchJumpScript"));
        pResult->setData(0, dlgTriggerEditor::IdRole, otherID);
        // OtherEvent's index in the script's list; in the row it is index 1
        pResult->setData(0, dlgTriggerEditor::PatternOrLineRole, 2);
        mpEditor->treeWidget_searchResults->addTopLevelItem(pResult);

        mpEditor->slot_itemSelectedInSearchResults(pResult);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        QCOMPARE(row()->items(), QStringList({qsl("MyEvent"), qsl("OtherEvent")}));
        QCOMPARE(row()->focusWidget(), row()->chipAt(1));
    }

    // Set on the row and the window, not the application: trigger_editor.ui pins
    // a font on the frame, and Qt does not propagate a font past a widget that
    // has a stylesheet applied
    void test_aLargerFontReMeasuresTheChips()
    {
        row()->setItems({qsl("AlphaEvent"), qsl("BetaEvent")});
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QCOMPARE(row()->count(), 2);

        QLabel* pLead = mpEditor->mpScriptsMainArea->label_script_registered_event_handlers;
        const int wasLine = row()->lineHeight();
        const QFont was = row()->font();

        // Twelve points: a chip line is never shorter than the field, and the
        // form's sheet gives the field a min-height a small step stays under
        QFont bigger = was;
        bigger.setPointSizeF(QFontInfo(was).pointSizeF() + 12.0);
        row()->setFont(bigger);
        mpEditor->setFont(bigger);
        QCoreApplication::processEvents();
        QTest::qWait(100ms);

        const int line = row()->lineHeight();
        QVERIFY2(line > wasLine, qPrintable(qsl("a font twelve points larger left a line of chips at %1, which is what it was").arg(QString::number(line))));
        for (int i = 0; i < row()->count(); ++i) {
            QVERIFY2(row()->chipAt(i)->height() == line,
                     qPrintable(qsl("chip %1 is %2 tall while a line of chips is now %3").arg(QString::number(i), QString::number(row()->chipAt(i)->height()), QString::number(line))));
        }
        QVERIFY2(addButton()->height() == line, qPrintable(qsl("the button that adds an event is %1 tall while a chip is now %2").arg(QString::number(addButton()->height()), QString::number(line))));
        QVERIFY2(pLead->height() == line,
                 qPrintable(qsl("the word leading the row is %1 tall while a line of chips is now %2, so it no longer sits level with the first of them")
                                    .arg(QString::number(pLead->height()), QString::number(line))));

        row()->setFont(was);
        mpEditor->setFont(was);
        QCoreApplication::processEvents();
        QTest::qWait(100ms);
    }

    void test_typingOffersTheEventsTheProfileKnows()
    {
        mpEditor->addScript(false);
        QCoreApplication::processEvents();
        QTest::qWait(100ms);
        QTreeWidgetItem* pOtherItem = mpEditor->mpCurrentScriptItem;
        QVERIFY2(pOtherItem != nullptr && pOtherItem != mpScriptItem, "addScript() left no second script to listen for anything");
        mpEditor->mpScriptsMainArea->lineEdit_script_name->setText(qsl("SuggestionScript"));
        mpEditor->slot_saveSelectedItem();
        QTest::qWait(50ms);
        const int otherID = pOtherItem->data(0, Qt::UserRole).toInt();

        auto* pTrigger = new TTrigger(qsl("SuggestionTrigger"), QStringList(), QList<int>(), false, mpHost);
        pTrigger->registerTrigger();
        pTrigger->setScript(qsl("raiseEvent(\"CustomRaised\", 1)"));

        // Leave the second script before writing its list, or leaving it saves
        // the row over it
        mpEditor->slot_scriptsSelected(mpScriptItem);
        QTest::qWait(50ms);
        TScript* pOther = mpHost->getScriptUnit()->getScript(otherID);
        QVERIFY2(pOther != nullptr, "the second script is not in the script unit");
        pOther->setEventHandlerList({qsl("MyOtherEvent")});
        TScript* pScript = mpHost->getScriptUnit()->getScript(mpScriptItem->data(0, Qt::UserRole).toInt());
        QVERIFY2(pScript != nullptr, "the script under test is not in the script unit");
        pScript->setEventHandlerList(startingEvents());

        // Reselecting the same item reloads the form without saving, and hands the
        // row the names to offer
        mpEditor->slot_scriptsSelected(mpScriptItem);
        QTest::qWait(50ms);
        QCOMPARE(row()->items(), startingEvents());

        row()->beginAdd();
        QCoreApplication::processEvents();
        QVERIFY2(field() != nullptr, "beginAdd() did not open the field");
        keyIntoField(qsl("sys"));

        QVERIFY2(suggestionList() != nullptr && suggestionList()->isVisible(), "typing part of an event name offered nothing");
        const QStringList onSys = offered();
        QVERIFY2(!onSys.isEmpty(), "the list of offered names is empty");
        for (const QString& name : onSys) {
            QVERIFY2(name.contains(qsl("sys"), Qt::CaseInsensitive), qPrintable(qsl("%1 was offered for \"sys\", which it does not carry").arg(name)));
        }
        QVERIFY2(onSys.contains(qsl("sysLoadEvent")), "an event Mudlet raises itself was not offered");
        QVERIFY2(!onSys.contains(qsl("sysConnectionEvent")), "a name the script is already showing was offered, which adding would only have refused");

        retypeField(qsl("myother"));
        QVERIFY2(offered().contains(qsl("MyOtherEvent")), qPrintable(qsl("what another script listens for was not offered: %1").arg(offered().join(qsl(", ")))));

        retypeField(qsl("custom"));
        QVERIFY2(offered().contains(qsl("CustomRaised")), qPrintable(qsl("what a trigger's Lua raises was not offered: %1").arg(offered().join(qsl(", ")))));

        QTest::keyClick(field(), Qt::Key_Escape);
        QCoreApplication::processEvents();
    }

    // Every item's code is read when the field opens, not on each click on a script: a profile with megabytes of
    // Lua spent tens of milliseconds a click on it, and a name written since the click is still offered
    void test_theNamesAreGatheredWhenTheFieldOpens()
    {
        mpEditor->slot_scriptsSelected(mpScriptItem);
        QTest::qWait(50ms);

        auto* pTrigger = new TTrigger(qsl("LateTrigger"), QStringList(), QList<int>(), false, mpHost);
        pTrigger->registerTrigger();
        pTrigger->setScript(qsl("raiseEvent(\"RaisedAfterTheClick\")"));

        row()->beginAdd();
        QCoreApplication::processEvents();
        QVERIFY2(field() != nullptr, "beginAdd() did not open the field");
        keyIntoField(qsl("raisedafter"));
        QVERIFY2(offered().contains(qsl("RaisedAfterTheClick")), qPrintable(qsl("a name written after the script was chosen was not offered: %1").arg(offered().join(qsl(", ")))));

        QTest::keyClick(field(), Qt::Key_Escape);
        QCoreApplication::processEvents();
    }

    // A layout's margins stay where they are written, so the chip mirrors its own
    void test_aChipKeepsItsLeadingPaddingRightToLeft()
    {
        mpEditor->slot_scriptsSelected(mpScriptItem);
        QTest::qWait(50ms);
        QWidget* pChip = row()->chipAt(0);
        QVERIFY2(pChip != nullptr, "SETUP: the script shows no chip");
        auto* pLabel = pChip->findChild<QLabel*>(qsl("editorChipLabel"));
        QVERIFY2(pLabel != nullptr, "the chip has no name on it");
        const int leading = pLabel->geometry().left();
        const auto restore = qScopeGuard([this]() {
            row()->setLayoutDirection(Qt::LeftToRight);
            QCoreApplication::processEvents();
        });

        row()->setLayoutDirection(Qt::RightToLeft);
        QCoreApplication::processEvents();
        pChip->layout()->activate();
        QCOMPARE(pChip->width() - 1 - pLabel->geometry().right(), leading);
    }

    void test_aChosenSuggestionBecomesAChip()
    {
        reopenFieldOn(startingEvents());
        QVERIFY2(field() != nullptr, "beginAdd() did not open the field");
        keyIntoField(qsl("sysLoa"));
        QAbstractItemView* pList = suggestionList();
        QVERIFY2(pList != nullptr && pList->isVisible(), "typing part of an event name offered nothing");

        // Keys go to the popup: it takes the keyboard while shown, as in the window
        QTest::keyClick(pList, Qt::Key_Down);
        QCoreApplication::processEvents();
        QVERIFY2(pList->currentIndex().isValid(), "Down did not walk into the list");
        QTest::keyClick(pList, Qt::Key_Return);

        // The chip is added through a queued connection, after QLineEdit fills the
        // field from the chosen name
        QTRY_VERIFY2(row()->items().contains(qsl("sysLoadEvent")), "the name chosen off the list did not become a chip");
        QVERIFY2(!pList->isVisible(), "the list is still up after a name was taken off it");
        QVERIFY2(field() != nullptr, "the field closed after a name was chosen, so the next one costs another click");
        QCOMPARE(field()->text(), QString());

        QTest::keyClick(field(), Qt::Key_Escape);
        QCoreApplication::processEvents();
    }

    void test_returnKeepsTheTypedNameOverTheList()
    {
        reopenFieldOn(startingEvents());
        QVERIFY2(field() != nullptr, "beginAdd() did not open the field");
        keyIntoField(qsl("sysLoa"));
        QAbstractItemView* pList = suggestionList();
        QVERIFY2(pList != nullptr && pList->isVisible(), "typing part of an event name offered nothing");
        QVERIFY2(!pList->currentIndex().isValid(), "typing alone walked into the list, so this case cannot tell the two apart");

        QTest::keyClick(pList, Qt::Key_Return);
        QCoreApplication::processEvents();

        QVERIFY2(row()->items().contains(qsl("sysLoa")), qPrintable(qsl("the typed name was not what was taken - the row shows %1").arg(row()->items().join(qsl(", ")))));
        QVERIFY2(!row()->items().contains(qsl("sysLoadEvent")), "the offered name was taken over the one that was typed");
        QVERIFY2(!pList->isVisible(), "the list is still up after the typed name was taken");

        QTest::keyClick(field(), Qt::Key_Escape);
        QCoreApplication::processEvents();
    }

    void test_theChipsStandAsTallAsTheField()
    {
        QWidget* pForm = row()->parentWidget();
        QVERIFY2(pForm != nullptr, "the row is not on a form to style");
        const QString formSheet = pForm->styleSheet();
        // Taller than any chip, as the editor's field rule makes it on a real screen
        pForm->setStyleSheet(formSheet + qsl(" QLineEdit { min-height: 40px; }"));
        QCoreApplication::processEvents();

        reopenFieldOn(startingEvents());
        QVERIFY2(field() != nullptr, "beginAdd() did not open the field");
        QWidget* pChip = row()->chipAt(0);
        QVERIFY2(pChip != nullptr, "the row has no chip to measure against the field");

        QTRY_VERIFY2(field()->height() >= 40, qPrintable(qsl("the field was cut down to %1px").arg(field()->height())));
        QVERIFY2(field()->height() >= field()->sizeHint().height(), "the field stands shorter than its sheet makes it");
        QCOMPARE(pChip->height(), field()->height());
        QCOMPARE(addButton()->minimumHeight(), field()->height());
        QCOMPARE(row()->lineHeight(), field()->height());

        QTest::keyClick(field(), Qt::Key_Escape);
        QCoreApplication::processEvents();
        pForm->setStyleSheet(formSheet);
        QCoreApplication::processEvents();
        QCOMPARE(row()->lineHeight(), pChip->height());
    }

    void test_anEventTheGameSentIsOffered()
    {
        raiseInProfile(qsl("gmcp.Char.Vitals"));
        raiseInProfile(qsl("RaisedAtRuntime"));

        // The row is handed the names to offer only as a script loads into the form
        mpEditor->slot_scriptsSelected(mpScriptItem);
        QTest::qWait(50ms);
        reopenFieldOn(startingEvents());
        QVERIFY2(field() != nullptr, "beginAdd() did not open the field");

        keyIntoField(qsl("vitals"));
        QVERIFY2(offered().contains(qsl("gmcp.Char.Vitals")), qPrintable(qsl("what the game sent was not offered: %1").arg(offered().join(qsl(", ")))));
        QCOMPARE(noteFor(qsl("gmcp.Char.Vitals")), qsl("game"));

        retypeField(qsl("raisedat"));
        QVERIFY2(offered().contains(qsl("RaisedAtRuntime")), qPrintable(qsl("a name raised at run time was not offered: %1").arg(offered().join(qsl(", ")))));
        QCOMPARE(noteFor(qsl("RaisedAtRuntime")), qsl("script"));

        QTest::keyClick(field(), Qt::Key_Escape);
        QCoreApplication::processEvents();
    }

    void test_eachNameSaysWhereItIsFrom()
    {
        reopenFieldOn(startingEvents());
        QVERIFY2(field() != nullptr, "beginAdd() did not open the field");

        keyIntoField(qsl("sysload"));
        QVERIFY2(offered().contains(qsl("sysLoadEvent")), qPrintable(qsl("an event Mudlet raises was not offered: %1").arg(offered().join(qsl(", ")))));
        QCOMPARE(noteFor(qsl("sysLoadEvent")), qsl("Mudlet"));

        // MyOtherEvent was put on a script by test_typingOffersTheEventsTheProfileKnows
        retypeField(qsl("myother"));
        QVERIFY2(offered().contains(qsl("MyOtherEvent")), qPrintable(qsl("what another script listens for was not offered: %1").arg(offered().join(qsl(", ")))));
        QCOMPARE(noteFor(qsl("MyOtherEvent")), qsl("script"));

        QTest::keyClick(field(), Qt::Key_Escape);
        QCoreApplication::processEvents();
    }

    // Read off where the editor says a name comes from, not off its spelling:
    // Mudlet raises mapOpenEvent without the sys prefix, and a script can give an
    // event of its own one
    void test_eachChipSaysWhereItsEventComesFrom()
    {
        const QString fromMudlet = qsl("Raised by Mudlet");
        const QString fromTheGame = qsl("Sent by the game");
        const QString fromAScript = qsl("Raised or listened for by a script in this profile");
        const QString unseen = qsl("Not seen in this profile yet");
        struct Expected
        {
            QString name;
            bool quiet;
            QString origin;
        };
        const QList<Expected> expected{{qsl("mapOpenEvent"), true, fromMudlet},
                                       {qsl("sysMapAreaChanged"), true, fromMudlet},
                                       {qsl("gmcp.Char.Vitals"), false, fromTheGame},
                                       {qsl("sysMadeUpByAScript"), false, fromAScript},
                                       {qsl("ChipToneOwnEvent"), false, fromAScript}};
        QStringList events;
        for (const Expected& chip : expected) {
            events << chip.name;
        }

        TScript* pScript = mpHost->getScriptUnit()->getScript(mpScriptItem->data(0, Qt::UserRole).toInt());
        QVERIFY2(pScript != nullptr, "the script under test is not in the script unit");
        pScript->setEventHandlerList(events);
        // Emptied first, so the chips the reselect makes know nothing of their names
        // and only the suggestions handed over after them can set them right
        row()->setSuggestions({});
        mpEditor->slot_scriptsSelected(mpScriptItem);
        QTest::qWait(50ms);
        QCOMPARE(row()->items(), events);

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const auto expectChip = [this, &tokens](const Expected& chip) {
            QWidget* pChip = row()->chipAt(row()->indexOf(chip.name));
            if (!pChip) {
                return qsl("there is no chip for %1").arg(chip.name);
            }
            const bool quiet = pChip->property("editorChipSystem").toBool();
            if (quiet != chip.quiet) {
                return qsl("%1 is marked %2, though Mudlet %3 raise it").arg(chip.name, quiet ? qsl("as Mudlet's own") : qsl("as not Mudlet's"), chip.quiet ? qsl("does") : qsl("does not"));
            }
            const QColor ink = wordInk(pChip, tokens.card);
            const QColor wanted = chip.quiet ? tokens.mutedText : tokens.text;
            const QColor other = chip.quiet ? tokens.text : tokens.mutedText;
            qInfo().noquote() << qsl("  %1 is written in %2, against %3 for Mudlet's events and %4 for the rest").arg(chip.name, ink.name(), tokens.mutedText.name(), tokens.text.name());
            if (TestPixels::distance(ink, wanted) >= TestPixels::distance(ink, other)) {
                return qsl("%1 is written in %2, nearer %3 than the %4 it is marked for").arg(chip.name, ink.name(), other.name(), wanted.name());
            }
            if (!originOn(pChip).contains(chip.origin)) {
                return qsl("hovering %1 says \"%2\" rather than \"%3\"").arg(chip.name, originOn(pChip), chip.origin);
            }
            const QToolButton* pCross = pChip->findChild<QToolButton*>(qsl("editorChipRemove"));
            if (!pCross || !pCross->toolTip().contains(qsl("Stop listening for %1").arg(chip.name))) {
                return qsl("the cross on %1 no longer says what it does").arg(chip.name);
            }
            if (pChip->accessibleName() != qsl("Event %1").arg(chip.name)) {
                return qsl("%1 is announced as \"%2\"").arg(chip.name, pChip->accessibleName());
            }
            return QString();
        };
        for (const Expected& chip : expected) {
            const QString miss = expectChip(chip);
            QVERIFY2(miss.isEmpty(), qPrintable(miss));
        }

        // Typed after the suggestions came, so the chip is made knowing them
        row()->beginAdd();
        QCoreApplication::processEvents();
        typeIntoField(qsl("ChipToneUnseenEvent"));
        QTest::keyClick(field(), Qt::Key_Escape);
        QCoreApplication::processEvents();
        const QString unseenMiss = expectChip({qsl("ChipToneUnseenEvent"), false, unseen});
        QVERIFY2(unseenMiss.isEmpty(), qPrintable(unseenMiss));

        // A rename keeps the chip, so it is the one thing that has to change its mind
        QWidget* pRenamed = row()->chipAt(row()->indexOf(qsl("ChipToneUnseenEvent")));
        QVERIFY2(pRenamed != nullptr, "the typed name did not become a chip");
        QTest::keyClick(pRenamed, Qt::Key_F2);
        QCoreApplication::processEvents();
        QVERIFY2(field() != nullptr, "F2 on a chip did not open it for renaming");
        typeIntoField(qsl("sysLoadEvent"));
        QCOMPARE(row()->chipAt(row()->indexOf(qsl("sysLoadEvent"))), pRenamed);
        const QString renamedMiss = expectChip({qsl("sysLoadEvent"), true, fromMudlet});
        QVERIFY2(renamedMiss.isEmpty(), qPrintable(renamedMiss));
    }
    // Typing cleans a name, but one a profile file carries is its own event: a script listening for
    // "ready," must not be moved to "ready" just because it was opened and saved
    void test_aStoredNameIsKeptAsItIs()
    {
        TScript* pScript = mpHost->getScriptUnit()->getScript(mpScriptItem->data(0, Qt::UserRole).toInt());
        QVERIFY2(pScript != nullptr, "the script is not in the script unit");
        const QStringList stored{qsl("ready,"), qsl(" padded "), qsl("MyEvent")};
        pScript->setEventHandlerList(stored);
        mpEditor->slot_scriptsSelected(mpScriptItem);
        QTest::qWait(50ms);
        QCOMPARE(row()->items(), stored);

        mpEditor->slot_saveSelectedItem();
        QTest::qWait(50ms);
        QCOMPARE(savedEvents(), stored);

        pScript->setEventHandlerList(startingEvents());
        mpEditor->slot_scriptsSelected(mpScriptItem);
        QTest::qWait(50ms);
    }
    // A script can build a new name every time it raises one, so only the most recent of those are
    // offered; the game's names stay, and so does a script's name it keeps raising
    void test_namesBuiltAtRunTimeDoNotPileUp()
    {
        auto offeredNames = [this]() {
            QStringList names;
            for (const eventNames::Entry& entry : mpHost->knownEventNames()) {
                names << entry.name;
            }
            return names;
        };
        raiseInProfile(qsl("gmcp.Room.Info"));
        for (int i = 0; i < 1000; ++i) {
            raiseInProfile(qsl("counterBuilt.%1").arg(i));
        }
        QStringList offered = offeredNames();
        const qsizetype built = offered.filter(qsl("counterBuilt.")).size();
        // 200 kept, and up to 50 more before the next trim (Host.h)
        QVERIFY2(built >= 200 && built <= 250, qPrintable(qsl("%1 of the 1000 names built at run time are offered").arg(built)));
        QVERIFY2(offered.contains(qsl("counterBuilt.999")), "the newest name built at run time is not offered");
        QVERIFY2(!offered.contains(qsl("counterBuilt.0")), "the oldest name built at run time is still offered");
        QVERIFY2(offered.contains(qsl("gmcp.Room.Info")), "a name the game sent was dropped with the script's");

        // Raised again while still kept, then outlived by 150 newer names: only the second raise keeps it
        raiseInProfile(qsl("RaisedAgain"));
        for (int i = 0; i < 100; ++i) {
            raiseInProfile(qsl("laterBuilt.%1").arg(i));
        }
        raiseInProfile(qsl("RaisedAgain"));
        for (int i = 100; i < 250; ++i) {
            raiseInProfile(qsl("laterBuilt.%1").arg(i));
        }
        QVERIFY2(offeredNames().contains(qsl("RaisedAgain")), "a name raised again was dropped with the old ones");
    }
};

#include "EditorEventChipRowTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorEventChipRowTest)
