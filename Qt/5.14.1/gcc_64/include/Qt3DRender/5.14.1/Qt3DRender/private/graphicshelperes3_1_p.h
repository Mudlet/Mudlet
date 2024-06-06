/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#ifndef QT3DRENDER_RENDER_GRAPHICSHELPERES3_1_H
#define QT3DRENDER_RENDER_GRAPHICSHELPERES3_1_H

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

#include <Qt3DRender/private/graphicshelperes3_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {
namespace Render {

class GraphicsHelperES3_1 : public GraphicsHelperES3
{
public:
    GraphicsHelperES3_1();
    ~GraphicsHelperES3_1();

    bool supportsFeature(Feature feature) const override;
    void bindImageTexture(GLuint imageUnit, GLuint texture, GLint mipLevel, GLboolean layered, GLint layer, GLenum access, GLenum format) override;
    void dispatchCompute(GLuint wx, GLuint wy, GLuint wz) override;
    void memoryBarrier(QMemoryBarrier::Operations barriers) override;
    void drawArraysIndirect(GLenum mode,void *indirect) override;
    void drawElementsIndirect(GLenum mode, GLenum type, void *indirect) override;
    void bindShaderStorageBlock(GLuint programId, GLuint shaderStorageBlockIndex, GLuint shaderStorageBlockBinding) override;
    QVector<ShaderStorageBlock> programShaderStorageBlocks(GLuint programId) override;

    // QGraphicHelperInterface interface
    UniformType uniformTypeFromGLType(GLenum glType) override;
    uint uniformByteSize(const ShaderUniform &description) override;
    void buildUniformBuffer(const QVariant &v, const ShaderUniform &description, QByteArray &buffer) override;
};

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // QT3DRENDER_RENDER_GRAPHICSHELPERES3_1_H
