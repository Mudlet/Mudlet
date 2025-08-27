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

#include "pre_guard.h"
#include <QDebug>
#include "post_guard.h"

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
    // Generate unit cube centered at origin with the same vertex order and normals as original
    mCubeTemplate.clear();
    
    // Bottom face (glNormal3f(0.57735, -0.57735, 0.57735), etc.)
    mCubeTemplate.vertices << 1.0f << -1.0f << 1.0f;
    mCubeTemplate.normals << 0.57735f << -0.57735f << 0.57735f;
    mCubeTemplate.vertices << -1.0f << -1.0f << 1.0f;
    mCubeTemplate.normals << -0.57735f << -0.57735f << 0.57735f;
    mCubeTemplate.vertices << -1.0f << -1.0f << -1.0f;
    mCubeTemplate.normals << -0.57735f << -0.57735f << -0.57735f;
    mCubeTemplate.vertices << 1.0f << -1.0f << 1.0f;
    mCubeTemplate.normals << 0.57735f << -0.57735f << 0.57735f;
    mCubeTemplate.vertices << -1.0f << -1.0f << -1.0f;
    mCubeTemplate.normals << -0.57735f << -0.57735f << -0.57735f;
    mCubeTemplate.vertices << 1.0f << -1.0f << -1.0f;
    mCubeTemplate.normals << 0.57735f << -0.57735f << -0.57735f;

    // Front face
    mCubeTemplate.vertices << 1.0f << 1.0f << 1.0f;
    mCubeTemplate.normals << 0.57735f << 0.57735f << 0.57735f;
    mCubeTemplate.vertices << -1.0f << 1.0f << 1.0f;
    mCubeTemplate.normals << -0.57735f << 0.57735f << 0.57735f;
    mCubeTemplate.vertices << -1.0f << -1.0f << 1.0f;
    mCubeTemplate.normals << -0.57735f << -0.57735f << 0.57735f;
    mCubeTemplate.vertices << 1.0f << 1.0f << 1.0f;
    mCubeTemplate.normals << 0.57735f << 0.57735f << 0.57735f;
    mCubeTemplate.vertices << -1.0f << -1.0f << 1.0f;
    mCubeTemplate.normals << -0.57735f << -0.57735f << 0.57735f;
    mCubeTemplate.vertices << 1.0f << -1.0f << 1.0f;
    mCubeTemplate.normals << 0.57735f << -0.57735f << 0.57735f;

    // Back face
    mCubeTemplate.vertices << -1.0f << 1.0f << -1.0f;
    mCubeTemplate.normals << -0.57735f << 0.57735f << -0.57735f;
    mCubeTemplate.vertices << 1.0f << 1.0f << -1.0f;
    mCubeTemplate.normals << 0.57735f << 0.57735f << -0.57735f;
    mCubeTemplate.vertices << 1.0f << -1.0f << -1.0f;
    mCubeTemplate.normals << 0.57735f << -0.57735f << -0.57735f;
    mCubeTemplate.vertices << -1.0f << 1.0f << -1.0f;
    mCubeTemplate.normals << -0.57735f << 0.57735f << -0.57735f;
    mCubeTemplate.vertices << 1.0f << -1.0f << -1.0f;
    mCubeTemplate.normals << 0.57735f << -0.57735f << -0.57735f;
    mCubeTemplate.vertices << -1.0f << -1.0f << -1.0f;
    mCubeTemplate.normals << -0.57735f << -0.57735f << -0.57735f;

    // Right face
    mCubeTemplate.vertices << 1.0f << 1.0f << -1.0f;
    mCubeTemplate.normals << 0.57735f << 0.57735f << -0.57735f;
    mCubeTemplate.vertices << 1.0f << 1.0f << 1.0f;
    mCubeTemplate.normals << 0.57735f << 0.57735f << 0.57735f;
    mCubeTemplate.vertices << 1.0f << -1.0f << 1.0f;
    mCubeTemplate.normals << 0.57735f << -0.57735f << 0.57735f;
    mCubeTemplate.vertices << 1.0f << 1.0f << -1.0f;
    mCubeTemplate.normals << 0.57735f << 0.57735f << -0.57735f;
    mCubeTemplate.vertices << 1.0f << -1.0f << 1.0f;
    mCubeTemplate.normals << 0.57735f << -0.57735f << 0.57735f;
    mCubeTemplate.vertices << 1.0f << -1.0f << -1.0f;
    mCubeTemplate.normals << 0.57735f << -0.57735f << -0.57735f;

    // Left face
    mCubeTemplate.vertices << -1.0f << 1.0f << 1.0f;
    mCubeTemplate.normals << -0.57735f << 0.57735f << 0.57735f;
    mCubeTemplate.vertices << -1.0f << 1.0f << -1.0f;
    mCubeTemplate.normals << -0.57735f << 0.57735f << -0.57735f;
    mCubeTemplate.vertices << -1.0f << -1.0f << -1.0f;
    mCubeTemplate.normals << -0.57735f << -0.57735f << -0.57735f;
    mCubeTemplate.vertices << -1.0f << 1.0f << 1.0f;
    mCubeTemplate.normals << -0.57735f << 0.57735f << 0.57735f;
    mCubeTemplate.vertices << -1.0f << -1.0f << -1.0f;
    mCubeTemplate.normals << -0.57735f << -0.57735f << -0.57735f;
    mCubeTemplate.vertices << -1.0f << -1.0f << 1.0f;
    mCubeTemplate.normals << -0.57735f << -0.57735f << 0.57735f;

    // Top face
    mCubeTemplate.vertices << 1.0f << 1.0f << -1.0f;
    mCubeTemplate.normals << 0.57735f << 0.57735f << -0.57735f;
    mCubeTemplate.vertices << -1.0f << 1.0f << -1.0f;
    mCubeTemplate.normals << -0.57735f << 0.57735f << -0.57735f;
    mCubeTemplate.vertices << -1.0f << 1.0f << 1.0f;
    mCubeTemplate.normals << -0.57735f << 0.57735f << 0.57735f;
    mCubeTemplate.vertices << 1.0f << 1.0f << -1.0f;
    mCubeTemplate.normals << 0.57735f << 0.57735f << -0.57735f;
    mCubeTemplate.vertices << -1.0f << 1.0f << 1.0f;
    mCubeTemplate.normals << -0.57735f << 0.57735f << 0.57735f;
    mCubeTemplate.vertices << 1.0f << 1.0f << 1.0f;
    mCubeTemplate.normals << 0.57735f << 0.57735f << 0.57735f;
    
    // Colors will be set per instance, so we don't populate them in the template
}

GeometryData GeometryManager::transformCubeTemplate(float x, float y, float z, float size, float r, float g, float b, float a, float zSquishFactor)
{
    GeometryData result;
    
    // Transform vertices and copy normals
    for (int i = 0; i < mCubeTemplate.vertices.size(); i += 3) {
        // Scale and translate vertex
        result.vertices << (mCubeTemplate.vertices[i] * size + x);
        result.vertices << (mCubeTemplate.vertices[i + 1] * size + y);  
        result.vertices << (mCubeTemplate.vertices[i + 2] * size/zSquishFactor + z);
        
        // Copy normal (no transformation needed since it's a uniform scale)
        result.normals << mCubeTemplate.normals[i];
        result.normals << mCubeTemplate.normals[i + 1];
        result.normals << mCubeTemplate.normals[i + 2];
        
        // Set color for this vertex
        result.colors << r << g << b << a;
    }
    
    return result;
}

GeometryData GeometryManager::generateCubeGeometry(float x, float y, float z, float size, float r, float g, float b, float a, float zSquishFactor)
{
    if (!mInitialized) {
        qWarning() << "GeometryManager: generateCubeGeometry called before initialize()";
        return GeometryData();
    }
    
    return transformCubeTemplate(x, y, z, size, r, g, b, a, zSquishFactor);
}

GeometryData GeometryManager::generateLineGeometry(const QVector<float>& vertices, const QVector<float>& colors)
{
    if (vertices.isEmpty() || colors.isEmpty()) {
        return GeometryData();
    }
    
    GeometryData result;
    result.vertices = vertices;
    result.colors = colors;
    
    // Create dummy normals for lines (pointing up)
    for (int i = 0; i < vertices.size() / 3; ++i) {
        result.normals << 0.0f << 0.0f << 1.0f;
    }
    
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
    
    // Create dummy normals for triangles (pointing up)
    for (int i = 0; i < vertices.size() / 3; ++i) {
        result.normals << 0.0f << 0.0f << 1.0f;
    }
    
    return result;
}

void GeometryManager::renderGeometry(const GeometryData& geometry,
                                   QOpenGLVertexArrayObject& vao,
                                   QOpenGLBuffer& vertexBuffer,
                                   QOpenGLBuffer& colorBuffer,
                                   QOpenGLBuffer& normalBuffer,
                                   GLenum drawMode)
{
    if (geometry.isEmpty()) {
        return;
    }
    
    QOpenGLVertexArrayObject::Binder vaoBinder(&vao);
    
    // Upload vertex data
    vertexBuffer.bind();
    vertexBuffer.allocate(geometry.vertices.data(), geometry.vertices.size() * sizeof(float));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    
    // Upload color data
    colorBuffer.bind();
    colorBuffer.allocate(geometry.colors.data(), geometry.colors.size() * sizeof(float));
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);
    
    // Upload normal data
    normalBuffer.bind();
    normalBuffer.allocate(geometry.normals.data(), geometry.normals.size() * sizeof(float));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(2);
    
    // Draw the geometry
    glDrawArrays(drawMode, 0, geometry.vertexCount());
}

void GeometryManager::renderGeometry(const GeometryData& geometry,
                                   QOpenGLVertexArrayObject& vao,
                                   QOpenGLBuffer& vertexBuffer,
                                   QOpenGLBuffer& colorBuffer,
                                   QOpenGLBuffer& normalBuffer,
                                   ResourceManager* resourceManager,
                                   GLenum drawMode)
{
    if (geometry.isEmpty()) {
        return;
    }
    
    // Call the original render method
    renderGeometry(geometry, vao, vertexBuffer, colorBuffer, normalBuffer, drawMode);
    
    // Track draw call statistics
    if (resourceManager) {
        resourceManager->onDrawCall(geometry.vertexCount());
    }
}
