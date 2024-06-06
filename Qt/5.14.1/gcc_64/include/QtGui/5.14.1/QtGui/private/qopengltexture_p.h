/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QABSTRACTOPENGLTEXTURE_P_H
#define QABSTRACTOPENGLTEXTURE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QT_NO_OPENGL

#include <QtGui/private/qtguiglobal_p.h>
#include "private/qobject_p.h"
#include "qopengltexture.h"
#include "qopengl.h"

#include <cmath>

namespace {
inline double qLog2(const double x)
{
    return std::log(x) / std::log(2.0);
}
}

QT_BEGIN_NAMESPACE

class QOpenGLContext;
class QOpenGLTextureHelper;
class QOpenGLFunctions;

class QOpenGLTexturePrivate
{
public:
    QOpenGLTexturePrivate(QOpenGLTexture::Target textureTarget,
                          QOpenGLTexture *qq);
    ~QOpenGLTexturePrivate();

    Q_DECLARE_PUBLIC(QOpenGLTexture)

    void resetFuncs(QOpenGLTextureHelper *funcs);
    void initializeOpenGLFunctions();

    bool create();
    void destroy();

    void bind();
    void bind(uint unit, QOpenGLTexture::TextureUnitReset reset = QOpenGLTexture::DontResetTextureUnit);
    void release();
    void release(uint unit, QOpenGLTexture::TextureUnitReset reset = QOpenGLTexture::DontResetTextureUnit);
    bool isBound() const;
    bool isBound(uint unit) const;

    void allocateStorage(QOpenGLTexture::PixelFormat pixelFormat, QOpenGLTexture::PixelType pixelType);
    void allocateMutableStorage(QOpenGLTexture::PixelFormat pixelFormat, QOpenGLTexture::PixelType pixelType);
    void allocateImmutableStorage();
    void setData(int mipLevel, int layer, int layerCount, QOpenGLTexture::CubeMapFace cubeFace,
                 QOpenGLTexture::PixelFormat sourceFormat, QOpenGLTexture::PixelType sourceType,
                 const void *data, const QOpenGLPixelTransferOptions * const options);
    void setData(int xOffset, int yOffset, int zOffset, int width, int height, int depth,
                 int mipLevel, int layer, int layerCount, QOpenGLTexture::CubeMapFace cubeFace,
                 QOpenGLTexture::PixelFormat sourceFormat, QOpenGLTexture::PixelType sourceType,
                 const void *data, const QOpenGLPixelTransferOptions * const options);
    void setCompressedData(int mipLevel, int layer, int layerCount, QOpenGLTexture::CubeMapFace cubeFace,
                           int dataSize, const void *data,
                           const QOpenGLPixelTransferOptions * const options);


    void setWrapMode(QOpenGLTexture::WrapMode mode);
    void setWrapMode(QOpenGLTexture::CoordinateDirection direction, QOpenGLTexture::WrapMode mode);
    QOpenGLTexture::WrapMode wrapMode(QOpenGLTexture::CoordinateDirection direction) const;

    QOpenGLTexture *createTextureView(QOpenGLTexture::Target target, QOpenGLTexture::TextureFormat viewFormat,
                                      int minimumMipmapLevel, int maximumMipmapLevel,
                                      int minimumLayer, int maximumLayer) const;

    int evaluateMipLevels() const;

    inline int maximumMipLevelCount() const
    {
        return 1 + std::floor(qLog2(qMax(dimensions[0], qMax(dimensions[1], dimensions[2]))));
    }

    static inline int mipLevelSize(int mipLevel, int baseLevelSize)
    {
        return std::floor(double(qMax(1, baseLevelSize >> mipLevel)));
    }

    bool isUsingImmutableStorage() const;

    QOpenGLTexture *q_ptr;
    QOpenGLContext *context;
    QOpenGLTexture::Target target;
    QOpenGLTexture::BindingTarget bindingTarget;
    GLuint textureId;
    QOpenGLTexture::TextureFormat format;
    QOpenGLTexture::TextureFormatClass formatClass;
    int dimensions[3];
    int requestedMipLevels;
    int mipLevels;
    int layers;
    int faces;

    int samples;
    bool fixedSamplePositions;

    int baseLevel;
    int maxLevel;

    QOpenGLTexture::SwizzleValue swizzleMask[4];
    QOpenGLTexture::DepthStencilMode depthStencilMode;
    QOpenGLTexture::ComparisonFunction comparisonFunction;
    QOpenGLTexture::ComparisonMode comparisonMode;

    QOpenGLTexture::Filter minFilter;
    QOpenGLTexture::Filter magFilter;
    float maxAnisotropy;
    QOpenGLTexture::WrapMode wrapModes[3];
    QVariantList borderColor;
    float minLevelOfDetail;
    float maxLevelOfDetail;
    float levelOfDetailBias;

    bool textureView;
    bool autoGenerateMipMaps;
    bool storageAllocated;

    QOpenGLTextureHelper *texFuncs;
    QOpenGLFunctions *functions;

    QOpenGLTexture::Features features;
};

QT_END_NAMESPACE

#undef Q_CALL_MEMBER_FUNCTION

#endif // QT_NO_OPENGL

#endif // QABSTRACTOPENGLTEXTURE_P_H
