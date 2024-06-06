/****************************************************************************
**
** Copyright (C) 2013 Imagination Technologies Limited, www.imgtec.com
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

#ifndef QDRAWHELPER_MIPS_DSP_P_H
#define QDRAWHELPER_MIPS_DSP_P_H

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

#include <private/qdrawhelper_p.h>

QT_BEGIN_NAMESPACE

#if defined(QT_COMPILER_SUPPORTS_MIPS_DSP)

extern "C" void qt_memfill32_asm_mips_dsp(quint32 *dest, quint32 value, qsizetype count);

extern "C" void comp_func_SourceOver_asm_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha);

extern "C" void comp_func_solid_DestinationOver_dsp_asm_x2(uint *dest, int length, uint color);

extern "C" void comp_func_solid_Source_dsp_asm_x2(uint *dest, int length, uint color, uint const_alpha);

extern "C" void comp_func_DestinationOver_dsp_asm_x2(uint *dest, const uint *src, int length, uint const_alpha);

extern "C" void comp_func_solid_SourceIn_dsp_asm_x2(uint *dest, int length, uint color, uint const_alpha);

extern "C" void comp_func_SourceIn_dsp_asm_x2(uint *dest, const uint *src, int length, uint const_alpha);

extern "C" void comp_func_solid_DestinationIn_dsp_asm_x2(uint *dest, int length, uint a);

extern "C" void comp_func_DestinationIn_dsp_asm_x2(uint *dest, const uint *src, int length, uint const_alpha);

extern "C" void comp_func_DestinationOut_dsp_asm_x2(uint *dest, const uint *src, int length, uint const_alpha);

extern "C" void comp_func_solid_SourceAtop_dsp_asm_x2(uint *dest, int length, uint color, uint const_alpha);

extern "C" void comp_func_SourceAtop_dsp_asm_x2(uint *dest, const uint *src, int length, uint const_alpha);

extern "C" void comp_func_solid_DestinationAtop_dsp_asm_x2(uint *dest, int length, uint color, uint const_alpha);

extern "C" void comp_func_DestinationAtop_dsp_asm_x2(uint *dest, const uint *src, int length, uint const_alpha);

extern "C" void comp_func_solid_XOR_dsp_asm_x2(uint *dest, int length, uint color, uint const_alpha);

extern "C" void comp_func_XOR_dsp_asm_x2(uint *dest, const uint *src, int length, uint const_alpha);

extern "C" void comp_func_solid_SourceOut_dsp_asm_x2(uint *dest, int length, uint color, uint const_alpha);

extern "C" void comp_func_SourceOut_dsp_asm_x2(uint *dest, const uint *src, int length, uint const_alpha);

extern "C" void comp_func_Source_dsp_asm_x2(uint *dest, const uint *src, int length, uint const_alpha);

extern "C" void qt_blend_argb32_on_argb32_mips_dsp_asm_x2(uint *dest, const uint *src, int length, uint const_alpha);

extern "C" void qt_blend_argb32_on_argb32_const_alpha_256_mips_dsp_asm(uint *dest, const uint *src, int length);

extern "C" void qt_blend_rgb16_on_rgb16_const_alpha_256_mips_dsp_asm(quint16 *dest, const quint16 *src, int length);

extern "C" void qt_blend_rgb16_on_rgb16_mips_dsp_asm(quint16 *dest, const quint16 *src, int length, uint const_alpha);

extern "C" uint * destfetchARGB32_asm_mips_dsp(uint *buffer, const uint *data, int length);

extern "C" uint * qt_destStoreARGB32_asm_mips_dsp(uint *buffer, const uint *data, int length);

extern "C" uint * fetchUntransformed_888_asm_mips_dsp(uint *buffer, const uchar *line, int length);

extern "C" uint * fetchUntransformed_444_asm_mips_dsp(uint *buffer, const uchar *line, int length);

extern "C" uint * fetchUntransformed_argb8565_premultiplied_asm_mips_dsp(uint *buffer, const uchar *line, int length);

void qt_blend_argb32_on_argb32_mips_dsp(uchar *destPixels, int dbpl,
                                      const uchar *srcPixels, int sbpl,
                                      int w, int h,
                                      int const_alpha);

void qt_blend_rgb32_on_rgb32_mips_dsp(uchar *destPixels, int dbpl,
                                      const uchar *srcPixels, int sbpl,
                                      int w, int h,
                                      int const_alpha);

void qt_blend_rgb16_on_rgb16_mips_dsp(uchar *destPixels, int dbpl,
                                      const uchar *srcPixels, int sbpl,
                                      int w, int h,
                                      int const_alpha);

void comp_func_Source_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha);

uint * QT_FASTCALL qt_destFetchARGB32_mips_dsp(uint *buffer,
                                          QRasterBuffer *rasterBuffer,
                                          int x, int y, int length);

void QT_FASTCALL qt_destStoreARGB32_mips_dsp(QRasterBuffer *rasterBuffer, int x, int y,
                                             const uint *buffer, int length);

void QT_FASTCALL comp_func_solid_Source_mips_dsp(uint *dest, int length, uint color, uint const_alpha);

void QT_FASTCALL comp_func_solid_SourceOver_mips_dsp(uint *dest, int length, uint color, uint const_alpha);

void QT_FASTCALL comp_func_solid_DestinationOver_mips_dsp(uint *dest, int length, uint color, uint const_alpha);

void QT_FASTCALL comp_func_solid_SourceOver_mips_dsp(uint *dest, int length, uint color, uint const_alpha);

void QT_FASTCALL comp_func_solid_DestinationOver_mips_dsp(uint *dest, int length, uint color, uint const_alpha);

void QT_FASTCALL comp_func_DestinationOver_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha);

void QT_FASTCALL comp_func_solid_SourceIn_mips_dsp(uint *dest, int length, uint color, uint const_alpha);

void QT_FASTCALL comp_func_SourceIn_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha);

void QT_FASTCALL comp_func_solid_DestinationIn_mips_dsp(uint *dest, int length, uint color, uint const_alpha);

void QT_FASTCALL comp_func_DestinationIn_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha);

void QT_FASTCALL comp_func_solid_DestinationOut_mips_dsp(uint *dest, int length, uint color, uint const_alpha);

void QT_FASTCALL comp_func_DestinationOut_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha);

void QT_FASTCALL comp_func_solid_SourceAtop_mips_dsp(uint *dest, int length, uint color, uint const_alpha);

void QT_FASTCALL comp_func_SourceAtop_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha);

void QT_FASTCALL comp_func_solid_DestinationAtop_mips_dsp(uint *dest, int length, uint color, uint const_alpha);

void QT_FASTCALL comp_func_DestinationAtop_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha);

void QT_FASTCALL comp_func_solid_XOR_mips_dsp(uint *dest, int length, uint color, uint const_alpha);

void QT_FASTCALL comp_func_solid_SourceOut_mips_dsp(uint *dest, int length, uint color, uint const_alpha);

void QT_FASTCALL comp_func_SourceOut_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha);

void QT_FASTCALL comp_func_XOR_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha);

const uint * QT_FASTCALL qt_fetchUntransformed_888_mips_dsp (uint *buffer,
                                                             const Operator *,
                                                             const QSpanData *data,
                                                             int y, int x, int length);

const uint * QT_FASTCALL qt_fetchUntransformed_444_mips_dsp (uint *buffer,
                                                             const Operator *,
                                                             const QSpanData *data,
                                                             int y, int x, int length);

const uint * QT_FASTCALL qt_fetchUntransformed_argb8565_premultiplied_mips_dsp (uint *buffer,
                                                                                const Operator *,
                                                                                const QSpanData *data,
                                                                                int y, int x, int length);



#if defined(__MIPS_DSPR2__)

extern "C" void qt_blend_rgb16_on_rgb16_mips_dspr2_asm(quint16 *dest, const quint16 *src, int length, uint const_alpha);

void qt_blend_rgb16_on_rgb16_mips_dspr2(uchar *destPixels, int dbpl,
                                        const uchar *srcPixels, int sbpl,
                                        int w, int h,
                                        int const_alpha);

const uint *QT_FASTCALL qt_fetchUntransformedRGB16_mips_dspr2(uint *buffer, const Operator *,
                                                              const QSpanData *data, int y, int x,
                                                              int length);
#endif // defined(__MIPS_DSPR2__)

#endif // QT_COMPILER_SUPPORTS_MIPS_DSP

QT_END_NAMESPACE

#endif // QDRAWHELPER_MIPS_DSP_P_H
