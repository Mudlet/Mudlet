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
 * Guards the variables form's layout, the hidden-variables switch's alignment,
 * and the code pane's caret readout and heading in the Variables view.
 *
 * Run with: ctest -R EditorVariablesFormTest -V
 */

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QStatusBar>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "ProfileSessionTestHelper.h"
#include "TLuaInterpreter.h"
#include "dlgSourceEditorArea.h"
#include "dlgTriggerEditor.h"
#include "dlgVarsMainArea.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorVariablesFormTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    QTreeWidget* mpVariablesTree = nullptr;
    const QString mProfileName = qsl("EditorVariablesForm-Test-Profile");
    // Code points rather than the glyphs, so this file reads the same wherever it is opened
    static constexpr char16_t scmLeftPointingArrow = 0x23f4;
    static constexpr char16_t scmDownPointingArrow = 0x23f7;

    // Mapped to the editor: each control is nested a different number of layouts deep
    int leftEdgeOf(QWidget* pWidget) const { return pWidget->mapTo(mpEditor, QPoint(0, 0)).x(); }
    int topEdgeOf(QWidget* pWidget) const { return pWidget->mapTo(mpEditor, QPoint(0, 0)).y(); }

    dlgVarsMainArea* form() const { return mpEditor->mpVarsMainArea; }
    QString caretReadoutSays() const
    {
        QLabel* pCaret = mpEditor->findChild<QLabel*>(qsl("editorCodeCaret"));
        return (pCaret && pCaret->isVisible()) ? pCaret->text() : QString();
    }

    void openTheVariablesView()
    {
        mpEditor->slot_showVariables();
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
    }

    QTreeWidgetItem* rootRow() const { return mpVariablesTree->topLevelItem(0); }

    QTreeWidgetItem* variableRow(const QString& name) const
    {
        QTreeWidgetItem* pBase = rootRow();
        for (int i = 0; pBase && i < pBase->childCount(); ++i) {
            if (pBase->child(i)->text(0) == name) {
                return pBase->child(i);
            }
        }
        return nullptr;
    }

    // Both calls are deliberate: setCurrentItem() reaches slot_variableSelected() through
    // itemSelectionChanged, and the explicit call stands in for itemClicked on release
    void selectVariable(QTreeWidgetItem* pItem)
    {
        mpVariablesTree->setCurrentItem(pItem);
        mpEditor->slot_variableSelected(pItem);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
    }

    // Returns false rather than asserting: QVERIFY expands to a bare return,
    // which would leave the caller working with nothing selected
    bool showTheStringVariable()
    {
        openTheVariablesView();
        QTreeWidgetItem* pRow = variableRow(qsl("myString"));
        if (!pRow) {
            return false;
        }
        selectVariable(pRow);
        return true;
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
        mpVariablesTree = mpEditor->treeWidget_variables;
        QVERIFY2(mpVariablesTree != nullptr, "the editor has no variables tree");

        QVERIFY2(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("myString = \"north\"; myTable = { a = 1 }")), "the two variables this walks could not be made");
        mpEditor->repopulateVars();
        QTest::qWait(50ms);
        QVERIFY2(showTheStringVariable(), "the Variables view did not show the string variable this walks");
    }

    void cleanupTestCase()
    {
        mpVariablesTree = nullptr;
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_theTwoTypePickersAreOneRowUnderTheName()
    {
        QVERIFY2(showTheStringVariable(), "the string variable is not showing");

        QComboBox* pKeyType = form()->comboBox_variable_key_type;
        QComboBox* pValueType = form()->comboBox_variable_value_type;
        QVERIFY2(pKeyType->isVisible() && pValueType->isVisible(), "the variables form is not showing both of its type pickers");
        QCOMPARE(pKeyType->parentWidget()->objectName(), qsl("editorVariableTypes"));
        QCOMPARE(pValueType->parentWidget()->objectName(), qsl("editorVariableTypes"));
        QVERIFY2(topEdgeOf(pKeyType) == topEdgeOf(pValueType),
                 qPrintable(
                         qsl("the two type pickers are on different lines: the key's at %1 and the value's at %2").arg(QString::number(topEdgeOf(pKeyType)), QString::number(topEdgeOf(pValueType)))));

        QCOMPARE(form()->label_variable_key->text(), qsl("Key"));
        QCOMPARE(form()->label_variable_value->text(), qsl("Value"));
        for (const QLabel* pLabel : {form()->label_variable_key, form()->label_variable_value}) {
            QVERIFY2(!pLabel->text().contains(QChar(scmLeftPointingArrow)) && !pLabel->text().contains(QChar(scmDownPointingArrow)),
                     qPrintable(qsl("\"%1\" still points at its picker with an arrow").arg(pLabel->text())));
        }

        const int nameEdge = leftEdgeOf(form()->lineEdit_var_name);
        qInfo().noquote() << qsl("  the name is typed at %1, the key picker starts at %2 and the switch under it at %3")
                                     .arg(QString::number(nameEdge), QString::number(leftEdgeOf(pKeyType)), QString::number(leftEdgeOf(form()->checkBox_variable_hidden)));
        QVERIFY2(leftEdgeOf(pKeyType) == nameEdge,
                 qPrintable(qsl("the key picker starts at %1 while the name above it is typed at %2").arg(QString::number(leftEdgeOf(pKeyType)), QString::number(nameEdge))));
        QVERIFY2(leftEdgeOf(form()->checkBox_variable_hidden) == nameEdge,
                 qPrintable(qsl("the switch that hides the variable starts at %1 while the picker above it starts at %2")
                                    .arg(QString::number(leftEdgeOf(form()->checkBox_variable_hidden)), QString::number(nameEdge))));

        for (QComboBox* pBox : {pKeyType, pValueType}) {
            for (int i = 0; i < pBox->count(); ++i) {
                QVERIFY2(!pBox->itemText(i).contains(QChar('\n')), qPrintable(qsl("\"%1\" is offered over more than one line").arg(pBox->itemText(i).simplified())));
            }
        }
    }

    void test_theHiddenVariablesSwitchStartsWhereTheSearchFieldDoes()
    {
        openTheVariablesView();

        QCheckBox* pSwitch = mpEditor->checkBox_displayAllVariables;
        QVERIFY2(pSwitch->isVisible(), "the Variables view is not offering the switch that shows hidden variables");
        const int searchEdge = leftEdgeOf(mpEditor->comboBox_searchTerms);
        qInfo().noquote() << qsl("  the search field starts at %1 and the switch under it at %2").arg(QString::number(searchEdge), QString::number(leftEdgeOf(pSwitch)));
        QVERIFY2(leftEdgeOf(pSwitch) == searchEdge,
                 qPrintable(qsl("the switch starts at %1 while the search field above it starts at %2").arg(QString::number(leftEdgeOf(pSwitch)), QString::number(searchEdge))));
        QCOMPARE(pSwitch->text(), qsl("Show hidden variables"));
    }

    // A root row has no value under it, so selecting one hides the code pane
    void test_theCaretReadoutGoesWithTheCodePane()
    {
        // The pane's state on the way in is whatever the case before this left
        openTheVariablesView();
        selectVariable(rootRow());

        QVERIFY2(showTheStringVariable(), "the string variable is not showing");
        QVERIFY2(mpEditor->mpSourceEditorArea->isVisible(), "the code pane is not showing the value of a string variable");
        QVERIFY2(!caretReadoutSays().isEmpty(), "the code pane came back without the caret readout that belongs to it");

        selectVariable(rootRow());
        QVERIFY2(!mpEditor->mpSourceEditorArea->isVisible(), "the code pane is still showing on the variables root row");
        QVERIFY2(caretReadoutSays().isEmpty(), qPrintable(qsl("the heading still reads out \"%1\" for a code pane that is not on show").arg(caretReadoutSays())));

        QVERIFY2(showTheStringVariable(), "the string variable is not showing again");
        QVERIFY2(!caretReadoutSays().isEmpty(), "the caret readout did not come back with the code pane");
    }

    void test_theCodePaneHeadingNamesWhatIsUnderIt()
    {
        openTheVariablesView();

        QLabel* pTitle = mpEditor->findChild<QLabel*>(qsl("editorCodeHeaderTitle"));
        QVERIFY2(pTitle != nullptr, "the code pane carries no heading");
        QCOMPARE(pTitle->text(), qsl("Value"));

        mpEditor->slot_showTriggers();
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QCOMPARE(pTitle->text(), qsl("Lua script"));
    }
};

#include "EditorVariablesFormTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorVariablesFormTest)
