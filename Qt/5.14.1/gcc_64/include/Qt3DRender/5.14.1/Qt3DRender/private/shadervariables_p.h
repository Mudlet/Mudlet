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

#ifndef QT3DRENDER_RENDER_SHADERVARIABLES_P_H
#define QT3DRENDER_RENDER_SHADERVARIABLES_P_H

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

#include <Qt3DRender/qt3drender_global.h>
#include <QOpenGLContext>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

namespace Render {

struct ShaderAttribute
{
    ShaderAttribute()
        : m_nameId(-1)
        , m_type(0)
        , m_size(0)
        , m_location(-1)
    {}

    QString m_name;
    int m_nameId;
    GLenum m_type;
    int m_size;
    int m_location;
};
QT3D_DECLARE_TYPEINFO_2(Qt3DRender, Render, ShaderAttribute, Q_MOVABLE_TYPE)

struct ShaderUniform
{
    ShaderUniform()
        : m_nameId(-1)
        , m_type(GL_NONE)
        , m_size(0)
        , m_offset(-1)
        , m_location(-1)
        , m_blockIndex(-1)
        , m_arrayStride(-1)
        , m_matrixStride(-1)
        , m_rawByteSize(0)
    {}

    QString m_name;
    int m_nameId;
    GLenum m_type;
    int m_size;
    int m_offset; // -1 default, >= 0 if uniform defined in uniform block
    int m_location; // -1 if uniform defined in a uniform block
    int m_blockIndex; // -1 is the default, >= 0 if uniform defined in uniform block
    int m_arrayStride; // -1 is the default, >= 0 if uniform defined in uniform block and if it's an array
    int m_matrixStride; // -1 is the default, >= 0 uniform defined in uniform block and is a matrix
    uint m_rawByteSize; // contains byte size (size / type / strides)
    // size, offset and strides are in bytes
};
QT3D_DECLARE_TYPEINFO_2(Qt3DRender, Render, ShaderUniform, Q_MOVABLE_TYPE)

struct ShaderUniformBlock
{
    ShaderUniformBlock()
        : m_nameId(-1)
        , m_index(-1)
        , m_binding(-1)
        , m_activeUniformsCount(0)
        , m_size(0)
    {}

    QString m_name;
    int m_nameId;
    int m_index;
    int m_binding;
    int m_activeUniformsCount;
    int m_size;
};
QT3D_DECLARE_TYPEINFO_2(Qt3DRender, Render, ShaderUniformBlock, Q_MOVABLE_TYPE)

struct ShaderStorageBlock
{
    ShaderStorageBlock()
        : m_nameId(-1)
        , m_index(-1)
        , m_binding(-1)
        , m_size(0)
        , m_activeVariablesCount(0)
    {}

    QString m_name;
    int m_nameId;
    int m_index;
    int m_binding;
    int m_size;
    int m_activeVariablesCount;
};
QT3D_DECLARE_TYPEINFO_2(Qt3DRender, Render, ShaderStorageBlock, Q_MOVABLE_TYPE)

} // namespace Render

} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // QT3DRENDER_RENDER_SHADERVARIABLES_P_H
