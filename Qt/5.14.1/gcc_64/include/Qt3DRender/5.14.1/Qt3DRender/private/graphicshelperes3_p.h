/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Copyright (C) 2016 Svenn-Arne Dragly.
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

#ifndef QT3DRENDER_RENDER_GRAPHICSHELPERES3_H
#define QT3DRENDER_RENDER_GRAPHICSHELPERES3_H

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

#include <Qt3DRender/private/graphicshelperes2_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {
namespace Render {

class GraphicsHelperES3 : public GraphicsHelperES2
{
public:
    GraphicsHelperES3();
    ~GraphicsHelperES3();

    // QGraphicHelperInterface interface
    void bindBufferBase(GLenum target, GLuint index, GLuint buffer) override;
    bool frameBufferNeedsRenderBuffer(const Attachment &attachment) override;
    void bindFrameBufferAttachment(QOpenGLTexture *texture, const Attachment &attachment) override;
    void bindFrameBufferObject(GLuint frameBufferId, FBOBindMode mode) override;
    void bindUniformBlock(GLuint programId, GLuint uniformBlockIndex, GLuint uniformBlockBinding) override;
    void blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) override;
    void buildUniformBuffer(const QVariant &v, const ShaderUniform &description, QByteArray &buffer) override;
    void drawBuffers(GLsizei n, const int *bufs) override;
    void drawArraysInstanced(GLenum primitiveType, GLint first, GLsizei count, GLsizei instances) override;
    void drawElementsInstancedBaseVertexBaseInstance(GLenum primitiveType, GLsizei primitiveCount, GLint indexType, void *indices, GLsizei instances, GLint baseVertex = 0,  GLint baseInstance = 0) override;
    void readBuffer(GLenum mode) override;
    void drawBuffer(GLenum mode) override;
    void initializeHelper(QOpenGLContext *context, QAbstractOpenGLFunctions *functions) override;
    char *mapBuffer(GLenum target, GLsizeiptr size) override;
    QVector<ShaderUniform> programUniformsAndLocations(GLuint programId) override;
    QVector<ShaderUniformBlock> programUniformBlocks(GLuint programId) override;
    bool supportsFeature(Feature feature) const override;
    GLboolean unmapBuffer(GLenum target) override;
    void vertexAttribDivisor(GLuint index, GLuint divisor) override;
    void vertexAttributePointer(GLenum shaderDataType, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer) override;

    UniformType uniformTypeFromGLType(GLenum glType) override;
    uint uniformByteSize(const ShaderUniform &description) override;

    void *fenceSync() override;
    void clientWaitSync(void *sync, GLuint64 nanoSecTimeout) override;
    void waitSync(void *sync) override;
    bool wasSyncSignaled(void *sync) override;
    void deleteSync(void *sync) override;

protected:
    QOpenGLExtraFunctions *m_extraFuncs = nullptr;
};

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // QT3DRENDER_RENDER_GRAPHICSHELPERES3_H
