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
 * Every strip drawn by uiDesign::tabBarStyleSheet() and prepareTabStrip() - the
 * notepad's notes, the package manager's views and both of the connection
 * window's strips - is read off a grab in both appearances: the chosen tab's
 * word stands on an accent underline and no other tab's does, every word starts
 * on its tab's leading edge and the first one on the line the content under the
 * strip starts on, the gap trails each word, a cross stands after its word,
 * choosing another tab moves nothing, no word is cut short, a word under the
 * pointer changes only its ink, and the hairline the tabs stand on runs on past
 * the last tab to the end of what is under the strip. The same windows made
 * right to left are read again, as the mirror image of all of that. Under a
 * script's sheet that names QTabBar, on the application, on the window or on a
 * widget above a dialog's window, the script draws every strip whole and the
 * design steps aside, and it comes back when the sheet goes, all while the
 * windows stay open. A tab widget on the recipe has the size its pages, its
 * strip and its pane need as its size hint, so given that size it lays its
 * pages out at theirs; under a script's sheet its hint is that of one the
 * design never touched.
 *
 * Run with: ctest -R UnderlinedTabStripTest -V
 */

#include <QApplication>
#include <QDialog>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QHash>
#include <QHoverEvent>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPlainTextEdit>
#include <QScopeGuard>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtTest/QtTest>

#include <algorithm>
#include <functional>

#include "Host.h"
#include "HostDialogs.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgConnectionProfiles.h"
#include "dlgNotepad.h"
#include "dlgPackageManager.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

namespace {
// Its size hints are the sizes it is given, so a tab widget's own can be held against them
class SizedWidget : public QWidget
{
public:
    SizedWidget(const QSize& hint, const QSize& least)
    : mHint(hint)
    , mLeast(least)
    {
    }

    QSize sizeHint() const override { return mHint; }
    QSize minimumSizeHint() const override { return mLeast; }

private:
    const QSize mHint;
    const QSize mLeast;
};
} // namespace

class UnderlinedTabStripTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    Host* mpHost = nullptr;
    dlgNotepad* mpNotepad = nullptr;
    dlgPackageManager* mpPackages = nullptr;
    dlgConnectionProfiles* mpConnection = nullptr;
    const QString mProfileName = qsl("UnderlinedTabStrip-Test-Profile");

    // A flat fill or a hairline is written, not blended, so it reads back to a level or two
    static constexpr int scmInkTolerance = 2;
    // A glyph's ink starts a fraction of a pixel off the side bearing its font reports
    static constexpr int scmBearingTolerance = 1;
    // The halfway threshold can leave out the faintest column at either end of a word's ink
    static constexpr int scmInkExtentSlack = 2;
    static constexpr int scmScriptTabMinimumWidth = 120;

    struct Strip
    {
        QString what;
        QTabBar* pBar = nullptr;
        // Grabbed whole: a see-through bar grabbed on its own comes back without what shows through it
        QWidget* pWindow = nullptr;
        // In pWindow's coordinates
        int contentLeft = 0;
        int contentRight = 0;
    };

    static bool rightToLeft(const Strip& strip) { return strip.pBar->layoutDirection() == Qt::RightToLeft; }

    static QRect inWindow(const Strip& strip, const QRect& inBar) { return QRect(strip.pBar->mapTo(strip.pWindow, inBar.topLeft()), inBar.size()); }

    // The row the tabs stand on, which the chosen tab's underline is drawn over
    static int hairlineRow(const Strip& strip) { return strip.pBar->mapTo(strip.pWindow, QPoint(0, strip.pBar->height() - 1)).y(); }

    // Neither scrolled out of the bar nor under one of the scroll buttons a crowded bar makes for itself
    static bool onShow(const QTabBar* pBar, const QRect& tab)
    {
        if (!pBar->rect().contains(tab)) {
            return false;
        }
        const auto arrows = pBar->findChildren<QToolButton*>(Qt::FindDirectChildrenOnly);
        return std::none_of(arrows.cbegin(), arrows.cend(), [&tab](const QToolButton* pArrow) {
            return pArrow->isVisible() && pArrow->geometry().intersects(tab);
        });
    }

    static QColor commonestColour(const QImage& grab, const QRect& logical)
    {
        QHash<QRgb, int> counts;
        for (int y = logical.top(); y <= logical.bottom(); ++y) {
            for (int x = logical.left(); x <= logical.right(); ++x) {
                const QColor read = TestPixels::sampled(grab, QPoint(x, y));
                if (read.isValid()) {
                    ++counts[read.rgb()];
                }
            }
        }
        QRgb commonest = 0;
        int most = -1;
        for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
            if (it.value() > most) {
                most = it.value();
                commonest = it.key();
            }
        }
        return QColor(commonest);
    }

    // The line the word is laid in: the tab less its trailing gap, the air above it and the underline's room below
    static QRect wordLine(const Strip& strip, const int index)
    {
        const QRect tab = strip.pBar->tabRect(index);
        QRect line(tab.left(), tab.top() + uiDesign::scmTabPaddingTop, tab.width() - uiDesign::scmTabGap, tab.height() - uiDesign::scmTabPaddingTop - uiDesign::scmTabPaddingBottom);
        if (rightToLeft(strip)) {
            line.moveRight(tab.right());
        }
        return inWindow(strip, line);
    }

    // The word's line less the cross, if the tab has one
    static QRect wordLineClearOfTheCross(const Strip& strip, const int index)
    {
        QRect line = wordLine(strip, index);
        if (QWidget* pCross = strip.pBar->tabButton(index, QTabBar::RightSide)) {
            const QRect cross = inWindow(strip, pCross->geometry());
            if (rightToLeft(strip)) {
                line.setLeft(cross.right() + 1);
            } else {
                line.setRight(cross.left() - 1);
            }
        }
        return line;
    }

    // First and last column of the word's ink, thresholded halfway to the word's own ink so antialiased fringes count
    // only where they are more word than surface. The cross, if any, is left out.
    static QPair<int, int> inkSpan(const QImage& grab, const Strip& strip, const int index, const QColor& ink)
    {
        const QRect line = wordLineClearOfTheCross(strip, index);
        const QColor behind = commonestColour(grab, line);
        const int reach = TestPixels::distance(behind, ink);
        int first = -1;
        int last = -1;
        for (int x = line.left(); x <= line.right(); ++x) {
            for (int y = line.top(); y <= line.bottom(); ++y) {
                if (2 * TestPixels::distance(TestPixels::sampled(grab, QPoint(x, y)), behind) >= reach) {
                    first = first < 0 ? x : first;
                    last = x;
                    break;
                }
            }
        }
        return {first, last};
    }

    static QRect inDevicePixels(const QImage& grab, const QRect& logical)
    {
        const qreal ratio = grab.devicePixelRatio();
        return QRect(qRound(logical.left() * ratio), qRound(logical.top() * ratio), qRound(logical.width() * ratio), qRound(logical.height() * ratio)) & grab.rect();
    }

    // Device pixels past halfway from the surface to the ink, the threshold inkSpan() reads a column by
    static int inkCoverage(const QImage& grab, const QRect& logical, const QColor& behind, const QColor& ink)
    {
        const QRect scanned = inDevicePixels(grab, logical);
        const int reach = TestPixels::distance(behind, ink);
        int covered = 0;
        for (int y = scanned.top(); y <= scanned.bottom(); ++y) {
            for (int x = scanned.left(); x <= scanned.right(); ++x) {
                covered += 2 * TestPixels::distance(grab.pixelColor(x, y), behind) >= reach ? 1 : 0;
            }
        }
        return covered;
    }

    // Antialiasing leaves only a word's stems at full coverage, so the pixel furthest from the surface carries its ink
    static QColor strongestInk(const QImage& grab, const QRect& logical, const QColor& behind)
    {
        const QRect scanned = inDevicePixels(grab, logical);
        QColor strongest = behind;
        for (int y = scanned.top(); y <= scanned.bottom(); ++y) {
            for (int x = scanned.left(); x <= scanned.right(); ++x) {
                const QColor pixel = grab.pixelColor(x, y);
                if (TestPixels::distance(pixel, behind) > TestPixels::distance(strongest, behind)) {
                    strongest = pixel;
                }
            }
        }
        return strongest;
    }

    // The same word drawn here at one weight, on the same surface in the same ink, for the grab's to be held against
    static int coverageAt(const QFont& font, const QString& text, const QColor& ink, const QColor& behind, const qreal ratio)
    {
        const QFontMetrics metrics(font);
        QImage drawn(QSize(metrics.horizontalAdvance(text) + 8, metrics.height() + 4) * ratio, QImage::Format_ARGB32);
        drawn.setDevicePixelRatio(ratio);
        drawn.fill(behind);
        QPainter painter(&drawn);
        painter.setFont(font);
        painter.setPen(ink);
        painter.drawText(QPoint(4, 2 + metrics.ascent()), text);
        painter.end();
        return inkCoverage(drawn, QRect(QPoint(0, 0), drawn.deviceIndependentSize().toSize()), behind, ink);
    }

    static QFont drawnFont(const QTabBar* pBar, const int index)
    {
        QFont font = pBar->font();
        if (index == pBar->currentIndex()) {
            font.setWeight(uiDesign::scmTabChosenWeight);
        }
        return font;
    }

    static QColor wordInk(const QTabBar* pBar, const int index, const uiDesign::ThemeTokens& tokens) { return index == pBar->currentIndex() ? tokens.accentText : tokens.mutedText; }

    // How far in from the edge it is drawn from a word's ink starts: its first glyph's bearing left to right, its last
    // glyph's right to left
    static int leadingBearing(const Strip& strip, const int index)
    {
        const QFontMetricsF metrics(drawnFont(strip.pBar, index));
        const QString word = strip.pBar->tabText(index);
        return std::max(0, qRound(rightToLeft(strip) ? metrics.rightBearing(word.back()) : metrics.leftBearing(word.front())));
    }

    // A sheet's :hover is read off the tab's State_MouseOver, which QTabBar sets from hover events of its own; no
    // pointer sends those offscreen
    static void hoverOver(QTabBar* pBar, const int index)
    {
        const QPointF over = pBar->tabRect(index).center();
        QHoverEvent hover(QEvent::HoverMove, over, pBar->mapToGlobal(over), QPointF(-1, -1));
        QApplication::sendEvent(pBar, &hover);
        TestPixels::settle();
    }

    static void hoverAway(QTabBar* pBar)
    {
        QHoverEvent leave(QEvent::HoverLeave, QPointF(-1, -1), QPointF(-1, -1), QPointF(-1, -1));
        QApplication::sendEvent(pBar, &leave);
        TestPixels::settle();
    }

    void readStrip(const Strip& strip, const QString& appearance)
    {
        QTabBar* pBar = strip.pBar;
        QVERIFY2(pBar && pBar->isVisible(), qPrintable(qsl("%1: the strip is not on show").arg(strip.what)));
        QVERIFY2(pBar->count() >= 2, qPrintable(qsl("SETUP: %1 holds %2 tabs, so there is no unchosen one to read").arg(strip.what, QString::number(pBar->count()))));
        pBar->setCurrentIndex(0);
        TestPixels::settle();

        const bool rtl = rightToLeft(strip);
        // A word's ink stands its bearing in from the edge it is drawn from: rightward left to right, leftward right to left
        const int inward = rtl ? -1 : 1;
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QString where = qsl("%1, %2 appearance%3").arg(strip.what, appearance, rtl ? qsl(", right to left") : QString());
        QImage grab = TestPixels::shot(strip.pWindow);
        const int underlineRow = hairlineRow(strip);

        // The pixel reads below hold under Fusion whatever the strip's style answers, and the macOS style centres the
        // tabs and elides their words
        const int alignment = pBar->style()->styleHint(QStyle::SH_TabBar_Alignment, nullptr, pBar) & Qt::AlignHorizontal_Mask;
        QVERIFY2(alignment == Qt::AlignLeft,
                 qPrintable(qsl("%1: the strip's style lines the tabs up by %2 rather than from the leading edge (%3)").arg(where, QString::number(alignment), QString::number(Qt::AlignLeft))));
        const int elision = pBar->style()->styleHint(QStyle::SH_TabBar_ElideMode, nullptr, pBar);
        QVERIFY2(elision == Qt::ElideNone, qPrintable(qsl("%1: the strip's style cuts a word short that has no room (elide mode %2) rather than scrolling").arg(where, QString::number(elision))));

        // (a) the chosen word stands on the accent; nothing else does
        const auto [chosenFirst, chosenLast] = inkSpan(grab, strip, 0, wordInk(pBar, 0, tokens));
        QVERIFY2(chosenFirst >= 0, qPrintable(qsl("%1: the chosen tab's word left no ink to read").arg(where)));
        for (int row = underlineRow - uiDesign::scmTabUnderlineWidth + 1; row <= underlineRow; ++row) {
            for (int x = chosenFirst; x <= chosenLast; ++x) {
                const QColor read = TestPixels::sampled(grab, QPoint(x, row));
                QVERIFY2(TestPixels::readsAs(read, tokens.accent, scmInkTolerance),
                         qPrintable(qsl("%1: under the chosen word, at %2,%3, the strip reads %4 rather than the accent %5 - the chosen tab stands on no underline")
                                            .arg(where, QString::number(x), QString::number(row), read.name(), tokens.accent.name())));
            }
        }
        // Under the whole of the chosen tab's line, its cross too
        const QRect chosenLine = wordLine(strip, 0);
        for (int row = underlineRow - uiDesign::scmTabUnderlineWidth + 1; row <= underlineRow; ++row) {
            for (int x = chosenLine.left(); x <= chosenLine.right(); ++x) {
                const QColor read = TestPixels::sampled(grab, QPoint(x, row));
                QVERIFY2(TestPixels::readsAs(read, tokens.accent, scmInkTolerance),
                         qPrintable(qsl("%1: under the chosen tab's line, at %2,%3, the strip reads %4 rather than the accent %5 - the underline does not run under the tab's word and cross")
                                            .arg(where, QString::number(x), QString::number(row), read.name(), tokens.accent.name())));
            }
        }
        for (int index = 1; index < pBar->count(); ++index) {
            const QRect tab = inWindow(strip, pBar->tabRect(index));
            for (int row = underlineRow - uiDesign::scmTabUnderlineWidth + 1; row <= underlineRow; ++row) {
                for (int x = tab.left(); x <= tab.right(); ++x) {
                    const QColor read = TestPixels::sampled(grab, QPoint(x, row));
                    QVERIFY2(!TestPixels::readsAs(read, tokens.accent, scmInkTolerance),
                             qPrintable(qsl("%1: tab %2, which is not chosen, carries the accent at %3,%4 - it reads as chosen too")
                                                .arg(where, QString::number(index), QString::number(x), QString::number(row))));
                }
            }
        }

        // (b) the first word starts on the content's line
        const int bearing = leadingBearing(strip, 0);
        const int chosenLead = rtl ? chosenLast : chosenFirst;
        const int contentLead = rtl ? strip.contentRight : strip.contentLeft;
        qInfo().noquote() << qsl("  %1: the first word's leading ink is at %2 with a %3px bearing, the content's leading edge under the strip at %4")
                                     .arg(where, QString::number(chosenLead), QString::number(bearing), QString::number(contentLead));
        QVERIFY2(std::abs(chosenLead - inward * bearing - contentLead) <= scmBearingTolerance,
                 qPrintable(qsl("%1: the first word's leading ink is at %2 (a %3px bearing in), off the line the content under the strip starts on, %4")
                                    .arg(where, QString::number(chosenLead), QString::number(bearing), QString::number(contentLead))));

        // No word is cut short, chosen or not: its ink is as wide as the word's own ink drawn whole. And each is drawn at
        // its own weight: its ink covers nearer what the word covers drawn at that weight than at the other
        QFont chosenWeight = pBar->font();
        chosenWeight.setWeight(uiDesign::scmTabChosenWeight);
        const bool weightsDrawApart = QFontInfo(chosenWeight).weight() > QFontInfo(pBar->font()).weight();
        if (!weightsDrawApart) {
            qInfo().noquote() << qsl("  %1: this machine's font has no face heavier than its regular one at the chosen weight, so no word's weight is read off the grab").arg(where);
        }
        for (int index = 0; index < pBar->count(); ++index) {
            const QColor ink = wordInk(pBar, index, tokens);
            const auto [first, last] = inkSpan(grab, strip, index, ink);
            const int inked = first < 0 ? 0 : last - first + 1;
            const int whole = qRound(QFontMetricsF(drawnFont(pBar, index)).tightBoundingRect(pBar->tabText(index)).width());
            QVERIFY2(first >= 0 && inked + scmInkExtentSlack >= whole,
                     qPrintable(qsl("%1: tab %2's word inks %3px of the %4px \"%5\" inks drawn whole, so it is cut short")
                                        .arg(where, QString::number(index), QString::number(inked), QString::number(whole), pBar->tabText(index))));

            const QRect line = wordLine(strip, index);
            const QColor behind = commonestColour(grab, line);
            if (weightsDrawApart) {
                QRect inkedLine = line;
                inkedLine.setLeft(first);
                inkedLine.setRight(last);
                const int covered = inkCoverage(grab, inkedLine, behind, ink);
                const int atBarWeight = coverageAt(pBar->font(), pBar->tabText(index), ink, behind, grab.devicePixelRatio());
                const int atChosenWeight = coverageAt(chosenWeight, pBar->tabText(index), ink, behind, grab.devicePixelRatio());
                QVERIFY2(atChosenWeight > atBarWeight,
                         qPrintable(qsl("SETUP: \"%1\" covers no more drawn at the chosen weight than at the bar's, so its weight cannot be read").arg(pBar->tabText(index))));
                const bool readsChosen = std::abs(covered - atChosenWeight) < std::abs(covered - atBarWeight);
                QVERIFY2(readsChosen == (index == pBar->currentIndex()),
                         qPrintable(qsl("%1: tab %2's word covers %3 pixels, where \"%4\" covers %5 at the bar's weight and %6 at the chosen one - it is drawn at the %7 weight")
                                            .arg(where,
                                                 QString::number(index),
                                                 QString::number(covered),
                                                 pBar->tabText(index),
                                                 QString::number(atBarWeight),
                                                 QString::number(atChosenWeight),
                                                 readsChosen ? qsl("chosen") : qsl("bar's"))));
            }

            // Every word, chosen or not, starts on its tab's leading edge
            const int lead = rtl ? last : first;
            const int lineLead = rtl ? line.right() : line.left();
            const int wordBearing = leadingBearing(strip, index);
            QVERIFY2(std::abs(lead - inward * wordBearing - lineLead) <= scmBearingTolerance,
                     qPrintable(qsl("%1: tab %2's word has its leading ink at %3 (a %4px bearing in), off its tab's leading edge at %5")
                                        .arg(where, QString::number(index), QString::number(lead), QString::number(wordBearing), QString::number(lineLead))));

            // The gap trails the word: nothing is drawn in it, and no underline runs into it
            const QRect tab = inWindow(strip, pBar->tabRect(index));
            const QRect gap = rtl ? QRect(QPoint(tab.left(), line.top()), QPoint(line.left() - 1, line.bottom())) : QRect(QPoint(line.right() + 1, line.top()), QPoint(tab.right(), line.bottom()));
            QVERIFY2(gap.width() == uiDesign::scmTabGap, qPrintable(qsl("SETUP: %1: tab %2 leaves a %3px gap to read").arg(where, QString::number(index), QString::number(gap.width()))));
            const int inGap = inkCoverage(grab, gap, behind, ink);
            QVERIFY2(!inGap,
                     qPrintable(qsl("%1: %2 pixels of tab %3 are drawn in the %4px on its %5 side, where the gap after its word belongs")
                                        .arg(where, QString::number(inGap), QString::number(index), QString::number(uiDesign::scmTabGap), rtl ? qsl("left") : qsl("right"))));
            for (int row = underlineRow - uiDesign::scmTabUnderlineWidth + 1; row <= underlineRow; ++row) {
                for (int x = gap.left(); x <= gap.right(); ++x) {
                    const QColor read = TestPixels::sampled(grab, QPoint(x, row));
                    QVERIFY2(!TestPixels::readsAs(read, tokens.accent, scmInkTolerance),
                             qPrintable(qsl("%1: the underline runs on into the gap after tab %2's word, at %3,%4").arg(where, QString::number(index), QString::number(x), QString::number(row))));
                }
            }

            // A cross ends its tab's line, beside the word rather than over it, as far from where the word starts as
            // the chosen word is wide and the close gap after it
            if (QWidget* pCross = pBar->tabButton(index, QTabBar::RightSide)) {
                const QRect cross = inWindow(strip, pCross->geometry());
                const int offTheEnd = rtl ? cross.left() - line.left() : line.right() - cross.right();
                const int fromTheLead = rtl ? line.right() - cross.right() : cross.left() - line.left();
                const int wordBox = QFontMetrics(chosenWeight).size(Qt::TextShowMnemonic, pBar->tabText(index)).width();
                const int clear = rtl ? first - cross.right() - 1 : cross.left() - last - 1;
                qInfo().noquote() << qsl("  %1: tab %2's cross stands at %3, %4px from where the word starts and %5px clear of its ink")
                                             .arg(where, QString::number(index), TestPixels::describe(cross), QString::number(fromTheLead), QString::number(clear));
                QVERIFY2(!offTheEnd, qPrintable(qsl("%1: the cross on tab %2 stands %3px off the trailing end of the tab's line").arg(where, QString::number(index), QString::number(offTheEnd))));
                QVERIFY2(fromTheLead == wordBox + uiDesign::scmTabCloseGap,
                         qPrintable(qsl("%1: the cross on tab %2 stands %3px from where the word starts, rather than the %4px word and the %5px gap after it")
                                            .arg(where, QString::number(index), QString::number(fromTheLead), QString::number(wordBox), QString::number(uiDesign::scmTabCloseGap))));
                QVERIFY2(clear >= uiDesign::scmTabCloseGap - scmBearingTolerance,
                         qPrintable(qsl("%1: tab %2's word inks to %3px of its cross at %4, so it runs under the cross rather than stopping the gap before it")
                                            .arg(where, QString::number(index), QString::number(clear), TestPixels::describe(cross))));
            }
        }

        // (d) the hairline runs on past the last tab to the end of the content under the strip
        const QRect lastTab = inWindow(strip, pBar->tabRect(pBar->count() - 1));
        const int contentEnd = rtl ? strip.contentLeft : strip.contentRight;
        const int lastTabEnd = rtl ? lastTab.left() : lastTab.right();
        QVERIFY2(rtl ? contentEnd < lastTabEnd : contentEnd > lastTabEnd,
                 qPrintable(qsl("SETUP: %1's tabs reach %2, the end of the content, so nothing beyond them could be read").arg(where, QString::number(lastTabEnd))));
        const QColor lineEnd = TestPixels::sampled(grab, QPoint(contentEnd, underlineRow));
        QVERIFY2(TestPixels::readsAs(lineEnd, tokens.border, scmInkTolerance),
                 qPrintable(qsl("%1: at the end of the content, %2,%3, the strip's bottom row reads %4 rather than the hairline %5")
                                    .arg(where, QString::number(contentEnd), QString::number(underlineRow), lineEnd.name(), tokens.border.name())));
        const QColor overTheLine = TestPixels::sampled(grab, QPoint(contentEnd, underlineRow - 1));
        QVERIFY2(!TestPixels::readsAs(overTheLine, tokens.border, scmInkTolerance), qPrintable(qsl("%1: the row over the hairline is the hairline's colour too, so it is no hairline").arg(where)));

        // (c) choosing another tab moves nothing
        const QSize stripSize = pBar->size();
        QList<QRect> before;
        for (int index = 0; index < pBar->count(); ++index) {
            before << pBar->tabRect(index);
        }
        pBar->setCurrentIndex(1);
        TestPixels::settle();
        QVERIFY2(pBar->size() == stripSize,
                 qPrintable(qsl("%1: the strip was %2 and is %3 once another tab is chosen").arg(where, TestPixels::describe(stripSize), TestPixels::describe(pBar->size()))));
        for (int index = 0; index < pBar->count(); ++index) {
            QVERIFY2(pBar->tabRect(index) == before.at(index),
                     qPrintable(qsl("%1: tab %2 stood at %3 and moved to %4 when the choice moved, so the strip steps sideways for the semibold word")
                                        .arg(where, QString::number(index), TestPixels::describe(before.at(index)), TestPixels::describe(pBar->tabRect(index)))));
        }
        grab = TestPixels::shot(strip.pWindow);
        const auto [movedFirst, movedLast] = inkSpan(grab, strip, 1, wordInk(pBar, 1, tokens));
        QVERIFY2(movedFirst >= 0, qPrintable(qsl("%1: the newly chosen tab's word left no ink to read").arg(where)));
        const QColor underMoved = TestPixels::sampled(grab, QPoint((movedFirst + movedLast) / 2, underlineRow));
        QVERIFY2(TestPixels::readsAs(underMoved, tokens.accent, scmInkTolerance), qPrintable(qsl("%1: the underline did not follow the choice to tab 1: it reads %2").arg(where, underMoved.name())));
        pBar->setCurrentIndex(0);
        TestPixels::settle();

        // (e) under the pointer a word not chosen takes the accent's ink, and nothing is laid behind it
        const QRect hoveredTab = inWindow(strip, pBar->tabRect(1));
        const QRect hoveredLine = wordLineClearOfTheCross(strip, 1);
        const QImage resting = TestPixels::shot(strip.pWindow);
        hoverOver(pBar, 1);
        const QImage hovered = TestPixels::shot(strip.pWindow);
        hoverAway(pBar);
        const QColor restingBehind = commonestColour(resting, hoveredTab);
        const QColor hoveredBehind = commonestColour(hovered, hoveredTab);
        const QColor restingInk = strongestInk(resting, hoveredLine, restingBehind);
        const QColor hoveredInk = strongestInk(hovered, hoveredLine, hoveredBehind);
        qInfo().noquote() << qsl("  %1: tab 1 reads %2 on %3 at rest and %4 on %5 under the pointer").arg(where, restingInk.name(), restingBehind.name(), hoveredInk.name(), hoveredBehind.name());
        QVERIFY2(TestPixels::distance(restingInk, tokens.mutedText) < TestPixels::distance(restingInk, tokens.accentText),
                 qPrintable(qsl("SETUP: %1: tab 1's word already reads %2 with no pointer over it, nearer the accent's %3 than the quiet %4")
                                    .arg(where, restingInk.name(), tokens.accentText.name(), tokens.mutedText.name())));
        QVERIFY2(TestPixels::distance(hoveredInk, tokens.accentText) < TestPixels::distance(hoveredInk, tokens.mutedText),
                 qPrintable(qsl("%1: under the pointer tab 1's word is written in %2, nearer the quiet %3 than the accent's ink %4")
                                    .arg(where, hoveredInk.name(), tokens.mutedText.name(), tokens.accentText.name())));
        QVERIFY2(TestPixels::readsAs(hoveredBehind, restingBehind, scmInkTolerance),
                 qPrintable(qsl("%1: under the pointer tab 1 is washed in %2 where it stands on %3 at rest").arg(where, hoveredBehind.name(), restingBehind.name())));
    }

    void readInBothAppearances(const std::function<Strip()>& stripNow)
    {
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            const Strip strip = stripNow();
            TestPixels::setAppearanceAndWait(appearance.second, strip.pWindow);
            // Measured again: a restyle can move the content under the strip
            readStrip(stripNow(), appearance.first);
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }

    static int leftIn(const QWidget* pWidget, const QWidget* pWindow) { return pWidget->mapTo(pWindow, QPoint(0, 0)).x(); }
    static int rightIn(const QWidget* pWidget, const QWidget* pWindow) { return pWidget->mapTo(pWindow, QPoint(pWidget->width() - 1, 0)).x(); }

    Strip notepadStrip() const
    {
        const QWidget* pNote = mpNotepad->tabWidget->currentWidget();
        return Strip{qsl("the notepad"), mpNotepad->tabWidget->tabBar(), mpNotepad, leftIn(pNote, mpNotepad), rightIn(pNote, mpNotepad)};
    }

    Strip packagesStrip() const
    {
        return Strip{qsl("the package manager"),
                     mpPackages->findChild<QTabBar*>(qsl("packagesViewBar")),
                     mpPackages,
                     leftIn(mpPackages->packageList, mpPackages),
                     rightIn(mpPackages->packageList, mpPackages)};
    }

    Strip gamesStrip() const
    {
        return Strip{qsl("the connection window's games"),
                     mpConnection->findChild<QTabBar*>(qsl("gamesTabBar")),
                     mpConnection,
                     leftIn(mpConnection->listWidget_profiles, mpConnection),
                     rightIn(mpConnection->listWidget_profiles, mpConnection)};
    }

    // The first word stands over the form's first label, and the hairline runs to the far side of the page
    Strip formStrip() const
    {
        QTabWidget* pTabs = mpConnection->tabWidget_connectionInfo;
        const QWidget* pLabel = mpConnection->label_profileName;
        const QWidget* pPage = mpConnection->tab_connection_info;
        const bool rtl = pTabs->layoutDirection() == Qt::RightToLeft;
        return Strip{qsl("the connection window's form"),
                     pTabs->tabBar(),
                     mpConnection,
                     rtl ? leftIn(pPage, mpConnection) : leftIn(pLabel, mpConnection),
                     rtl ? rightIn(pLabel, mpConnection) : rightIn(pPage, mpConnection)};
    }

    QList<Strip> everyStrip() const { return {notepadStrip(), packagesStrip(), gamesStrip(), formStrip()}; }

    // theme-fixed: a script's sheet writes colours of its own, and these are what its tabs are read against
    static QColor scriptTabFill() { return QColor(0x33, 0x33, 0x33); }
    static QColor scriptTabInk() { return QColor(Qt::white); }

    static QString scriptTabSheet()
    {
        return qsl("QTabBar::tab { background: %1; color: %2; min-width: %3px; padding: 8px; }").arg(scriptTabFill().name(), scriptTabInk().name(), QString::number(scmScriptTabMinimumWidth));
    }

    // The script's sheet draws every tab whole, and nothing of the design's is left on the strip: no style of its own
    // on the bar, none of tabBarStyleSheet()'s rules in a sheet on the way to it, no underline, no hairline.
    // pSheetHolder is the widget the script's sheet is on - the window or one above it - or null for the application's.
    void readTheScriptsStrip(const Strip& strip, const QWidget* pSheetHolder)
    {
        QTabBar* pBar = strip.pBar;
        QVERIFY2(pBar && pBar->isVisible(), qPrintable(qsl("%1: the strip is not on show").arg(strip.what)));
        pBar->setCurrentIndex(0);
        TestPixels::settle();
        const QString holder = !pSheetHolder ? qsl("the application") : pSheetHolder == strip.pWindow ? qsl("the window") : qsl("%1, above the window").arg(pSheetHolder->objectName());
        const QString where = qsl("%1 under a script's sheet on %2").arg(strip.what, holder);
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

        QVERIFY2(!pBar->testAttribute(Qt::WA_SetStyle), qPrintable(qsl("%1: the bar still wears a style of its own, which the design set").arg(where)));
        for (const QWidget* pOnTheWay = pBar; pOnTheWay; pOnTheWay = pOnTheWay->parentWidget()) {
            if (pOnTheWay == pSheetHolder) {
                continue;
            }
            const QString sheet = pOnTheWay->styleSheet();
            QVERIFY2(!sheet.contains(qsl("QTabBar")) && !sheet.contains(qsl("::tab")) && !sheet.contains(qsl("::pane")),
                     qPrintable(qsl("%1: the sheet on %2 still carries the design's tab rules, and it reaches the bar").arg(where, pOnTheWay->objectName())));
        }

        // A crowded bar under Fusion scrolls, as a plain one does: the package manager's column holds one of the script's
        // tabs whole
        QVERIFY2(onShow(pBar, pBar->tabRect(0)), qPrintable(qsl("SETUP: %1 does not show its chosen tab whole, so it cannot be read").arg(where)));
        const QImage grab = TestPixels::shot(strip.pWindow);
        for (int index = 0; index < pBar->count(); ++index) {
            const QRect tab = inWindow(strip, pBar->tabRect(index));
            QVERIFY2(tab.width() >= scmScriptTabMinimumWidth,
                     qPrintable(qsl("%1: tab %2 is %3px wide, short of the %4px the script's sheet asks for")
                                        .arg(where, QString::number(index), QString::number(tab.width()), QString::number(scmScriptTabMinimumWidth))));
            if (!onShow(pBar, pBar->tabRect(index))) {
                qInfo().noquote() << qsl("  %1: tab %2 is scrolled out of sight, so its face is not read").arg(where, QString::number(index));
                continue;
            }
            const QColor fill = commonestColour(grab, tab);
            QVERIFY2(TestPixels::readsAs(fill, scriptTabFill(), scmInkTolerance),
                     qPrintable(qsl("%1: tab %2 is filled %3 rather than the script's %4").arg(where, QString::number(index), fill.name(), scriptTabFill().name())));
            const QColor ink = strongestInk(grab, tab, fill);
            QVERIFY2(TestPixels::readsAs(ink, scriptTabInk(), scmInkTolerance),
                     qPrintable(qsl("%1: tab %2's word is written in %3 rather than the script's %4").arg(where, QString::number(index), ink.name(), scriptTabInk().name())));

            // Qt's own cross: the size the application style gives it, where Qt lays it at the tab's trailing end rather
            // than beside the word, a gap short of it
            auto* pCross = qobject_cast<QAbstractButton*>(pBar->tabButton(index, QTabBar::RightSide));
            if (!pCross) {
                continue;
            }
            QVERIFY2(pCross->minimumSize() != pCross->maximumSize() && pCross->size() == pCross->sizeHint(),
                     qPrintable(qsl("%1: tab %2's cross is held at %3 where the style's own is %4")
                                        .arg(where, QString::number(index), TestPixels::describe(pCross->size()), TestPixels::describe(pCross->sizeHint()))));
            const QRect cross = inWindow(strip, pCross->geometry());
            const int shortOfTheEnd = rightToLeft(strip) ? cross.left() - tab.left() : tab.right() - cross.right();
            QVERIFY2(
                    shortOfTheEnd < uiDesign::scmTabGap,
                    qPrintable(qsl("%1: tab %2's cross stands %3px short of the tab's end, beside the word as the design puts it").arg(where, QString::number(index), QString::number(shortOfTheEnd))));
        }

        // Down to the bar's bottom row, where the design lays the underline
        QRect chosen = inWindow(strip, pBar->tabRect(0));
        chosen.setBottom(std::max(chosen.bottom(), hairlineRow(strip)));
        for (int row = chosen.top(); row <= chosen.bottom(); ++row) {
            for (int x = chosen.left(); x <= chosen.right(); ++x) {
                const QColor read = TestPixels::sampled(grab, QPoint(x, row));
                QVERIFY2(!TestPixels::readsAs(read, tokens.accent, scmInkTolerance),
                         qPrintable(qsl("%1: the chosen tab carries the design's accent at %2,%3").arg(where, QString::number(x), QString::number(row))));
            }
        }

        // Past the last tab on show, over the content's own edge, on the bar's last two rows: the design draws its hairline
        // on the bottom one. The row under the bar is the content's own, where a note's field starts with its frame.
        const bool rtl = rightToLeft(strip);
        int lastOnShow = pBar->count() - 1;
        while (lastOnShow > 0 && !onShow(pBar, pBar->tabRect(lastOnShow))) {
            --lastOnShow;
        }
        const QRect lastTab = inWindow(strip, pBar->tabRect(lastOnShow));
        const int from = rtl ? strip.contentLeft : lastTab.right() + 1;
        const int to = rtl ? lastTab.left() - 1 : strip.contentRight;
        QVERIFY2(to > from, qPrintable(qsl("SETUP: %1's tabs reach the end of the content, so nothing beyond them could be read").arg(where)));
        for (int row = hairlineRow(strip) - 1; row <= hairlineRow(strip); ++row) {
            int border = 0;
            for (int x = from; x <= to; ++x) {
                border += TestPixels::readsAs(TestPixels::sampled(grab, QPoint(x, row)), tokens.border, scmInkTolerance) ? 1 : 0;
            }
            qInfo().noquote() << qsl("  %1: row %2 reads the design's hairline in %3 of the %4 pixels past the last tab")
                                         .arg(where, QString::number(row), QString::number(border), QString::number(to - from + 1));
            QVERIFY2(2 * border < to - from + 1,
                     qPrintable(qsl("%1: row %2 runs the design's hairline %3 across the content past the last tab").arg(where, QString::number(row), tokens.border.name())));
        }
    }

    void readEveryStripAsTheScripts(const QWidget* pSheetHolder = nullptr)
    {
        for (const Strip& strip : everyStrip()) {
            readTheScriptsStrip(strip, pSheetHolder);
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }

    void readEveryStripAsTheDesigns()
    {
        for (const Strip& strip : everyStrip()) {
            readStrip(strip, qsl("the current"));
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }

    // The + stands on the strip's line at its far end, its box ending where the hairline and the note do
    void readTheAddButton()
    {
        auto* pAdd = mpNotepad->findChild<QToolButton*>(qsl("notepadAddTab"));
        QVERIFY(pAdd);
        const Strip strip = notepadStrip();
        const bool rtl = rightToLeft(strip);
        const QString where = rtl ? qsl(", right to left") : QString();
        auto* pNote = mpNotepad->tabWidget->currentWidget();
        const QRect add(pAdd->mapTo(mpNotepad, QPoint(0, 0)), pAdd->size());
        const int noteEnd = rtl ? leftIn(pNote, mpNotepad) : rightIn(pNote, mpNotepad);
        qInfo().noquote()
                << qsl("  the + stands at %1 over the hairline on row %2, the note ends at %3%4").arg(TestPixels::describe(add), QString::number(hairlineRow(strip)), QString::number(noteEnd), where);
        QVERIFY2(add.bottom() + 1 == hairlineRow(strip),
                 qPrintable(qsl("the + ends on row %1 rather than standing on the hairline on row %2%3").arg(QString::number(add.bottom()), QString::number(hairlineRow(strip)), where)));
        const QRect lastTab = inWindow(strip, strip.pBar->tabRect(strip.pBar->count() - 1));
        QVERIFY2(rtl ? add.right() < lastTab.left() : add.left() > lastTab.right(),
                 qPrintable(qsl("the + at %1 does not stand past the last tab, %2%3").arg(TestPixels::describe(add), TestPixels::describe(lastTab), where)));
        // Its sheet's margin is the box it draws in, which is what has to end with the line
        const int boxEnd = rtl ? add.left() + uiDesign::scmTabPaneInset : add.right() - uiDesign::scmTabPaneInset;
        QVERIFY2(boxEnd == noteEnd, qPrintable(qsl("the +'s box ends at %1 rather than where the hairline and the note do, %2%3").arg(QString::number(boxEnd), QString::number(noteEnd), where)));

        // Under the pointer the box is washed, which shows where the margin left it; a stylesheet's :hover is read
        // off WA_UnderMouse, which no pointer sets offscreen
        pAdd->setAttribute(Qt::WA_UnderMouse, true);
        const QImage grab = TestPixels::shot(mpNotepad);
        const QColor outside = TestPixels::sampled(grab, QPoint(rtl ? add.left() + 1 : add.right() - 1, add.center().y()));
        const QColor inside = TestPixels::sampled(grab, QPoint(rtl ? boxEnd + 1 : boxEnd - 1, add.center().y()));
        pAdd->setAttribute(Qt::WA_UnderMouse, false);
        qInfo().noquote() << qsl("  under the pointer the + reads %1 inside its box and %2 in its margin").arg(inside.name(), outside.name());
        QVERIFY2(inside != outside, qPrintable(qsl("the +'s hover wash reaches into its margin (%1 both inside and out), so the box is not where its sheet says%2").arg(inside.name(), where)));
    }

    // The spacing under the games strip keeps the list's frame off the hairline
    void readTheGamesListsSpacing()
    {
        const Strip strip = gamesStrip();
        const int listTop = mpConnection->listWidget_profiles->mapTo(mpConnection, QPoint(0, 0)).y();
        const int gap = listTop - (hairlineRow(strip) + 1);
        QVERIFY2(gap == uiDesign::scmTabPaneInset,
                 qPrintable(qsl("the games list starts %1px under the hairline on row %2 rather than the strip's inset, %3")
                                    .arg(QString::number(gap), QString::number(hairlineRow(strip)), QString::number(uiDesign::scmTabPaneInset))));
    }

    // With no inset the form's strip starts on the tab widget's own edge, the line the box under the form stands on
    void readTheFormStripsInset()
    {
        QTabWidget* pTabs = mpConnection->tabWidget_connectionInfo;
        const QRect bar = pTabs->tabBar()->geometry();
        const bool rtl = pTabs->layoutDirection() == Qt::RightToLeft;
        const int inset = rtl ? pTabs->width() - 1 - bar.right() : bar.left();
        QVERIFY2(!inset, qPrintable(qsl("the form's strip stands %1px in from the tab widget's %2 edge, where it starts with no inset").arg(QString::number(inset), rtl ? qsl("right") : qsl("left"))));
    }

    // Given its size hint the tab widget lays its pages out at theirs, with no band under them and nothing cut short,
    // and its least size is the pages' and the strip's least in the same pane. The pane is the inset at the sides and
    // the foot and, under the strip, the hairline with the inset below it; the hairline is lifted onto the strip's
    // bottom row, so it adds no row of its own.
    void readTheTabWidgetsSize(QTabWidget* pTabs, const int inset, const QString& what)
    {
        const QString where = what + (pTabs->layoutDirection() == Qt::RightToLeft ? qsl(", right to left") : QString());
        auto* pStack = pTabs->findChild<QStackedWidget*>(QString(), Qt::FindDirectChildrenOnly);
        QVERIFY2(pStack && pTabs->isVisible(), qPrintable(qsl("SETUP: %1 is not on show with its pages").arg(where)));
        const QTabBar* pBar = pTabs->tabBar();

        QSize pages(0, 0);
        for (int i = 0; i < pTabs->count(); ++i) {
            if (pTabs->isTabVisible(i)) {
                pages = pages.expandedTo(pTabs->widget(i)->sizeHint());
            }
        }
        int cornersWide = 0;
        int leastCornersWide = 0;
        int cornersTall = 0;
        for (const Qt::Corner corner : {Qt::TopLeftCorner, Qt::TopRightCorner}) {
            const QWidget* pCorner = pTabs->cornerWidget(corner);
            if (pCorner && pCorner->isVisible()) {
                cornersWide += pCorner->sizeHint().width();
                leastCornersWide += pCorner->minimumSizeHint().width();
                cornersTall = std::max({cornersTall, pCorner->sizeHint().height(), pCorner->minimumSizeHint().height()});
            }
        }
        const int strip = pBar->sizeHint().height();
        QVERIFY2(cornersTall <= strip && pBar->minimumSizeHint().height() == strip,
                 qPrintable(qsl("SETUP: %1: a corner widget %2px tall or a strip %3px tall at least stands beside the %4px strip, so the pages' share of the height cannot be told")
                                    .arg(where, QString::number(cornersTall), QString::number(pBar->minimumSizeHint().height()), QString::number(strip))));
        // QTabWidget counts a strip that scrolls as 200px wide at most
        const int stripWide = pTabs->usesScrollButtons() ? std::min(pBar->sizeHint().width(), 200) : pBar->sizeHint().width();
        QVERIFY2(pages.width() >= stripWide + cornersWide,
                 qPrintable(qsl("SETUP: %1: the strip and its corner count as %2px, more than the pages' %3px, so the hint's width is not the pages'")
                                    .arg(where, QString::number(stripWide + cornersWide), QString::number(pages.width()))));

        const QSize wanted(pages.width() + 2 * inset, pages.height() + strip + 2 * inset);
        const QSize leastPages = pStack->minimumSizeHint();
        const QSize wantedLeast(std::max(leastPages.width(), pBar->minimumSizeHint().width() + leastCornersWide) + 2 * inset, leastPages.height() + strip + 2 * inset);
        qInfo().noquote() << qsl("  %1: pages hinting %2 (at least %3) under a %4px strip in a pane inset %5px need %6 (at least %7); the tab widget's hint is %8 (at least %9)")
                                     .arg(where,
                                          TestPixels::describe(pages),
                                          TestPixels::describe(leastPages),
                                          QString::number(strip),
                                          QString::number(inset),
                                          TestPixels::describe(wanted),
                                          TestPixels::describe(wantedLeast),
                                          TestPixels::describe(pTabs->sizeHint()),
                                          TestPixels::describe(pTabs->minimumSizeHint()));
        QVERIFY2(pTabs->sizeHint() == wanted,
                 qPrintable(qsl("%1: the tab widget's size hint is %2 where its pages, its strip and the pane round them need %3")
                                    .arg(where, TestPixels::describe(pTabs->sizeHint()), TestPixels::describe(wanted))));
        QVERIFY2(pTabs->minimumSizeHint() == wantedLeast,
                 qPrintable(qsl("%1: the tab widget's minimum size hint is %2 where its pages' least, its strip's and the pane round them need %3")
                                    .arg(where, TestPixels::describe(pTabs->minimumSizeHint()), TestPixels::describe(wantedLeast))));

        // A tab widget on show places its pages from its own resize event, so they are measured before the window's
        // layout gives the size back
        const QSize before = pTabs->size();
        pTabs->resize(pTabs->sizeHint());
        const QSize laidOut = pStack->size();
        const QSize current = pTabs->currentWidget()->size();
        pTabs->resize(before);
        QVERIFY2(laidOut == pages && current == pages,
                 qPrintable(qsl("%1: at its %2 size hint the tab widget lays its pages out at %3, the current one at %4, rather than at their own %5")
                                    .arg(where, TestPixels::describe(pTabs->sizeHint()), TestPixels::describe(laidOut), TestPixels::describe(current), TestPixels::describe(pages))));
    }

    // As a dialog sets up its own: the strip prepared before the sheet goes on, and no sheet while a script's takes the
    // tabs
    static bool designTheTabWidget(QTabWidget* pTabs, const int inset)
    {
        const bool designed = uiDesign::prepareTabStrip(pTabs->tabBar());
        pTabs->setStyleSheet(designed ? uiDesign::tabBarStyleSheet(qsl("QTabWidget#%1").arg(pTabs->objectName()), uiDesign::themeTokens(), inset, pTabs->layoutDirection()) : QString());
        return designed;
    }

    // Neither page is the larger both ways, so each side of the tab widget's size hint comes from a different page
    static QTabWidget* tabWidgetWithKnownPages(QWidget* pHost, const QString& name, const bool corner)
    {
        auto* pTabs = new QTabWidget(pHost);
        pTabs->setObjectName(name);
        pTabs->addTab(new SizedWidget(QSize(320, 140), QSize(220, 90)), qsl("First"));
        pTabs->addTab(new SizedWidget(QSize(260, 180), QSize(180, 120)), qsl("Second"));
        if (corner) {
            pTabs->setCornerWidget(new SizedWidget(QSize(24, 18), QSize(24, 18)), Qt::TopRightCorner);
        }
        return pTabs;
    }

    // Set up as dlgPackageManager sets itself up: the tab rules on a container inside the window only while
    // prepareTabStrip() answers true, and decided again on a StyleChange that tabStripOutOfDate() answers true for
    class RecipeDialog : public QDialog
    {
    public:
        explicit RecipeDialog(QWidget* pParent)
        : QDialog(pParent)
        , mpBody(new QWidget(this))
        , mpTabs(tabWidgetWithKnownPages(mpBody, qsl("recipeTabs"), false))
        {
            setObjectName(qsl("recipeDialog"));
            mpBody->setObjectName(qsl("recipeBody"));
            auto* pBodyLayout = new QVBoxLayout(mpBody);
            pBodyLayout->addWidget(mpTabs);
            auto* pLayout = new QVBoxLayout(this);
            pLayout->setContentsMargins(0, 0, 0, 0);
            pLayout->addWidget(mpBody);
            restyle();
        }

        QTabWidget* tabs() const { return mpTabs; }
        bool designed() const { return mDesigned; }
        int styleChanges() const { return mStyleChanges; }

    protected:
        void changeEvent(QEvent* pEvent) override
        {
            QDialog::changeEvent(pEvent);
            if (pEvent->type() == QEvent::StyleChange) {
                ++mStyleChanges;
                if (uiDesign::tabStripOutOfDate(mpTabs->tabBar())) {
                    restyle();
                }
            }
        }

    private:
        void restyle()
        {
            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            mDesigned = uiDesign::prepareTabStrip(mpTabs->tabBar());
            mpBody->setStyleSheet(qsl("QWidget#recipeBody { background-color: %1; }").arg(tokens.page.name())
                                  + (mDesigned ? uiDesign::tabBarStyleSheet(qsl("#recipeTabs"), tokens, uiDesign::scmTabPaneInset, mpTabs->layoutDirection()) : QString()));
        }

        QWidget* mpBody;
        QTabWidget* mpTabs;
        bool mDesigned = false;
        int mStyleChanges = 0;
    };

    // A dialog under a holder under a grandparent, as one made under the mapper stands under the widgets
    // setProfileStyleSheet() gives the profile's sheet. All three go in cleanup(), so no sheet on them outlives a test.
    QWidget* mpGrandparent = nullptr;
    QWidget* mpHolder = nullptr;
    RecipeDialog* mpRecipeDialog = nullptr;

    void makeTheDialogsHolders()
    {
        mpGrandparent = new QWidget;
        mpGrandparent->setObjectName(qsl("grandparent"));
        mpHolder = new QWidget(mpGrandparent);
        mpHolder->setObjectName(qsl("holder"));
    }

    // Neither holder is shown: a dialog is a window of its own, which needs no parent on show
    void openTheRecipeDialog()
    {
        mpRecipeDialog = new RecipeDialog(mpHolder);
        mpRecipeDialog->resize(600, 400);
        mpRecipeDialog->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpRecipeDialog));
        TestPixels::settle();
    }

    Strip recipeDialogStrip() const
    {
        const QWidget* pPage = mpRecipeDialog->tabs()->currentWidget();
        return Strip{qsl("a dialog on the recipe"), mpRecipeDialog->tabs()->tabBar(), mpRecipeDialog, leftIn(pPage, mpRecipeDialog), rightIn(pPage, mpRecipeDialog)};
    }

    void openTheNotepad(const int notes)
    {
        mudlet::self()->slot_notes();
        mpNotepad = HostDialogs::of(mpHost).mpNotePad;
        QVERIFY2(mpNotepad, "the notepad did not open at all");
        mpNotepad->resize(760, 520);
        mpNotepad->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpNotepad));
        auto* pAdd = mpNotepad->findChild<QToolButton*>(qsl("notepadAddTab"));
        QVERIFY2(pAdd, "the notepad has no 'notepadAddTab' to make another note with");
        for (int tries = 0; mpNotepad->tabWidget->count() < notes && tries < notes; ++tries) {
            pAdd->click();
        }
        QCOMPARE(mpNotepad->tabWidget->count(), notes);
    }

    void openThePackageManager()
    {
        mpPackages = new dlgPackageManager(nullptr, mpHost);
        mpPackages->resize(900, 600);
        mpPackages->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPackages));
    }

    // A profile is chosen, so the form rather than the first-launch invitation is on show
    void openTheConnectionWindow()
    {
        mpConnection = new dlgConnectionProfiles(mudlet::self());
        mpConnection->fillout_form();
        const auto items = mpConnection->findData(*mpConnection->listWidget_profiles, mProfileName, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "the profile is not in the connection window's games list");
        mpConnection->listWidget_profiles->setCurrentItem(items.first());
        mpConnection->resize(880, 560);
        mpConnection->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpConnection));
    }

    void closeTheWindows()
    {
        if (mpConnection) {
            mpConnection->close();
            delete mpConnection;
            mpConnection = nullptr;
        }
        if (mpPackages) {
            mpPackages->close();
            delete mpPackages;
            mpPackages = nullptr;
        }
        // Saves the notes, and HostDialogs lets go of it
        delete mpNotepad;
        mpNotepad = nullptr;
    }

    // Made afresh rather than flipped, as a window made under a right to left language is
    void reopenTheWindows()
    {
        closeTheWindows();
        openTheNotepad(3);
        if (QTest::currentTestFailed()) {
            return;
        }
        openThePackageManager();
        if (QTest::currentTestFailed()) {
            return;
        }
        openTheConnectionWindow();
        TestPixels::settle();
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));
        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        openTheNotepad(2);
        if (QTest::currentTestFailed()) {
            return;
        }
        openThePackageManager();
        if (QTest::currentTestFailed()) {
            return;
        }
        openTheConnectionWindow();
        TestPixels::settle();
    }

    void cleanupTestCase()
    {
        if (mpConnection) {
            mpConnection->close();
            delete mpConnection;
            mpConnection = nullptr;
        }
        if (mpPackages) {
            mpPackages->close();
            delete mpPackages;
            mpPackages = nullptr;
        }
        mpNotepad = nullptr;
        mpHost = nullptr;
        if (mudlet::self()) {
            mudlet::self()->setAppearance(enums::Appearance::systemSetting);
        }
        mSession.stop();
    }

    // A test that stops part way leaves its script's sheet set, which would take the next test's strips
    void cleanup()
    {
        qApp->setStyleSheet(QString());
        if (mpConnection) {
            mpConnection->setStyleSheet(QString());
        }
        if (mpHost && !mpHost->mProfileStyleSheet.isEmpty()) {
            mpHost->setProfileStyleSheet(QString());
        }
        delete mpGrandparent;
        mpGrandparent = nullptr;
        mpHolder = nullptr;
        mpRecipeDialog = nullptr;
        TestPixels::settle();
    }

    void test_theNotepadsNotes()
    {
        readInBothAppearances([this]() {
            return notepadStrip();
        });
        if (QTest::currentTestFailed()) {
            return;
        }
        readTheAddButton();
    }

    // From the keyboard the chosen tab's underline thickens up into the air under the word, and nothing else moves;
    // a click gives the bar no focus, as it takes focus by Tab alone
    void test_theKeyboardThickensTheUnderline()
    {
        QTabBar* pBar = mpNotepad->tabWidget->tabBar();
        const Strip strip{qsl("the notepad"), pBar, mpNotepad, 0, 0};
        QVERIFY2(pBar->focusPolicy() == Qt::TabFocus, "the notepad's strip takes the focus some other way than by Tab, so a click could leave the thick underline behind");
        pBar->setCurrentIndex(0);
        mpNotepad->activateWindow();
        if (!QTest::qWaitForWindowActive(mpNotepad)) {
            QSKIP("the notepad cannot be made the active window here, so no widget in it can hold the focus");
        }
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const int underlineRow = hairlineRow(strip);
        mpNotepad->tabWidget->currentWidget()->setFocus(Qt::OtherFocusReason);
        TestPixels::settle();
        QVERIFY2(!pBar->hasFocus(), "SETUP: the strip still holds the focus with the note given it");
        const QImage resting = TestPixels::shot(mpNotepad);
        const auto [restingFirst, restingLast] = inkSpan(resting, strip, 0, tokens.accentText);
        const QRect restingTab = pBar->tabRect(0);

        pBar->setFocus(Qt::TabFocusReason);
        TestPixels::settle();
        QVERIFY2(pBar->hasFocus(), "SETUP: the strip did not take the focus it was given");
        const QImage focused = TestPixels::shot(mpNotepad);
        const auto [focusedFirst, focusedLast] = inkSpan(focused, strip, 0, tokens.accentText);
        QVERIFY2(focusedFirst == restingFirst && focusedLast == restingLast && pBar->tabRect(0) == restingTab, "the chosen word or its tab moved when the strip took the focus");
        for (int row = underlineRow - uiDesign::scmTabFocusUnderlineWidth + 1; row <= underlineRow; ++row) {
            const QColor read = TestPixels::sampled(focused, QPoint((focusedFirst + focusedLast) / 2, row));
            QVERIFY2(TestPixels::readsAs(read, tokens.accent, scmInkTolerance),
                     qPrintable(qsl("from the keyboard the underline is not %1px deep: row %2 reads %3").arg(QString::number(uiDesign::scmTabFocusUnderlineWidth), QString::number(row), read.name())));
        }
        const QColor overIt = TestPixels::sampled(focused, QPoint((focusedFirst + focusedLast) / 2, underlineRow - uiDesign::scmTabFocusUnderlineWidth));
        QVERIFY2(!TestPixels::readsAs(overIt, tokens.accent, scmInkTolerance), "from the keyboard the underline grows deeper than the design's four pixels");
        const QColor restingRow = TestPixels::sampled(resting, QPoint((restingFirst + restingLast) / 2, underlineRow - uiDesign::scmTabUnderlineWidth));
        QVERIFY2(!TestPixels::readsAs(restingRow, tokens.accent, scmInkTolerance), "the underline is already thicker than two pixels with no focus on the strip");

        pBar->clearFocus();
        QTest::mouseClick(pBar, Qt::LeftButton, Qt::NoModifier, pBar->tabRect(1).center());
        TestPixels::settle();
        QVERIFY2(pBar->currentIndex() == 1 && !pBar->hasFocus(), "a click on a tab chose it and left the strip holding the focus");
        pBar->setCurrentIndex(0);
        TestPixels::settle();
    }

    void test_thePackageManagersViews()
    {
        readInBothAppearances([this]() {
            return packagesStrip();
        });
        QVERIFY2(leftIn(mpPackages->lineEdit_searchBar, mpPackages) == leftIn(mpPackages->packageList, mpPackages), "the search field and the list under the strip do not start on one line");
    }

    void test_theConnectionWindowsGames()
    {
        readInBothAppearances([this]() {
            return gamesStrip();
        });
        if (QTest::currentTestFailed()) {
            return;
        }
        readTheGamesListsSpacing();
    }

    void test_theConnectionWindowsForm()
    {
        readInBothAppearances([this]() {
            return formStrip();
        });
        if (QTest::currentTestFailed()) {
            return;
        }
        readTheFormStripsInset();
    }

    // Either window can size itself from its tab widget
    void test_eachWindowsTabWidgetHintsWhatItsPagesNeed()
    {
        readTheTabWidgetsSize(mpNotepad->tabWidget, uiDesign::scmTabPaneInset, qsl("the notepad's tab widget"));
        if (QTest::currentTestFailed()) {
            return;
        }
        readTheTabWidgetsSize(mpConnection->tabWidget_connectionInfo, 0, qsl("the connection window's form"));
    }

    // As a dialog on the recipe gets it: with no inset or the default, with a corner widget or none, either way round
    void test_aTabWidgetOnTheRecipeHintsWhatItsPagesNeed()
    {
        for (const Qt::LayoutDirection direction : {Qt::LeftToRight, Qt::RightToLeft}) {
            for (const int inset : {0, uiDesign::scmTabPaneInset}) {
                for (const bool corner : {false, true}) {
                    QWidget host;
                    host.setLayoutDirection(direction);
                    host.resize(800, 600);
                    QTabWidget* pTabs = tabWidgetWithKnownPages(&host, qsl("recipeTabs"), corner);
                    QVERIFY2(designTheTabWidget(pTabs, inset), "SETUP: a script's sheet takes the tabs with none set");
                    host.show();
                    QVERIFY(QTest::qWaitForWindowExposed(&host));
                    TestPixels::settle();
                    readTheTabWidgetsSize(pTabs, inset, qsl("a tab widget on the recipe inset %1px with %2").arg(QString::number(inset), corner ? qsl("a corner widget") : qsl("no corner widget")));
                    if (QTest::currentTestFailed()) {
                        return;
                    }
                }
            }
        }
    }

    // While a script's sheet takes the tabs the tab widget's size hints are those of one the design never touched, and
    // what its pages need again once the sheet goes
    void test_aScriptSheetLeavesTheTabWidgetQtsOwnSize()
    {
        QWidget host;
        host.resize(800, 800);
        QTabWidget* pDesigned = tabWidgetWithKnownPages(&host, qsl("designedTabs"), true);
        QTabWidget* pPlain = tabWidgetWithKnownPages(&host, qsl("plainTabs"), true);
        pPlain->move(0, 400);
        QVERIFY2(designTheTabWidget(pDesigned, uiDesign::scmTabPaneInset), "SETUP: a script's sheet takes the tabs with none set");
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));

        qApp->setStyleSheet(scriptTabSheet());
        TestPixels::settle();
        QVERIFY2(!designTheTabWidget(pDesigned, uiDesign::scmTabPaneInset), "SETUP: the design keeps the tabs under a script's sheet that names QTabBar");
        TestPixels::settle();
        qInfo().noquote() << qsl("  under a script's sheet the tab widget's size hint is %1 (at least %2), one the design never touched has %3 (at least %4)")
                                     .arg(TestPixels::describe(pDesigned->sizeHint()),
                                          TestPixels::describe(pDesigned->minimumSizeHint()),
                                          TestPixels::describe(pPlain->sizeHint()),
                                          TestPixels::describe(pPlain->minimumSizeHint()));
        QVERIFY2(pDesigned->sizeHint() == pPlain->sizeHint() && pDesigned->minimumSizeHint() == pPlain->minimumSizeHint(),
                 qPrintable(qsl("under a script's sheet the tab widget's size hint is %1 (at least %2) where one the design never touched has %3 (at least %4)")
                                    .arg(TestPixels::describe(pDesigned->sizeHint()),
                                         TestPixels::describe(pDesigned->minimumSizeHint()),
                                         TestPixels::describe(pPlain->sizeHint()),
                                         TestPixels::describe(pPlain->minimumSizeHint()))));

        qApp->setStyleSheet(QString());
        TestPixels::settle();
        QVERIFY2(designTheTabWidget(pDesigned, uiDesign::scmTabPaneInset), "SETUP: the design does not take the tabs back once the script's sheet goes");
        TestPixels::settle();
        readTheTabWidgetsSize(pDesigned, uiDesign::scmTabPaneInset, qsl("a tab widget on the recipe once the script's sheet went"));
    }

    // setAppStyleSheet() while the windows are open: the script takes every strip, and the design comes back when the
    // sheet goes
    void test_anApplicationSheetNamingTheTabBarTakesEveryStrip()
    {
        qApp->setStyleSheet(scriptTabSheet());
        TestPixels::settle();
        readEveryStripAsTheScripts();
        if (QTest::currentTestFailed()) {
            return;
        }
        qApp->setStyleSheet(QString());
        TestPixels::settle();
        readEveryStripAsTheDesigns();
    }

    // Windows made while the sheet is set start out as the script's, and the strips change hands again and again
    // without the windows being made afresh
    void test_aWindowMadeUnderTheSheetStartsAsTheScripts()
    {
        qApp->setStyleSheet(scriptTabSheet());
        reopenTheWindows();
        if (QTest::currentTestFailed()) {
            return;
        }
        readEveryStripAsTheScripts();
        if (QTest::currentTestFailed()) {
            return;
        }
        for (int round = 0; round < 2; ++round) {
            qApp->setStyleSheet(QString());
            TestPixels::settle();
            readEveryStripAsTheDesigns();
            if (QTest::currentTestFailed()) {
                return;
            }
            qApp->setStyleSheet(scriptTabSheet());
            TestPixels::settle();
            readEveryStripAsTheScripts();
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }

    // A sheet about something else leaves the design in place, and one changed to name QTabBar takes the strips
    void test_aSheetNamingNoTabBarLeavesTheDesign()
    {
        // theme-fixed: the script's own colour for its buttons, which no strip may take as a reason to step aside
        qApp->setStyleSheet(qsl("QPushButton { color: %1; }").arg(QColor(Qt::red).name()));
        TestPixels::settle();
        readEveryStripAsTheDesigns();
        if (QTest::currentTestFailed()) {
            return;
        }
        qApp->setStyleSheet(scriptTabSheet());
        TestPixels::settle();
        readEveryStripAsTheScripts();
    }

    // As a profile's sheet would be: on the dialog itself, which reaches no other window
    void test_aSheetOnTheConnectionWindowTakesItsStrips()
    {
        mpConnection->setStyleSheet(scriptTabSheet());
        TestPixels::settle();
        readTheScriptsStrip(gamesStrip(), mpConnection);
        if (QTest::currentTestFailed()) {
            return;
        }
        readTheScriptsStrip(formStrip(), mpConnection);
        if (QTest::currentTestFailed()) {
            return;
        }
        readStrip(notepadStrip(), qsl("the current"));
        if (QTest::currentTestFailed()) {
            return;
        }
        readStrip(packagesStrip(), qsl("the current"));
        if (QTest::currentTestFailed()) {
            return;
        }

        mpConnection->setStyleSheet(QString());
        TestPixels::settle();
        readStrip(gamesStrip(), qsl("the current"));
        if (QTest::currentTestFailed()) {
            return;
        }
        readStrip(formStrip(), qsl("the current"));
    }

    // setProfileStyleSheet() sets the profile's sheet on the notepad's window
    void test_aProfileSheetTakesTheNotepadsStrip()
    {
        mpHost->setProfileStyleSheet(scriptTabSheet());
        TestPixels::settle();
        QVERIFY2(mpNotepad->styleSheet() == scriptTabSheet(), "SETUP: the profile's sheet did not reach the notepad's window");
        readTheScriptsStrip(notepadStrip(), mpNotepad);
        if (QTest::currentTestFailed()) {
            return;
        }
        mpHost->setProfileStyleSheet(QString());
        TestPixels::settle();
        readStrip(notepadStrip(), qsl("the current"));
    }

    // Qt carries a parent's sheet into a dialog, across the window's edge: a profile's sheet on the mapper reaches a
    // dialog made under it
    void test_aSheetOnTheDialogsParentTakesItsStrip()
    {
        makeTheDialogsHolders();
        mpHolder->setStyleSheet(scriptTabSheet());
        openTheRecipeDialog();
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY2(!mpRecipeDialog->designed(), "prepareTabStrip() gave the design a dialog's strip made under a parent whose sheet names QTabBar");
        readTheScriptsStrip(recipeDialogStrip(), mpHolder);
    }

    void test_aSheetFurtherUpTakesTheDialogsStrip()
    {
        makeTheDialogsHolders();
        mpGrandparent->setStyleSheet(scriptTabSheet());
        openTheRecipeDialog();
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY2(!mpRecipeDialog->designed(), "prepareTabStrip() gave the design a dialog's strip made two widgets under a sheet that names QTabBar");
        readTheScriptsStrip(recipeDialogStrip(), mpGrandparent);
    }

    // setProfileStyleSheet() while the dialog is open: the strip changes hands on the StyleChange the parent's sheet
    // sends the dialog, and back again when the sheet goes
    void test_aSheetOnTheDialogsParentComesAndGoesWhileItIsOpen()
    {
        makeTheDialogsHolders();
        openTheRecipeDialog();
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY2(mpRecipeDialog->designed(), "SETUP: a script's sheet takes the dialog's tabs with none set");

        const int beforeTheSheet = mpRecipeDialog->styleChanges();
        mpHolder->setStyleSheet(scriptTabSheet());
        TestPixels::settle();
        qInfo().noquote() << qsl("  setting the parent's sheet sent the dialog %1 StyleChange events").arg(QString::number(mpRecipeDialog->styleChanges() - beforeTheSheet));
        QVERIFY2(mpRecipeDialog->styleChanges() > beforeTheSheet, "setting a sheet on the dialog's parent sent the dialog no StyleChange, so it cannot learn that a script's sheet now reaches it");
        QVERIFY2(!mpRecipeDialog->designed(), "the design kept the dialog's strip when a sheet naming QTabBar went on its parent");
        readTheScriptsStrip(recipeDialogStrip(), mpHolder);
        if (QTest::currentTestFailed()) {
            return;
        }

        const int whileTheSheetHeld = mpRecipeDialog->styleChanges();
        mpHolder->setStyleSheet(QString());
        TestPixels::settle();
        qInfo().noquote() << qsl("  clearing the parent's sheet sent the dialog %1 StyleChange events").arg(QString::number(mpRecipeDialog->styleChanges() - whileTheSheetHeld));
        QVERIFY2(mpRecipeDialog->styleChanges() > whileTheSheetHeld, "clearing the sheet on the dialog's parent sent the dialog no StyleChange, so it cannot learn that the script's sheet went");
        QVERIFY2(mpRecipeDialog->designed(), "the design did not take the dialog's strip back once its parent's sheet went");
        readStrip(recipeDialogStrip(), qsl("the current"));
    }

    // A sheet above the dialog about something else leaves the design in place, and one changed to name QTabBar while
    // the dialog is open takes the strip
    void test_aParentsSheetNamingNoTabBarLeavesTheDesign()
    {
        makeTheDialogsHolders();
        // theme-fixed: the script's own colour for its buttons, which no strip may take as a reason to step aside
        mpHolder->setStyleSheet(qsl("QPushButton { color: %1; }").arg(QColor(Qt::red).name()));
        openTheRecipeDialog();
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY2(mpRecipeDialog->designed(), "the design stepped aside from a dialog's strip for a sheet on its parent that names no QTabBar");
        readStrip(recipeDialogStrip(), qsl("the current"));
        if (QTest::currentTestFailed()) {
            return;
        }

        mpHolder->setStyleSheet(scriptTabSheet());
        TestPixels::settle();
        QVERIFY2(!mpRecipeDialog->designed(), "the design kept the dialog's strip when its parent's sheet changed to one naming QTabBar");
        readTheScriptsStrip(recipeDialogStrip(), mpHolder);
    }

    // Last, as it leaves the windows made right to left for cleanupTestCase() to close
    void test_rightToLeftEveryStripIsTheMirrorImage()
    {
        const auto backToTheLanguagesDirection = qScopeGuard([]() {
            QGuiApplication::setLayoutDirection(Qt::LayoutDirectionAuto);
        });
        QGuiApplication::setLayoutDirection(Qt::RightToLeft);
        reopenTheWindows();
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY2(mpNotepad->tabWidget->tabBar()->layoutDirection() == Qt::RightToLeft, "SETUP: the notepad made after the switch is not right to left");
        TestPixels::setAppearanceAndWait(enums::Appearance::light, mpNotepad);

        readStrip(notepadStrip(), qsl("light"));
        if (QTest::currentTestFailed()) {
            return;
        }
        readStrip(packagesStrip(), qsl("light"));
        if (QTest::currentTestFailed()) {
            return;
        }
        readStrip(gamesStrip(), qsl("light"));
        if (QTest::currentTestFailed()) {
            return;
        }
        readStrip(formStrip(), qsl("light"));
        if (QTest::currentTestFailed()) {
            return;
        }

        readTheAddButton();
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY2(rightIn(mpPackages->lineEdit_searchBar, mpPackages) == rightIn(mpPackages->packageList, mpPackages),
                 "right to left the search field and the list under the strip do not end on one line");
        readTheGamesListsSpacing();
        if (QTest::currentTestFailed()) {
            return;
        }
        readTheFormStripsInset();
        if (QTest::currentTestFailed()) {
            return;
        }
        readTheTabWidgetsSize(mpNotepad->tabWidget, uiDesign::scmTabPaneInset, qsl("the notepad's tab widget"));
        if (QTest::currentTestFailed()) {
            return;
        }
        readTheTabWidgetsSize(mpConnection->tabWidget_connectionInfo, 0, qsl("the connection window's form"));
    }
};

#include "UnderlinedTabStripTest.moc"
MUDLET_GROUPED_TEST_MAIN(UnderlinedTabStripTest)
