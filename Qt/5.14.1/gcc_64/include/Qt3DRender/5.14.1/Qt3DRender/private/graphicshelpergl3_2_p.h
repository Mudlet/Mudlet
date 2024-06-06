/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT3DRENDER_RENDER_GRAPHICSHELPERGL3_H
#define QT3DRENDER_RENDER_GRAPHICSHELPERGL3_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <Qt3DRender/private/graphicshelperinterface_p.h>
#include <QtCore/qscopedpointer.h>

#ifndef QT_OPENGL_ES_2

QT_BEGIN_NAMESPACE

class QOpenGLFunctions_3_2_Core;
class QOpenGLExtension_ARB_tessellation_shader;

namespace Qt3DRender {
namespace Render {

class Q_AUTOTEST_EXPORT GraphicsHelperGL3_2 : public GraphicsHelperInterface
{
public:
    GraphicsHelperGL3_2();
    ~GraphicsHelperGL3_2();

    // QGraphicHelperInterface interface
    void alphaTest(GLenum mode1, GLenum mode2) override;
    void bindBufferBase(GLenum target, GLuint index, GLuint buffer) override;
    void bindFragDataLocation(GLuint shader, const QHash<QString, int> &outputs) override;
    bool frameBufferNeedsRenderBuffer(const Attachment &attachment) override;
    void bindFrameBufferAttachment(QOpenGLTexture *texture, const Attachment &attachment) override;
    void bindFrameBufferAttachment(RenderBuffer *renderBuffer, const Attachment &attachment) override;
    void bindFrameBufferObject(GLuint frameBufferId, FBOBindMode mode) override;
    void bindImageTexture(GLuint imageUnit, GLuint texture, GLint mipLevel, GLboolean layered, GLint layer, GLenum access, GLenum format) override;
    void bindShaderStorageBlock(GLuint programId, GLuint shaderStorageBlockIndex, GLuint shaderStorageBlockBinding) override;
    void bindUniformBlock(GLuint programId, GLuint uniformBlockIndex, GLuint uniformBlockBinding) override;
    void blendEquation(GLenum mode) override;
    void blendFunci(GLuint buf, GLenum sfactor, GLenum dfactor) override;
    void blendFuncSeparatei(GLuint buf, GLenum sRGB, GLenum dRGB, GLenum sAlpha, GLenum dAlpha) override;
    void blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) override;
    GLuint boundFrameBufferObject() override;
    void buildUniformBuffer(const QVariant &v, const ShaderUniform &description, QByteArray &buffer) override;
    bool checkFrameBufferComplete() override;
    void clearBufferf(GLint drawbuffer, const QVector4D &values) override;
    GLuint createFrameBufferObject() override;
    void depthMask(GLenum mode) override;
    void depthRange(GLdouble nearValue, GLdouble farValue) override;
    void depthTest(GLenum mode) override;
    void disableClipPlane(int clipPlane) override;
    void disablei(GLenum cap, GLuint index) override;
    void disablePrimitiveRestart() override;
    void dispatchCompute(GLuint wx, GLuint wy, GLuint wz) override;
    char *mapBuffer(GLenum target, GLsizeiptr size) override;
    GLboolean unmapBuffer(GLenum target) override;
    void drawArrays(GLenum primitiveType, GLint first, GLsizei count) override;
    void drawArraysIndirect(GLenum mode,void *indirect) override;
    void drawArraysInstanced(GLenum primitiveType, GLint first, GLsizei count, GLsizei instances) override;
    void drawArraysInstancedBaseInstance(GLenum primitiveType, GLint first, GLsizei count, GLsizei instances, GLsizei baseInstance) override;
    void drawBuffers(GLsizei n, const int *bufs) override;
    void drawElements(GLenum primitiveType, GLsizei primitiveCount, GLint indexType, void *indices, GLint baseVertex = 0) override;
    void drawElementsIndirect(GLenum mode, GLenum type, void *indirect) override;
    void drawElementsInstancedBaseVertexBaseInstance(GLenum primitiveType, GLsizei primitiveCount, GLint indexType, void *indices, GLsizei instances, GLint baseVertex = 0,  GLint baseInstance = 0) override;
    void enableClipPlane(int clipPlane) override;
    void enablei(GLenum cap, GLuint index) override;
    void enablePrimitiveRestart(int primitiveRestartIndex) override;
    void enableVertexAttributeArray(int location) override;
    void frontFace(GLenum mode) override;
    QSize getRenderBufferDimensions(GLuint renderBufferId) override;
    QSize getTextureDimensions(GLuint textureId, GLenum target, uint level = 0) override;
    void initializeHelper(QOpenGLContext *context, QAbstractOpenGLFunctions *functions) override;
    void pointSize(bool programmable, GLfloat value) override;
    GLint maxClipPlaneCount() override;
    void memoryBarrier(QMemoryBarrier::Operations barriers) override;
    QVector<ShaderUniformBlock> programUniformBlocks(GLuint programId) override;
    QVector<ShaderAttribute> programAttributesAndLocations(GLuint programId) override;
    QVector<ShaderUniform> programUniformsAndLocations(GLuint programId) override;
    QVector<ShaderStorageBlock> programShaderStorageBlocks(GLuint programId) override;
    void releaseFrameBufferObject(GLuint frameBufferId) override;
    void setMSAAEnabled(bool enable) override;
    void setAlphaCoverageEnabled(bool enable) override;
    void setClipPlane(int clipPlane, const QVector3D &normal, float distance) override;
    void setSeamlessCubemap(bool enable) override;
    void setVerticesPerPatch(GLint verticesPerPatch) override;
    bool supportsFeature(Feature feature) const override;
    uint uniformByteSize(const ShaderUniform &description) override;
    void useProgram(GLuint programId) override;
    void vertexAttribDivisor(GLuint index, GLuint divisor) override;
    void vertexAttributePointer(GLenum shaderDataType, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer) override;
    void readBuffer(GLenum mode) override;
    void drawBuffer(GLenum mode) override;
    void rasterMode(GLenum faceMode, GLenum rasterMode) override;

    void *fenceSync() override;
    void clientWaitSync(void *sync, GLuint64 nanoSecTimeout) override;
    void waitSync(void *sync) override;
    bool wasSyncSignaled(void *sync) override;
    void deleteSync(void *sync) override;

    void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) override;
    void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) override;
    void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) override;
    void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) override;

    void glUniform1iv(GLint location, GLsizei count, const GLint *value) override;
    void glUniform2iv(GLint location, GLsizei count, const GLint *value) override;
    void glUniform3iv(GLint location, GLsizei count, const GLint *value) override;
    void glUniform4iv(GLint location, GLsizei count, const GLint *value) override;

    void glUniform1uiv(GLint location, GLsizei count, const GLuint *value) override;
    void glUniform2uiv(GLint location, GLsizei count, const GLuint *value) override;
    void glUniform3uiv(GLint location, GLsizei count, const GLuint *value) override;
    void glUniform4uiv(GLint location, GLsizei count, const GLuint *value) override;

    void glUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *value) override;
    void glUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *value) override;
    void glUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *value) override;
    void glUniformMatrix2x3fv(GLint location, GLsizei count, const GLfloat *value) override;
    void glUniformMatrix3x2fv(GLint location, GLsizei count, const GLfloat *value) override;
    void glUniformMatrix2x4fv(GLint location, GLsizei count, const GLfloat *value) override;
    void glUniformMatrix4x2fv(GLint location, GLsizei count, const GLfloat *value) override;
    void glUniformMatrix3x4fv(GLint location, GLsizei count, const GLfloat *value) override;
    void glUniformMatrix4x3fv(GLint location, GLsizei count, const GLfloat *value) override;

    UniformType uniformTypeFromGLType(GLenum glType) override;

private:
    QOpenGLFunctions_3_2_Core *m_funcs;
    QScopedPointer<QOpenGLExtension_ARB_tessellation_shader> m_tessFuncs;
};

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // !QT_OPENGL_ES_2

#endif // QT3DRENDER_RENDER_GRAPHICSHELPERGL3_H
