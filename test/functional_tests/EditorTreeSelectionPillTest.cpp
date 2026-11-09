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
 * A chosen tree row must be one rounded pill in one tone. Guards the style-drawn
 * branch column (QTreeWidget::item rules do not reach it) and the row-wide focus
 * rect QTreeView::drawRow draws through the style, where "outline: none" cannot reach.
 *
 * Run with: ctest -R EditorTreeSelectionPillTest -V
 */

#include <QImage>
#include <QListWidget>
#include <QMouseEvent>
#include <QPixmap>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>
#include <cstdlib>
#include <tuple>

#include "Host.h"
#include "HostDialogs.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "TTreeWidget.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorTreeSelectionPillTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorTreeSelectionPill-Test-Profile");

    // The inner group is the chosen row: two levels down, so the view lays two
    // branch cells out ahead of it, and holding a trigger, so it has a chevron
    QTreeWidgetItem* mpOuterGroup = nullptr;
    QTreeWidgetItem* mpInnerGroup = nullptr;

    // The pill's 8px corner cuts into the line under its top edge, so readings
    // stay this far in to land on the fill rather than the arc
    static constexpr int scmClearOfTheCorners = 12;

    // The fill is composited and then rounded to eight bits per channel
    static constexpr int scmChannelSlack = 2;

    TTreeWidget* tree() const { return mpEditor->treeWidget_triggers; }

    QPoint inViewport(const QPoint& point) const { return tree()->viewport()->mapTo(mpEditor, point); }

    // The viewport's full width: an indented row's visualItemRect starts past
    // the arrow column the selection is drawn into
    QRect rowBand(QTreeWidgetItem* pItem) const
    {
        const QRect rowRect = tree()->visualItemRect(pItem);
        return QRect(0, rowRect.top(), tree()->viewport()->width(), rowRect.height());
    }

    static QString describe(const QColor& measured, const uiDesign::ThemeTokens& tokens)
    {
        return qsl("%1 - the pane is %2, the accent %3, the page %4").arg(measured.name(), tokens.pane.name(), tokens.accent.name(), tokens.page.name());
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
        mpEditor->resize(1000, 800);
        mpEditor->slot_showTriggers();
        QTest::qWait(50ms);

        // addTrigger() adds inside the chosen folder and then chooses the new
        // item, so each nests in the one before
        mpEditor->addTrigger(true);
        mpOuterGroup = tree()->currentItem();
        mpEditor->addTrigger(true);
        mpInnerGroup = tree()->currentItem();
        mpEditor->addTrigger(false);
        QTest::qWait(50ms);

        QVERIFY2(mpOuterGroup && mpInnerGroup, "The editor did not make the two groups this test reads a row out of");
        QVERIFY2(mpInnerGroup->parent() == mpOuterGroup, "The second group was not made inside the first, so the row read below is not a nested one");
        QVERIFY2(mpInnerGroup->childCount() == 1, "The trigger was not made inside the second group, so that group has nothing to fold and no chevron");

        mpEditor->mpTriggerBaseItem->setExpanded(true);
        mpOuterGroup->setExpanded(true);
        mpInnerGroup->setExpanded(true);
        tree()->setCurrentItem(mpInnerGroup);
        // The platform focus rect that squares the pill off is drawn only on a
        // focused tree's current row
        mpEditor->activateWindow();
        tree()->setFocus();
        QTest::qWait(50ms);
        QVERIFY2(tree()->hasFocus(), "The tree does not hold the keyboard, so the state a chosen row is drawn in here is not the one the reader sees");

        QVERIFY2(!rowBand(mpInnerGroup).isEmpty(), "The chosen row is not laid out");
        QVERIFY2(rowBand(mpInnerGroup).width() > 4 * scmClearOfTheCorners, "The panel of items is too narrow to read a row across");
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    // Read one pixel under the row's top: the chevron, dot and name are centred
    // on its middle, so this reads the fill
    void test_theArrowColumnAndTheNameColumnAreOneTone()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QRect band = rowBand(mpInnerGroup);
        const QImage shot = TestPixels::shot(mpEditor);
        const int y = band.top() + 1;

        // The second indentation step: this row's chevron, and the innermost
        // branch cell the view lays out for a row this deep
        const QColor arrowColumn = shot.pixelColor(inViewport(QPoint(band.left() + 30, y)));
        const QColor nameColumn = shot.pixelColor(inViewport(QPoint(band.right() - 20, y)));

        qInfo().noquote() << qsl("measured across the chosen row: arrow column %1, name column %2; the pane is %3 and the accent %4")
                                     .arg(arrowColumn.name(), nameColumn.name(), tokens.pane.name(), tokens.accent.name());

        QVERIFY2(arrowColumn.rgb() != tokens.pane.rgb(), qPrintable(qsl("the chosen row is not painted at all: the arrow column is %1").arg(describe(arrowColumn, tokens))));
        QVERIFY2(arrowColumn.rgb() == nameColumn.rgb(),
                 qPrintable(qsl("the chosen row is two tones: its arrow column is painted %1 and the far end of it %2").arg(arrowColumn.name(), nameColumn.name())));

        QStringList uneven;
        for (int x = band.left() + scmClearOfTheCorners; x <= band.right() - scmClearOfTheCorners; ++x) {
            const QColor measured = shot.pixelColor(inViewport(QPoint(x, y)));
            if (measured.rgb() != arrowColumn.rgb()) {
                uneven << qsl("x=%1 %2").arg(QString::number(x), measured.name());
            }
        }
        QVERIFY2(uneven.isEmpty(),
                 qPrintable(qsl("the chosen row is not one tone from end to end: it is %1 at the arrow column and %2 elsewhere").arg(arrowColumn.name(), uneven.mid(0, 12).join(qsl(", ")))));
    }

    void test_theSelectionIsAPillWithRoundedCorners()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QRect band = rowBand(mpInnerGroup);
        const QImage shot = TestPixels::shot(mpEditor);

        const QColor fill = shot.pixelColor(inViewport(QPoint(band.center().x(), band.top() + 1)));
        const QColor topMiddle = shot.pixelColor(inViewport(QPoint(band.center().x(), band.top())));
        // The accent bar is cut to the pill's corner too, so it paints nothing
        // at the leading corners
        const QList<QPair<QString, QColor>> corners{{qsl("leading top"), shot.pixelColor(inViewport(band.topLeft()))},
                                                    {qsl("trailing top"), shot.pixelColor(inViewport(band.topRight()))},
                                                    {qsl("leading bottom"), shot.pixelColor(inViewport(band.bottomLeft()))},
                                                    {qsl("trailing bottom"), shot.pixelColor(inViewport(band.bottomRight()))}};

        QStringList measured;
        for (const auto& corner : corners) {
            measured << qsl("%1 %2").arg(corner.first, corner.second.name());
        }
        qInfo().noquote()
                << qsl("corners of the chosen row: %1; its fill is %2, the middle of its top edge %3, the pane %4").arg(measured.join(qsl(", ")), fill.name(), topMiddle.name(), tokens.pane.name());

        QVERIFY2(topMiddle.rgb() != tokens.pane.rgb(),
                 qPrintable(qsl("the chosen row does not reach the top of its band, so the corners below say nothing: the middle of that edge is %1").arg(describe(topMiddle, tokens))));
        for (const auto& corner : corners) {
            // Exact: the pill's arc does not reach the band's extreme corner, so
            // only a square focus rect could paint this pixel
            QVERIFY2(corner.second.rgb() == tokens.pane.rgb(),
                     qPrintable(qsl("the chosen row's %1 corner is not cut away to the panel: it is painted %2, %3 off the panel and %4 off the row's own fill of %5")
                                        .arg(corner.first,
                                             describe(corner.second, tokens),
                                             QString::number(TestPixels::distance(corner.second, tokens.pane)),
                                             QString::number(TestPixels::distance(corner.second, fill)),
                                             fill.name())));
        }
    }

    // The bar's top and bottom lines are checked because a border-left bar
    // pinches inward into a bracket and a plain rectangle stands square past
    // the pill's corners
    void test_theChosenRowLeadsWithTheAccentBar()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QRect chosenBand = rowBand(mpInnerGroup);
        // Not hovered yet, so its leading edge is the panel: the bar is the
        // selection's rather than every row's
        const QRect plainBand = rowBand(mpOuterGroup);
        QVERIFY2(plainBand.top() < chosenBand.top(), "The row read as the unchosen one is not above the chosen one");
        const QImage shot = TestPixels::shot(mpEditor);

        // The corner radius exceeds the bar's width, so the arc clears the whole
        // bar on the row's first and last line
        const QList<std::tuple<QString, int, QColor>> lines{
                {qsl("top"), chosenBand.top(), tokens.pane}, {qsl("middle"), chosenBand.center().y(), tokens.accent}, {qsl("bottom"), chosenBand.bottom(), tokens.pane}};
        QStringList measuredBar;
        QStringList measuredPlain;
        QStringList wrong;
        for (const auto& [name, y, expected] : lines) {
            QStringList acrossTheBar;
            for (int x = 0; x < uiDesign::scmAccentBarWidth; ++x) {
                const QColor bar = shot.pixelColor(inViewport(QPoint(chosenBand.left() + x, y)));
                acrossTheBar << bar.name();
                if (bar.rgb() != expected.rgb()) {
                    wrong << qsl("the chosen row's %1 line is %2 at x=%3 where it should be %4").arg(name, bar.name(), QString::number(x), expected.name());
                }
            }
            measuredBar << qsl("%1 %2").arg(name, acrossTheBar.join(qsl(" ")));
        }
        for (int x = 0; x < uiDesign::scmAccentBarWidth; ++x) {
            const QColor plain = shot.pixelColor(inViewport(QPoint(plainBand.left() + x, plainBand.center().y())));
            measuredPlain << plain.name();
            if (plain.rgb() != tokens.pane.rgb()) {
                wrong << qsl("the row above it is %1 at x=%2 where the panel is %3").arg(plain.name(), QString::number(x), tokens.pane.name());
            }
        }
        const QColor afterTheBar = shot.pixelColor(inViewport(QPoint(chosenBand.left() + uiDesign::scmAccentBarWidth, chosenBand.center().y())));
        qInfo().noquote()
                << qsl("  the chosen row's leading %1px are %2 and the row above's are %3; the pixel after the bar is %4, the accent %5 and the panel %6")
                           .arg(QString::number(uiDesign::scmAccentBarWidth), measuredBar.join(qsl(", ")), measuredPlain.join(qsl(" ")), afterTheBar.name(), tokens.accent.name(), tokens.pane.name());

        QVERIFY2(wrong.isEmpty(), qPrintable(qsl("the accent bar down the chosen row is not what is painted: %1").arg(wrong.join(qsl("; ")))));
        QVERIFY2(afterTheBar.rgb() != tokens.accent.rgb(), qPrintable(qsl("the chosen row is the accent past the bar as well, at %1").arg(afterTheBar.name())));
        QVERIFY2(afterTheBar.rgb() != tokens.pane.rgb(), qPrintable(qsl("the chosen row is not filled past its bar: it is %1, which is the panel").arg(afterTheBar.name())));
    }

    void test_theChosenRowAndTheHoveredRowAreTheSameShape()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

        // Another row, so both states come off one grab: a change in the window
        // between two grabs would read as a difference in the shape
        QTreeWidgetItem* pHovered = mpOuterGroup;
        QVERIFY2(pHovered != mpInnerGroup, "The hovered and the chosen row must be two different rows");
        const QRect hoveredBand = rowBand(pHovered);
        const QPoint hoveredCentre = hoveredBand.center();
        QMouseEvent move(QEvent::MouseMove, QPointF(hoveredCentre), QPointF(hoveredCentre), tree()->viewport()->mapToGlobal(hoveredCentre), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(tree()->viewport(), &move);
        QTest::qWait(50ms);

        const QImage shot = TestPixels::shot(mpEditor);
        const QRect chosenBand = rowBand(mpInnerGroup);
        auto at = [&shot, this](const QPoint& point) {
            return shot.pixelColor(inViewport(point));
        };

        const QColor hoveredFill = at(QPoint(hoveredBand.center().x(), hoveredBand.top() + 3));
        const QColor chosenFill = at(QPoint(chosenBand.center().x(), chosenBand.top() + 3));
        QVERIFY2(hoveredFill.rgb() != tokens.pane.rgb(), qPrintable(qsl("the row under the pointer is not tinted at all: it is painted %1").arg(describe(hoveredFill, tokens))));
        QVERIFY2(chosenFill.rgb() != tokens.pane.rgb(), qPrintable(qsl("the chosen row is not painted at all: it is %1").arg(describe(chosenFill, tokens))));
        QVERIFY2(hoveredFill.rgb() != chosenFill.rgb(), "the hovered row and the chosen row are filled with the same colour, so this case is reading one state twice");

        const QList<QPair<QString, QPoint>> readings{
                {qsl("leading top corner"), QPoint(0, 0)}, {qsl("trailing top corner"), QPoint(1, 0)}, {qsl("leading bottom corner"), QPoint(0, 1)}, {qsl("trailing bottom corner"), QPoint(1, 1)}};
        QStringList measured;
        QStringList disagreed;
        for (const auto& reading : readings) {
            // Only the chosen row has the accent bar, but it is cut to the
            // pill's corner, so it paints nothing at the very corner either
            const auto cornerOf = [&reading](const QRect& band) {
                return QPoint(reading.second.x() == 0 ? band.left() : band.right(), reading.second.y() == 0 ? band.top() : band.bottom());
            };
            const QColor hovered = at(cornerOf(hoveredBand));
            const QColor chosen = at(cornerOf(chosenBand));
            measured << qsl("%1 hovered %2 chosen %3").arg(reading.first, hovered.name(), chosen.name());
            if (hovered.rgb() != tokens.pane.rgb() || chosen.rgb() != tokens.pane.rgb()) {
                disagreed << qsl("%1: hovered %2, chosen %3").arg(reading.first, hovered.name(), chosen.name());
            }
        }
        qInfo().noquote()
                << qsl("corners of the two rows: %1; the panel is %2, the hover fill %3, the chosen fill %4").arg(measured.join(qsl("; ")), tokens.pane.name(), hoveredFill.name(), chosenFill.name());

        QVERIFY2(disagreed.isEmpty(),
                 qPrintable(
                         qsl("the hovered row and the chosen row are not cut to the same corner - the panel is %1 and these corners are not: %2").arg(tokens.pane.name(), disagreed.join(qsl(", ")))));

        QStringList outlined;
        for (const int x : {chosenBand.left() + scmClearOfTheCorners, chosenBand.center().x(), chosenBand.right() - scmClearOfTheCorners}) {
            const QColor chosenEdge = at(QPoint(x, chosenBand.top()));
            const QColor hoveredEdge = at(QPoint(x, hoveredBand.top()));
            if (chosenEdge.rgb() != chosenFill.rgb() || hoveredEdge.rgb() != hoveredFill.rgb()) {
                outlined << qsl("x=%1 chosen %2 against a fill of %3, hovered %4 against %5").arg(QString::number(x), chosenEdge.name(), chosenFill.name(), hoveredEdge.name(), hoveredFill.name());
            }
        }
        QVERIFY2(outlined.isEmpty(), qPrintable(qsl("one of the two rows carries an outline the other does not: %1").arg(outlined.join(qsl(", ")))));
    }

    // Control: a QListWidget draws no row-wide focus rect, so a failure here
    // means the corners are measured wrong rather than painted wrong
    void test_theSidebarsChosenRowIsCutToTheSameCorner()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QListWidget* pSidebar = mpEditor->mpListWidget_editorSidebar;
        QVERIFY2(pSidebar != nullptr, "The editor has no sidebar");
        QListWidgetItem* pChosen = pSidebar->currentItem();
        QVERIFY2(pChosen != nullptr, "No sidebar row is chosen");

        const QRect rowRect = pSidebar->visualItemRect(pChosen);
        QVERIFY2(!rowRect.isEmpty(), "The chosen sidebar row is not laid out");
        const QImage shot = TestPixels::shot(mpEditor);
        auto at = [&](const QPoint& point) {
            return shot.pixelColor(pSidebar->viewport()->mapTo(mpEditor, point));
        };

        const QColor fill = at(QPoint(rowRect.center().x(), rowRect.center().y()));
        const QList<QPair<QString, QPoint>> corners{
                {qsl("leading top"), rowRect.topLeft()}, {qsl("trailing top"), rowRect.topRight()}, {qsl("leading bottom"), rowRect.bottomLeft()}, {qsl("trailing bottom"), rowRect.bottomRight()}};
        QStringList measured;
        for (const auto& corner : corners) {
            measured << qsl("%1 %2").arg(corner.first, at(corner.second).name());
        }
        qInfo().noquote() << qsl("corners of the sidebar's chosen row: %1; its fill is %2 and the page %3").arg(measured.join(qsl(", ")), fill.name(), tokens.page.name());

        QVERIFY2(fill.rgb() != tokens.page.rgb(), qPrintable(qsl("the sidebar's chosen row is not painted at all: its middle is %1").arg(fill.name())));
        for (const auto& corner : corners) {
            const QColor cornerColor = at(corner.second);
            QVERIFY2(TestPixels::distance(cornerColor, tokens.page) < TestPixels::distance(cornerColor, fill),
                     qPrintable(qsl("the sidebar's %1 corner is not cut away: it is painted %2 against a page of %3 and a fill of %4")
                                        .arg(corner.first, cornerColor.name(), tokens.page.name(), fill.name())));
        }
    }

    // Compared with what uiDesign::blend() mixes rather than a fraction taken
    // off the pixel, which would hold at any weight
    void test_theToneIsTheAccentMixedIntoThePane()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QRect band = rowBand(mpInnerGroup);
        const QColor measured = TestPixels::shot(mpEditor).pixelColor(inViewport(QPoint(band.center().x(), band.top() + 1)));
        const QColor wanted = uiDesign::blend(tokens.pane, tokens.accent, uiDesign::scmAccentWashStrength);

        qInfo().noquote() << qsl("  the chosen row is painted %1 against the %2 the design mixes at %3 of the way from the pane to the accent")
                                     .arg(describe(measured, tokens), wanted.name(), QString::number(uiDesign::scmAccentWashStrength, 'f', 2));

        const int channels[3][2] = {{measured.red(), wanted.red()}, {measured.green(), wanted.green()}, {measured.blue(), wanted.blue()}};
        for (const auto& channel : channels) {
            QVERIFY2(std::abs(channel[0] - channel[1]) <= scmChannelSlack,
                     qPrintable(qsl("the chosen row is painted %1, which is not the pane with %2 of the accent mixed into it - that is %3")
                                        .arg(describe(measured, tokens), QString::number(uiDesign::scmAccentWashStrength, 'f', 2), wanted.name())));
        }
    }
};

#include "EditorTreeSelectionPillTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorTreeSelectionPillTest)
