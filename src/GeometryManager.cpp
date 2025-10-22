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

#include <QDebug>

GeometryManager::GeometryManager()
{
}

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
    mInitialized = true;
}

void GeometryManager::cleanup()
{
    mCubeTemplate.clear();
    mInitialized = false;
}

void GeometryManager::generateCubeTemplate()
{
    // Generate unit cube centered at origin using indexed geometry
    mCubeTemplate.clear();

    // Define 24 unique vertices + normals for a unit cube
    // Vertex order: front face (counter-clockwise from bottom-left), then back face
    // Front bottom-left
    mCubeTemplate.vertices << -1.0f << -1.0f <<  1.0f << 0.0 << 0.0 << 1.0;   // 0: Front bottom-left (normal:front)
    mCubeTemplate.vertices << -1.0f << -1.0f <<  1.0f << 0.0 << -1.0 << 0.0;  // 1: Front bottom-left (normal:bottom)
    mCubeTemplate.vertices << -1.0f << -1.0f <<  1.0f << -1.0 << 0.0 << 0.0;  // 2: Front bottom-left (normal:left)
    // Front bottom-right
    mCubeTemplate.vertices <<  1.0f << -1.0f <<  1.0f << 0.0 << 0.0 << 1.0;   // 3: Front bottom-right (normal:front)
    mCubeTemplate.vertices <<  1.0f << -1.0f <<  1.0f << 0.0 << -1.0 << 0.0;  // 4: Front bottom-right (normal:bottom)
    mCubeTemplate.vertices <<  1.0f << -1.0f <<  1.0f << 1.0 << 0.0 << 0.0;   // 5: Front bottom-right (normal:right)
    // Front top-right
    mCubeTemplate.vertices <<  1.0f <<  1.0f <<  1.0f << 0.0 << 0.0 << 1.0;   // 6: Front top-right (normal:front)
    mCubeTemplate.vertices <<  1.0f <<  1.0f <<  1.0f << 0.0 << 1.0 << 0.0;   // 7: Front top-right (normal:top)
    mCubeTemplate.vertices <<  1.0f <<  1.0f <<  1.0f << 1.0 << 0.0 << 0.0;   // 8: Front top-right (normal:right)
    // Front top-left
    mCubeTemplate.vertices << -1.0f <<  1.0f <<  1.0f << 0.0 << 0.0 << 1.0;   // 9: Front top-left (normal:front)
    mCubeTemplate.vertices << -1.0f <<  1.0f <<  1.0f << 0.0 << 1.0 << 0.0;   // 10: Front top-left (normal:top)
    mCubeTemplate.vertices << -1.0f <<  1.0f <<  1.0f << -1.0 << 0.0 << 0.0;  // 11: Front top-left (normal:left)
    // Back bottom-left
    mCubeTemplate.vertices << -1.0f << -1.0f << -1.0f << 0.0 << 0.0 << -1.0;  // 12: Back bottom-left (normal:back)
    mCubeTemplate.vertices << -1.0f << -1.0f << -1.0f << 0.0 << -1.0 << 0.0;  // 13: Back bottom-left (normal:bottom)
    mCubeTemplate.vertices << -1.0f << -1.0f << -1.0f << -1.0 << 0.0 << 0.0;  // 14: Back bottom-left (normal:left)
    // Back bottom-right
    mCubeTemplate.vertices <<  1.0f << -1.0f << -1.0f << 0.0 << 0.0 << -1.0;  // 15: Back bottom-right (normal:back)
    mCubeTemplate.vertices <<  1.0f << -1.0f << -1.0f << 0.0 << -1.0 << 0.0;  // 16: Back bottom-right (normal:bottom)
    mCubeTemplate.vertices <<  1.0f << -1.0f << -1.0f << 1.0 << 0.0 << 0.0;   // 17: Back bottom-right (normal:right)
    // Back top-right
    mCubeTemplate.vertices <<  1.0f <<  1.0f << -1.0f << 0.0 << 0.0 << -1.0;  // 18: Back top-right (normal:back)
    mCubeTemplate.vertices <<  1.0f <<  1.0f << -1.0f << 0.0 << 1.0 << 0.0;   // 19: Back top-right (normal:top)
    mCubeTemplate.vertices <<  1.0f <<  1.0f << -1.0f << 1.0 << 0.0 << 0.0;   // 20: Back top-right (normal:right)
    // Back top-left
    mCubeTemplate.vertices << -1.0f <<  1.0f << -1.0f << 0.0 << 0.0 << -1.0;  // 21: Back top-left (normal:back)
    mCubeTemplate.vertices << -1.0f <<  1.0f << -1.0f << 0.0 << 1.0 << 0.0;   // 22: Back top-left (normal:top)
    mCubeTemplate.vertices << -1.0f <<  1.0f << -1.0f << -1.0 << 0.0 << 0.0;  // 23: Back top-left (normal:left)

    // Define indices for the 12 triangles (6 faces × 2 triangles each)
    // Counter-clockwise winding order for front-facing triangles
    QVector<unsigned int> indices = {
        // Front face
        0, 3, 6,  0, 6, 9,
        // Back face
        12, 15, 18,  12, 18, 21,
        // Left face
        2, 11, 23,  2, 23, 14,
        // Right face
        5, 8, 20,  5, 20, 17,
        // Bottom face
        1, 4, 16,  1, 13, 16,
        // Top face
        7, 10, 22,  7, 22, 19,
    };

    mCubeTemplate.indices = indices;

    // Colors will be set per instance, so we don't populate them in the template
}

GeometryData GeometryManager::transformCubeTemplate(QMatrix4x4 transform, float r, float g, float b, float a)
{
    GeometryData result;

    // Transform vertices and copy normals
    for (int i = 0; i < mCubeTemplate.vertices.size(); i += 6) {
        // Scale and translate vertex
        QVector3D vertex = QVector3D(mCubeTemplate.vertices[i], mCubeTemplate.vertices[i+1], mCubeTemplate.vertices[i+2]);
        vertex = transform.map(vertex);
        result.vertices << vertex.x();
        result.vertices << vertex.y();
        result.vertices << vertex.z();

        // Copy normal (no transformation needed since it's a uniform scale)
        vertex = QVector3D(mCubeTemplate.vertices[i+3], mCubeTemplate.vertices[i+4], mCubeTemplate.vertices[i+5]);
        vertex = transform.map(vertex) - transform.map(QVector3D());
        vertex.normalize();
        result.vertices << vertex.x();
        result.vertices << vertex.y();
        result.vertices << vertex.z();

        // Set color for this vertex
        result.colors << r << g << b << a;
    }

    // Copy indices (they don't need transformation)
    result.indices = mCubeTemplate.indices;

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
    return transformCubeTemplate(transform, r, g, b, a);
}

GeometryData GeometryManager::generateLineGeometry(const QVector<float>& vertices, const QVector<float>& colors)
{
    if (vertices.isEmpty() || colors.isEmpty()) {
        return GeometryData();
    }

    GeometryData result;
    result.vertices = vertices;
    result.colors = colors;

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
    //result.vertices = vertices;
    result.colors = colors;

    // add normals pointing up
    for (int i=0; i < vertices.size(); i+=3) {
        result.vertices << vertices[i] << vertices[i+1] << vertices[i+2] << 0.0f << 0.0f << 1.0f;
    }

    return result;
}

void GeometryManager::renderGeometry(const GeometryData& geometry,
                                   QOpenGLVertexArrayObject& vao,
                                   QOpenGLBuffer& vertexBuffer,
                                   QOpenGLBuffer& colorBuffer,
                                   QOpenGLBuffer& normalBuffer,
                                   QOpenGLBuffer& indexBuffer,
                                   GLenum drawMode)
{
    if (geometry.isEmpty()) {
        return;
    }

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Upload vertex data
    vertexBuffer.bind();
    vertexBuffer.allocate(geometry.vertices.data(), geometry.vertices.size() * sizeof(float));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Upload color data
    colorBuffer.bind();
    colorBuffer.allocate(geometry.colors.data(), geometry.colors.size() * sizeof(float));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(2);

    // Draw the geometry - use indexed rendering if indices are available
    if (geometry.hasIndices()) {
        // Upload index data
        indexBuffer.bind();
        indexBuffer.allocate(geometry.indices.data(), geometry.indices.size() * sizeof(unsigned int));

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

void GeometryManager::renderInstancedCubes(const QVector<CubeInstanceData>& instances,
                                          QOpenGLVertexArrayObject& vao,
                                          QOpenGLBuffer& vertexBuffer,
                                          QOpenGLBuffer& colorBuffer,
                                          QOpenGLBuffer& normalBuffer,
                                          QOpenGLBuffer& indexBuffer,
                                          QOpenGLBuffer& instanceBuffer,
                                          GLenum drawMode)
{
    if (!mInitialized || instances.isEmpty()) {
        return;
    }

    // Check if instancing functions are available
    if (!glVertexAttribDivisor || !glDrawElementsInstanced) {
        qWarning() << "GeometryManager: Instancing functions not available, falling back to individual cubes";
        // Fallback to individual cube rendering
        for (const auto& instance : instances) {
            GeometryData cubeGeometry = transformCubeTemplate(instance.transform, instance.color[0], instance.color[1], instance.color[2], instance.color[3]);
            renderGeometry(cubeGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, drawMode);
        }
        return;
    }

    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);

    // Upload cube template vertex and normal data
    vertexBuffer.bind();
    vertexBuffer.allocate(mCubeTemplate.vertices.data(), mCubeTemplate.vertices.size() * sizeof(float));
    // Pointer to vertices
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // Pointer to normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Upload cube template index data
    indexBuffer.bind();
    indexBuffer.allocate(mCubeTemplate.indices.data(), mCubeTemplate.indices.size() * sizeof(unsigned int));

    // Upload instance data to GPU
    instanceBuffer.bind();
    instanceBuffer.allocate(instances.data(), instances.size() * sizeof(CubeInstanceData));

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
    glDrawElementsInstanced(drawMode, mCubeTemplate.indexCount(), GL_UNSIGNED_INT, nullptr, instances.size());

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

void GeometryManager::renderInstancedCubes(const QVector<CubeInstanceData>& instances,
                                          QOpenGLVertexArrayObject& vao,
                                          QOpenGLBuffer& vertexBuffer,
                                          QOpenGLBuffer& colorBuffer,
                                          QOpenGLBuffer& normalBuffer,
                                          QOpenGLBuffer& indexBuffer,
                                          QOpenGLBuffer& instanceBuffer,
                                          ResourceManager* resourceManager,
                                          GLenum drawMode)
{
    if (instances.isEmpty()) {
        return;
    }

    // Call the original instanced render method
    renderInstancedCubes(instances, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, instanceBuffer, drawMode);

    // Track draw call statistics - one draw call for all instances
    if (resourceManager) {
        resourceManager->onDrawCall(instances.size() * (mCubeTemplate.indexCount() / 3)); // Count triangles for all instances
    }
}
