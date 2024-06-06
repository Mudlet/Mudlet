/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QOPENGLPAINTENGINE_P_H
#define QOPENGLPAINTENGINE_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QDebug>

#include <qopenglpaintdevice.h>

#include <private/qpaintengineex_p.h>
#include <private/qopenglengineshadermanager_p.h>
#include <private/qopengl2pexvertexarray_p.h>
#include <private/qfontengine_p.h>
#include <private/qdatabuffer_p.h>
#include <private/qtriangulatingstroker_p.h>

#include <private/qopenglextensions_p.h>

#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

enum EngineMode {
    ImageDrawingMode,
    TextDrawingMode,
    BrushDrawingMode,
    ImageArrayDrawingMode,
    ImageOpacityArrayDrawingMode
};

QT_BEGIN_NAMESPACE

#define GL_STENCIL_HIGH_BIT         GLuint(0x80)
#define QT_UNKNOWN_TEXTURE_UNIT     GLuint(-1)
#define QT_DEFAULT_TEXTURE_UNIT     GLuint(0)
#define QT_BRUSH_TEXTURE_UNIT       GLuint(0)
#define QT_IMAGE_TEXTURE_UNIT       GLuint(0) //Can be the same as brush texture unit
#define QT_MASK_TEXTURE_UNIT        GLuint(1)
#define QT_BACKGROUND_TEXTURE_UNIT  GLuint(2)

class QOpenGL2PaintEngineExPrivate;

class QOpenGL2PaintEngineState : public QPainterState
{
public:
    QOpenGL2PaintEngineState(QOpenGL2PaintEngineState &other);
    QOpenGL2PaintEngineState();
    ~QOpenGL2PaintEngineState();

    uint isNew : 1;
    uint needsClipBufferClear : 1;
    uint clipTestEnabled : 1;
    uint canRestoreClip : 1;
    uint matrixChanged : 1;
    uint compositionModeChanged : 1;
    uint opacityChanged : 1;
    uint renderHintsChanged : 1;
    uint clipChanged : 1;
    uint currentClip : 8;

    QRect rectangleClip;
};

class Q_GUI_EXPORT QOpenGL2PaintEngineEx : public QPaintEngineEx
{
    Q_DECLARE_PRIVATE(QOpenGL2PaintEngineEx)
public:
    QOpenGL2PaintEngineEx();
    ~QOpenGL2PaintEngineEx();

    bool begin(QPaintDevice *device) override;
    void ensureActive();
    bool end() override;

    virtual void clipEnabledChanged() override;
    virtual void penChanged() override;
    virtual void brushChanged() override;
    virtual void brushOriginChanged() override;
    virtual void opacityChanged() override;
    virtual void compositionModeChanged() override;
    virtual void renderHintsChanged() override;
    virtual void transformChanged() override;

    virtual void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;
    virtual void drawPixmapFragments(const QPainter::PixmapFragment *fragments, int fragmentCount, const QPixmap &pixmap,
                                     QPainter::PixmapFragmentHints hints) override;
    virtual void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                           Qt::ImageConversionFlags flags = Qt::AutoColor) override;
    virtual void drawTextItem(const QPointF &p, const QTextItem &textItem) override;
    virtual void fill(const QVectorPath &path, const QBrush &brush) override;
    virtual void stroke(const QVectorPath &path, const QPen &pen) override;
    virtual void clip(const QVectorPath &path, Qt::ClipOperation op) override;

    virtual void drawStaticTextItem(QStaticTextItem *textItem) override;

    bool drawTexture(const QRectF &r, GLuint textureId, const QSize &size, const QRectF &sr);

    Type type() const override { return OpenGL2; }

    virtual void setState(QPainterState *s) override;
    virtual QPainterState *createState(QPainterState *orig) const override;
    inline QOpenGL2PaintEngineState *state() {
        return static_cast<QOpenGL2PaintEngineState *>(QPaintEngineEx::state());
    }
    inline const QOpenGL2PaintEngineState *state() const {
        return static_cast<const QOpenGL2PaintEngineState *>(QPaintEngineEx::state());
    }

    void beginNativePainting() override;
    void endNativePainting() override;

    void invalidateState();

    void setRenderTextActive(bool);

    bool isNativePaintingActive() const;
    bool requiresPretransformedGlyphPositions(QFontEngine *, const QTransform &) const override { return false; }
    bool shouldDrawCachedGlyphs(QFontEngine *, const QTransform &) const override;

private:
    Q_DISABLE_COPY_MOVE(QOpenGL2PaintEngineEx)

    friend class QOpenGLEngineShaderManager;
};

// This probably needs to grow to GL_MAX_VERTEX_ATTRIBS, but 3 is ok for now as that's
// all the GL2 engine uses:
#define QT_GL_VERTEX_ARRAY_TRACKED_COUNT 3

class QOpenGL2PaintEngineExPrivate : public QPaintEngineExPrivate
{
    Q_DECLARE_PUBLIC(QOpenGL2PaintEngineEx)
public:
    enum StencilFillMode {
        OddEvenFillMode,
        WindingFillMode,
        TriStripStrokeFillMode
    };

    QOpenGL2PaintEngineExPrivate(QOpenGL2PaintEngineEx *q_ptr) :
            q(q_ptr),
            shaderManager(nullptr),
            width(0), height(0),
            ctx(nullptr),
            useSystemClip(true),
            elementIndicesVBOId(0),
            opacityArray(0),
            snapToPixelGrid(false),
            nativePaintingActive(false),
            inverseScale(1),
            lastTextureUnitUsed(QT_UNKNOWN_TEXTURE_UNIT),
            vertexBuffer(QOpenGLBuffer::VertexBuffer),
            texCoordBuffer(QOpenGLBuffer::VertexBuffer),
            opacityBuffer(QOpenGLBuffer::VertexBuffer),
            indexBuffer(QOpenGLBuffer::IndexBuffer)
    { }

    ~QOpenGL2PaintEngineExPrivate();

    void updateBrushTexture();
    void updateBrushUniforms();
    void updateMatrix();
    void updateCompositionMode();

    enum TextureUpdateMode { UpdateIfNeeded, ForceUpdate };
    template<typename T>
    void updateTexture(GLenum textureUnit, const T &texture, GLenum wrapMode, GLenum filterMode, TextureUpdateMode updateMode = UpdateIfNeeded);
    template<typename T>
    GLuint bindTexture(const T &texture);
    void activateTextureUnit(GLenum textureUnit);

    void resetGLState();

    // fill, stroke, drawTexture, drawPixmaps & drawCachedGlyphs are the main rendering entry-points,
    // however writeClip can also be thought of as en entry point as it does similar things.
    void fill(const QVectorPath &path);
    void stroke(const QVectorPath &path, const QPen &pen);
    void drawTexture(const QOpenGLRect& dest, const QOpenGLRect& src, const QSize &textureSize, bool opaque, bool pattern = false);
    void drawPixmapFragments(const QPainter::PixmapFragment *fragments, int fragmentCount, const QPixmap &pixmap,
                             QPainter::PixmapFragmentHints hints);
    void drawCachedGlyphs(QFontEngine::GlyphFormat glyphFormat, QStaticTextItem *staticTextItem);

    // Calls glVertexAttributePointer if the pointer has changed
    inline void uploadData(unsigned int arrayIndex, const GLfloat *data, GLuint count);
    inline bool uploadIndexData(const void *data, GLenum indexValueType, GLuint count);

    // draws whatever is in the vertex array:
    void drawVertexArrays(const float *data, int *stops, int stopCount, GLenum primitive);
    void drawVertexArrays(QOpenGL2PEXVertexArray &vertexArray, GLenum primitive) {
        drawVertexArrays((const float *) vertexArray.data(), vertexArray.stops(), vertexArray.stopCount(), primitive);
    }

    // Composites the bounding rect onto dest buffer:
    void composite(const QOpenGLRect& boundingRect);

    // Calls drawVertexArrays to render into stencil buffer:
    void fillStencilWithVertexArray(const float *data, int count, int *stops, int stopCount, const QOpenGLRect &bounds, StencilFillMode mode);
    void fillStencilWithVertexArray(QOpenGL2PEXVertexArray& vertexArray, bool useWindingFill) {
        fillStencilWithVertexArray((const float *) vertexArray.data(), 0, vertexArray.stops(), vertexArray.stopCount(),
                                   vertexArray.boundingRect(),
                                   useWindingFill ? WindingFillMode : OddEvenFillMode);
    }

    void setBrush(const QBrush& brush);
    void transferMode(EngineMode newMode);
    bool prepareForDraw(bool srcPixelsAreOpaque); // returns true if the program has changed
    bool prepareForCachedGlyphDraw(const QFontEngineGlyphCache &cache);
    inline void useSimpleShader();
    inline GLuint location(const QOpenGLEngineShaderManager::Uniform uniform) {
        return shaderManager->getUniformLocation(uniform);
    }

    void clearClip(uint value);
    void writeClip(const QVectorPath &path, uint value);
    void resetClipIfNeeded();

    void updateClipScissorTest();
    void setScissor(const QRect &rect);
    void regenerateClip();
    void systemStateChanged() override;

    void setVertexAttribArrayEnabled(int arrayIndex, bool enabled = true);
    void syncGlState();

    static QOpenGLEngineShaderManager* shaderManagerForEngine(QOpenGL2PaintEngineEx *engine) { return engine->d_func()->shaderManager; }
    static QOpenGL2PaintEngineExPrivate *getData(QOpenGL2PaintEngineEx *engine) { return engine->d_func(); }
    static void cleanupVectorPath(QPaintEngineEx *engine, void *data);

    QOpenGLExtensions funcs;

    QOpenGL2PaintEngineEx* q;
    QOpenGLEngineShaderManager* shaderManager;
    QOpenGLPaintDevice* device;
    int width, height;
    QOpenGLContext *ctx;
    EngineMode mode;
    QFontEngine::GlyphFormat glyphCacheFormat;

    bool vertexAttributeArraysEnabledState[QT_GL_VERTEX_ARRAY_TRACKED_COUNT];

    // Dirty flags
    bool matrixDirty; // Implies matrix uniforms are also dirty
    bool compositionModeDirty;
    bool brushTextureDirty;
    bool brushUniformsDirty;
    bool opacityUniformDirty;
    bool matrixUniformDirty;

    bool stencilClean; // Has the stencil not been used for clipping so far?
    bool useSystemClip;
    QRegion dirtyStencilRegion;
    QRect currentScissorBounds;
    uint maxClip;

    QBrush currentBrush; // May not be the state's brush!
    const QBrush noBrush;

    QImage currentBrushImage;

    QOpenGL2PEXVertexArray vertexCoordinateArray;
    QOpenGL2PEXVertexArray textureCoordinateArray;
    QVector<GLushort> elementIndices;
    GLuint elementIndicesVBOId;
    QDataBuffer<GLfloat> opacityArray;
    GLfloat staticVertexCoordinateArray[8];
    GLfloat staticTextureCoordinateArray[8];

    bool snapToPixelGrid;
    bool nativePaintingActive;
    GLfloat pmvMatrix[3][3];
    GLfloat inverseScale;

    GLenum lastTextureUnitUsed;
    GLuint lastTextureUsed;

    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer texCoordBuffer;
    QOpenGLBuffer opacityBuffer;
    QOpenGLBuffer indexBuffer;

    bool needsSync;
    bool multisamplingAlwaysEnabled;

    QTriangulatingStroker stroker;
    QDashedStrokeProcessor dasher;

    QVector<GLuint> unusedVBOSToClean;
    QVector<GLuint> unusedIBOSToClean;

    const GLfloat *vertexAttribPointers[3];
};


void QOpenGL2PaintEngineExPrivate::uploadData(unsigned int arrayIndex, const GLfloat *data, GLuint count)
{
    Q_ASSERT(arrayIndex < 3);

    // If a vertex array object is created we have a profile that supports them
    // and we will upload the data via a QOpenGLBuffer. Otherwise we will use
    // the legacy way of uploading the data via glVertexAttribPointer.
    if (vao.isCreated()) {
        if (arrayIndex == QT_VERTEX_COORDS_ATTR) {
            vertexBuffer.bind();
            vertexBuffer.allocate(data, count * sizeof(float));
        }
        if (arrayIndex == QT_TEXTURE_COORDS_ATTR) {
            texCoordBuffer.bind();
            texCoordBuffer.allocate(data, count * sizeof(float));
        }
        if (arrayIndex == QT_OPACITY_ATTR) {
            opacityBuffer.bind();
            opacityBuffer.allocate(data, count * sizeof(float));
        }
        if (arrayIndex == QT_OPACITY_ATTR)
            funcs.glVertexAttribPointer(arrayIndex, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
        else
            funcs.glVertexAttribPointer(arrayIndex, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    } else {
        // If we already uploaded the data we don't have to do it again
        if (data == vertexAttribPointers[arrayIndex])
            return;

        // Store the data in cache and upload it to the graphics card.
        vertexAttribPointers[arrayIndex] = data;
        if (arrayIndex == QT_OPACITY_ATTR)
            funcs.glVertexAttribPointer(arrayIndex, 1, GL_FLOAT, GL_FALSE, 0, data);
        else
            funcs.glVertexAttribPointer(arrayIndex, 2, GL_FLOAT, GL_FALSE, 0, data);
    }
}

bool QOpenGL2PaintEngineExPrivate::uploadIndexData(const void *data, GLenum indexValueType, GLuint count)
{
    // Follow the uploadData() logic: VBOs are used only when VAO support is available.
    // Otherwise the legacy client-side pointer path is used.
    if (vao.isCreated()) {
        Q_ASSERT(indexValueType == GL_UNSIGNED_SHORT || indexValueType == GL_UNSIGNED_INT);
        indexBuffer.bind();
        indexBuffer.allocate(data, count * (indexValueType == GL_UNSIGNED_SHORT ? sizeof(quint16) : sizeof(quint32)));
        return true;
    }
    return false;
}

QT_END_NAMESPACE

#endif
