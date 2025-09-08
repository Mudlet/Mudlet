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

#include "pre_guard.h"
#include <QVector>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include "post_guard.h"

struct GeometryData {
    QVector<float> vertices;
    QVector<float> colors;
    QVector<float> normals;
    QVector<unsigned int> indices;
    
    void clear() {
        vertices.clear();
        colors.clear();
        normals.clear();
        indices.clear();
    }
    
    bool isEmpty() const {
        return vertices.isEmpty();
    }
    
    int vertexCount() const {
        return vertices.size() / 3;
    }
    
    int indexCount() const {
        return indices.size();
    }
    
    bool hasIndices() const {
        return !indices.isEmpty();
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
    
    // Render geometry using provided VAO and buffers
    void renderGeometry(const GeometryData& geometry, 
                       QOpenGLVertexArrayObject& vao,
                       QOpenGLBuffer& vertexBuffer,
                       QOpenGLBuffer& colorBuffer,
                       QOpenGLBuffer& normalBuffer,
                       QOpenGLBuffer& indexBuffer,
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
    
    // Function pointers for instancing (OpenGL 3.3+)
    typedef void (QOPENGLF_APIENTRYP PFNGLVERTEXATTRIBDIVISORPROC) (GLuint index, GLuint divisor);
    typedef void (QOPENGLF_APIENTRYP PFNGLDRAWELEMENTSINSTANCEDPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
    
    PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor = nullptr;
    PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced = nullptr;
    
    void generateCubeTemplate();
    GeometryData transformCubeTemplate(QMatrix4x4 transform, float r, float g, float b, float a);
};

#endif // MUDLET_GEOMETRY_MANAGER_H
