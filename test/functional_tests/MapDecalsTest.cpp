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
 * What a map style draws on and around rooms, and which of the profile's
 * settings it themes, decided without GL and with no Lua entry point.
 */

#include <QtTest/QtTest>

#include <QImage>

#include "Host.h"
#include "MapDecals.h"
#include "MapStyle.h"
#include "TMapLabel.h"

#include "GroupedTest.h"

namespace {
constexpr int scmSize = 64;

int alphaAt(const QImage& image, const int x, const int y)
{
    return qAlpha(image.pixel(x, y));
}

QRect paintedArea(const QImage& image)
{
    QRect area;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (alphaAt(image, x, y) > 128) {
                area |= QRect(x, y, 1, 1);
            }
        }
    }
    return area;
}

const MapStyle& tiles()
{
    return *MapStyle::find(qsl("tiles"));
}

const MapStyle& blueprint()
{
    return *MapStyle::find(qsl("blueprint"));
}

TMapLabel textLabel(const QColor& fg, const QColor& bg)
{
    TMapLabel label;
    label.text = qsl("Market Square");
    label.fgColor = fg;
    label.bgColor = bg;
    label.outlineColor = QColorConstants::Black;
    label.font = QFont(QString(), 30);
    label.size = QSizeF(4.0, 1.0);
    QPixmap pixmap(200, 50);
    pixmap.fill(Qt::black);
    label.pix = pixmap;
    return label;
}
} // namespace

class MapDecalsTest : public QObject
{
    Q_OBJECT

private slots:
    void test_softPaletteKeepsTheHueButLiftsAndMutes()
    {
        MapStyle style;
        style.palette = MapStyle::Palette::Soft;
        const QColor red = style.roomColor(QColor(200, 0, 0));
        QCOMPARE(red.hslHue(), 0);
        QVERIFY(red.hslSaturationF() < 0.8f);
        QVERIFY(red.lightnessF() > QColor(200, 0, 0).lightnessF());
        QVERIFY2(style.roomColor(Qt::black).lightness() > 40, "black rooms stay apart from a dark background");
        QVERIFY2(style.roomColor(Qt::white).lightness() < 240, "white rooms are toned down");
        QCOMPARE(MapStyle().roomColor(QColor(200, 0, 0)), QColor(200, 0, 0));
    }

    void test_defaultBackgroundOnlyReplacesBlack()
    {
        MapStyle style;
        style.defaultBackground = QColor(16, 22, 32);
        QCOMPARE(style.backgroundColor(Qt::black), QColor(16, 22, 32));
        QCOMPARE(style.backgroundColor(QColor(40, 0, 0)), QColor(40, 0, 0));
        QCOMPARE(MapStyle().backgroundColor(Qt::black), QColor(Qt::black));
    }

    void test_playerMarkerIsThemedOnlyOverTheOriginalStyle()
    {
        QVERIFY(tiles().themesPlayerMarker(0, false));
        QVERIFY2(!tiles().themesPlayerMarker(1, false), "a player room style picked in the profile is kept");
        QVERIFY2(!tiles().themesPlayerMarker(0, true), "the strong highlight is kept");
        QVERIFY2(!MapStyle().themesPlayerMarker(0, false), "a style without a ring or color keeps the profile's marker");
    }

    void test_infoBackgroundIsThemedOnlyWhereTheProfileKeepsTheDefault()
    {
        const QColor profileDefault = Host::scmDefaultMapInfoBg;
        QCOMPARE(tiles().infoBackgroundColor(profileDefault), tiles().infoBackground);
        QCOMPARE(tiles().infoBackgroundColor(QColor(0, 80, 0, 200)), QColor(0, 80, 0, 200));
        QCOMPARE(MapStyle().infoBackgroundColor(profileDefault), profileDefault);
    }

    void test_roomNamesAreQuieterButKeepAColorGivenThem()
    {
        const QColor background(16, 22, 32);
        QCOMPARE(tiles().roomNameColorFor(QString(), background), tiles().roomNameColor);
        QVERIFY(tiles().roomNameColor.alpha() < 255);
        QVERIFY(tiles().roomNameScale < 1.0f);
        QCOMPARE(tiles().roomNameColorFor(qsl("#ff8000"), background), QColor(255, 128, 0));
        QCOMPARE(MapStyle().roomNameColorFor(QString(), background), QColor(Qt::white));
    }

    void test_defaultRoomBordersBecomeTheBlocksBase()
    {
        const QColor room(200, 100, 40);
        const QColor background(0, 20, 40);
        QCOMPARE(tiles().roomBorderColor(Host::scmDefaultRoomBorderColor, room, background).rgb(), QColor(100, 60, 40).rgb());
        QCOMPARE(tiles().roomBorderColor(QColor(Qt::yellow), room, background), QColor(Qt::yellow));
        QCOMPARE(MapStyle().roomBorderColor(Host::scmDefaultRoomBorderColor, room, background), Host::scmDefaultRoomBorderColor);
    }

    void test_glowFadesOutFromTheMiddle()
    {
        const QImage image = MapDecals::highlight(MapStyle::HighlightShape::Glow, Qt::red, Qt::white, false, scmSize);
        QVERIFY(alphaAt(image, scmSize / 2, scmSize / 2) > 150);
        QVERIFY(alphaAt(image, scmSize / 2, scmSize / 8) < alphaAt(image, scmSize / 2, scmSize / 3));
        QCOMPARE(alphaAt(image, 0, 0), 0);
        QCOMPARE(QColor(image.pixel(scmSize / 2, scmSize / 2)).hue(), 0);
        QVERIFY(MapDecals::highlightOnGround(MapStyle::HighlightShape::Glow));
    }

    void test_ringShapesAreClearInTheMiddle_data()
    {
        QTest::addColumn<MapStyle::HighlightShape>("shape");
        QTest::newRow("dashed ellipse") << MapStyle::HighlightShape::DashedEllipse;
        QTest::newRow("double ellipse") << MapStyle::HighlightShape::DoubleEllipse;
        QTest::newRow("halo") << MapStyle::HighlightShape::Halo;
        QTest::newRow("rings") << MapStyle::HighlightShape::Rings;
    }

    void test_ringShapesAreClearInTheMiddle()
    {
        QFETCH(MapStyle::HighlightShape, shape);
        const QImage image = MapDecals::highlight(shape, Qt::yellow, Qt::white, false, scmSize);
        QVERIFY(alphaAt(image, scmSize / 2, scmSize / 2) < 60);
        int strongest = 0;
        for (int y = 0; y < scmSize; ++y) {
            for (int x = 0; x < scmSize; ++x) {
                strongest = qMax(strongest, alphaAt(image, x, y));
            }
        }
        QVERIFY2(strongest > 100, "the ring itself shows");
    }

    void test_cellFillsTheRoom()
    {
        const QImage image = MapDecals::highlight(MapStyle::HighlightShape::Cell, Qt::yellow, Qt::white, false, scmSize);
        QVERIFY(alphaAt(image, 0, 0) > 150);
        QVERIFY(alphaAt(image, scmSize - 1, scmSize - 1) > 150);
        const QImage round = MapDecals::highlight(MapStyle::HighlightShape::Cell, Qt::yellow, Qt::white, true, scmSize);
        QCOMPARE(alphaAt(round, 0, 0), 0);
        QVERIFY(alphaAt(round, scmSize / 2, scmSize / 2) > 150);
        QVERIFY(MapDecals::highlightFillsRoom(MapStyle::HighlightShape::Cell));
        QVERIFY(!MapDecals::highlightFillsRoom(MapStyle::HighlightShape::Glow));
    }

    void test_thinPlayerRingIsClearInside()
    {
        const QGradientStops stops = MapDecals::thinPlayerRingStops(QColor(255, 208, 90));
        QCOMPARE(stops.constFirst().second.alpha(), 0);
        bool opaque = false;
        for (const QGradientStop& stop : stops) {
            opaque = opaque || (stop.first > 0.7 && stop.second.alpha() == 255);
        }
        QVERIFY(opaque);
    }

    void test_blockSheenIsLitFromTheNorthWest()
    {
        // The image's first row is drawn to the map's north
        const QImage sheen = MapDecals::blockSheen(false, scmSize);
        const QColor northWest = QColor::fromRgba(sheen.pixel(2, 2));
        const QColor southEast = QColor::fromRgba(sheen.pixel(scmSize - 3, scmSize - 3));
        QVERIFY(northWest.lightness() > 200 && northWest.alpha() > 40);
        QVERIFY(southEast.lightness() < 50 && southEast.alpha() > 30);
    }

    void test_blockShadowDarkensTowardItsMiddle()
    {
        const QImage shadow = MapDecals::blockShadow(true, scmSize);
        QVERIFY(alphaAt(shadow, scmSize / 2, scmSize / 2) > alphaAt(shadow, scmSize / 2, scmSize / 6));
        QCOMPARE(alphaAt(shadow, 0, 0), 0);
    }

    void test_defaultLabelIsDrawnInTheStyleColorWithoutItsBox()
    {
        const TMapLabel label = textLabel(Qt::white, QColor(0, 0, 0, 50));
        const QImage image = MapDecals::themedLabel(label, tiles());
        QVERIFY(!image.isNull());
        QCOMPARE(alphaAt(image, 0, 0), 0);
        bool styleColor = false;
        for (int y = 0; y < image.height() && !styleColor; ++y) {
            for (int x = 0; x < image.width() && !styleColor; ++x) {
                const QColor pixel = QColor::fromRgba(image.pixel(x, y));
                styleColor = pixel.alpha() == 255 && pixel.rgb() == tiles().mapLabelColor.rgb();
            }
        }
        QVERIFY(styleColor);
    }

    void test_labelKeepsColorsPickedForIt()
    {
        const TMapLabel label = textLabel(QColor(220, 30, 30), QColor(20, 40, 200));
        const QImage image = MapDecals::themedLabel(label, tiles());
        QVERIFY(!image.isNull());
        QCOMPARE(QColor::fromRgba(image.pixel(0, 0)), QColor(20, 40, 200));
        bool red = false;
        for (int y = 0; y < image.height() && !red; ++y) {
            for (int x = 0; x < image.width() && !red; ++x) {
                red = QColor(image.pixel(x, y)) == QColor(220, 30, 30);
            }
        }
        QVERIFY(red);
    }

    void test_labelsAreLeftAloneByAnUnthemedStyle()
    {
        QVERIFY(MapDecals::themedLabel(textLabel(Qt::white, Qt::black), MapStyle()).isNull());
        TMapLabel picture = textLabel(Qt::white, Qt::black);
        picture.text.clear();
        QVERIFY(MapDecals::themedLabel(picture, tiles()).isNull());
    }

    void test_themedLabelKeepsItsPictureSizeAndShrinksItsTextToTheLimit()
    {
        const TMapLabel label = textLabel(Qt::white, Qt::black);
        MapStyle style;
        style.themedMapLabels = true;
        const QImage full = MapDecals::themedLabel(label, style);
        QCOMPARE(full.size(), label.pix.size());
        style.mapLabelMaxHeight = 0.25f;
        const QImage capped = MapDecals::themedLabel(label, style);
        QCOMPARE(capped.size(), label.pix.size());
        const QRect fullText = paintedArea(full);
        const QRect cappedText = paintedArea(capped);
        QVERIFY2(cappedText.height() <= 0.25 * label.pix.height() + 2, qPrintable(qsl("%1 pixels tall").arg(cappedText.height())));
        QVERIFY(cappedText.height() < fullText.height());
        QVERIFY2(qAbs(cappedText.center().x() - fullText.center().x()) <= 3, "the text stays centered where the label was put");
    }

    void test_registrationMarkCrossesItsRingAndRunsPastIt()
    {
        const QImage mark = MapDecals::registrationMark(QColor(255, 112, 90), scmSize);
        const int middle = scmSize / 2;
        QVERIFY2(alphaAt(mark, middle, middle) > 150, "a dot marks the middle");
        QVERIFY2(alphaAt(mark, 3, middle) > 100 && alphaAt(mark, middle, scmSize - 4) > 100, "the cross runs on past the ring");
        const int diagonal = qRound(0.52 * middle * M_SQRT1_2);
        QVERIFY2(alphaAt(mark, middle + diagonal, middle - diagonal) > 60, "the ring shows between the arms");
        QCOMPARE(alphaAt(mark, 4, 4), 0);
        QCOMPARE(alphaAt(mark, middle - 6, middle - 6), 0);
    }

    void test_calloutsAreSpreadApartInOrderAndKeptInView()
    {
        const QList<qreal> placed = MapDecals::spreadCallouts({50.0, 100.0, 100.0, 90.0, 400.0}, 20.0, 0.0, 1000.0);
        QCOMPARE(placed, (QList<qreal>{50.0, 100.0, 120.0, 140.0, 400.0}));

        const QList<qreal> squeezed = MapDecals::spreadCallouts({190.0, 195.0, 199.0}, 20.0, 0.0, 200.0);
        QCOMPARE(squeezed.at(2), 200.0);
        QCOMPARE(squeezed.at(1), 180.0);
        QCOMPARE(squeezed.at(0), 160.0);

        const QList<qreal> lifted = MapDecals::spreadCallouts({-30.0}, 20.0, 10.0, 200.0);
        QCOMPARE(lifted.at(0), 10.0);
        QVERIFY(MapDecals::spreadCallouts({}, 20.0, 0.0, 100.0).isEmpty());
    }

    void test_blueprintLettersInSpacedCapitals()
    {
        const QFont font = blueprint().labelFont(QFont(qsl("Bitstream Vera Sans"), 10));
        QCOMPARE(font.family(), blueprint().labelFontFamily);
        QCOMPARE(font.capitalization(), QFont::AllUppercase);
        QVERIFY(font.letterSpacing() > 100.0);
        QCOMPARE(font.letterSpacingType(), QFont::PercentageSpacing);
        const QFont plain = MapStyle().labelFont(QFont(qsl("Bitstream Vera Sans"), 10));
        QCOMPARE(plain.capitalization(), QFont::MixedCase);
        QCOMPARE(plain.letterSpacing(), 0.0);
    }

    void test_blueprintMarksThePlayerWithACrosshairAndNoBeacon()
    {
        QCOMPARE(blueprint().playerRing, MapStyle::PlayerRing::Crosshair);
        QVERIFY(blueprint().themesPlayerMarker(0, false));
        QVERIFY(!blueprint().playerBeacon);
        QVERIFY(tiles().playerBeacon);
        QCOMPARE(blueprint().highlightShape, MapStyle::HighlightShape::DashedEllipse);
        QVERIFY(blueprint().titleBlock && blueprint().levelCallouts);
        QVERIFY(!tiles().titleBlock && !tiles().levelCallouts);
    }

    void test_blueprintTintsRoomsByTheirEnvironment()
    {
        const QColor sheet = blueprint().background;
        const QColor forest = blueprint().roomColor(QColor(40, 160, 60));
        // A third of the way from the sheet to the environment's color, enough to tell areas apart at a glance
        QVERIFY2(forest.green() - sheet.green() > 25, qPrintable(qsl("green %1 on a sheet of %2").arg(forest.green()).arg(sheet.green())));
        QVERIFY(forest.blue() < sheet.blue());
    }
};

#include "MapDecalsTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapDecalsTest)
