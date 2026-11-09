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
 * Guards that every row of the six item trees is one height and leads with a
 * tinted glyph: a row that takes its picture's height (24px folder, 9px dot)
 * makes the hover fill and the selection pill visibly different sizes.
 *
 * Run with: ctest -R EditorTreeRowHeightTest -V
 */

#include <QImage>
#include <QMouseEvent>
#include <QStyleOptionViewItem>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>

#include "EditorTreeDelegate.h"
#include "Host.h"
#include "HostDialogs.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTreeWidget.h"
#include "TTrigger.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorTreeRowHeightTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorTreeRowHeight-Test-Profile");

    // Written out rather than read from the delegate: a test that reads the
    // number out of the code under test cannot fail when that number moves
    static constexpr int scmMarkSize = 16;
    // The same glyph composed twice at one ratio can round a unit or two apart
    // per channel
    static constexpr int scmInkTolerance = 2;
    // Further in than the pill's 8px corner, so the reading lands on the fill
    // rather than on the arc
    static constexpr int scmClearOfTheCorners = 20;

    int mTriggerFolderId = 0;
    int mTriggerItemId = 0;
    int mFilterChainId = 0;
    int mFilteredTriggerId = 0;
    int mBrokenTriggerId = 0;
    int mTimerFolderId = 0;
    int mTimerItemId = 0;
    int mOffsetTimerId = 0;
    int mAliasFolderId = 0;
    int mAliasItemId = 0;
    int mScriptFolderId = 0;
    int mScriptItemId = 0;
    int mActionFolderId = 0;
    int mActionItemId = 0;
    int mKeyFolderId = 0;
    int mKeyItemId = 0;

    // Built in the profile rather than posted into the trees, so the rows come
    // from fillout_form() and carry a saved item's accessible description
    void buildTheProfilesItems()
    {
        auto* pTriggerFolder = new TTrigger(qsl("A group of triggers"), QStringList(), QList<int>(), false, mpHost);
        pTriggerFolder->setIsFolder(true);
        pTriggerFolder->setIsActive(true);
        pTriggerFolder->registerTrigger();
        mTriggerFolderId = pTriggerFolder->getID();

        auto* pTrigger = new TTrigger(pTriggerFolder, mpHost);
        pTrigger->setName(qsl("A trigger"));
        pTrigger->setRegexCodeList(QStringList{qsl("hello")}, QList<int>{REGEX_SUBSTRING}, false);
        pTrigger->setIsActive(true);
        pTrigger->registerTrigger();
        mTriggerItemId = pTrigger->getID();

        // A trigger with a pattern is a filter chain only once it holds a child
        auto* pFilterChain = new TTrigger(qsl("A filter chain"), QStringList{qsl("chapter")}, QList<int>{REGEX_SUBSTRING}, false, mpHost);
        pFilterChain->setIsActive(true);
        pFilterChain->registerTrigger();
        mFilterChainId = pFilterChain->getID();
        auto* pFiltered = new TTrigger(pFilterChain, mpHost);
        pFiltered->setName(qsl("Matched inside the chain"));
        pFiltered->setRegexCodeList(QStringList{qsl("verse")}, QList<int>{REGEX_SUBSTRING}, false);
        pFiltered->registerTrigger();
        mFilteredTriggerId = pFiltered->getID();

        // PCRE cannot compile an unterminated group, which is the errors glyph's state
        auto* pBroken = new TTrigger(qsl("A broken trigger"), QStringList{qsl("(")}, QList<int>{REGEX_PERL}, false, mpHost);
        pBroken->registerTrigger();
        mBrokenTriggerId = pBroken->getID();

        auto* pTimerFolder = new TTimer(qsl("A group of timers"), QTime(), mpHost);
        pTimerFolder->setIsFolder(true);
        mpHost->getTimerUnit()->registerTimer(pTimerFolder);
        mTimerFolderId = pTimerFolder->getID();

        auto* pTimer = new TTimer(pTimerFolder, mpHost);
        pTimer->setName(qsl("A timer"));
        pTimer->setTime(QTime(0, 0, 5));
        mpHost->getTimerUnit()->registerTimer(pTimer);
        mTimerItemId = pTimer->getID();

        // Being held by a timer rather than a folder is what makes an offset timer
        auto* pOffsetTimer = new TTimer(pTimer, mpHost);
        pOffsetTimer->setName(qsl("An offset timer"));
        pOffsetTimer->setTime(QTime(0, 0, 1));
        mpHost->getTimerUnit()->registerTimer(pOffsetTimer);
        mOffsetTimerId = pOffsetTimer->getID();

        auto* pAliasFolder = new TAlias(qsl("A group of aliases"), mpHost);
        pAliasFolder->setIsFolder(true);
        pAliasFolder->setIsActive(true);
        pAliasFolder->registerAlias();
        mAliasFolderId = pAliasFolder->getID();
        auto* pAlias = new TAlias(pAliasFolder, mpHost);
        pAlias->setName(qsl("An alias"));
        pAlias->setRegexCode(qsl("^greet$"));
        pAlias->setIsActive(true);
        pAlias->registerAlias();
        mAliasItemId = pAlias->getID();

        auto* pScriptFolder = new TScript(qsl("A group of scripts"), mpHost);
        pScriptFolder->setIsFolder(true);
        pScriptFolder->setIsActive(true);
        pScriptFolder->registerScript();
        mScriptFolderId = pScriptFolder->getID();
        auto* pScript = new TScript(pScriptFolder, mpHost);
        pScript->setName(qsl("A script"));
        pScript->setScript(qsl("-- nothing to do\n"));
        pScript->setIsActive(true);
        pScript->registerScript();
        mScriptItemId = pScript->getID();

        auto* pActionFolder = new TAction(qsl("A toolbar"), mpHost);
        pActionFolder->setIsFolder(true);
        pActionFolder->setIsActive(true);
        pActionFolder->registerAction();
        mActionFolderId = pActionFolder->getID();
        auto* pAction = new TAction(pActionFolder, mpHost);
        pAction->setName(qsl("A button"));
        pAction->setIsActive(true);
        pAction->registerAction();
        mActionItemId = pAction->getID();

        auto* pKeyFolder = new TKey(qsl("A group of keys"), mpHost);
        pKeyFolder->setIsFolder(true);
        pKeyFolder->setIsActive(true);
        pKeyFolder->registerKey();
        mKeyFolderId = pKeyFolder->getID();
        auto* pKey = new TKey(pKeyFolder, mpHost);
        pKey->setName(qsl("A key"));
        pKey->setIsActive(true);
        pKey->registerKey();
        mKeyItemId = pKey->getID();
    }

    QList<QPair<QString, QPair<TTreeWidget*, QList<QPair<QString, int>>>>> trees() const
    {
        return {{qsl("Triggers"),
                 {mpEditor->treeWidget_triggers,
                  {{qsl("folder"), mTriggerFolderId},
                   {qsl("item"), mTriggerItemId},
                   {qsl("filter chain"), mFilterChainId},
                   {qsl("filtered item"), mFilteredTriggerId},
                   {qsl("broken item"), mBrokenTriggerId}}}},
                {qsl("Timers"), {mpEditor->treeWidget_timers, {{qsl("folder"), mTimerFolderId}, {qsl("item"), mTimerItemId}, {qsl("offset timer"), mOffsetTimerId}}}},
                {qsl("Aliases"), {mpEditor->treeWidget_aliases, {{qsl("folder"), mAliasFolderId}, {qsl("item"), mAliasItemId}}}},
                {qsl("Scripts"), {mpEditor->treeWidget_scripts, {{qsl("folder"), mScriptFolderId}, {qsl("item"), mScriptItemId}}}},
                {qsl("Buttons"), {mpEditor->treeWidget_actions, {{qsl("folder"), mActionFolderId}, {qsl("item"), mActionItemId}}}},
                {qsl("Keys"), {mpEditor->treeWidget_keys, {{qsl("folder"), mKeyFolderId}, {qsl("item"), mKeyItemId}}}}};
    }

    QTreeWidgetItem* headingOf(const TTreeWidget* pTree) const
    {
        if (pTree == mpEditor->treeWidget_triggers) {
            return mpEditor->mpTriggerBaseItem;
        }
        if (pTree == mpEditor->treeWidget_timers) {
            return mpEditor->mpTimerBaseItem;
        }
        if (pTree == mpEditor->treeWidget_aliases) {
            return mpEditor->mpAliasBaseItem;
        }
        if (pTree == mpEditor->treeWidget_scripts) {
            return mpEditor->mpScriptsBaseItem;
        }
        if (pTree == mpEditor->treeWidget_actions) {
            return mpEditor->mpActionBaseItem;
        }
        return mpEditor->mpKeyBaseItem;
    }

    // The delegate is asked rather than the window grabbed: off a screenshot, a
    // 16px mark could only say what colour is somewhere near it
    QImage markOf(TTreeWidget* pTree, QTreeWidgetItem* pItem, const bool selected) const
    {
        auto* pDelegate = qobject_cast<uiDesign::EditorTreeDelegate*>(pTree->itemDelegateForIndex(QModelIndex()));
        if (!pDelegate || !pItem) {
            return {};
        }
        const QRect rowRect = pTree->visualItemRect(pItem);
        if (rowRect.isEmpty()) {
            return {};
        }
        QStyleOptionViewItem option = pTree->viewItemOption();
        option.rect = rowRect;
        if (selected) {
            option.state |= QStyle::State_Selected;
        }
        pDelegate->initStyleOption(&option, pTree->indexAt(rowRect.center()));

        const qreal ratio = pTree->devicePixelRatioF();
        const QImage decoration = option.icon.pixmap(option.decorationSize, ratio).toImage().convertToFormat(QImage::Format_ARGB32);
        const int side = qRound(scmMarkSize * ratio);
        if (decoration.width() < side || decoration.height() < side) {
            return {};
        }
        // The mark is the last thing in the composed leading edge, and the slot
        // is exactly its height
        return decoration.copy(QRect(decoration.width() - side, decoration.height() - side, side, side));
    }

    QImage markDrawnFrom(const TTreeWidget* pTree, const QString& glyphFile, const QColor& ink) const
    {
        const qreal ratio = pTree->devicePixelRatioF();
        return uiDesign::tintedGlyph(uiDesign::glyphPixmap(glyphFile), ink)
                .scaled(QSize(scmMarkSize, scmMarkSize) * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                .toImage()
                .convertToFormat(QImage::Format_ARGB32);
    }

    static int worstDifference(const QImage& one, const QImage& other)
    {
        if (one.isNull() || other.isNull() || one.size() != other.size()) {
            return 255;
        }
        int worst = 0;
        for (int y = 0; y < one.height(); ++y) {
            for (int x = 0; x < one.width(); ++x) {
                const QColor here = one.pixelColor(x, y);
                const QColor there = other.pixelColor(x, y);
                worst = std::max({worst, std::abs(here.red() - there.red()), std::abs(here.green() - there.green()), std::abs(here.blue() - there.blue()), std::abs(here.alpha() - there.alpha())});
            }
        }
        return worst;
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        buildTheProfilesItems();

        mudlet::self()->slot_showTriggerDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1000, 800);
        // The last case reads a grab, so wait for the first paint; not fatal,
        // because grab() renders the widget tree itself
        if (!QTest::qWaitForWindowExposed(mpEditor, 2000)) {
            qInfo().noquote() << qsl("  the editor window was never exposed; the readings below are off a rendered widget tree alone");
        }

        // The editor is built with the profile, so its trees were filled before
        // the items above existed
        for (const auto& tree : trees()) {
            tree.second.first->clear();
        }
        mpEditor->fillout_form();
        mpEditor->slot_showTriggers();
        QTest::qWait(50ms);

        // A collapsed row has no rectangle to measure, and the trees open only
        // their heading row by themselves
        for (const auto& tree : trees()) {
            tree.second.first->expandAll();
        }
        QTest::qWait(50ms);

        // Checked first: a tree missing a row would leave a case comparing
        // nothing at all and passing
        QStringList missing;
        for (const auto& tree : trees()) {
            QTreeWidgetItem* pHeading = headingOf(tree.second.first);
            if (!pHeading || tree.second.first->visualItemRect(pHeading).isEmpty()) {
                missing << qsl("%1: no heading row").arg(tree.first);
                continue;
            }
            for (const auto& row : tree.second.second) {
                QTreeWidgetItem* pRow = TestPixels::rowFor(pHeading, row.second);
                if (!pRow || tree.second.first->visualItemRect(pRow).isEmpty()) {
                    missing << qsl("%1: no %2 row").arg(tree.first, row.first);
                }
            }
        }
        QVERIFY2(missing.isEmpty(), qPrintable(qsl("the profile the cases below read was not built: %1").arg(missing.join(qsl("; ")))));

        TTrigger* pFilterChain = mpHost->getTriggerUnit()->getTrigger(mFilterChainId);
        QVERIFY2(pFilterChain && pFilterChain->isFilterChain(), "The trigger the fixture meant as a filter chain is not one");
        TTrigger* pBroken = mpHost->getTriggerUnit()->getTrigger(mBrokenTriggerId);
        QVERIFY2(pBroken && !pBroken->state(), "The trigger the fixture meant to leave broken compiles after all");
        TTimer* pOffset = mpHost->getTimerUnit()->getTimer(mOffsetTimerId);
        QVERIFY2(pOffset && pOffset->isOffsetTimer(), "The timer the fixture meant as an offset timer is not one");
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_everyRowOfEveryTreeIsTheSameHeight()
    {
        QStringList uneven;
        QStringList measured;
        for (const auto& tree : trees()) {
            TTreeWidget* pTree = tree.second.first;
            QTreeWidgetItem* pHeading = headingOf(pTree);
            const int headingHeight = pTree->visualItemRect(pHeading).height();
            QStringList heights{qsl("heading %1").arg(headingHeight)};
            for (const auto& row : tree.second.second) {
                const int height = pTree->visualItemRect(TestPixels::rowFor(pHeading, row.second)).height();
                heights << qsl("%1 %2").arg(row.first, QString::number(height));
                if (height != headingHeight) {
                    uneven << qsl("%1: the heading row is %2px and the %3 row %4px").arg(tree.first, QString::number(headingHeight), row.first, QString::number(height));
                }
            }
            measured << qsl("%1 - %2").arg(tree.first, heights.join(qsl(", ")));
        }
        qInfo().noquote() << qsl("  row heights: %1").arg(measured.join(qsl("; ")));

        QVERIFY2(uneven.isEmpty(), qPrintable(qsl("rows in the panel of items are not all one height: %1").arg(uneven.join(qsl("; ")))));
    }

    void test_eachKindOfRowCarriesItsOwnGlyph()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        TTreeWidget* pTriggers = mpEditor->treeWidget_triggers;
        TTreeWidget* pTimers = mpEditor->treeWidget_timers;
        QTreeWidgetItem* pTriggerHeading = mpEditor->mpTriggerBaseItem;
        QTreeWidgetItem* pTimerHeading = mpEditor->mpTimerBaseItem;

        const QList<QPair<QString, QPair<QImage, QImage>>> readings{
                {qsl("a folder"), {markOf(pTriggers, TestPixels::rowFor(pTriggerHeading, mTriggerFolderId), false), markDrawnFrom(pTriggers, qsl(":/icons/editor-folder.svg"), tokens.mutedText)}},
                {qsl("a filter chain"), {markOf(pTriggers, TestPixels::rowFor(pTriggerHeading, mFilterChainId), false), markDrawnFrom(pTriggers, qsl(":/icons/editor-filter.svg"), tokens.mutedText)}},
                // Not chrome: the red of its compile note, held to a contrast
                // floor in EditorTreeErrorMarkTest
                {qsl("a broken item"),
                 {markOf(pTriggers, TestPixels::rowFor(pTriggerHeading, mBrokenTriggerId), false), markDrawnFrom(pTriggers, qsl(":/icons/editor-errors.svg"), uiDesign::errorInk(tokens))}},
                {qsl("an offset timer"),
                 {markOf(pTimers, TestPixels::rowFor(pTimerHeading, mOffsetTimerId), false), markDrawnFrom(pTimers, qsl(":/icons/editor-offset-timer.svg"), tokens.mutedText)}}};

        QStringList wrong;
        QStringList measured;
        for (const auto& reading : readings) {
            const int difference = worstDifference(reading.second.first, reading.second.second);
            measured << qsl("%1 %2").arg(reading.first, TestPixels::inkOf(reading.second.first));
            if (difference > scmInkTolerance) {
                wrong << qsl("%1: the row is drawn %2 where the glyph it should carry is %3, %4 apart at their worst pixel")
                                 .arg(reading.first, TestPixels::inkOf(reading.second.first), TestPixels::inkOf(reading.second.second), QString::number(difference));
            }
        }
        qInfo().noquote() << qsl("  marks at %1px: %2; the muted tone is %3").arg(QString::number(scmMarkSize), measured.join(qsl(", ")), tokens.mutedText.name());

        QVERIFY2(wrong.isEmpty(), qPrintable(qsl("a row does not lead with the glyph that says what it is: %1").arg(wrong.join(qsl("; ")))));
    }

    void test_aChosenRowsMarkIsInkedWithItsName()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        TTreeWidget* pTree = mpEditor->treeWidget_triggers;
        QTreeWidgetItem* pFolder = TestPixels::rowFor(mpEditor->mpTriggerBaseItem, mTriggerFolderId);

        const QImage unchosen = markOf(pTree, pFolder, false);
        const QImage chosen = markOf(pTree, pFolder, true);
        const QImage wantedChosen = markDrawnFrom(pTree, qsl(":/icons/editor-folder.svg"), tokens.accentText);
        qInfo().noquote() << qsl("  the folder's mark is %1 unchosen and %2 chosen; the muted tone is %3 and the chosen text colour %4")
                                     .arg(TestPixels::inkOf(unchosen), TestPixels::inkOf(chosen), tokens.mutedText.name(), tokens.accentText.name());

        QVERIFY2(worstDifference(unchosen, chosen) > scmInkTolerance, qPrintable(qsl("a chosen row's mark is inked the same as an unchosen one's: both are %1").arg(TestPixels::inkOf(chosen))));
        QVERIFY2(worstDifference(chosen, wantedChosen) <= scmInkTolerance,
                 qPrintable(qsl("a chosen row's mark is not inked in the colour its name is written in: it is %1 where the name's colour would give %2")
                                    .arg(TestPixels::inkOf(chosen), TestPixels::inkOf(wantedChosen))));
    }

    void test_theHoverFillAndTheSelectionPillAreOneHeight()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        TTreeWidget* pTree = mpEditor->treeWidget_triggers;
        QTreeWidgetItem* pHeading = mpEditor->mpTriggerBaseItem;
        // A row with a mark and one without, far enough apart that the two
        // shapes cannot run together and be measured as one
        QTreeWidgetItem* pHovered = TestPixels::rowFor(pHeading, mTriggerFolderId);
        QTreeWidgetItem* pChosen = TestPixels::rowFor(pHeading, mFilteredTriggerId);
        QVERIFY2(pHovered && pChosen, "One of the two rows this case reads is not in the tree");

        pTree->setCurrentItem(pChosen);
        // A row scrolled out of the viewport is painted nowhere
        pTree->scrollToItem(pHovered, QAbstractItemView::PositionAtTop);
        mpEditor->activateWindow();
        pTree->setFocus();
        QTest::qWait(50ms);

        QVERIFY2(pTree->viewport()->rect().contains(pTree->visualItemRect(pHovered)) && pTree->viewport()->rect().contains(pTree->visualItemRect(pChosen)),
                 "Both rows have to be on screen to be read off a shot of the window");
        QVERIFY2(std::abs(pTree->visualItemRect(pHovered).top() - pTree->visualItemRect(pChosen).top()) > pTree->visualItemRect(pHovered).height() + pTree->visualItemRect(pChosen).height(),
                 "The hovered and the chosen row are neighbours, so their two shapes would be measured as one");

        // Read near the trailing end: clear of the corner arc and past the rows'
        // names, so the run stops at the shape's edge, not in the next row's letters
        const auto paintedHeightAt = [&](const QImage& shot, const QRect& band) {
            // On a HiDPI screen the grab is devicePixelRatio times the window's size
            const qreal shotRatio = shot.devicePixelRatio();
            const int x = band.right() - scmClearOfTheCorners;
            const auto painted = [&](const int y) {
                const QPoint inWindow = pTree->viewport()->mapTo(mpEditor, QPoint(x, y));
                const QPoint inShot(qRound(inWindow.x() * shotRatio), qRound(inWindow.y() * shotRatio));
                return shot.rect().contains(inShot) && shot.pixelColor(inShot).rgb() != tokens.pane.rgb();
            };
            const int middle = band.center().y();
            if (!painted(middle)) {
                return 0;
            }
            int top = middle;
            int bottom = middle;
            const int reach = 3 * band.height();
            while (middle - top < reach && painted(top - 1)) {
                --top;
            }
            while (bottom - middle < reach && painted(bottom + 1)) {
                ++bottom;
            }
            return bottom - top + 1;
        };

        // The view drops its hover state on a leave or a lost activation (another
        // process opening a window is enough), so the move is resent each attempt
        constexpr int scmAttempts = 5;
        QRect hoveredBand;
        QRect chosenBand;
        int hoveredHeight = 0;
        int chosenHeight = 0;
        int attempts = 0;
        while (attempts < scmAttempts && (hoveredHeight == 0 || chosenHeight == 0)) {
            ++attempts;
            hoveredBand = pTree->visualItemRect(pHovered);
            chosenBand = pTree->visualItemRect(pChosen);
            const QPoint hoveredAt = hoveredBand.center();
            QMouseEvent move(QEvent::MouseMove, QPointF(hoveredAt), QPointF(hoveredAt), pTree->viewport()->mapToGlobal(hoveredAt), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(pTree->viewport(), &move);
            QCoreApplication::sendPostedEvents();
            QTest::qWait(50ms);

            const QImage shot = mpEditor->grab().toImage();
            hoveredHeight = paintedHeightAt(shot, hoveredBand);
            chosenHeight = paintedHeightAt(shot, chosenBand);
        }
        qInfo().noquote() << qsl("  the hover fill is %1px tall and the selection pill %2px, on rows of %3px and %4px, read on attempt %5; the panel is %6")
                                     .arg(QString::number(hoveredHeight),
                                          QString::number(chosenHeight),
                                          QString::number(hoveredBand.height()),
                                          QString::number(chosenBand.height()),
                                          QString::number(attempts),
                                          tokens.pane.name());

        QVERIFY2(hoveredHeight > 0, qPrintable(qsl("the row under the pointer is not tinted at all after %1 attempts, so there is nothing to compare the pill with").arg(QString::number(attempts))));
        QVERIFY2(chosenHeight > 0, qPrintable(qsl("the chosen row is not painted at all after %1 attempts, so there is nothing to compare the hover with").arg(QString::number(attempts))));
        QVERIFY2(hoveredHeight == chosenHeight,
                 qPrintable(qsl("the wash under the hovered row is %1px tall and the pill under the chosen row %2px").arg(QString::number(hoveredHeight), QString::number(chosenHeight))));
    }
};

#include "EditorTreeRowHeightTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorTreeRowHeightTest)
