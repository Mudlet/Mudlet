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
 *   This class provides custom text encodings for character sets not      *
 *   available through Qt's standard QStringConverter or for special       *
 *   cases. Migrated from QTextCodec to standalone converters for Qt6.     *
 *   The tables map from (key) the 8-bit bytes with the most significant   *
 *   bit set (so 128 to 255) to (value) the Unicode codepoint (in the      *
 *   Basic Multi-Plane, a.k.a. BMP which only needs a *single* 16-bit to   *
 *   represent it). Not all "Extended ASCII" encodings encode every byte   *
 *   value and for those we use the "Replacement" character (U+FFFD).      *
 ***************************************************************************/

#include "TTextCodec.h"

// Lookup tables copied from original TTextCodec implementation
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
    QChar(0x038A), QChar(0x03AA), QChar(0x038C), QChar(0xFFFD), QChar(0xFFFD), QChar(0x038E), QChar(0x03AB), QChar(0x00A9),  // 90-97
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

const QVector<QChar> TTextCodec_medievia::CptoUnicode{
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
    QChar(0xE129), QChar(0xE12A), QChar(0xE12B), QChar(0x2620), QChar(0xE12C), QChar(0xE12D), QChar(0x2690), QChar(0xE12E),  // E0-E7
    QChar(0xE12F), QChar(0xE130), QChar(0xE131), QChar(0xE132), QChar(0x21E7), QChar(0x21E9), QChar(0x21E8), QChar(0x21E6),  // E8-EF
    QChar(0x25BA), QChar(0xE114), QChar(0x25AC), QChar(0x21A8), QChar(0xE119), QChar(0x2193), QChar(0x221F), QChar(0xE139),  // F0-F7
    QChar(0x25B2), QChar(0xE124), QChar(0xE125), QChar(0xE137), QChar(0xE138), QChar(0x25BC), QChar(0x2660), QChar(0xFFFD)}; // F8-FF
// clang-format on

// We give ours a distinct different name with an M_ prefix so we can tell it
// apart from a system one
QByteArray TTextCodec_437::name()
{
    return "M_CP437";
}

QByteArray TTextCodec_667::name()
{
    return "M_CP667";
}

QByteArray TTextCodec_737::name()
{
    return "M_CP737";
}

QByteArray TTextCodec_869::name()
{
    return "M_CP869";
}

QByteArray TTextCodec_medievia::name()
{
    return "M_MEDIEVIA";
}

// Data for the following two types of methods taken from entry in:
// http://www.iana.org/assignments/character-sets/character-sets.xml
QList<QByteArray> TTextCodec_437::aliases()
{
    return {"IBM437", "437", "cp437", "csPC8CodePage437"};
}

int TTextCodec_437::mibEnum()
{
    return 2011; // Same as IBM437
}

QList<QByteArray> TTextCodec_667::aliases()
{
    return {"Mazovia", "667", "cp6677"};
}

int TTextCodec_667::mibEnum()
{
    return 2999; // Vendor, and used just to give a number
}

QList<QByteArray> TTextCodec_737::aliases()
{
    return {"737", "cp737"};
}

int TTextCodec_737::mibEnum()
{
    return 2998; // Vendor, and used just to give a number
}

QList<QByteArray> TTextCodec_869::aliases()
{
    return {"IBM869", "ibm-869", "869", "cp869", "csIBM869", "ibm-869_P100-1995", "windows-869", "cp-gr"};
}

int TTextCodec_869::mibEnum()
{
    return 2054;
}

QList<QByteArray> TTextCodec_medievia::aliases()
{
    return {};
}

int TTextCodec_medievia::mibEnum()
{
    return 2997; // Made-up, and used just to give a number
}

// Convert bytes to Unicode string
QString TTextCodec_437::toUnicode(const QByteArray& bytes)
{
    QString result;
    result.reserve(bytes.size());
    
    for (unsigned char byte : bytes) {
        if (byte < 0x80) {
            result += QLatin1Char(byte);
        } else {
            result += CptoUnicode[byte - 0x80];
        }
    }
    return result;
}

QString TTextCodec_667::toUnicode(const QByteArray& bytes)
{
    QString result;
    result.reserve(bytes.size());
    
    for (unsigned char byte : bytes) {
        if (byte < 0x80) {
            result += QLatin1Char(byte);
        } else {
            result += CptoUnicode[byte - 0x80];
        }
    }
    return result;
}

QString TTextCodec_737::toUnicode(const QByteArray& bytes)
{
    QString result;
    result.reserve(bytes.size());
    
    for (unsigned char byte : bytes) {
        if (byte < 0x80) {
            result += QLatin1Char(byte);
        } else {
            result += CptoUnicode[byte - 0x80];
        }
    }
    return result;
}

QString TTextCodec_869::toUnicode(const QByteArray& bytes)
{
    QString result;
    result.reserve(bytes.size());
    
    for (unsigned char byte : bytes) {
        if (byte < 0x80) {
            result += QLatin1Char(byte);
        } else {
            result += CptoUnicode[byte - 0x80];
        }
    }
    return result;
}

QString TTextCodec_medievia::toUnicode(const QByteArray& bytes)
{
    QString result;
    result.reserve(bytes.size());
    
    for (unsigned char byte : bytes) {
        if (byte < 0x80) {
            result += QLatin1Char(byte);
        } else {
            result += CptoUnicode[byte - 0x80];
        }
    }
    return result;
}

// Convert Unicode string to bytes
QByteArray TTextCodec_437::fromUnicode(const QString& str)
{
    QByteArray result;
    result.reserve(str.size());
    
    for (const QChar& ch : str) {
        if (ch < QLatin1Char('\x80')) {
            result += ch.cell();
        } else {
            const int pos = CptoUnicode.indexOf(ch);
            if (pos < 0) {
                result += '?';
            } else {
                result += static_cast<char>(pos + 128);
            }
        }
    }
    return result;
}

QByteArray TTextCodec_667::fromUnicode(const QString& str)
{
    QByteArray result;
    result.reserve(str.size());
    
    for (const QChar& ch : str) {
        if (ch < QLatin1Char('\x80')) {
            result += ch.cell();
        } else {
            const int pos = CptoUnicode.indexOf(ch);
            if (pos < 0) {
                result += '?';
            } else {
                result += static_cast<char>(pos + 128);
            }
        }
    }
    return result;
}

QByteArray TTextCodec_737::fromUnicode(const QString& str)
{
    QByteArray result;
    result.reserve(str.size());
    
    for (const QChar& ch : str) {
        if (ch < QLatin1Char('\x80')) {
            result += ch.cell();
        } else {
            const int pos = CptoUnicode.indexOf(ch);
            if (pos < 0) {
                result += '?';
            } else {
                result += static_cast<char>(pos + 128);
            }
        }
    }
    return result;
}

QByteArray TTextCodec_869::fromUnicode(const QString& str)
{
    QByteArray result;
    result.reserve(str.size());
    
    for (const QChar& ch : str) {
        if (ch < QLatin1Char('\x80')) {
            result += ch.cell();
        } else {
            const int pos = CptoUnicode.indexOf(ch);
            if (pos < 0) {
                result += '?';
            } else {
                result += static_cast<char>(pos + 128);
            }
        }
    }
    return result;
}

QByteArray TTextCodec_medievia::fromUnicode(const QString& str)
{
    QByteArray result;
    result.reserve(str.size());
    
    for (const QChar& ch : str) {
        if (ch < QLatin1Char('\x80')) {
            result += ch.cell();
        } else {
            const int pos = CptoUnicode.indexOf(ch);
            if (pos < 0) {
                result += '?';
            } else {
                result += static_cast<char>(pos + 128);
            }
        }
    }
    return result;
}

// Check if string can be encoded
bool TTextCodec_437::canEncode(const QString& str)
{
    for (const QChar& ch : str) {
        if (ch >= QLatin1Char('\x80')) {
            const int pos = CptoUnicode.indexOf(ch);
            if (pos < 0) {
                return false;
            }
        }
    }
    return true;
}

bool TTextCodec_667::canEncode(const QString& str)
{
    for (const QChar& ch : str) {
        if (ch >= QLatin1Char('\x80')) {
            const int pos = CptoUnicode.indexOf(ch);
            if (pos < 0) {
                return false;
            }
        }
    }
    return true;
}

bool TTextCodec_737::canEncode(const QString& str)
{
    for (const QChar& ch : str) {
        if (ch >= QLatin1Char('\x80')) {
            const int pos = CptoUnicode.indexOf(ch);
            if (pos < 0) {
                return false;
            }
        }
    }
    return true;
}

bool TTextCodec_869::canEncode(const QString& str)
{
    for (const QChar& ch : str) {
        if (ch >= QLatin1Char('\x80')) {
            const int pos = CptoUnicode.indexOf(ch);
            if (pos < 0) {
                return false;
            }
        }
    }
    return true;
}

bool TTextCodec_medievia::canEncode(const QString& str)
{
    for (const QChar& ch : str) {
        if (ch >= QLatin1Char('\x80')) {
            const int pos = CptoUnicode.indexOf(ch);
            if (pos < 0) {
                return false;
            }
        }
    }
    return true;
}
