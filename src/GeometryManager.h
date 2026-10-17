#ifndef MUDLET_GEOMETRY_MANAGER_H
#define MUDLET_GEOMETRY_MANAGER_H

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

#include <QLineF>
#include <QList>
#include <QRectF>
#include <QVector>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <optional>
#include <utility>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class QColor;

struct GeometryData
{
    QVector<float> vertices;
    QVector<float> colors;
    QVector<float> normals;
    QVector<float> textureCoords;
    QVector<unsigned int> indices;

    // PBR textures
    unsigned int baseColorTextureId = 0;
    unsigned int metallicRoughnessTextureId = 0;
    unsigned int normalTextureId = 0;

    // PBR material factors
    float baseColorFactor[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // RGBA
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    // Non-PBR texture support
    unsigned int textureId = 0;

    mutable bool verticesUploaded = false;
    mutable bool colorsUploaded = false;
    mutable bool normalsUploaded = false;
    mutable bool texCoordsUploaded = false;
    mutable bool indicesUploaded = false;

    void clear()
    {
        vertices.clear();
        colors.clear();
        normals.clear();
        textureCoords.clear();
        indices.clear();
        textureId = 0;
        baseColorTextureId = 0;
        metallicRoughnessTextureId = 0;
        normalTextureId = 0;
        baseColorFactor[0] = baseColorFactor[1] = baseColorFactor[2] = baseColorFactor[3] = 1.0f;
        metallicFactor = 1.0f;
        roughnessFactor = 1.0f;
        verticesUploaded = false;
        colorsUploaded = false;
        normalsUploaded = false;
        texCoordsUploaded = false;
        indicesUploaded = false;
        // Note: texture cleanup is handled by clearTexture()
    }

    void clearTexture()
    {
        if (textureId != 0) {
            glDeleteTextures(1, &textureId);
            textureId = 0;
        }
        if (baseColorTextureId != 0) {
            glDeleteTextures(1, &baseColorTextureId);
            baseColorTextureId = 0;
        }
        if (metallicRoughnessTextureId != 0) {
            glDeleteTextures(1, &metallicRoughnessTextureId);
            metallicRoughnessTextureId = 0;
        }
        if (normalTextureId != 0) {
            glDeleteTextures(1, &normalTextureId);
            normalTextureId = 0;
        }
    }

    bool isEmpty() const { return vertices.isEmpty(); }

    // Positions are tightly packed, three floats per vertex; normals live in their own array
    int vertexCount() const { return vertices.size() / 3; }

    int indexCount() const { return indices.size(); }

    bool hasIndices() const { return !indices.isEmpty(); }

    bool hasTexture() const { return (textureId != 0 || baseColorTextureId != 0) && !textureCoords.isEmpty(); }

    bool hasPBRTextures() const { return baseColorTextureId != 0 || metallicRoughnessTextureId != 0 || normalTextureId != 0; }
};

// Instance data for instanced cube rendering
struct CubeInstanceData
{
    float color[4];       // r, g, b, a color components
    QMatrix4x4 transform; // encodes scale -> rotate -> translate

    CubeInstanceData() = default;

    CubeInstanceData(const QMatrix4x4& trafo, float r, float g, float b, float a)
    {
        color[0] = r;
        color[1] = g;
        color[2] = b;
        color[3] = a;
        transform = trafo;
    }
};

// A stretch of a RetainedInstances' instances, drawn in one call
struct InstanceRun
{
    qsizetype first = 0;
    qsizetype count = 0;
};

// Instances kept in a buffer on the GPU from frame to frame, so that a frame uploads only what changed
struct RetainedInstances
{
    QVector<CubeInstanceData> instances;
    QOpenGLBuffer buffer{QOpenGLBuffer::VertexBuffer};
    // The buffer is allocated again when the count changes, otherwise just these instances are written again
    bool resized = true;
    qsizetype changedFrom = 0;
    qsizetype changedTo = 0;

    void setInstances(QVector<CubeInstanceData>&& fresh);
};

// Flat triangles kept in buffers on the GPU from frame to frame, written again only once set to other arrays
struct RetainedTriangles
{
    RetainedTriangles() = default;
    // A copy would share the buffers but not know when the other let go of them
    Q_DISABLE_COPY_MOVE(RetainedTriangles)

    // Three coordinates and four color components a corner
    QVector<float> vertices;
    QVector<float> colors;
    QOpenGLBuffer vertexBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer colorBuffer{QOpenGLBuffer::VertexBuffer};
    bool written = false;

    void setTriangles(const QVector<float>& freshVertices, const QVector<float>& freshColors);
    // Needs the buffers' context current
    void release();
};

class GeometryManager : protected QOpenGLFunctions
{
public:
    // Must match the layout location of aTexCoord in shaders/vertex.glsl
    static constexpr GLuint scmTexCoordLocation = 8;

    // What each instance's transform places: the cube [-1, 1], or an upright cylinder of radius 1 from z -1 to 1
    enum class InstanceShape { Box, Cylinder };

    GeometryManager();
    ~GeometryManager();

    void initialize();
    void cleanup();

    // Generate geometry data for different primitives
    GeometryData generateCubeGeometry(float x, float y, float z, float size, float r, float g, float b, float a);
    GeometryData generateLineGeometry(const QVector<float>& vertices, const QVector<float>& colors);
    GeometryData generateTriangleGeometry(const QVector<float>& vertices, const QVector<float>& colors);
    GeometryData generatePlayerIconGeometry(float scale = 0.005f, float rotX = 0.0f, float rotY = 0.0f, float rotZ = 90.0f);
    void clearPlayerIconTemplate(); // Clear cached template to free memory

    // Generate billboard (camera-facing quad) geometry for labels; cameraRight and cameraUp are the screen's right and up in world space
    GeometryData generateBillboardGeometry(float centerX, float centerY, float centerZ, float width, float height, const QVector3D& cameraRight, const QVector3D& cameraUp, GLuint textureId);

    // Appends a square lying flat on the map, centered on center, its texture's top turned toward the top of the
    // screen so that it is never upside down or mirrored; cameraRight and cameraUp as for a billboard
    static void appendGroundQuad(GeometryData& batch, const QVector3D& center, float size, const QVector3D& cameraRight, const QVector3D& cameraUp, const QVector4D& color);
    // The unit vectors on the map's plane that appendGroundQuad() turns a texture's right and top toward
    static std::pair<QVector3D, QVector3D> groundAxes(const QVector3D& cameraRight, const QVector3D& cameraUp);
    // Appends a textured parallelogram: the texture's bottom-left at origin, its bottom edge along across
    // and its left edge along upward
    static void appendQuad(GeometryData& batch, const QVector3D& origin, const QVector3D& across, const QVector3D& upward, const QVector4D& color);
    // Appends a map label lying flat with its top-left corner at topLeft; turned, it is rotated half
    // around in place, so that it reads upright while the top of the screen faces south
    static void appendLabelQuad(GeometryData& batch, const QVector3D& topLeft, float width, float height, bool turned, const QVector4D& color);
    // The level a map label is on: its height rounded, as the 2D map picks labels by, or none for a
    // height that no level could have
    static std::optional<int> labelLevel(qreal z);
    // Places the unit cube [-1, 1] as a bar from one point to another, width across and height tall,
    // with its width kept level with the map
    static QMatrix4x4 barTransform(const QVector3D& from, const QVector3D& to, float width, float height);
    // The indices of the screen rectangles to keep, taking them in the order given: one shorter than
    // minimumHeight, or overlapping one kept before it, is left out
    static QList<qsizetype> declutteredRects(const QList<QRectF>& rects, qreal minimumHeight);

    // How a level other than the viewed one recedes: how far its colors go toward the background, and
    // how opaque it stays
    struct LevelFade
    {
        float towardBackground = 0.0f;
        float opacity = 1.0f;
    };
    // levelsAbove is how many levels above the viewed one, negative for below
    static LevelFade levelFade(int levelsAbove, bool moreTransparent);
    static QVector4D fadedColor(const QColor& color, const LevelFade& fade, const QColor& background);
    // What a level fades toward when the mapper draws the levels above and below in their own colors: the
    // upper or lower level color laid over the background by its alpha, or the background on the viewed level
    static QColor levelFadeTarget(int levelsAbove, const QColor& background, const QColor& upperLevelColor, const QColor& lowerLevelColor);
    // The map grid's lines across extent, one at each whole coordinate as the 2D map draws them, through rooms' centers
    static QList<QLineF> gridLines(const QRectF& extent);
    // An upright cylinder of radius 1 from z -1 to 1, its side smooth shaded, each face wound counterclockwise from outside
    static GeometryData cylinderMesh(int segments);
    // The sides of the cylinder a round room is drawn as; a box selection tests the corners of that same rim
    static constexpr int scmCylinderSegments = 24;
    // The tint for a texture drawn on a faded room, which can only make it more see-through
    static QVector4D fadedDecalTint(const LevelFade& fade);

    // Render geometry using provided VAO and buffers
    void renderGeometry(const GeometryData& geometry,
                        QOpenGLVertexArrayObject& vao,
                        QOpenGLBuffer& vertexBuffer,
                        QOpenGLBuffer& colorBuffer,
                        QOpenGLBuffer& normalBuffer,
                        QOpenGLBuffer& indexBuffer,
                        GLenum drawMode = GL_TRIANGLES);

    // Render geometry with texture coordinate support
    void renderGeometry(const GeometryData& geometry,
                        QOpenGLVertexArrayObject& vao,
                        QOpenGLBuffer& vertexBuffer,
                        QOpenGLBuffer& colorBuffer,
                        QOpenGLBuffer& normalBuffer,
                        QOpenGLBuffer& indexBuffer,
                        QOpenGLBuffer& texCoordBuffer,
                        GLenum drawMode = GL_TRIANGLES);

    // Render geometry with resource tracking
    void renderGeometry(const GeometryData& geometry,
                        QOpenGLVertexArrayObject& vao,
                        QOpenGLBuffer& vertexBuffer,
                        QOpenGLBuffer& colorBuffer,
                        QOpenGLBuffer& normalBuffer,
                        QOpenGLBuffer& indexBuffer,
                        class ResourceManager* resourceManager,
                        GLenum drawMode = GL_TRIANGLES);

    // Render geometry with texture coordinate support and resource tracking
    void renderGeometry(const GeometryData& geometry,
                        QOpenGLVertexArrayObject& vao,
                        QOpenGLBuffer& vertexBuffer,
                        QOpenGLBuffer& colorBuffer,
                        QOpenGLBuffer& normalBuffer,
                        QOpenGLBuffer& indexBuffer,
                        QOpenGLBuffer& texCoordBuffer,
                        class ResourceManager* resourceManager,
                        GLenum drawMode = GL_TRIANGLES);

    // Instanced rendering of boxes or cylinders
    void renderInstancedCubes(const QVector<CubeInstanceData>& instances,
                              QOpenGLVertexArrayObject& vao,
                              QOpenGLBuffer& vertexBuffer,
                              QOpenGLBuffer& colorBuffer,
                              QOpenGLBuffer& normalBuffer,
                              QOpenGLBuffer& indexBuffer,
                              GLenum drawMode = GL_TRIANGLES,
                              InstanceShape shape = InstanceShape::Box);

    void renderInstancedCubes(const QVector<CubeInstanceData>& instances,
                              QOpenGLVertexArrayObject& vao,
                              QOpenGLBuffer& vertexBuffer,
                              QOpenGLBuffer& colorBuffer,
                              QOpenGLBuffer& normalBuffer,
                              QOpenGLBuffer& indexBuffer,
                              class ResourceManager* resourceManager,
                              GLenum drawMode = GL_TRIANGLES,
                              InstanceShape shape = InstanceShape::Box);

    // Draws runs of retained instances, first writing to its buffer whatever changed since it was last drawn
    void renderInstancedRuns(RetainedInstances& retained,
                             const QList<InstanceRun>& runs,
                             QOpenGLVertexArrayObject& vao,
                             QOpenGLBuffer& vertexBuffer,
                             QOpenGLBuffer& colorBuffer,
                             QOpenGLBuffer& normalBuffer,
                             QOpenGLBuffer& indexBuffer,
                             class ResourceManager* resourceManager,
                             GLenum drawMode = GL_TRIANGLES,
                             InstanceShape shape = InstanceShape::Box);

    void renderRetainedTriangles(RetainedTriangles& retained, QOpenGLVertexArrayObject& vao, class ResourceManager* resourceManager);

private:
    bool mInitialized = false;

    // Cached cube geometry template (will be transformed for each cube)
    GeometryData mCubeTemplate;
    QOpenGLBuffer mCubeVertexBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer mCubeIndexBuffer{QOpenGLBuffer::IndexBuffer};
    GeometryData mCylinderTemplate;
    QOpenGLBuffer mCylinderVertexBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer mCylinderIndexBuffer{QOpenGLBuffer::IndexBuffer};
    QOpenGLBuffer mInstanceBuffer{QOpenGLBuffer::VertexBuffer};

    // Cached player icon geometry
    mutable std::optional<GeometryData> mPlayerIconTemplate;

    // Function pointers for instancing (OpenGL 3.3+)
    typedef void(QOPENGLF_APIENTRYP PFNGLVERTEXATTRIBDIVISORPROC)(GLuint index, GLuint divisor);
    typedef void(QOPENGLF_APIENTRYP PFNGLDRAWELEMENTSINSTANCEDPROC)(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount);

    PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor = nullptr;
    PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced = nullptr;

    void generateCubeTemplate();
    // Binds the shape's mesh to the bound vertex array, leaving its instance attributes to be pointed
    const GeometryData& bindInstanceMesh(InstanceShape shape);
    // Points the instance attributes at the bound buffer's instances, from offset bytes in
    void pointInstanceAttributes(quintptr offset);
    void releaseInstanceAttributes();
    static void uploadTemplate(const GeometryData& geometry, QOpenGLBuffer& vertexBuffer, QOpenGLBuffer& indexBuffer);
    static GeometryData transformTemplate(const GeometryData& geometry, const QMatrix4x4& transform, float r, float g, float b, float a);

    void loadPlayerIconTemplate(float scale = 0.005f, float rotX = 0.0f, float rotY = 0.0f, float rotZ = 90.0f);
};

#endif // MUDLET_GEOMETRY_MANAGER_H
