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
 * grammar, and its kept check box, type mark and value preview behave. Also the
 * two switches over the tree, the one for functions among them.
 *
 * Run with: ctest -R EditorVariablesTreeTest -V
 */

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QScopeGuard>
#include <QStyleOptionButton>
#include <QStyleOptionViewItem>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <algorithm>

#include "EditorTreeRowMetrics.h"
#include "Host.h"
#include "HostDialogs.h"
#include "LuaInterface.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "SearchResultDelegate.h"
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
    // Per channel: an antialiased edge drawn onto nothing and then laid over the row rounds a step or two
    // away from the same edge drawn straight onto the row
    static constexpr int scmBoxMatchTolerance = 6;
    // Tick, dash and empty box have to differ by far more than that rounding
    static constexpr int scmBoxStatesApart = 40;
    // Between the wrapped lines of an option row on the editor's forms and the stacked check boxes on a settings card;
    // written out for the same reason as the mark's size
    static constexpr int scmStackedChoiceGap = 8;

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
    }

    QString previewOf(QTreeWidgetItem* pItem) const { return pItem ? pItem->data(0, uiDesign::scmRole_variablePreview).toString() : QString(); }

    QRect switchBoxInWindow(QCheckBox* pSwitch) const
    {
        pSwitch->ensurePolished();
        QStyleOptionButton option;
        option.initFrom(pSwitch);
        const QRect indicator = pSwitch->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, pSwitch);
        return QRect(pSwitch->mapTo(mpEditor, indicator.topLeft()), indicator.size());
    }

    static QRect inDevicePixels(const QRect& logical, const QImage& shot)
    {
        const qreal ratio = shot.devicePixelRatio();
        return QRect(QPoint(qRound(logical.left() * ratio), qRound(logical.top() * ratio)), QPoint(qRound((logical.right() + 1) * ratio) - 1, qRound((logical.bottom() + 1) * ratio) - 1))
                .intersected(shot.rect());
    }

    // A real check box on a form, choiceStyleSheet() and all, rendered onto the surface the mark under test sits
    // on and cut down to its indicator
    static QImage formCheckBoxIndicator(const Qt::CheckState state, const QColor& surface, const qreal ratio, const bool onAChosenRow = false)
    {
        QCheckBox box;
        box.setStyleSheet(uiDesign::choiceStyleSheet(uiDesign::themeTokens()));
        box.setTristate(true);
        box.setCheckState(state);
        box.ensurePolished();
        box.resize(box.sizeHint());
        QStyleOptionButton option;
        option.initFrom(&box);
        const QRect indicator = box.style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, &box);

        QImage rendered(box.size() * ratio, QImage::Format_ARGB32_Premultiplied);
        rendered.setDevicePixelRatio(ratio);
        rendered.fill(onAChosenRow ? QColor(Qt::transparent) : surface);
        box.render(&rendered, QPoint(), QRegion(), QWidget::DrawChildren);
        QImage cut = rendered.copy(inDevicePixels(indicator, rendered));
        cut.setDevicePixelRatio(ratio);
        if (!onAChosenRow) {
            return cut;
        }

        // A row's decoration carries no Selected picture of its own, so a chosen row's every mark is drawn
        // through the tint QIcon has the application style generate for one
        QStyleOption tintOption;
        tintOption.palette = QGuiApplication::palette();
        const QPixmap tinted = QApplication::style()->generatedIconPixmap(QIcon::Selected, QPixmap::fromImage(cut), &tintOption);
        QImage onTheRow(cut.size(), QImage::Format_ARGB32);
        onTheRow.setDevicePixelRatio(ratio);
        onTheRow.fill(surface);
        QPainter painter(&onTheRow);
        painter.drawImage(QPoint(0, 0), tinted.toImage());
        painter.end();
        return onTheRow;
    }

    // The largest difference in any channel at whichever place inside searched, in the grab's pixels, the
    // two agree best: the slack round a mark covers where a half pixel of centring rounds to
    static int worstDifferenceAtTheBestFit(const QImage& shot, const QRect& searched, const QImage& wanted)
    {
        int best = 255;
        for (int top = searched.top(); top + wanted.height() <= searched.bottom() + 1; ++top) {
            for (int left = searched.left(); left + wanted.width() <= searched.right() + 1; ++left) {
                int worst = 0;
                for (int y = 0; y < wanted.height() && worst < best; ++y) {
                    for (int x = 0; x < wanted.width() && worst < best; ++x) {
                        const QColor read = shot.pixelColor(left + x, top + y);
                        const QColor drawn = wanted.pixelColor(x, y);
                        worst = std::max({worst, std::abs(read.red() - drawn.red()), std::abs(read.green() - drawn.green()), std::abs(read.blue() - drawn.blue())});
                    }
                }
                best = std::min(best, worst);
            }
        }
        return best;
    }

    // Guards the comparison itself: a reference the platform drew would match a tree the platform drew
    static QStringList notTheDesignsBox(const QImage& empty, const QImage& ticked, const QColor& surface)
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QStringList misses;
        const QColor centre = empty.pixelColor(empty.width() / 2, empty.height() / 2);
        if (!TestPixels::readsAs(centre, tokens.field, 2)) {
            misses << qsl("the reference box is filled %1 rather than the field tone %2").arg(centre.name(), tokens.field.name());
        }
        const QColor tickedEdge = ticked.pixelColor(ticked.width() / 2, 0);
        if (!TestPixels::readsAs(tickedEdge, tokens.accent, 8)) {
            misses << qsl("a ticked reference box is outlined %1 rather than in the accent %2").arg(tickedEdge.name(), tokens.accent.name());
        }
        const QColor corner = empty.pixelColor(0, 0);
        const QColor edge = empty.pixelColor(empty.width() / 2, 0);
        if (TestPixels::distance(corner, surface) >= TestPixels::distance(edge, surface)) {
            misses << qsl("the reference box's corner %1 is drawn as fully as its edge %2, so it is not rounded").arg(corner.name(), edge.name());
        }
        return misses;
    }

    // Every row under the heading by its dotted path, and the ones standing for a function among them
    static void collectRows(QTreeWidgetItem* pParent, const QString& prefix, QStringList& paths, QStringList& functions)
    {
        for (int i = 0; pParent && i < pParent->childCount(); ++i) {
            QTreeWidgetItem* pRow = pParent->child(i);
            const QString path = prefix.isEmpty() ? pRow->text(0) : qsl("%1.%2").arg(prefix, pRow->text(0));
            paths << path;
            if (pRow->data(0, Qt::UserRole).toInt() == LUA_TFUNCTION) {
                functions << path;
            }
            collectRows(pRow, path, paths, functions);
        }
    }

    // The heading of each item the search found, which for a variable is the expression that reaches it
    QStringList searchResultTitles(const QString& term)
    {
        QComboBox* pTerms = mpEditor->comboBox_searchTerms;
        pTerms->clear();
        pTerms->addItem(term);
        mpEditor->slot_searchMudletItems(0);
        QCoreApplication::processEvents();
        return currentSearchResultTitles();
    }

    QStringList currentSearchResultTitles() const
    {
        QStringList titles;
        const QTreeWidget* pResults = mpEditor->treeWidget_searchResults;
        for (int i = 0; i < pResults->topLevelItemCount(); ++i) {
            titles << pResults->topLevelItem(i)->data(0, uiDesign::SearchResultDelegate::TitleRole).toString();
        }
        return titles;
    }

    static QStringList mentioning(const QStringList& titles, const QString& name)
    {
        QStringList found;
        for (const QString& title : titles) {
            if (title.contains(name)) {
                found << title;
            }
        }
        return found;
    }

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
        QCoreApplication::processEvents();
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
                                                                     "myNested = { kept = \"yes\" }\n"
                                                                     "myParagraph = \"first line\\nsecond line\"\n"
                                                                     "myLeadingBreak = \"\\nafter a break\\n\"\n")),
                 "the variables these cases read could not be made");

        // So the triggers tree has a row to measure a variable's row against
        mpEditor->slot_showTriggers();
        QCoreApplication::processEvents();
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

    // First of the cases: a change of appearance restyles both switches, which would cover for one the editor opened
    // without its rules
    void test_theFunctionsSwitchOpensWithTheHiddenSwitchsRules()
    {
        QCheckBox* pHiddenSwitch = mpEditor->checkBox_displayAllVariables;
        QCheckBox* pFunctionsSwitch = mpEditor->mpCheckBox_showFunctionVariables;
        QVERIFY2(pFunctionsSwitch != nullptr, "the Variables view has no switch for functions");
        QVERIFY2(!pHiddenSwitch->styleSheet().isEmpty(), "the switch for hidden variables opens with no rules of its own, so this case says nothing");
        QVERIFY2(pFunctionsSwitch->styleSheet() == pHiddenSwitch->styleSheet(), "the switch for functions opens without the check box rules the switch above it carries");
        QCOMPARE(pFunctionsSwitch->focusPolicy(), pHiddenSwitch->focusPolicy());
    }

    void test_everyRowIsTheHeightAnItemTreesRowIs()
    {
        QVERIFY2(qobject_cast<uiDesign::VariableTreeDelegate*>(mpVariablesTree->itemDelegateForIndex(QModelIndex())) != nullptr,
                 "the variables tree is not drawn by VariableTreeDelegate, so its rows are whatever the platform makes of them");

        mpEditor->slot_showTriggers();
        QCoreApplication::processEvents();
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

    void test_clickingTheKeptBoxKeepsTheVariable()
    {
        openTheVariablesView();
        QTreeWidgetItem* pString = variableRow(qsl("myString"));
        QVERIFY(pString);

        const QRect box = keptRectOf(pString);
        QVERIFY2(!box.isEmpty(), "a variable's row reports no box to click");

        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, box.center());
        QCoreApplication::processEvents();
        QCOMPARE(pString->checkState(0), Qt::Checked);
        QVERIFY2(varUnit()->savedVars.contains(qsl("myString")), "clicking the box did not put the variable among the ones the profile keeps");

        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, keptRectOf(pString).center());
        QCoreApplication::processEvents();
        QCOMPARE(pString->checkState(0), Qt::Unchecked);
        QVERIFY2(!varUnit()->savedVars.contains(qsl("myString")), "clicking the box a second time did not stop the profile keeping the variable");

        const QRect row = mpVariablesTree->visualItemRect(pString);
        const QPoint onTheName(row.right() - 8, row.center().y());
        QVERIFY2(!keptRectOf(pString).contains(onTheName), "the point this case clicks has to be clear of the box");
        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, onTheName);
        QCoreApplication::processEvents();
        QCOMPARE(mpVariablesTree->currentItem(), pString);
        QCOMPARE(pString->checkState(0), Qt::Unchecked);
    }

    // Qt's tristate cascade ticks every member, and each tick is what enrols it
    void test_clickingATablesBoxKeepsItsMembers()
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
    void test_aFunctionsRowHasNoBoxToClick()
    {
        openTheVariablesView();
        QTreeWidgetItem* pFunction = variableRow(qsl("myFunction"));
        QTreeWidgetItem* pString = variableRow(qsl("myString"));
        QVERIFY(pFunction && pString);

        QVERIFY2(delegate()->dotHitRect(delegate()->rowOption(indexOf(pFunction)), indexOf(pFunction)).isNull(), "a variable the profile cannot keep still reports a box to click");

        const QRect box = keptRectOf(pString);
        mpVariablesTree->scrollToItem(pFunction);
        QCoreApplication::processEvents();
        const QRect row = mpVariablesTree->visualItemRect(pFunction);
        const QPoint whereTheBoxWouldBe(box.center().x(), row.center().y());

        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, whereTheBoxWouldBe);
        QCoreApplication::processEvents();

        QCOMPARE(pFunction->checkState(0), Qt::Unchecked);
        QVERIFY2(!varUnit()->savedVars.contains(qsl("myFunction")), "a click where the box would be enrolled a variable the profile cannot keep");
    }

    // A dot or a square in the running dot's blue read as a switch left behind in a tree that has none
    void test_eachKeptStateIsTheFormsCheckBox()
    {
        openTheVariablesView();
        QTreeWidgetItem* pString = variableRow(qsl("myString"));
        QTreeWidgetItem* pNumber = variableRow(qsl("myNumber"));
        QTreeWidgetItem* pKeyed = variableRow(qsl("myKeyed"));
        QTreeWidgetItem* pAlpha = memberRow(pKeyed, qsl("alpha"));
        QVERIFY(pString && pNumber && pKeyed && pAlpha);

        const auto appearanceBefore = mudlet::self()->mAppearance;
        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, keptRectOf(pString).center());
        QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, keptRectOf(pAlpha).center());
        QCoreApplication::processEvents();
        // Put back, so the cases after this one start where this one did
        const auto restore = qScopeGuard([this, appearanceBefore, pString, pAlpha]() {
            mudlet::self()->setAppearance(appearanceBefore);
            for (QTreeWidgetItem* pRow : {pString, pAlpha}) {
                if (pRow->checkState(0) == Qt::Checked) {
                    QTest::mouseClick(viewport(), Qt::LeftButton, Qt::NoModifier, keptRectOf(pRow).center());
                }
            }
            QCoreApplication::processEvents();
        });
        QCOMPARE(pString->checkState(0), Qt::Checked);
        QCOMPARE(pNumber->checkState(0), Qt::Unchecked);
        QCOMPARE(pKeyed->checkState(0), Qt::PartiallyChecked);

        struct Mark
        {
            QString name;
            QTreeWidgetItem* pRow = nullptr;
            Qt::CheckState state = Qt::Unchecked;
            bool chosen = false;
        };
        // The chosen row last, since choosing it moves the selection's wash onto it
        const QList<Mark> marks{{qsl("kept"), pString, Qt::Checked, false},
                                {qsl("part kept"), pKeyed, Qt::PartiallyChecked, false},
                                {qsl("not kept"), pNumber, Qt::Unchecked, false},
                                {qsl("kept on the chosen row"), pString, Qt::Checked, true}};

        QStringList misses;
        QStringList measured;
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            TestPixels::setAppearanceAndWait(appearance.second, mpEditor);
            // Otherwise a run that never switched would measure one appearance twice and pass
            QCOMPARE(uiDesign::themeTokens().darkPage, appearance.second == enums::Appearance::dark);
            mpVariablesTree->setCurrentItem(headingRow());
            QCoreApplication::processEvents();

            for (const Mark& mark : marks) {
                if (mark.chosen) {
                    mpVariablesTree->setCurrentItem(mark.pRow);
                    QCoreApplication::processEvents();
                }
                const QRect hit = keptRectOf(mark.pRow);
                QVERIFY2(!hit.isEmpty(), qPrintable(qsl("%1: the %2 row reports no box").arg(appearance.first, mark.name)));
                const QImage shot = TestPixels::shot(viewport());
                // Just before the box is the row's own surface: a table's chevron stands a step further out
                const QColor surface = TestPixels::sampled(shot, QPoint(hit.left(), hit.center().y()));
                const QImage wanted = formCheckBoxIndicator(mark.state, surface, shot.devicePixelRatio(), mark.chosen);
                const int worst = worstDifferenceAtTheBestFit(shot, inDevicePixels(hit, shot), wanted);
                measured << qsl("%1 %2 on %3: %4 apart at the best fit").arg(appearance.first, mark.name, surface.name(), QString::number(worst));
                if (worst > scmBoxMatchTolerance) {
                    misses << qsl("%1: the %2 row's mark is %3 apart in some channel from a form's %4px check box in that state")
                                      .arg(appearance.first, mark.name, QString::number(worst), QString::number(qRound(wanted.width() / wanted.devicePixelRatio())));
                }
            }

            const QColor pane = uiDesign::themeTokens().pane;
            const QImage empty = formCheckBoxIndicator(Qt::Unchecked, pane, 1.0);
            const QImage dashed = formCheckBoxIndicator(Qt::PartiallyChecked, pane, 1.0);
            const QImage ticked = formCheckBoxIndicator(Qt::Checked, pane, 1.0);
            for (const QString& miss : notTheDesignsBox(empty, ticked, pane)) {
                misses << qsl("%1: %2").arg(appearance.first, miss);
            }
            const int emptyFromDashed = worstDifferenceAtTheBestFit(empty, empty.rect(), dashed);
            const int dashedFromTicked = worstDifferenceAtTheBestFit(dashed, dashed.rect(), ticked);
            const int emptyFromTicked = worstDifferenceAtTheBestFit(empty, empty.rect(), ticked);
            if (std::min({emptyFromDashed, dashedFromTicked, emptyFromTicked}) <= scmBoxStatesApart) {
                misses << qsl("%1: the three states are not told apart: empty and dash %2 apart, dash and tick %3, empty and tick %4")
                                  .arg(appearance.first, QString::number(emptyFromDashed), QString::number(dashedFromTicked), QString::number(emptyFromTicked));
            }
        }
        qInfo().noquote() << qsl("  %1").arg(measured.join(qsl("\n  ")));
        QVERIFY2(misses.isEmpty(), qPrintable(qsl("\n  %1").arg(misses.join(qsl("\n  ")))));
    }

    // Platform drawn, the switch's box was square and unlike every other box in the window
    void test_bothSwitchesCarryTheFormsCheckBox()
    {
        openTheVariablesView();
        QCheckBox* pHiddenSwitch = mpEditor->checkBox_displayAllVariables;
        QCheckBox* pFunctionsSwitch = mpEditor->mpCheckBox_showFunctionVariables;
        QCheckBox* pFormBox = mpEditor->mpVarsMainArea->checkBox_variable_hidden;
        QVERIFY2(pHiddenSwitch->isVisible(), "the Variables view is not offering the switch that shows hidden variables");
        QVERIFY2(pFunctionsSwitch && pFunctionsSwitch->isVisible(), "the Variables view is not offering the switch that shows functions");
        const QList<QPair<QString, QCheckBox*>> switches{{qsl("hidden variables"), pHiddenSwitch}, {qsl("functions"), pFunctionsSwitch}};
        QCOMPARE(pFunctionsSwitch->mapTo(mpEditor, QPoint()).x(), pHiddenSwitch->mapTo(mpEditor, QPoint()).x());
        QVERIFY2(pFunctionsSwitch->mapTo(mpEditor, QPoint()).y() > pHiddenSwitch->mapTo(mpEditor, QPoint()).y(), "the switch for functions is not on a row under the one for hidden variables");
        QCOMPARE(pFunctionsSwitch->focusPolicy(), pHiddenSwitch->focusPolicy());
        const auto appearanceBefore = mudlet::self()->mAppearance;
        const auto restore = qScopeGuard([appearanceBefore]() {
            mudlet::self()->setAppearance(appearanceBefore);
            QCoreApplication::processEvents();
        });

        const auto indicatorAndGap = [](QCheckBox* pBox) {
            pBox->ensurePolished();
            QStyleOptionButton option;
            option.initFrom(pBox);
            const QRect indicator = pBox->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, pBox);
            const QRect contents = pBox->style()->subElementRect(QStyle::SE_CheckBoxContents, &option, pBox);
            return qMakePair(indicator, contents.left() - indicator.right() - 1);
        };

        QStringList misses;
        QStringList measured;
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            TestPixels::setAppearanceAndWait(appearance.second, mpEditor);
            QCOMPARE(uiDesign::themeTokens().darkPage, appearance.second == enums::Appearance::dark);

            for (const auto& [name, pSwitch] : switches) {
                const QString which = qsl("%1, the %2 switch").arg(appearance.first, name);
                const auto [indicator, gap] = indicatorAndGap(pSwitch);
                const auto [formIndicator, formGap] = indicatorAndGap(pFormBox);
                const QImage shot = TestPixels::shot(mpEditor);
                const QRect inWindow(pSwitch->mapTo(mpEditor, indicator.topLeft()), indicator.size());
                const QColor surface = TestPixels::sampled(shot, QPoint(inWindow.left() - 2, inWindow.center().y()));
                const QImage wanted = formCheckBoxIndicator(pSwitch->checkState(), surface, shot.devicePixelRatio());
                const int slack = uiDesign::scmTreeMarkHitSlack;
                const int worst = worstDifferenceAtTheBestFit(shot, inDevicePixels(inWindow.adjusted(-slack, -slack, slack, slack), shot), wanted);
                const QColor ink = pSwitch->palette().color(QPalette::Active, pSwitch->foregroundRole());
                const QColor formInk = pFormBox->palette().color(QPalette::Active, pFormBox->foregroundRole());
                measured << qsl("%1: a %2x%3 box on %4, %5 apart from a form's at the best fit, its words %6px after it in %7; a form's %8px after it in %9")
                                    .arg(which, QString::number(indicator.width()), QString::number(indicator.height()), surface.name(), QString::number(worst))
                                    .arg(QString::number(gap), ink.name(), QString::number(formGap), formInk.name());

                if (indicator.size() != formIndicator.size()) {
                    misses << qsl("%1: the switch's box is %2x%3 where a form's is %4x%5")
                                      .arg(which)
                                      .arg(indicator.width())
                                      .arg(indicator.height())
                                      .arg(formIndicator.width())
                                      .arg(formIndicator.height());
                }
                if (worst > scmBoxMatchTolerance) {
                    misses << qsl("%1: the switch's box is %2 apart in some channel from a form's check box").arg(which, QString::number(worst));
                }
                if (gap != formGap) {
                    misses << qsl("%1: the switch's words stand %2px after its box where a form's stand %3px after theirs").arg(which, QString::number(gap), QString::number(formGap));
                }
                if (ink != formInk) {
                    misses << qsl("%1: the switch's words are %2 where a form's check box's are %3").arg(which, ink.name(), formInk.name());
                }
            }
        }
        qInfo().noquote() << qsl("  %1").arg(measured.join(qsl("\n  ")));
        QVERIFY2(misses.isEmpty(), qPrintable(qsl("\n  %1").arg(misses.join(qsl("\n  ")))));
    }

    // Touching, the two boxes read as one control; and nearer the search field than the tree, or the other way round,
    // the pair reads as belonging to one of them
    void test_theSwitchesStandApartAsOneBlock()
    {
        openTheVariablesView();
        QCheckBox* pHiddenSwitch = mpEditor->checkBox_displayAllVariables;
        QCheckBox* pFunctionsSwitch = mpEditor->mpCheckBox_showFunctionVariables;
        QVERIFY2(pHiddenSwitch->isVisible() && pFunctionsSwitch && pFunctionsSwitch->isVisible(), "the Variables view is not offering both switches");

        QWidget* pField = mpEditor->comboBox_searchTerms;
        const int fieldBottom = pField->mapTo(mpEditor, QPoint(0, pField->height() - 1)).y();
        const QRect upper = switchBoxInWindow(pHiddenSwitch);
        const QRect lower = switchBoxInWindow(pFunctionsSwitch);
        // The tree draws no frame, so the first thing on show in it is its first row, at the top of the viewport
        const int treeTop = viewport()->mapTo(mpEditor, QPoint()).y();
        const int aboveTheBlock = upper.top() - fieldBottom - 1;
        const int inTheBlock = lower.top() - upper.bottom() - 1;
        const int underTheBlock = treeTop - lower.bottom() - 1;
        qInfo().noquote() << qsl("  search field to the first box %1px, box to box %2px, second box to the tree %3px")
                                     .arg(QString::number(aboveTheBlock), QString::number(inTheBlock), QString::number(underTheBlock));

        QVERIFY2(inTheBlock >= scmStackedChoiceGap,
                 qPrintable(qsl("the two switches' boxes stand %1px apart where stacked check boxes stand at least %2px").arg(QString::number(inTheBlock), QString::number(scmStackedChoiceGap))));
        // A pixel either way: a box is centred in a switch that its words can make taller than the box
        QVERIFY2(std::abs(aboveTheBlock - underTheBlock) <= 1,
                 qPrintable(qsl("the switches stand %1px under the search field but %2px over the tree").arg(QString::number(aboveTheBlock), QString::number(underTheBlock))));
        QVERIFY2(std::min(aboveTheBlock, underTheBlock) > inTheBlock,
                 qPrintable(qsl("the switches stand %1px from each other and no further from what is round them, so they do not read as one block").arg(QString::number(inTheBlock))));

        // The gaps belong to the switches: hidden, they must not push the other views' trees down
        const auto backToVariables = qScopeGuard([this]() {
            openTheVariablesView();
        });
        mpEditor->slot_showTriggers();
        QCoreApplication::processEvents();
        QVERIFY2(!pHiddenSwitch->isVisible() && !pFunctionsSwitch->isVisible(), "the switches are still showing outside the Variables view");
        QVERIFY2(mpEditor->mpLayout_variableSwitches->geometry().height() == 0,
                 qPrintable(qsl("outside the Variables view the hidden switches still take %1px over the tree").arg(QString::number(mpEditor->mpLayout_variableSwitches->geometry().height()))));
    }

    void test_theTrailingEdgeSaysWhatTheValueIs()
    {
        openTheVariablesView();
        const QList<QPair<QString, QString>> expected{{qsl("myString"), qsl("\"north\"")},
                                                      {qsl("myNumber"), qsl("42")},
                                                      {qsl("myBoolean"), qsl("true")},
                                                      {qsl("myKeyed"), qsl("{ 2 keys }")},
                                                      {qsl("myList"), qsl("{ 3 items }")},
                                                      {qsl("myFunction"), qsl("function")},
                                                      {qsl("myParagraph"), qsl("\"first line\u2026\"")},
                                                      {qsl("myLeadingBreak"), qsl("\"after a break\"")}};
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

    // Grayscale icons, as in colour the braces take the old picture's colour rather than the chrome's tone
    void test_theHeadingRowCarriesTheBracesAndNoBox()
    {
        mudlet::self()->setColouredIcons(false);
        const auto restore = qScopeGuard([]() {
            mudlet::self()->setColouredIcons(true);
        });
        openTheVariablesView();
        QTreeWidgetItem* pHeading = headingRow();
        QVERIFY(pHeading);
        QCOMPARE(pHeading->text(0), qsl("Variables"));

        QVERIFY2(delegate()->dotHitRect(delegate()->rowOption(indexOf(pHeading)), indexOf(pHeading)).isNull(), "the tree's own heading row reports a box the profile could keep it by");
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

    // A function as a global and as a table's member, since a walk of the globals alone would pass a filter that
    // skips only globals
    void test_theFunctionsSwitchLeavesFunctionsOutAtEveryDepth()
    {
        QVERIFY2(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("qaToolbox = { qaToolboxRun = function() return 2 end, qaToolboxLabel = \"tools\" }\n"
                                                                     "qaToolboxCall = function() return 3 end\n")),
                 "the variables this case reads could not be made");
        QCheckBox* pSwitch = mpEditor->mpCheckBox_showFunctionVariables;
        QVERIFY2(pSwitch != nullptr, "the Variables view has no switch for functions");
        const auto restore = qScopeGuard([this, pSwitch]() {
            pSwitch->setChecked(true);
            mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("qaToolbox = nil\nqaToolboxCall = nil\n"));
            openTheVariablesView();
        });
        openTheVariablesView();
        QVERIFY2(pSwitch->isVisible(), "the Variables view is not offering the switch for functions");
        QVERIFY2(pSwitch->isChecked(), "functions are left out of the Variables view before anybody asked for that");

        QStringList everyRow;
        QStringList functionRows;
        collectRows(headingRow(), QString(), everyRow, functionRows);
        for (const QString& function : {qsl("myFunction"), qsl("qaToolboxCall"), qsl("qaToolbox.qaToolboxRun")}) {
            QVERIFY2(functionRows.contains(function), qPrintable(qsl("%1 is not listed as a function with the switch on, so this case says nothing about it").arg(function)));
        }
        const QSet<QString> keptBefore = varUnit()->savedVars;

        pSwitch->setChecked(false);
        QCoreApplication::processEvents();
        QStringList rowsWithout;
        QStringList functionsWithout;
        collectRows(headingRow(), QString(), rowsWithout, functionsWithout);
        qInfo().noquote() << qsl("  %1 rows with functions shown, %2 of them functions; %3 rows without")
                                     .arg(QString::number(everyRow.size()), QString::number(functionRows.size()), QString::number(rowsWithout.size()));
        QVERIFY2(functionsWithout.isEmpty(), qPrintable(qsl("with the switch off these functions are still listed: %1").arg(functionsWithout.join(qsl(", ")))));
        QStringList expected = everyRow;
        for (const QString& function : std::as_const(functionRows)) {
            expected.removeAll(function);
        }
        QCOMPARE(rowsWithout, expected);
        QVERIFY2(varUnit()->savedVars == keptBefore, "leaving the functions out changed what the profile keeps");
        QCOMPARE(mpVariablesTree->currentItem(), headingRow());

        pSwitch->setChecked(true);
        QCoreApplication::processEvents();
        QStringList rowsAgain;
        QStringList functionsAgain;
        collectRows(headingRow(), QString(), rowsAgain, functionsAgain);
        QCOMPARE(rowsAgain, everyRow);
        QVERIFY2(varUnit()->savedVars == keptBefore, "listing the functions again changed what the profile keeps");

        mpEditor->slot_showTriggers();
        QCoreApplication::processEvents();
        QVERIFY2(!pSwitch->isVisible(), "the switch for functions is still showing outside the Variables view");
    }

    // The search walks Lua's tree, not the rows, so it leaves functions out by a test of its own
    void test_theSearchLeavesFunctionsOutWhileTheSwitchIsOff()
    {
        QVERIFY2(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("qaToolbox = { qaToolboxRun = function() return 2 end, qaToolboxLabel = \"tools\" }\n"
                                                                     "qaToolboxCall = function() return 3 end\n")),
                 "the variables this case searches for could not be made");
        QCheckBox* pSwitch = mpEditor->mpCheckBox_showFunctionVariables;
        const enums::EditorSearchOptions optionsBefore = mpEditor->mSearchOptions;
        const auto restore = qScopeGuard([this, pSwitch, optionsBefore]() {
            mpEditor->slot_clearSearchResults();
            mpEditor->setSearchOptions(optionsBefore);
            pSwitch->setChecked(true);
            mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("qaToolbox = nil\nqaToolboxCall = nil\n"));
            openTheVariablesView();
        });
        openTheVariablesView();
        // Off, as the Variables view searches variables, and searches them again, whatever the option says
        mpEditor->setSearchOptions(enums::EditorSearchOptionNone);

        const QStringList shown = searchResultTitles(qsl("qaToolbox"));
        qInfo().noquote() << qsl("  with functions shown the search finds: %1").arg(shown.join(qsl(", ")));
        QVERIFY2(!mentioning(shown, qsl("qaToolboxRun")).isEmpty() && !mentioning(shown, qsl("qaToolboxCall")).isEmpty(),
                 "the search does not find the functions even with the switch on, so this case says nothing");

        // Results on show are searched for again, as one for a function would open nothing once its row is gone
        pSwitch->setChecked(false);
        QCoreApplication::processEvents();
        const QStringList leftOnShow = currentSearchResultTitles();
        const QStringList functionsLeftOnShow = mentioning(leftOnShow, qsl("qaToolboxRun")) + mentioning(leftOnShow, qsl("qaToolboxCall"));
        QVERIFY2(functionsLeftOnShow.isEmpty(), qPrintable(qsl("turning the switch off left these results on show: %1").arg(functionsLeftOnShow.join(qsl(", ")))));
        QVERIFY2(!mentioning(leftOnShow, qsl("qaToolboxLabel")).isEmpty(), "turning the switch off lost the result for a member that is not a function");

        mpEditor->slot_clearSearchResults();
        const QStringList hidden = searchResultTitles(qsl("qaToolbox"));
        qInfo().noquote() << qsl("  with functions left out the search finds: %1").arg(hidden.join(qsl(", ")));
        const QStringList functionsFound = mentioning(hidden, qsl("qaToolboxRun")) + mentioning(hidden, qsl("qaToolboxCall"));
        QVERIFY2(functionsFound.isEmpty(), qPrintable(qsl("with the switch off the search still finds: %1").arg(functionsFound.join(qsl(", ")))));
        QVERIFY2(!mentioning(hidden, qsl("qaToolboxLabel")).isEmpty(), "with the switch off the search lost a member that is not a function");
    }
};

#include "EditorVariablesTreeTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorVariablesTreeTest)
