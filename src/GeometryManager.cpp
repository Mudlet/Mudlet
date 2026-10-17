/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "GeometryManager.h"
#include "ResourceManager.h"

#include <QColor>
#include <QDebug>
#include <QFile>
#include <QHash>
#include <QImage>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace {
QVector<float> upNormals(int vertexCount)
{
    QVector<float> normals;
    normals.reserve(vertexCount * 3);
    for (int i = 0; i < vertexCount; ++i) {
        normals << 0.0f << 0.0f << 1.0f;
    }
    return normals;
}
} // namespace

GeometryManager::GeometryManager() {}

GeometryManager::~GeometryManager()
{
    cleanup();
}

void GeometryManager::initialize()
{
    if (mInitialized) {
        return;
    }

    initializeOpenGLFunctions();

    // Get function pointers for instancing (OpenGL 3.3+)
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (context) {
        glVertexAttribDivisor = reinterpret_cast<PFNGLVERTEXATTRIBDIVISORPROC>(context->getProcAddress("glVertexAttribDivisor"));
        glDrawElementsInstanced = reinterpret_cast<PFNGLDRAWELEMENTSINSTANCEDPROC>(context->getProcAddress("glDrawElementsInstanced"));
    }

    generateCubeTemplate();
    mShapeTemplates[static_cast<size_t>(InstanceShape::Box)].mesh = mCubeTemplate;
    mShapeTemplates[static_cast<size_t>(InstanceShape::Cylinder)].mesh = cylinderMesh(scmCylinderSegments);
    mShapeTemplates[static_cast<size_t>(InstanceShape::Cone)].mesh = coneMesh(scmCylinderSegments);
    mShapeTemplates[static_cast<size_t>(InstanceShape::Pyramid)].mesh = pyramidMesh();
    mShapeTemplates[static_cast<size_t>(InstanceShape::Sphere)].mesh = sphereMesh(scmCylinderSegments, scmCylinderSegments / 2);
    mInitialized = true;
}

void GeometryManager::cleanup()
{
    if (mPlayerIconTemplate.has_value()) {
        mPlayerIconTemplate->clearTexture();
    }

    for (ShapeTemplate& shape : mShapeTemplates) {
        shape.vertexBuffer.destroy();
        shape.indexBuffer.destroy();
        shape.mesh.clear();
    }
    mInstanceBuffer.destroy();
    mCubeTemplate.clear();
    mPlayerIconTemplate.reset();
    mInitialized = false;
}

void GeometryManager::generateCubeTemplate()
{
    // Generate unit cube centered at origin using indexed geometry
    mCubeTemplate.clear();

    const auto addVertex = [this](float x, float y, float z, float nx, float ny, float nz) {
        mCubeTemplate.vertices << x << y << z;
        mCubeTemplate.normals << nx << ny << nz;
    };

    // Define 24 unique vertices + normals for a unit cube
    // Vertex order: front face (counter-clockwise from bottom-left), then back face
    // Front bottom-left
    addVertex(-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f);  // 0: Front bottom-left (normal:front)
    addVertex(-1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f); // 1: Front bottom-left (normal:bottom)
    addVertex(-1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f); // 2: Front bottom-left (normal:left)
    // Front bottom-right
    addVertex(1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f);  // 3: Front bottom-right (normal:front)
    addVertex(1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f); // 4: Front bottom-right (normal:bottom)
    addVertex(1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f);  // 5: Front bottom-right (normal:right)
    // Front top-right
    addVertex(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f); // 6: Front top-right (normal:front)
    addVertex(1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f); // 7: Front top-right (normal:top)
    addVertex(1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f); // 8: Front top-right (normal:right)
    // Front top-left
    addVertex(-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);  // 9: Front top-left (normal:front)
    addVertex(-1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f);  // 10: Front top-left (normal:top)
    addVertex(-1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f); // 11: Front top-left (normal:left)
    // Back bottom-left
    addVertex(-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f); // 12: Back bottom-left (normal:back)
    addVertex(-1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f); // 13: Back bottom-left (normal:bottom)
    addVertex(-1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f); // 14: Back bottom-left (normal:left)
    // Back bottom-right
    addVertex(1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f); // 15: Back bottom-right (normal:back)
    addVertex(1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f); // 16: Back bottom-right (normal:bottom)
    addVertex(1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f);  // 17: Back bottom-right (normal:right)
    // Back top-right
    addVertex(1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f); // 18: Back top-right (normal:back)
    addVertex(1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f);  // 19: Back top-right (normal:top)
    addVertex(1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f);  // 20: Back top-right (normal:right)
    // Back top-left
    addVertex(-1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f); // 21: Back top-left (normal:back)
    addVertex(-1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f);  // 22: Back top-left (normal:top)
    addVertex(-1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f); // 23: Back top-left (normal:left)

    // Define indices for the 12 triangles (6 faces × 2 triangles each)
    // Counter-clockwise winding order for front-facing triangles
    QVector<unsigned int> indices = {
            // Front face
            0,
            3,
            6,
            0,
            6,
            9,
            // Back face
            12,
            15,
            18,
            12,
            18,
            21,
            // Left face
            2,
            11,
            23,
            2,
            23,
            14,
            // Right face
            5,
            8,
            20,
            5,
            20,
            17,
            // Bottom face
            1,
            4,
            16,
            1,
            13,
            16,
            // Top face
            7,
            10,
            22,
            7,
            22,
            19,
    };

    mCubeTemplate.indices = indices;

    // Colors will be set per instance, so we don't populate them in the template
}

GeometryData GeometryManager::transformTemplate(const GeometryData& geometry, const QMatrix4x4& transform, float r, float g, float b, float a)
{
    GeometryData result;
    const QMatrix4x4 normalTransform = transform.inverted().transposed();

    for (int i = 0; i < geometry.vertices.size(); i += 3) {
        const QVector3D vertex = transform.map(QVector3D(geometry.vertices[i], geometry.vertices[i + 1], geometry.vertices[i + 2]));
        result.vertices << vertex.x() << vertex.y() << vertex.z();

        const QVector3D normal = normalTransform.mapVector(QVector3D(geometry.normals[i], geometry.normals[i + 1], geometry.normals[i + 2])).normalized();
        result.normals << normal.x() << normal.y() << normal.z();

        result.colors << r << g << b << a;
    }

    // Copy indices (they don't need transformation)
    result.indices = geometry.indices;

    return result;
}

GeometryData GeometryManager::generateCubeGeometry(float x, float y, float z, float size, float r, float g, float b, float a)
{
    if (!mInitialized) {
        qWarning() << "GeometryManager: generateCubeGeometry called before initialize()";
        return GeometryData();
    }

    QMatrix4x4 transform = QMatrix4x4();
    transform.translate(x, y, z);
    transform.scale(size);
    return transformTemplate(mCubeTemplate, transform, r, g, b, a);
}

GeometryData GeometryManager::generateLineGeometry(const QVector<float>& vertices, const QVector<float>& colors)
{
    if (vertices.isEmpty() || colors.isEmpty()) {
        return GeometryData();
    }

    GeometryData result;
    result.vertices = vertices;
    result.colors = colors;
    result.normals = upNormals(vertices.size() / 3);

    return result;
}

GeometryData GeometryManager::generateTriangleGeometry(const QVector<float>& vertices, const QVector<float>& colors)
{
    if (vertices.isEmpty() || colors.isEmpty() || vertices.size() % 3 != 0 || colors.size() % 4 != 0) {
        qDebug() << "GeometryManager: Invalid vertex or color array size";
        return GeometryData();
    }

    // Check that we have the right ratio: 3 floats per vertex, 4 floats per color
    if (vertices.size() / 3 != colors.size() / 4) {
        qDebug() << "GeometryManager: Vertex count doesn't match color count";
        return GeometryData();
    }

    GeometryData result;
    result.vertices = vertices;
    result.colors = colors;
    result.normals = upNormals(vertices.size() / 3);

    return result;
}

GeometryData GeometryManager::generatePlayerIconGeometry(float scale, float rotX, float rotY, float rotZ)
{
    // Check if we need to regenerate the template
    static float lastScale = -1.0f;
    static float lastRotX = -999.0f;
    static float lastRotY = -999.0f;
    static float lastRotZ = -999.0f;

    bool parametersChanged = (scale != lastScale || rotX != lastRotX || rotY != lastRotY || rotZ != lastRotZ);

    // Only regenerate if parameters changed or template doesn't exist
    if (parametersChanged || !mPlayerIconTemplate.has_value()) {
        loadPlayerIconTemplate(scale, rotX, rotY, rotZ);
        lastScale = scale;
        lastRotX = rotX;
        lastRotY = rotY;
        lastRotZ = rotZ;
    }

    return mPlayerIconTemplate.value_or(GeometryData{});
}

void GeometryManager::clearPlayerIconTemplate()
{
    mPlayerIconTemplate.reset();
}

GeometryData
GeometryManager::generateBillboardGeometry(float centerX, float centerY, float centerZ, float width, float height, const QVector3D& cameraRight, const QVector3D& cameraUp, GLuint textureId)
{
    GeometryData result;

    // Calculate half dimensions
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;

    // Calculate the four corners of the billboard quad using camera vectors
    // This makes the quad always face the camera
    QVector3D center(centerX, centerY, centerZ);
    QVector3D bottomLeft = center - cameraRight * halfWidth - cameraUp * halfHeight;
    QVector3D bottomRight = center + cameraRight * halfWidth - cameraUp * halfHeight;
    QVector3D topRight = center + cameraRight * halfWidth + cameraUp * halfHeight;
    QVector3D topLeft = center - cameraRight * halfWidth + cameraUp * halfHeight;

    // Points toward the camera, as cameraRight is the screen's right
    QVector3D normal = QVector3D::crossProduct(cameraRight, cameraUp).normalized();

    // Two counter-clockwise triangles as seen from the camera
    result.vertices << bottomLeft.x() << bottomLeft.y() << bottomLeft.z();
    result.vertices << bottomRight.x() << bottomRight.y() << bottomRight.z();
    result.vertices << topRight.x() << topRight.y() << topRight.z();

    result.vertices << bottomLeft.x() << bottomLeft.y() << bottomLeft.z();
    result.vertices << topRight.x() << topRight.y() << topRight.z();
    result.vertices << topLeft.x() << topLeft.y() << topLeft.z();

    for (int i = 0; i < 6; ++i) {
        result.normals << normal.x() << normal.y() << normal.z();
    }

    result.textureCoords << 0.0f << 0.0f; // bottomLeft
    result.textureCoords << 1.0f << 0.0f; // bottomRight
    result.textureCoords << 1.0f << 1.0f; // topRight

    result.textureCoords << 0.0f << 0.0f; // bottomLeft
    result.textureCoords << 1.0f << 1.0f; // topRight
    result.textureCoords << 0.0f << 1.0f; // topLeft

    // Add white color for all vertices (texture will provide actual color)
    for (int i = 0; i < 6; ++i) {
        result.colors << 1.0f << 1.0f << 1.0f << 1.0f;
    }

    // Set texture ID
    result.textureId = textureId;

    return result;
}

std::pair<QVector3D, QVector3D> GeometryManager::groundAxes(const QVector3D& cameraRight, const QVector3D& cameraUp)
{
    // The screen's right laid flat on the map; looking straight along it, the screen's up laid flat
    // is perpendicular to it instead
    QVector2D right(cameraRight.x(), cameraRight.y());
    if (right.length() < 1e-3f) {
        right = QVector2D(cameraUp.y(), -cameraUp.x());
    }
    right = right.length() < 1e-3f ? QVector2D(1.0f, 0.0f) : right.normalized();
    return {QVector3D(right.x(), right.y(), 0.0f), QVector3D(-right.y(), right.x(), 0.0f)};
}

void GeometryManager::appendQuad(GeometryData& batch, const QVector3D& origin, const QVector3D& across, const QVector3D& upward, const QVector4D& color, const QRectF& textureRect)
{
    const QVector3D bottomLeft = origin;
    const QVector3D bottomRight = origin + across;
    const QVector3D topRight = origin + across + upward;
    const QVector3D topLeft = origin + upward;

    for (const QVector3D& corner : {bottomLeft, bottomRight, topRight, bottomLeft, topRight, topLeft}) {
        batch.vertices << corner.x() << corner.y() << corner.z();
        batch.normals << 0.0f << 0.0f << 1.0f;
        batch.colors << color.x() << color.y() << color.z() << color.w();
    }
    const auto left = static_cast<float>(textureRect.left());
    const auto right = static_cast<float>(textureRect.right());
    const auto bottom = static_cast<float>(textureRect.top());
    const auto top = static_cast<float>(textureRect.bottom());
    batch.textureCoords << left << bottom << right << bottom << right << top;
    batch.textureCoords << left << bottom << right << top << left << top;
}

void GeometryManager::appendLabelQuad(GeometryData& batch, const QVector3D& topLeft, const float width, const float height, const bool turned, const QVector4D& color)
{
    if (turned) {
        appendQuad(batch, topLeft + QVector3D(width, 0.0f, 0.0f), QVector3D(-width, 0.0f, 0.0f), QVector3D(0.0f, -height, 0.0f), color);
    } else {
        appendQuad(batch, topLeft - QVector3D(0.0f, height, 0.0f), QVector3D(width, 0.0f, 0.0f), QVector3D(0.0f, height, 0.0f), color);
    }
}

std::optional<int> GeometryManager::labelLevel(const qreal z)
{
    const qreal rounded = std::round(z);
    // Written so that NaN fails it too
    if (!(rounded >= std::numeric_limits<int>::min() && rounded <= std::numeric_limits<int>::max())) {
        return std::nullopt;
    }
    return static_cast<int>(rounded);
}

void GeometryManager::appendGroundQuad(
        GeometryData& batch, const QVector3D& center, const float size, const QVector3D& cameraRight, const QVector3D& cameraUp, const QVector4D& color, const QRectF& textureRect)
{
    const auto [right, up] = groundAxes(cameraRight, cameraUp);
    appendQuad(batch, center - (right + up) * (size / 2.0f), right * size, up * size, color, textureRect);
}

QList<qsizetype> GeometryManager::declutteredRects(const QList<QRectF>& rects, const qreal minimumHeight)
{
    // Kept rectangles are filed under every grid cell they touch, so each test looks only at near ones
    constexpr qreal cellSize = 64.0;
    const auto cellKey = [](const qint64 column, const qint64 row) {
        return (static_cast<quint64>(static_cast<quint32>(column)) << 32) | static_cast<quint32>(row);
    };
    QHash<quint64, QList<qsizetype>> keptByCell;
    QList<qsizetype> kept;
    for (qsizetype index = 0; index < rects.size(); ++index) {
        const QRectF& rect = rects.at(index);
        if (!rect.isValid() || rect.height() < minimumHeight) {
            continue;
        }
        const auto firstColumn = static_cast<qint64>(std::floor(rect.left() / cellSize));
        const auto lastColumn = static_cast<qint64>(std::floor(rect.right() / cellSize));
        const auto firstRow = static_cast<qint64>(std::floor(rect.top() / cellSize));
        const auto lastRow = static_cast<qint64>(std::floor(rect.bottom() / cellSize));
        bool overlaps = false;
        for (qint64 column = firstColumn; column <= lastColumn && !overlaps; ++column) {
            for (qint64 row = firstRow; row <= lastRow && !overlaps; ++row) {
                for (const qsizetype other : keptByCell.value(cellKey(column, row))) {
                    if (rects.at(other).intersects(rect)) {
                        overlaps = true;
                        break;
                    }
                }
            }
        }
        if (overlaps) {
            continue;
        }
        kept.append(index);
        for (qint64 column = firstColumn; column <= lastColumn; ++column) {
            for (qint64 row = firstRow; row <= lastRow; ++row) {
                keptByCell[cellKey(column, row)].append(index);
            }
        }
    }
    return kept;
}

bool GeometryManager::outranks(const NamePriority& first, const NamePriority& second)
{
    if (first.rank != second.rank) {
        return first.rank < second.rank;
    }
    if (first.playerDistance != second.playerDistance) {
        return first.playerDistance < second.playerDistance;
    }
    return first.cameraDistance < second.cameraDistance;
}

float GeometryManager::steppedOpacity(const float opacity, const float target, const qint64 elapsedMilliseconds, const int fadeMilliseconds)
{
    if (fadeMilliseconds <= 0) {
        return target;
    }
    const float step = static_cast<float>(qMax(qint64{0}, elapsedMilliseconds)) / static_cast<float>(fadeMilliseconds);
    return opacity < target ? qMin(target, opacity + step) : qMax(target, opacity - step);
}

int GeometryManager::textSharpness(const qreal pixelsPerTexel, const qreal devicePixelRatio, const qreal widthTexels, const int maxTextureSize)
{
    // A quarter pixel's slack, so that a name drawn at very nearly its size is not drawn twice as large for it
    const int wanted = qBound(1, qCeil(pixelsPerTexel * devicePixelRatio - 0.25), 3);
    // With a tenth to spare, as a font made larger is not hinted to exactly that many times as wide
    const int fitting = widthTexels > 0.0 ? qMax(1, qFloor(maxTextureSize / (1.1 * widthTexels))) : wanted;
    return qMin(wanted, fitting);
}

GeometryManager::LevelFade GeometryManager::levelFade(const int levelsAbove, const bool moreTransparent)
{
    const int distance = std::abs(levelsAbove);
    LevelFade fade;
    if (distance == 0) {
        return fade;
    }
    if (moreTransparent) {
        fade.towardBackground = distance == 1 ? 0.5f : (distance == 2 ? 0.8f : 0.95f);
        return fade;
    }
    if (levelsAbove > 0) {
        // Above the viewed level, so see-through enough not to hide it
        fade.towardBackground = std::min(0.8f, 0.35f + 0.15f * static_cast<float>(distance - 1));
        fade.opacity = std::max(0.1f, 0.35f - 0.1f * static_cast<float>(distance - 1));
    } else {
        fade.towardBackground = std::min(0.75f, 0.3f * static_cast<float>(distance));
    }
    return fade;
}

QVector4D GeometryManager::fadedColor(const QColor& color, const LevelFade& fade, const QColor& background)
{
    const float keep = 1.0f - fade.towardBackground;
    const auto mixed = [&](const float own, const float back) {
        return own * keep + back * fade.towardBackground;
    };
    return QVector4D(mixed(color.redF(), background.redF()), mixed(color.greenF(), background.greenF()), mixed(color.blueF(), background.blueF()), color.alphaF() * fade.opacity);
}

QColor GeometryManager::levelFadeTarget(const int levelsAbove, const QColor& background, const QColor& upperLevelColor, const QColor& lowerLevelColor)
{
    if (levelsAbove == 0) {
        return background;
    }
    const QColor& levelColor = levelsAbove > 0 ? upperLevelColor : lowerLevelColor;
    const float cover = levelColor.alphaF();
    const auto over = [cover](const float own, const float back) {
        return own * cover + back * (1.0f - cover);
    };
    return QColor::fromRgbF(over(levelColor.redF(), background.redF()), over(levelColor.greenF(), background.greenF()), over(levelColor.blueF(), background.blueF()));
}

QList<QLineF> GeometryManager::gridLines(const QRectF& extent)
{
    QList<QLineF> lines;
    if (!extent.isValid()) {
        return lines;
    }
    const auto firstX = static_cast<qint64>(std::ceil(extent.left()));
    const auto lastX = static_cast<qint64>(std::floor(extent.right()));
    const auto firstY = static_cast<qint64>(std::ceil(extent.top()));
    const auto lastY = static_cast<qint64>(std::floor(extent.bottom()));
    lines.reserve(qMax<qint64>(0, lastX - firstX + 1) + qMax<qint64>(0, lastY - firstY + 1));
    for (qint64 x = firstX; x <= lastX; ++x) {
        lines.append(QLineF(static_cast<qreal>(x), extent.top(), static_cast<qreal>(x), extent.bottom()));
    }
    for (qint64 y = firstY; y <= lastY; ++y) {
        lines.append(QLineF(extent.left(), static_cast<qreal>(y), extent.right(), static_cast<qreal>(y)));
    }
    return lines;
}

GeometryData GeometryManager::cylinderMesh(const int segments)
{
    GeometryData mesh;
    const int count = qMax(3, segments);
    const auto addVertex = [&mesh](const float x, const float y, const float z, const float nx, const float ny, const float nz) {
        mesh.vertices << x << y << z;
        mesh.normals << nx << ny << nz;
    };
    const auto rim = [count](const int index) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(index % count) / static_cast<float>(count);
        return std::pair<float, float>(std::cos(angle), std::sin(angle));
    };
    // The side's rings share no vertex with the caps, whose normals point straight up or down
    for (const float z : {-1.0f, 1.0f}) {
        for (int index = 0; index < count; ++index) {
            const auto [x, y] = rim(index);
            addVertex(x, y, z, x, y, 0.0f);
        }
    }
    const auto u = [](const int index) {
        return static_cast<unsigned int>(index);
    };
    for (int index = 0; index < count; ++index) {
        const int next = (index + 1) % count;
        mesh.indices << u(index) << u(next) << u(count + next);
        mesh.indices << u(index) << u(count + next) << u(count + index);
    }
    for (const float z : {1.0f, -1.0f}) {
        const int center = mesh.vertexCount();
        addVertex(0.0f, 0.0f, z, 0.0f, 0.0f, z);
        for (int index = 0; index < count; ++index) {
            const auto [x, y] = rim(index);
            addVertex(x, y, z, 0.0f, 0.0f, z);
        }
        for (int index = 0; index < count; ++index) {
            const int here = center + 1 + index;
            const int next = center + 1 + (index + 1) % count;
            if (z > 0.0f) {
                mesh.indices << u(center) << u(here) << u(next);
            } else {
                mesh.indices << u(center) << u(next) << u(here);
            }
        }
    }
    return mesh;
}

GeometryData GeometryManager::coneMesh(const int segments)
{
    GeometryData mesh;
    const int count = qMax(3, segments);
    const auto addVertex = [&mesh](const float x, const float y, const float z, const float nx, const float ny, const float nz) {
        mesh.vertices << x << y << z;
        mesh.normals << nx << ny << nz;
    };
    const auto u = [](const int index) {
        return static_cast<unsigned int>(index);
    };
    // Twice as tall as it is wide, so the side leans out by half a unit for each unit it falls
    const float slope = 1.0f / std::sqrt(5.0f);
    for (int index = 0; index < count; ++index) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(index) / static_cast<float>(count);
        addVertex(std::cos(angle), std::sin(angle), -1.0f, 2.0f * slope * std::cos(angle), 2.0f * slope * std::sin(angle), slope);
    }
    // A tip of its own for each face, leaning as the middle of the face does, so the side shades smoothly up to it
    for (int index = 0; index < count; ++index) {
        const float angle = 2.0f * std::numbers::pi_v<float> * (static_cast<float>(index) + 0.5f) / static_cast<float>(count);
        addVertex(0.0f, 0.0f, 1.0f, 2.0f * slope * std::cos(angle), 2.0f * slope * std::sin(angle), slope);
    }
    for (int index = 0; index < count; ++index) {
        mesh.indices << u(index) << u((index + 1) % count) << u(count + index);
    }
    const int center = mesh.vertexCount();
    addVertex(0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f);
    for (int index = 0; index < count; ++index) {
        const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(index) / static_cast<float>(count);
        addVertex(std::cos(angle), std::sin(angle), -1.0f, 0.0f, 0.0f, -1.0f);
    }
    for (int index = 0; index < count; ++index) {
        mesh.indices << u(center) << u(center + 1 + (index + 1) % count) << u(center + 1 + index);
    }
    return mesh;
}

GeometryData GeometryManager::pyramidMesh()
{
    GeometryData mesh;
    const auto addVertex = [&mesh](const QVector3D& at, const QVector3D& normal) {
        mesh.vertices << at.x() << at.y() << at.z();
        mesh.normals << normal.x() << normal.y() << normal.z();
    };
    const auto u = [](const int index) {
        return static_cast<unsigned int>(index);
    };
    const std::array<QVector3D, 4> corners{QVector3D(-1.0f, -1.0f, -1.0f), QVector3D(1.0f, -1.0f, -1.0f), QVector3D(1.0f, 1.0f, -1.0f), QVector3D(-1.0f, 1.0f, -1.0f)};
    const QVector3D tip(0.0f, 0.0f, 1.0f);
    for (size_t index = 0; index < corners.size(); ++index) {
        const QVector3D& from = corners.at(index);
        const QVector3D& to = corners.at((index + 1) % corners.size());
        const QVector3D normal = QVector3D::crossProduct(to - from, tip - from).normalized();
        const int first = mesh.vertexCount();
        addVertex(from, normal);
        addVertex(to, normal);
        addVertex(tip, normal);
        mesh.indices << u(first) << u(first + 1) << u(first + 2);
    }
    const int base = mesh.vertexCount();
    for (const QVector3D& corner : corners) {
        addVertex(corner, QVector3D(0.0f, 0.0f, -1.0f));
    }
    mesh.indices << u(base) << u(base + 2) << u(base + 1) << u(base) << u(base + 3) << u(base + 2);
    return mesh;
}

GeometryData GeometryManager::sphereMesh(const int segments, const int rings)
{
    GeometryData mesh;
    const int around = qMax(3, segments);
    const int down = qMax(2, rings);
    for (int ring = 0; ring <= down; ++ring) {
        const float polar = std::numbers::pi_v<float> * static_cast<float>(ring) / static_cast<float>(down);
        for (int index = 0; index <= around; ++index) {
            const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(index) / static_cast<float>(around);
            const float x = std::sin(polar) * std::cos(angle);
            const float y = std::sin(polar) * std::sin(angle);
            const float z = std::cos(polar);
            mesh.vertices << x << y << z;
            mesh.normals << x << y << z;
        }
    }
    const auto at = [around](const int ring, const int index) {
        return static_cast<unsigned int>(ring * (around + 1) + index);
    };
    for (int ring = 0; ring < down; ++ring) {
        for (int index = 0; index < around; ++index) {
            if (ring > 0) {
                mesh.indices << at(ring, index) << at(ring + 1, index) << at(ring, index + 1);
            }
            if (ring < down - 1) {
                mesh.indices << at(ring, index + 1) << at(ring + 1, index) << at(ring + 1, index + 1);
            }
        }
    }
    return mesh;
}

QVector4D GeometryManager::fadedDecalTint(const LevelFade& fade)
{
    return QVector4D(1.0f, 1.0f, 1.0f, (1.0f - fade.towardBackground) * fade.opacity);
}

QMatrix4x4 GeometryManager::barTransform(const QVector3D& from, const QVector3D& to, const float width, const float height)
{
    const QVector3D line = to - from;
    const float length = line.length();
    const QVector3D along = length > 0.0f ? line / length : QVector3D(1.0f, 0.0f, 0.0f);
    QVector3D side = QVector3D::crossProduct(QVector3D(0.0f, 0.0f, 1.0f), along);
    side = side.lengthSquared() < 1e-6f ? QVector3D(0.0f, 1.0f, 0.0f) : side.normalized();
    const QVector3D rise = QVector3D::crossProduct(along, side);
    const QVector3D center = (from + to) / 2.0f;
    const QVector3D x = along * (length / 2.0f);
    const QVector3D y = side * (width / 2.0f);
    const QVector3D z = rise * (height / 2.0f);
    return QMatrix4x4(x.x(), y.x(), z.x(), center.x(), x.y(), y.y(), z.y(), center.y(), x.z(), y.z(), z.z(), center.z(), 0.0f, 0.0f, 0.0f, 1.0f);
}

void GeometryManager::loadPlayerIconTemplate(float scale, float rotX, float rotY, float rotZ)
{
    GeometryData result;
    QFile file(":/3d-models/sword/sword.glb");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "GeometryManager: Failed to open sword GLB model file";
        mPlayerIconTemplate = result;
        return;
    }

    QByteArray modelData = file.readAll();
    file.close();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFileFromMemory(modelData.constData(), modelData.size(), aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        qWarning() << "GeometryManager: Failed to load sword model:" << importer.GetErrorString();
        mPlayerIconTemplate = result;
        return;
    }

    // Process the first mesh in model
    if (scene->mNumMeshes > 0) {
        const aiMesh* mesh = scene->mMeshes[0];

        // Create transformation matrices for the additional rotations
        QMatrix4x4 rotationX;
        rotationX.rotate(rotX, 1.0f, 0.0f, 0.0f);

        QMatrix4x4 rotationY;
        rotationY.rotate(rotY, 0.0f, 1.0f, 0.0f);

        QMatrix4x4 rotationZ;
        rotationZ.rotate(rotZ, 0.0f, 0.0f, 1.0f);

        // Base rotation to make sword point upward (90 degrees around Z)
        QMatrix4x4 baseRotation;
        baseRotation.rotate(90.0f, 0.0f, 0.0f, 1.0f);

        // Combined transformation: scale * user rotations * base rotation
        QMatrix4x4 combinedTransform = rotationZ * rotationY * rotationX;

        // Extract vertices, normals, and texture coordinates
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            // Original vertex
            QVector3D vertex(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            // Apply base rotation first (90-degree rotation around Z-axis to make sword point upward)
            QVector3D rotatedVertex(-vertex.y(), vertex.x(), vertex.z());

            // Apply user rotations
            rotatedVertex = combinedTransform.map(rotatedVertex);

            // Apply scaling
            rotatedVertex *= scale;

            result.vertices << rotatedVertex.x() << rotatedVertex.y() << rotatedVertex.z();

            // Apply same transformations to normals (normals shouldn't be scaled)
            if (mesh->HasNormals()) {
                QVector3D normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
                // Apply base rotation
                QVector3D rotatedNormal(-normal.y(), normal.x(), normal.z());
                // Apply user rotations
                rotatedNormal = combinedTransform.map(rotatedNormal);

                result.normals << rotatedNormal.x() << rotatedNormal.y() << rotatedNormal.z();
            } else {
                result.normals << 0.0f << 0.0f << 1.0f;
            }

            // Extract texture coordinates if available
            if (mesh->mTextureCoords[0]) {
                float u = mesh->mTextureCoords[0][i].x;
                float v = 1.0f - mesh->mTextureCoords[0][i].y; // Flip V coordinate for OpenGL
                result.textureCoords << u << v;

                // UV coordinates processed
            } else {
                result.textureCoords << 0.0f << 0.0f;
            }

            // Set color to white for textured rendering (texture will provide color)
            result.colors << 1.0f << 1.0f << 1.0f << 1.0f;
        }

        // Extract indices - ensure we have triangulated faces
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];
            if (face.mNumIndices == 3) { // Only process triangles
                result.indices.append(face.mIndices[0]);
                result.indices.append(face.mIndices[1]);
                result.indices.append(face.mIndices[2]);
            } else {
                qWarning() << "GeometryManager: Non-triangular face found with" << face.mNumIndices << "indices";
            }
        }

        // Process material textures
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            // Check for diffuse/base color texture
            aiTextureType textureType = aiTextureType_DIFFUSE;
            if (material->GetTextureCount(aiTextureType_BASE_COLOR) > 0) {
                textureType = aiTextureType_BASE_COLOR;
            }

            if (material->GetTextureCount(textureType) > 0) {
                aiString texturePath;
                material->GetTexture(textureType, 0, &texturePath);
                // Processing texture reference

                // Check if it's an embedded texture reference (format: *0, *1, etc.)
                QString texturePathStr = QString::fromStdString(texturePath.C_Str());
                if (texturePathStr.startsWith("*")) {
                    bool ok;
                    int textureIndex = texturePathStr.mid(1).toInt(&ok);
                    if (ok && textureIndex >= 0 && textureIndex < static_cast<int>(scene->mNumTextures)) {
                        // Using embedded texture
                        // We'll use this index below instead of hardcoded 0
                    }
                }
            }
        }

        // Load PBR textures if available
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            // Helper function to load a texture by type
            auto loadTextureByType = [&](aiTextureType textureType, unsigned int& textureId, const QString& typeName) {
                if (material->GetTextureCount(textureType) > 0) {
                    aiString texturePath;
                    aiTextureMapping mapping;
                    unsigned int uvIndex = 0;
                    ai_real blend;
                    aiTextureOp op;
                    aiTextureMapMode mapMode;

                    material->GetTexture(textureType, 0, &texturePath, &mapping, &uvIndex, &blend, &op, &mapMode);
                    // Texture uses UV channel

                    QString texturePathStr = QString::fromStdString(texturePath.C_Str());

                    if (texturePathStr.startsWith("*")) {
                        bool ok;
                        int materialTextureIndex = texturePathStr.mid(1).toInt(&ok);
                        if (ok && materialTextureIndex >= 0 && materialTextureIndex < static_cast<int>(scene->mNumTextures)) {
                            // Loading texture from index

                            const aiTexture* texture = scene->mTextures[materialTextureIndex];

                            // Create OpenGL texture
                            GLuint newTextureId;
                            glGenTextures(1, &newTextureId);
                            glBindTexture(GL_TEXTURE_2D, newTextureId);

                            // Create QImage from embedded texture data
                            QImage image;
                            if (texture->mHeight == 0) {
                                // Compressed texture data
                                QByteArray textureData(reinterpret_cast<const char*>(texture->pcData), texture->mWidth);
                                image = QImage::fromData(textureData);
                            } else {
                                // Uncompressed texture data (RGBA)
                                const unsigned char* data = reinterpret_cast<const unsigned char*>(texture->pcData);
                                image = QImage(data, texture->mWidth, texture->mHeight, QImage::Format_RGBA8888);
                            }

                            if (!image.isNull()) {
                                // Convert to OpenGL format and upload
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
                                QImage glImage = image.convertToFormat(QImage::Format_RGBA8888).flipped(Qt::Vertical);
#else
                                // Deprecated in 6.9 and due for removal in 6.13:
                                QImage glImage = image.convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);
#endif
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.constBits());

                                // Set texture parameters based on texture type
                                if (typeName == "base color") {
                                    // Base color textures benefit from mipmapping for distance
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                    glGenerateMipmap(GL_TEXTURE_2D);
                                } else if (typeName == "normal") {
                                    // Normal maps should use linear filtering, no mipmaps to preserve detail
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                } else {
                                    // Metallic/roughness and other maps
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                }
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                                // Set texture filtering

                                textureId = newTextureId;
                                // Texture loaded successfully
                            } else {
                                qWarning() << "GeometryManager: Failed to load" << typeName << "texture data";
                                glDeleteTextures(1, &newTextureId);
                            }
                        }
                    }
                }
            };

            // Load all PBR texture types
            loadTextureByType(aiTextureType_BASE_COLOR, result.baseColorTextureId, "base color");
            if (result.baseColorTextureId == 0) {
                loadTextureByType(aiTextureType_DIFFUSE, result.baseColorTextureId, "diffuse");
            }
            loadTextureByType(aiTextureType_METALNESS, result.metallicRoughnessTextureId, "metallic roughness");
            loadTextureByType(aiTextureType_NORMALS, result.normalTextureId, "normal");

            // Extract PBR material factors
            aiColor4D baseColor;
            if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS) {
                result.baseColorFactor[0] = baseColor.r;
                result.baseColorFactor[1] = baseColor.g;
                result.baseColorFactor[2] = baseColor.b;
                result.baseColorFactor[3] = baseColor.a;
            }

            material->Get(AI_MATKEY_METALLIC_FACTOR, result.metallicFactor);

            material->Get(AI_MATKEY_ROUGHNESS_FACTOR, result.roughnessFactor);

            // Check for emissive factor (material self-illumination)
            aiColor3D emissive;
            material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);

            // Check for opacity/transparency
            float opacity = 1.0f;
            material->Get(AI_MATKEY_OPACITY, opacity);

            // Set legacy textureId for backward compatibility with non-PBR rendering
            result.textureId = result.baseColorTextureId;
        }
    }

    qDebug() << "Loaded model" << file.fileName();

    mPlayerIconTemplate = std::move(result);
}

void GeometryManager::renderGeometry(
        const GeometryData& geometry, QOpenGLVertexArrayObject& vao, QOpenGLBuffer& vertexBuffer, QOpenGLBuffer& colorBuffer, QOpenGLBuffer& normalBuffer, QOpenGLBuffer& indexBuffer, GLenum drawMode)
{
    if (geometry.isEmpty()) {
        return;
    }

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Upload vertex data (cached)
    // Shader layout: location 0 = aPos (vec3)
    vertexBuffer.bind();
    if (!geometry.verticesUploaded) {
        vertexBuffer.allocate(geometry.vertices.data(), geometry.vertices.size() * sizeof(float));
        geometry.verticesUploaded = true;
    }
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    // Upload normal data (cached)
    // Shader layout: location 1 = aNormal (vec3)
    normalBuffer.bind();
    if (!geometry.normalsUploaded) {
        normalBuffer.allocate(geometry.normals.data(), geometry.normals.size() * sizeof(float));
        geometry.normalsUploaded = true;
    }
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);

    // Upload color data (cached)
    // Shader layout: location 2 = aColor (vec4)
    colorBuffer.bind();
    if (!geometry.colorsUploaded) {
        colorBuffer.allocate(geometry.colors.data(), geometry.colors.size() * sizeof(float));
        geometry.colorsUploaded = true;
    }
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(2);

    // A textured draw may have left this enabled, and this geometry has no texture coordinates to read
    glDisableVertexAttribArray(scmTexCoordLocation);

    // Draw the geometry - use indexed rendering if indices are available
    if (geometry.hasIndices()) {
        // Upload index data (cached)
        indexBuffer.bind();
        if (!geometry.indicesUploaded) {
            indexBuffer.allocate(geometry.indices.data(), geometry.indices.size() * sizeof(unsigned int));
            geometry.indicesUploaded = true;
        }

        // Draw using indices
        glDrawElements(drawMode, geometry.indexCount(), GL_UNSIGNED_INT, nullptr);
    } else {
        // Draw using vertex arrays (for flat triangles)
        glDrawArrays(drawMode, 0, geometry.vertexCount());
    }
}

void GeometryManager::renderGeometry(const GeometryData& geometry,
                                     QOpenGLVertexArrayObject& vao,
                                     QOpenGLBuffer& vertexBuffer,
                                     QOpenGLBuffer& colorBuffer,
                                     QOpenGLBuffer& normalBuffer,
                                     QOpenGLBuffer& indexBuffer,
                                     ResourceManager* resourceManager,
                                     GLenum drawMode)
{
    if (geometry.isEmpty()) {
        return;
    }

    // Call the original render method
    renderGeometry(geometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, drawMode);

    // Track draw call statistics
    if (resourceManager) {
        if (geometry.hasIndices()) {
            resourceManager->onDrawCall(geometry.indexCount() / 3); // Count triangles for indexed geometry
        } else {
            resourceManager->onDrawCall(geometry.vertexCount());
        }
    }
}

void GeometryManager::renderGeometry(const GeometryData& geometry,
                                     QOpenGLVertexArrayObject& vao,
                                     QOpenGLBuffer& vertexBuffer,
                                     QOpenGLBuffer& colorBuffer,
                                     QOpenGLBuffer& normalBuffer,
                                     QOpenGLBuffer& indexBuffer,
                                     QOpenGLBuffer& texCoordBuffer,
                                     GLenum drawMode)
{
    if (geometry.isEmpty()) {
        return;
    }

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Bind PBR textures if available
    if (geometry.hasPBRTextures()) {
        // Bind base color texture to unit 0
        if (geometry.baseColorTextureId != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, geometry.baseColorTextureId);
        }

        // Bind metallic/roughness texture to unit 1
        if (geometry.metallicRoughnessTextureId != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, geometry.metallicRoughnessTextureId);
        }

        // Bind normal texture to unit 2
        if (geometry.normalTextureId != 0) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, geometry.normalTextureId);
        }

        // Reset to texture unit 0
        glActiveTexture(GL_TEXTURE0);
    } else if (geometry.hasTexture()) {
        // Fallback to legacy single texture
        // Binding legacy texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, geometry.textureId);
    } else {
        // No textures available for rendering
    }

    // Upload vertex data (cached)
    // Shader layout: location 0 = aPos (vec3)
    vertexBuffer.bind();
    if (!geometry.verticesUploaded) {
        vertexBuffer.allocate(geometry.vertices.data(), geometry.vertices.size() * sizeof(float));
        geometry.verticesUploaded = true;
    }
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    // Upload normal data (cached)
    // Shader layout: location 1 = aNormal (vec3)
    normalBuffer.bind();
    if (!geometry.normalsUploaded) {
        normalBuffer.allocate(geometry.normals.data(), geometry.normals.size() * sizeof(float));
        geometry.normalsUploaded = true;
    }
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);

    // Upload color data (cached)
    // Shader layout: location 2 = aColor (vec4)
    colorBuffer.bind();
    if (!geometry.colorsUploaded) {
        colorBuffer.allocate(geometry.colors.data(), geometry.colors.size() * sizeof(float));
        geometry.colorsUploaded = true;
    }
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(2);

    // Upload texture coordinate data if available (cached)
    if (!geometry.textureCoords.isEmpty()) {
        texCoordBuffer.bind();
        if (!geometry.texCoordsUploaded) {
            texCoordBuffer.allocate(geometry.textureCoords.data(), geometry.textureCoords.size() * sizeof(float));
            geometry.texCoordsUploaded = true;
        }
        glVertexAttribPointer(scmTexCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(scmTexCoordLocation);
    } else {
        glDisableVertexAttribArray(scmTexCoordLocation);
    }

    // Draw the geometry - use indexed rendering if indices are available
    if (geometry.hasIndices()) {
        // Upload index data (cached)
        indexBuffer.bind();
        if (!geometry.indicesUploaded) {
            indexBuffer.allocate(geometry.indices.data(), geometry.indices.size() * sizeof(unsigned int));
            geometry.indicesUploaded = true;
        }

        // Draw using indices
        glDrawElements(drawMode, geometry.indexCount(), GL_UNSIGNED_INT, nullptr);
    } else {
        // Draw using vertex arrays (for lines and triangles)
        glDrawArrays(drawMode, 0, geometry.vertexCount());
    }
}

void GeometryManager::renderGeometry(const GeometryData& geometry,
                                     QOpenGLVertexArrayObject& vao,
                                     QOpenGLBuffer& vertexBuffer,
                                     QOpenGLBuffer& colorBuffer,
                                     QOpenGLBuffer& normalBuffer,
                                     QOpenGLBuffer& indexBuffer,
                                     QOpenGLBuffer& texCoordBuffer,
                                     ResourceManager* resourceManager,
                                     GLenum drawMode)
{
    if (geometry.isEmpty()) {
        return;
    }

    // Call the textured render method
    renderGeometry(geometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, texCoordBuffer, drawMode);

    // Track draw call statistics
    if (resourceManager) {
        if (geometry.hasIndices()) {
            resourceManager->onDrawCall(geometry.indexCount() / 3); // Count triangles for indexed geometry
        } else {
            resourceManager->onDrawCall(geometry.vertexCount());
        }
    }
}

void GeometryManager::renderInstancedCubes(const QVector<CubeInstanceData>& instances,
                                           QOpenGLVertexArrayObject& vao,
                                           QOpenGLBuffer& vertexBuffer,
                                           QOpenGLBuffer& colorBuffer,
                                           QOpenGLBuffer& normalBuffer,
                                           QOpenGLBuffer& indexBuffer,
                                           GLenum drawMode,
                                           const InstanceShape shape)
{
    if (!mInitialized || instances.isEmpty()) {
        return;
    }
    ShapeTemplate& shapeTemplate = mShapeTemplates[static_cast<size_t>(shape)];
    const GeometryData& mesh = shapeTemplate.mesh;
    QOpenGLBuffer& meshVertexBuffer = shapeTemplate.vertexBuffer;
    QOpenGLBuffer& meshIndexBuffer = shapeTemplate.indexBuffer;

    // Check if instancing functions are available
    if (!glVertexAttribDivisor || !glDrawElementsInstanced) {
        qWarning() << "GeometryManager: Instancing functions not available, falling back to individual meshes";
        for (const auto& instance : instances) {
            GeometryData cubeGeometry = transformTemplate(mesh, instance.transform, instance.color[0], instance.color[1], instance.color[2], instance.color[3]);
            renderGeometry(cubeGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, drawMode);
        }
        return;
    }

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    if (!meshVertexBuffer.isCreated()) {
        uploadTemplate(mesh, meshVertexBuffer, meshIndexBuffer);
    }
    if (!mInstanceBuffer.isCreated()) {
        mInstanceBuffer.create();
        mInstanceBuffer.setUsagePattern(QOpenGLBuffer::StreamDraw);
    }

    const int positionBytes = static_cast<int>(mesh.vertices.size() * sizeof(float));
    meshVertexBuffer.bind();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(static_cast<quintptr>(positionBytes)));
    glEnableVertexAttribArray(1);
    meshIndexBuffer.bind();

    // The shader takes color from the instance; a stale per-vertex color array may be shorter than the mesh
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(scmTexCoordLocation);

    mInstanceBuffer.bind();
    mInstanceBuffer.allocate(instances.data(), static_cast<int>(instances.size() * sizeof(CubeInstanceData)));

    // Set up instance attributes
    // Color: location 3
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(CubeInstanceData), reinterpret_cast<void*>(0));
    glVertexAttribDivisor(3, 1);

    // Transform matrix: location 4-7
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(CubeInstanceData), reinterpret_cast<void*>(4 * sizeof(float)));
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(CubeInstanceData), reinterpret_cast<void*>(8 * sizeof(float)));
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(CubeInstanceData), reinterpret_cast<void*>(12 * sizeof(float)));
    glVertexAttribDivisor(6, 1);
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(CubeInstanceData), reinterpret_cast<void*>(16 * sizeof(float)));
    glVertexAttribDivisor(7, 1);

    // Draw all instances with a single call
    glDrawElementsInstanced(drawMode, mesh.indexCount(), GL_UNSIGNED_INT, nullptr, instances.size());

    // Clean up instance attributes
    glVertexAttribDivisor(3, 0);
    glVertexAttribDivisor(4, 0);
    glVertexAttribDivisor(5, 0);
    glVertexAttribDivisor(6, 0);
    glVertexAttribDivisor(7, 0);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);
    glDisableVertexAttribArray(5);
    glDisableVertexAttribArray(6);
    glDisableVertexAttribArray(7);
}

void GeometryManager::uploadTemplate(const GeometryData& geometry, QOpenGLBuffer& vertexBuffer, QOpenGLBuffer& indexBuffer)
{
    const int positionBytes = static_cast<int>(geometry.vertices.size() * sizeof(float));
    const int normalBytes = static_cast<int>(geometry.normals.size() * sizeof(float));
    vertexBuffer.create();
    vertexBuffer.bind();
    vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vertexBuffer.allocate(positionBytes + normalBytes);
    vertexBuffer.write(0, geometry.vertices.constData(), positionBytes);
    vertexBuffer.write(positionBytes, geometry.normals.constData(), normalBytes);

    indexBuffer.create();
    indexBuffer.bind();
    indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    indexBuffer.allocate(geometry.indices.constData(), static_cast<int>(geometry.indices.size() * sizeof(unsigned int)));
}

void GeometryManager::renderInstancedCubes(const QVector<CubeInstanceData>& instances,
                                           QOpenGLVertexArrayObject& vao,
                                           QOpenGLBuffer& vertexBuffer,
                                           QOpenGLBuffer& colorBuffer,
                                           QOpenGLBuffer& normalBuffer,
                                           QOpenGLBuffer& indexBuffer,
                                           ResourceManager* resourceManager,
                                           GLenum drawMode,
                                           const InstanceShape shape)
{
    if (instances.isEmpty()) {
        return;
    }

    // Call the original instanced render method
    renderInstancedCubes(instances, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, drawMode, shape);

    // Track draw call statistics - one draw call for all instances
    if (resourceManager) {
        const GeometryData& mesh = mShapeTemplates[static_cast<size_t>(shape)].mesh;
        resourceManager->onDrawCall(instances.size() * (mesh.indexCount() / 3)); // Count triangles for all instances
    }
}
