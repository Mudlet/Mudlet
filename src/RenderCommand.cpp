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

#include "pre_guard.h"
#include <QDebug>
#include "post_guard.h"

// RenderCubeCommand implementation
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
                               QOpenGLBuffer& normalBuffer)
{    
    GeometryData cubeGeometry = geometryManager->generateCubeGeometry(mX, mY, mZ, mSize, mR, mG, mB, mA);
    
    // Set uniforms
    QMatrix4x4 mvp = mProjectionMatrix * mViewMatrix * mModelMatrix;
    shader->setUniformValue("uMVP", mvp);
    shader->setUniformValue("uModel", mModelMatrix);

    // Normal matrix (inverse transpose of model matrix)
    QMatrix3x3 normalMatrix = mModelMatrix.normalMatrix();
    shader->setUniformValue("uNormalMatrix", normalMatrix);
    
    geometryManager->renderGeometry(cubeGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, resourceManager, GL_TRIANGLES);
}

// RenderLinesCommand implementation
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
                                QOpenGLBuffer& normalBuffer)
{
    GeometryData lineGeometry = geometryManager->generateLineGeometry(mVertices, mColors);
    
    if (lineGeometry.isEmpty()) {
        return;
    }
    
    // Set uniforms
    QMatrix4x4 mvp = mProjectionMatrix * mViewMatrix * mModelMatrix;
    shader->setUniformValue("uMVP", mvp);
    shader->setUniformValue("uModel", mModelMatrix);

    QMatrix3x3 normalMatrix = mModelMatrix.normalMatrix();
    shader->setUniformValue("uNormalMatrix", normalMatrix);
    
    geometryManager->renderGeometry(lineGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, resourceManager, GL_LINES);
}

// RenderTrianglesCommand implementation
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
                                    QOpenGLBuffer& normalBuffer)
{
    GeometryData triangleGeometry = geometryManager->generateTriangleGeometry(mVertices, mColors);
    
    if (triangleGeometry.isEmpty()) {
        return;
    }
    
    // Set uniforms
    QMatrix4x4 mvp = mProjectionMatrix * mViewMatrix * mModelMatrix;
    shader->setUniformValue("uMVP", mvp);
    shader->setUniformValue("uModel", mModelMatrix);

    QMatrix3x3 normalMatrix = mModelMatrix.normalMatrix();
    shader->setUniformValue("uNormalMatrix", normalMatrix);
    
    geometryManager->renderGeometry(triangleGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, resourceManager, GL_TRIANGLES);
}

// GLStateCommand implementation
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
                            QOpenGLBuffer& normalBuffer)
{
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
