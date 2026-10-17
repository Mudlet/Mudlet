/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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
 * GeometryManager hands its arrays to the GPU as tightly packed attributes:
 * three floats of position per vertex, with normals in an array of their own
 * rather than interleaved, so anything else in the position array is drawn as
 * a vertex.
 * A mat4 vertex attribute takes four consecutive locations, so an attribute
 * declared inside that span aliases one of its columns.
 */

#include <QtTest/QtTest>

#include <QColor>
#include <QFile>
#include <QFontMetricsF>
#include <QRegularExpression>
#include <QVector2D>

#include <algorithm>

#include "GeometryManager.h"
#include "Host.h"
#include "LabelTextureCache.h"
#include "MapStyle.h"

#include "GroupedTest.h"

class ModernMapperGeometryTest : public QObject
{
    Q_OBJECT

private:
    struct ShaderAttribute
    {
        int location;
        QString type;
        QString name;
    };

    static QList<ShaderAttribute> vertexShaderAttributes()
    {
        QFile shader(qsl(":/shaders/vertex.glsl"));
        if (!shader.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {};
        }
        const QString source = QString::fromUtf8(shader.readAll());
        static const QRegularExpression declaration(qsl(R"(layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*in\s+(\w+)\s+(\w+))"));
        QList<ShaderAttribute> attributes;
        auto matches = declaration.globalMatch(source);
        while (matches.hasNext()) {
            const auto match = matches.next();
            attributes.append({match.captured(1).toInt(), match.captured(2), match.captured(3)});
        }
        return attributes;
    }

    static void verifyTightlyPacked(const GeometryData& geometry, const QVector<float>& positions)
    {
        const int expectedVertices = positions.size() / 3;
        QCOMPARE(geometry.vertexCount(), expectedVertices);
        QCOMPARE(geometry.vertices, positions);
        QCOMPARE(geometry.normals.size(), expectedVertices * 3);
        QCOMPARE(geometry.colors.size(), expectedVertices * 4);
    }

private slots:
    void test_trianglePositionsAreNotInterleavedWithNormals()
    {
        GeometryManager geometryManager;
        const QVector<float> positions{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, -1.0f, -2.0f, -3.0f, -4.0f, -5.0f, -6.0f, -7.0f, -8.0f, -9.0f};
        QVector<float> colors;
        for (int i = 0; i < positions.size() / 3; ++i) {
            colors << 0.5f << 0.5f << 0.5f << 1.0f;
        }

        const GeometryData geometry = geometryManager.generateTriangleGeometry(positions, colors);

        verifyTightlyPacked(geometry, positions);
        for (int i = 0; i < geometry.normals.size(); i += 3) {
            QCOMPARE(QVector3D(geometry.normals[i], geometry.normals[i + 1], geometry.normals[i + 2]), QVector3D(0.0f, 0.0f, 1.0f));
        }
    }

    void test_lineGeometryCountsEveryVertex()
    {
        GeometryManager geometryManager;
        const QVector<float> positions{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 2.0f, 0.0f, 1.0f, 3.0f, 1.0f, 1.0f};
        const QVector<float> colors(positions.size() / 3 * 4, 1.0f);

        const GeometryData geometry = geometryManager.generateLineGeometry(positions, colors);

        verifyTightlyPacked(geometry, positions);
    }

    void test_groundQuadsForOneTextureShareABatch()
    {
        GeometryData batch;
        const QVector3D first(0.0f, 0.0f, 0.0f);
        const QVector3D second(5.0f, 3.0f, 1.0f);
        GeometryManager::appendGroundQuad(batch, first, 1.0f, QVector3D(1.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f), QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
        GeometryManager::appendGroundQuad(batch, second, 1.0f, QVector3D(1.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f), QVector4D(1.0f, 1.0f, 1.0f, 1.0f));

        QCOMPARE(batch.vertices.size(), 36);
        QCOMPARE(batch.normals.size(), 36);
        QCOMPARE(batch.colors.size(), 48);
        QCOMPARE(batch.textureCoords.size(), 24);
        for (const auto& [quad, center] : {std::pair{0, first}, std::pair{1, second}}) {
            QVector3D sum;
            for (int vertex = quad * 6; vertex < quad * 6 + 6; ++vertex) {
                sum += QVector3D(batch.vertices[vertex * 3], batch.vertices[vertex * 3 + 1], batch.vertices[vertex * 3 + 2]);
            }
            QCOMPARE(sum / 6.0f, center);
        }
    }

    void test_aGroundQuadSeenEdgeOnLinesUpWithTheScreensUp()
    {
        GeometryData batch;
        // The screen's right points straight up off the map, so lays flat to nothing; the quad's up follows the screen's
        GeometryManager::appendGroundQuad(batch, QVector3D(), 2.0f, QVector3D(0.0f, 0.0f, 1.0f), QVector3D(1.0f, 0.0f, 0.0f), QVector4D(1.0f, 1.0f, 1.0f, 1.0f));

        QCOMPARE(batch.vertices.size(), 18);
        const QVector3D bottomLeft(batch.vertices[0], batch.vertices[1], batch.vertices[2]);
        const QVector3D bottomRight(batch.vertices[3], batch.vertices[4], batch.vertices[5]);
        const QVector3D topRight(batch.vertices[6], batch.vertices[7], batch.vertices[8]);
        QCOMPARE((bottomRight - bottomLeft).length(), 2.0f);
        QCOMPARE(topRight - bottomRight, QVector3D(2.0f, 0.0f, 0.0f));
    }

    void test_roomTexturesToEvict_data()
    {
        QTest::addColumn<QStringList>("cached");
        QTest::addColumn<qsizetype>("maximum");
        QTest::addColumn<QStringList>("inUse");
        QTest::addColumn<QStringList>("evicted");

        QTest::newRow("room for one more") << QStringList{qsl("a"), qsl("b")} << qsizetype(3) << QStringList{} << QStringList{};
        QTest::newRow("full, nothing in use") << QStringList{qsl("a"), qsl("b"), qsl("c")} << qsizetype(3) << QStringList{} << QStringList{qsl("a")};
        QTest::newRow("full, the oldest in use") << QStringList{qsl("a"), qsl("b"), qsl("c")} << qsizetype(3) << QStringList{qsl("a")} << QStringList{qsl("b")};
        QTest::newRow("over, some in use") << QStringList{qsl("a"), qsl("b"), qsl("c"), qsl("d"), qsl("e")} << qsizetype(3) << QStringList{qsl("a"), qsl("c")}
                                           << QStringList{qsl("b"), qsl("d"), qsl("e")};
        QTest::newRow("all in use") << QStringList{qsl("a"), qsl("b"), qsl("c")} << qsizetype(3) << QStringList{qsl("a"), qsl("b"), qsl("c")} << QStringList{};
    }

    void test_roomTexturesToEvict()
    {
        QFETCH(QStringList, cached);
        QFETCH(qsizetype, maximum);
        QFETCH(QStringList, inUse);
        QFETCH(QStringList, evicted);

        const QSet<QString> keep(inUse.cbegin(), inUse.cend());
        QCOMPARE(LabelTextureCache::keysToEvict(cached, maximum, keep), evicted);
    }

    // The instanced cube spans -1 to 1 on each axis
    void test_barTransformStretchesTheCubeAlongTheLine_data()
    {
        QTest::addColumn<QVector3D>("from");
        QTest::addColumn<QVector3D>("to");
        QTest::newRow("east") << QVector3D(1.0f, 2.0f, 0.0f) << QVector3D(3.0f, 2.0f, 0.0f);
        QTest::newRow("northwest") << QVector3D(0.0f, 0.0f, 1.0f) << QVector3D(-1.0f, 1.0f, 1.0f);
        QTest::newRow("up a level") << QVector3D(4.0f, 4.0f, 0.0f) << QVector3D(4.0f, 4.0f, 1.0f);
        QTest::newRow("down and away") << QVector3D(0.0f, 0.0f, 2.0f) << QVector3D(1.0f, 0.0f, 0.0f);
    }

    void test_barTransformStretchesTheCubeAlongTheLine()
    {
        QFETCH(QVector3D, from);
        QFETCH(QVector3D, to);
        const float width = 0.1f;
        const float height = 0.04f;
        const QMatrix4x4 transform = GeometryManager::barTransform(from, to, width, height);
        QVERIFY((transform.map(QVector3D(-1.0f, 0.0f, 0.0f)) - from).length() < 1e-5f);
        QVERIFY((transform.map(QVector3D(1.0f, 0.0f, 0.0f)) - to).length() < 1e-5f);

        const QVector3D center = (from + to) / 2.0f;
        const QVector3D across = transform.map(QVector3D(0.0f, 1.0f, 0.0f)) - center;
        const QVector3D upward = transform.map(QVector3D(0.0f, 0.0f, 1.0f)) - center;
        QVERIFY(qAbs(across.length() - width / 2.0f) < 1e-5f);
        QVERIFY(qAbs(upward.length() - height / 2.0f) < 1e-5f);
        const QVector3D along = (to - from).normalized();
        QVERIFY(qAbs(QVector3D::dotProduct(across, along)) < 1e-5f);
        QVERIFY(qAbs(QVector3D::dotProduct(upward, along)) < 1e-5f);
        QVERIFY(qAbs(QVector3D::dotProduct(across, upward)) < 1e-5f);
        if (qAbs(along.z()) < 1e-5f) {
            QVERIFY2(qAbs(across.z()) < 1e-5f, "a level line is not level across");
            QVERIFY2(upward.z() > 0.0f, "a level line's height is not upward");
        }
    }

    void test_quadTexturesFromItsOrigin()
    {
        GeometryData quad;
        const QVector3D origin(1.0f, 2.0f, 3.0f);
        const QVector3D across(2.0f, 0.0f, 0.0f);
        const QVector3D upward(0.0f, 1.0f, 0.0f);
        GeometryManager::appendQuad(quad, origin, across, upward, QVector4D(1.0f, 0.5f, 0.25f, 1.0f));
        QCOMPARE(quad.vertexCount(), 6);
        QCOMPARE(quad.textureCoords.size(), 12);
        QCOMPARE(quad.colors.size(), 24);
        for (int vertex = 0; vertex < 6; ++vertex) {
            const QVector3D position(quad.vertices[vertex * 3], quad.vertices[vertex * 3 + 1], quad.vertices[vertex * 3 + 2]);
            const float u = quad.textureCoords[vertex * 2];
            const float v = quad.textureCoords[vertex * 2 + 1];
            QCOMPARE(position, origin + across * u + upward * v);
        }
    }

    // Room names on the 3D view: the nearest of an overlapping pair is kept, and one too small to read is not
    void test_declutteredRectsDropOverlappingAndTinyNames()
    {
        const QList<QRectF> rects{
                QRectF(0, 0, 100, 12),     // kept
                QRectF(50, 6, 100, 12),    // overlaps the first
                QRectF(200, 0, 100, 12),   // clear of both
                QRectF(400, 0, 100, 4),    // too short to read
                QRectF(),                  // off screen
                QRectF(-90, 500, 100, 12), // clear, though in a grid cell left of the origin
                QRectF(-80, 505, 20, 12),  // overlaps the one before, across the origin's cell boundary
                QRectF(130, 2, 60, 12),    // between the first and third, touching neither
        };
        QCOMPARE(GeometryManager::declutteredRects(rects, 7.0), (QList<qsizetype>{0, 2, 5, 7}));
        QCOMPARE(GeometryManager::declutteredRects(rects, 2.0), (QList<qsizetype>{0, 2, 3, 5, 7}));
        QVERIFY(GeometryManager::declutteredRects({}, 7.0).isEmpty());
    }

    // Where names overlap: the player's room's first, then a highlighted room's, then those round the player, the
    // nearer first, then the nearer the camera
    void test_namesRankByWhatMattersBeforeByNearness()
    {
        using Priority = GeometryManager::NamePriority;
        const Priority player{0, 0.0f, 9.0f};
        const Priority highlighted{1, 0.0f, 8.0f};
        const Priority besidePlayer{2, 1.0f, 7.0f};
        const Priority twoAway{2, 2.0f, 1.0f};
        const Priority nearCamera{3, 0.0f, 0.5f};
        const Priority farCamera{3, 0.0f, 5.0f};
        const QList<Priority> order{player, highlighted, besidePlayer, twoAway, nearCamera, farCamera};
        for (qsizetype first = 0; first < order.size(); ++first) {
            QVERIFY2(!GeometryManager::outranks(order.at(first), order.at(first)), "a name outranks itself");
            for (qsizetype second = first + 1; second < order.size(); ++second) {
                QVERIFY2(GeometryManager::outranks(order.at(first), order.at(second)), qPrintable(qsl("%1 does not outrank %2").arg(first).arg(second)));
                QVERIFY2(!GeometryManager::outranks(order.at(second), order.at(first)), qPrintable(qsl("%1 outranks %2").arg(second).arg(first)));
            }
        }
    }

    // A name's opacity moves toward where it is going by the part of the fade that has passed, and no further
    void test_namesFadeByTheTimePassed()
    {
        QCOMPARE(GeometryManager::steppedOpacity(1.0f, 0.0f, 50, 200), 0.75f);
        QCOMPARE(GeometryManager::steppedOpacity(0.25f, 1.0f, 50, 200), 0.5f);
        QCOMPARE(GeometryManager::steppedOpacity(0.1f, 0.0f, 50, 200), 0.0f);
        QCOMPARE(GeometryManager::steppedOpacity(0.9f, 1.0f, 500, 200), 1.0f);
        QCOMPARE(GeometryManager::steppedOpacity(0.5f, 1.0f, -5, 200), 0.5f);
        QCOMPARE(GeometryManager::steppedOpacity(0.5f, 0.0f, 0, 0), 0.0f);
    }

    // A name drawn at as many texels as the device pixels it covers, from once to three times its usual resolution,
    // counting the screen's pixel ratio, and never wider than a texture can be
    void test_namesAreDrawnAsSharpAsTheyAreSeen()
    {
        QCOMPARE(GeometryManager::textSharpness(0.0, 1.0, 100.0, 4096), 1);
        QCOMPARE(GeometryManager::textSharpness(0.6, 1.0, 100.0, 4096), 1);
        QCOMPARE(GeometryManager::textSharpness(1.2, 1.0, 100.0, 4096), 1);
        QCOMPARE(GeometryManager::textSharpness(1.3, 1.0, 100.0, 4096), 2);
        QCOMPARE(GeometryManager::textSharpness(2.5, 1.0, 100.0, 4096), 3);
        QCOMPARE(GeometryManager::textSharpness(9.0, 1.0, 100.0, 4096), 3);
        // A high resolution screen's name covers twice the device pixels
        QCOMPARE(GeometryManager::textSharpness(0.6, 2.0, 100.0, 4096), 1);
        QCOMPARE(GeometryManager::textSharpness(1.0, 2.0, 100.0, 4096), 2);
        QCOMPARE(GeometryManager::textSharpness(1.2, 2.0, 100.0, 4096), 3);
        // A long name is drawn no sharper than a texture can hold it
        QCOMPARE(GeometryManager::textSharpness(9.0, 2.0, 1500.0, 4096), 2);
        QCOMPARE(GeometryManager::textSharpness(9.0, 2.0, 3000.0, 4096), 1);
        QCOMPARE(GeometryManager::textSharpness(9.0, 2.0, 9000.0, 4096), 1);
    }

    // Two rooms whose names just fit side by side: spaced capitals run together there, so blueprint keeps a space
    // between names and drops one, where every other style keeps both as before
    void test_onlySpacedNamesAreKeptASpaceApart()
    {
        const MapStyle& tiles = *MapStyle::find(qsl("tiles"));
        const MapStyle& blueprint = *MapStyle::find(qsl("blueprint"));
        QFont font = blueprint.labelFont(QFont(qsl("Bitstream Vera Sans"), 10));
        font.setPixelSize(20);
        const QFontMetricsF metrics(font);
        const QString name = qsl("Street 12");
        const qreal nameWidth = metrics.horizontalAdvance(name);
        const qreal spaceWidth = metrics.horizontalAdvance(QLatin1Char(' '));
        const qreal roomSpacing = nameWidth + 1.0;
        const auto namesKept = [&](const MapStyle& style) {
            const qreal span = style.roomNameSpan(nameWidth, spaceWidth);
            QList<QRectF> rects;
            for (const qreal center : {100.0, 100.0 + roomSpacing}) {
                rects.append(QRectF(center - span / 2.0, 100.0, span, metrics.height()));
            }
            return GeometryManager::declutteredRects(rects, 7.0).size();
        };
        QCOMPARE(namesKept(tiles), 2);
        QCOMPARE(namesKept(blueprint), 1);
        QCOMPARE(tiles.roomNameSpan(nameWidth, spaceWidth), nameWidth);
    }

    // Levels below the viewed one fade toward the background, further the further down; those above turn see-through
    void test_levelsFadeTowardTheBackgroundWithDistance()
    {
        const QColor room(200, 40, 40);
        // Light, so a fade that only darkened would move away from it
        const QColor background(240, 240, 240);
        const auto fromBackground = [&background](const QVector4D& color) {
            return QVector3D(color.x() - background.redF(), color.y() - background.greenF(), color.z() - background.blueF()).length();
        };
        const QVector4D viewed = GeometryManager::fadedColor(room, GeometryManager::levelFade(0, false), background);
        QCOMPARE(viewed, QVector4D(room.redF(), room.greenF(), room.blueF(), 1.0f));
        QCOMPARE(GeometryManager::fadedDecalTint(GeometryManager::levelFade(0, false)), QVector4D(1.0f, 1.0f, 1.0f, 1.0f));

        const QVector4D oneBelow = GeometryManager::fadedColor(room, GeometryManager::levelFade(-1, false), background);
        const QVector4D twoBelow = GeometryManager::fadedColor(room, GeometryManager::levelFade(-2, false), background);
        QVERIFY2(fromBackground(oneBelow) < fromBackground(viewed), "the level below does not fade toward the background");
        QVERIFY2(fromBackground(twoBelow) < fromBackground(oneBelow), "two levels below fades no further than one");
        QCOMPARE(oneBelow.w(), 1.0f);
        QVERIFY(GeometryManager::fadedDecalTint(GeometryManager::levelFade(-1, false)).w() < 1.0f);

        const QVector4D oneAbove = GeometryManager::fadedColor(room, GeometryManager::levelFade(1, false), background);
        QVERIFY2(oneAbove.w() < 0.5f, "the level above would hide the viewed one");
        QVERIFY(GeometryManager::levelFade(2, false).opacity < GeometryManager::levelFade(1, false).opacity);
    }

    // Every level above or below fades toward the upper or lower level color, covering the background as far as its
    // alpha, as the 2D map draws it
    void test_levelsFadeTowardTheUpperAndLowerLevelColors()
    {
        const QColor background(Qt::black);
        const QColor upper(Qt::white);
        const QColor lower(255, 0, 0, 128);
        QCOMPARE(GeometryManager::levelFadeTarget(0, background, upper, lower), background);
        const QColor above = GeometryManager::levelFadeTarget(1, background, upper, lower);
        QCOMPARE(GeometryManager::levelFadeTarget(3, background, upper, lower), above);
        QCOMPARE(above, upper);
        const QColor below = GeometryManager::levelFadeTarget(-2, background, upper, lower);
        QVERIFY2(below.red() >= 126 && below.red() <= 130 && below.green() == 0 && below.blue() == 0, qPrintable(below.name()));
        QCOMPARE(below.alpha(), 255);
    }

    // The profile's default level colors are only a hint of above and below, so that the levels still sink into the
    // background, while one the user chose takes its full effect
    void test_onlyTheDefaultLevelColorsAreCalmed()
    {
        const QColor background(30, 30, 40);
        for (const QColor& defaultColor : {Host::scmDefaultUpperLevelColor, Host::scmDefaultLowerLevelColor}) {
            const QColor calmed = GeometryManager::calmedLevelColor(defaultColor, defaultColor);
            QCOMPARE(calmed.rgb(), defaultColor.rgb());
            QVERIFY2(calmed.alphaF() < 0.5f * defaultColor.alphaF(), qPrintable(qsl("the default %1 is kept at alpha %2").arg(defaultColor.name()).arg(calmed.alpha())));
            const QColor target = GeometryManager::levelFadeTarget(1, background, calmed, calmed);
            QVERIFY2(target.red() < (background.red() + defaultColor.red()) / 2, qPrintable(qsl("the default %1 lights the level up to %2").arg(defaultColor.name(), target.name())));
            const QColor chosen(255, 0, 0);
            QCOMPARE(GeometryManager::calmedLevelColor(chosen, defaultColor), chosen);
            QCOMPARE(GeometryManager::levelFadeTarget(-1, background, calmed, GeometryManager::calmedLevelColor(chosen, defaultColor)), chosen);
        }
    }

    // Another level's rooms lose their hue and sink toward the background the further they are from the viewed one,
    // above or below, so that the viewed level's colors are the ones that catch the eye
    void test_otherLevelsGrayAndSinkWithDistance()
    {
        const QColor room(200, 40, 40);
        // Gray, so that whatever hue a ghost shows is its own rather than the background's
        const QColor background(24, 24, 24);
        const auto shown = [&background](const QVector4D& color) {
            const QColor top = QColor::fromRgbF(color.x(), color.y(), color.z(), color.w());
            return MapStyle::over(top, background);
        };
        const auto chroma = [](const QColor& color) {
            return std::max({color.red(), color.green(), color.blue()}) - std::min({color.red(), color.green(), color.blue()});
        };
        for (const int direction : {1, -1}) {
            int lastChroma = chroma(room);
            qreal lastContrast = MapStyle::contrastRatio(room, background);
            for (int distance = 1; distance <= 3; ++distance) {
                const QColor ghost = shown(GeometryManager::fadedColor(room, GeometryManager::levelFade(direction * distance, false), background));
                const QString where = qsl("%1 levels %2").arg(distance).arg(direction > 0 ? qsl("above") : qsl("below"));
                QVERIFY2(chroma(ghost) < lastChroma, qPrintable(qsl("%1 keeps as much hue as nearer, %2").arg(where, ghost.name())));
                const qreal contrast = MapStyle::contrastRatio(ghost, background);
                QVERIFY2(contrast < lastContrast, qPrintable(qsl("%1 stands out %2:1, no less than nearer").arg(where).arg(contrast)));
                lastChroma = chroma(ghost);
                lastContrast = contrast;
            }
        }
        QVERIFY(GeometryManager::levelFade(1, false).saturation < 1.0f);
        QCOMPARE(GeometryManager::levelFade(0, false).saturation, 1.0f);
    }

    // Faded as it is, a room one level off can still be found: in every style that draws other levels in place, what
    // marks a room stands out from the background by a little, yet by less than on the viewed level
    void test_aRoomOneLevelOffStillShowsInEveryStyle()
    {
        // Far below text's 4.5:1, as a ghost is meant to sit back, but still a block where there is one rather than
        // nothing; a room that barely stands out on the viewed level need only keep a share of that
        constexpr qreal leastGhostContrast = 1.05;
        constexpr qreal leastShareOfFaintContrast = 0.25;
        const QList<QColor> environments{QColor(170, 0, 0), QColor(0, 170, 0), QColor(0, 0, 170), QColor(128, 128, 128), QColor(20, 20, 60), QColor(255, 255, 85)};
        for (const MapStyle& style : MapStyle::all()) {
            if (style.levelPlatforms) {
                continue;
            }
            const QColor background = style.backgroundColor(QColor(Qt::black));
            const bool glowing = style.edges == MapStyle::Edges::GlowingWireframes;
            for (const QColor& environment : environments) {
                const QColor roomColor = style.roomColor(environment);
                QColor mark = roomColor;
                if (glowing) {
                    mark = style.wireColor(QColor(), roomColor);
                } else if (style.edges == MapStyle::Edges::InkOutlines) {
                    mark = style.ink;
                }
                // A lozenge is ringed by a line the shader darkens from the room's color
                const float line = style.edges == MapStyle::Edges::InkLozenges ? 0.28f : 1.0f;
                const QColor viewed = MapStyle::over(QColor::fromRgbF(mark.redF() * line, mark.greenF() * line, mark.blueF() * line), background);
                for (const int levelsAbove : {1, -1}) {
                    GeometryManager::LevelFade fade = GeometryManager::levelFade(levelsAbove, false);
                    QColor ghostColor = mark;
                    if (glowing) {
                        ghostColor = style.otherLevelColorFor(mark, levelsAbove > 0);
                        fade.saturation = style.otherLevelSaturation(fade.saturation, levelsAbove > 0);
                    }
                    const QVector4D faded = GeometryManager::fadedColor(ghostColor, fade, background) * QVector4D(line, line, line, 1.0f);
                    QColor ghost;
                    if (glowing) {
                        // Added onto the background, as a wireframe is
                        ghost = QColor::fromRgbF(
                                qMin(1.0f, background.redF() + faded.x() * faded.w()), qMin(1.0f, background.greenF() + faded.y() * faded.w()), qMin(1.0f, background.blueF() + faded.z() * faded.w()));
                    } else {
                        ghost = MapStyle::over(QColor::fromRgbF(faded.x(), faded.y(), faded.z(), faded.w()), background);
                    }
                    const qreal ghostContrast = MapStyle::contrastRatio(ghost, background);
                    const qreal viewedContrast = MapStyle::contrastRatio(viewed, background);
                    const QString what = qsl("%1's %2 room %3 the viewed level").arg(style.id, environment.name(), levelsAbove > 0 ? qsl("above") : qsl("below"));
                    const qreal least = std::min(leastGhostContrast, 1.0 + leastShareOfFaintContrast * (viewedContrast - 1.0));
                    QVERIFY2(ghostContrast >= least, qPrintable(qsl("%1 stands out only %2:1, against %3:1 on it").arg(what).arg(ghostContrast).arg(viewedContrast)));
                    QVERIFY2(ghostContrast < viewedContrast, qPrintable(qsl("%1 stands out %2:1, no less than on it").arg(what).arg(ghostContrast)));
                }
            }
        }
    }

    // In a hologram another level is a dim cyan echo, whatever its rooms' hues, while the viewed level keeps them
    void test_hologramDrawsOtherLevelsInCyan()
    {
        const MapStyle* hologram = MapStyle::find(qsl("hologram"));
        QVERIFY(hologram);
        QColor red = hologram->roomColor(QColor(200, 30, 30));
        red.setAlpha(100);
        QVERIFY(red.hslHue() < 20 || red.hslHue() > 340);
        for (const bool above : {true, false}) {
            const QColor ghost = hologram->otherLevelColorFor(red, above);
            QVERIFY2(ghost.hslHue() >= 175 && ghost.hslHue() <= 225, qPrintable(qsl("a level off glows in hue %1").arg(ghost.hslHue())));
            QVERIFY2(std::abs(ghost.alpha() - red.alpha()) <= 1, qPrintable(qsl("a level off is drawn at alpha %1 for %2").arg(ghost.alpha()).arg(red.alpha())));
        }
        // Kept vivid rather than grayed, as it is the one hue those levels share
        QCOMPARE(hologram->otherLevelSaturation(0.4f, true), 1.0f);
        QCOMPARE(hologram->otherLevelSaturation(0.4f, false), 1.0f);
        const MapStyle* tiles = MapStyle::find(qsl("tiles"));
        QVERIFY(tiles);
        QCOMPARE(tiles->otherLevelColorFor(red, true), red);
        QCOMPARE(tiles->otherLevelColorFor(red, false), red);
        QCOMPARE(tiles->otherLevelSaturation(0.4f, true), 0.4f);
    }

    // The levels above and below are told apart in a hologram even where they are faded alike, as when they are made
    // more transparent
    void test_hologramTellsTheLevelsAboveFromThoseBelow()
    {
        const MapStyle* hologram = MapStyle::find(qsl("hologram"));
        QVERIFY(hologram);
        const QColor background = hologram->backgroundColor(QColor(Qt::black));
        const QColor drawn = hologram->wireColor(QColor(), hologram->roomColor(QColor(200, 30, 30)));
        for (const bool moreTransparent : {false, true}) {
            const auto shown = [&](const int levelsAbove) {
                GeometryManager::LevelFade fade = GeometryManager::levelFade(levelsAbove, moreTransparent);
                fade.saturation = hologram->otherLevelSaturation(fade.saturation, levelsAbove > 0);
                const QVector4D color = GeometryManager::fadedColor(hologram->otherLevelColorFor(drawn, levelsAbove > 0), fade, background);
                return QColor::fromRgbF(color.x(), color.y(), color.z());
            };
            const QColor above = shown(1);
            const QColor below = shown(-1);
            QVERIFY2(std::abs(above.hsvHue() - below.hsvHue()) >= 20,
                     qPrintable(qsl("above %1 and below %2 look alike%3").arg(above.name(), below.name(), moreTransparent ? qsl(" made more transparent") : QString())));
        }
    }

    void test_gridLinesRunThroughEveryWholeCoordinate()
    {
        const QRectF extent(QPointF(-1.5, -0.5), QPointF(1.5, 2.25));
        const QList<QLineF> lines = GeometryManager::gridLines(extent);
        QList<qreal> across;
        QList<qreal> upward;
        for (const QLineF& line : lines) {
            if (line.x1() == line.x2()) {
                QCOMPARE(line.y1(), extent.top());
                QCOMPARE(line.y2(), extent.bottom());
                across.append(line.x1());
            } else {
                QCOMPARE(line.y1(), line.y2());
                QCOMPARE(line.x1(), extent.left());
                QCOMPARE(line.x2(), extent.right());
                upward.append(line.y1());
            }
        }
        QCOMPARE(across, (QList<qreal>{-1.0, 0.0, 1.0}));
        QCOMPARE(upward, (QList<qreal>{0.0, 1.0, 2.0}));
        QVERIFY(GeometryManager::gridLines(QRectF()).isEmpty());
    }

    // Round rooms are drawn from this, scaled like the cube [-1, 1], so it must fill the same height and the circle inside it
    void test_theCylinderFitsInsideTheUnitCube()
    {
        const GeometryData mesh = GeometryManager::cylinderMesh(24);
        QVERIFY(mesh.vertexCount() > 0);
        QCOMPARE(mesh.normals.size(), mesh.vertices.size());
        QCOMPARE(mesh.indexCount() % 3, 0);
        float lowest = 0.0f;
        float highest = 0.0f;
        float widest = 0.0f;
        for (int i = 0; i < mesh.vertexCount(); ++i) {
            const QVector3D vertex(mesh.vertices[3 * i], mesh.vertices[3 * i + 1], mesh.vertices[3 * i + 2]);
            const QVector3D normal(mesh.normals[3 * i], mesh.normals[3 * i + 1], mesh.normals[3 * i + 2]);
            QVERIFY(qAbs(normal.length() - 1.0f) < 1e-5f);
            lowest = qMin(lowest, vertex.z());
            highest = qMax(highest, vertex.z());
            widest = qMax(widest, QVector2D(vertex.x(), vertex.y()).length());
        }
        QCOMPARE(lowest, -1.0f);
        QCOMPARE(highest, 1.0f);
        QVERIFY(qAbs(widest - 1.0f) < 1e-5f);
        // Every triangle is wound counterclockwise seen from outside, as the cube's are
        for (int i = 0; i < mesh.indexCount(); i += 3) {
            QVector3D corners[3];
            for (int k = 0; k < 3; ++k) {
                const auto v = static_cast<int>(mesh.indices[i + k]);
                QVERIFY(v < mesh.vertexCount());
                corners[k] = QVector3D(mesh.vertices[3 * v], mesh.vertices[3 * v + 1], mesh.vertices[3 * v + 2]);
            }
            const QVector3D facing = QVector3D::crossProduct(corners[1] - corners[0], corners[2] - corners[0]);
            const QVector3D middle = (corners[0] + corners[1] + corners[2]) / 3.0f;
            const QVector3D outward = qAbs(middle.z()) > 0.999f ? QVector3D(0.0f, 0.0f, middle.z()) : QVector3D(middle.x(), middle.y(), 0.0f);
            QVERIFY2(QVector3D::dotProduct(facing, outward) > 0.0f, qPrintable(qsl("triangle %1 faces inward").arg(i / 3)));
        }
    }

    void test_aLabelIsOnTheLevelItsHeightRoundsTo_data()
    {
        QTest::addColumn<qreal>("z");
        QTest::addColumn<bool>("onALevel");
        QTest::addColumn<int>("level");
        QTest::newRow("on a level") << 2.0 << true << 2;
        QTest::newRow("three quarters up") << 0.75 << true << 1;
        QTest::newRow("three quarters down") << -0.75 << true << -1;
        QTest::newRow("a quarter up") << 0.25 << true << 0;
        QTest::newRow("a quarter down") << -0.25 << true << 0;
        // Halves round away from zero, as the 2D map's qRound does
        QTest::newRow("half up") << 0.5 << true << 1;
        QTest::newRow("half down") << -0.5 << true << -1;
        QTest::newRow("one and a half down") << -1.5 << true << -2;
        QTest::newRow("beyond any level") << 1e12 << false << 0;
        QTest::newRow("not a number") << qQNaN() << false << 0;
        QTest::newRow("infinite") << -qInf() << false << 0;
    }

    void test_aLabelIsOnTheLevelItsHeightRoundsTo()
    {
        QFETCH(qreal, z);
        QFETCH(bool, onALevel);
        QFETCH(int, level);
        const std::optional<int> found = GeometryManager::labelLevel(z);
        QCOMPARE(found.has_value(), onALevel);
        if (onALevel) {
            QCOMPARE(*found, level);
            QCOMPARE(*found, qRound(z));
        }
    }

    void test_aLabelQuadCoversTheLabelAndTurnsInPlace_data()
    {
        QTest::addColumn<bool>("turned");
        QTest::addColumn<QVector3D>("textureTopLeft");
        QTest::newRow("upright") << false << QVector3D(1.0f, 2.0f, 0.5f);
        QTest::newRow("turned") << true << QVector3D(4.0f, 1.0f, 0.5f);
    }

    void test_aLabelQuadCoversTheLabelAndTurnsInPlace()
    {
        QFETCH(bool, turned);
        QFETCH(QVector3D, textureTopLeft);
        GeometryData quad;
        // Three wide and one tall, with its top-left corner at (1, 2)
        GeometryManager::appendLabelQuad(quad, QVector3D(1.0f, 2.0f, 0.5f), 3.0f, 1.0f, turned, QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
        QCOMPARE(quad.vertexCount(), 6);
        bool sawTextureTopLeft = false;
        for (int vertex = 0; vertex < 6; ++vertex) {
            const QVector3D position(quad.vertices[vertex * 3], quad.vertices[vertex * 3 + 1], quad.vertices[vertex * 3 + 2]);
            QVERIFY(position.x() == 1.0f || position.x() == 4.0f);
            QVERIFY(position.y() == 1.0f || position.y() == 2.0f);
            QCOMPARE(position.z(), 0.5f);
            if (quad.textureCoords[vertex * 2] == 0.0f && quad.textureCoords[vertex * 2 + 1] == 1.0f) {
                QCOMPARE(position, textureTopLeft);
                sawTextureTopLeft = true;
            }
        }
        QVERIFY(sawTextureTopLeft);
    }

    void test_vertexShaderAttributeLocationsDoNotOverlap()
    {
        const QList<ShaderAttribute> attributes = vertexShaderAttributes();
        QVERIFY2(!attributes.isEmpty(), "no attribute declarations were found in the vertex shader");
        QMap<int, QString> taken;
        for (const auto& attribute : attributes) {
            const int span = attribute.type == qsl("mat4") ? 4 : attribute.type == qsl("mat3") ? 3 : attribute.type == qsl("mat2") ? 2 : 1;
            for (int slot = attribute.location; slot < attribute.location + span; ++slot) {
                QVERIFY2(!taken.contains(slot), qPrintable(qsl("%1 and %2 both use attribute location %3").arg(taken.value(slot), attribute.name).arg(slot)));
                taken.insert(slot, attribute.name);
            }
        }
    }

    // GeometryManager binds these locations by number
    void test_vertexShaderAttributeLocationsMatchGeometryManager()
    {
        const QMap<QString, int> expected{{qsl("aPos"), 0},
                                          {qsl("aNormal"), 1},
                                          {qsl("aColor"), 2},
                                          {qsl("aInstanceColor"), 3},
                                          {qsl("aInstanceTransform"), 4},
                                          {qsl("aTexCoord"), static_cast<int>(GeometryManager::scmTexCoordLocation)}};
        QMap<QString, int> declared;
        for (const auto& attribute : vertexShaderAttributes()) {
            declared.insert(attribute.name, attribute.location);
        }
        QCOMPARE(declared, expected);
    }
};

#include "ModernMapperGeometryTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModernMapperGeometryTest)
