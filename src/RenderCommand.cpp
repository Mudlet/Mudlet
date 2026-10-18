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
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>

namespace {
// The shader's uInk for a shading, 0 for none of the ink ones
int inkMode(const RenderCommand::Shading shading)
{
    switch (shading) {
    case RenderCommand::Shading::InkBlot:
        return 1;
    case RenderCommand::Shading::InkStroke:
        return 2;
    case RenderCommand::Shading::Seal:
        return 3;
    case RenderCommand::Shading::Paper:
        return 4;
    case RenderCommand::Shading::InkLozenge:
        return 5;
    case RenderCommand::Shading::InkHatched:
        return 6;
    case RenderCommand::Shading::InkDashed:
        return 7;
    case RenderCommand::Shading::Lit:
    case RenderCommand::Shading::Unlit:
    case RenderCommand::Shading::Wireframe:
    case RenderCommand::Shading::ModelBlock:
    case RenderCommand::Shading::ModelPath:
    case RenderCommand::Shading::ModelShadow:
    case RenderCommand::Shading::ModelWater:
    case RenderCommand::Shading::Sky:
        return 0;
    }
    return 0;
}

// The shader's uModelEdges for a template
int modelEdges(const GeometryManager::InstanceShape shape)
{
    switch (shape) {
    case GeometryManager::InstanceShape::Box:
        return 1;
    case GeometryManager::InstanceShape::Cylinder:
        return 2;
    case GeometryManager::InstanceShape::Pyramid:
        return 3;
    case GeometryManager::InstanceShape::Cone:
        return 4;
    case GeometryManager::InstanceShape::Sphere:
        return 0;
    }
    return 0;
}

// The shader's uModelShading for a shading, 0 for none of the model ones
int modelMode(const RenderCommand::Shading shading)
{
    switch (shading) {
    case RenderCommand::Shading::ModelBlock:
        return 1;
    case RenderCommand::Shading::ModelPath:
        return 2;
    case RenderCommand::Shading::ModelShadow:
        return 3;
    case RenderCommand::Shading::Sky:
        return 4;
    case RenderCommand::Shading::ModelWater:
        return 5;
    case RenderCommand::Shading::Lit:
    case RenderCommand::Shading::Unlit:
    case RenderCommand::Shading::Wireframe:
    case RenderCommand::Shading::InkBlot:
    case RenderCommand::Shading::InkStroke:
    case RenderCommand::Shading::Seal:
    case RenderCommand::Shading::Paper:
    case RenderCommand::Shading::InkLozenge:
    case RenderCommand::Shading::InkHatched:
    case RenderCommand::Shading::InkDashed:
        return 0;
    }
    return 0;
}

// How much of a wireframe's color fills its faces, between the edges
constexpr float scmWireFill = 0.06f;
} // namespace

RenderCubeCommand::RenderCubeCommand(
        float x, float y, float z, float size, float r, float g, float b, float a, const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix)
: mX(x)
, mY(y)
, mZ(z)
, mSize(size)
, mR(r)
, mG(g)
, mB(b)
, mA(a)
, mProjectionMatrix(projectionMatrix)
, mViewMatrix(viewMatrix)
, mModelMatrix(modelMatrix)
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

RenderLinesCommand::RenderLinesCommand(const QVector<float>& vertices, const QVector<float>& colors, const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix)
: mVertices(vertices)
, mColors(colors)
, mProjectionMatrix(projectionMatrix)
, mViewMatrix(viewMatrix)
, mModelMatrix(modelMatrix)
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

RenderTrianglesCommand::RenderTrianglesCommand(
        const QVector<float>& vertices, const QVector<float>& colors, const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix, const Shading shading)
: mVertices(vertices)
, mColors(colors)
, mProjectionMatrix(projectionMatrix)
, mViewMatrix(viewMatrix)
, mModelMatrix(modelMatrix)
, mShading(shading)
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

    const bool unlit = mShading == Shading::Unlit || mShading == Shading::Paper || mShading == Shading::Sky;
    if (unlit) {
        shader->setUniformValue("uDisableLighting", true);
    }
    const int ink = inkMode(mShading);
    if (ink) {
        shader->setUniformValue("uInk", ink);
    }
    const int model = modelMode(mShading);
    if (model) {
        shader->setUniformValue("uModelShading", model);
    }

    geometryManager->renderGeometry(triangleGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, resourceManager, GL_TRIANGLES);

    if (unlit) {
        shader->setUniformValue("uDisableLighting", false);
    }
    if (ink) {
        shader->setUniformValue("uInk", 0);
    }
    if (model) {
        shader->setUniformValue("uModelShading", 0);
    }
}

RenderTexturedTrianglesCommand::RenderTexturedTrianglesCommand(
        const GeometryData& geometry, const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix, const QMatrix4x4& modelMatrix, const Shading shading)
: mGeometry(geometry)
, mProjectionMatrix(projectionMatrix)
, mViewMatrix(viewMatrix)
, mModelMatrix(modelMatrix)
, mShading(shading)
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
        shader->setUniformValue("uTexture", 0); // Use texture unit 0
    }

    // Normal matrix (inverse transpose of model matrix)
    QMatrix3x3 normalMatrix = mModelMatrix.normalMatrix();
    shader->setUniformValue("uNormalMatrix", normalMatrix);

    const bool unlit = mShading == Shading::Unlit;
    if (unlit) {
        shader->setUniformValue("uDisableLighting", true);
    }

    // Use the textured rendering method
    geometryManager->renderGeometry(mGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, texCoordBuffer, resourceManager, GL_TRIANGLES);

    if (unlit) {
        shader->setUniformValue("uDisableLighting", false);
    }
}

RenderInstancedCubesCommand::RenderInstancedCubesCommand(const QVector<CubeInstanceData>& instances,
                                                         const QMatrix4x4& projectionMatrix,
                                                         const QMatrix4x4& viewMatrix,
                                                         const QMatrix4x4& modelMatrix,
                                                         const Shading shading,
                                                         const GeometryManager::InstanceShape shape)
: mInstances(instances)
, mProjectionMatrix(projectionMatrix)
, mViewMatrix(viewMatrix)
, mModelMatrix(modelMatrix)
, mShading(shading)
, mShape(shape)
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

    const bool unlit = mShading != Shading::Lit;
    const bool wireframe = mShading == Shading::Wireframe;
    if (unlit) {
        shader->setUniformValue("uDisableLighting", true);
    }
    if (wireframe) {
        shader->setUniformValue("uWireframe", true);
        shader->setUniformValue("uWireRound", mShape == GeometryManager::InstanceShape::Cylinder);
        shader->setUniformValue("uWireSides", static_cast<float>(GeometryManager::scmCylinderSegments));
        shader->setUniformValue("uWireFill", scmWireFill);
    }
    const int ink = inkMode(mShading);
    if (ink) {
        shader->setUniformValue("uInk", ink);
        shader->setUniformValue("uInkRound", mShape == GeometryManager::InstanceShape::Cylinder);
    }
    const int model = modelMode(mShading);
    if (model) {
        shader->setUniformValue("uModelShading", model);
        shader->setUniformValue("uModelRound", mShape == GeometryManager::InstanceShape::Cylinder);
        shader->setUniformValue("uModelBevel", mShape == GeometryManager::InstanceShape::Box || mShape == GeometryManager::InstanceShape::Cylinder);
        shader->setUniformValue("uModelEdges", mEdges ? modelEdges(mShape) : 0);
        shader->setUniformValue("uWireSides", static_cast<float>(GeometryManager::scmCylinderSegments));
    }

    geometryManager->renderInstancedCubes(mInstances, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, resourceManager, GL_TRIANGLES, mShape);

    if (unlit) {
        shader->setUniformValue("uDisableLighting", false);
    }
    if (wireframe) {
        shader->setUniformValue("uWireframe", false);
    }
    if (ink) {
        shader->setUniformValue("uInk", 0);
    }
    if (model) {
        shader->setUniformValue("uModelShading", 0);
        shader->setUniformValue("uModelEdges", 0);
    }
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
    case ENABLE_DEPTH_WRITE:
        gl->glDepthMask(GL_TRUE);
        break;
    case DISABLE_DEPTH_WRITE:
        gl->glDepthMask(GL_FALSE);
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
    case ENABLE_POLYGON_OFFSET:
        gl->glEnable(GL_POLYGON_OFFSET_FILL);
        gl->glPolygonOffset(-1.0f, -4.0f);
        break;
    case DISABLE_POLYGON_OFFSET:
        gl->glDisable(GL_POLYGON_OFFSET_FILL);
        gl->glPolygonOffset(0.0f, 0.0f);
        break;
    case ENABLE_COLOR_WRITE:
        gl->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        break;
    case DISABLE_COLOR_WRITE:
        gl->glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        break;
    case DEPTH_FUNC_LEQUAL:
        gl->glDepthFunc(GL_LEQUAL);
        break;
    case DEPTH_FUNC_LESS:
        gl->glDepthFunc(GL_LESS);
        break;
    case DEPTH_FUNC_GREATER:
        gl->glDepthFunc(GL_GREATER);
        break;
    case ADDITIVE_BLEND:
        gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    case ALPHA_BLEND:
        gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
}

RenderLabelCommand::RenderLabelCommand(float centerX,
                                       float centerY,
                                       float centerZ,
                                       float width,
                                       float height,
                                       GLuint textureId,
                                       const QVector3D& cameraRight,
                                       const QVector3D& cameraUp,
                                       const QVector4D& tint,
                                       const QMatrix4x4& projectionMatrix,
                                       const QMatrix4x4& viewMatrix,
                                       const QMatrix4x4& modelMatrix)
: mCenterX(centerX)
, mCenterY(centerY)
, mCenterZ(centerZ)
, mWidth(width)
, mHeight(height)
, mTextureId(textureId)
, mCameraRight(cameraRight)
, mCameraUp(cameraUp)
, mTint(tint)
, mProjectionMatrix(projectionMatrix)
, mViewMatrix(viewMatrix)
, mModelMatrix(modelMatrix)
{
}

void RenderLabelCommand::execute(QOpenGLFunctions* gl,
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

    if (mTextureId == 0) {
        return;
    }

    // Generate billboard geometry
    GeometryData labelGeometry = geometryManager->generateBillboardGeometry(mCenterX, mCenterY, mCenterZ, mWidth, mHeight, mCameraRight, mCameraUp, mTextureId);

    if (labelGeometry.isEmpty()) {
        return;
    }

    for (int i = 0; i < labelGeometry.colors.size(); i += 4) {
        for (int channel = 0; channel < 4; ++channel) {
            labelGeometry.colors[i + channel] *= mTint[channel];
        }
    }

    // Set uniforms
    QMatrix4x4 mvp = mProjectionMatrix * mViewMatrix * mModelMatrix;
    shader->setUniformValue("uMVP", mvp);
    shader->setUniformValue("uModel", mModelMatrix);
    shader->setUniformValue("uUseInstancing", false);
    shader->setUniformValue("uUsePBR", false);
    shader->setUniformValue("uUseTexture", true);
    shader->setUniformValue("uDisableLighting", true);
    shader->setUniformValue("uTexture", 0);

    // Normal matrix (inverse transpose of model matrix)
    QMatrix3x3 normalMatrix = mModelMatrix.normalMatrix();
    shader->setUniformValue("uNormalMatrix", normalMatrix);

    // Use textured rendering method
    geometryManager->renderGeometry(labelGeometry, vao, vertexBuffer, colorBuffer, normalBuffer, indexBuffer, texCoordBuffer, resourceManager, GL_TRIANGLES);

    // Reset lighting for subsequent draw calls
    shader->setUniformValue("uDisableLighting", false);
}

DepthSnapshot::DepthSnapshot() = default;

DepthSnapshot::~DepthSnapshot() = default;

void DepthSnapshot::save()
{
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context) {
        return;
    }
    QOpenGLExtraFunctions* gl = context->extraFunctions();
    GLint frame = 0;
    gl->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &frame);
    GLint viewport[4] = {0, 0, 0, 0};
    gl->glGetIntegerv(GL_VIEWPORT, viewport);
    GLint samples = 0;
    gl->glGetIntegerv(GL_SAMPLES, &samples);
    const QSize size(viewport[2], viewport[3]);
    if (size.isEmpty()) {
        return;
    }
    // A blit between multisampled buffers wants the same samples and depth format at both ends
    if (!mFrame || mFrame->size() != size || mFrame->format().samples() != samples) {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(samples);
        mFrame = std::make_unique<QOpenGLFramebufferObject>(size, format);
    }
    gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(frame));
    gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFrame->handle());
    gl->glBlitFramebuffer(0, 0, size.width(), size.height(), 0, 0, size.width(), size.height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(frame));
    mSaved = true;
}

void DepthSnapshot::restore()
{
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!mSaved || !mFrame || !context) {
        return;
    }
    mSaved = false;
    QOpenGLExtraFunctions* gl = context->extraFunctions();
    GLint frame = 0;
    gl->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &frame);
    const QSize size = mFrame->size();
    gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, mFrame->handle());
    gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(frame));
    gl->glBlitFramebuffer(0, 0, size.width(), size.height(), 0, 0, size.width(), size.height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(frame));
}

void DepthSnapshot::cleanup()
{
    mFrame.reset();
    mSaved = false;
}

DepthSnapshotCommand::DepthSnapshotCommand(DepthSnapshot* snapshot, const bool save)
: mSnapshot(snapshot)
, mSave(save)
{
}

void DepthSnapshotCommand::execute(QOpenGLFunctions* gl,
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
    Q_UNUSED(shader)
    Q_UNUSED(geometryManager)
    Q_UNUSED(resourceManager)
    Q_UNUSED(vao)
    Q_UNUSED(vertexBuffer)
    Q_UNUSED(colorBuffer)
    Q_UNUSED(normalBuffer)
    Q_UNUSED(indexBuffer)
    Q_UNUSED(texCoordBuffer)

    if (!mSnapshot) {
        return;
    }
    if (mSave) {
        mSnapshot->save();
    } else {
        mSnapshot->restore();
    }
}
