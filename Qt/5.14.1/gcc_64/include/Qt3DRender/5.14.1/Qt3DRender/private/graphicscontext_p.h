/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#ifndef QT3DRENDER_RENDER_GRAPHICSCONTEXT_H
#define QT3DRENDER_RENDER_GRAPHICSCONTEXT_H

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

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QHash>
#include <QColor>
#include <QMatrix4x4>
#include <QBitArray>
#include <QImage>
#include <Qt3DRender/private/shaderparameterpack_p.h>
#include <Qt3DRender/qclearbuffers.h>
#include <Qt3DRender/private/shader_p.h>
#include <Qt3DRender/private/glbuffer_p.h>
#include <Qt3DRender/qattribute.h>
#include <Qt3DRender/qmemorybarrier.h>
#include <Qt3DRender/private/handle_types_p.h>
#include <Qt3DRender/private/qgraphicsapifilter_p.h>
#include <Qt3DRender/private/shadercache_p.h>
#include <Qt3DRender/private/uniform_p.h>
#include <Qt3DRender/private/graphicshelperinterface_p.h>
#include <Qt3DRender/private/qblitframebuffer_p.h>
#include <qmath.h>

QT_BEGIN_NAMESPACE

class QOpenGLShaderProgram;
class QAbstractOpenGLFunctions;
class QOpenGLDebugLogger;

namespace Qt3DRender {

namespace Render {

class GraphicsHelperInterface;
class RenderTarget;
class AttachmentPack;
class ShaderManager;

typedef QPair<QString, int> NamedUniformLocation;

class Q_AUTOTEST_EXPORT GraphicsContext
{
public:
    GraphicsContext();
    ~GraphicsContext();

    void setShaderCache(ShaderCache *shaderCache) { m_shaderCache = shaderCache; }
    ShaderCache *shaderCache() const { return m_shaderCache; }

    void setOpenGLContext(QOpenGLContext* ctx);
    QOpenGLContext *openGLContext() { return m_gl; }
    bool makeCurrent(QSurface *surface);
    void doneCurrent();
    bool hasValidGLHelper() const;
    bool isInitialized() const;

    // Shaders
    QOpenGLShaderProgram *createShaderProgram(Shader *shaderNode);
    void introspectShaderInterface(Shader *shader, QOpenGLShaderProgram *shaderProgram);
    void loadShader(Shader* shader, ShaderManager *manager);
    void removeShaderProgramReference(Shader *shaderNode);

    GLuint defaultFBO() const { return m_defaultFBO; }

    const GraphicsApiFilterData *contextInfo() const;

    // Wrapper methods
    void    clearBackBuffer(QClearBuffers::BufferTypeFlags buffers);
    void    alphaTest(GLenum mode1, GLenum mode2);
    void    bindFramebuffer(GLuint fbo, GraphicsHelperInterface::FBOBindMode mode);
    void    bindBufferBase(GLenum target, GLuint bindingIndex, GLuint buffer);
    void    bindFragOutputs(GLuint shader, const QHash<QString, int> &outputs);
    void    bindImageTexture(GLuint imageUnit, GLuint texture, GLint mipLevel, GLboolean layered, GLint layer, GLenum access, GLenum format);
    void    bindUniformBlock(GLuint programId, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
    void    bindShaderStorageBlock(GLuint programId, GLuint shaderStorageBlockIndex, GLuint shaderStorageBlockBinding);
    void    blendEquation(GLenum mode);
    void    blendFunci(GLuint buf, GLenum sfactor, GLenum dfactor);
    void    blendFuncSeparatei(GLuint buf, GLenum sRGB, GLenum dRGB, GLenum sAlpha, GLenum dAlpha);
    GLuint  boundFrameBufferObject();
    void    buildUniformBuffer(const QVariant &v, const ShaderUniform &description, QByteArray &buffer);
    void    clearBufferf(GLint drawbuffer, const QVector4D &values);
    void    clearColor(const QColor &color);
    void    clearDepthValue(float depth);
    void    clearStencilValue(int stencil);
    void    depthRange(GLdouble nearValue, GLdouble farValue);
    void    depthMask(GLenum mode);
    void    depthTest(GLenum mode);
    void    disableClipPlane(int clipPlane);
    void    disablei(GLenum cap, GLuint index);
    void    disablePrimitiveRestart();
    void    dispatchCompute(int x, int y, int z);
    char *  mapBuffer(GLenum target, GLsizeiptr size);
    GLboolean unmapBuffer(GLenum target);
    void    drawArrays(GLenum primitiveType, GLint first, GLsizei count);
    void    drawArraysIndirect(GLenum mode,void *indirect);
    void    drawArraysInstanced(GLenum primitiveType, GLint first, GLsizei count, GLsizei instances);
    void    drawArraysInstancedBaseInstance(GLenum primitiveType, GLint first, GLsizei count, GLsizei instances, GLsizei baseinstance);
    void    drawElements(GLenum primitiveType, GLsizei primitiveCount, GLint indexType, void * indices, GLint baseVertex);
    void    drawElementsIndirect(GLenum mode, GLenum type, void *indirect);
    void    drawElementsInstancedBaseVertexBaseInstance(GLenum primitiveType, GLsizei primitiveCount, GLint indexType, void * indices, GLsizei instances, GLint baseVertex, GLint baseInstance);
    void    enableClipPlane(int clipPlane);
    void    enablei(GLenum cap, GLuint index);
    void    enablePrimitiveRestart(int restartIndex);
    void    frontFace(GLenum mode);
    GLint   maxClipPlaneCount();
    GLint   maxTextureUnitsCount() const;
    GLint   maxImageUnitsCount() const;
    void    pointSize(bool programmable, GLfloat value);
    void    readBuffer(GLenum mode);
    void    drawBuffer(GLenum mode);
    void    drawBuffers(GLsizei n, const int *bufs);
    void    setMSAAEnabled(bool enabled);
    void    setAlphaCoverageEnabled(bool enabled);
    void    setClipPlane(int clipPlane, const QVector3D &normal, float distance);
    void    setSeamlessCubemap(bool enable);
    void    setVerticesPerPatch(GLint verticesPerPatch);
    void    memoryBarrier(QMemoryBarrier::Operations barriers);
    void    activateDrawBuffers(const AttachmentPack &attachments);
    void    rasterMode(GLenum faceMode, GLenum rasterMode);

    // Helper methods
    static GLint elementType(GLint type);
    static GLint tupleSizeFromType(GLint type);
    static GLuint byteSizeFromType(GLint type);
    static GLint glDataTypeFromAttributeDataType(QAttribute::VertexBaseType dataType);

    bool supportsDrawBuffersBlend() const;
    bool supportsVAO() const { return m_supportsVAO; }

    void initialize();
    void initializeHelpers(QSurface *surface);
    GraphicsHelperInterface *resolveHighestOpenGLFunctions();

    bool m_initialized;
    bool m_supportsVAO;
    GLint m_maxTextureUnits;
    GLint m_maxImageUnits;
    GLuint m_defaultFBO;
    QOpenGLContext *m_gl;
    GraphicsHelperInterface *m_glHelper;

    QHash<QSurface *, GraphicsHelperInterface*> m_glHelpers;
    GraphicsApiFilterData m_contextInfo;
    ShaderCache *m_shaderCache;
    QScopedPointer<QOpenGLDebugLogger> m_debugLogger;

    friend class OpenGLVertexArrayObject;
    OpenGLVertexArrayObject *m_currentVAO;

    void applyUniform(const ShaderUniform &description, const UniformValue &v);

    template<UniformType>
    void applyUniformHelper(const ShaderUniform &, const UniformValue &) const
    {
        Q_ASSERT_X(false, Q_FUNC_INFO, "Uniform: Didn't provide specialized apply() implementation");
    }
};

#define QT3D_UNIFORM_TYPE_PROTO(UniformTypeEnum, BaseType, Func) \
template<> \
void GraphicsContext::applyUniformHelper<UniformTypeEnum>(const ShaderUniform &description, const UniformValue &value) const;

#define QT3D_UNIFORM_TYPE_IMPL(UniformTypeEnum, BaseType, Func) \
    template<> \
    void GraphicsContext::applyUniformHelper<UniformTypeEnum>(const ShaderUniform &description, const UniformValue &value) const \
{ \
    const int count = qMin(description.m_size, int(value.byteSize() / description.m_rawByteSize)); \
    m_glHelper->Func(description.m_location, count, value.constData<BaseType>()); \
}


QT3D_UNIFORM_TYPE_PROTO(UniformType::Float, float, glUniform1fv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::Vec2, float, glUniform2fv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::Vec3, float, glUniform3fv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::Vec4, float, glUniform4fv)

// OpenGL expects int* as values for booleans
QT3D_UNIFORM_TYPE_PROTO(UniformType::Bool, int, glUniform1iv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::BVec2, int, glUniform2iv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::BVec3, int, glUniform3iv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::BVec4, int, glUniform4iv)

QT3D_UNIFORM_TYPE_PROTO(UniformType::Int, int, glUniform1iv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::IVec2, int, glUniform2iv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::IVec3, int, glUniform3iv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::IVec4, int, glUniform4iv)

QT3D_UNIFORM_TYPE_PROTO(UniformType::UInt, uint, glUniform1uiv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::UIVec2, uint, glUniform2uiv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::UIVec3, uint, glUniform3uiv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::UIVec4, uint, glUniform4uiv)

QT3D_UNIFORM_TYPE_PROTO(UniformType::Mat2, float, glUniformMatrix2fv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::Mat3, float, glUniformMatrix3fv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::Mat4, float, glUniformMatrix4fv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::Mat2x3, float, glUniformMatrix2x3fv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::Mat3x2, float, glUniformMatrix3x2fv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::Mat2x4, float, glUniformMatrix2x4fv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::Mat4x2, float, glUniformMatrix4x2fv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::Mat3x4, float, glUniformMatrix3x4fv)
QT3D_UNIFORM_TYPE_PROTO(UniformType::Mat4x3, float, glUniformMatrix4x3fv)

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // QT3DRENDER_RENDER_GRAPHICSCONTEXT_H
