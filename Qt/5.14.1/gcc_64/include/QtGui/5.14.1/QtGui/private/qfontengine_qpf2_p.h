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

#ifndef QFONTENGINE_QPF2_P_H
#define QFONTENGINE_QPF2_P_H

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
#include <QtCore/qconfig.h>
#include <QtCore/qglobal.h>
#include <QtCore/qendian.h>
#include <QtCore/QBuffer>

#include "qfontengine_p.h"

#include <QtCore/QFile>

QT_BEGIN_NAMESPACE

class QFontEngine;
class QFreetypeFace;
class QBuffer;

class Q_GUI_EXPORT QFontEngineQPF2 : public QFontEngine
{
public:
    // if you add new tags please make sure to update the tables in
    // qpfutil.cpp and tools/makeqpf/qpf2.cpp
    enum HeaderTag {
        Tag_FontName,          // 0 string
        Tag_FileName,          // 1 string
        Tag_FileIndex,         // 2 quint32
        Tag_FontRevision,      // 3 quint32
        Tag_FreeText,          // 4 string
        Tag_Ascent,            // 5 QFixed
        Tag_Descent,           // 6 QFixed
        Tag_Leading,           // 7 QFixed
        Tag_XHeight,           // 8 QFixed
        Tag_AverageCharWidth,  // 9 QFixed
        Tag_MaxCharWidth,      // 10 QFixed
        Tag_LineThickness,     // 11 QFixed
        Tag_MinLeftBearing,    // 12 QFixed
        Tag_MinRightBearing,   // 13 QFixed
        Tag_UnderlinePosition, // 14 QFixed
        Tag_GlyphFormat,       // 15 quint8
        Tag_PixelSize,         // 16 quint8
        Tag_Weight,            // 17 quint8
        Tag_Style,             // 18 quint8
        Tag_EndOfHeader,       // 19 string
        Tag_WritingSystems,    // 20 bitfield

        NumTags
    };

    enum TagType {
        StringType,
        FixedType,
        UInt8Type,
        UInt32Type,
        BitFieldType
    };

    struct Tag
    {
        quint16 tag;
        quint16 size;
    };

    enum GlyphFormat {
        BitmapGlyphs = 1,
        AlphamapGlyphs = 8
    };

    enum {
        CurrentMajorVersion = 2,
        CurrentMinorVersion = 0
    };

    // The CMap is identical to the TrueType CMap table format
    // The GMap table is a normal array with the total number of
    // covered glyphs in the TrueType font
    enum BlockTag {
        CMapBlock,
        GMapBlock,
        GlyphBlock
    };

    struct Header
    {
        char magic[4]; // 'QPF2'
        quint32 lock;  // values: 0 = unlocked, 0xffffffff = read-only, otherwise qws client id of locking process
        quint8 majorVersion;
        quint8 minorVersion;
        quint16 dataSize;
    };

    struct Block
    {
        quint16 tag;
        quint16 pad;
        quint32 dataSize;
    };

    struct Glyph
    {
        quint8 width;
        quint8 height;
        quint8 bytesPerLine;
        qint8 x;
        qint8 y;
        qint8 advance;
    };

    QFontEngineQPF2(const QFontDef &def, const QByteArray &data);
    ~QFontEngineQPF2();

    FaceId faceId() const override { return face_id; }
    bool getSfntTableData(uint tag, uchar *buffer, uint *length) const override;

    virtual glyph_t glyphIndex(uint ucs4) const override;
    bool stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs, int *nglyphs, ShaperFlags flags) const override;
    void recalcAdvances(QGlyphLayout *, ShaperFlags) const override;

    void addOutlineToPath(qreal x, qreal y, const QGlyphLayout &glyphs, QPainterPath *path, QTextItem::RenderFlags flags) override;
    QImage alphaMapForGlyph(glyph_t t) override;

    glyph_metrics_t boundingBox(const QGlyphLayout &glyphs) override;
    glyph_metrics_t boundingBox(glyph_t glyph) override;

    QFixed ascent() const override;
    QFixed capHeight() const override;
    QFixed descent() const override;
    QFixed leading() const override;
    qreal maxCharWidth() const override;
    qreal minLeftBearing() const override;
    qreal minRightBearing() const override;
    QFixed underlinePosition() const override;
    QFixed lineThickness() const override;

    virtual int glyphCount() const override { return glyphMapEntries; }

    bool isValid() const;

    const Glyph *findGlyph(glyph_t g) const;

    static bool verifyHeader(const uchar *data, int size);
    static QVariant extractHeaderField(const uchar *data, HeaderTag tag);

private:

    const uchar *fontData;
    int dataSize;
    const uchar *cmap;
    quint32 cmapOffset;
    int cmapSize;
    quint32 glyphMapOffset;
    quint32 glyphMapEntries;
    quint32 glyphDataOffset;
    quint32 glyphDataSize;
    QString internalFileName;
    QString encodedFileName;
    bool readOnly;

    FaceId face_id;
    QByteArray freetypeCMapTable;
    mutable bool kerning_pairs_loaded;
};

struct QPF2Generator
{
    QPF2Generator(QBuffer *device, QFontEngine *engine)
        : dev(device), fe(engine) {}

    void generate();
    void writeHeader();
    void writeGMap();
    void writeBlock(QFontEngineQPF2::BlockTag tag, const QByteArray &data);

    void writeTaggedString(QFontEngineQPF2::HeaderTag tag, const QByteArray &string);
    void writeTaggedUInt32(QFontEngineQPF2::HeaderTag tag, quint32 value);
    void writeTaggedUInt8(QFontEngineQPF2::HeaderTag tag, quint8 value);
    void writeTaggedQFixed(QFontEngineQPF2::HeaderTag tag, QFixed value);

    void writeUInt16(quint16 value) { value = qToBigEndian(value); dev->write((const char *)&value, sizeof(value)); }
    void writeUInt32(quint32 value) { value = qToBigEndian(value); dev->write((const char *)&value, sizeof(value)); }
    void writeUInt8(quint8 value) { dev->write((const char *)&value, sizeof(value)); }
    void writeInt8(qint8 value) { dev->write((const char *)&value, sizeof(value)); }

    void align4() { while (dev->pos() & 3) { dev->putChar('\0'); } }

    QBuffer *dev;
    QFontEngine *fe;
};

QT_END_NAMESPACE

#endif // QFONTENGINE_QPF2_P_H
