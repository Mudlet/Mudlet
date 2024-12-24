/***************************************************************************
 *   Copyright (C) 2020, 2024 by Stephen Lyons - slysven@virginmedia.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/***************************************************************************
 *   This class is entirely concerned with providing some codecs on        *
 *   platforms that do not come with a Qt provided QTextCodec for the      *
 *   encodings - which seems to be the Windows AppVeyor CI at this time,   *
 *   or for any "special cases".                                           *
 *   The tables map from (key) the 8-bit bytes with the most significant   *
 *   bit set (so 128 to 255) to (value) the Unicode codepoint (in the      *
 *   Basic Multi-Plane, a.k.a. BMP which only needs a *single* 16-bit to   *
 *   represent it - this system will break if we need to go beyond it)     *
 *   that it represents. Not all such "Extended ASCII" encodings encode    *
 *   every byte value and for those we use instead the "Replacement"       *
 *   character glyph (U+FFFD).                                             *
 ***************************************************************************/

#include "TTextCodec.h"

// These are copied from the tables I created for the TBuffer class (now moved
// to the TEncodingTable::csmEncodings table in that class) - though why do we
// need to have them *duplicated* HERE?
// clang-format off
const QVector<QChar> TTextCodec_437::CptoUnicode{
    //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
    QChar(0x00C7), QChar(0x00FC), QChar(0x00E9), QChar(0x00E2), QChar(0x00E4), QChar(0x00E0), QChar(0x00E5), QChar(0x00E7),  // 80-87
    QChar(0x00EA), QChar(0x00EB), QChar(0x00E8), QChar(0x00EF), QChar(0x00EE), QChar(0x00EC), QChar(0x00C4), QChar(0x00C5),  // 88-8F
    QChar(0x00C9), QChar(0x00E6), QChar(0x00C6), QChar(0x00F4), QChar(0x00F6), QChar(0x00F2), QChar(0x00FB), QChar(0x00F9),  // 90-97
    QChar(0x00FF), QChar(0x00D6), QChar(0x00DC), QChar(0x00A2), QChar(0x00A3), QChar(0x00A5), QChar(0x20A7), QChar(0x0192),  // 98-9F
    QChar(0x00E1), QChar(0x00ED), QChar(0x00F3), QChar(0x00FA), QChar(0x00F1), QChar(0x00D1), QChar(0x00AA), QChar(0x00BA),  // A0-A7
    QChar(0x00BF), QChar(0x2310), QChar(0x00AC), QChar(0x00BD), QChar(0x00BC), QChar(0x00A1), QChar(0x00AB), QChar(0x00BB),  // A8-AF
    QChar(0x2591), QChar(0x2592), QChar(0x2593), QChar(0x2502), QChar(0x2524), QChar(0x2561), QChar(0x2562), QChar(0x2556),  // B0-B7
    QChar(0x2555), QChar(0x2563), QChar(0x2551), QChar(0x2557), QChar(0x255D), QChar(0x255C), QChar(0x255B), QChar(0x2510),  // B8-BF
    QChar(0x2514), QChar(0x2534), QChar(0x252C), QChar(0x251C), QChar(0x2500), QChar(0x253C), QChar(0x255E), QChar(0x255F),  // C0-C7
    QChar(0x255A), QChar(0x2554), QChar(0x2569), QChar(0x2566), QChar(0x2560), QChar(0x2550), QChar(0x256C), QChar(0x2567),  // C8-CF
    QChar(0x2568), QChar(0x2564), QChar(0x2565), QChar(0x2559), QChar(0x2558), QChar(0x2552), QChar(0x2553), QChar(0x256B),  // D0-D7
    QChar(0x256A), QChar(0x2518), QChar(0x250C), QChar(0x2588), QChar(0x2584), QChar(0x258C), QChar(0x2590), QChar(0x2580),  // D8-DF
    QChar(0x03B1), QChar(0x00DF), QChar(0x0393), QChar(0x03C0), QChar(0x03A3), QChar(0x03C3), QChar(0x00B5), QChar(0x03C4),  // E0-E7
    QChar(0x03A6), QChar(0x0398), QChar(0x03A9), QChar(0x03B4), QChar(0x221E), QChar(0x03C6), QChar(0x03B5), QChar(0x2229),  // E8-EF
    QChar(0x2261), QChar(0x00B1), QChar(0x2265), QChar(0x2264), QChar(0x2320), QChar(0x2321), QChar(0x00F7), QChar(0x2248),  // F0=F7
    QChar(0x00B0), QChar(0x2219), QChar(0x00B7), QChar(0x221A), QChar(0x207F), QChar(0x00B2), QChar(0x25A0), QChar(0x00A0)}; // F8-FF

const QVector<QChar> TTextCodec_667::CptoUnicode{
    //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
    QChar(0x00C7), QChar(0x00FC), QChar(0x00E9), QChar(0x00E2), QChar(0x00E4), QChar(0x00E0), QChar(0x0105), QChar(0x00E7),  // 80-87
    QChar(0x00EA), QChar(0x00EB), QChar(0x00E8), QChar(0x00EF), QChar(0x00EE), QChar(0x0107), QChar(0x00C4), QChar(0x0104),  // 88-8F
    QChar(0x0118), QChar(0x0119), QChar(0x0142), QChar(0x00F4), QChar(0x00F6), QChar(0x0106), QChar(0x00FB), QChar(0x00F9),  // 90-97
    QChar(0x015A), QChar(0x00D6), QChar(0x00DC), QChar(0x00A2), QChar(0x0141), QChar(0x00A5), QChar(0x015B), QChar(0x0192),  // 98-9F
    QChar(0x0179), QChar(0x017B), QChar(0x00F3), QChar(0x0144), QChar(0x0143), QChar(0x017A), QChar(0x017C), QChar(0x00BA),  // A0-A7
    QChar(0x00BF), QChar(0x2310), QChar(0x00AC), QChar(0x00BD), QChar(0x00BC), QChar(0x00A1), QChar(0x00AB), QChar(0x00BB),  // A8-AF
    QChar(0x2591), QChar(0x2592), QChar(0x2593), QChar(0x2502), QChar(0x2524), QChar(0x2561), QChar(0x2562), QChar(0x2556),  // B0-B7
    QChar(0x2555), QChar(0x2563), QChar(0x2551), QChar(0x2557), QChar(0x255D), QChar(0x255C), QChar(0x255B), QChar(0x2510),  // B8-BF
    QChar(0x2514), QChar(0x2534), QChar(0x252C), QChar(0x251C), QChar(0x2500), QChar(0x253C), QChar(0x255E), QChar(0x255F),  // C0-C7
    QChar(0x255A), QChar(0x2554), QChar(0x2569), QChar(0x2566), QChar(0x2560), QChar(0x2550), QChar(0x256C), QChar(0x2567),  // C8-CF
    QChar(0x2568), QChar(0x2564), QChar(0x2565), QChar(0x2559), QChar(0x2558), QChar(0x2552), QChar(0x2553), QChar(0x256B),  // D0-D7
    QChar(0x256A), QChar(0x2518), QChar(0x250C), QChar(0x2588), QChar(0x2584), QChar(0x258C), QChar(0x2590), QChar(0x2580),  // D8-DF
    QChar(0x03B1), QChar(0x00DF), QChar(0x0393), QChar(0x03C0), QChar(0x03A3), QChar(0x03C3), QChar(0x00B5), QChar(0x03C4),  // E0-E7
    QChar(0x03A6), QChar(0x0398), QChar(0x03A9), QChar(0x03B4), QChar(0x221E), QChar(0x03C6), QChar(0x03B5), QChar(0x2229),  // E8-EF
    QChar(0x2261), QChar(0x00B1), QChar(0x2265), QChar(0x2264), QChar(0x2320), QChar(0x2321), QChar(0x00F7), QChar(0x2248),  // F0=F7
    QChar(0x00B0), QChar(0x2219), QChar(0x00B7), QChar(0x221A), QChar(0x207F), QChar(0x00B2), QChar(0x25A0), QChar(0x00A0)}; // F8-FF

const QVector<QChar> TTextCodec_737::CptoUnicode{
    //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
    QChar(0x0391), QChar(0x0392), QChar(0x0393), QChar(0x0394), QChar(0x0395), QChar(0x0396), QChar(0x0397), QChar(0x0398),  // 80-87
    QChar(0x0399), QChar(0x039A), QChar(0x039B), QChar(0x039C), QChar(0x039D), QChar(0x039E), QChar(0x039F), QChar(0x03A0),  // 88-8F
    QChar(0x03A1), QChar(0x03A3), QChar(0x03A4), QChar(0x03A5), QChar(0x03A6), QChar(0x03A7), QChar(0x03A8), QChar(0x03A9),  // 90-97
    QChar(0x00FF), QChar(0x00D6), QChar(0x00DC), QChar(0x00A2), QChar(0x00A3), QChar(0x00A5), QChar(0x20A7), QChar(0x0192),  // 98-9F
    QChar(0x00E1), QChar(0x00ED), QChar(0x00F3), QChar(0x00FA), QChar(0x00F1), QChar(0x00D1), QChar(0x00AA), QChar(0x00BA),  // A0-A7
    QChar(0x00BF), QChar(0x2310), QChar(0x00AC), QChar(0x00BD), QChar(0x00BC), QChar(0x00A1), QChar(0x00AB), QChar(0x00BB),  // A8-AF
    QChar(0x2591), QChar(0x2592), QChar(0x2593), QChar(0x2502), QChar(0x2524), QChar(0x2561), QChar(0x2562), QChar(0x2556),  // B0-B7
    QChar(0x2555), QChar(0x2563), QChar(0x2551), QChar(0x2557), QChar(0x255D), QChar(0x255C), QChar(0x255B), QChar(0x2510),  // B8-BF
    QChar(0x2514), QChar(0x2534), QChar(0x252C), QChar(0x251C), QChar(0x2500), QChar(0x253C), QChar(0x255E), QChar(0x255F),  // C0-C7
    QChar(0x255A), QChar(0x2554), QChar(0x2569), QChar(0x2566), QChar(0x2560), QChar(0x2550), QChar(0x256C), QChar(0x2567),  // C8-CF
    QChar(0x2568), QChar(0x2564), QChar(0x2565), QChar(0x2559), QChar(0x2558), QChar(0x2552), QChar(0x2553), QChar(0x256B),  // D0-D7
    QChar(0x256A), QChar(0x2518), QChar(0x250C), QChar(0x2588), QChar(0x2584), QChar(0x258C), QChar(0x2590), QChar(0x2580),  // D8-DF
    QChar(0x03C9), QChar(0x03AC), QChar(0x03AD), QChar(0x03AE), QChar(0x03CA), QChar(0x03AF), QChar(0x03CC), QChar(0x03CD),  // E0-E7
    QChar(0x03CB), QChar(0x03CE), QChar(0x0386), QChar(0x0388), QChar(0x0389), QChar(0x038A), QChar(0x038C), QChar(0x038E),  // E8-EF
    QChar(0x03C9), QChar(0x00B1), QChar(0x2265), QChar(0x2264), QChar(0x03AA), QChar(0x03AB), QChar(0x00F7), QChar(0x2248),  // F0=F7
    QChar(0x00B0), QChar(0x2219), QChar(0x00B7), QChar(0x221A), QChar(0x207F), QChar(0x00B2), QChar(0x25A0), QChar(0x00A0)}; // F8-FF

const QVector<QChar> TTextCodec_869::CptoUnicode{
    //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
    QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0x0386), QChar(0x20AC),  // 80-87
    QChar(0x00B7), QChar(0x00AC), QChar(0x00A6), QChar(0x2018), QChar(0x2019), QChar(0x0388), QChar(0x2015), QChar(0x0389),  // 88-8F
    QChar(0x038A), QChar(0x03AA), QChar(0x038C), QChar(0xFFFD), QChar(0xFFFD), QChar(0x038E), QChar(0x03A8), QChar(0x00A9),  // 90-97
    QChar(0x038F), QChar(0x00B2), QChar(0x00B3), QChar(0x03AC), QChar(0x00A3), QChar(0x03AD), QChar(0x03AE), QChar(0x03AF),  // 98-9F
    QChar(0x03CA), QChar(0x0390), QChar(0x03CC), QChar(0x03CD), QChar(0x0391), QChar(0x0392), QChar(0x0393), QChar(0x0394),  // A0-A7
    QChar(0x0395), QChar(0x0396), QChar(0x0397), QChar(0x00BD), QChar(0x0398), QChar(0x0399), QChar(0x00AB), QChar(0x00BB),  // A8-AF
    QChar(0x2591), QChar(0x2592), QChar(0x2593), QChar(0x2502), QChar(0x2524), QChar(0x039A), QChar(0x039B), QChar(0x039C),  // B0-B7
    QChar(0x039D), QChar(0x2563), QChar(0x2551), QChar(0x2557), QChar(0x255D), QChar(0x039E), QChar(0x039F), QChar(0x2510),  // B8-BF
    QChar(0x2514), QChar(0x2534), QChar(0x252C), QChar(0x251C), QChar(0x2500), QChar(0x253C), QChar(0x03A0), QChar(0x03A1),  // C0-C7
    QChar(0x255A), QChar(0x2554), QChar(0x2569), QChar(0x2566), QChar(0x2560), QChar(0x2550), QChar(0x256C), QChar(0x03A3),  // C8-CF
    QChar(0x03A4), QChar(0x03A5), QChar(0x03A6), QChar(0x03A7), QChar(0x03A8), QChar(0x03A9), QChar(0x03B1), QChar(0x03B2),  // D0-D7
    QChar(0x03B3), QChar(0x2518), QChar(0x250C), QChar(0x2588), QChar(0x2584), QChar(0x03B4), QChar(0x03B5), QChar(0x2580),  // D8-DF
    QChar(0x03B6), QChar(0x03B7), QChar(0x03B8), QChar(0x03B9), QChar(0x03BA), QChar(0x03BB), QChar(0x03BC), QChar(0x03BD),  // E0-E7
    QChar(0x03BE), QChar(0x03BF), QChar(0x03C0), QChar(0x03C1), QChar(0x03C3), QChar(0x03C2), QChar(0x03C4), QChar(0x0384),  // E8-EF
    QChar(0x00AD), QChar(0x00B1), QChar(0x03C5), QChar(0x03C6), QChar(0x03C7), QChar(0x00A7), QChar(0x03C8), QChar(0x0385),  // F0=F7
    QChar(0x00B0), QChar(0x00A8), QChar(0x03C9), QChar(0x03CB), QChar(0x03B0), QChar(0x03CE), QChar(0x25A0), QChar(0x00A0)}; // F8-FF

// This was based on WINDOWS-1252 but has some additonal codepoints that are
// not defined in the original. They wil map to glyphs in the Medievia specific
// font at: https://www.github.com/SlySven/Medievia_fonts which has some
// weird glpyhs at odd/non-standard locations (many of them duplicated to the
// PUA starting at U+E100) but others are now in the "expected" place for their
// codepoints.
const QVector<QChar> TTextCodec_medievia::CptoUnicode{
    //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
    //    Arc_L_D        Arc_R_D        Arc_L_U        Arc_R_U          Trees          Hills      Mountains       V Bridge
    QChar(0x256E), QChar(0x256D), QChar(0x256F), QChar(0x2570), QChar(0xE100), QChar(0xE101), QChar(0xE102), QChar(0xE103),  // 80-87
    //   H Bridge          Swamp          Ocean          Pines          Moors        Volcano      Graveyard      Lava pool
    QChar(0xE104), QChar(0xE105), QChar(0xE106), QChar(0xE107), QChar(0xE108), QChar(0xE109), QChar(0x26FC), QChar(0xE10A),  // 88-8F
    //Undergrowth       {unused}          Cliff      Waterfall    S Waterfall          River           Wall     Embankment
    QChar(0xE10B), QChar(0XFFFD), QChar(0xE10C), QChar(0xE10D), QChar(0xE10E), QChar(0xE10F), QChar(0xE110), QChar(0xE111),  // 90-97
    //      Ruins       Plateaux    Battlefield         Shrubs          Field      Tradeshop           Lake         Garden
    QChar(0xE112), QChar(0xE113), QChar(0x2694), QChar(0xE115), QChar(0xE116), QChar(0xE117), QChar(0xE118), QChar(0x2698),  // 98-9F
    //      Marsh           Reef           Sign         Dragon        Serpent        Holiday          Rocks  {unused but should be Moat}
    QChar(0xE11A), QChar(0xE11B), QChar(0xE11C), QChar(0xE11D), QChar(0xE11E), QChar(0xE11F), QChar(0xE120), QChar(0x2E1F),  // A0-A7
    // Silvershrine     Farmland           Gate          House          Altar        Archway         Jungle      Sandstone
    QChar(0xE121), QChar(0xE122), QChar(0xE123), QChar(0x2302), QChar(0x2625), QChar(0xE126), QChar(0xE127), QChar(0xE128),  // A8-AF
    // Light Box      Medium Box      Heavy Box        Line_V1   Line_T_L1_V1   Line_T_L2_V1   Line_T_L1_V2   Line_C_L1_D2
    QChar(0x2591), QChar(0x2592), QChar(0x2593), QChar(0x2502), QChar(0x2524), QChar(0x2561), QChar(0x2562), QChar(0x2556),  // B0-B7
    // Line_C_L2_D1 Line_T_L2_V2        Line_V2   Line_C_L2_D2   Line_C_L2_U2   Line_C_L1_U2   Line_C_L2_U1   Line_C_L1_D1
    QChar(0x2555), QChar(0x2563), QChar(0x2551), QChar(0x2557), QChar(0x255D), QChar(0x255C), QChar(0x255B), QChar(0x2510),  // B8-BF
    // Line_C_R1_U1 Line T_H1_U1   Line_T_H1_D1   Line_C_R1_V1        Line_H1   Line_X_H1_V1   Linw_T_R2_V1   Line_T_R1_V2
    QChar(0x2514), QChar(0x2534), QChar(0x252C), QChar(0x251C), QChar(0x2500), QChar(0x253C), QChar(0x255E), QChar(0x255F),  // C0-C7
    // Line_C_R2_V2 Line_C_R2_D2   Line_T_H2_U2   Line_T_H2_D2   Line_T_R2_V2        Line_H2   Line_X_H2_V2   Line_T_H2_U1
    QChar(0x255A), QChar(0x2554), QChar(0x2569), QChar(0x2566), QChar(0x2560), QChar(0x2550), QChar(0x256C), QChar(0x2567),  // C8-CF
    // Line_T_H1_U2 Line_T_H2_D1   Line_T_H1_D2   Line_C_R1_U2   Line_C_R2_U1   Line_C_R1_D2   Line_C_R2_D1   Line_X_H1_V2
    QChar(0x2568), QChar(0x2564), QChar(0x2565), QChar(0x2559), QChar(0x2558), QChar(0x2552), QChar(0x2553), QChar(0x256B),  // D0-D7
    // Line_X_H2_V1 Line_C_L1_U1   Line_C_R1_D1       Full Box     Bottom Box       Left Box      Right Box        Top Box
    QChar(0x256A), QChar(0x2518), QChar(0x250C), QChar(0x2588), QChar(0x2584), QChar(0x258C), QChar(0x2590), QChar(0x2580),  // D8-DF
    //      Brick     Flagstones        Cobbles          Skull           Book         Marble           Flag          Crown
    QChar(0xE129), QChar(0xE12A), QChar(0xE12B), QChar(0x2620), QChar(0xE12C), QChar(0xE12D), QChar(0x2690), QChar(0xE12E),  // E0-E7
    //       Shop         Planks         Player     Hollow Box        Ship_up      Ship_down     Ship_right      Ship_left
    QChar(0xE12F), QChar(0xE130), QChar(0xE131), QChar(0xE132), QChar(0x21E7), QChar(0x21E9), QChar(0x21E8), QChar(0x21E6),  // E8-EF
    //      TT_16          TT_21          TT_22          TT_23       Fountain          TT_25          TT_28          TT_29
    QChar(0x25BA), QChar(0xE114), QChar(0x25AC), QChar(0x21A8), QChar(0xE119), QChar(0x2193), QChar(0x221F), QChar(0xE139),  // F0-F7
    //      TT_30      Heavy Dot      Light Dot  Zone_entrance   Internal use          TT_31           TT_6       {unused}
    QChar(0x25B2), QChar(0xE124), QChar(0xE125), QChar(0xE137), QChar(0xE138), QChar(0x25BC), QChar(0x2660), QChar(0xFFFD)}; // F8-FF
// clang-format on

// We give ours a distict different name with an M_ prefix so we can tell it
// apart from a system one
QByteArray TTextCodec_437::name() const
{
    return "M_CP437";
}

QByteArray TTextCodec_667::name() const
{
    return "M_CP667";
}

QByteArray TTextCodec_737::name() const
{
    return "M_CP737";
}

QByteArray TTextCodec_869::name() const
{
    return "M_CP869";
}

QByteArray TTextCodec_medievia::name() const
{
    return "M_MEDIEVIA";
}

// Data for the following two types of methods taken from entry in:
// http://www.iana.org/assignments/character-sets/character-sets.xml
QList<QByteArray> TTextCodec_437::aliases() const
{
    return {"IBM437", "437", "cp437", "csPC8CodePage437"};
}

int TTextCodec_437::mibEnum() const
{
    return 2011; // Same as IBM437
}

QList<QByteArray> TTextCodec_667::aliases() const
{
    return {"Mazovia", "667", "cp6677"};
}

int TTextCodec_667::mibEnum() const
{
    return 2999; // Vendor, and used just to give a number
}

QList<QByteArray> TTextCodec_737::aliases() const
{
    return {"737", "cp737"};
}

int TTextCodec_737::mibEnum() const
{
    return 2998; // Vendor, and used just to give a number
}

QList<QByteArray> TTextCodec_869::aliases() const
{
    return {"IBM869", "ibm-869", "869", "cp869", "csIBM869", "ibm-869_P100-1995", "windows-869", "cp-gr"};
}

int TTextCodec_869::mibEnum() const
{
    return 2054;
}

QList<QByteArray> TTextCodec_medievia::aliases() const
{
    return {};
}

int TTextCodec_medievia::mibEnum() const
{
    return 2997; // Made-up, and used just to give a number
}

// All 8-bit values are valid for conversion from CP437 to Unicode so there will
// never be any invalid characters or any left over that cannot be converted and
// need to wait for the next chunk of data in the next call. However IF we have
// a non-null ConverterState pointer we ought to consider the first of the three
// int state_data[3] array and if castable to a (bool) false, i.e. zero we
// should insert a BOM if the IgnoreHeader flag is not set. If there is no state
// we should insert the BOM for each call:
QString TTextCodec_437::convertToUnicode(const char *in, int length, ConverterState *state) const
{
    QString result;
    bool headerDone = false;
    if (state) {
        // zero is false:
        if (!state->state_data[0]) {
            if (state->flags & QTextCodec::IgnoreHeader) {
                headerDone = true;
            }
            // non-zero, including -1 is true:
            state->state_data[0] = -1;
        }
    }

    // If we get here and headerDone is false then we need to insert the BOM
    if (!headerDone) {
        result += QChar::ByteOrderMark;
    }
    for (int i = 0; i < length; ++i) {
        unsigned char const ch = in[i];
        if (ch < 0x80) {
            // ASCII - which is the same as Latin1 when the MS Bit is not set:
            result += QLatin1Char(ch);
        } else {
            // Extended ASCII - look it up in the CptoUnicode table:
            result += CptoUnicode.at(in[i] - 128);
        }
    }
    return result;
}

QString TTextCodec_667::convertToUnicode(const char *in, int length, ConverterState *state) const
{
    QString result;
    bool headerDone = false;
    if (state) {
        // zero is false:
        if (!state->state_data[0]) {
            if (state->flags & QTextCodec::IgnoreHeader) {
                headerDone = true;
            }
            // non-zero, including -1 is true:
            state->state_data[0] = -1;
        }
    }

    // If we get here and headerDone is false then we need to insert the BOM
    if (!headerDone) {
        result += QChar::ByteOrderMark;
    }
    for (int i = 0; i < length; ++i) {
        unsigned char const ch = in[i];
        if (ch < 0x80) {
            // ASCII - which is the same as Latin1 when the MS Bit is not set:
            result += QLatin1Char(ch);
        } else {
            // Extended ASCII - look it up in the CptoUnicode table:
            result += CptoUnicode.at(in[i] - 128);
        }
    }
    return result;
}

QString TTextCodec_737::convertToUnicode(const char *in, int length, ConverterState *state) const
{
    QString result;
    bool headerDone = false;
    if (state) {
        // zero is false:
        if (!state->state_data[0]) {
            if (state->flags & QTextCodec::IgnoreHeader) {
                headerDone = true;
            }
            // non-zero, including -1 is true:
            state->state_data[0] = -1;
        }
    }

    // If we get here and headerDone is false then we need to insert the BOM
    if (!headerDone) {
        result += QChar::ByteOrderMark;
    }
    for (int i = 0; i < length; ++i) {
        unsigned char const ch = in[i];
        if (ch < 0x80) {
            // ASCII - which is the same as Latin1 when the MS Bit is not set:
            result += QLatin1Char(ch);
        } else {
            // Extended ASCII - look it up in the CptoUnicode table:
            result += CptoUnicode.at(in[i] - 128);
        }
    }
    return result;
}

// This one does have some invalid codes in the table so we need to also
// need to spot any incoming (char)in values that correspond to the invalid
// (replacement) codepoint:
QString TTextCodec_869::convertToUnicode(const char *in, int length, ConverterState *state) const
{
    QString result;
    bool headerDone = false;
    int invalidCharacters = 0;
    if (state) {
        // zero is false:
        if (!state->state_data[0]) {
            if (state->flags & QTextCodec::IgnoreHeader) {
                headerDone = true;
            }
            // non-zero, including -1 is true:
            state->state_data[0] = -1;
        }
    }

    // If we get here and headerDone is false then we need to insert the BOM
    if (!headerDone) {
        result += QChar::ByteOrderMark;
    }
    for (int i = 0; i < length; ++i) {
        unsigned char const ch = in[i];
        if (ch < 0x80) {
            // ASCII - which is the same as Latin1 when the MS Bit is not set:
            result += QLatin1Char(ch);
        } else {
            // Extended ASCII - look it up in the CptoUnicode table:
            if (CptoUnicode.at(in[i] - 128) == QChar(0xFFFD)) {
                // Oops, isn't a valid character there!
                ++invalidCharacters;
            }
            result += CptoUnicode.at(in[i] - 128);
        }
    }

    if (state) {
        state->invalidChars += invalidCharacters;
    }

    return result;
}

// This one does have some invalid codes in the table so we need to also
// need to spot any incoming (char)in values that correspond to the invalid
// (replacement) codepoint:
QString TTextCodec_medievia::convertToUnicode(const char *in, int length, ConverterState *state) const
{
    QString result;
    bool headerDone = false;
    int invalidCharacters = 0;
    if (state) {
        if (state->state_data[0] == 0) {
            if (state->flags & QTextCodec::IgnoreHeader) {
                headerDone = true;
            }
            // non-zero, including -1 is true:
            state->state_data[0] = -1;
        }
    }

    // If we get here and headerDone is false then we need to insert the BOM
    if (!headerDone) {
        result += QChar::ByteOrderMark;
    }
    for (int i = 0; i < length; ++i) {
        unsigned char const ch = in[i];
        if (ch < 0x80) {
            // ASCII - which is the same as Latin1 when the MS Bit is not set:
            result += QLatin1Char(ch);
        } else {
            // Extended ASCII - look it up in the CptoUnicode table:
            if (CptoUnicode.at(in[i] - 128) == QChar(0xFFFD)) {
                // Oops, isn't a valid character there!
                ++invalidCharacters;
            }
            result += CptoUnicode.at(in[i] - 128);
        }
    }

    if (state) {
        state->invalidChars += invalidCharacters;
    }

    return result;
}

// Only some QChar values are valid for conversion from Unicode to CP437 so
// there can be invalid characters and, for surrogate pairs or there can be a
// left over High surrogate that cannot be converted and need to wait for the
// following Low one in the next chunk of data in the next call. However IF we
// have a non-null ConverterState pointer we ought to:
// * consider the second of the three int state_data[3] array and if castable to
//   a (bool) false, i.e. zero we should remove a BOM if the IgnoreHeader flag
//   is not set and it is present at the start of the data. If there is no state
//   we should remove such a BOM at the beginning of "in" for all calls.
// * if ConvertInvalidToNull is set we are to convert invalid individual QChars
//   to nulls or if NOT set do what other encoders do and insert a '?' as we
//   should if there is no state
// * need to update invalidChars and remainingChars
QByteArray TTextCodec_437::convertFromUnicode(const QChar *in, int length, ConverterState *state) const
{
    int invalidChars = 0;
    int remainingChars = length;
    // Use '?' as replacement character (like some of Qt's own QTextCodec s)
    // unless we are told to use a NULL:
    const char replacement = (state && (state->flags & QTextCodec::ConvertInvalidToNull)) ? '\0' : '?';
    int i = 0;
    QByteArray result;

    if (!length) {
        // Avoid extra tests elsewhere on an empty Unicode string
        return {};
    }

    // Without state OR
    // With state AND state_data[1] being zero
    // - skip over first QChar if it IS the Byte Order Mark
    if (state) {
        if (state->state_data[1] == 0) {
            if (in[i] == QChar::ByteOrderMark) {
                ++i;
                --remainingChars;
            }
            // Record that we have inserted the BOM so we do not do it again in
            // THIS instance:
            state->state_data[1] = -1;
        }

    } else {
        if (in[i] == QChar::ByteOrderMark) {
            ++i;
            // Redundant but keeps things simpler if debugging.
            --remainingChars;
        }
    }

    for ( ; i < length; ++i) {
        if (Q_UNLIKELY(in[i].isHighSurrogate())) {
            // No character in the CP437 encoding is beyond the BMP so this is
            // always going to be an invalid character
            if ((i+1) < length) {
                // We do have at least one more QChar
                if (in[i+1].isLowSurrogate()) {
                    // And the pair of them are a non-BMP character,
                    // So produce a replacement and skip over them BOTH:
                    ++i;
                    --remainingChars;
                }

            } else {
                if (state) {
                    // exit the loop without changing remainingChars so that
                    // this character remains to be processed next time:
                    break;
                }
            }
            // produce a replacement character
            result += replacement;
            ++invalidChars;


        } else if (Q_UNLIKELY(in[i].isLowSurrogate())) {
            // We would already have skipped over the Low part of a surrogate
            // pair in the prior branch so this is a separate bogus Low
            // surrogate one, so produce a replacement and skip over just this QChar
            result += replacement;
            ++invalidChars;

        } else if (in[i] < QLatin1Char('\x80')) {
            // Within BMP and is ASCII QChar, so can use QLatinChar
            result += in[i].cell();

        } else {
            // In range of Extended ASCII \x80 up to end of BMP \xffff so let's
            // find out if it is in the lookup table.
            const int pos = CptoUnicode.indexOf(in[i]);
            // Protect against a bogus index that will break the use of an
            // quint8 afterwards:
            Q_ASSERT_X(pos < 128, "TTextCodec_437", "lookup table is malformed and oversized so that a bogus index of 128 or more was found");
            if (pos < 0) {
                // -1 is the sentinel value for not there, greater than 127 is
                // unreachable - or should be.
                result += replacement;
                ++invalidChars;
            } else {
                result += static_cast<char>(pos + 128);
            }
        }
        --remainingChars;
    }

    if (state) {
        // If this is raised above zero then the QTextCodec::canEncode(...)
        // methods will return a false value:
        state->invalidChars += invalidChars;
        // If we get to the end of the supplied QChar array then remainingChars
        // will, as the name suggests, be zero, otherwise it will increment the
        // provided value:
        state->remainingChars += remainingChars;
    }
    return {result};
}

QByteArray TTextCodec_667::convertFromUnicode(const QChar *in, int length, ConverterState *state) const
{
    int invalidChars = 0;
    int remainingChars = length;
    // Use '?' as replacement character (like some of Qt's own QTextCodec s)
    // unless we are told to use a NULL:
    const char replacement = (state && (state->flags & QTextCodec::ConvertInvalidToNull)) ? '\0' : '?';
    int i = 0;
    QByteArray result;

    if (!length) {
        // Avoid extra tests elsewhere on an empty Unicode string
        return {};
    }

    // Without state OR
    // With state AND state_data[1] being zero
    // - skip over first QChar if it IS the Byte Order Mark
    if (state) {
        if (state->state_data[1] == 0) {
            if (in[i] == QChar::ByteOrderMark) {
                ++i;
                --remainingChars;
            }
            // Record that we have inserted the BOM so we do not do it again in
            // THIS instance:
            state->state_data[1] = -1;
        }

    } else {
        if (in[i] == QChar::ByteOrderMark) {
            ++i;
            // Redundant but keeps things simpler if debugging.
            --remainingChars;
        }
    }

    for ( ; i < length; ++i) {
        if (Q_UNLIKELY(in[i].isHighSurrogate())) {
            // No character in the CP667 encoding is beyond the BMP so this is
            // always going to be an invalid character
            if ((i+1) < length) {
                // We do have at least one more QChar
                if (in[i+1].isLowSurrogate()) {
                    // And the pair of them are a non-BMP character,
                    // So produce a replacement and skip over them BOTH:
                    ++i;
                    --remainingChars;
                }

            } else {
                if (state) {
                    // exit the loop without changing remainingChars so that
                    // this character remains to be processed next time:
                    break;
                }
            }
            // produce a replacement character
            result += replacement;
            ++invalidChars;


        } else if (Q_UNLIKELY(in[i].isLowSurrogate())) {
            // We would already have skipped over the Low part of a surrogate
            // pair in the prior branch so this is a separate bogus Low
            // surrogate one, so produce a replacement and skip over just this QChar
            result += replacement;
            ++invalidChars;

        } else if (in[i] < QLatin1Char('\x80')) {
            // Within BMP and is ASCII QChar, so can use QLatinChar
            result += in[i].cell();

        } else {
            // In range of Extended ASCII \x80 up to end of BMP \xffff so let's
            // find out if it is in the lookup table.
            const int pos = CptoUnicode.indexOf(in[i]);
            // Protect against a bogus index that will break the use of an
            // quint8 afterwards:
            Q_ASSERT_X(pos < 128, "TTextCodec_667", "lookup table is malformed and oversized so that a bogus index of 128 or more was found");
            if (pos < 0) {
                // -1 is the sentinel value for not there, greater than 127 is
                // unreachable - or should be.
                result += replacement;
                ++invalidChars;
            } else {
                result += static_cast<char>(pos + 128);
            }
        }
        --remainingChars;
    }

    if (state) {
        // If this is raised above zero then the QTextCodec::canEncode(...)
        // methods will return a false value:
        state->invalidChars += invalidChars;
        // If we get to the end of the supplied QChar array then remainingChars
        // will, as the name suggests, be zero, otherwise it will increment the
        // provided value:
        state->remainingChars += remainingChars;
    }
    return {result};
}

QByteArray TTextCodec_737::convertFromUnicode(const QChar *in, int length, ConverterState *state) const
{
    int invalidChars = 0;
    int remainingChars = length;
    // Use '?' as replacement character (like some of Qt's own QTextCodec s)
    // unless we are told to use a NULL:
    const char replacement = (state && (state->flags & QTextCodec::ConvertInvalidToNull)) ? '\0' : '?';
    int i = 0;
    QByteArray result;

    if (!length) {
        // Avoid extra tests elsewhere on an empty Unicode string
        return {};
    }

    // Without state OR
    // With state AND state_data[1] being zero
    // - skip over first QChar if it IS the Byte Order Mark
    if (state) {
        if (state->state_data[1] == 0) {
            if (in[i] == QChar::ByteOrderMark) {
                ++i;
                --remainingChars;
            }
            // Record that we have inserted the BOM so we do not do it again in
            // THIS instance:
            state->state_data[1] = -1;
        }

    } else {
        if (in[i] == QChar::ByteOrderMark) {
            ++i;
            // Redundant but keeps things simpler if debugging.
            --remainingChars;
        }
    }

    for ( ; i < length; ++i) {
        if (Q_UNLIKELY(in[i].isHighSurrogate())) {
            // No character in the CP737 encoding is beyond the BMP so this is
            // always going to be an invalid character
            if ((i+1) < length) {
                // We do have at least one more QChar
                if (in[i+1].isLowSurrogate()) {
                    // And the pair of them are a non-BMP character,
                    // So produce a replacement and skip over them BOTH:
                    ++i;
                    --remainingChars;
                }

            } else {
                if (state) {
                    // exit the loop without changing remainingChars so that
                    // this character remains to be processed next time:
                    break;
                }
            }
            // produce a replacement character
            result += replacement;
            ++invalidChars;


        } else if (Q_UNLIKELY(in[i].isLowSurrogate())) {
            // We would already have skipped over the Low part of a surrogate
            // pair in the prior branch so this is a separate bogus Low
            // surrogate one, so produce a replacement and skip over just this QChar
            result += replacement;
            ++invalidChars;

        } else if (in[i] < QLatin1Char('\x80')) {
            // Within BMP and is ASCII QChar, so can use QLatinChar
            result += in[i].cell();

        } else {
            // In range of Extended ASCII \x80 up to end of BMP \xffff so let's
            // find out if it is in the lookup table.
            const int pos = CptoUnicode.indexOf(in[i]);
            // Protect against a bogus index that will break the use of an
            // quint8 afterwards:
            Q_ASSERT_X(pos < 128, "TTextCodec_737", "lookup table is malformed and oversized so that a bogus index of 128 or more was found");
            if (pos < 0) {
                // -1 is the sentinel value for not there, greater than 127 is
                // unreachable - or should be.
                result += replacement;
                ++invalidChars;
            } else {
                result += static_cast<char>(pos + 128);
            }
        }
        --remainingChars;
    }

    if (state) {
        // If this is raised above zero then the QTextCodec::canEncode(...)
        // methods will return a false value:
        state->invalidChars += invalidChars;
        // If we get to the end of the supplied QChar array then remainingChars
        // will, as the name suggests, be zero, otherwise it will increment the
        // provided value:
        state->remainingChars += remainingChars;
    }
    return {result};
}

QByteArray TTextCodec_869::convertFromUnicode(const QChar *in, int length, ConverterState *state) const
{
    int invalidChars = 0;
    int remainingChars = length;
    // Use '?' as replacement character (like some of Qt's own QTextCodec s)
    // unless we are told to use a NULL:
    const char replacement = (state && (state->flags & QTextCodec::ConvertInvalidToNull)) ? '\0' : '?';
    int i = 0;
    QByteArray result;

    if (!length) {
        // Avoid extra tests elsewhere on an empty Unicode string
        return {};
    }

    // Without state OR
    // With state AND state_data[1] being zero
    // - skip over first QChar if it IS the Byte Order Mark
    if (state) {
        if (state->state_data[1] == 0) {
            if (in[i] == QChar::ByteOrderMark) {
                ++i;
                --remainingChars;
            }
            // Record that we have inserted the BOM so we do not do it again in
            // THIS instance:
            state->state_data[1] = -1;
        }

    } else {
        if (in[i] == QChar::ByteOrderMark) {
            ++i;
            // Redundant but keeps things simpler if debugging.
            --remainingChars;
        }
    }

    for ( ; i < length; ++i) {
        if (Q_UNLIKELY(in[i].isHighSurrogate())) {
            // No character in the CP869 encoding is beyond the BMP so this is
            // always going to be an invalid character
            if ((i+1) < length) {
                // We do have at least one more QChar
                if (in[i+1].isLowSurrogate()) {
                    // And the pair of them are a non-BMP character,
                    // So produce a replacement and skip over them BOTH:
                    ++i;
                    --remainingChars;
                }

            } else {
                if (state) {
                    // exit the loop without changing remainingChars so that
                    // this character remains to be processed next time:
                    break;
                }
            }
            // produce a replacement character
            result += replacement;
            ++invalidChars;


        } else if (Q_UNLIKELY(in[i].isLowSurrogate())) {
            // We would already have skipped over the Low part of a surrogate
            // pair in the prior branch so this is a separate bogus Low
            // surrogate one, so produce a replacement and skip over just this QChar
            result += replacement;
            ++invalidChars;

        } else if (in[i] < QLatin1Char('\x80')) {
            // Within BMP and is ASCII QChar, so can use QLatinChar
            result += in[i].cell();

        } else {
            // In range of Extended ASCII \x80 up to end of BMP \xffff so let's
            // find out if it is in the lookup table.
            const int pos = CptoUnicode.indexOf(in[i]);
            // Protect against a bogus index that will break the use of an
            // quint8 afterwards:
            Q_ASSERT_X(pos < 128, "TTextCodec_869", "lookup table is malformed and oversized so that a bogus index of 128 or more was found");
            if (pos < 0) {
                // -1 is the sentinel value for not there, greater than 127 is
                // unreachable - or should be.
                result += replacement;
                ++invalidChars;
            } else {
                result += static_cast<char>(pos + 128);
            }
        }
        --remainingChars;
    }

    if (state) {
        // If this is raised above zero then the QTextCodec::canEncode(...)
        // methods will return a false value:
        state->invalidChars += invalidChars;
        // If we get to the end of the supplied QChar array then remainingChars
        // will, as the name suggests, be zero, otherwise it will increment the
        // provided value:
        state->remainingChars += remainingChars;
    }
    return {result};
}

QByteArray TTextCodec_medievia::convertFromUnicode(const QChar *in, int length, ConverterState *state) const
{
    int invalidChars = 0;
    int remainingChars = length;
    // Use '?' as replacement character (like some of Qt's own QTextCodec s)
    // unless we are told to use a NULL:
    const char replacement = (state && (state->flags & QTextCodec::ConvertInvalidToNull)) ? '\0' : '?';
    int i = 0;
    QByteArray result;

    if (!length) {
        // Avoid extra tests elsewhere on an empty Unicode string
        return {};
    }

    // Without state OR
    // With state AND state_data[1] being zero
    // - skip over first QChar if it IS the Byte Order Mark
    if (state) {
        if (state->state_data[1] == 0) {
            if (in[i] == QChar::ByteOrderMark) {
                ++i;
                --remainingChars;
            }
            // Record that we have inserted the BOM so we do not do it again in
            // THIS instance:
            state->state_data[1] = -1;
        }

    } else {
        if (in[i] == QChar::ByteOrderMark) {
            ++i;
            // Redundant but keeps things simpler if debugging.
            --remainingChars;
        }
    }

    for ( ; i < length; ++i) {
        if (Q_UNLIKELY(in[i].isHighSurrogate())) {
            // CHECK: Is any character in the Medievia encoding beyond the BMP?
            if ((i+1) < length) {
                // We do have at least one more QChar
                if (in[i+1].isLowSurrogate()) {
                    // And the pair of them are a non-BMP character,
                    // So produce a replacement and skip over them BOTH:
                    ++i;
                    --remainingChars;
                }

            } else {
                if (state) {
                    // exit the loop without changing remainingChars so that
                    // this character remains to be processed next time:
                    break;
                }
            }
            // produce a replacement character
            result += replacement;
            ++invalidChars;


        } else if (Q_UNLIKELY(in[i].isLowSurrogate())) {
            // We would already have skipped over the Low part of a surrogate
            // pair in the prior branch so this is a separate bogus Low
            // surrogate one, so produce a replacement and skip over just this QChar
            result += replacement;
            ++invalidChars;

        } else if (in[i] < QLatin1Char('\x80')) {
            // Within BMP and is ASCII QChar, so can use QLatinChar
            result += in[i].cell();

        } else {
            // In range of Extended ASCII \x80 up to end of BMP \xffff so let's
            // find out if it is in the lookup table.
            const int pos = CptoUnicode.indexOf(in[i]);
            // Protect against a bogus index that will break the use of an
            // quint8 afterwards:
            Q_ASSERT_X(pos < 128, "TTextCodec_medievia", "lookup table is malformed and oversized so that a bogus index of 128 or more was found");
            if (pos < 0) {
                // -1 is the sentinel value for not there, greater than 127 is
                // unreachable - or should be.
                result += replacement;
                ++invalidChars;
            } else {
                result += static_cast<char>(pos + 128);
            }
        }
        --remainingChars;
    }

    if (state) {
        // If this is raised above zero then the QTextCodec::canEncode(...)
        // methods will return a false value:
        state->invalidChars += invalidChars;
        // If we get to the end of the supplied QChar array then remainingChars
        // will, as the name suggests, be zero, otherwise it will increment the
        // provided value:
        state->remainingChars += remainingChars;
    }
    return {result};
}
