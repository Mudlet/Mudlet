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

#include <QFontMetricsF>
#include <QImage>

#include "EnvironmentKit.h"
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
        QVERIFY(MapDecals::highlightOnGround(MapStyle::HighlightShape::Rings));
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

    void test_everyStyleHighlightsInTheScriptsColor()
    {
        const QColor blue(0, 0, 255);
        QList<MapStyle> styles = MapStyle::all();
        MapStyle ownColor = tiles();
        ownColor.id = qsl("tiles with a highlight color");
        ownColor.highlightColor = QColor(255, 255, 85);
        styles.append(ownColor);
        for (const MapStyle& style : styles) {
            const QColor color = style.highlightColorFor(blue);
            QVERIFY2(color == blue, qPrintable(style.id));
            const QImage image = MapDecals::highlight(style.highlightShape, color, blue, false, scmSize);
            int shown = 0;
            for (int y = 0; y < scmSize; ++y) {
                for (int x = 0; x < scmSize; ++x) {
                    const QColor pixel = QColor::fromRgba(image.pixel(x, y));
                    if (pixel.alpha() > 100) {
                        QVERIFY2(pixel.hue() == blue.hue(), qPrintable(style.id));
                        ++shown;
                    }
                }
            }
            QVERIFY2(shown > 0, qPrintable(style.id));
        }
    }

    void test_styleColorOnlyStandsInForAMissingHighlightColor()
    {
        MapStyle style = tiles();
        QVERIFY(!style.highlightColorFor(QColor()).isValid());
        style.highlightColor = QColor(255, 255, 85);
        QCOMPARE(style.highlightColorFor(QColor()), QColor(255, 255, 85));
    }

    void test_aClearHighlightColorGivesWayToItsCenterColor()
    {
        const MapStyle style = tiles();
        const QColor blue(0, 0, 255);
        const QColor clearRed(255, 0, 0, 0);
        QCOMPARE(style.highlightColorFor(clearRed, blue), blue);
        QCOMPARE(style.highlightColorFor(blue, clearRed), blue);
        QCOMPARE(style.highlightColorFor(blue, QColor(255, 0, 0)), blue);
        QCOMPARE(style.highlightColorFor(clearRed, QColor(0, 0, 255, 0)), clearRed);
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

    void test_dioramaLabelsSitOnARoundedTagThatFitsTheirPicture()
    {
        const MapStyle& diorama = *MapStyle::find(qsl("diorama"));
        QVERIFY(diorama.themedMapLabels && diorama.mapLabelPlate.isValid());
        QVERIFY(diorama.roomNameColor.isValid() && diorama.compassFace.isValid() && diorama.infoBackground.isValid() && diorama.infoTextColor.isValid());
        const TMapLabel label = textLabel(Qt::white, QColor(0, 0, 0, 50));
        const QImage image = MapDecals::themedLabel(label, diorama);
        QCOMPARE(image.size(), label.pix.size());
        // The tag's rounded ends leave the corners clear
        QCOMPARE(alphaAt(image, 0, 0), 0);
        QCOMPARE(alphaAt(image, image.width() - 1, image.height() - 1), 0);
        // The tag shows round the letters: in the tag's color across the middle, before the first letter
        const QRect tag = paintedArea(image);
        QVERIFY(!tag.isEmpty());
        const int y = tag.center().y();
        int tagged = 0;
        for (int x = tag.left(); x < tag.left() + tag.height() / 2; ++x) {
            tagged += QColor::fromRgba(image.pixel(x, y)).rgb() == diorama.mapLabelPlate.rgb();
        }
        QVERIFY2(tagged > 0, "no tag before the text");
        bool text = false;
        for (int x = tag.left(); x <= tag.right() && !text; ++x) {
            text = QColor::fromRgba(image.pixel(x, y)).rgb() == diorama.mapLabelColor.rgb();
        }
        QVERIFY2(text, "the text is not on the tag");

        // A light color of the label's own would be lost on the tag, so it goes without
        const QImage yellow = MapDecals::themedLabel(textLabel(QColor(255, 255, 0), QColor(0, 0, 0, 50)), diorama);
        QVERIFY(!yellow.isNull());
        bool yellowText = false;
        for (int x = 0; x < yellow.width(); ++x) {
            const QRgb pixel = yellow.pixel(x, yellow.height() / 2);
            QVERIFY2(qAlpha(pixel) < 200 || qBlue(pixel) < 200, "a light label is put on a white tag");
            yellowText = yellowText || (qAlpha(pixel) > 200 && qRed(pixel) > 200 && qGreen(pixel) > 200 && qBlue(pixel) < 60);
        }
        QVERIFY2(yellowText, "a light label has no text");
    }

    // Names stand out in bold on a white tag, as a model's name cards do; no other style's change
    void test_dioramaNamesSitOnABoldTag()
    {
        const MapStyle& diorama = *MapStyle::find(qsl("diorama"));
        QVERIFY(diorama.roomNameBold && diorama.roomNamePlate.isValid());
        QVERIFY(MapStyle::readsOnPlate(diorama.roomNameColor, diorama.roomNamePlate));
        for (const MapStyle& style : MapStyle::all()) {
            if (style.id != diorama.id) {
                QVERIFY2(!style.roomNameBold && !style.roomNamePlate.isValid(), qPrintable(style.id));
            }
        }

        QFont font(qsl("Bitstream Vera Sans"), 20);
        font.setBold(true);
        const QFontMetricsF metrics(font);
        constexpr int pad = 4;
        const QImage image = MapDecals::taggedName(qsl("Market Cross"), diorama.roomNameColor, diorama.roomNamePlate, font, metrics, pad);
        QCOMPARE(image.size(), QSize(qCeil(metrics.horizontalAdvance(qsl("Market Cross"))) + 2 * pad, qCeil(metrics.height()) + 2 * pad));
        // Rounded corners, and the tag's color round the text
        QCOMPARE(alphaAt(image, 0, 0), 0);
        QCOMPARE(alphaAt(image, image.width() - 1, image.height() - 1), 0);
        QCOMPARE(QColor::fromRgba(image.pixel(pad / 2, image.height() / 2)).rgb(), diorama.roomNamePlate.rgb());
        QCOMPARE(alphaAt(image, pad / 2, image.height() / 2), diorama.roomNamePlate.alpha());
        bool text = false;
        for (int x = pad; x < image.width() - pad && !text; ++x) {
            text = QColor::fromRgba(image.pixel(x, image.height() / 2)).rgb() == diorama.roomNameColor.rgb();
        }
        QVERIFY2(text, "the name is not on the tag");
    }

    void test_dioramaSymbolsStandOnATagInAColorThatShowsOnIt()
    {
        const MapStyle& diorama = *MapStyle::find(qsl("diorama"));
        QVERIFY(diorama.symbolPlate.isValid());
        const QColor darkPurple(60, 30, 90);
        QCOMPARE(diorama.symbolColor(QColor(), darkPurple), QColor(Qt::black));
        QCOMPARE(tiles().symbolColor(QColor(), darkPurple), QColor(Qt::white));
        QCOMPARE(diorama.symbolColor(QColor(200, 30, 30), darkPurple), QColor(200, 30, 30));
        QVERIFY(MapStyle::readsOnPlate(Qt::black, diorama.symbolPlate));
        QVERIFY(!MapStyle::readsOnPlate(QColor(250, 250, 210), diorama.symbolPlate));
        // So faint that the tag would show through it rather than it showing on the tag
        QVERIFY(!MapStyle::readsOnPlate(QColor(0, 0, 0, 40), diorama.symbolPlate));

        const QImage image = MapDecals::taggedSymbol(qsl("$"), Qt::black, diorama.symbolPlate, QFont(QString(), 20), 1.0, scmSize);
        QCOMPARE(image.size(), QSize(scmSize, scmSize));
        QCOMPARE(alphaAt(image, 0, 0), 0);
        QCOMPARE(alphaAt(image, scmSize / 2, 2), 0);
        QVERIFY2(qGray(image.pixel(scmSize / 2, scmSize / 6)) > 240 && alphaAt(image, scmSize / 2, scmSize / 6) > 200, "no tag round the symbol");
        bool glyph = false;
        for (int y = scmSize / 4; y < 3 * scmSize / 4 && !glyph; ++y) {
            for (int x = scmSize / 4; x < 3 * scmSize / 4 && !glyph; ++x) {
                glyph = qGray(image.pixel(x, y)) < 60 && alphaAt(image, x, y) > 200;
            }
        }
        QVERIFY2(glyph, "no symbol on the tag");
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

    // A pale wash of the environment's color over the sheet, however dark the color
    void test_blueprintTintsRoomsByTheirEnvironment()
    {
        const QColor sheet = blueprint().background;
        const QColor forest = blueprint().roomColor(QColor(40, 160, 60));
        QVERIFY2(forest.green() - sheet.green() > 25, qPrintable(qsl("green %1 on a sheet of %2").arg(forest.green()).arg(sheet.green())));
        QVERIFY(forest.green() - sheet.green() > forest.red() - sheet.red() && forest.green() - sheet.green() > forest.blue() - sheet.blue());
        const QColor cellar = blueprint().roomColor(QColor(50, 10, 70));
        QVERIFY2(cellar.lightness() - sheet.lightness() > 20, qPrintable(qsl("a dark room's lightness is %1 on a sheet of %2").arg(cellar.lightness()).arg(sheet.lightness())));
        QVERIFY2(cellar.red() - sheet.red() > 25, qPrintable(qsl("a purple room's red is %1 on a sheet of %2").arg(cellar.red()).arg(sheet.red())));
        QCOMPARE(blueprint().roomColor(QColor(40, 160, 60, 100)).alpha(), 100);
    }

    // Light line-work on the sheet: a fill only a little lighter than the sheet, under an outline in the much lighter ink
    void test_blueprintFillsRoomsPaleUnderItsInk()
    {
        const QColor sheet = blueprint().background;
        for (const QColor& environment : {QColor(40, 160, 60), QColor(50, 90, 200), QColor(220, 220, 220), QColor(128, 0, 0), QColor(255, 255, 0), QColor(Qt::white)}) {
            const QColor fill = blueprint().roomColor(environment);
            const int lift = fill.lightness() - sheet.lightness();
            QVERIFY2(lift > 5 && lift < 40, qPrintable(qsl("%1 fills a room %2 lighter than the sheet").arg(environment.name()).arg(lift)));
            QVERIFY2(blueprint().ink.lightness() - fill.lightness() > 100, qPrintable(qsl("%1 fills a room nearly as light as the ink").arg(environment.name())));
        }
    }

    void test_blueprintHighlightsAColorlessRoomInItsInk()
    {
        QCOMPARE(blueprint().highlightColorFor(QColor()).rgb(), blueprint().ink.rgb());
        QVERIFY(!tiles().highlightColor.isValid());
    }

    // Each digit is a texture of its own, drawn side by side, so spacing would leave gaps inside the number
    void test_blueprintRoomIdsAreNotLetterSpaced()
    {
        const QFont symbolFont(qsl("Bitstream Vera Sans"), 10);
        const QFont ids = blueprint().roomIdFont(symbolFont);
        QCOMPARE(ids.family(), blueprint().labelFontFamily);
        QCOMPARE(ids.letterSpacing(), symbolFont.letterSpacing());
        QCOMPARE(ids.letterSpacingType(), symbolFont.letterSpacingType());
        QVERIFY(blueprint().labelFont(symbolFont).letterSpacing() > 100.0);
        QCOMPARE(tiles().roomIdFont(symbolFont), tiles().labelFont(symbolFont));
    }

    void test_calloutsNameTheNearestLevelsTopToBottom_data()
    {
        QTest::addColumn<QList<int>>("levels");
        QTest::addColumn<int>("shown");
        QTest::addColumn<int>("most");
        QTest::addColumn<QList<int>>("named");
        QTest::newRow("all fit") << QList<int>{-1, 0, 1, 2} << 0 << 8 << QList<int>{2, 1, -1};
        QTest::newRow("the shown level is never named") << QList<int>{0} << 0 << 8 << QList<int>{};
        QTest::newRow("the furthest left out") << QList<int>{-4, -3, -2, -1, 0, 1, 2, 3} << 0 << 4 << QList<int>{2, 1, -1, -2};
        QTest::newRow("of two as near, the one above") << QList<int>{-1, 0, 1} << 0 << 1 << QList<int>{1};
        QTest::newRow("from a shown level up high") << QList<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10} << 9 << 8 << QList<int>{10, 8, 7, 6, 5, 4, 3, 2};
        QTest::newRow("no room for any") << QList<int>{1, 2} << 0 << 0 << QList<int>{};
        QTest::newRow("unsorted, with repeats") << QList<int>{3, -2, 3, 1} << 0 << 8 << QList<int>{3, 1, -2};
    }

    void test_calloutsNameTheNearestLevelsTopToBottom()
    {
        QFETCH(QList<int>, levels);
        QFETCH(int, shown);
        QFETCH(int, most);
        QFETCH(QList<int>, named);
        QCOMPARE(MapDecals::calloutLevels(levels, shown, most), named);
    }

    void test_calloutsAndTitleNameLevels()
    {
        QCOMPARE(MapDecals::levelCallout(1, 16), qsl("L1 · 16"));
        QCOMPARE(MapDecals::levelCallout(0, 3), qsl("L0 · 3"));
        QCOMPARE(MapDecals::levelCallout(-2, 6), qsl("B2 · 6"));
        QCOMPARE(MapDecals::titleLevelLine(0, {-1, 0, 1}), qsl("Level 0 · sheet 2 of 3"));
        QCOMPARE(MapDecals::titleLevelLine(-1, {-1, 0, 1}), qsl("Level -1 · sheet 1 of 3"));
        QCOMPARE(MapDecals::titleLevelLine(5, {-1, 0, 1}), qsl("Level 5"));
        QCOMPARE(MapDecals::levelName(2), qsl("L2"));
        QCOMPARE(MapDecals::levelName(0), qsl("L0"));
        QCOMPARE(MapDecals::levelName(-1), qsl("B1"));
    }

    void test_platformsStackAwayFromTheirAnchorAndStayInView()
    {
        QCOMPARE(MapDecals::stackPlatforms(3, 300.0, -40.0, 0.0, 1000.0), (QList<qreal>{180.0, 220.0, 260.0}));
        QCOMPARE(MapDecals::stackPlatforms(2, 300.0, 40.0, 0.0, 1000.0), (QList<qreal>{340.0, 380.0}));
        QCOMPARE(MapDecals::stackPlatforms(2, 300.0, 40.0, 0.0, 350.0), (QList<qreal>{340.0, 380.0}));
        QCOMPARE(MapDecals::stackPlatforms(2, 100.0, -40.0, 50.0, 1000.0), (QList<qreal>{50.0, 90.0}));
        QCOMPARE(MapDecals::stackPlatforms(2, 900.0, 40.0, 0.0, 950.0), (QList<qreal>{940.0, 980.0}));
        QVERIFY(MapDecals::stackPlatforms(0, 300.0, 40.0, 0.0, 1000.0).isEmpty());
    }

    void test_inkwashRingsAColorlessRoomTwiceInOchreAndAScriptsColorWins()
    {
        const MapStyle& inkwash = *MapStyle::find(qsl("inkwash"));
        QCOMPARE(inkwash.highlightShape, MapStyle::HighlightShape::DoubleEllipse);
        const QColor ochre = inkwash.highlightColorFor(QColor());
        QVERIFY(ochre.isValid());
        QVERIFY(ochre.hue() >= 25 && ochre.hue() <= 50);
        const QColor blue(0, 0, 255);
        QCOMPARE(inkwash.highlightColorFor(blue, QColor(0, 255, 0)), blue);
        QCOMPARE(inkwash.highlightColorFor(QColor(255, 0, 0, 0), blue), blue);
        // Two separate rings along a line out from the middle
        const QImage image = MapDecals::highlight(inkwash.highlightShape, ochre, ochre, false, scmSize);
        int rings = 0;
        bool inRing = false;
        for (int x = scmSize / 2; x < scmSize; ++x) {
            const bool painted = alphaAt(image, x, scmSize / 2) > 100;
            rings += painted && !inRing;
            inRing = painted;
        }
        QCOMPARE(rings, 2);
    }

    void test_dioramaGlowsAColorlessRoomInLamplightAndAScriptsColorWins()
    {
        const MapStyle& diorama = *MapStyle::find(qsl("diorama"));
        QCOMPARE(diorama.highlightShape, MapStyle::HighlightShape::Glow);
        QVERIFY(MapDecals::highlightOnGround(diorama.highlightShape));
        const QColor lamplight = diorama.highlightColorFor(QColor());
        QVERIFY(lamplight.isValid());
        QVERIFY(lamplight.hue() >= 40 && lamplight.hue() <= 55);
        const QColor blue(0, 0, 255);
        QCOMPARE(diorama.highlightColorFor(blue, blue), blue);
        QCOMPARE(diorama.highlightColorFor(QColor(255, 0, 0, 0), blue), blue);
    }

    void test_inkwashWashesAreSoftEdgedAndDifferPerVariant()
    {
        constexpr int size = 128;
        QList<QImage> washes;
        for (int variant = 0; variant < MapDecals::scmWashVariants; ++variant) {
            const QImage wash = MapDecals::wash(variant, size);
            QCOMPARE(wash.size(), QSize(size, size));
            QCOMPARE(alphaAt(wash, 0, 0), 0);
            QCOMPARE(alphaAt(wash, size - 1, size / 2), 0);
            QVERIFY(alphaAt(wash, size / 2, size / 2) > 100);
            QCOMPARE(QColor::fromRgba(wash.pixel(size / 2, size / 2)).rgb(), QColor(Qt::white).rgb());
            // Pooled darker just inside its edge than in its middle, as watercolor dries
            int edge = 0;
            for (int x = size / 2; x < size; ++x) {
                edge = qMax(edge, alphaAt(wash, x, size / 2));
            }
            QVERIFY(edge > alphaAt(wash, size / 2, size / 2));
            for (const QImage& other : std::as_const(washes)) {
                QVERIFY(other != wash);
            }
            washes.append(wash);
        }
        QCOMPARE(MapDecals::wash(1, size), washes.at(1));
    }

    void test_inkwashWashKeepsTheHueAndFadesForGray()
    {
        const MapStyle& inkwash = *MapStyle::find(qsl("inkwash"));
        const QColor green = inkwash.washColor(QColor(40, 160, 60));
        QVERIFY(std::abs(green.hue() - QColor(40, 160, 60).hue()) <= 2);
        const QColor gray = inkwash.washColor(QColor(128, 128, 128));
        QVERIFY(gray.alpha() < green.alpha());
        // However dark the environment, the room stays pale enough for the ink line on it to read
        QVERIFY(inkwash.roomColor(QColor(80, 0, 0)).lightness() > 180);
        QVERIFY(inkwash.roomColor(QColor(10, 10, 10)).lightness() > 180);
    }

    void test_environmentMarksOnlyForestAndWater()
    {
        const QColor ink(38, 34, 32);
        for (const EnvironmentKit::Kit kit : {EnvironmentKit::Kit::Forest, EnvironmentKit::Kit::Water}) {
            QVERIFY(MapDecals::environmentMarked(kit));
            const QImage mark = MapDecals::environmentMark(kit, ink, scmSize);
            QVERIFY(!paintedArea(mark).isEmpty());
            QCOMPARE(alphaAt(mark, 0, 0), 0);
        }
        for (const EnvironmentKit::Kit kit : {EnvironmentKit::Kit::Plain, EnvironmentKit::Kit::Road, EnvironmentKit::Kit::Building}) {
            QVERIFY(!MapDecals::environmentMarked(kit));
            QVERIFY(MapDecals::environmentMark(kit, ink, scmSize).isNull());
        }
    }

    void test_scaleBarSpansARoundNumberOfRooms()
    {
        QCOMPARE(MapDecals::scaleBarRooms(40.0, 60.0, 240.0), 2);
        QCOMPARE(MapDecals::scaleBarRooms(10.0, 60.0, 240.0), 10);
        QCOMPARE(MapDecals::scaleBarRooms(1.0, 60.0, 240.0), 100);
        QCOMPARE(MapDecals::scaleBarRooms(100.0, 60.0, 240.0), 2);
        QCOMPARE(MapDecals::scaleBarRooms(200.0, 60.0, 240.0), 0);
        QCOMPARE(MapDecals::scaleBarRooms(0.0, 60.0, 240.0), 0);
        QCOMPARE(MapDecals::scaleBarRooms(120.0, 60.0, 240.0), 2);
        QCOMPARE(MapDecals::scaleBarRooms(0.06, 60.0, 240.0), 1000);
        // So far out that even the longest would be too short to read
        QCOMPARE(MapDecals::scaleBarRooms(0.05, 60.0, 240.0), 0);
    }

    void test_bullseyeIsADotInARingOnWhite()
    {
        const QColor ink(35, 35, 40);
        const QImage mark = MapDecals::bullseye(ink, scmSize);
        const int middle = scmSize / 2;
        QCOMPARE(mark.pixelColor(middle, middle).rgb(), ink.rgb());
        QCOMPARE(mark.pixelColor(middle + qRound(middle * 0.6), middle).rgb(), QColor(Qt::white).rgb());
        QCOMPARE(mark.pixelColor(middle + qRound(middle * 0.82), middle).rgb(), ink.rgb());
        QCOMPARE(alphaAt(mark, 2, 2), 0);
    }
};

#include "MapDecalsTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapDecalsTest)
