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

#include <QFile>
#include <QRegularExpression>

#include "GeometryManager.h"
#include "LabelTextureCache.h"

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
