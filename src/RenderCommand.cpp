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

#include "RenderCommand.h"

#include <QDebug>

RenderCubeCommand::RenderCubeCommand(float x, float y, float z, float size, float r, float g, float b, float a,
                                   const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix)
    : mX(x), mY(y), mZ(z), mSize(size), mR(r), mG(g), mB(b), mA(a)
    , mProjectionMatrix(projectionMatrix), mViewMatrix(viewMatrix), mModelMatrix(modelMatrix)
{
}

void RenderCubeCommand::execute(QOpenGLFunctions* gl,
                               QOpenGLShaderProgram* shader,
                               GeometryManager* geometryManager,
                               ResourceManager* resourceManager,
                               QOpenGLVertexArrayObject& vao,
                               QOpenGLBuffer& vertexBuffer,
                               QOpenGLBuffer& colorBuffer,
                               QOpenGLBuffer& normalBuffer,
                               QOpenGLBuffer& indexBuffer,
                               QOpenGLBuffer& texCoordBuffer)
{
    Q_UNUSED(gl)
    Q_UNUSED(texCoordBuffer)

    GeometryData cubeGeometry = geometryManager->generateCubeGeometry(mX, mY, mZ, mSize, mR, mG, mB, mA);

    // Set uniforms
    QMatrix4x4 mvp = mProjectionMatrix * mViewMatrix * mModelMatrix;
    shader->setUniformValue("uMVP", mvp);
    shader->setUniformValue("uModel", mModelMatrix);
    shader->setUniformValue("uUseInstancing", false);
    shader->setUniformValue("uUseTexture", false);
    shader->setUniformValue("uUsePBR", false);

    // Normal matrix (inverse transpose of model matrix)
    QMatrix3x3 normalMatrix = mModelMatrix.normalMatrix();
    shader->setUniformValue("uNormalMatrix", normalMatrix);

    geometryManager->renderGeometry(cubeGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, resourceManager, GL_TRIANGLES);
}

RenderLinesCommand::RenderLinesCommand(const QVector<float>& vertices, const QVector<float>& colors,
                                     const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix)
    : mVertices(vertices), mColors(colors)
    , mProjectionMatrix(projectionMatrix), mViewMatrix(viewMatrix), mModelMatrix(modelMatrix)
{
}

void RenderLinesCommand::execute(QOpenGLFunctions* gl,
                                QOpenGLShaderProgram* shader,
                                GeometryManager* geometryManager,
                                ResourceManager* resourceManager,
                                QOpenGLVertexArrayObject& vao,
                                QOpenGLBuffer& vertexBuffer,
                                QOpenGLBuffer& colorBuffer,
                                QOpenGLBuffer& normalBuffer,
                                QOpenGLBuffer& indexBuffer,
                                QOpenGLBuffer& texCoordBuffer)
{
    Q_UNUSED(gl)
    Q_UNUSED(texCoordBuffer)

    GeometryData lineGeometry = geometryManager->generateLineGeometry(mVertices, mColors);

    if (lineGeometry.isEmpty()) {
        return;
    }

    // Set uniforms
    QMatrix4x4 mvp = mProjectionMatrix * mViewMatrix * mModelMatrix;
    shader->setUniformValue("uMVP", mvp);
    shader->setUniformValue("uModel", mModelMatrix);
    shader->setUniformValue("uUseInstancing", false);
    shader->setUniformValue("uUseTexture", false);
    shader->setUniformValue("uUsePBR", false);

    QMatrix3x3 normalMatrix = mModelMatrix.normalMatrix();
    shader->setUniformValue("uNormalMatrix", normalMatrix);

    geometryManager->renderGeometry(lineGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, resourceManager, GL_LINES);
}

RenderTrianglesCommand::RenderTrianglesCommand(const QVector<float>& vertices, const QVector<float>& colors,
                                             const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix)
    : mVertices(vertices), mColors(colors)
    , mProjectionMatrix(projectionMatrix), mViewMatrix(viewMatrix), mModelMatrix(modelMatrix)
{
}

void RenderTrianglesCommand::execute(QOpenGLFunctions* gl,
                                    QOpenGLShaderProgram* shader,
                                    GeometryManager* geometryManager,
                                    ResourceManager* resourceManager,
                                    QOpenGLVertexArrayObject& vao,
                                    QOpenGLBuffer& vertexBuffer,
                                    QOpenGLBuffer& colorBuffer,
                                    QOpenGLBuffer& normalBuffer,
                                    QOpenGLBuffer& indexBuffer,
                                    QOpenGLBuffer& texCoordBuffer)
{
    Q_UNUSED(gl)
    Q_UNUSED(texCoordBuffer)

    GeometryData triangleGeometry = geometryManager->generateTriangleGeometry(mVertices, mColors);

    if (triangleGeometry.isEmpty()) {
        return;
    }

    // Set uniforms
    QMatrix4x4 mvp = mProjectionMatrix * mViewMatrix * mModelMatrix;
    shader->setUniformValue("uMVP", mvp);
    shader->setUniformValue("uModel", mModelMatrix);
    shader->setUniformValue("uUseInstancing", false);
    shader->setUniformValue("uUseTexture", false);
    shader->setUniformValue("uUsePBR", false);

    QMatrix3x3 normalMatrix = mModelMatrix.normalMatrix();
    shader->setUniformValue("uNormalMatrix", normalMatrix);

    geometryManager->renderGeometry(triangleGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, resourceManager, GL_TRIANGLES);
}

RenderTexturedTrianglesCommand::RenderTexturedTrianglesCommand(const GeometryData& geometry,
                                                              const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix)
    : mGeometry(geometry), mProjectionMatrix(projectionMatrix), mViewMatrix(viewMatrix), mModelMatrix(modelMatrix)
{
}

void RenderTexturedTrianglesCommand::execute(QOpenGLFunctions* gl,
                                           QOpenGLShaderProgram* shader,
                                           GeometryManager* geometryManager,
                                           ResourceManager* resourceManager,
                                           QOpenGLVertexArrayObject& vao,
                                           QOpenGLBuffer& vertexBuffer,
                                           QOpenGLBuffer& colorBuffer,
                                           QOpenGLBuffer& normalBuffer,
                                           QOpenGLBuffer& indexBuffer,
                                           QOpenGLBuffer& texCoordBuffer)
{
    Q_UNUSED(gl)

    if (mGeometry.isEmpty()) {
        return;
    }
    
    // Set uniforms
    QMatrix4x4 mvp = mProjectionMatrix * mViewMatrix * mModelMatrix;
    shader->setUniformValue("uMVP", mvp);
    shader->setUniformValue("uModel", mModelMatrix);
    shader->setUniformValue("uUseInstancing", false);
    
    // Set texture uniforms based on available textures
    if (mGeometry.hasPBRTextures()) {
        shader->setUniformValue("uUsePBR", true);
        shader->setUniformValue("uUseTexture", false);
        shader->setUniformValue("uBaseColorTexture", 0);         // Texture unit 0
        shader->setUniformValue("uMetallicRoughnessTexture", 1); // Texture unit 1  
        shader->setUniformValue("uNormalTexture", 2);            // Texture unit 2
        
        // Set PBR material factors
        shader->setUniformValue("uBaseColorFactor", QVector4D(mGeometry.baseColorFactor[0], mGeometry.baseColorFactor[1], mGeometry.baseColorFactor[2], mGeometry.baseColorFactor[3]));
        shader->setUniformValue("uMetallicFactor", mGeometry.metallicFactor);
        shader->setUniformValue("uRoughnessFactor", mGeometry.roughnessFactor);
    } else {
        shader->setUniformValue("uUsePBR", false);
        shader->setUniformValue("uUseTexture", mGeometry.hasTexture());
        shader->setUniformValue("uTexture", 0);          // Use texture unit 0
    }

    // Normal matrix (inverse transpose of model matrix)
    QMatrix3x3 normalMatrix = mModelMatrix.normalMatrix();
    shader->setUniformValue("uNormalMatrix", normalMatrix);
    
    // Use the textured rendering method
    geometryManager->renderGeometry(mGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, texCoordBuffer, resourceManager, GL_TRIANGLES);
}

RenderInstancedCubesCommand::RenderInstancedCubesCommand(const QVector<CubeInstanceData>& instances,
                                                       const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix)
    : mInstances(instances), mProjectionMatrix(projectionMatrix), mViewMatrix(viewMatrix), mModelMatrix(modelMatrix)
{
}

void RenderInstancedCubesCommand::execute(QOpenGLFunctions* gl,
                                         QOpenGLShaderProgram* shader,
                                         GeometryManager* geometryManager,
                                         ResourceManager* resourceManager,
                                         QOpenGLVertexArrayObject& vao,
                                         QOpenGLBuffer& vertexBuffer,
                                         QOpenGLBuffer& colorBuffer,
                                         QOpenGLBuffer& normalBuffer,
                                         QOpenGLBuffer& indexBuffer,
                                         QOpenGLBuffer& texCoordBuffer)
{
    Q_UNUSED(gl)
    Q_UNUSED(texCoordBuffer)

    if (mInstances.isEmpty()) {
        return;
    }

    // Set uniforms
    QMatrix4x4 mvp = mProjectionMatrix * mViewMatrix * mModelMatrix;
    shader->setUniformValue("uMVP", mvp);
    shader->setUniformValue("uModel", mModelMatrix);
    shader->setUniformValue("uUseInstancing", true);
    shader->setUniformValue("uUseTexture", false);
    shader->setUniformValue("uUsePBR", false);

    QMatrix3x3 normalMatrix = mModelMatrix.normalMatrix();
    shader->setUniformValue("uNormalMatrix", normalMatrix);

    // For now, we need to create a temporary instance buffer
    // This will be improved when we add the instance buffer to ModernGLWidget
    QOpenGLBuffer instanceBuffer(QOpenGLBuffer::VertexBuffer);
    instanceBuffer.create();

    geometryManager->renderInstancedCubes(mInstances, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, instanceBuffer, resourceManager, GL_TRIANGLES);

    instanceBuffer.destroy();
}

GLStateCommand::GLStateCommand(StateType stateType)
    : mStateType(stateType)
{
}

void GLStateCommand::execute(QOpenGLFunctions* gl,
                            QOpenGLShaderProgram* shader,
                            GeometryManager* geometryManager,
                            ResourceManager* resourceManager,
                            QOpenGLVertexArrayObject& vao,
                            QOpenGLBuffer& vertexBuffer,
                            QOpenGLBuffer& colorBuffer,
                            QOpenGLBuffer& normalBuffer,
                            QOpenGLBuffer& indexBuffer,
                            QOpenGLBuffer& texCoordBuffer)
{
    Q_UNUSED(shader)
    Q_UNUSED(geometryManager)
    Q_UNUSED(resourceManager)
    Q_UNUSED(vao)
    Q_UNUSED(vertexBuffer)
    Q_UNUSED(colorBuffer)
    Q_UNUSED(normalBuffer)
    Q_UNUSED(indexBuffer)
    Q_UNUSED(texCoordBuffer)

    switch (mStateType) {
        case ENABLE_DEPTH_TEST:
            gl->glEnable(GL_DEPTH_TEST);
            break;
        case DISABLE_DEPTH_TEST:
            gl->glDisable(GL_DEPTH_TEST);
            break;
        case ENABLE_BLEND:
            gl->glEnable(GL_BLEND);
            break;
        case DISABLE_BLEND:
            gl->glDisable(GL_BLEND);
            break;
        case CLEAR_BUFFERS:
            gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            break;
    }
}
