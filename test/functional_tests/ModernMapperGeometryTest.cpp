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
