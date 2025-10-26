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

#include <QVector>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <optional>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct GeometryData {
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

    void clear() {
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

    void clearTexture() {
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

    bool isEmpty() const {
        return vertices.isEmpty();
    }

    int vertexCount() const {
        return vertices.size() / 6;
    }

    int indexCount() const {
        return indices.size();
    }

    bool hasIndices() const {
        return !indices.isEmpty();
    }

    bool hasTexture() const {
        return (textureId != 0 || baseColorTextureId != 0) && !textureCoords.isEmpty();
    }

    bool hasPBRTextures() const {
        return baseColorTextureId != 0 || metallicRoughnessTextureId != 0 || normalTextureId != 0;
    }
};

// Instance data for instanced cube rendering
struct CubeInstanceData {
    float color[4];             // r, g, b, a color components
    QMatrix4x4 transform;       // encodes scale -> rotate -> translate

    CubeInstanceData() = default;

    CubeInstanceData(QMatrix4x4 trafo, float r, float g, float b, float a) {
        color[0] = r; color[1] = g; color[2] = b; color[3] = a;
        transform = trafo;
    }
};

class GeometryManager : protected QOpenGLFunctions
{
public:
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

    // Instanced rendering methods for cube batching
    void renderInstancedCubes(const QVector<CubeInstanceData>& instances,
                             QOpenGLVertexArrayObject& vao,
                             QOpenGLBuffer& vertexBuffer,
                             QOpenGLBuffer& colorBuffer,
                             QOpenGLBuffer& normalBuffer,
                             QOpenGLBuffer& indexBuffer,
                             QOpenGLBuffer& instanceBuffer,
                             GLenum drawMode = GL_TRIANGLES);

    void renderInstancedCubes(const QVector<CubeInstanceData>& instances,
                             QOpenGLVertexArrayObject& vao,
                             QOpenGLBuffer& vertexBuffer,
                             QOpenGLBuffer& colorBuffer,
                             QOpenGLBuffer& normalBuffer,
                             QOpenGLBuffer& indexBuffer,
                             QOpenGLBuffer& instanceBuffer,
                             class ResourceManager* resourceManager,
                             GLenum drawMode = GL_TRIANGLES);

private:
    bool mInitialized = false;

    // Cached cube geometry template (will be transformed for each cube)
    GeometryData mCubeTemplate;

    // Cached player icon geometry
    mutable std::optional<GeometryData> mPlayerIconTemplate;

    // Function pointers for instancing (OpenGL 3.3+)
    typedef void (QOPENGLF_APIENTRYP PFNGLVERTEXATTRIBDIVISORPROC) (GLuint index, GLuint divisor);
    typedef void (QOPENGLF_APIENTRYP PFNGLDRAWELEMENTSINSTANCEDPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);

    PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor = nullptr;
    PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced = nullptr;

    void generateCubeTemplate();
    GeometryData transformCubeTemplate(QMatrix4x4 transform, float r, float g, float b, float a);

    void loadPlayerIconTemplate(float scale = 0.005f, float rotX = 0.0f, float rotY = 0.0f, float rotZ = 90.0f);
};

#endif // MUDLET_GEOMETRY_MANAGER_H
