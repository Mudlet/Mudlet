/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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


#ifndef QGL_ENGINE_SHADER_SOURCE_H
#define QGL_ENGINE_SHADER_SOURCE_H

#include "qglengineshadermanager_p.h"

QT_BEGIN_NAMESPACE



static const char* const qglslMainVertexShader = "\n\
    void setPosition(); \n\
    void main(void) \n\
    { \n\
        setPosition(); \n\
    }\n";

static const char* const qglslMainWithTexCoordsVertexShader = "\n\
    attribute highp   vec2      textureCoordArray; \n\
    varying   highp   vec2      textureCoords; \n\
    void setPosition(); \n\
    void main(void) \n\
    { \n\
        setPosition(); \n\
        textureCoords = textureCoordArray; \n\
    }\n";

static const char* const qglslMainWithTexCoordsAndOpacityVertexShader = "\n\
    attribute highp   vec2      textureCoordArray; \n\
    attribute lowp    float     opacityArray; \n\
    varying   highp   vec2      textureCoords; \n\
    varying   lowp    float     opacity; \n\
    void setPosition(); \n\
    void main(void) \n\
    { \n\
        setPosition(); \n\
        textureCoords = textureCoordArray; \n\
        opacity = opacityArray; \n\
    }\n";

// NOTE: We let GL do the perspective correction so texture lookups in the fragment
//       shader are also perspective corrected.
static const char* const qglslPositionOnlyVertexShader = "\n\
    attribute highp   vec2      vertexCoordsArray; \n\
    attribute highp   vec3      pmvMatrix1; \n\
    attribute highp   vec3      pmvMatrix2; \n\
    attribute highp   vec3      pmvMatrix3; \n\
    void setPosition(void) \n\
    { \n\
        highp mat3 pmvMatrix = mat3(pmvMatrix1, pmvMatrix2, pmvMatrix3); \n\
        vec3 transformedPos = pmvMatrix * vec3(vertexCoordsArray.xy, 1.0); \n\
        gl_Position = vec4(transformedPos.xy, 0.0, transformedPos.z); \n\
    }\n";

static const char* const qglslComplexGeometryPositionOnlyVertexShader = "\n\
    uniform highp mat3 matrix; \n\
    uniform highp float translateZ; \n\
    attribute highp vec2 vertexCoordsArray; \n\
    void setPosition(void) \n\
    { \n\
      vec3 v = matrix * vec3(vertexCoordsArray, 1.0); \n\
      vec4 vz = mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, translateZ, 1) * vec4(v, 1.0); \n\
      gl_Position = vec4(vz.xyz, 1.0);\n\
    } \n";

static const char* const qglslUntransformedPositionVertexShader = "\n\
    attribute highp   vec4      vertexCoordsArray; \n\
    void setPosition(void) \n\
    { \n\
        gl_Position = vertexCoordsArray; \n\
    }\n";

// Pattern Brush - This assumes the texture size is 8x8 and thus, the inverted size is 0.125
static const char* const qglslPositionWithPatternBrushVertexShader = "\n\
    attribute highp   vec2      vertexCoordsArray; \n\
    attribute highp   vec3      pmvMatrix1; \n\
    attribute highp   vec3      pmvMatrix2; \n\
    attribute highp   vec3      pmvMatrix3; \n\
    uniform   mediump vec2      halfViewportSize; \n\
    uniform   highp   vec2      invertedTextureSize; \n\
    uniform   highp   mat3      brushTransform; \n\
    varying   highp   vec2      patternTexCoords; \n\
    void setPosition(void) \n\
    { \n\
        highp mat3 pmvMatrix = mat3(pmvMatrix1, pmvMatrix2, pmvMatrix3); \n\
        vec3 transformedPos = pmvMatrix * vec3(vertexCoordsArray.xy, 1.0); \n\
        gl_Position.xy = transformedPos.xy / transformedPos.z; \n\
        mediump vec2 viewportCoords = (gl_Position.xy + 1.0) * halfViewportSize; \n\
        mediump vec3 hTexCoords = brushTransform * vec3(viewportCoords, 1.0); \n\
        mediump float invertedHTexCoordsZ = 1.0 / hTexCoords.z; \n\
        gl_Position = vec4(gl_Position.xy * invertedHTexCoordsZ, 0.0, invertedHTexCoordsZ); \n\
        patternTexCoords.xy = (hTexCoords.xy * 0.125) * invertedHTexCoordsZ; \n\
    }\n";

static const char* const qglslAffinePositionWithPatternBrushVertexShader
                 = qglslPositionWithPatternBrushVertexShader;

static const char* const qglslPatternBrushSrcFragmentShader = "\n\
    uniform           sampler2D brushTexture; \n\
    uniform   lowp    vec4      patternColor; \n\
    varying   highp   vec2      patternTexCoords;\n\
    lowp vec4 srcPixel() \n\
    { \n\
        return patternColor * (1.0 - texture2D(brushTexture, patternTexCoords).r); \n\
    }\n";


// Linear Gradient Brush
static const char* const qglslPositionWithLinearGradientBrushVertexShader = "\n\
    attribute highp   vec2      vertexCoordsArray; \n\
    attribute highp   vec3      pmvMatrix1; \n\
    attribute highp   vec3      pmvMatrix2; \n\
    attribute highp   vec3      pmvMatrix3; \n\
    uniform   mediump vec2      halfViewportSize; \n\
    uniform   highp   vec3      linearData; \n\
    uniform   highp   mat3      brushTransform; \n\
    varying   mediump float     index; \n\
    void setPosition() \n\
    { \n\
        highp mat3 pmvMatrix = mat3(pmvMatrix1, pmvMatrix2, pmvMatrix3); \n\
        vec3 transformedPos = pmvMatrix * vec3(vertexCoordsArray.xy, 1.0); \n\
        gl_Position.xy = transformedPos.xy / transformedPos.z; \n\
        mediump vec2 viewportCoords = (gl_Position.xy + 1.0) * halfViewportSize; \n\
        mediump vec3 hTexCoords = brushTransform * vec3(viewportCoords, 1); \n\
        mediump float invertedHTexCoordsZ = 1.0 / hTexCoords.z; \n\
        gl_Position = vec4(gl_Position.xy * invertedHTexCoordsZ, 0.0, invertedHTexCoordsZ); \n\
        index = (dot(linearData.xy, hTexCoords.xy) * linearData.z) * invertedHTexCoordsZ; \n\
    }\n";

static const char* const qglslAffinePositionWithLinearGradientBrushVertexShader
                 = qglslPositionWithLinearGradientBrushVertexShader;

static const char* const qglslLinearGradientBrushSrcFragmentShader = "\n\
    uniform           sampler2D brushTexture; \n\
    varying   mediump float     index; \n\
    lowp vec4 srcPixel() \n\
    { \n\
        mediump vec2 val = vec2(index, 0.5); \n\
        return texture2D(brushTexture, val); \n\
    }\n";


// Conical Gradient Brush
static const char* const qglslPositionWithConicalGradientBrushVertexShader = "\n\
    attribute highp   vec2      vertexCoordsArray; \n\
    attribute highp   vec3      pmvMatrix1; \n\
    attribute highp   vec3      pmvMatrix2; \n\
    attribute highp   vec3      pmvMatrix3; \n\
    uniform   mediump vec2      halfViewportSize; \n\
    uniform   highp   mat3      brushTransform; \n\
    varying   highp   vec2      A; \n\
    void setPosition(void) \n\
    { \n\
        highp mat3 pmvMatrix = mat3(pmvMatrix1, pmvMatrix2, pmvMatrix3); \n\
        vec3 transformedPos = pmvMatrix * vec3(vertexCoordsArray.xy, 1.0); \n\
        gl_Position.xy = transformedPos.xy / transformedPos.z; \n\
        mediump vec2  viewportCoords = (gl_Position.xy + 1.0) * halfViewportSize; \n\
        mediump vec3 hTexCoords = brushTransform * vec3(viewportCoords, 1); \n\
        mediump float invertedHTexCoordsZ = 1.0 / hTexCoords.z; \n\
        gl_Position = vec4(gl_Position.xy * invertedHTexCoordsZ, 0.0, invertedHTexCoordsZ); \n\
        A = hTexCoords.xy * invertedHTexCoordsZ; \n\
    }\n";

static const char* const qglslAffinePositionWithConicalGradientBrushVertexShader
                 = qglslPositionWithConicalGradientBrushVertexShader;

static const char* const qglslConicalGradientBrushSrcFragmentShader = "\n\
    #define INVERSE_2PI 0.1591549430918953358 \n\
    uniform           sampler2D brushTexture; \n\
    uniform   mediump float     angle; \n\
    varying   highp   vec2      A; \n\
    lowp vec4 srcPixel() \n\
    { \n\
        highp float t; \n\
        if (abs(A.y) == abs(A.x)) \n\
            t = (atan(-A.y + 0.002, A.x) + angle) * INVERSE_2PI; \n\
        else \n\
            t = (atan(-A.y, A.x) + angle) * INVERSE_2PI; \n\
        return texture2D(brushTexture, vec2(t - floor(t), 0.5)); \n\
    }\n";


// Radial Gradient Brush
static const char* const qglslPositionWithRadialGradientBrushVertexShader = "\n\
    attribute highp   vec2      vertexCoordsArray;\n\
    attribute highp   vec3      pmvMatrix1; \n\
    attribute highp   vec3      pmvMatrix2; \n\
    attribute highp   vec3      pmvMatrix3; \n\
    uniform   mediump vec2      halfViewportSize; \n\
    uniform   highp   mat3      brushTransform; \n\
    uniform   highp   vec2      fmp; \n\
    uniform   mediump vec3      bradius; \n\
    varying   highp   float     b; \n\
    varying   highp   vec2      A; \n\
    void setPosition(void) \n\
    {\n\
        highp mat3 pmvMatrix = mat3(pmvMatrix1, pmvMatrix2, pmvMatrix3); \n\
        vec3 transformedPos = pmvMatrix * vec3(vertexCoordsArray.xy, 1.0); \n\
        gl_Position.xy = transformedPos.xy / transformedPos.z; \n\
        mediump vec2 viewportCoords = (gl_Position.xy + 1.0) * halfViewportSize; \n\
        mediump vec3 hTexCoords = brushTransform * vec3(viewportCoords, 1); \n\
        mediump float invertedHTexCoordsZ = 1.0 / hTexCoords.z; \n\
        gl_Position = vec4(gl_Position.xy * invertedHTexCoordsZ, 0.0, invertedHTexCoordsZ); \n\
        A = hTexCoords.xy * invertedHTexCoordsZ; \n\
        b = bradius.x + 2.0 * dot(A, fmp); \n\
    }\n";

static const char* const qglslAffinePositionWithRadialGradientBrushVertexShader
                 = qglslPositionWithRadialGradientBrushVertexShader;

static const char* const qglslRadialGradientBrushSrcFragmentShader = "\n\
    uniform           sampler2D brushTexture; \n\
    uniform   highp   float     fmp2_m_radius2; \n\
    uniform   highp   float     inverse_2_fmp2_m_radius2; \n\
    uniform   highp   float     sqrfr; \n\
    varying   highp   float     b; \n\
    varying   highp   vec2      A; \n\
    uniform   mediump vec3      bradius; \n\
    lowp vec4 srcPixel() \n\
    { \n\
        highp float c = sqrfr-dot(A, A); \n\
        highp float det = b*b - 4.0*fmp2_m_radius2*c; \n\
        lowp vec4 result = vec4(0.0); \n\
        if (det >= 0.0) { \n\
            highp float detSqrt = sqrt(det); \n\
            highp float w = max((-b - detSqrt) * inverse_2_fmp2_m_radius2, (-b + detSqrt) * inverse_2_fmp2_m_radius2); \n\
            if (bradius.y + w * bradius.z >= 0.0) \n\
                result = texture2D(brushTexture, vec2(w, 0.5)); \n\
        } \n\
        return result; \n\
    }\n";


// Texture Brush
static const char* const qglslPositionWithTextureBrushVertexShader = "\n\
    attribute highp   vec2      vertexCoordsArray; \n\
    attribute highp   vec3      pmvMatrix1; \n\
    attribute highp   vec3      pmvMatrix2; \n\
    attribute highp   vec3      pmvMatrix3; \n\
    uniform   mediump vec2      halfViewportSize; \n\
    uniform   highp   vec2      invertedTextureSize; \n\
    uniform   highp   mat3      brushTransform; \n\
    varying   highp   vec2      brushTextureCoords; \n\
    void setPosition(void) \n\
    { \n\
        highp mat3 pmvMatrix = mat3(pmvMatrix1, pmvMatrix2, pmvMatrix3); \n\
        vec3 transformedPos = pmvMatrix * vec3(vertexCoordsArray.xy, 1.0); \n\
        gl_Position.xy = transformedPos.xy / transformedPos.z; \n\
        mediump vec2 viewportCoords = (gl_Position.xy + 1.0) * halfViewportSize; \n\
        mediump vec3 hTexCoords = brushTransform * vec3(viewportCoords, 1); \n\
        mediump float invertedHTexCoordsZ = 1.0 / hTexCoords.z; \n\
        gl_Position = vec4(gl_Position.xy * invertedHTexCoordsZ, 0.0, invertedHTexCoordsZ); \n\
        brushTextureCoords.xy = (hTexCoords.xy * invertedTextureSize) * gl_Position.w; \n\
    }\n";

static const char* const qglslAffinePositionWithTextureBrushVertexShader
                 = qglslPositionWithTextureBrushVertexShader;

// OpenGL ES does not support GL_REPEAT wrap modes for NPOT textures. So instead,
// we emulate GL_REPEAT by only taking the fractional part of the texture coords.
// TODO: Special case POT textures which don't need this emulation
static const char* const qglslTextureBrushSrcFragmentShader_ES = "\n\
    varying highp   vec2      brushTextureCoords; \n\
    uniform         sampler2D brushTexture; \n\
    lowp vec4 srcPixel() { \n\
        return texture2D(brushTexture, fract(brushTextureCoords)); \n\
    }\n";

static const char* const qglslTextureBrushSrcFragmentShader_desktop = "\n\
    varying   highp   vec2      brushTextureCoords; \n\
    uniform           sampler2D brushTexture; \n\
    lowp vec4 srcPixel() \n\
    { \n\
        return texture2D(brushTexture, brushTextureCoords); \n\
    }\n";

static const char* const qglslTextureBrushSrcWithPatternFragmentShader = "\n\
    varying   highp   vec2      brushTextureCoords; \n\
    uniform   lowp    vec4      patternColor; \n\
    uniform           sampler2D brushTexture; \n\
    lowp vec4 srcPixel() \n\
    { \n\
        return patternColor * (1.0 - texture2D(brushTexture, brushTextureCoords).r); \n\
    }\n";

// Solid Fill Brush
static const char* const qglslSolidBrushSrcFragmentShader = "\n\
    uniform   lowp    vec4      fragmentColor; \n\
    lowp vec4 srcPixel() \n\
    { \n\
        return fragmentColor; \n\
    }\n";

static const char* const qglslImageSrcFragmentShader = "\n\
    varying   highp   vec2      textureCoords; \n\
    uniform           sampler2D imageTexture; \n\
    lowp vec4 srcPixel() \n\
    { \n"
        "return texture2D(imageTexture, textureCoords); \n"
    "}\n";

static const char* const qglslCustomSrcFragmentShader = "\n\
    varying   highp   vec2      textureCoords; \n\
    uniform           sampler2D imageTexture; \n\
    lowp vec4 srcPixel() \n\
    { \n\
        return customShader(imageTexture, textureCoords); \n\
    }\n";

static const char* const qglslImageSrcWithPatternFragmentShader = "\n\
    varying   highp   vec2      textureCoords; \n\
    uniform   lowp    vec4      patternColor; \n\
    uniform           sampler2D imageTexture; \n\
    lowp vec4 srcPixel() \n\
    { \n\
        return patternColor * (1.0 - texture2D(imageTexture, textureCoords).r); \n\
    }\n";

static const char* const qglslNonPremultipliedImageSrcFragmentShader = "\n\
    varying   highp   vec2      textureCoords; \n\
    uniform          sampler2D imageTexture; \n\
    lowp vec4 srcPixel() \n\
    { \n\
        lowp vec4 sample = texture2D(imageTexture, textureCoords); \n\
        sample.rgb = sample.rgb * sample.a; \n\
        return sample; \n\
    }\n";

static const char* const qglslShockingPinkSrcFragmentShader = "\n\
    lowp vec4 srcPixel() \n\
    { \n\
        return vec4(0.98, 0.06, 0.75, 1.0); \n\
    }\n";

static const char* const qglslMainFragmentShader_ImageArrays = "\n\
    varying   lowp    float     opacity; \n\
    lowp vec4 srcPixel(); \n\
    void main() \n\
    { \n\
        gl_FragColor = srcPixel() * opacity; \n\
    }\n";

static const char* const qglslMainFragmentShader_CMO = "\n\
    uniform   lowp    float     globalOpacity; \n\
    lowp vec4 srcPixel(); \n\
    lowp vec4 applyMask(lowp vec4); \n\
    lowp vec4 compose(lowp vec4); \n\
    void main() \n\
    { \n\
        gl_FragColor = applyMask(compose(srcPixel()*globalOpacity))); \n\
    }\n";

static const char* const qglslMainFragmentShader_CM = "\n\
    lowp vec4 srcPixel(); \n\
    lowp vec4 applyMask(lowp vec4); \n\
    lowp vec4 compose(lowp vec4); \n\
    void main() \n\
    { \n\
        gl_FragColor = applyMask(compose(srcPixel())); \n\
    }\n";

static const char* const qglslMainFragmentShader_MO = "\n\
    uniform   lowp    float     globalOpacity; \n\
    lowp vec4 srcPixel(); \n\
    lowp vec4 applyMask(lowp vec4); \n\
    void main() \n\
    { \n\
        gl_FragColor = applyMask(srcPixel()*globalOpacity); \n\
    }\n";

static const char* const qglslMainFragmentShader_M = "\n\
    lowp vec4 srcPixel(); \n\
    lowp vec4 applyMask(lowp vec4); \n\
    void main() \n\
    { \n\
        gl_FragColor = applyMask(srcPixel()); \n\
    }\n";

static const char* const qglslMainFragmentShader_CO = "\n\
    uniform   lowp    float     globalOpacity; \n\
    lowp vec4 srcPixel(); \n\
    lowp vec4 compose(lowp vec4); \n\
    void main() \n\
    { \n\
        gl_FragColor = compose(srcPixel()*globalOpacity); \n\
    }\n";

static const char* const qglslMainFragmentShader_C = "\n\
    lowp vec4 srcPixel(); \n\
    lowp vec4 compose(lowp vec4); \n\
    void main() \n\
    { \n\
        gl_FragColor = compose(srcPixel()); \n\
    }\n";

static const char* const qglslMainFragmentShader_O = "\n\
    uniform   lowp    float     globalOpacity; \n\
    lowp vec4 srcPixel(); \n\
    void main() \n\
    { \n\
        gl_FragColor = srcPixel()*globalOpacity; \n\
    }\n";

static const char* const qglslMainFragmentShader = "\n\
    lowp vec4 srcPixel(); \n\
    void main() \n\
    { \n\
        gl_FragColor = srcPixel(); \n\
    }\n";

static const char* const qglslMaskFragmentShader = "\n\
    varying   highp   vec2      textureCoords;\n\
    uniform           sampler2D maskTexture;\n\
    lowp vec4 applyMask(lowp vec4 src) \n\
    {\n\
        lowp vec4 mask = texture2D(maskTexture, textureCoords); \n\
        return src * mask.a; \n\
    }\n";

// For source over with subpixel antialiasing, the final color is calculated per component as follows
// (.a is alpha component, .c is red, green or blue component):
// alpha = src.a * mask.c * opacity
// dest.c = dest.c * (1 - alpha) + src.c * alpha
//
// In the first pass, calculate: dest.c = dest.c * (1 - alpha) with blend funcs: zero, 1 - source color
// In the second pass, calculate: dest.c = dest.c + src.c * alpha with blend funcs: one, one
//
// If source is a solid color (src is constant), only the first pass is needed, with blend funcs: constant, 1 - source color

// For source composition with subpixel antialiasing, the final color is calculated per component as follows:
// alpha = src.a * mask.c * opacity
// dest.c = dest.c * (1 - mask.c) + src.c * alpha
//

static const char* const qglslRgbMaskFragmentShaderPass1 = "\n\
    varying   highp   vec2      textureCoords;\n\
    uniform           sampler2D maskTexture;\n\
    lowp vec4 applyMask(lowp vec4 src) \n\
    { \n\
        lowp vec4 mask = texture2D(maskTexture, textureCoords); \n\
        return src.a * mask; \n\
    }\n";

static const char* const qglslRgbMaskFragmentShaderPass2 = "\n\
    varying   highp   vec2      textureCoords;\n\
    uniform           sampler2D maskTexture;\n\
    lowp vec4 applyMask(lowp vec4 src) \n\
    { \n\
        lowp vec4 mask = texture2D(maskTexture, textureCoords); \n\
        return src * mask; \n\
    }\n";

/*
    Left to implement:
        RgbMaskFragmentShader,
        RgbMaskWithGammaFragmentShader,

        MultiplyCompositionModeFragmentShader,
        ScreenCompositionModeFragmentShader,
        OverlayCompositionModeFragmentShader,
        DarkenCompositionModeFragmentShader,
        LightenCompositionModeFragmentShader,
        ColorDodgeCompositionModeFragmentShader,
        ColorBurnCompositionModeFragmentShader,
        HardLightCompositionModeFragmentShader,
        SoftLightCompositionModeFragmentShader,
        DifferenceCompositionModeFragmentShader,
        ExclusionCompositionModeFragmentShader,
*/

QT_END_NAMESPACE

#endif // GLGC_SHADER_SOURCE_H
