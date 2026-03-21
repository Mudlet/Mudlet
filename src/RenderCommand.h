#ifndef MUDLET_RENDER_COMMAND_H
#define MUDLET_RENDER_COMMAND_H

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

#include <QMatrix4x4>
#include <QMatrix3x3>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>

#include "GeometryManager.h"

// Forward declarations
class GeometryManager;
class ResourceManager;

// Base class for all render commands
class RenderCommand
{
public:
    virtual ~RenderCommand() = default;
    virtual void execute(QOpenGLFunctions* gl,
                        QOpenGLShaderProgram* shader,
                        GeometryManager* geometryManager,
                        ResourceManager* resourceManager,
                        QOpenGLVertexArrayObject& vao,
                        QOpenGLBuffer& vertexBuffer,
                        QOpenGLBuffer& colorBuffer,
                        QOpenGLBuffer& normalBuffer,
                        QOpenGLBuffer& indexBuffer,
                        QOpenGLBuffer& texCoordBuffer) = 0;

    virtual const char* getCommandName() const = 0;
};

// Command to render cube geometry
class RenderCubeCommand : public RenderCommand
{
public:
    RenderCubeCommand(float x, float y, float z, float size, float r, float g, float b, float a,
                     const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix);

    void execute(QOpenGLFunctions* gl,
                QOpenGLShaderProgram* shader,
                GeometryManager* geometryManager,
                ResourceManager* resourceManager,
                QOpenGLVertexArrayObject& vao,
                QOpenGLBuffer& vertexBuffer,
                QOpenGLBuffer& colorBuffer,
                QOpenGLBuffer& normalBuffer,
                QOpenGLBuffer& indexBuffer,
                QOpenGLBuffer& texCoordBuffer) override;

    const char* getCommandName() const override { return "RenderCube"; }

private:
    float mX, mY, mZ, mSize;
    float mR, mG, mB, mA;
    QMatrix4x4 mProjectionMatrix;
    QMatrix4x4 mViewMatrix;
    QMatrix4x4 mModelMatrix;
};

// Command to render line geometry
class RenderLinesCommand : public RenderCommand
{
public:
    RenderLinesCommand(const QVector<float>& vertices, const QVector<float>& colors,
                      const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix);

    void execute(QOpenGLFunctions* gl,
                QOpenGLShaderProgram* shader,
                GeometryManager* geometryManager,
                ResourceManager* resourceManager,
                QOpenGLVertexArrayObject& vao,
                QOpenGLBuffer& vertexBuffer,
                QOpenGLBuffer& colorBuffer,
                QOpenGLBuffer& normalBuffer,
                QOpenGLBuffer& indexBuffer,
                QOpenGLBuffer& texCoordBuffer) override;

    const char* getCommandName() const override { return "RenderLines"; }

private:
    QVector<float> mVertices;
    QVector<float> mColors;
    QMatrix4x4 mProjectionMatrix;
    QMatrix4x4 mViewMatrix;
    QMatrix4x4 mModelMatrix;
};

// Command to render triangle geometry
class RenderTrianglesCommand : public RenderCommand
{
public:
    RenderTrianglesCommand(const QVector<float>& vertices, const QVector<float>& colors,
                          const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix);

    void execute(QOpenGLFunctions* gl,
                QOpenGLShaderProgram* shader,
                GeometryManager* geometryManager,
                ResourceManager* resourceManager,
                QOpenGLVertexArrayObject& vao,
                QOpenGLBuffer& vertexBuffer,
                QOpenGLBuffer& colorBuffer,
                QOpenGLBuffer& normalBuffer,
                QOpenGLBuffer& indexBuffer,
                QOpenGLBuffer& texCoordBuffer) override;

    const char* getCommandName() const override { return "RenderTriangles"; }

private:
    QVector<float> mVertices;
    QVector<float> mColors;
    QMatrix4x4 mProjectionMatrix;
    QMatrix4x4 mViewMatrix;
    QMatrix4x4 mModelMatrix;
};

class RenderTexturedTrianglesCommand : public RenderCommand
{
public:
    RenderTexturedTrianglesCommand(const GeometryData& geometry,
                                  const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix);
    
    void execute(QOpenGLFunctions* gl,
                QOpenGLShaderProgram* shader,
                GeometryManager* geometryManager,
                ResourceManager* resourceManager,
                QOpenGLVertexArrayObject& vao,
                QOpenGLBuffer& vertexBuffer,
                QOpenGLBuffer& colorBuffer,
                QOpenGLBuffer& normalBuffer,
                QOpenGLBuffer& indexBuffer,
                QOpenGLBuffer& texCoordBuffer) override;
                
    const char* getCommandName() const override { return "RenderTexturedTriangles"; }

private:
    GeometryData mGeometry;
    QMatrix4x4 mProjectionMatrix;
    QMatrix4x4 mViewMatrix;
    QMatrix4x4 mModelMatrix;
};

// Command to render multiple cube instances in a single draw call
class RenderInstancedCubesCommand : public RenderCommand
{
public:
    RenderInstancedCubesCommand(const QVector<CubeInstanceData>& instances,
                               const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix);

    void execute(QOpenGLFunctions* gl,
                QOpenGLShaderProgram* shader,
                GeometryManager* geometryManager,
                ResourceManager* resourceManager,
                QOpenGLVertexArrayObject& vao,
                QOpenGLBuffer& vertexBuffer,
                QOpenGLBuffer& colorBuffer,
                QOpenGLBuffer& normalBuffer,
                QOpenGLBuffer& indexBuffer,
                QOpenGLBuffer& texCoordBuffer) override;

    const char* getCommandName() const override { return "RenderInstancedCubes"; }

private:
    QVector<CubeInstanceData> mInstances;
    QMatrix4x4 mProjectionMatrix;
    QMatrix4x4 mViewMatrix;
    QMatrix4x4 mModelMatrix;
};

// Command to change OpenGL state
class GLStateCommand : public RenderCommand
{
public:
    enum StateType {
        ENABLE_DEPTH_TEST,
        DISABLE_DEPTH_TEST,
        ENABLE_DEPTH_WRITE,
        DISABLE_DEPTH_WRITE,
        ENABLE_BLEND,
        DISABLE_BLEND,
        CLEAR_BUFFERS
    };

    explicit GLStateCommand(StateType stateType);

    void execute(QOpenGLFunctions* gl,
                QOpenGLShaderProgram* shader,
                GeometryManager* geometryManager,
                ResourceManager* resourceManager,
                QOpenGLVertexArrayObject& vao,
                QOpenGLBuffer& vertexBuffer,
                QOpenGLBuffer& colorBuffer,
                QOpenGLBuffer& normalBuffer,
                QOpenGLBuffer& indexBuffer,
                QOpenGLBuffer& texCoordBuffer) override;

    const char* getCommandName() const override { return "GLState"; }

private:
    StateType mStateType;
};

// Command to render a map label as a textured billboard
class RenderLabelCommand : public RenderCommand
{
public:
    RenderLabelCommand(float centerX, float centerY, float centerZ,
                      float width, float height,
                      GLuint textureId,
                      const QVector3D& cameraRight,
                      const QVector3D& cameraUp,
                      bool highlight,
                      const QMatrix4x4& projectionMatrix,
                      const QMatrix4x4& viewMatrix,
                      const QMatrix4x4& modelMatrix);

    void execute(QOpenGLFunctions* gl,
                QOpenGLShaderProgram* shader,
                GeometryManager* geometryManager,
                ResourceManager* resourceManager,
                QOpenGLVertexArrayObject& vao,
                QOpenGLBuffer& vertexBuffer,
                QOpenGLBuffer& colorBuffer,
                QOpenGLBuffer& normalBuffer,
                QOpenGLBuffer& indexBuffer,
                QOpenGLBuffer& texCoordBuffer) override;

    const char* getCommandName() const override { return "RenderLabel"; }

private:
    float mCenterX, mCenterY, mCenterZ;
    float mWidth, mHeight;
    GLuint mTextureId;
    QVector3D mCameraRight;
    QVector3D mCameraUp;
    bool mHighlight;
    QMatrix4x4 mProjectionMatrix;
    QMatrix4x4 mViewMatrix;
    QMatrix4x4 mModelMatrix;
};

#endif // MUDLET_RENDER_COMMAND_H
