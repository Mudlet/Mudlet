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
 * TStyle paints the profile strip with no stylesheet - the word, its underline, the hairline, the
 * cross, the indicator and their geometry - so only a grab shows them, and in both appearances,
 * since an ink can read in one and vanish in the other. The marks a profile's new output puts on
 * its tab are read off the grab too, as that is the one place a player sees them.
 *
 * Run with: ctest -R ProfileTabBarStyleTest -V
 */

#include <QAbstractButton>
#include <QApplication>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetrics>
#include <QHash>
#include <QHoverEvent>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QProxyStyle>
#include <QResizeEvent>
#include <QScopeGuard>
#include <QStyleFactory>
#include <QStyleOptionTab>
#include <QtTest/QtTest>

#include "DarkTheme.h"
#include "PixelTestHelper.h"
#include "TTabBar.h"
#include "uiDesign.h"

#include "GroupedTest.h"

// initStyleOption() is protected, and the option it fills is the only source of the text's rectangle
class ExposedTabBar : public TTabBar
{
public:
    using QTabBar::initStyleOption;
    using TTabBar::TTabBar;
};

// Stands in for the native style TStyle's proxy base would build on demand, which on Windows 10
// paints from the OS theme rather than from Mudlet's palette (#8999)
class NativeBaseTrap : public QProxyStyle
{
public:
    NativeBaseTrap()
    : QProxyStyle(QStyleFactory::create(qsl("Fusion")))
    {
    }

    mutable QStringList mPainted;

    void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override
    {
        mPainted << qsl("control %1").arg(QString::number(element));
        QProxyStyle::drawControl(element, option, painter, widget);
    }
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override
    {
        mPainted << qsl("primitive %1").arg(QString::number(element));
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const override
    {
        mPainted << qsl("complex control %1").arg(QString::number(control));
        QProxyStyle::drawComplexControl(control, option, painter, widget);
    }
};

class ProfileTabBarStyleTest : public QObject
{
    Q_OBJECT

private:
    ExposedTabBar* mpTabBar = nullptr;
    QPalette mSavedPalette;

    // A flat fill is written rather than blended, so it agrees with the token to a level or two
    static constexpr int scmInkTolerance = 2;
    // Wider than three short names need, so the hairline has room to run on past the last tab
    static constexpr int scmBarWidth = 600;

    static QString saidAs(const QColor& colour) { return colour.name(QColor::HexArgb); }

    // A never-shown widget keeps its resize pending until rendered, and QTabBar lays out its tabs
    // on that event, so the tabs would be measured for a top-level's default width
    static void layOutAtItsOwnSize(QTabBar* pTabBar)
    {
        QResizeEvent resized(pTabBar->size(), QSize());
        QApplication::sendEvent(pTabBar, &resized);
        QCoreApplication::processEvents();
    }

    static QColor commonestColour(const QImage& image)
    {
        QHash<QRgb, int> counts;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                ++counts[image.pixel(x, y)];
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

    static QRect scannedIn(const QImage& strip, const QRect& box)
    {
        const qreal ratio = strip.devicePixelRatio();
        return QRect(qRound(box.left() * ratio), qRound(box.top() * ratio), qRound(box.width() * ratio), qRound(box.height() * ratio)) & strip.rect();
    }

    // Thresholded against the word's own ink: a fixed distance would count a stronger ink as more word
    static int inkCoverage(const QImage& strip, const QRect& box, const QColor& ink)
    {
        const QRect scanned = scannedIn(strip, box);
        const QColor behind = commonestColour(strip.copy(scanned));
        const int reach = TestPixels::distance(behind, ink);
        int touched = 0;
        for (int y = scanned.top(); y <= scanned.bottom(); ++y) {
            for (int x = scanned.left(); x <= scanned.right(); ++x) {
                if (2 * TestPixels::distance(QColor(strip.pixel(x, y)), behind) >= reach) {
                    ++touched;
                }
            }
        }
        return touched;
    }

    static int inkedWidth(const QImage& strip, const QRect& box, const QColor& ink)
    {
        const qreal ratio = strip.devicePixelRatio();
        const QRect scanned = scannedIn(strip, box);
        const QColor behind = commonestColour(strip.copy(scanned));
        const int reach = TestPixels::distance(behind, ink);
        int left = -1;
        int right = -1;
        for (int x = scanned.left(); x <= scanned.right(); ++x) {
            for (int y = scanned.top(); y <= scanned.bottom(); ++y) {
                if (2 * TestPixels::distance(QColor(strip.pixel(x, y)), behind) >= reach) {
                    left = left < 0 ? x : left;
                    right = x;
                    break;
                }
            }
        }
        return left < 0 ? 0 : qRound((right - left + 1) / ratio);
    }

    // The pixel of a word furthest from what it is written on carries its ink: antialiasing leaves
    // only the stems at full coverage
    static QColor strongestInk(const QImage& strip, const QRect& box)
    {
        const QRect scanned = scannedIn(strip, box);
        const QColor behind = commonestColour(strip.copy(scanned));
        QColor strongest = behind;
        for (int y = scanned.top(); y <= scanned.bottom(); ++y) {
            for (int x = scanned.left(); x <= scanned.right(); ++x) {
                const QColor pixel(strip.pixel(x, y));
                if (TestPixels::distance(pixel, behind) > TestPixels::distance(strongest, behind)) {
                    strongest = pixel;
                }
            }
        }
        return strongest;
    }

    static QRect wordBoxOf(ExposedTabBar& bar, const int index)
    {
        QStyleOptionTab option;
        bar.initStyleOption(&option, index);
        return bar.style()->subElementRect(QStyle::SE_TabBarTabText, &option, &bar);
    }

    // Mirrors TStyle::layoutTab(): each tab leads with scmTabStripInset of the gap and trails the rest
    static QRect contentOf(const QTabBar& bar, const int index) { return bar.tabRect(index).adjusted(uiDesign::scmTabStripInset, 0, uiDesign::scmTabStripInset - uiDesign::scmTabGap, 0); }
    QRect contentOf(const int index) const { return contentOf(*mpTabBar, index); }

    static QRect lineOf(const QTabBar& bar, const int index)
    {
        const QRect tab = bar.tabRect(index);
        return QRect(tab.left(), tab.top() + uiDesign::scmTabPaddingTop, tab.width(), tab.height() - uiDesign::scmTabPaddingTop - uiDesign::scmTabPaddingBottom);
    }

    // In the gap a tab trails, on the word's line, where nothing but the surface is drawn
    QPoint gapSpot(const int index) const
    {
        const QRect tab = mpTabBar->tabRect(index);
        return QPoint(tab.right() - (uiDesign::scmTabGap - uiDesign::scmTabStripInset) / 2, lineOf(*mpTabBar, index).center().y());
    }

    QRect indicatorOf(const int index) const
    {
        const QRect line = lineOf(*mpTabBar, index);
        return QRect(contentOf(index).left(), line.top() + (line.height() - TStyle::sIndicatorDiameter) / 2, TStyle::sIndicatorDiameter, TStyle::sIndicatorDiameter);
    }

    QAbstractButton* crossOf(const int index) const { return qobject_cast<QAbstractButton*>(mpTabBar->tabButton(index, QTabBar::RightSide)); }

    // QTest::mouseMove does not always reach a window nothing is compositing, so the hover event
    // is sent as well
    void hoverOver(const int index)
    {
        const QPoint over = mpTabBar->tabRect(index).center();
        QTest::mouseMove(mpTabBar, over);
        QCoreApplication::processEvents();
        QHoverEvent hover(QEvent::HoverMove, QPointF(over), QPointF(over), QPointF(-1, -1));
        QApplication::sendEvent(mpTabBar, &hover);
        QCoreApplication::processEvents();
    }

    // The accent on the bottom rows under a tab's content, the hairline under the rest
    static bool standsOnTheUnderline(const QImage& strip, const QTabBar& bar, const int index, const uiDesign::ThemeTokens& tokens, QString* pWhy)
    {
        const QRect content = contentOf(bar, index);
        const int bottom = bar.tabRect(index).bottom();
        for (int row = bottom - uiDesign::scmTabUnderlineWidth + 1; row <= bottom; ++row) {
            for (int x = content.left(); x <= content.right(); ++x) {
                const QColor read = TestPixels::sampled(strip, QPoint(x, row));
                if (!TestPixels::readsAs(read, tokens.accent, scmInkTolerance)) {
                    *pWhy = qsl("at %1,%2 under tab %3 it reads %4 rather than the accent %5")
                                    .arg(QString::number(x), QString::number(row), QString::number(index), saidAs(read), saidAs(tokens.accent));
                    return false;
                }
            }
        }
        return true;
    }

    static bool carriesNoAccent(const QImage& strip, const QTabBar& bar, const int index, const uiDesign::ThemeTokens& tokens, QString* pWhy)
    {
        const QRect tab = bar.tabRect(index);
        for (int row = tab.bottom() - uiDesign::scmTabFocusUnderlineWidth + 1; row <= tab.bottom(); ++row) {
            for (int x = tab.left(); x <= tab.right(); ++x) {
                const QColor read = TestPixels::sampled(strip, QPoint(x, row));
                if (TestPixels::readsAs(read, tokens.accent, scmInkTolerance)) {
                    *pWhy = qsl("tab %1 carries the accent at %2,%3").arg(QString::number(index), QString::number(x), QString::number(row));
                    return false;
                }
            }
        }
        return true;
    }

    void theChosenTabStandsOnTheUnderline()
    {
        const QImage strip = TestPixels::shot(mpTabBar);
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor surface = commonestColour(strip);

        for (const int index : {0, 2}) {
            const QColor beside = TestPixels::sampled(strip, gapSpot(index));
            QVERIFY2(TestPixels::readsAs(beside, surface, 0),
                     qPrintable(qsl("tab %1 is drawn on %2 rather than on the strip's own surface %3 - a tab is still filled").arg(QString::number(index), saidAs(beside), saidAs(surface))));
        }

        QString why;
        QVERIFY2(standsOnTheUnderline(strip, *mpTabBar, 0, tokens, &why), qPrintable(qsl("the tab on show stands on no accent underline: %1").arg(why)));
        const QColor overTheUnderline = TestPixels::sampled(strip, QPoint(contentOf(0).center().x(), mpTabBar->tabRect(0).bottom() - uiDesign::scmTabUnderlineWidth));
        QVERIFY2(!TestPixels::readsAs(overTheUnderline, tokens.accent, scmInkTolerance), "the underline is thicker than the design's two pixels, though the bar takes no keyboard focus");
        QVERIFY2(carriesNoAccent(strip, *mpTabBar, 2, tokens, &why), qPrintable(qsl("a tab that is not on show reads as chosen: %1").arg(why)));

        const QColor underTheRest = TestPixels::sampled(strip, QPoint(contentOf(2).center().x(), mpTabBar->tabRect(2).bottom()));
        QVERIFY2(TestPixels::readsAs(underTheRest, tokens.border, scmInkTolerance),
                 qPrintable(qsl("the tab that is not on show stands on %1 rather than the hairline %2").arg(saidAs(underTheRest), saidAs(tokens.border))));
        const int lastEnd = mpTabBar->tabRect(mpTabBar->count() - 1).right();
        QVERIFY2(lastEnd < mpTabBar->width() - 2, "SETUP: the tabs fill the bar, so the hairline past them cannot be read");
        const QColor farEnd = TestPixels::sampled(strip, QPoint(mpTabBar->width() - 1, mpTabBar->height() - 1));
        QVERIFY2(TestPixels::readsAs(farEnd, tokens.border, scmInkTolerance),
                 qPrintable(qsl("at the bar's far end, past every tab, its bottom row reads %1 rather than the hairline %2").arg(saidAs(farEnd), saidAs(tokens.border))));
    }

    // Nearer the accent's ink than the chrome tone or the full text ink
    void theChosenWordIsTheAccentInk()
    {
        const QImage strip = TestPixels::shot(mpTabBar);
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor wanted = uiDesign::readableOn(tokens.page, tokens.accentText, tokens.text, uiDesign::scmTextMinimumRatio);
        const QColor ink = strongestInk(strip, wordBoxOf(*mpTabBar, 0));
        QVERIFY2(TestPixels::distance(ink, wanted) < TestPixels::distance(ink, tokens.mutedText) && TestPixels::distance(ink, wanted) < TestPixels::distance(ink, tokens.text),
                 qPrintable(qsl("the word on the tab on show is written in %1, nearer the chrome tone %2 or the text ink %3 than the accent's ink %4")
                                    .arg(saidAs(ink), saidAs(tokens.mutedText), saidAs(tokens.text), saidAs(wanted))));
    }

    void theIndicatorIsAStateColour()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

        for (const int index : {0, 2}) {
            mpTabBar->setTabConnectionIndicator(index, TabConnectionIndicator::Connected);
            QImage strip = TestPixels::shot(mpTabBar);
            const QColor dot = TestPixels::sampled(strip, indicatorOf(index).center());
            const QColor wantedDot = uiDesign::readableOn(tokens.page, uiDesign::stateColor(uiDesign::scmStateHue_ok, tokens.darkPage), tokens.text, uiDesign::scmQuietMinimumRatio);
            QVERIFY2(TestPixels::readsAs(dot, wantedDot, scmInkTolerance),
                     qPrintable(
                             qsl("a connected profile's dot on tab %1 is not the state colour for something working: %2 rather than %3").arg(QString::number(index), saidAs(dot), saidAs(wantedDot))));
            mpTabBar->setTabConnectionIndicator(index, TabConnectionIndicator::None);
        }

        // The ring is in the ink of the word beside it
        mpTabBar->setTabConnectionIndicator(2, TabConnectionIndicator::Disconnected);
        const QImage strip = TestPixels::shot(mpTabBar);
        const QRect box = indicatorOf(2);
        const QColor ring = TestPixels::sampled(strip, QPoint(box.left(), box.center().y()));
        QVERIFY2(TestPixels::readsAs(ring, tokens.mutedText, scmInkTolerance),
                 qPrintable(qsl("a disconnected profile's ring is not drawn in the chrome tone of its word: %1 rather than %2").arg(saidAs(ring), saidAs(tokens.mutedText))));
        mpTabBar->setTabConnectionIndicator(2, TabConnectionIndicator::None);
        QCoreApplication::processEvents();
    }

private slots:
    void init()
    {
        mSavedPalette = QApplication::palette();
        mpTabBar = new ExposedTabBar(nullptr);
        // In mudlet.cpp's order: the crosses have to exist before the tabs, or QTabBar silently makes its own
        mpTabBar->setMovable(true);
        mpTabBar->setTabsClosable(true);
        // A top-level here, with no main window frame to fill the surface behind the tabs
        mpTabBar->setAutoFillBackground(true);
        for (const QString& profileName : {qsl("Alpha"), qsl("Bravo"), qsl("Delta")}) {
            mpTabBar->setTabData(mpTabBar->addTab(profileName), profileName);
        }
        mpTabBar->resize(scmBarWidth, mpTabBar->sizeHint().height());
        layOutAtItsOwnSize(mpTabBar);
    }

    void cleanup()
    {
        delete mpTabBar;
        mpTabBar = nullptr;
        QApplication::setPalette(mSavedPalette);
    }

    void test_theCrossTrailsTheWordAndIsTheDesignsSize()
    {
        const QSize wanted(uiDesign::scmTabCloseBoxSize, uiDesign::scmTabCloseBoxSize);
        for (int i = 0; i < mpTabBar->count(); ++i) {
            QAbstractButton* pCross = crossOf(i);
            QVERIFY2(pCross, qPrintable(qsl("tab %1 carries no cross on its trailing edge").arg(QString::number(i))));
            QVERIFY2(!mpTabBar->tabButton(i, QTabBar::LeftSide), qPrintable(qsl("tab %1 carries a cross in front of its word, where the platform puts it").arg(QString::number(i))));
            QVERIFY2(pCross->size() == wanted,
                     qPrintable(qsl("the cross on tab %1 is %2x%3 rather than the %4x%4 box the design draws it in")
                                        .arg(QString::number(i), QString::number(pCross->size().width()), QString::number(pCross->size().height()), QString::number(uiDesign::scmTabCloseBoxSize))));
            QVERIFY2(pCross->geometry().right() == contentOf(i).right(),
                     qPrintable(qsl("the cross on tab %1 ends at %2 rather than where the tab's own extent does, %3, before the gap to the next tab")
                                        .arg(QString::number(i), QString::number(pCross->geometry().right()), QString::number(contentOf(i).right()))));
        }

        QVERIFY2(mpTabBar->drawBase(), "the strip draws no base, so there is no hairline for its tabs to stand on");
        QVERIFY2(!mpTabBar->expanding(), "the tabs share the width of the window rather than standing as words from its leading edge");
        QVERIFY2(mpTabBar->testAttribute(Qt::WA_Hover), "the bar asks for no hover events, so no word can ever light under the pointer");
    }

    void test_theCrossAndTheWordShareOneLine()
    {
        for (int i = 0; i < mpTabBar->count(); ++i) {
            const int lineCentre = lineOf(*mpTabBar, i).center().y();

            QAbstractButton* pCross = crossOf(i);
            QVERIFY(pCross);
            const int crossCentre = pCross->geometry().center().y();
            QVERIFY2(std::abs(crossCentre - lineCentre) <= 1,
                     qPrintable(qsl("the cross on tab %1 sits %2px off the word's line").arg(QString::number(i), QString::number(std::abs(crossCentre - lineCentre)))));

            const QRect text = wordBoxOf(*mpTabBar, i);
            QVERIFY2(std::abs(text.center().y() - lineCentre) <= 1,
                     qPrintable(qsl("the word on tab %1 sits %2px off its line").arg(QString::number(i), QString::number(std::abs(text.center().y() - lineCentre)))));
            QVERIFY2(text.right() < pCross->geometry().left(), qPrintable(qsl("the word on tab %1 runs under the cross rather than stopping in front of it").arg(QString::number(i))));
        }
    }

    void test_theTabIsTheRecipesSize()
    {
        const int wantedHeight = std::max(mpTabBar->fontMetrics().height(), uiDesign::scmTabCloseBoxSize) + uiDesign::scmTabPaddingTop + uiDesign::scmTabPaddingBottom;
        QCOMPARE(mpTabBar->tabSizeHint(0).height(), wantedHeight);

        QCOMPARE(mpTabBar->tabRect(0).left(), 0);
        QVERIFY2(wordBoxOf(*mpTabBar, 0).left() == uiDesign::scmTabStripInset,
                 qPrintable(qsl("the first name starts at %1 rather than at the strip's inset, %2").arg(QString::number(wordBoxOf(*mpTabBar, 0).left()), QString::number(uiDesign::scmTabStripInset))));
        for (int i = 1; i < mpTabBar->count(); ++i) {
            QVERIFY2(mpTabBar->tabRect(i).left() == mpTabBar->tabRect(i - 1).right() + 1,
                     qPrintable(qsl("tab %1 starts at %2 rather than where the one before it ends, %3 - the gap between them is inside a tab's own rectangle")
                                        .arg(QString::number(i), QString::number(mpTabBar->tabRect(i).left()), QString::number(mpTabBar->tabRect(i - 1).right() + 1))));
            const int gap = wordBoxOf(*mpTabBar, i).left() - (contentOf(i - 1).right() + 1);
            QVERIFY2(gap == uiDesign::scmTabGap,
                     qPrintable(qsl("tab %1's word starts %2px after the tab before it ends, rather than the design's %3")
                                        .arg(QString::number(i), QString::number(gap), QString::number(uiDesign::scmTabGap))));
        }

        const int lastEdge = mpTabBar->tabRect(mpTabBar->count() - 1).right();
        QVERIFY2(lastEdge < mpTabBar->width() - 1,
                 qPrintable(
                         qsl("three tabs end at %1 on a %2px bar, so they are stretched across it rather than standing as words").arg(QString::number(lastEdge), QString::number(mpTabBar->width()))));
    }

    void test_theChosenWordIsSemiboldAndTheStripDoesNotShiftForIt()
    {
        // The same word on both tabs, so only the choice can differ between the two readings
        ExposedTabBar bar(nullptr);
        bar.setAutoFillBackground(true);
        bar.setTabData(bar.addTab(qsl("Same")), qsl("Same-A"));
        bar.setTabData(bar.addTab(qsl("Same")), qsl("Same-B"));
        bar.resize(scmBarWidth, bar.sizeHint().height());
        layOutAtItsOwnSize(&bar);
        bar.setCurrentIndex(0);
        QCoreApplication::processEvents();

        const QImage strip = bar.grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

        QFont semibold = bar.font();
        semibold.setWeight(uiDesign::scmTabChosenWeight);
        if (QFontInfo(semibold).weight() > QFontInfo(bar.font()).weight()) {
            // Each word's own ink, or the reading counts the whole box as word
            const int chosenInk = inkCoverage(strip, wordBoxOf(bar, 0), tokens.accentText);
            const int restingInk = inkCoverage(strip, wordBoxOf(bar, 1), tokens.mutedText);
            QVERIFY2(restingInk > 0, "the resting tab's word marks nothing at all, so there is no reading to compare the chosen one against");
            QVERIFY2(100 * chosenInk >= 105 * restingInk,
                     qPrintable(qsl("the word on the chosen tab marks %1 pixels of its box against the %2 the same word marks on the resting tab beside it, so it is not drawn any heavier")
                                        .arg(QString::number(chosenInk), QString::number(restingInk))));
        } else {
            qInfo("  this machine's font has no face heavier than its regular one at the chosen weight, so the weight is not read off the grab");
        }

        // A pixel of slack: Qt measures a tab's text with QFontMetrics::size(), this with horizontalAdvance()
        QFont bold = bar.font();
        bold.setBold(true);
        for (int i = 0; i < bar.count(); ++i) {
            const int boldAdvance = QFontMetrics(bold).horizontalAdvance(bar.tabText(i));
            QVERIFY2(boldAdvance > bar.fontMetrics().horizontalAdvance(bar.tabText(i)), "bold and regular measure this word the same here, so nothing below could be told apart");
            QVERIFY2(wordBoxOf(bar, i).width() + 1 >= boldAdvance,
                     qPrintable(qsl("tab %1 leaves its word %2px, less than the %3px that word takes in bold - so a tab marked with new output is measured narrower than it is drawn")
                                        .arg(QString::number(i), QString::number(wordBoxOf(bar, i).width()), QString::number(boldAdvance))));
        }

        QList<QRect> before;
        for (int i = 0; i < bar.count(); ++i) {
            before << bar.tabRect(i);
        }
        bar.setCurrentIndex(1);
        QCoreApplication::processEvents();
        for (int i = 0; i < bar.count(); ++i) {
            QVERIFY2(bar.tabRect(i) == before.at(i),
                     qPrintable(qsl("tab %1 stood at %2 and moved to %3 when the choice moved to the tab beside it, so the whole strip steps sideways on every switch")
                                        .arg(QString::number(i), TestPixels::describe(before.at(i)), TestPixels::describe(bar.tabRect(i)))));
        }
    }

    // tabText() is not always the profile's name: #2056 found a '&' accelerator in it on FreeBSD,
    // and applyPrefixToDisplayedText() prefixes it
    void test_anActivityMarkKeyedOnTheProfileNameReachesThePainting()
    {
        ExposedTabBar bar(nullptr);
        bar.setAutoFillBackground(true);
        bar.setTabData(bar.addTab(qsl("Chosen")), qsl("Chosen"));
        // The same displayed word on both resting tabs, so only the mark can tell them apart; set
        // through QTabBar because TTabBar's own setters escape the '&' the platform would have added
        bar.setTabData(bar.QTabBar::addTab(qsl("&Gorbash")), qsl("Gorbash"));
        bar.setTabData(bar.QTabBar::addTab(qsl("&Gorbash")), qsl("Gorbash-Twin"));
        QCOMPARE(bar.tabText(1), qsl("&Gorbash"));
        bar.resize(scmBarWidth, bar.sizeHint().height());
        layOutAtItsOwnSize(&bar);
        bar.setCurrentIndex(0);

        bar.markActivity(qsl("Gorbash"), false);
        QCoreApplication::processEvents();

        const QImage strip = bar.grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const QColor ink = uiDesign::themeTokens().mutedText;
        const int markedInk = inkCoverage(strip, wordBoxOf(bar, 1), ink);
        const int unmarkedInk = inkCoverage(strip, wordBoxOf(bar, 2), ink);
        QVERIFY2(unmarkedInk > 0, "the unmarked tab's word marks nothing at all, so there is no reading to compare the marked one against");
        QVERIFY2(100 * markedInk >= 115 * unmarkedInk,
                 qPrintable(qsl("the word on the tab with new output marks %1 pixels of its box against the %2 the same word marks on the tab beside it, so the mark never reached the "
                                "painting")
                                    .arg(QString::number(markedInk), QString::number(unmarkedInk))));
    }

    // QTabBar measures a tab once and keeps the answer, so a mark that only repainted would clip the
    // bold name. Not expanding, as a share of a wide bar leaves room whatever was measured.
    void test_aTabTurnedBoldByItsMarkIsNotClippedAndDoesNotMove()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Built with WITH_FONTS=NO, so there is no font here that is known to be wider in bold");
#else
        // The machine's default font may be no wider in bold, which would leave nothing to see
        for (const QString& file : {qsl(":/fonts/ttf-bitstream-vera-1.10/Vera.ttf"), qsl(":/fonts/ttf-bitstream-vera-1.10/VeraBd.ttf")}) {
            QVERIFY2(QFontDatabase::addApplicationFont(file) != -1, qPrintable(qsl("Could not register the bundled font %1").arg(file)));
        }
        const QFont plain(qsl("Bitstream Vera Sans"), 10);
        QCOMPARE(QFontInfo(plain).family(), qsl("Bitstream Vera Sans"));

        const QString profileName = qsl("Zugg the Unyielding");
        ExposedTabBar bar(nullptr);
        bar.setFont(plain);
        bar.setExpanding(false);
        bar.setAutoFillBackground(true);
        bar.setTabData(bar.addTab(qsl("Gorbash")), qsl("Gorbash"));
        const int index = bar.addTab(profileName);
        bar.setTabData(index, profileName);
        // The debug tag's prefix, so the mark has to reach the painting by the name in the tab's data
        bar.applyPrefixToDisplayedText(profileName, qsl("[1] "));
        bar.resize(1000, bar.sizeHint().height());
        layOutAtItsOwnSize(&bar);
        bar.setCurrentIndex(0);
        QCoreApplication::processEvents();

        // Room for a glyph's bearings either side of the ink
        constexpr int bearingSlack = 3;
        QFont bold(plain);
        bold.setBold(true);
        const int plainAdvance = QFontMetrics(plain).horizontalAdvance(bar.tabText(index));
        const int boldAdvance = QFontMetrics(bold).horizontalAdvance(bar.tabText(index));
        QVERIFY2(boldAdvance - plainAdvance > 2 * bearingSlack, "the name is barely wider in bold, so a plain or clipped word could not be told from a bold one");

        const QRect before = bar.tabRect(index);
        bar.markActivity(profileName, false);
        QCoreApplication::processEvents();
        QVERIFY2(bar.tabRect(index) == before,
                 qPrintable(qsl("the tab stood at %1 and moved to %2 when its mark went on, so the strip steps sideways whenever output arrives")
                                    .arg(TestPixels::describe(before), TestPixels::describe(bar.tabRect(index)))));

        // The word's line, not the whole tab, whose bottom row is the hairline running under every tab
        const QImage strip = bar.grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const int inked = inkedWidth(strip, lineOf(bar, index), uiDesign::themeTokens().mutedText);
        QVERIFY2(inked + bearingSlack >= boldAdvance,
                 qPrintable(qsl("the marked name inks %1px of the %2px it takes in bold (%3px in plain), so it is clipped or drawn without the mark")
                                    .arg(QString::number(inked), QString::number(boldAdvance), QString::number(plainAdvance))));
#endif
    }

    // The mark is what tells a player a profile behind another tab has text waiting, so it has to
    // stand out from a quiet tab, stay apart from the chosen one and go once the tab is chosen
    void test_newOutputMarksAnUnchosenTabInTheFullInk()
    {
        ExposedTabBar bar(nullptr);
        bar.setAutoFillBackground(true);
        bar.setTabsClosable(true);
        bar.setTabData(bar.addTab(qsl("Gorbash")), qsl("Gorbash"));
        bar.setTabData(bar.addTab(qsl("Zugg")), qsl("Zugg"));
        bar.setTabData(bar.addTab(qsl("Kaldor")), qsl("Kaldor"));
        bar.resize(scmBarWidth, bar.sizeHint().height());
        layOutAtItsOwnSize(&bar);
        bar.setCurrentIndex(0);
        QCoreApplication::processEvents();
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

        const QImage quiet = bar.grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const QRect zuggBefore = bar.tabRect(1);
        bar.markActivity(qsl("Zugg"), false);
        bar.markActivity(qsl("Kaldor"), true);
        QCoreApplication::processEvents();
        QVERIFY(bar.tabBold(qsl("Zugg")) && bar.tabItalic(qsl("Kaldor")));
        QVERIFY2(bar.tabRect(1) == zuggBefore, qPrintable(qsl("the marked tab stood at %1 and moved to %2").arg(TestPixels::describe(zuggBefore), TestPixels::describe(bar.tabRect(1)))));
        const QImage marked = bar.grab().toImage().convertToFormat(QImage::Format_ARGB32);

        const QRect zuggWord = wordBoxOf(bar, 1);
        const QRect scanned = scannedIn(marked, zuggWord);
        int differing = 0;
        for (int y = scanned.top(); y <= scanned.bottom(); ++y) {
            for (int x = scanned.left(); x <= scanned.right(); ++x) {
                differing += quiet.pixel(x, y) != marked.pixel(x, y) ? 1 : 0;
            }
        }
        qInfo().noquote() << qsl("  %1 of the marked word's pixels changed with the mark").arg(QString::number(differing));
        QVERIFY2(differing > 0, "the tab marked with new output is drawn exactly as it was before the mark");

        for (const int index : {1, 2}) {
            const QColor ink = strongestInk(marked, wordBoxOf(bar, index));
            QVERIFY2(TestPixels::distance(ink, tokens.text) < TestPixels::distance(ink, tokens.mutedText),
                     qPrintable(qsl("the marked word on tab %1 is written in %2, nearer the quiet chrome tone %3 than the full text ink %4, so the mark barely shows")
                                        .arg(QString::number(index), saidAs(ink), saidAs(tokens.mutedText), saidAs(tokens.text))));
        }
        const QColor quietInk = strongestInk(quiet, zuggWord);
        QVERIFY2(TestPixels::distance(quietInk, tokens.mutedText) < TestPixels::distance(quietInk, tokens.text),
                 qPrintable(qsl("a quiet tab is already written in %1, nearer the text ink %2 than the chrome tone %3, so the mark adds nothing")
                                    .arg(saidAs(quietInk), saidAs(tokens.text), saidAs(tokens.mutedText))));

        QString why;
        QVERIFY2(standsOnTheUnderline(marked, bar, 0, tokens, &why), qPrintable(qsl("the tab on show lost its underline when another was marked: %1").arg(why)));
        for (const int index : {1, 2}) {
            QVERIFY2(carriesNoAccent(marked, bar, index, tokens, &why), qPrintable(qsl("a marked tab reads as chosen: %1").arg(why)));
        }

        QFont bold = bar.font();
        bold.setBold(true);
        const int boldAdvance = QFontMetrics(bold).horizontalAdvance(bar.tabText(1));
        const int inked = inkedWidth(marked, zuggWord, tokens.text);
        QVERIFY2(inked + 3 >= boldAdvance, qPrintable(qsl("the marked name inks %1px of the %2px it takes in bold, so it is clipped").arg(QString::number(inked), QString::number(boldAdvance))));

        // As mudlet's tab switch does: choose, then clear
        bar.setCurrentIndex(1);
        bar.clearActivity(1);
        QCoreApplication::processEvents();
        QVERIFY2(!bar.tabBold(qsl("Zugg")), "the tab the player chose still carries the mark for text they have now seen");
        QVERIFY2(bar.tabItalic(qsl("Kaldor")), "choosing one tab took the mark off another");
        const QImage chosen = bar.grab().toImage().convertToFormat(QImage::Format_ARGB32);
        QVERIFY2(standsOnTheUnderline(chosen, bar, 1, tokens, &why), qPrintable(qsl("the newly chosen tab stands on no underline: %1").arg(why)));
        const QColor chosenInk = strongestInk(chosen, wordBoxOf(bar, 1));
        const QColor accentInk = uiDesign::readableOn(tokens.page, tokens.accentText, tokens.text, uiDesign::scmTextMinimumRatio);
        QVERIFY2(TestPixels::distance(chosenInk, accentInk) < TestPixels::distance(chosenInk, tokens.text),
                 qPrintable(qsl("the chosen tab's word is written in %1, nearer the mark's text ink %2 than the accent's %3").arg(saidAs(chosenInk), saidAs(tokens.text), saidAs(accentInk))));
    }

    // The marks are kept by the profile's name, so a tab dragged to another place keeps its own
    void test_aMarkStaysWithItsTabWhenTheTabIsMoved()
    {
        mpTabBar->setCurrentIndex(0);
        mpTabBar->markActivity(qsl("Delta"), false);
        QCoreApplication::processEvents();
        mpTabBar->moveTab(2, 1);
        QCoreApplication::processEvents();
        QCOMPARE(mpTabBar->tabIndex(qsl("Delta")), 1);
        QVERIFY2(mpTabBar->tabBold(1), "the moved tab left its mark behind at its old place");
        QVERIFY2(!mpTabBar->tabBold(2), "the tab now at the marked tab's old place took its mark");

        const QImage strip = TestPixels::shot(mpTabBar);
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor ink = strongestInk(strip, wordBoxOf(*mpTabBar, 1));
        QVERIFY2(TestPixels::distance(ink, tokens.text) < TestPixels::distance(ink, tokens.mutedText),
                 qPrintable(qsl("the moved tab's word is written in %1 rather than the mark's text ink %2").arg(saidAs(ink), saidAs(tokens.text))));
    }

    // A tab bar whose script sheet draws the tabs still shows which profile has text waiting
    void test_aScriptSheetStillCarriesTheMarks()
    {
        const auto restoreSheet = qScopeGuard([this]() {
            mpTabBar->setStyleSheet(QString());
        });
        mpTabBar->setStyleSheet(qsl("QTabBar::tab { background: rgb(250, 250, 250); color: black; }"));
        QTRY_VERIFY2(mpTabBar->expanding(), "a script's sheet took the tabs, yet they no longer share the bar's width as they did before the design");
        mpTabBar->setTabData(mpTabBar->addTab(qsl("Bravo")), qsl("Bravo-Twin"));
        mpTabBar->setCurrentIndex(0);
        mpTabBar->markActivity(qsl("Bravo"), false);
        QCoreApplication::processEvents();

        const QImage strip = TestPixels::shot(mpTabBar);
        // theme-fixed: the sheet above writes black, so black is what the words are read against
        const QColor sheetInk(Qt::black);
        const int markedInk = inkCoverage(strip, wordBoxOf(*mpTabBar, 1), sheetInk);
        const int unmarkedInk = inkCoverage(strip, wordBoxOf(*mpTabBar, 3), sheetInk);
        QVERIFY2(unmarkedInk > 0, "the unmarked tab's word marks nothing under the script's sheet");
        QVERIFY2(100 * markedInk >= 115 * unmarkedInk,
                 qPrintable(qsl("under a script's sheet the marked word marks %1 pixels against the %2 of the same word unmarked, so the mark is lost")
                                    .arg(QString::number(markedInk), QString::number(unmarkedInk))));

        mpTabBar->setStyleSheet(QString());
        QTRY_VERIFY2(!mpTabBar->expanding(), "the tabs still share the bar's width after the script's sheet went");
    }

    void test_anApplicationSheetNamingTheTabBarTakesTheTabs()
    {
        const auto restoreSheet = qScopeGuard([this]() {
            qApp->setStyleSheet(QString());
            mpTabBar->refreshAfterApplicationStyleChange();
        });
        mpTabBar->setCurrentIndex(0);
        const int tabWidth = mpTabBar->tabSizeHint(0).width();
        constexpr int sheetWidth = 500;
        QVERIFY2(tabWidth < sheetWidth, "the tab is already as wide as the sheet below asks for, so nothing could be told from its width");

        qApp->setStyleSheet(qsl("QPushButton { min-width: %1px; }").arg(QString::number(sheetWidth)));
        mpTabBar->refreshAfterApplicationStyleChange();
        QVERIFY2(mpTabBar->tabSizeHint(0).width() == tabWidth,
                 qPrintable(qsl("a sheet about push buttons changed the tab's width from %1 to %2px").arg(QString::number(tabWidth), QString::number(mpTabBar->tabSizeHint(0).width()))));
        theChosenTabStandsOnTheUnderline();
        if (QTest::currentTestFailed()) {
            return;
        }

        qApp->setStyleSheet(qsl("QTabBar::tab { min-width: %1px; }").arg(QString::number(sheetWidth)));
        mpTabBar->refreshAfterApplicationStyleChange();
        QVERIFY2(mpTabBar->tabSizeHint(0).width() >= sheetWidth,
                 qPrintable(qsl("the tab is %1px wide under a sheet asking for at least %2px, so the design kept the tabs from it")
                                    .arg(QString::number(mpTabBar->tabSizeHint(0).width()), QString::number(sheetWidth))));
        QVERIFY2(mpTabBar->expanding(), "under a script's sheet the tabs no longer share the bar's width as they did before the design");
    }

    // setProfileStyleSheet() puts the profile's sheet on the bar itself
    void test_aProfileSheetNamingTheTabBarTakesTheTabs()
    {
        const auto restoreSheet = qScopeGuard([this]() {
            mpTabBar->setStyleSheet(QString());
        });
        mpTabBar->setCurrentIndex(0);
        constexpr int sheetTypePixels = 30;
        mpTabBar->setStyleSheet(qsl("QTabBar::tab { background: rgb(250, 250, 250); color: black; font-size: %1px; }").arg(QString::number(sheetTypePixels)));
        QTRY_VERIFY2(mpTabBar->tabSizeHint(0).height() > sheetTypePixels,
                     qPrintable(qsl("the tab is %1px tall under a sheet setting its name in %2px type").arg(QString::number(mpTabBar->tabSizeHint(0).height()), QString::number(sheetTypePixels))));

        const QImage picture = mpTabBar->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const QRect tab = mpTabBar->tabRect(0).intersected(picture.rect());
        int dark = 0;
        int inkTop = tab.bottom();
        int inkBottom = tab.top();
        for (int y = tab.top(); y <= tab.bottom(); ++y) {
            for (int x = tab.left(); x <= tab.right(); ++x) {
                if (qGray(picture.pixel(x, y)) < 64) {
                    ++dark;
                    inkTop = std::min(inkTop, y);
                    inkBottom = std::max(inkBottom, y);
                }
            }
        }
        QVERIFY2(dark > 20, qPrintable(qsl("the chosen tab holds %1 dark pixels, so its name is not in the sheet's black").arg(QString::number(dark))));
        // A capital in 30px type stands about 21px tall; in the bar's own font about half that
        QVERIFY2(inkBottom - inkTop + 1 > sheetTypePixels / 2,
                 qPrintable(qsl("the name is drawn %1px tall, so not in the sheet's %2px type").arg(QString::number(inkBottom - inkTop + 1), QString::number(sheetTypePixels))));
    }

    // TStyle's proxy base is never given a style, and a QProxyStyle without one builds a fresh
    // native style rather than falling back on the application's
    void test_nothingIsPaintedByABareNativeStyle()
    {
        auto* pTabStyle = qobject_cast<QProxyStyle*>(mpTabBar->style());
        QVERIFY2(pTabStyle && pTabStyle != qApp->style(), "the tab bar wears no proxy style of its own, so there is no base for anything to reach");
        auto* pTrap = new NativeBaseTrap;
        // Owned by the tab bar's style from here, and deleted with it
        pTabStyle->setBaseStyle(pTrap);

        mpTabBar->setCurrentIndex(0);
        mpTabBar->setTabConnectionIndicator(1, TabConnectionIndicator::Connected);
        hoverOver(2);
        TestPixels::shot(mpTabBar);
        mpTabBar->setTabConnectionIndicator(1, TabConnectionIndicator::None);
        QVERIFY2(pTrap->mPainted.isEmpty(), qPrintable(qsl("drawing the strip painted through the bare native style: %1").arg(pTrap->mPainted.join(qsl(", ")))));

        QImage canvas(QSize(40, 20), QImage::Format_ARGB32);
        canvas.fill(Qt::transparent);
        QPainter painter(&canvas);
        QStyleOption option;
        option.initFrom(mpTabBar);
        option.rect = canvas.rect();
        pTabStyle->drawControl(QStyle::CE_FocusFrame, &option, &painter, mpTabBar);
        painter.end();
        QVERIFY2(pTrap->mPainted.isEmpty(), qPrintable(qsl("a control the tab style does not draw itself went to the bare native style: %1").arg(pTrap->mPainted.join(qsl(", ")))));
    }

    void test_theChosenTabStandsOnTheUnderline() { theChosenTabStandsOnTheUnderline(); }

    void test_theChosenWordIsTheAccentInk() { theChosenWordIsTheAccentInk(); }

    void test_theIndicatorIsAStateColour() { theIndicatorIsAStateColour(); }

    // Read as distance from the surface, not a colour match: a 1.5px stroke at 8px leaves no pixel
    // at full coverage, and a cross in the palette's text colour overshoots the chrome tone
    void test_theCrossIsDrawnInChromeInk()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QAbstractButton* pCross = crossOf(2);
        QVERIFY(pCross);
        const QImage drawn = pCross->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const QColor surface = commonestColour(drawn);

        const int chromeReach = TestPixels::distance(surface, tokens.mutedText);
        QVERIFY2(chromeReach > 0, "the chrome tone is the surface itself here, so nothing could be told from a drawn mark");

        int furthest = 0;
        QColor furthestInk;
        QRect markedBox;
        const qreal ratio = drawn.devicePixelRatio();
        for (int y = 0; y < drawn.height(); ++y) {
            for (int x = 0; x < drawn.width(); ++x) {
                const QColor pixel(drawn.pixel(x, y));
                if (TestPixels::distance(surface, pixel) > furthest) {
                    furthest = TestPixels::distance(surface, pixel);
                    furthestInk = pixel;
                }
                if (TestPixels::distance(surface, pixel) > 3 * scmInkTolerance) {
                    markedBox |= QRect(x, y, 1, 1);
                }
            }
        }

        // A platform's own x fills nearly all of the box; the recipe's is scmTabCloseGlyphSize across
        const QSize markSize(qRound(markedBox.width() / ratio), qRound(markedBox.height() / ratio));
        QVERIFY2(std::abs(markSize.width() - uiDesign::scmTabCloseGlyphSize) <= 2 && std::abs(markSize.height() - uiDesign::scmTabCloseGlyphSize) <= 2,
                 qPrintable(qsl("the mark inside the cross's box measures %1x%2 rather than the %3x%3 the recipe draws it at, so it is not this design's x")
                                    .arg(QString::number(markSize.width()), QString::number(markSize.height()), QString::number(uiDesign::scmTabCloseGlyphSize))));

        QVERIFY2(furthest * 2 >= chromeReach,
                 qPrintable(qsl("the cross barely marks its box: the furthest pixel from the surface %1 is %2, only %3 of the %4 the chrome tone %5 would reach")
                                    .arg(saidAs(surface), saidAs(furthestInk), QString::number(furthest), QString::number(chromeReach), saidAs(tokens.mutedText))));
        QVERIFY2(furthest <= chromeReach + 3 * scmInkTolerance,
                 qPrintable(qsl("the cross is inked past the chrome tone %1 - its furthest pixel %2 is %3 from the surface %4, where the chrome tone is %5, so it is drawn in "
                                "something stronger such as the palette's own %6")
                                    .arg(saidAs(tokens.mutedText),
                                         saidAs(furthestInk),
                                         QString::number(furthest),
                                         saidAs(surface),
                                         QString::number(chromeReach),
                                         saidAs(QApplication::palette().color(QPalette::WindowText)))));
    }

    void test_aHoveredTabChangesOnlyItsWordsInk()
    {
        mpTabBar->setCurrentIndex(0);
        hoverOver(1);
        const QImage strip = TestPixels::shot(mpTabBar);
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor surface = commonestColour(strip);

        const QColor beside = TestPixels::sampled(strip, gapSpot(1));
        QVERIFY2(TestPixels::readsAs(beside, surface, 0), qPrintable(qsl("the tab under the pointer is washed: %1 where the strip is %2").arg(saidAs(beside), saidAs(surface))));
        QString why;
        QVERIFY2(carriesNoAccent(strip, *mpTabBar, 1, tokens, &why), qPrintable(qsl("the tab under the pointer reads as chosen: %1").arg(why)));
        const QColor ink = strongestInk(strip, wordBoxOf(*mpTabBar, 1));
        QVERIFY2(TestPixels::distance(ink, tokens.accentText) < TestPixels::distance(ink, tokens.mutedText),
                 qPrintable(qsl("the word under the pointer is written in %1, nearer the quiet %2 than the accent's ink %3").arg(saidAs(ink), saidAs(tokens.mutedText), saidAs(tokens.accentText))));
    }

    void test_bothAppearances()
    {
        theChosenTabStandsOnTheUnderline();
        theChosenWordIsTheAccentInk();
        theIndicatorIsAStateColour();

        DarkTheme theme;
        QPalette dark = QApplication::palette();
        theme.polish(dark);
        QApplication::setPalette(dark);
        QCoreApplication::processEvents();
        QVERIFY2(uiDesign::themeTokens().darkPage, "the dark palette did not take, so the second pass would repeat the first");

        theChosenTabStandsOnTheUnderline();
        theChosenWordIsTheAccentInk();
        theIndicatorIsAStateColour();
    }
};

#include "ProfileTabBarStyleTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileTabBarStyleTest)
