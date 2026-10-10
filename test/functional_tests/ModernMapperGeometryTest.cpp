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
 * three floats of position per vertex in one buffer and three floats of normal
 * in another, so anything else in the position array is drawn as a vertex.
 * A mat4 vertex attribute takes four consecutive locations, so an attribute
 * declared inside that span aliases one of its columns.
 */

#include <QtTest/QtTest>

#include <QFile>
#include <QRegularExpression>

#include "GeometryManager.h"

#include "GroupedTest.h"

class ModernMapperGeometryTest : public QObject
{
    Q_OBJECT

private:
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

    void test_vertexShaderAttributeLocationsDoNotOverlap()
    {
        QFile shader(qsl(":/shaders/vertex.glsl"));
        QVERIFY2(shader.open(QIODevice::ReadOnly | QIODevice::Text), "the vertex shader is not in the resources");
        const QString source = QString::fromUtf8(shader.readAll());

        static const QRegularExpression attribute(qsl(R"(layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*in\s+(\w+)\s+(\w+))"));
        QMap<int, QString> taken;
        auto matches = attribute.globalMatch(source);
        QVERIFY2(matches.hasNext(), "no attribute declarations were found in the vertex shader");
        while (matches.hasNext()) {
            const auto match = matches.next();
            const int location = match.captured(1).toInt();
            const QString type = match.captured(2);
            const QString name = match.captured(3);
            const int span = type == qsl("mat4") ? 4 : type == qsl("mat3") ? 3 : type == qsl("mat2") ? 2 : 1;
            for (int slot = location; slot < location + span; ++slot) {
                QVERIFY2(!taken.contains(slot), qPrintable(qsl("%1 and %2 both use attribute location %3").arg(taken.value(slot), name).arg(slot)));
                taken.insert(slot, name);
            }
        }
    }
};

#include "ModernMapperGeometryTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModernMapperGeometryTest)
