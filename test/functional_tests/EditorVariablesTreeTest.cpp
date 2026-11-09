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
 * Guards VariableTreeDelegate: the variables tree keeps the six item trees' row
 * grammar, and its kept square, type mark and value preview behave.
 *
 * Run with: ctest -R EditorVariablesTreeTest -V
 */

#include <QCheckBox>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QPalette>
#include <QPixmap>
#include <QStyleOptionViewItem>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>

#include "EditorTreeRowMetrics.h"
#include "Host.h"
#include "HostDialogs.h"
#include "LuaInterface.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "TLuaInterpreter.h"
#include "TTreeWidget.h"
#include "TVar.h"
#include "VarUnit.h"
#include "VariableTreeDelegate.h"
#include "dlgSourceEditorArea.h"
#include "dlgTriggerEditor.h"
#include "dlgVarsMainArea.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

namespace {
// indexFromItem() is protected and an off-screen row has no rect for indexAt();
// the member pointer reaches it without casting the tree to a type it is not
struct TreeIndexPeek : QTreeWidget
{
    static QModelIndex of(const QTreeWidget* pTree, QTreeWidgetItem* pItem)
    {
        QModelIndex (QTreeWidget::*pGetter)(const QTreeWidgetItem*, int) const = &TreeIndexPeek::indexFromItem;
        return (pTree->*pGetter)(pItem, 0);
    }
};
} // namespace

class EditorVariablesTreeTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    TTreeWidget* mpVariablesTree = nullptr;
    const QString mProfileName = qsl("EditorVariablesTree-Test-Profile");
    // Written out rather than read from the code under test, so the test fails
    // when that size moves
    static constexpr int scmMarkSize = 16;
    // Antialiasing blends a stroke's edge into the background; only its middle
    // lands on the colour itself
    static constexpr int scmInkTolerance = 24;

    uiDesign::VariableTreeDelegate* delegate() const { return mpEditor->mpVariableTreeDelegate; }

    QWidget* viewport() const { return mpVariablesTree->viewport(); }

    VarUnit* varUnit() const { return mpHost->getLuaInterface()->getVarUnit(); }

    QLabel* countLabel() const { return mpEditor->findChild<QLabel*>(qsl("editorHiddenVariablesCount")); }

    QTreeWidgetItem* headingRow() const { return mpVariablesTree->topLevelItem(0); }

    // Rebuilds the tree, so every row pointer a case holds is taken afresh
    void openTheVariablesView()
    {
        mpEditor->slot_showVariables();
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        mpVariablesTree->expandAll();
        QCoreApplication::processEvents();
    }

    QTreeWidgetItem* variableRow(const QString& name) const
    {
        QTreeWidgetItem* pBase = headingRow();
        for (int i = 0; pBase && i < pBase->childCount(); ++i) {
            if (pBase->child(i)->text(0) == name) {
                return pBase->child(i);
            }
        }
        return nullptr;
    }

    static QTreeWidgetItem* memberRow(QTreeWidgetItem* pParent, const QString& name)
    {
        for (int i = 0; pParent && i < pParent->childCount(); ++i) {
            if (pParent->child(i)->text(0) == name) {
                return pParent->child(i);
            }
        }
        return nullptr;
    }

    QModelIndex indexOf(QTreeWidgetItem* pItem) const { return TreeIndexPeek::of(mpVariablesTree, pItem); }

    // The row has to be on screen for its marks to have anywhere to be clicked
    QRect keptRectOf(QTreeWidgetItem* pItem) const
    {
        mpVariablesTree->scrollToItem(pItem);
        QCoreApplication::processEvents();
        const QModelIndex index = indexOf(pItem);
        return delegate()->dotHitRect(delegate()->rowOption(index), index);
    }

    void selectVariable(QTreeWidgetItem* pItem)
    {
        mpVariablesTree->setCurrentItem(pItem);
        mpEditor->slot_variableSelected(pItem);
        QCoreApplication::processEvents();
        QTest::qWait(20ms);
    }

    QString previewOf(QTreeWidgetItem* pItem) const { return pItem ? pItem->data(0, uiDesign::scmRole_variablePreview).toString() : QString(); }

    // Mirrors updateHiddenVariablesCount(), so a case knows the expected count
    // without reading the label it checks
    int hiddenGlobals() const
    {
        int hiddenCount = 0;
        if (TVar* pBase = varUnit()->getBase()) {
            const QList<TVar*> globals = pBase->getChildren(false);
            for (TVar* pGlobal : globals) {
                if (varUnit()->isHidden(pGlobal)) {
                    ++hiddenCount;
                }
            }
        }
        return hiddenCount;
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
        if (!QTest::qWaitForWindowExposed(mpEditor, 2000)) {
            qInfo().noquote() << qsl("  the editor window was never exposed; the readings below are off a rendered widget tree alone");
        }
        mpVariablesTree = mpEditor->treeWidget_variables;
        QVERIFY2(mpVariablesTree != nullptr, "the editor has no variables tree");

        QVERIFY2(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("myString = \"north\"\n"
                                                                     "myNumber = 42\n"
                                                                     "myBoolean = true\n"
                                                                     "myFunction = function() return 1 end\n"
                                                                     "myKeyed = { alpha = 1, beta = 2 }\n"
                                                                     "myList = { 10, 20, 30 }\n"
                                                                     "myNested = { kept = \"yes\" }\n")),
                 "the variables these cases read could not be made");

        // So the triggers tree has a row to measure a variable's row against
        mpEditor->slot_showTriggers();
        QTest::qWait(50ms);
        mpEditor->addTrigger(false);
        QCoreApplication::processEvents();

        openTheVariablesView();
        QStringList missing;
        for (const QString& name : {qsl("myString"), qsl("myNumber"), qsl("myBoolean"), qsl("myFunction"), qsl("myKeyed"), qsl("myList"), qsl("myNested")}) {
            if (!variableRow(name)) {
                missing << name;
            }
        }
        QVERIFY2(missing.isEmpty(), qPrintable(qsl("the Variables view is not showing: %1").arg(missing.join(qsl(", ")))));
        QVERIFY2(delegate() != nullptr, "the variables tree is drawn by no delegate of ours");
    }

    void cleanupTestCase()
    {
        mpVariablesTree = nullptr;
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_everyRowIsTheHeightAnItemTreesRowIs()
    {
        QVERIFY2(qobject_cast<uiDesign::VariableTreeDelegate*>(mpVariablesTree->itemDelegateForIndex(QModelIndex())) != nullptr,
                 "the variables tree is not drawn by VariableTreeDelegate, so its rows are whatever the platform makes of them");

        mpEditor->slot_showTriggers();
        QTest::qWait(50ms);
        const int triggerHeading = mpEditor->treeWidget_triggers->visualItemRect(mpEditor->mpTriggerBaseItem).height();
        QVERIFY2(triggerHeading > 0, "the triggers tree was not laid out, so there is nothing to measure against");

        openTheVariablesView();
        QStringList measured;
        QStringList uneven;
        const QList<QPair<QString, QTreeWidgetItem*>> rows{{qsl("heading"), headingRow()},
                                                           {qsl("string"), variableRow(qsl("myString"))},
                                                           {qsl("function"), variableRow(qsl("myFunction"))},
                                                           {qsl("table"), variableRow(qsl("myNested"))},
                                                           {qsl("member"), memberRow(variableRow(qsl("myNested")), qsl("kept"))}};
        for (const auto& row : rows) {
            QVERIFY2(row.second != nullptr, qPrintable(qsl("the %1 row this case measures is not in the tree").arg(row.first)));
            mpVariablesTree->scrollToItem(row.second);
            QCoreApplication::processEvents();
            const int height = mpVariablesTree->visualItemRect(row.second).height();
            measured << qsl("%1 %2").arg(row.first, QString::number(height));
            if (height != triggerHeading) {
                uneven << qsl("the %1 row is %2px where a triggers row is %3px").arg(row.first, QString::number(height), QString::number(triggerHeading));
            }
        }
        qInfo().noquote() << qsl("  a triggers row is %1px; variables rows: %2").arg(QString::number(triggerHeading), measured.join(qsl(", ")));

        QVERIFY2(uneven.isEmpty(), qPrintable(qsl("the variables tree is not on the item trees' row grammar: %1").arg(uneven.join(qsl("; ")))));
    }

    void test_clickingTheKeptSquareKeepsTheVariable()
    {
        openTheVariablesView();
        QTreeWidgetItem* pString = variableRow(qsl("myString"));
        QVERIFY(pString);

        const QRect square = keptRectOf(pString);
        QVERIFY2(!square.isEmpty(), "a variable's row reports no square to click");

        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, square.center());
        QCoreApplication::processEvents();
        QCOMPARE(pString->checkState(0), Qt::Checked);
        QVERIFY2(varUnit()->savedVars.contains(qsl("myString")), "clicking the square did not put the variable among the ones the profile keeps");

        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, keptRectOf(pString).center());
        QCoreApplication::processEvents();
        QCOMPARE(pString->checkState(0), Qt::Unchecked);
        QVERIFY2(!varUnit()->savedVars.contains(qsl("myString")), "clicking the square a second time did not stop the profile keeping the variable");

        const QRect row = mpVariablesTree->visualItemRect(pString);
        const QPoint onTheName(row.right() - 8, row.center().y());
        QVERIFY2(!keptRectOf(pString).contains(onTheName), "the point this case clicks has to be clear of the square");
        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, onTheName);
        QCoreApplication::processEvents();
        QCOMPARE(mpVariablesTree->currentItem(), pString);
        QCOMPARE(pString->checkState(0), Qt::Unchecked);
    }

    // Qt's tristate cascade ticks every member, and each tick is what enrols it
    void test_clickingATablesSquareKeepsItsMembers()
    {
        openTheVariablesView();
        QTreeWidgetItem* pTable = variableRow(qsl("myNested"));
        QVERIFY(pTable);
        QTreeWidgetItem* pMember = memberRow(pTable, qsl("kept"));
        QVERIFY2(pMember != nullptr, "the table this case keeps has no member in the tree");

        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, keptRectOf(pTable).center());
        QCoreApplication::processEvents();

        QCOMPARE(pTable->checkState(0), Qt::Checked);
        QCOMPARE(pMember->checkState(0), Qt::Checked);
        QVERIFY2(varUnit()->savedVars.contains(qsl("myNested")) && varUnit()->savedVars.contains(qsl("myNested.kept")),
                 qPrintable(qsl("keeping the table did not enrol it and its member; what is kept is: %1").arg(QStringList(varUnit()->savedVars.begin(), varUnit()->savedVars.end()).join(qsl(", ")))));

        // Put back, so the cases after this one start where this one did
        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, keptRectOf(pTable).center());
        QCoreApplication::processEvents();
        QCOMPARE(pTable->checkState(0), Qt::Unchecked);
    }

    // The profile cannot save a Lua function
    void test_aFunctionsRowHasNoSquareToClick()
    {
        openTheVariablesView();
        QTreeWidgetItem* pFunction = variableRow(qsl("myFunction"));
        QTreeWidgetItem* pString = variableRow(qsl("myString"));
        QVERIFY(pFunction && pString);

        QVERIFY2(delegate()->dotHitRect(delegate()->rowOption(indexOf(pFunction)), indexOf(pFunction)).isNull(), "a variable the profile cannot keep still reports a square to click");

        const QRect square = keptRectOf(pString);
        mpVariablesTree->scrollToItem(pFunction);
        QCoreApplication::processEvents();
        const QRect row = mpVariablesTree->visualItemRect(pFunction);
        const QPoint whereTheSquareWouldBe(square.center().x(), row.center().y());

        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, whereTheSquareWouldBe);
        QCoreApplication::processEvents();

        QCOMPARE(pFunction->checkState(0), Qt::Unchecked);
        QVERIFY2(!varUnit()->savedVars.contains(qsl("myFunction")), "a click where the square would be enrolled a variable the profile cannot keep");
    }

    void test_theTrailingEdgeSaysWhatTheValueIs()
    {
        openTheVariablesView();
        const QList<QPair<QString, QString>> expected{{qsl("myString"), qsl("\"north\"")},
                                                      {qsl("myNumber"), qsl("42")},
                                                      {qsl("myBoolean"), qsl("true")},
                                                      {qsl("myKeyed"), qsl("{ 2 keys }")},
                                                      {qsl("myList"), qsl("{ 3 items }")},
                                                      {qsl("myFunction"), qsl("function")}};
        QStringList wrong;
        QStringList measured;
        for (const auto& row : expected) {
            const QString preview = previewOf(variableRow(row.first));
            measured << qsl("%1 %2").arg(row.first, preview.isEmpty() ? qsl("(nothing)") : preview);
            if (preview != row.second) {
                wrong << qsl("%1 reads \"%2\" where it should read \"%3\"").arg(row.first, preview, row.second);
            }
        }
        qInfo().noquote() << qsl("  previews: %1").arg(measured.join(qsl(", ")));
        QVERIFY2(wrong.isEmpty(), qPrintable(qsl("a row does not say what its value is: %1").arg(wrong.join(qsl("; ")))));

        QTreeWidgetItem* pString = variableRow(qsl("myString"));
        selectVariable(pString);
        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("south"));
        mpEditor->saveVar();
        QCoreApplication::processEvents();
        QCOMPARE(previewOf(pString), qsl("\"south\""));

        // Put back, so the cases after this one read what the fixture wrote
        selectVariable(pString);
        mpEditor->mpSourceEditorEdbeeDocument->setText(qsl("north"));
        mpEditor->saveVar();
        QCoreApplication::processEvents();
        QCOMPARE(previewOf(pString), qsl("\"north\""));

        // The name is short and the preview sits at the trailing edge, so muted
        // ink in the last third of the row can only be the preview
        mpVariablesTree->clearSelection();
        mpVariablesTree->setCurrentItem(headingRow());
        mpVariablesTree->scrollToItem(pString);
        QCoreApplication::processEvents();
        const QRect row = mpVariablesTree->visualItemRect(pString);
        QVERIFY2(row.width() > 150, qPrintable(qsl("the panel is %1px wide, which is too narrow for this reading").arg(QString::number(row.width()))));
        const QImage shot = mpVariablesTree->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const QColor chrome = uiDesign::themeTokens().mutedText;
        int inked = 0;
        for (int y = row.top(); y <= row.bottom() && y < shot.height(); ++y) {
            for (int x = row.left() + (2 * row.width()) / 3; x <= row.right() && x < shot.width(); ++x) {
                const QColor pixel = shot.pixelColor(x, y);
                if (TestPixels::readsAs(pixel, chrome, scmInkTolerance)) {
                    ++inked;
                }
            }
        }
        qInfo().noquote() << qsl("  %1 pixels of the chrome tone were drawn in the last third of a %2px row").arg(QString::number(inked), QString::number(row.width()));
        QVERIFY2(inked > 0, "nothing is drawn at the trailing edge of the row, so the preview never reaches the panel");
    }

    // The row's own text stays the bare number: the editor reaches the variable by it
    void test_anIndexKeyIsDrawnAsAPlaceInAList()
    {
        openTheVariablesView();
        QTreeWidgetItem* pList = variableRow(qsl("myList"));
        QVERIFY(pList);
        QCOMPARE(pList->childCount(), 3);

        QStringList wrong;
        QStringList measured;
        for (int i = 0; i < pList->childCount(); ++i) {
            QTreeWidgetItem* pMember = pList->child(i);
            const QString drawn = delegate()->displayNameFor(indexOf(pMember));
            const QString wanted = qsl("[%1]").arg(pMember->text(0));
            measured << qsl("%1 as %2").arg(pMember->text(0), drawn);
            if (drawn != wanted) {
                wrong << qsl("member %1 is drawn \"%2\" where it should be drawn \"%3\"").arg(pMember->text(0), drawn, wanted);
            }
            if (!QRegularExpression(qsl("^[0-9]+$")).match(pMember->text(0)).hasMatch()) {
                wrong << qsl("the row's own text is \"%1\" rather than the bare number the editor reaches it by").arg(pMember->text(0));
            }
        }
        QTreeWidgetItem* pKeyed = variableRow(qsl("myKeyed"));
        QVERIFY(pKeyed && pKeyed->childCount() > 0);
        const QString namedMember = delegate()->displayNameFor(indexOf(pKeyed->child(0)));
        if (namedMember != pKeyed->child(0)->text(0)) {
            wrong << qsl("a member reached by name is drawn \"%1\" rather than \"%2\"").arg(namedMember, pKeyed->child(0)->text(0));
        }
        qInfo().noquote() << qsl("  index keys: %1; a named member is drawn %2").arg(measured.join(qsl(", ")), namedMember);

        QVERIFY2(wrong.isEmpty(), qPrintable(qsl("a key is not drawn for what it is: %1").arg(wrong.join(qsl("; ")))));
    }

    void test_eachRowCarriesTheMarkForItsKindOfValue()
    {
        openTheVariablesView();
        const QList<QPair<QString, QString>> expected{{qsl("myString"), qsl(":/icons/editor-type-string.svg")},
                                                      {qsl("myNumber"), qsl(":/icons/editor-type-number.svg")},
                                                      {qsl("myBoolean"), qsl(":/icons/editor-type-boolean.svg")},
                                                      {qsl("myFunction"), qsl(":/icons/editor-type-function.svg")},
                                                      {qsl("myKeyed"), qsl(":/icons/editor-variables.svg")},
                                                      {qsl("myList"), qsl(":/icons/editor-variables.svg")}};
        QStringList wrong;
        QStringList measured;
        for (const auto& row : expected) {
            const QString drawn = delegate()->typeGlyphFile(indexOf(variableRow(row.first)));
            measured << qsl("%1 %2").arg(row.first, drawn.isEmpty() ? qsl("(nothing)") : drawn.section(QChar('/'), -1));
            if (drawn != row.second) {
                wrong << qsl("%1 is marked with %2 where it should be marked with %3").arg(row.first, drawn, row.second);
            }
        }
        // A file left out of the resource list reads back as a null pixmap
        for (const QString& file : {qsl(":/icons/editor-type-string.svg"),
                                    qsl(":/icons/editor-type-number.svg"),
                                    qsl(":/icons/editor-type-boolean.svg"),
                                    qsl(":/icons/editor-type-function.svg"),
                                    qsl(":/icons/editor-type-other.svg"),
                                    qsl(":/icons/editor-hidden.svg")}) {
            if (QPixmap(file).isNull()) {
                wrong << qsl("%1 is not in the resources").arg(file);
            }
        }
        qInfo().noquote() << qsl("  type marks: %1").arg(measured.join(qsl(", ")));
        QVERIFY2(wrong.isEmpty(), qPrintable(qsl("a row is not marked for the kind of value it holds: %1").arg(wrong.join(qsl("; ")))));
    }

    void test_aHiddenVariableIsMarkedAndCounted()
    {
        openTheVariablesView();
        QTreeWidgetItem* pString = variableRow(qsl("myString"));
        QVERIFY(pString);
        const int before = hiddenGlobals();
        QLabel* pCount = countLabel();
        QVERIFY2(pCount != nullptr, "the switch that shows hidden variables carries no reading of how many there are");

        selectVariable(pString);
        QCheckBox* pHide = mpEditor->mpVarsMainArea->checkBox_variable_hidden;
        QVERIFY2(!pHide->isChecked(), "the variable this case hides is hidden already");
        pHide->click();
        QCoreApplication::processEvents();
        QCOMPARE(hiddenGlobals(), before + 1);
        qInfo().noquote() << qsl("  %1 globals were hidden before this case and the switch now reads \"%2\"").arg(QString::number(before), pCount->text());
        QCOMPARE(pCount->text(), qsl("%1 hidden").arg(before + 1));
        QVERIFY2(pCount->isVisible(), "the reading of how many variables are hidden is not being shown");
        QCOMPARE(pCount->palette().color(QPalette::WindowText).name(), uiDesign::themeTokens().mutedText.name());
        QVERIFY2(mpEditor->checkBox_displayAllVariables->accessibleName().contains(QString::number(before + 1)),
                 qPrintable(qsl("the switch is announced as \"%1\", which does not say how many are hidden").arg(mpEditor->checkBox_displayAllVariables->accessibleName())));

        mpEditor->checkBox_displayAllVariables->setChecked(true);
        QCoreApplication::processEvents();
        mpVariablesTree->expandAll();
        QTreeWidgetItem* pShown = variableRow(qsl("myString"));
        QVERIFY2(pShown != nullptr, "a hidden variable is not listed even with the switch on");
        QVERIFY2(delegate()->carriesHiddenMark(indexOf(pShown)), "a hidden variable's row carries no mark saying it is hidden");
        QVERIFY2(!delegate()->carriesHiddenMark(indexOf(variableRow(qsl("myNumber")))), "a variable that is not hidden carries the mark that says it is");

        mpEditor->checkBox_displayAllVariables->setChecked(false);
        QCoreApplication::processEvents();
        mpVariablesTree->expandAll();
        QVERIFY2(variableRow(qsl("myString")) == nullptr, "a hidden variable is still listed with the switch off");

        // Put back, since the cases after this one read the same variable
        varUnit()->removeHidden(qsl("myString"));
        openTheVariablesView();
        QVERIFY2(variableRow(qsl("myString")) != nullptr, "the variable this case hid was not put back");
        QCOMPARE(hiddenGlobals(), before);
        QVERIFY2(pCount->text() == (before > 0 ? qsl("%1 hidden").arg(before) : QString()),
                 qPrintable(qsl("the reading beside the switch says \"%1\" with %2 hidden").arg(pCount->text(), QString::number(before))));
    }

    void test_aVariableThatCannotBeKeptIsDrawnAsUnavailable()
    {
        openTheVariablesView();
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor functionInk = delegate()->rowInk(indexOf(variableRow(qsl("myFunction"))), false);
        const QColor stringInk = delegate()->rowInk(indexOf(variableRow(qsl("myString"))), false);
        qInfo().noquote() << qsl("  a function's row is inked %1 and a string's %2; the quiet tone is %3 and the unavailable one %4")
                                     .arg(functionInk.name(), stringInk.name(), tokens.mutedText.name(), tokens.disabledText.name());

        QCOMPARE(functionInk.name(), tokens.disabledText.name());
        QCOMPARE(stringInk.name(), tokens.mutedText.name());

        // ReadabilityAuditTest reads widget palettes and cannot see an ink a
        // delegate paints with, so this tone's contrast floor is checked here
        const qreal ratio = uiDesign::contrastRatio(tokens.disabledText, tokens.pane);
        qInfo().noquote() << qsl("  the unavailable tone clears %1:1 on the panel").arg(QString::number(ratio, 'f', 2));
        QVERIFY2(ratio >= uiDesign::scmQuietMinimumRatio,
                 qPrintable(qsl("a variable the profile cannot keep is drawn in %1, which is %2:1 on the panel it is drawn on - under the %3:1 an unavailable word is held to")
                                    .arg(tokens.disabledText.name(), QString::number(ratio, 'f', 2), QString::number(uiDesign::scmQuietMinimumRatio, 'f', 1))));
    }

    void test_theHeadingRowCarriesTheBracesAndNoSquare()
    {
        openTheVariablesView();
        QTreeWidgetItem* pHeading = headingRow();
        QVERIFY(pHeading);
        QCOMPARE(pHeading->text(0), qsl("Variables"));

        QVERIFY2(delegate()->dotHitRect(delegate()->rowOption(indexOf(pHeading)), indexOf(pHeading)).isNull(), "the tree's own heading row reports a square the profile could keep it by");
        QVERIFY2(delegate()->typeGlyphFile(indexOf(pHeading)).isEmpty(), "the tree's own heading row is marked as though it held a Lua value");

        // Read from initStyleOption() rather than the item: the heading is the
        // one row whose own picture the delegate hands the style
        mpVariablesTree->scrollToItem(pHeading);
        QCoreApplication::processEvents();
        QStyleOptionViewItem option = mpVariablesTree->viewItemOption();
        option.rect = mpVariablesTree->visualItemRect(pHeading);
        delegate()->initStyleOption(&option, indexOf(pHeading));
        QCOMPARE(option.decorationSize, QSize(scmMarkSize, scmMarkSize));

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QImage onTheHeading = TestPixels::drawnAt(option.icon, scmMarkSize);
        const QImage wanted = TestPixels::drawnAt(QIcon(uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/editor-variables.svg")), tokens.mutedText)), scmMarkSize);
        QVERIFY2(!onTheHeading.isNull(), "the tree's heading row carries no picture at all");
        QVERIFY2(onTheHeading == wanted, "the tree's heading row does not carry the braces its sidebar row carries, inked in the tone the editor's chrome is");
    }
};

#include "EditorVariablesTreeTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorVariablesTreeTest)
