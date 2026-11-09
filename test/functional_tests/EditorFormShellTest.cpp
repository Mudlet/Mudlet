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
 * The five fixed-field forms hold their column to their fields' height against the
 * code pane's handle, the Timers root row hides the code pane as the other roots do,
 * and every shelled form's fields line up with its name field.
 *
 * Run with: ctest -R EditorFormShellTest -V
 */

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QtTest/QtTest>
#include <chrono>

#include "ChipRow.h"
#include "GripSplitter.h"
#include "Host.h"
#include "HostDialogs.h"
#include "ProfileSessionTestHelper.h"
#include "dlgActionMainArea.h"
#include "dlgAliasMainArea.h"
#include "dlgKeysMainArea.h"
#include "dlgScriptsMainArea.h"
#include "dlgSourceEditorArea.h"
#include "dlgTimersMainArea.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggersMainArea.h"
#include "dlgVarsMainArea.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorFormShellTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorFormShell-Test-Profile");

    static constexpr int scmPush = 200;

    // The window's frame, since each field is nested a different number of layouts deep
    int leftEdgeOf(QWidget* pWidget) const { return pWidget->mapTo(mpEditor, QPoint(0, 0)).x(); }

    int topEdgeOf(const QWidget* pWidget) const { return pWidget->mapTo(mpEditor, QPoint(0, 0)).y(); }

    void enterView(const EditorViewType view)
    {
        switch (view) {
        case EditorViewType::cmTriggerView:
            mpEditor->slot_showTriggers();
            break;
        case EditorViewType::cmAliasView:
            mpEditor->slot_showAliases();
            break;
        case EditorViewType::cmTimerView:
            mpEditor->slot_showTimers();
            break;
        case EditorViewType::cmKeysView:
            mpEditor->slot_showKeys();
            break;
        case EditorViewType::cmScriptView:
            mpEditor->slot_showScripts();
            break;
        case EditorViewType::cmVarsView:
            mpEditor->slot_showVariables();
            break;
        case EditorViewType::cmActionView:
            mpEditor->slot_showActions();
            break;
        default:
            QFAIL("this test knows nothing about that view");
        }
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
    }

    void chooseTheFirstItem(QTreeWidget* pTree)
    {
        QTreeWidgetItem* pBase = pTree->topLevelItem(0);
        if (!pBase || !pBase->childCount()) {
            return;
        }
        QTreeWidgetItem* pRow = pBase->child(0);
        pTree->setCurrentItem(pRow);
        emit pTree->itemClicked(pRow, 0);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
    }

    QLineEdit* nameFieldOf(const EditorViewType view) const
    {
        switch (view) {
        case EditorViewType::cmTriggerView:
            return mpEditor->mpTriggersMainArea->lineEdit_trigger_name;
        case EditorViewType::cmAliasView:
            return mpEditor->mpAliasMainArea->lineEdit_alias_name;
        case EditorViewType::cmTimerView:
            return mpEditor->mpTimersMainArea->lineEdit_timer_name;
        case EditorViewType::cmKeysView:
            return mpEditor->mpKeysMainArea->lineEdit_key_name;
        case EditorViewType::cmScriptView:
            return mpEditor->mpScriptsMainArea->lineEdit_script_name;
        case EditorViewType::cmVarsView:
            return mpEditor->mpVarsMainArea->lineEdit_var_name;
        case EditorViewType::cmActionView:
            return mpEditor->mpActionsMainArea->lineEdit_action_name;
        default:
            return nullptr;
        }
    }

    QWidget* leadFieldOf(const EditorViewType view) const
    {
        switch (view) {
        case EditorViewType::cmAliasView:
            return mpEditor->mpAliasMainArea->lineEdit_alias_pattern;
        case EditorViewType::cmKeysView:
            return mpEditor->mpKeysMainArea->lineEdit_key_binding;
        case EditorViewType::cmVarsView:
            return mpEditor->mpVarsMainArea->comboBox_variable_key_type;
        case EditorViewType::cmScriptView:
            return mpEditor->mpChipRow_scriptEvents;
        case EditorViewType::cmTimerView:
            // The sentence starts with words, so the row rather than its first field aligns with the name
            return mpEditor->mpWidget_timerInterval;
        case EditorViewType::cmActionView:
            // The rotation picker is the first thing on the row leading the Buttons form
            return mpEditor->mpActionsMainArea->comboBox_action_button_rotation;
        default:
            return nullptr;
        }
    }

    QWidget* formAreaOf(const EditorViewType view) const
    {
        switch (view) {
        case EditorViewType::cmTriggerView:
            return mpEditor->mpTriggersMainArea;
        case EditorViewType::cmAliasView:
            return mpEditor->mpAliasMainArea;
        case EditorViewType::cmTimerView:
            return mpEditor->mpTimersMainArea;
        case EditorViewType::cmKeysView:
            return mpEditor->mpKeysMainArea;
        case EditorViewType::cmScriptView:
            return mpEditor->mpScriptsMainArea;
        case EditorViewType::cmVarsView:
            return mpEditor->mpVarsMainArea;
        case EditorViewType::cmActionView:
            return mpEditor->mpActionsMainArea;
        default:
            return nullptr;
        }
    }

    static QString nameOf(const EditorViewType view)
    {
        switch (view) {
        case EditorViewType::cmTriggerView:
            return qsl("triggers");
        case EditorViewType::cmAliasView:
            return qsl("aliases");
        case EditorViewType::cmTimerView:
            return qsl("timers");
        case EditorViewType::cmKeysView:
            return qsl("keys");
        case EditorViewType::cmScriptView:
            return qsl("scripts");
        case EditorViewType::cmVarsView:
            return qsl("variables");
        case EditorViewType::cmActionView:
            return qsl("buttons");
        default:
            return qsl("an unknown view");
        }
    }

    static QList<EditorViewType> fixedViews()
    {
        return {EditorViewType::cmAliasView, EditorViewType::cmTimerView, EditorViewType::cmKeysView, EditorViewType::cmScriptView, EditorViewType::cmVarsView};
    }

    // Buttons is not a fixed form, as its stylesheet editor takes any room the column gets, but
    // its name and rows are laid out to the same measurements
    static QList<EditorViewType> shelledViews()
    {
        QList<EditorViewType> views = fixedViews();
        views.append(EditorViewType::cmActionView);
        return views;
    }

    void openAndChoose(const EditorViewType view)
    {
        enterView(view);
        switch (view) {
        case EditorViewType::cmTriggerView:
            chooseTheFirstItem(mpEditor->treeWidget_triggers);
            break;
        case EditorViewType::cmAliasView:
            chooseTheFirstItem(mpEditor->treeWidget_aliases);
            break;
        case EditorViewType::cmTimerView:
            chooseTheFirstItem(mpEditor->treeWidget_timers);
            break;
        case EditorViewType::cmKeysView:
            chooseTheFirstItem(mpEditor->treeWidget_keys);
            break;
        case EditorViewType::cmScriptView:
            chooseTheFirstItem(mpEditor->treeWidget_scripts);
            break;
        case EditorViewType::cmVarsView:
            chooseTheFirstItem(mpEditor->treeWidget_variables);
            break;
        case EditorViewType::cmActionView:
            chooseTheFirstItem(mpEditor->treeWidget_actions);
            break;
        default:
            break;
        }
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
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1100, 900);
        QTest::qWait(50ms);

        // One item of every kind, so no form shows the empty-view notice instead
        mpEditor->slot_showTriggers();
        mpEditor->addTrigger(false);
        mpEditor->slot_showAliases();
        mpEditor->addAlias(false);
        mpEditor->slot_showTimers();
        mpEditor->addTimer(false);
        mpEditor->slot_showKeys();
        mpEditor->addKey(false);
        mpEditor->slot_showScripts();
        mpEditor->addScript(false);
        mpEditor->slot_showActions();
        mpEditor->addAction(false);
        QTest::qWait(100ms);
        QVERIFY2(mpEditor->mpCurrentActionItem != nullptr, "addAction() left no current button, so the buttons form has nothing to show");
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_theCodeHeadingOnlyDragsWhereTheFormCanUseTheRoom()
    {
        openAndChoose(EditorViewType::cmTriggerView);
        QVERIFY2(mpEditor->splitter_right->handleResizes(1), "the code pane's heading has stopped resizing the trigger form, which holds the pattern list");

        for (const EditorViewType view : fixedViews()) {
            openAndChoose(view);
            QVERIFY2(!mpEditor->splitter_right->handleResizes(1), qPrintable(qsl("the code pane's heading still resizes the %1 form, which is a fixed set of fields").arg(nameOf(view))));
        }
    }

    // Where the item has nothing to edit (a Lua table) the code pane is away, and the form must
    // then lead the pane at its own height rather than be centred in it
    void test_aFixedFormColumnStaysAtTheHeightItsFieldsAskFor()
    {
        // The same y in every fixed view, so a form with no code pane under it is held to it
        int formTopOverACodePane = 0;
        for (const EditorViewType view : fixedViews()) {
            openAndChoose(view);
            // The notice's label wraps, so its minimum is taller than the line it shows and the
            // column's size hint would stop matching its fields
            mpEditor->hideSystemMessageArea();
            QCoreApplication::processEvents();
            QTest::qWait(50ms);

            QWidget* pForm = formAreaOf(view);
            QVERIFY2(pForm != nullptr && pForm->isVisible(), qPrintable(qsl("the %1 form is not showing, so there is nothing to hold to a height").arg(nameOf(view))));
            if (mpEditor->mpSourceEditorArea->isHidden()) {
                const int wantedForm = pForm->sizeHint().height();
                qInfo().noquote()
                        << qsl("  %1: no code pane, so the form leads the pane - it starts at %2 where one over a code pane starts at %3, and is %4 tall where its fields ask for %5")
                                   .arg(nameOf(view), QString::number(topEdgeOf(pForm)), QString::number(formTopOverACodePane), QString::number(pForm->height()), QString::number(wantedForm));
                QVERIFY2(formTopOverACodePane > 0, "no view in this walk came up with a code pane, so there is nothing to measure a form without one against");
                QVERIFY2(topEdgeOf(pForm) == topEdgeOf(mpEditor->mpNonCodeWidgets),
                         qPrintable(qsl("the %1 form starts at %2 in a column starting at %3")
                                            .arg(nameOf(view), QString::number(topEdgeOf(pForm)), QString::number(topEdgeOf(mpEditor->mpNonCodeWidgets)))));
                QVERIFY2(topEdgeOf(pForm) == formTopOverACodePane,
                         qPrintable(qsl("the %1 form starts at %2 with no code pane under it and at %3 with one, so it is being centred in the pane rather than leading it")
                                            .arg(nameOf(view), QString::number(topEdgeOf(pForm)), QString::number(formTopOverACodePane))));
                QVERIFY2(pForm->height() == wantedForm,
                         qPrintable(
                                 qsl("the %1 form is %2 tall with the pane to itself, where its fields ask for %3").arg(nameOf(view), QString::number(pForm->height()), QString::number(wantedForm))));
                continue;
            }

            formTopOverACodePane = topEdgeOf(pForm);
            const int wanted = mpEditor->mpNonCodeWidgets->sizeHint().height();
            QList<int> sizes = mpEditor->splitter_right->sizes();
            QVERIFY2(sizes.size() >= 2, "the right hand splitter has lost a pane");
            qInfo().noquote() << qsl("  %1: the column asks for %2 and the panes are %3 / %4").arg(nameOf(view), QString::number(wanted), QString::number(sizes.at(0)), QString::number(sizes.at(1)));
            QCOMPARE(mpEditor->mpNonCodeWidgets->height(), wanted);

            sizes[1] -= scmPush;
            sizes[0] += scmPush;
            mpEditor->splitter_right->setSizes(sizes);
            QCoreApplication::processEvents();
            QTest::qWait(50ms);

            // The widget's height, not sizes(): QSplitter records the size it asked for, and the
            // column's own maximum is what holds it
            QVERIFY2(mpEditor->mpNonCodeWidgets->height() == wanted,
                     qPrintable(qsl("the %1 form column was pushed to %2 and stayed there - it asks for %3")
                                        .arg(nameOf(view), QString::number(mpEditor->mpNonCodeWidgets->height()), QString::number(wanted))));
        }
    }

    void test_theTriggerFormColumnStillTakesAPush()
    {
        openAndChoose(EditorViewType::cmTriggerView);
        QList<int> sizes = mpEditor->splitter_right->sizes();
        QVERIFY2(sizes.at(1) > scmPush + 150, "not enough code pane to take the push off for this case");
        const int before = sizes.at(0);
        sizes[0] += scmPush;
        sizes[1] -= scmPush;
        mpEditor->splitter_right->setSizes(sizes);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        QVERIFY2(mpEditor->splitter_right->sizes().at(0) > before,
                 qPrintable(qsl("the trigger form column was pushed from %1 and came back to %2").arg(QString::number(before), QString::number(mpEditor->splitter_right->sizes().at(0)))));
    }

    void test_theTimersRootRowHidesTheCodePane()
    {
        openAndChoose(EditorViewType::cmTimerView);
        QVERIFY2(mpEditor->mpSourceEditorArea->isVisible(), "the code pane is not showing for a chosen timer, so this case says nothing");

        QTreeWidgetItem* pRoot = mpEditor->treeWidget_timers->topLevelItem(0);
        QVERIFY2(pRoot != nullptr, "the timers tree has no root row");
        mpEditor->treeWidget_timers->setCurrentItem(pRoot);
        emit mpEditor->treeWidget_timers->itemClicked(pRoot, 0);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        QVERIFY2(!mpEditor->mpTimersMainArea->isVisible(), "the timer form is still showing on the root row");
        QVERIFY2(!mpEditor->mpSourceEditorArea->isVisible(), "the code pane is still showing on the timers root row, holding the last timer's script under the placeholder notice");
    }

    void test_everyViewsNameFieldStartsAtTheSameX()
    {
        openAndChoose(EditorViewType::cmTriggerView);
        const int triggerEdge = leftEdgeOf(nameFieldOf(EditorViewType::cmTriggerView));
        QVERIFY2(triggerEdge > 0, "the trigger form's name field is not on the window");

        for (const EditorViewType view : shelledViews()) {
            openAndChoose(view);
            const int edge = leftEdgeOf(nameFieldOf(view));
            qInfo().noquote() << qsl("  %1: the name field starts at %2, the trigger form's at %3").arg(nameOf(view), QString::number(edge), QString::number(triggerEdge));
            QVERIFY2(edge == triggerEdge,
                     qPrintable(qsl("the %1 form types its name at %2 while the trigger form types it at %3").arg(nameOf(view), QString::number(edge), QString::number(triggerEdge))));
        }
    }

    void test_aRowUnderTheHeadRowStartsWhereTheNameDoes()
    {
        for (const EditorViewType view : shelledViews()) {
            QWidget* pLeadField = leadFieldOf(view);
            if (!pLeadField) {
                continue;
            }
            openAndChoose(view);
            QVERIFY2(pLeadField->isVisible(), qPrintable(qsl("the %1 form's second row is not showing").arg(nameOf(view))));
            const int nameEdge = leftEdgeOf(nameFieldOf(view));
            const int leadEdge = leftEdgeOf(pLeadField);
            qInfo().noquote() << qsl("  %1: the name field starts at %2 and the row under it at %3").arg(nameOf(view), QString::number(nameEdge), QString::number(leadEdge));
            QVERIFY2(leadEdge == nameEdge,
                     qPrintable(qsl("the %1 form starts the row under its name at %2 while the name starts at %3").arg(nameOf(view), QString::number(leadEdge), QString::number(nameEdge))));
        }
    }

    // TKey::match() never matches a folder, so a key group has no keystroke to set
    void test_aKeyGroupIsNotOfferedAKeystroke()
    {
        openAndChoose(EditorViewType::cmKeysView);
        QVERIFY2(mpEditor->mpKeysMainArea->lineEdit_key_binding->isVisible(), "a key is not offered the row that sets its keystroke");

        mpEditor->slot_showKeys();
        mpEditor->addKey(true);
        QCoreApplication::processEvents();
        QTest::qWait(100ms);

        QVERIFY2(!mpEditor->mpKeysMainArea->lineEdit_key_binding->isVisible(), "a key group is still offered a keystroke it can never match");
        QVERIFY2(!mpEditor->mpLabel_keyHint->isVisible(), "a key group is still told how to set a keystroke");
    }

    void test_theButtonsFormIsBuiltLikeTheOtherForms()
    {
        openAndChoose(EditorViewType::cmActionView);
        // The previous case left the column capped to a fixed form; the cap comes off on the layout
        // request this view's entry posts, which has to arrive before anything is measured
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        auto* pGrid = qobject_cast<QGridLayout*>(mpEditor->mpActionsMainArea->layout());
        QVERIFY2(pGrid != nullptr, "the buttons form is not laid out as a grid, so it was never shelled over its .ui file");

        // Measured off the alias form, not named: the width depends on the font the window runs at
        const int leadWidth = mpEditor->mpAliasMainArea->label_alias_pattern->minimumWidth();
        QVERIFY2(leadWidth > 0, "the alias form's lead word has no width of its own, so there is nothing to hold the buttons form's to");

        QStringList words;
        for (int i = 0; i < pGrid->count(); ++i) {
            int row = 0;
            int column = 0;
            int rowSpan = 1;
            int columnSpan = 1;
            pGrid->getItemPosition(i, &row, &column, &rowSpan, &columnSpan);
            // The head row spans the grid; a row's lead word sits alone in column 0
            if (column != 0 || columnSpan != 1) {
                continue;
            }
            auto* pLabel = qobject_cast<QLabel*>(pGrid->itemAt(i)->widget());
            if (!pLabel || !pLabel->isVisible()) {
                continue;
            }
            words << pLabel->text();
            QVERIFY2(!pLabel->text().trimmed().endsWith(QChar(':')), qPrintable(qsl("the buttons form leads a row with \"%1\", which ends in a colon").arg(pLabel->text())));
            QVERIFY2(
                    pLabel->width() == leadWidth,
                    qPrintable(qsl("the buttons form's \"%1\" is %2 wide where every other form's lead word is %3").arg(pLabel->text(), QString::number(pLabel->width()), QString::number(leadWidth))));
        }
        qInfo().noquote() << qsl("  the buttons form leads its rows with %1").arg(words.join(qsl(", ")));
        QVERIFY2(words.size() >= 3, qPrintable(qsl("a chosen button shows %1 row(s) of the buttons form, where rotation, command and the stylesheet are three").arg(QString::number(words.size()))));

        QPlainTextEdit* pStyleSheet = mpEditor->mpActionsMainArea->plainTextEdit_action_css;
        QVERIFY2(pStyleSheet->isVisible(), "the buttons form is not showing the stylesheet editor, so there is nothing for the column's room to go to");
        const int formBefore = mpEditor->mpActionsMainArea->height();
        const int styleSheetBefore = pStyleSheet->height();
        QList<int> sizes = mpEditor->splitter_right->sizes();
        QVERIFY2(sizes.size() >= 2 && sizes.at(1) > scmPush + 150, "not enough code pane to take the push off for this case");
        sizes[0] += scmPush;
        sizes[1] -= scmPush;
        mpEditor->splitter_right->setSizes(sizes);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        const int formGrew = mpEditor->mpActionsMainArea->height() - formBefore;
        const int styleSheetGrew = pStyleSheet->height() - styleSheetBefore;
        qInfo().noquote() << qsl("  pushed by %1: the form grew by %2 and the stylesheet by %3").arg(QString::number(scmPush), QString::number(formGrew), QString::number(styleSheetGrew));
        QVERIFY2(formGrew > 0, "the buttons form column did not take the push, so there is no room for anything on it to have been given");
        QVERIFY2(
                styleSheetGrew == formGrew,
                qPrintable(qsl("the buttons form grew by %1 and its stylesheet by %2, so the rest of it went between the rows above").arg(QString::number(formGrew), QString::number(styleSheetGrew))));
    }
};

#include "EditorFormShellTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorFormShellTest)
