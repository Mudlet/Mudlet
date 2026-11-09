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
 * TStyle paints the profile strip's chips with no stylesheet - fill or wash, word, cross,
 * indicator and their geometry - so only a grab shows them, and in both appearances, since an
 * ink can read in one and vanish in the other.
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

    // A wash is composited rather than blitted, so it agrees with the token mix to a level or two
    static constexpr int scmInkTolerance = 2;
    // Wider than three short names need, so a chip held to its word's width would show
    static constexpr int scmBarWidth = 600;
    // Short of the 7:1 the ink is mixed for: antialiasing leaves only the glyph's stems at full ink
    static constexpr qreal scmChosenWordFloor = 6.5;
    // The same allowance on a filled chip, as a fraction of the white's own contrast on the fill,
    // for a rasteriser that leaves no pixel at full coverage
    static constexpr qreal scmOnFillWordReach = 0.9;

    // Mixed here rather than asked of TStyle, so the test is an independent reading of the rule
    static QColor chosenFillOnLightPage(const uiDesign::ThemeTokens& tokens) { return uiDesign::readableOn(tokens.field, tokens.accent, tokens.text, uiDesign::scmTextMinimumRatio); }

    static QString saidAs(const QColor& colour) { return colour.name(QColor::HexArgb); }

    // A never-shown widget keeps its resize pending until rendered, and QTabBar lays out its tabs
    // on that event, so the chips would be measured for a top-level's default width
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

    // Thresholded against the word's own ink: a fixed distance would count the chosen chip's
    // stronger ink as more word
    static int inkCoverage(const QImage& strip, const QRect& box, const QColor& ink)
    {
        const qreal ratio = strip.devicePixelRatio();
        const QRect scanned = QRect(qRound(box.left() * ratio), qRound(box.top() * ratio), qRound(box.width() * ratio), qRound(box.height() * ratio)) & strip.rect();
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
        const QRect scanned = QRect(qRound(box.left() * ratio), qRound(box.top() * ratio), qRound(box.width() * ratio), qRound(box.height() * ratio)) & strip.rect();
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

    // Mirrors TStyle::layoutChip()
    QRect chipOf(const int index) const { return mpTabBar->tabRect(index).adjusted(0, 0, -uiDesign::scmTabGap, 0); }

    // Past the accent bar a chosen chip carries, where only the wash is drawn
    QPoint washSpot(const int index) const
    {
        const QRect chip = chipOf(index);
        return QPoint(chip.left() + uiDesign::scmAccentBarWidth + 2, chip.center().y());
    }

    // Mid-height on the accent bar, clear of the arc at either end of it
    QPoint barSpot(const int index) const
    {
        const QRect chip = chipOf(index);
        return QPoint(chip.left() + 1, chip.center().y());
    }

    QRect indicatorOf(const int index) const
    {
        const QRect chip = chipOf(index);
        return QRect(chip.left() + uiDesign::scmInputBorderWidth + uiDesign::scmTabPaddingHorizontal,
                     chip.top() + (chip.height() - TStyle::sIndicatorDiameter) / 2,
                     TStyle::sIndicatorDiameter,
                     TStyle::sIndicatorDiameter);
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

    void theChosenChipIsWashed()
    {
        const QImage strip = TestPixels::shot(mpTabBar);
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor surface = commonestColour(strip);

        const QColor resting = TestPixels::sampled(strip, washSpot(2));
        QVERIFY2(TestPixels::readsAs(resting, surface, 0),
                 qPrintable(qsl("a tab that is not the one on show is drawn on something other than the strip's own surface: %1 rather than %2").arg(saidAs(resting), saidAs(surface))));
        const QColor restingEdge = TestPixels::sampled(strip, barSpot(2));
        QVERIFY2(TestPixels::readsAs(restingEdge, surface, 0), qPrintable(qsl("a tab that is not on show carries a bar on its leading edge: %1").arg(saidAs(restingEdge))));
        const QRect restingChip = chipOf(2);
        const QColor restingTop = TestPixels::sampled(strip, QPoint(restingChip.center().x(), restingChip.top()));
        QVERIFY2(TestPixels::readsAs(restingTop, surface, 0),
                 qPrintable(qsl("a tab that is not on show is outlined: %1 on its top edge rather than the strip's own %2").arg(saidAs(restingTop), saidAs(surface))));

        const QRect chosenChip = chipOf(0);
        if (!tokens.darkPage) {
            // No bar or outline on a light page: in the accent they would say nothing on the accent fill
            const QColor fill = chosenFillOnLightPage(tokens);
            struct Spot
            {
                const char* where;
                QPoint at;
            };
            const Spot spots[]{{"inside its leading padding", washSpot(0)}, {"where a bar would stand", barSpot(0)}, {"on its top edge", QPoint(chosenChip.center().x(), chosenChip.top())}};
            for (const Spot& spot : spots) {
                const QColor read = TestPixels::sampled(strip, spot.at);
                QVERIFY2(TestPixels::readsAs(read, fill, scmInkTolerance),
                         qPrintable(qsl("the tab on show is not filled with %1, the accent %2 walked until the field's white reads on it: %3 it is %4")
                                            .arg(saidAs(fill), saidAs(tokens.accent), QString::fromUtf8(spot.where), saidAs(read))));
            }
            return;
        }

        const QColor chosen = TestPixels::sampled(strip, washSpot(0));
        const QColor wanted = TestPixels::composited(tokens.accentWash, surface);
        QVERIFY2(TestPixels::readsAs(chosen, wanted, scmInkTolerance),
                 qPrintable(qsl("the tab on show is not washed in the accent: %1 where the accent wash over %2 is %3").arg(saidAs(chosen), saidAs(surface), saidAs(wanted))));

        // On a dark page the wash alone does not say which profile is on show
        const QColor bar = TestPixels::sampled(strip, barSpot(0));
        QVERIFY2(TestPixels::readsAs(bar, tokens.accent, scmInkTolerance),
                 qPrintable(qsl("the tab on show has no accent bar on its leading edge: %1 where the accent is %2").arg(saidAs(bar), saidAs(tokens.accent))));

        const QColor edge = TestPixels::sampled(strip, QPoint(chosenChip.center().x(), chosenChip.top()));
        QVERIFY2(TestPixels::readsAs(edge, tokens.accent, scmInkTolerance),
                 qPrintable(qsl("the tab on show has no accent outline: %1 on its top edge where the accent is %2").arg(saidAs(edge), saidAs(tokens.accent))));
    }

    // The accent ink at the design's floor was mixed against the deepest wash and fades into this
    // strip's own, so the dark-page word is walked past it. The strongest pixel is read because
    // only the glyph's stems are at full ink.
    void theChosenWordIsStrong()
    {
        const QImage strip = TestPixels::shot(mpTabBar);
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor behind = tokens.darkPage ? TestPixels::composited(tokens.accentWash, commonestColour(strip)) : chosenFillOnLightPage(tokens);

        QStyleOptionTab option;
        mpTabBar->initStyleOption(&option, 0);
        const QRect word = mpTabBar->style()->subElementRect(QStyle::SE_TabBarTabText, &option, mpTabBar);
        const qreal ratio = strip.devicePixelRatio();
        const QRect scanned = QRect(qRound(word.left() * ratio), qRound(word.top() * ratio), qRound(word.width() * ratio), qRound(word.height() * ratio)) & strip.rect();
        QVERIFY2(!scanned.isEmpty(), "the chosen chip leaves its word no room at all, so there is nothing to read");

        qreal strongest = 0.0;
        QColor strongestInk;
        for (int y = scanned.top(); y <= scanned.bottom(); ++y) {
            for (int x = scanned.left(); x <= scanned.right(); ++x) {
                const QColor pixel(strip.pixel(x, y));
                const qreal reads = uiDesign::contrastRatio(pixel, behind);
                if (reads > strongest) {
                    strongest = reads;
                    strongestInk = pixel;
                }
            }
        }

        if (!tokens.darkPage) {
            const qreal inkReads = uiDesign::contrastRatio(tokens.field, behind);
            const bool atFullInk = TestPixels::readsAs(strongestInk, tokens.field, scmInkTolerance);
            if (!atFullInk) {
                qDebug("no pixel of the word is at the field's own white, so it is read by how far it reaches instead");
            }
            QVERIFY2(atFullInk || strongest >= scmOnFillWordReach * inkReads,
                     qPrintable(qsl("the word on the tab on show is not written in the field's white %1 on the fill %2: its strongest pixel is %3, reaching %4:1, where that white reads at %5:1")
                                        .arg(saidAs(tokens.field), saidAs(behind), saidAs(strongestInk), QString::number(strongest, 'f', 2), QString::number(inkReads, 'f', 2))));
            return;
        }

        const QColor walked = uiDesign::readableOn(behind, tokens.accentText, tokens.text, TStyle::sChosenWordMinimumRatio);
        QVERIFY2(strongest >= scmChosenWordFloor,
                 qPrintable(qsl("the word on the tab on show reaches only %1:1 against its wash %2 - its strongest pixel is %3, where the ink at the design's floor %4 reads at %5:1 and the "
                                "walked ink %6 at %7:1")
                                    .arg(QString::number(strongest, 'f', 2),
                                         saidAs(behind),
                                         saidAs(strongestInk),
                                         saidAs(tokens.accentText),
                                         QString::number(uiDesign::contrastRatio(tokens.accentText, behind), 'f', 2),
                                         saidAs(walked),
                                         QString::number(uiDesign::contrastRatio(walked, behind), 'f', 2))));
    }

    void theIndicatorIsAStateColour()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

        // Tab 2 rests on the page; the chosen chip's light-page fill is checked separately below
        mpTabBar->setTabConnectionIndicator(2, TabConnectionIndicator::Connected);
        QImage strip = TestPixels::shot(mpTabBar);
        const QColor dot = TestPixels::sampled(strip, indicatorOf(2).center());
        const QColor wantedDot = uiDesign::readableOn(tokens.page, uiDesign::stateColor(uiDesign::scmStateHue_ok, tokens.darkPage), tokens.text, uiDesign::scmQuietMinimumRatio);
        QVERIFY2(TestPixels::readsAs(dot, wantedDot, scmInkTolerance),
                 qPrintable(qsl("a connected profile's dot is not the state colour for something working: %1 rather than %2").arg(saidAs(dot), saidAs(wantedDot))));

        mpTabBar->setTabConnectionIndicator(2, TabConnectionIndicator::Disconnected);
        strip = TestPixels::shot(mpTabBar);
        const QRect box = indicatorOf(2);
        const QColor ring = TestPixels::sampled(strip, QPoint(box.left(), box.center().y()));
        QVERIFY2(TestPixels::readsAs(ring, tokens.mutedText, scmInkTolerance),
                 qPrintable(qsl("a disconnected profile's ring is not drawn in the chrome tone: %1 rather than %2").arg(saidAs(ring), saidAs(tokens.mutedText))));

        mpTabBar->setTabConnectionIndicator(2, TabConnectionIndicator::None);
        QCoreApplication::processEvents();

        if (tokens.darkPage) {
            return;
        }

        // A dot walked for the page, or a ring in the chrome tone, would be swallowed by a light page's fill
        const QColor fill = chosenFillOnLightPage(tokens);
        mpTabBar->setTabConnectionIndicator(0, TabConnectionIndicator::Connected);
        strip = TestPixels::shot(mpTabBar);
        const QColor chosenDot = TestPixels::sampled(strip, indicatorOf(0).center());
        const QColor wantedChosenDot = uiDesign::readableOn(fill, uiDesign::stateColor(uiDesign::scmStateHue_ok, tokens.darkPage), tokens.text, uiDesign::scmQuietMinimumRatio);
        QVERIFY2(TestPixels::readsAs(chosenDot, wantedChosenDot, scmInkTolerance),
                 qPrintable(qsl("the dot on the tab on show is not the state colour walked against the fill %1 it stands on: %2 rather than %3")
                                    .arg(saidAs(fill), saidAs(chosenDot), saidAs(wantedChosenDot))));

        mpTabBar->setTabConnectionIndicator(0, TabConnectionIndicator::Disconnected);
        strip = TestPixels::shot(mpTabBar);
        const QRect chosenBox = indicatorOf(0);
        const QColor chosenRing = TestPixels::sampled(strip, QPoint(chosenBox.left(), chosenBox.center().y()));
        QVERIFY2(TestPixels::readsAs(chosenRing, tokens.field, scmInkTolerance),
                 qPrintable(qsl("the ring on the tab on show is not drawn in the field's white its word is: %1 rather than %2").arg(saidAs(chosenRing), saidAs(tokens.field))));

        mpTabBar->setTabConnectionIndicator(0, TabConnectionIndicator::None);
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
        // A top-level here, with no main window frame to fill the surface behind the chips
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
        }

        QVERIFY2(!mpTabBar->drawBase(), "the strip still draws the band a platform fills the whole bar with behind the tabs");
        QVERIFY2(mpTabBar->expanding(), "the chips are held to the width of their words rather than sharing the width of the window, as the profile strip always has");
        QVERIFY2(mpTabBar->testAttribute(Qt::WA_Hover), "the bar asks for no hover events, so no chip can ever be washed under the pointer");
    }

    // Guards the platform tabs' light-mode cross sitting visibly higher than the word
    void test_everythingIsCentredOnTheChip()
    {
        for (int i = 0; i < mpTabBar->count(); ++i) {
            const int chipCentre = mpTabBar->tabRect(i).center().y();

            QAbstractButton* pCross = crossOf(i);
            QVERIFY(pCross);
            const int crossCentre = pCross->geometry().center().y();
            QVERIFY2(std::abs(crossCentre - chipCentre) <= 1,
                     qPrintable(qsl("the cross on tab %1 sits %2px off the chip's centre line, where the word is").arg(QString::number(i), QString::number(std::abs(crossCentre - chipCentre)))));

            QStyleOptionTab option;
            mpTabBar->initStyleOption(&option, i);
            const QRect text = mpTabBar->style()->subElementRect(QStyle::SE_TabBarTabText, &option, mpTabBar);
            QVERIFY2(std::abs(text.center().y() - chipCentre) <= 1,
                     qPrintable(qsl("the word on tab %1 sits %2px off the chip's centre line").arg(QString::number(i), QString::number(std::abs(text.center().y() - chipCentre)))));
            QVERIFY2(text.right() < pCross->geometry().left(), qPrintable(qsl("the word on tab %1 runs under the cross rather than stopping in front of it").arg(QString::number(i))));
        }
    }

    void test_theChipIsTheRecipesSize()
    {
        const int wantedHeight = mpTabBar->fontMetrics().height() + 2 * (uiDesign::scmTabPaddingVertical + uiDesign::scmInputBorderWidth);
        QCOMPARE(mpTabBar->tabSizeHint(0).height(), wantedHeight);

        QCOMPARE(mpTabBar->tabRect(0).left(), 0);
        for (int i = 1; i < mpTabBar->count(); ++i) {
            QVERIFY2(mpTabBar->tabRect(i).left() == mpTabBar->tabRect(i - 1).right() + 1,
                     qPrintable(qsl("chip %1 starts at %2 rather than where the one before it ends, %3 - the gap between them is inside a chip's own rectangle")
                                        .arg(QString::number(i), QString::number(mpTabBar->tabRect(i).left()), QString::number(mpTabBar->tabRect(i - 1).right() + 1))));
        }

        const int lastEdge = mpTabBar->tabRect(mpTabBar->count() - 1).right();
        QVERIFY2(lastEdge == mpTabBar->width() - 1,
                 qPrintable(qsl("three chips end at %1 on a %2px bar, so they are not sharing its width between them").arg(QString::number(lastEdge), QString::number(mpTabBar->width()))));
    }

    void test_theChosenWordIsBoldAndTheStripDoesNotShiftForIt()
    {
        // The same word on both chips, so only the choice can differ between the two readings
        ExposedTabBar bar(nullptr);
        bar.setAutoFillBackground(true);
        bar.setTabData(bar.addTab(qsl("Same")), qsl("Same-A"));
        bar.setTabData(bar.addTab(qsl("Same")), qsl("Same-B"));
        bar.resize(scmBarWidth, bar.sizeHint().height());
        layOutAtItsOwnSize(&bar);
        bar.setCurrentIndex(0);
        QCoreApplication::processEvents();

        const auto wordBox = [&bar](const int index) {
            QStyleOptionTab option;
            bar.initStyleOption(&option, index);
            return bar.style()->subElementRect(QStyle::SE_TabBarTabText, &option, &bar);
        };

        const QImage strip = bar.grab().toImage().convertToFormat(QImage::Format_ARGB32);

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        // Each word's own ink, or the reading counts the whole box as word
        const int chosenInk = inkCoverage(strip, wordBox(0), tokens.darkPage ? tokens.accentText : tokens.field);
        const int restingInk = inkCoverage(strip, wordBox(1), tokens.mutedText);
        QVERIFY2(restingInk > 0, "the resting chip's word marks nothing at all, so there is no reading to compare the chosen one against");
        QVERIFY2(100 * chosenInk >= 115 * restingInk,
                 qPrintable(qsl("the word on the chosen chip marks %1 pixels of its box against the %2 the same word marks on the resting chip beside it, so it is not drawn any bolder")
                                    .arg(QString::number(chosenInk), QString::number(restingInk))));

        // A pixel of slack: Qt measures a tab's text with QFontMetrics::size(), this with horizontalAdvance()
        QFont bold = bar.font();
        bold.setBold(true);
        for (int i = 0; i < bar.count(); ++i) {
            const int boldAdvance = QFontMetrics(bold).horizontalAdvance(bar.tabText(i));
            QVERIFY2(boldAdvance > bar.fontMetrics().horizontalAdvance(bar.tabText(i)), "bold and regular measure this word the same here, so nothing below could be told apart");
            QVERIFY2(wordBox(i).width() + 1 >= boldAdvance,
                     qPrintable(qsl("chip %1 leaves its word %2px, less than the %3px that word takes in bold - so the chip a reader chooses is measured narrower than it is drawn")
                                        .arg(QString::number(i), QString::number(wordBox(i).width()), QString::number(boldAdvance))));
        }

        QList<QRect> before;
        for (int i = 0; i < bar.count(); ++i) {
            before << bar.tabRect(i);
        }
        bar.setCurrentIndex(1);
        QCoreApplication::processEvents();
        for (int i = 0; i < bar.count(); ++i) {
            QVERIFY2(bar.tabRect(i) == before.at(i),
                     qPrintable(qsl("chip %1 stood at %2 and moved to %3 when the choice moved to the chip beside it, so the whole strip steps sideways on every switch")
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
        // The same displayed word on both resting chips, so only the mark can tell them apart
        bar.setTabData(bar.addTab(qsl("&Gorbash")), qsl("Gorbash"));
        bar.setTabData(bar.addTab(qsl("&Gorbash")), qsl("Gorbash-Twin"));
        bar.resize(scmBarWidth, bar.sizeHint().height());
        layOutAtItsOwnSize(&bar);
        bar.setCurrentIndex(0);

        bar.markActivity(qsl("Gorbash"), false);
        QCoreApplication::processEvents();

        const auto wordBox = [&bar](const int index) {
            QStyleOptionTab option;
            bar.initStyleOption(&option, index);
            return bar.style()->subElementRect(QStyle::SE_TabBarTabText, &option, &bar);
        };
        const QImage strip = bar.grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const QColor ink = uiDesign::themeTokens().mutedText;
        const int markedInk = inkCoverage(strip, wordBox(1), ink);
        const int unmarkedInk = inkCoverage(strip, wordBox(2), ink);
        QVERIFY2(unmarkedInk > 0, "the unmarked chip's word marks nothing at all, so there is no reading to compare the marked one against");
        QVERIFY2(100 * markedInk >= 115 * unmarkedInk,
                 qPrintable(qsl("the word on the chip with new output marks %1 pixels of its box against the %2 the same word marks on the chip beside it, so the mark never reached the "
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

        const QImage strip = bar.grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const int inked = inkedWidth(strip, bar.tabRect(index), uiDesign::themeTokens().mutedText);
        QVERIFY2(inked + bearingSlack >= boldAdvance,
                 qPrintable(qsl("the marked name inks %1px of the %2px it takes in bold (%3px in plain), so it is clipped or drawn without the mark")
                                    .arg(QString::number(inked), QString::number(boldAdvance), QString::number(plainAdvance))));
#endif
    }

    void test_anApplicationSheetNamingTheTabBarTakesTheTabs()
    {
        const auto restoreSheet = qScopeGuard([this]() {
            qApp->setStyleSheet(QString());
            mpTabBar->refreshAfterApplicationStyleChange();
        });
        mpTabBar->setCurrentIndex(0);
        const int chipWidth = mpTabBar->tabSizeHint(0).width();
        constexpr int sheetWidth = 500;
        QVERIFY2(chipWidth < sheetWidth, "the chip is already as wide as the sheet below asks for, so nothing could be told from its width");

        qApp->setStyleSheet(qsl("QPushButton { min-width: %1px; }").arg(QString::number(sheetWidth)));
        mpTabBar->refreshAfterApplicationStyleChange();
        QVERIFY2(mpTabBar->tabSizeHint(0).width() == chipWidth,
                 qPrintable(qsl("a sheet about push buttons changed the chip's width from %1 to %2px").arg(QString::number(chipWidth), QString::number(mpTabBar->tabSizeHint(0).width()))));
        theChosenChipIsWashed();
        if (QTest::currentTestFailed()) {
            return;
        }

        qApp->setStyleSheet(qsl("QTabBar::tab { min-width: %1px; }").arg(QString::number(sheetWidth)));
        mpTabBar->refreshAfterApplicationStyleChange();
        QVERIFY2(mpTabBar->tabSizeHint(0).width() >= sheetWidth,
                 qPrintable(qsl("the tab is %1px wide under a sheet asking for at least %2px, so the chips kept the tabs from it")
                                    .arg(QString::number(mpTabBar->tabSizeHint(0).width()), QString::number(sheetWidth))));
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

    void test_theChosenChipIsWashedInTheAccent() { theChosenChipIsWashed(); }

    void test_theChosenWordIsWalkedPastTheFloor() { theChosenWordIsStrong(); }

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
        QRect marked;
        const qreal ratio = drawn.devicePixelRatio();
        for (int y = 0; y < drawn.height(); ++y) {
            for (int x = 0; x < drawn.width(); ++x) {
                const QColor pixel(drawn.pixel(x, y));
                if (TestPixels::distance(surface, pixel) > furthest) {
                    furthest = TestPixels::distance(surface, pixel);
                    furthestInk = pixel;
                }
                if (TestPixels::distance(surface, pixel) > 3 * scmInkTolerance) {
                    marked |= QRect(x, y, 1, 1);
                }
            }
        }

        // A platform's own x fills nearly all of the box; the recipe's is scmTabCloseGlyphSize across
        const QSize markSize(qRound(marked.width() / ratio), qRound(marked.height() / ratio));
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

        if (tokens.darkPage) {
            return;
        }

        // The chrome tone would vanish on a light page's fill. Read off the strip, since the button's
        // own grab paints the page behind its mark rather than the chip under it.
        QAbstractButton* pChosenCross = crossOf(0);
        QVERIFY(pChosenCross);
        const QImage strip = TestPixels::shot(mpTabBar);
        const qreal stripRatio = strip.devicePixelRatio();
        const QRect box = pChosenCross->geometry();
        const QRect scanned = QRect(qRound(box.left() * stripRatio), qRound(box.top() * stripRatio), qRound(box.width() * stripRatio), qRound(box.height() * stripRatio)) & strip.rect();
        QVERIFY2(!scanned.isEmpty(), "the cross on the tab on show stands outside the strip, so there is nothing to read");

        const QColor fill = chosenFillOnLightPage(tokens);
        const QColor filled = commonestColour(strip.copy(scanned));
        QVERIFY2(TestPixels::readsAs(filled, fill, scmInkTolerance),
                 qPrintable(qsl("the cross on the tab on show is not standing on the fill: what is behind it is %1 rather than %2").arg(saidAs(filled), saidAs(fill))));

        const int inkReach = TestPixels::distance(fill, tokens.field);
        QVERIFY2(inkReach > 0, "the field's white is the fill itself here, so nothing could be told from a drawn mark");

        int chosenFurthest = 0;
        QColor chosenFurthestInk;
        for (int y = scanned.top(); y <= scanned.bottom(); ++y) {
            for (int x = scanned.left(); x <= scanned.right(); ++x) {
                const QColor pixel(strip.pixel(x, y));
                if (TestPixels::distance(fill, pixel) > chosenFurthest) {
                    chosenFurthest = TestPixels::distance(fill, pixel);
                    chosenFurthestInk = pixel;
                }
            }
        }

        QVERIFY2(chosenFurthest * 2 >= inkReach,
                 qPrintable(qsl("the cross on the tab on show barely marks the fill %1: its furthest pixel %2 gets %3 from it, only part of the %4 the field's white %5 would reach")
                                    .arg(saidAs(fill), saidAs(chosenFurthestInk), QString::number(chosenFurthest), QString::number(inkReach), saidAs(tokens.field))));
        // Past what the chrome tone reaches on the fill even at full coverage, which tells a cross
        // written for the fill from one still in the page's grey
        const int chromeOnTheFill = TestPixels::distance(fill, tokens.mutedText);
        QVERIFY2(chosenFurthest > chromeOnTheFill + scmInkTolerance,
                 qPrintable(qsl("the cross on the tab on show is still inked in the chrome tone %1 rather than in the field's white %2: its furthest pixel %3 gets %4 from the fill, where the "
                                "chrome tone itself gets %5")
                                    .arg(saidAs(tokens.mutedText), saidAs(tokens.field), saidAs(chosenFurthestInk), QString::number(chosenFurthest), QString::number(chromeOnTheFill))));
        QVERIFY2(chosenFurthest <= inkReach + 3 * scmInkTolerance,
                 qPrintable(qsl("the cross on the tab on show is inked past the field's white %1 on the fill %2: its furthest pixel %3 is %4 from the fill, where that white is %5")
                                    .arg(saidAs(tokens.field), saidAs(fill), saidAs(chosenFurthestInk), QString::number(chosenFurthest), QString::number(inkReach))));
    }

    void test_theHoveredChipIsWashed()
    {
        hoverOver(1);
        const QImage strip = TestPixels::shot(mpTabBar);
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor surface = commonestColour(strip);

        const QColor hovered = TestPixels::sampled(strip, washSpot(1));
        const QColor wanted = TestPixels::composited(tokens.hoverWash, surface);
        QVERIFY2(TestPixels::readsAs(hovered, wanted, scmInkTolerance),
                 qPrintable(qsl("the chip under the pointer is not washed: %1 where the hover wash over %2 is %3").arg(saidAs(hovered), saidAs(surface), saidAs(wanted))));
    }

    void test_bothAppearances()
    {
        theChosenChipIsWashed();
        theChosenWordIsStrong();
        theIndicatorIsAStateColour();

        DarkTheme theme;
        QPalette dark = QApplication::palette();
        theme.polish(dark);
        QApplication::setPalette(dark);
        QCoreApplication::processEvents();
        QVERIFY2(uiDesign::themeTokens().darkPage, "the dark palette did not take, so the second pass would repeat the first");

        theChosenChipIsWashed();
        theChosenWordIsStrong();
        theIndicatorIsAStateColour();
    }
};

#include "ProfileTabBarStyleTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileTabBarStyleTest)
