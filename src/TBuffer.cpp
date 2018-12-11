/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "TBuffer.h"

#include "mudlet.h"
#include "TConsole.h"

#include "pre_guard.h"
#include <QTextCodec>
#include <QRegularExpression>
#include "post_guard.h"

// Define this to get qDebug() messages about the decoding of UTF-8 data when it
// is not the single bytes of pure ASCII text:
// #define DEBUG_UTF8_PROCESSING
// Define this to get qDebug() messages about the decoding of GB2312/GBK/GB18030
// data when it is not the single bytes of pure ASCII text:
// #define DEBUG_GB_PROCESSING
// Define this to get qDebug() messages about the decoding of BIG5
// data when it is not the single bytes of pure ASCII text:
// #define DEBUG_BIG5_PROCESSING
// Define this to get qDebug() messags about the decoding of ANSI SGR sequences:
// #define DEBUG_SGR_PROCESSING
// Define this to get qDebug() messags about the decoding of ANSI OSC sequences:
// #define DEBUG_OSC_PROCESSING


// These tables have been regenerated from examination of the Qt source code
// particularly from:
// https://gitorious.org/qt/qt?p=qt:qt.git;a=blob;f=src/corelib/codecs/qsimplecodec.cpp;h=cb52ce35f369f7fbe5b04ff2c2cf1600bd794f4e;hb=HEAD
// It represents the Unicode codepoints that are to be used to represent
// extended ASCII characters with the MS Bit set.  The name is one that will
// be recognized as a valid encoding name to supply to:
// QTextCodec::codecForName(...)
// "ISO 885901" is not included here as it is inherent in Qt and has a straight
// one for one mapping of characters in the range 128 to 255 to the
// corresponding Unicode codepoint.
// Note that the codepoint 0xFFFD is the Unicode replacement character and
// is reserved to show, amongst other things, where a UTF-8 sequence of bytes
// is not a known character or, in these tables, the fact that no character was
// defined at that Extended ASCII character code.

// a map of computer-friendly encoding names as keys
// values are a pair of human-friendly name + encoding data

// clang-format off
// Do not let code reformatting tool mess this around!
// PLACEMARKER: Extended ASCII decoder Unicode codepoint lookup tables
const QMap<QString, QPair<QString, QVector<QChar>>> TBuffer::csmEncodingTable = {
    {QStringLiteral("ISO 8859-2"),
    qMakePair(tr("ISO 8859-2 (Central European)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0x0104), QChar(0x02D8), QChar(0x0141), QChar(0x00A4), QChar(0x013D), QChar(0x015A), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x0160), QChar(0x015E), QChar(0x0164), QChar(0x0179), QChar(0x00AD), QChar(0x017D), QChar(0x017B),  // A8-AF
        QChar(0x00B0), QChar(0x0105), QChar(0x02DB), QChar(0x0142), QChar(0x00B4), QChar(0x013E), QChar(0x015B), QChar(0x02C7),  // B0-B7
        QChar(0x00B8), QChar(0x0161), QChar(0x015F), QChar(0x0165), QChar(0x017A), QChar(0x02DD), QChar(0x017E), QChar(0x017C),  // B8-BF
        QChar(0x0154), QChar(0x00C1), QChar(0x00C2), QChar(0x0102), QChar(0x00C4), QChar(0x0139), QChar(0x0106), QChar(0x00C7),  // C0-C7
        QChar(0x010C), QChar(0x00C9), QChar(0x0118), QChar(0x00CB), QChar(0x011A), QChar(0x00CD), QChar(0x00CE), QChar(0x010E),  // C8-CF
        QChar(0x0110), QChar(0x0143), QChar(0x0147), QChar(0x00D3), QChar(0x00D4), QChar(0x0150), QChar(0x00D6), QChar(0x00D7),  // D0-D7
        QChar(0x0158), QChar(0x016E), QChar(0x00DA), QChar(0x0170), QChar(0x00DC), QChar(0x00DD), QChar(0x0162), QChar(0x00DF),  // D8-DF
        QChar(0x0155), QChar(0x00E1), QChar(0x00E2), QChar(0x0103), QChar(0x00E4), QChar(0x013A), QChar(0x0107), QChar(0x00E7),  // E0-E7
        QChar(0x010D), QChar(0x00E9), QChar(0x0119), QChar(0x00EB), QChar(0x011B), QChar(0x00ED), QChar(0x00EE), QChar(0x010F),  // E8-EF
        QChar(0x0111), QChar(0x0144), QChar(0x0148), QChar(0x00F3), QChar(0x00F4), QChar(0x0151), QChar(0x00F6), QChar(0x00F7),  // F0=F7
        QChar(0x0159), QChar(0x016F), QChar(0x00FA), QChar(0x0171), QChar(0x00FC), QChar(0x00FD), QChar(0x0163), QChar(0x02D9)}))}, // F8-FF
    {QStringLiteral("ISO 8859-3"),
    qMakePair(tr("ISO 8859-3 (South European)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0x0126), QChar(0x02D8), QChar(0x00A3), QChar(0x00A4), QChar(0xFFFD), QChar(0x0124), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x0130), QChar(0x015E), QChar(0x011E), QChar(0x0134), QChar(0x00AD), QChar(0xFFFD), QChar(0x017B),  // A8-AF
        QChar(0x00B0), QChar(0x0127), QChar(0x00B2), QChar(0x00B3), QChar(0x00B4), QChar(0x00B5), QChar(0x0125), QChar(0x00B7),  // B0-B7
        QChar(0x00B8), QChar(0x0131), QChar(0x015F), QChar(0x011F), QChar(0x0135), QChar(0x00BD), QChar(0xFFFD), QChar(0x017C),  // B8-BF
        QChar(0x00C0), QChar(0x00C1), QChar(0x00C2), QChar(0xFFFD), QChar(0x00C4), QChar(0x010A), QChar(0x0108), QChar(0x00C7),  // C0-C7
        QChar(0x00C8), QChar(0x00C9), QChar(0x00CA), QChar(0x00CB), QChar(0x00CC), QChar(0x00CD), QChar(0x00CE), QChar(0x00CF),  // C8-CF
        QChar(0xFFFD), QChar(0x00D1), QChar(0x00D2), QChar(0x00D3), QChar(0x00D4), QChar(0x0120), QChar(0x00D6), QChar(0x00D7),  // D0-D7
        QChar(0x011C), QChar(0x00D9), QChar(0x00DA), QChar(0x00DB), QChar(0x00DC), QChar(0x016C), QChar(0x015C), QChar(0x00DF),  // D8-DF
        QChar(0x00E0), QChar(0x00E1), QChar(0x00E2), QChar(0xFFFD), QChar(0x00E4), QChar(0x010B), QChar(0x0109), QChar(0x00E7),  // E0-E7
        QChar(0x00E8), QChar(0x00E9), QChar(0x00EA), QChar(0x00EB), QChar(0x00EC), QChar(0x00ED), QChar(0x00EE), QChar(0x00EF),  // E8-EF
        QChar(0xFFFD), QChar(0x00F1), QChar(0x00F2), QChar(0x00F3), QChar(0x00F4), QChar(0x0121), QChar(0x00F6), QChar(0x00F7),  // F0=F7
        QChar(0x011D), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x016D), QChar(0x015D), QChar(0x02D9)}))},// F8-FF
    {QStringLiteral("ISO 8859-4"),
    qMakePair(tr("ISO 8859-4 (Baltic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0x0104), QChar(0x0138), QChar(0x0156), QChar(0x00A4), QChar(0x0128), QChar(0x013B), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x0160), QChar(0x0112), QChar(0x0122), QChar(0x0166), QChar(0x00AD), QChar(0x017D), QChar(0x00AF),  // A8-AF
        QChar(0x00B0), QChar(0x0105), QChar(0x02DB), QChar(0x0157), QChar(0x00B4), QChar(0x0129), QChar(0x013C), QChar(0x02C7),  // B0-B7
        QChar(0x00B8), QChar(0x0161), QChar(0x0113), QChar(0x0123), QChar(0x0167), QChar(0x014A), QChar(0x017E), QChar(0x014B),  // B8-BF
        QChar(0x0100), QChar(0x00C1), QChar(0x00C2), QChar(0x00C3), QChar(0x00C4), QChar(0x00C5), QChar(0x00C6), QChar(0x012E),  // C0-C7
        QChar(0x010C), QChar(0x00C9), QChar(0x0118), QChar(0x00CB), QChar(0x0116), QChar(0x00CD), QChar(0x00CE), QChar(0x012A),  // C8-CF
        QChar(0x0110), QChar(0x0145), QChar(0x014C), QChar(0x0136), QChar(0x00D4), QChar(0x00D5), QChar(0x00D6), QChar(0x00D7),  // D0-D7
        QChar(0x00D8), QChar(0x0172), QChar(0x00DA), QChar(0x00DB), QChar(0x00DC), QChar(0x0168), QChar(0x016A), QChar(0x00DF),  // D8-DF
        QChar(0x0101), QChar(0x00E1), QChar(0x00E2), QChar(0x00E3), QChar(0x00E4), QChar(0x00E5), QChar(0x00E6), QChar(0x012F),  // E0-E7
        QChar(0x010D), QChar(0x00E9), QChar(0x0119), QChar(0x00EB), QChar(0x0117), QChar(0x00ED), QChar(0x00EE), QChar(0x012B),  // E8-EF
        QChar(0x0111), QChar(0x0146), QChar(0x014D), QChar(0x0137), QChar(0x00F4), QChar(0x00F5), QChar(0x00F6), QChar(0x00F7),  // F0=F7
        QChar(0x00F8), QChar(0x0173), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x0169), QChar(0x016B), QChar(0x02D9)}))},// F8-FF
    {QStringLiteral("ISO 8859-5"),
    qMakePair(tr("ISO 8859-5 (Cyrillic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0x0401), QChar(0x0402), QChar(0x0403), QChar(0x0404), QChar(0x0405), QChar(0x0406), QChar(0x0407),  // A0-A7
        QChar(0x0408), QChar(0x0409), QChar(0x040A), QChar(0x040B), QChar(0x040C), QChar(0x00AD), QChar(0x040E), QChar(0x040F),  // A8-AF
        QChar(0x0410), QChar(0x0411), QChar(0x0412), QChar(0x0413), QChar(0x0414), QChar(0x0415), QChar(0x0416), QChar(0x0417),  // B0-B7
        QChar(0x0418), QChar(0x0419), QChar(0x041A), QChar(0x041B), QChar(0x041C), QChar(0x041D), QChar(0x041E), QChar(0x041F),  // B8-BF
        QChar(0x0420), QChar(0x0421), QChar(0x0422), QChar(0x0423), QChar(0x0424), QChar(0x0425), QChar(0x0426), QChar(0x0427),  // C0-C7
        QChar(0x0428), QChar(0x0429), QChar(0x042A), QChar(0x042B), QChar(0x042C), QChar(0x042D), QChar(0x042E), QChar(0x042F),  // C8-CF
        QChar(0x0430), QChar(0x0431), QChar(0x0432), QChar(0x0433), QChar(0x0434), QChar(0x0435), QChar(0x0436), QChar(0x0437),  // D0-D7
        QChar(0x0438), QChar(0x0439), QChar(0x043A), QChar(0x043B), QChar(0x043C), QChar(0x043D), QChar(0x043E), QChar(0x043F),  // D8-DF
        QChar(0x0440), QChar(0x0441), QChar(0x0442), QChar(0x0443), QChar(0x0444), QChar(0x0445), QChar(0x0446), QChar(0x0447),  // E0-E7
        QChar(0x0448), QChar(0x0449), QChar(0x044A), QChar(0x044B), QChar(0x044C), QChar(0x044D), QChar(0x044E), QChar(0x044F),  // E8-EF
        QChar(0x2116), QChar(0x0451), QChar(0x0452), QChar(0x0453), QChar(0x0454), QChar(0x0455), QChar(0x0456), QChar(0x0457),  // F0=F7
        QChar(0x0458), QChar(0x0459), QChar(0x045A), QChar(0x045B), QChar(0x045C), QChar(0x00A7), QChar(0x045E), QChar(0x045F)}))},// F8-FF
    {QStringLiteral("ISO 8859-6"),
    qMakePair(tr("ISO 8859-6 (Arabic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0x00A4), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // A0-A7
        QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0x060C), QChar(0x00AD), QChar(0xFFFD), QChar(0xFFFD),  // A8-AF
        QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // B0-B7
        QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0x061B), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0x061F),  // B8-BF
        QChar(0xFFFD), QChar(0x0621), QChar(0x0622), QChar(0x0623), QChar(0x0624), QChar(0x0625), QChar(0x0626), QChar(0x0627),  // C0-C7
        QChar(0x0628), QChar(0x0629), QChar(0x062A), QChar(0x062B), QChar(0x062C), QChar(0x062D), QChar(0x062E), QChar(0x062F),  // C8-CF
        QChar(0x0630), QChar(0x0631), QChar(0x0632), QChar(0x0633), QChar(0x0634), QChar(0x0635), QChar(0x0636), QChar(0x0637),  // D0-D7
        QChar(0x0638), QChar(0x0639), QChar(0x063A), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // D8-DF
        QChar(0x0640), QChar(0x0641), QChar(0x0642), QChar(0x0643), QChar(0x0644), QChar(0x0645), QChar(0x0646), QChar(0x0647),  // E0-E7
        QChar(0x0648), QChar(0x0649), QChar(0x064A), QChar(0x064B), QChar(0x064C), QChar(0x064D), QChar(0x064E), QChar(0x064F),  // E8-EF
        QChar(0x0650), QChar(0x0651), QChar(0x0652), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // F0=F7
        QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD)}))},// F8-FF
    {QStringLiteral("ISO 8859-7"),
    qMakePair(tr("ISO 8859-7 (Greek)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0x2018), QChar(0x2019), QChar(0x00A3), QChar(0xFFFD), QChar(0xFFFD), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x00A9), QChar(0xFFFD), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0xFFFD), QChar(0x2015),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x00B2), QChar(0x00B3), QChar(0x0384), QChar(0x0385), QChar(0x0386), QChar(0x00B7),  // B0-B7
        QChar(0x0388), QChar(0x0389), QChar(0x038A), QChar(0x00BB), QChar(0x038C), QChar(0x00BD), QChar(0x038E), QChar(0x038F),  // B8-BF
        QChar(0x0390), QChar(0x0391), QChar(0x0392), QChar(0x0393), QChar(0x0394), QChar(0x0395), QChar(0x0396), QChar(0x0397),  // C0-C7
        QChar(0x0398), QChar(0x0399), QChar(0x039A), QChar(0x039B), QChar(0x039C), QChar(0x039D), QChar(0x039E), QChar(0x039F),  // C8-CF
        QChar(0x03A0), QChar(0x03A1), QChar(0xFFFD), QChar(0x03A3), QChar(0x03A4), QChar(0x03A5), QChar(0x03A6), QChar(0x03A7),  // D0-D7
        QChar(0x03A8), QChar(0x03A9), QChar(0x03AA), QChar(0x03AB), QChar(0x03AC), QChar(0x03AD), QChar(0x03AE), QChar(0x03AF),  // D8-DF
        QChar(0x03B0), QChar(0x03B1), QChar(0x03B2), QChar(0x03B3), QChar(0x03B4), QChar(0x03B5), QChar(0x03B6), QChar(0x03B7),  // E0-E7
        QChar(0x03B8), QChar(0x03B9), QChar(0x03BA), QChar(0x03BB), QChar(0x03BC), QChar(0x03BD), QChar(0x03BE), QChar(0x03BF),  // E8-EF
        QChar(0x03C0), QChar(0x03C1), QChar(0x03C2), QChar(0x03C3), QChar(0x03C4), QChar(0x03C5), QChar(0x03C6), QChar(0x03C7),  // F0=F7
        QChar(0x03C8), QChar(0x03C9), QChar(0x03CA), QChar(0x03CB), QChar(0x03CC), QChar(0x03CD), QChar(0x03CE), QChar(0xFFFD)}))},// F8-FF
    {QStringLiteral("ISO 8859-8"),
    qMakePair(tr("ISO 8859-8 (Hebrew Visual)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0xFFFD), QChar(0x00A2), QChar(0x00A3), QChar(0x00A4), QChar(0x00A5), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x00A9), QChar(0x00D7), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x203E),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x00B2), QChar(0x00B3), QChar(0x00B4), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x00B8), QChar(0x00B9), QChar(0x00F7), QChar(0x00BB), QChar(0x00BC), QChar(0x00BD), QChar(0x00BE), QChar(0xFFFD),  // B8-BF
        QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // C0-C7
        QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // C8-CF
        QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // D0-D7
        QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0x2017),  // D8-DF
        QChar(0x05D0), QChar(0x05D1), QChar(0x05D2), QChar(0x05D3), QChar(0x05D4), QChar(0x05D5), QChar(0x05D6), QChar(0x05D7),  // E0-E7
        QChar(0x05D8), QChar(0x05D9), QChar(0x05DA), QChar(0x05DB), QChar(0x05DC), QChar(0x05DD), QChar(0x05DE), QChar(0x05DF),  // E8-EF
        QChar(0x05E0), QChar(0x05E1), QChar(0x05E2), QChar(0x05E3), QChar(0x05E4), QChar(0x05E5), QChar(0x05E6), QChar(0x05E7),  // F0=F7
        QChar(0x05E8), QChar(0x05E9), QChar(0x05EA), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD)}))},// F8-FF
    {QStringLiteral("ISO 8859-9"),
    qMakePair(tr("ISO 8859-9 (Turkish)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0x00A1), QChar(0x00A2), QChar(0x00A3), QChar(0x00A4), QChar(0x00A5), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x00A9), QChar(0x00AA), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x00AF),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x00B2), QChar(0x00B3), QChar(0x00B4), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x00B8), QChar(0x00B9), QChar(0x00BA), QChar(0x00BB), QChar(0x00BC), QChar(0x00BD), QChar(0x00BE), QChar(0x00BF),  // B8-BF
        QChar(0x00C0), QChar(0x00C1), QChar(0x00C2), QChar(0x00C3), QChar(0x00C4), QChar(0x00C5), QChar(0x00C6), QChar(0x00C7),  // C0-C7
        QChar(0x00C8), QChar(0x00C9), QChar(0x00CA), QChar(0x00CB), QChar(0x00CC), QChar(0x00CD), QChar(0x00CE), QChar(0x00CF),  // C8-CF
        QChar(0x011E), QChar(0x00D1), QChar(0x00D2), QChar(0x00D3), QChar(0x00D4), QChar(0x00D5), QChar(0x00D6), QChar(0x00D7),  // D0-D7
        QChar(0x00D8), QChar(0x00D9), QChar(0x00DA), QChar(0x00DB), QChar(0x00DC), QChar(0x0130), QChar(0x015E), QChar(0x00DF),  // D8-DF
        QChar(0x00E0), QChar(0x00E1), QChar(0x00E2), QChar(0x00E3), QChar(0x00E4), QChar(0x00E5), QChar(0x00E6), QChar(0x00E7),  // E0-E7
        QChar(0x00E8), QChar(0x00E9), QChar(0x00EA), QChar(0x00EB), QChar(0x00EC), QChar(0x00ED), QChar(0x00EE), QChar(0x00EF),  // E8-EF
        QChar(0x011F), QChar(0x00F1), QChar(0x00F2), QChar(0x00F3), QChar(0x00F4), QChar(0x00F5), QChar(0x00F6), QChar(0x00F7),  // F0=F7
        QChar(0x00F8), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x0131), QChar(0x015F), QChar(0x00FF)}))},// F8-FF
    {QStringLiteral("ISO 8859-10"),
    qMakePair(tr("ISO 8859-10 (Nordic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0x0104), QChar(0x0112), QChar(0x0122), QChar(0x012A), QChar(0x0128), QChar(0x0136), QChar(0x00A7),  // A0-A7
        QChar(0x013B), QChar(0x0110), QChar(0x0160), QChar(0x0166), QChar(0x017D), QChar(0x00AD), QChar(0x016A), QChar(0x014A),  // A8-AF
        QChar(0x00B0), QChar(0x0105), QChar(0x0113), QChar(0x0123), QChar(0x012B), QChar(0x0129), QChar(0x0137), QChar(0x00B7),  // B0-B7
        QChar(0x013C), QChar(0x0111), QChar(0x0161), QChar(0x0167), QChar(0x017E), QChar(0x2015), QChar(0x016B), QChar(0x014B),  // B8-BF
        QChar(0x0100), QChar(0x00C1), QChar(0x00C2), QChar(0x00C3), QChar(0x00C4), QChar(0x00C5), QChar(0x00C6), QChar(0x012E),  // C0-C7
        QChar(0x010C), QChar(0x00C9), QChar(0x0118), QChar(0x00CB), QChar(0x0116), QChar(0x00CD), QChar(0x00CE), QChar(0x00CF),  // C8-CF
        QChar(0x00D0), QChar(0x0145), QChar(0x014C), QChar(0x00D3), QChar(0x00D4), QChar(0x00D5), QChar(0x00D6), QChar(0x0168),  // D0-D7
        QChar(0x00D8), QChar(0x0172), QChar(0x00DA), QChar(0x00DB), QChar(0x00DC), QChar(0x00DD), QChar(0x00DE), QChar(0x00DF),  // D8-DF
        QChar(0x0101), QChar(0x00E1), QChar(0x00E2), QChar(0x00E3), QChar(0x00E4), QChar(0x00E5), QChar(0x00E6), QChar(0x012F),  // E0-E7
        QChar(0x010D), QChar(0x00E9), QChar(0x0119), QChar(0x00EB), QChar(0x0117), QChar(0x00ED), QChar(0x00EE), QChar(0x00EF),  // E8-EF
        QChar(0x00F0), QChar(0x0146), QChar(0x014D), QChar(0x00F3), QChar(0x00F4), QChar(0x00F5), QChar(0x00F6), QChar(0x0169),  // F0=F7
        QChar(0x00F8), QChar(0x0173), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x00FD), QChar(0x00FE), QChar(0x0138)}))},// F8-FF
    {QStringLiteral("ISO 8859-11"),
    qMakePair(tr("ISO 8859-11 (Latin/Thai)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x20AC), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0x2026), QChar(0xFFFD), QChar(0xFFFD),  // 80-87
        QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 88-8F
        QChar(0xFFFD), QChar(0x2018), QChar(0x2019), QChar(0x201C), QChar(0x201D), QChar(0x2022), QChar(0x2013), QChar(0x2014),  // 90-97
        QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 98-9F
        QChar(0xFFFD), QChar(0x0E01), QChar(0x0E02), QChar(0x0E03), QChar(0x0E04), QChar(0x0E05), QChar(0x0E06), QChar(0x0E07),  // A0-A7
        QChar(0x0E08), QChar(0x0E09), QChar(0x0E0A), QChar(0x0E0B), QChar(0x0E0C), QChar(0x0E0D), QChar(0x0E0E), QChar(0x0E0F),  // A8-AF
        QChar(0x0E10), QChar(0x0E11), QChar(0x0E12), QChar(0x0E13), QChar(0x0E14), QChar(0x0E15), QChar(0x0E16), QChar(0x0E17),  // B0-B7
        QChar(0x0E18), QChar(0x0E19), QChar(0x0E1A), QChar(0x0E1B), QChar(0x0E1C), QChar(0x0E1D), QChar(0x0E1E), QChar(0x0E1F),  // B8-BF
        QChar(0x0E20), QChar(0x0E21), QChar(0x0E22), QChar(0x0E23), QChar(0x0E24), QChar(0x0E25), QChar(0x0E26), QChar(0x0E27),  // C0-C7
        QChar(0x0E28), QChar(0x0E29), QChar(0x0E2A), QChar(0x0E2B), QChar(0x0E2C), QChar(0x0E2D), QChar(0x0E2E), QChar(0x0E2F),  // C8-CF
        QChar(0x0E30), QChar(0x0E31), QChar(0x0E32), QChar(0x0E33), QChar(0x0E34), QChar(0x0E35), QChar(0x0E36), QChar(0x0E37),  // D0-D7
        QChar(0x0E38), QChar(0x0E39), QChar(0x0E3A), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0x0E3F),  // D8-DF
        QChar(0x0E40), QChar(0x0E41), QChar(0x0E42), QChar(0x0E43), QChar(0x0E44), QChar(0x0E45), QChar(0x0E46), QChar(0x0E47),  // E0-E7
        QChar(0x0E48), QChar(0x0E49), QChar(0x0E4A), QChar(0x0E4B), QChar(0x0E4C), QChar(0x0E4D), QChar(0x0E4E), QChar(0x0E4F),  // E8-EF
        QChar(0x0E50), QChar(0x0E51), QChar(0x0E52), QChar(0x0E53), QChar(0x0E54), QChar(0x0E55), QChar(0x0E56), QChar(0x0E57),  // F0=F7
        QChar(0x0E58), QChar(0x0E59), QChar(0x0E5A), QChar(0x0E5B), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD)}))},// F8-FF
    {QStringLiteral("ISO 8859-13"),
    qMakePair(tr("ISO 8859-13 (Baltic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0x201D), QChar(0x00A2), QChar(0x00A3), QChar(0x00A4), QChar(0x201E), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x00D8), QChar(0x00A9), QChar(0x0156), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x00C6),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x00B2), QChar(0x00B3), QChar(0x201C), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x00F8), QChar(0x00B9), QChar(0x0157), QChar(0x00BB), QChar(0x00BC), QChar(0x00BD), QChar(0x00BE), QChar(0x00E6),  // B8-BF
        QChar(0x0104), QChar(0x012E), QChar(0x0100), QChar(0x0106), QChar(0x00C4), QChar(0x00C5), QChar(0x0118), QChar(0x0112),  // C0-C7
        QChar(0x010C), QChar(0x00C9), QChar(0x0179), QChar(0x0116), QChar(0x0122), QChar(0x0136), QChar(0x012A), QChar(0x013B),  // C8-CF
        QChar(0x0160), QChar(0x0143), QChar(0x0145), QChar(0x00D3), QChar(0x014C), QChar(0x00D5), QChar(0x00D6), QChar(0x00D7),  // D0-D7
        QChar(0x0172), QChar(0x0141), QChar(0x015A), QChar(0x016A), QChar(0x00DC), QChar(0x017B), QChar(0x017D), QChar(0x00DF),  // D8-DF
        QChar(0x0105), QChar(0x012F), QChar(0x0101), QChar(0x0107), QChar(0x00E4), QChar(0x00E5), QChar(0x0119), QChar(0x0113),  // E0-E7
        QChar(0x010D), QChar(0x00E9), QChar(0x017A), QChar(0x0117), QChar(0x0123), QChar(0x0137), QChar(0x012B), QChar(0x013C),  // E8-EF
        QChar(0x0161), QChar(0x0144), QChar(0x0146), QChar(0x00F3), QChar(0x014D), QChar(0x00F5), QChar(0x00F6), QChar(0x00F7),  // F0=F7
        QChar(0x0173), QChar(0x0142), QChar(0x015B), QChar(0x016B), QChar(0x00FC), QChar(0x017C), QChar(0x017E), QChar(0x2019)}))},// F8-FF
    {QStringLiteral("ISO 8859-14"),
    qMakePair(tr("ISO 8859-14 (Celtic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0x1E02), QChar(0x1E03), QChar(0x00A3), QChar(0x010A), QChar(0x010B), QChar(0x1E0A), QChar(0x00A7),  // A0-A7
        QChar(0x1E80), QChar(0x00A9), QChar(0x1E82), QChar(0x1E0B), QChar(0x1EF2), QChar(0x00AD), QChar(0x00AE), QChar(0x0178),  // A8-AF
        QChar(0x1E1E), QChar(0x1E1F), QChar(0x0120), QChar(0x0121), QChar(0x1E40), QChar(0x1E41), QChar(0x00B6), QChar(0x1E56),  // B0-B7
        QChar(0x1E81), QChar(0x1E57), QChar(0x1E83), QChar(0x1E60), QChar(0x1EF3), QChar(0x1E84), QChar(0x1E85), QChar(0x1E61),  // B8-BF
        QChar(0x00C0), QChar(0x00C1), QChar(0x00C2), QChar(0x00C3), QChar(0x00C4), QChar(0x00C5), QChar(0x00C6), QChar(0x00C7),  // C0-C7
        QChar(0x00C8), QChar(0x00C9), QChar(0x00CA), QChar(0x00CB), QChar(0x00CC), QChar(0x00CD), QChar(0x00CE), QChar(0x00CF),  // C8-CF
        QChar(0x0174), QChar(0x00D1), QChar(0x00D2), QChar(0x00D3), QChar(0x00D4), QChar(0x00D5), QChar(0x00D6), QChar(0x1E6A),  // D0-D7
        QChar(0x00D8), QChar(0x00D9), QChar(0x00DA), QChar(0x00DB), QChar(0x00DC), QChar(0x00DD), QChar(0x0176), QChar(0x00DF),  // D8-DF
        QChar(0x00E0), QChar(0x00E1), QChar(0x00E2), QChar(0x00E3), QChar(0x00E4), QChar(0x00E5), QChar(0x00E6), QChar(0x00E7),  // E0-E7
        QChar(0x00E8), QChar(0x00E9), QChar(0x00EA), QChar(0x00EB), QChar(0x00EC), QChar(0x00ED), QChar(0x00EE), QChar(0x00EF),  // E8-EF
        QChar(0x0175), QChar(0x00F1), QChar(0x00F2), QChar(0x00F3), QChar(0x00F4), QChar(0x00F5), QChar(0x00F6), QChar(0x1E6B),  // F0=F7
        QChar(0x00F8), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x00FD), QChar(0x0177), QChar(0x00FF)}))},// F8-FF
    {QStringLiteral("ISO 8859-15"),
    qMakePair(tr("ISO 8859-15 (Western)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0x00A1), QChar(0x00A2), QChar(0x00A3), QChar(0x20AC), QChar(0x00A5), QChar(0x0160), QChar(0x00A7),  // A0-A7
        QChar(0x0161), QChar(0x00A9), QChar(0x00AA), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x00AF),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x00B2), QChar(0x00B3), QChar(0x017D), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x017E), QChar(0x00B9), QChar(0x00BA), QChar(0x00BB), QChar(0x0152), QChar(0x0153), QChar(0x0178), QChar(0x00BF),  // B8-BF
        QChar(0x00C0), QChar(0x00C1), QChar(0x00C2), QChar(0x00C3), QChar(0x00C4), QChar(0x00C5), QChar(0x00C6), QChar(0x00C7),  // C0-C7
        QChar(0x00C8), QChar(0x00C9), QChar(0x00CA), QChar(0x00CB), QChar(0x00CC), QChar(0x00CD), QChar(0x00CE), QChar(0x00CF),  // C8-CF
        QChar(0x00D0), QChar(0x00D1), QChar(0x00D2), QChar(0x00D3), QChar(0x00D4), QChar(0x00D5), QChar(0x00D6), QChar(0x00D7),  // D0-D7
        QChar(0x00D8), QChar(0x00D9), QChar(0x00DA), QChar(0x00DB), QChar(0x00DC), QChar(0x00DD), QChar(0x00DE), QChar(0x00DF),  // D8-DF
        QChar(0x00E0), QChar(0x00E1), QChar(0x00E2), QChar(0x00E3), QChar(0x00E4), QChar(0x00E5), QChar(0x00E6), QChar(0x00E7),  // E0-E7
        QChar(0x00E8), QChar(0x00E9), QChar(0x00EA), QChar(0x00EB), QChar(0x00EC), QChar(0x00ED), QChar(0x00EE), QChar(0x00EF),  // E8-EF
        QChar(0x00F0), QChar(0x00F1), QChar(0x00F2), QChar(0x00F3), QChar(0x00F4), QChar(0x00F5), QChar(0x00F6), QChar(0x00F7),  // F0=F7
        QChar(0x00F8), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x00FD), QChar(0x00FE), QChar(0x00FF)}))},// F8-FF
    {QStringLiteral("ISO 8859-16"),
    qMakePair(tr("ISO 8859-16 (Romanian)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0080), QChar(0x0081), QChar(0x0082), QChar(0x0083), QChar(0x0084), QChar(0x0085), QChar(0x0086), QChar(0x0087),  // 80-87
        QChar(0x0088), QChar(0x0089), QChar(0x008A), QChar(0x008B), QChar(0x008C), QChar(0x008D), QChar(0x008E), QChar(0x008F),  // 88-8F
        QChar(0x0090), QChar(0x0091), QChar(0x0092), QChar(0x0093), QChar(0x0094), QChar(0x0095), QChar(0x0096), QChar(0x0097),  // 90-97
        QChar(0x0098), QChar(0x0099), QChar(0x009A), QChar(0x009B), QChar(0x009C), QChar(0x009D), QChar(0x009E), QChar(0x009F),  // 98-9F
        QChar(0x00A0), QChar(0x0104), QChar(0x0105), QChar(0x0141), QChar(0x20AC), QChar(0x201E), QChar(0x0160), QChar(0x00A7),  // A0-A7
        QChar(0x0161), QChar(0x00A9), QChar(0x0218), QChar(0x00AB), QChar(0x0179), QChar(0x00AD), QChar(0x017A), QChar(0x017B),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x010C), QChar(0x0142), QChar(0x017D), QChar(0x201D), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x017E), QChar(0x010D), QChar(0x0219), QChar(0x00BB), QChar(0x0152), QChar(0x0153), QChar(0x0178), QChar(0x017C),  // B8-BF
        QChar(0x00C0), QChar(0x00C1), QChar(0x00C2), QChar(0x0102), QChar(0x00C4), QChar(0x0106), QChar(0x00C6), QChar(0x00C7),  // C0-C7
        QChar(0x00C8), QChar(0x00C9), QChar(0x00CA), QChar(0x00CB), QChar(0x00CC), QChar(0x00CD), QChar(0x00CE), QChar(0x00CF),  // C8-CF
        QChar(0x0110), QChar(0x0143), QChar(0x00D2), QChar(0x00D3), QChar(0x00D4), QChar(0x0150), QChar(0x00D6), QChar(0x015A),  // D0-D7
        QChar(0x0170), QChar(0x00D9), QChar(0x00DA), QChar(0x00DB), QChar(0x00DC), QChar(0x0118), QChar(0x021A), QChar(0x00DF),  // D8-DF
        QChar(0x00E0), QChar(0x00E1), QChar(0x00E2), QChar(0x0103), QChar(0x00E4), QChar(0x0107), QChar(0x00E6), QChar(0x00E7),  // E0-E7
        QChar(0x00E8), QChar(0x00E9), QChar(0x00EA), QChar(0x00EB), QChar(0x00EC), QChar(0x00ED), QChar(0x00EE), QChar(0x00EF),  // E8-EF
        QChar(0x0111), QChar(0x0144), QChar(0x00F2), QChar(0x00F3), QChar(0x00F4), QChar(0x0151), QChar(0x00F6), QChar(0x015B),  // F0=F7
        QChar(0x0171), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x0119), QChar(0x021B), QChar(0x00FF)}))},// F8-FF
    {QStringLiteral("CP850"),
    qMakePair(tr("CP850 (Western Europe)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x00C7), QChar(0x00FC), QChar(0x00E9), QChar(0x00E2), QChar(0x00E4), QChar(0x00E0), QChar(0x00E5), QChar(0x00E7),  // 80-87
        QChar(0x00EA), QChar(0x00EB), QChar(0x00E8), QChar(0x00EF), QChar(0x00EE), QChar(0x00EC), QChar(0x00C4), QChar(0x00C5),  // 88-8F
        QChar(0x00C9), QChar(0x00E6), QChar(0x00C6), QChar(0x00F4), QChar(0x00F6), QChar(0x00F2), QChar(0x00FB), QChar(0x00F9),  // 90-97
        QChar(0x00FF), QChar(0x00D6), QChar(0x00DC), QChar(0x00F8), QChar(0x00A3), QChar(0x00D8), QChar(0x00D7), QChar(0x0192),  // 98-9F
        QChar(0x00E1), QChar(0x00ED), QChar(0x00F3), QChar(0x00FA), QChar(0x00F1), QChar(0x00D1), QChar(0x00AA), QChar(0x00BA),  // A0-A7
        QChar(0x00BF), QChar(0x00AE), QChar(0x00AC), QChar(0x00BD), QChar(0x00BC), QChar(0x00A1), QChar(0x00AB), QChar(0x00BB),  // A8-AF
        QChar(0x2591), QChar(0x2592), QChar(0x2593), QChar(0x2502), QChar(0x2524), QChar(0x00C1), QChar(0x00C2), QChar(0x00C0),  // B0-B7
        QChar(0x00A9), QChar(0x2563), QChar(0x2551), QChar(0x2557), QChar(0x255D), QChar(0x00A2), QChar(0x00A5), QChar(0x2510),  // B8-BF
        QChar(0x2514), QChar(0x2534), QChar(0x252C), QChar(0x251C), QChar(0x2500), QChar(0x253C), QChar(0x00E3), QChar(0x00C3),  // C0-C7
        QChar(0x255A), QChar(0x2554), QChar(0x2569), QChar(0x2566), QChar(0x2560), QChar(0x2550), QChar(0x256C), QChar(0x00A4),  // C8-CF
        QChar(0x00F0), QChar(0x00D0), QChar(0x00CA), QChar(0x00CB), QChar(0x00C8), QChar(0x0131), QChar(0x00CD), QChar(0x00CE),  // D0-D7
        QChar(0x00CF), QChar(0x2518), QChar(0x250C), QChar(0x2588), QChar(0x2584), QChar(0x00A6), QChar(0x00CC), QChar(0x2580),  // D8-DF
        QChar(0x00D3), QChar(0x00DF), QChar(0x00D4), QChar(0x00D2), QChar(0x00F5), QChar(0x00D5), QChar(0x00B5), QChar(0x00FE),  // E0-E7
        QChar(0x00DE), QChar(0x00DA), QChar(0x00DB), QChar(0x00D9), QChar(0x00FD), QChar(0x00DD), QChar(0x00AF), QChar(0x00B4),  // E8-EF
        QChar(0x00AD), QChar(0x00B1), QChar(0x2017), QChar(0x00BE), QChar(0x00B6), QChar(0x00A7), QChar(0x00F7), QChar(0x00B8),  // F0=F7
        QChar(0x00B0), QChar(0x00A8), QChar(0x00B7), QChar(0x00B9), QChar(0x00B3), QChar(0x00B2), QChar(0x25A0), QChar(0x00A0)}))},// F8-FF
    {QStringLiteral("CP866"),
    qMakePair(tr("CP866 (Cyrillic/Russian)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0410), QChar(0x0411), QChar(0x0412), QChar(0x0413), QChar(0x0414), QChar(0x0415), QChar(0x0416), QChar(0x0417),  // 80-87
        QChar(0x0418), QChar(0x0419), QChar(0x041A), QChar(0x041B), QChar(0x041C), QChar(0x041D), QChar(0x041E), QChar(0x041F),  // 88-8F
        QChar(0x0420), QChar(0x0421), QChar(0x0422), QChar(0x0423), QChar(0x0424), QChar(0x0425), QChar(0x0426), QChar(0x0427),  // 90-97
        QChar(0x0428), QChar(0x0429), QChar(0x042A), QChar(0x042B), QChar(0x042C), QChar(0x042D), QChar(0x042E), QChar(0x042F),  // 98-9F
        QChar(0x0430), QChar(0x0431), QChar(0x0432), QChar(0x0433), QChar(0x0434), QChar(0x0435), QChar(0x0436), QChar(0x0437),  // A0-A7
        QChar(0x0438), QChar(0x0439), QChar(0x043A), QChar(0x043B), QChar(0x043C), QChar(0x043D), QChar(0x043E), QChar(0x043F),  // A8-AF
        QChar(0x2591), QChar(0x2592), QChar(0x2593), QChar(0x2502), QChar(0x2524), QChar(0x2561), QChar(0x2562), QChar(0x2556),  // B0-B7
        QChar(0x2555), QChar(0x2563), QChar(0x2551), QChar(0x2557), QChar(0x255D), QChar(0x255C), QChar(0x255B), QChar(0x2510),  // B8-BF
        QChar(0x2514), QChar(0x2534), QChar(0x252C), QChar(0x251C), QChar(0x2500), QChar(0x253C), QChar(0x255E), QChar(0x255F),  // C0-C7
        QChar(0x255A), QChar(0x2554), QChar(0x2569), QChar(0x2566), QChar(0x2560), QChar(0x2550), QChar(0x256C), QChar(0x2567),  // C8-CF
        QChar(0x2568), QChar(0x2564), QChar(0x2565), QChar(0x2559), QChar(0x2558), QChar(0x2552), QChar(0x2553), QChar(0x256B),  // D0-D7
        QChar(0x256A), QChar(0x2518), QChar(0x250C), QChar(0x2588), QChar(0x2584), QChar(0x258C), QChar(0x2590), QChar(0x2580),  // D8-DF
        QChar(0x0440), QChar(0x0441), QChar(0x0442), QChar(0x0443), QChar(0x0444), QChar(0x0445), QChar(0x0446), QChar(0x0447),  // E0-E7
        QChar(0x0448), QChar(0x0449), QChar(0x044A), QChar(0x044B), QChar(0x044C), QChar(0x044D), QChar(0x044E), QChar(0x044F),  // E8-EF
        QChar(0x0401), QChar(0x0451), QChar(0x0404), QChar(0x0454), QChar(0x0407), QChar(0x0457), QChar(0x040E), QChar(0x045E),  // F0=F7
        QChar(0x00B0), QChar(0x2219), QChar(0x00B7), QChar(0x221A), QChar(0x2116), QChar(0x00A4), QChar(0x25A0), QChar(0x00A0)}))},// F8-FF
    {QStringLiteral("CP874"),
    qMakePair(tr("ISO 8859-3 (South European)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x20AC), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0x2026), QChar(0xFFFD), QChar(0xFFFD),  // 80-87
        QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 88-8F
        QChar(0xFFFD), QChar(0x2018), QChar(0x2019), QChar(0x201C), QChar(0x201D), QChar(0x2022), QChar(0x2013), QChar(0x2014),  // 90-97
        QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 98-9F
        QChar(0x00A0), QChar(0x0E01), QChar(0x0E02), QChar(0x0E03), QChar(0x0E04), QChar(0x0E05), QChar(0x0E06), QChar(0x0E07),  // A0-A7
        QChar(0x0E08), QChar(0x0E09), QChar(0x0E0A), QChar(0x0E0B), QChar(0x0E0C), QChar(0x0E0D), QChar(0x0E0E), QChar(0x0E0F),  // A8-AF
        QChar(0x0E10), QChar(0x0E11), QChar(0x0E12), QChar(0x0E13), QChar(0x0E14), QChar(0x0E15), QChar(0x0E16), QChar(0x0E17),  // B0-B7
        QChar(0x0E18), QChar(0x0E19), QChar(0x0E1A), QChar(0x0E1B), QChar(0x0E1C), QChar(0x0E1D), QChar(0x0E1E), QChar(0x0E1F),  // B8-BF
        QChar(0x0E20), QChar(0x0E21), QChar(0x0E22), QChar(0x0E23), QChar(0x0E24), QChar(0x0E25), QChar(0x0E26), QChar(0x0E27),  // C0-C7
        QChar(0x0E28), QChar(0x0E29), QChar(0x0E2A), QChar(0x0E2B), QChar(0x0E2C), QChar(0x0E2D), QChar(0x0E2E), QChar(0x0E2F),  // C8-CF
        QChar(0x0E30), QChar(0x0E31), QChar(0x0E32), QChar(0x0E33), QChar(0x0E34), QChar(0x0E35), QChar(0x0E36), QChar(0x0E37),  // D0-D7
        QChar(0x0E38), QChar(0x0E39), QChar(0x0E3A), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0x0E3F),  // D8-DF
        QChar(0x0E40), QChar(0x0E41), QChar(0x0E42), QChar(0x0E43), QChar(0x0E44), QChar(0x0E45), QChar(0x0E46), QChar(0x0E47),  // E0-E7
        QChar(0x0E48), QChar(0x0E49), QChar(0x0E4A), QChar(0x0E4B), QChar(0x0E4C), QChar(0x0E4D), QChar(0x0E4E), QChar(0x0E4F),  // E8-EF
        QChar(0x0E50), QChar(0x0E51), QChar(0x0E52), QChar(0x0E53), QChar(0x0E54), QChar(0x0E55), QChar(0x0E56), QChar(0x0E57),  // F0=F7
        QChar(0x0E58), QChar(0x0E59), QChar(0x0E5A), QChar(0x0E5B), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD)}))},// F8-FF
    {QStringLiteral("KOI8-R"),
    qMakePair(tr("KOI8-R (Cyrillic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x2500), QChar(0x2502), QChar(0x250C), QChar(0x2510), QChar(0x2514), QChar(0x2518), QChar(0x251C), QChar(0x2524),  // 80-87
        QChar(0x252C), QChar(0x2534), QChar(0x253C), QChar(0x2580), QChar(0x2584), QChar(0x2588), QChar(0x258C), QChar(0x2590),  // 88-8F
        QChar(0x2591), QChar(0x2592), QChar(0x2593), QChar(0x2320), QChar(0x25A0), QChar(0x2219), QChar(0x221A), QChar(0x2248),  // 90-97
        QChar(0x2264), QChar(0x2265), QChar(0x00A0), QChar(0x2321), QChar(0x00B0), QChar(0x00B2), QChar(0x00B7), QChar(0x00F7),  // 98-9F
        QChar(0x2550), QChar(0x2551), QChar(0x2552), QChar(0x0451), QChar(0x2553), QChar(0x2554), QChar(0x2555), QChar(0x2556),  // A0-A7
        QChar(0x2557), QChar(0x2558), QChar(0x2559), QChar(0x255A), QChar(0x255B), QChar(0x255C), QChar(0x255D), QChar(0x255E),  // A8-AF
        QChar(0x255F), QChar(0x2560), QChar(0x2561), QChar(0x0401), QChar(0x2562), QChar(0x2563), QChar(0x2564), QChar(0x2565),  // B0-B7
        QChar(0x2566), QChar(0x2567), QChar(0x2568), QChar(0x2569), QChar(0x256A), QChar(0x256B), QChar(0x256C), QChar(0x00A9),  // B8-BF
        QChar(0x044E), QChar(0x0430), QChar(0x0431), QChar(0x0446), QChar(0x0434), QChar(0x0435), QChar(0x0444), QChar(0x0433),  // C0-C7
        QChar(0x0445), QChar(0x0438), QChar(0x0439), QChar(0x043A), QChar(0x043B), QChar(0x043C), QChar(0x043D), QChar(0x043E),  // C8-CF
        QChar(0x043F), QChar(0x044F), QChar(0x0440), QChar(0x0441), QChar(0x0442), QChar(0x0443), QChar(0x0436), QChar(0x0432),  // D0-D7
        QChar(0x044C), QChar(0x044B), QChar(0x0437), QChar(0x0448), QChar(0x044D), QChar(0x0449), QChar(0x0447), QChar(0x044A),  // D8-DF
        QChar(0x042E), QChar(0x0410), QChar(0x0411), QChar(0x0426), QChar(0x0414), QChar(0x0415), QChar(0x0424), QChar(0x0413),  // E0-E7
        QChar(0x0425), QChar(0x0418), QChar(0x0419), QChar(0x041A), QChar(0x041B), QChar(0x041C), QChar(0x041D), QChar(0x041E),  // E8-EF
        QChar(0x041F), QChar(0x042F), QChar(0x0420), QChar(0x0421), QChar(0x0422), QChar(0x0423), QChar(0x0416), QChar(0x0412),  // F0=F7
        QChar(0x042C), QChar(0x042B), QChar(0x0417), QChar(0x0428), QChar(0x042D), QChar(0x0429), QChar(0x0427), QChar(0x042A)}))},// F8-FF
    {QStringLiteral("KOI8-U"),
    qMakePair(tr("KOI8-U (Cyrillic/Ukrainian)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x2500), QChar(0x2502), QChar(0x250C), QChar(0x2510), QChar(0x2514), QChar(0x2518), QChar(0x251C), QChar(0x2524),  // 80-87
        QChar(0x252C), QChar(0x2534), QChar(0x253C), QChar(0x2580), QChar(0x2584), QChar(0x2588), QChar(0x258C), QChar(0x2590),  // 88-8F
        QChar(0x2591), QChar(0x2592), QChar(0x2593), QChar(0x2320), QChar(0x25A0), QChar(0x2219), QChar(0x221A), QChar(0x2248),  // 90-97
        QChar(0x2264), QChar(0x2265), QChar(0x00A0), QChar(0x2321), QChar(0x00B0), QChar(0x00B2), QChar(0x00B7), QChar(0x00F7),  // 98-9F
        QChar(0x2550), QChar(0x2551), QChar(0x2552), QChar(0x0451), QChar(0x0454), QChar(0x2554), QChar(0x0456), QChar(0x0457),  // A0-A7
        QChar(0x2557), QChar(0x2558), QChar(0x2559), QChar(0x255A), QChar(0x255B), QChar(0x0491), QChar(0x255D), QChar(0x255E),  // A8-AF
        QChar(0x255F), QChar(0x2560), QChar(0x2561), QChar(0x0401), QChar(0x0404), QChar(0x2563), QChar(0x0406), QChar(0x0407),  // B0-B7
        QChar(0x2566), QChar(0x2567), QChar(0x2568), QChar(0x2569), QChar(0x256A), QChar(0x0490), QChar(0x256C), QChar(0x00A9),  // B8-BF
        QChar(0x044E), QChar(0x0430), QChar(0x0431), QChar(0x0446), QChar(0x0434), QChar(0x0435), QChar(0x0444), QChar(0x0433),  // C0-C7
        QChar(0x0445), QChar(0x0438), QChar(0x0439), QChar(0x043A), QChar(0x043B), QChar(0x043C), QChar(0x043D), QChar(0x043E),  // C8-CF
        QChar(0x043F), QChar(0x044F), QChar(0x0440), QChar(0x0441), QChar(0x0442), QChar(0x0443), QChar(0x0436), QChar(0x0432),  // D0-D7
        QChar(0x044C), QChar(0x044B), QChar(0x0437), QChar(0x0448), QChar(0x044D), QChar(0x0449), QChar(0x0447), QChar(0x044A),  // D8-DF
        QChar(0x042E), QChar(0x0410), QChar(0x0411), QChar(0x0426), QChar(0x0414), QChar(0x0415), QChar(0x0424), QChar(0x0413),  // E0-E7
        QChar(0x0425), QChar(0x0418), QChar(0x0419), QChar(0x041A), QChar(0x041B), QChar(0x041C), QChar(0x041D), QChar(0x041E),  // E8-EF
        QChar(0x041F), QChar(0x042F), QChar(0x0420), QChar(0x0421), QChar(0x0422), QChar(0x0423), QChar(0x0416), QChar(0x0412),  // F0=F7
        QChar(0x042C), QChar(0x042B), QChar(0x0417), QChar(0x0428), QChar(0x042D), QChar(0x0429), QChar(0x0427), QChar(0x042A)}))},// F8-FF
    {QStringLiteral("MACINTOSH"),
    qMakePair(tr("MACINTOSH", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x00C4), QChar(0x00C5), QChar(0x00C7), QChar(0x00C9), QChar(0x00D1), QChar(0x00D6), QChar(0x00DC), QChar(0x00E1),  // 80-87
        QChar(0x00E0), QChar(0x00E2), QChar(0x00E4), QChar(0x00E3), QChar(0x00E5), QChar(0x00E7), QChar(0x00E9), QChar(0x00E8),  // 88-8F
        QChar(0x00EA), QChar(0x00EB), QChar(0x00ED), QChar(0x00EC), QChar(0x00EE), QChar(0x00EF), QChar(0x00F1), QChar(0x00F3),  // 90-97
        QChar(0x00F2), QChar(0x00F4), QChar(0x00F6), QChar(0x00F5), QChar(0x00FA), QChar(0x00F9), QChar(0x00FB), QChar(0x00FC),  // 98-9F
        QChar(0x2020), QChar(0x00B0), QChar(0x00A2), QChar(0x00A3), QChar(0x00A7), QChar(0x2022), QChar(0x00B6), QChar(0x00DF),  // A0-A7
        QChar(0x00AE), QChar(0x00A9), QChar(0x2122), QChar(0x00B4), QChar(0x00A8), QChar(0x2260), QChar(0x00C6), QChar(0x00D8),  // A8-AF
        QChar(0x221E), QChar(0x00B1), QChar(0x2264), QChar(0x2265), QChar(0x00A5), QChar(0x00B5), QChar(0x2202), QChar(0x2211),  // B0-B7
        QChar(0x220F), QChar(0x03C0), QChar(0x222B), QChar(0x00AA), QChar(0x00BA), QChar(0x03A9), QChar(0x00E6), QChar(0x00F8),  // B8-BF
        QChar(0x00BF), QChar(0x00A1), QChar(0x00AC), QChar(0x221A), QChar(0x0192), QChar(0x2248), QChar(0x2206), QChar(0x00AB),  // C0-C7
        QChar(0x00BB), QChar(0x2026), QChar(0x00A0), QChar(0x00C0), QChar(0x00C3), QChar(0x00D5), QChar(0x0152), QChar(0x0153),  // C8-CF
        QChar(0x2013), QChar(0x2014), QChar(0x201C), QChar(0x201D), QChar(0x2018), QChar(0x2019), QChar(0x00F7), QChar(0x25CA),  // D0-D7
        QChar(0x00FF), QChar(0x0178), QChar(0x2044), QChar(0x20AC), QChar(0x2039), QChar(0x203A), QChar(0xFB01), QChar(0xFB02),  // D8-DF
        QChar(0x2021), QChar(0x00B7), QChar(0x201A), QChar(0x201E), QChar(0x2030), QChar(0x00C2), QChar(0x00CA), QChar(0x00C1),  // E0-E7
        QChar(0x00CB), QChar(0x00C8), QChar(0x00CD), QChar(0x00CE), QChar(0x00CF), QChar(0x00CC), QChar(0x00D3), QChar(0x00D4),  // E8-EF
        QChar(0xF8FF), QChar(0x00D2), QChar(0x00DA), QChar(0x00DB), QChar(0x00D9), QChar(0x0131), QChar(0x02C6), QChar(0x02DC),  // F0=F7
        QChar(0x00AF), QChar(0x02D8), QChar(0x02D9), QChar(0x02DA), QChar(0x00B8), QChar(0x02DD), QChar(0x02DB), QChar(0x02C7)}))},// F8-FF
    {QStringLiteral("WINDOWS-1250"),
    qMakePair(tr("WINDOWS-1250 (Central European)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x20AC), QChar(0xFFFD), QChar(0x201A), QChar(0xFFFD), QChar(0x201E), QChar(0x2026), QChar(0x2020), QChar(0x2021),  // 80-87
        QChar(0xFFFD), QChar(0x2030), QChar(0x0160), QChar(0x2039), QChar(0x015A), QChar(0x0164), QChar(0x017D), QChar(0x0179),  // 88-8F
        QChar(0xFFFD), QChar(0x2018), QChar(0x2019), QChar(0x201C), QChar(0x201D), QChar(0x2022), QChar(0x2013), QChar(0x2014),  // 90-97
        QChar(0xFFFD), QChar(0x2122), QChar(0x0161), QChar(0x203A), QChar(0x015B), QChar(0x0165), QChar(0x017E), QChar(0x017A),  // 98-9F
        QChar(0x00A0), QChar(0x02C7), QChar(0x02D8), QChar(0x0141), QChar(0x00A4), QChar(0x0104), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x00A9), QChar(0x015E), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x017B),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x02DB), QChar(0x0142), QChar(0x00B4), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x00B8), QChar(0x0105), QChar(0x015F), QChar(0x00BB), QChar(0x013D), QChar(0x02DD), QChar(0x013E), QChar(0x017C),  // B8-BF
        QChar(0x0154), QChar(0x00C1), QChar(0x00C2), QChar(0x0102), QChar(0x00C4), QChar(0x0139), QChar(0x0106), QChar(0x00C7),  // C0-C7
        QChar(0x010C), QChar(0x00C9), QChar(0x0118), QChar(0x00CB), QChar(0x011A), QChar(0x00CD), QChar(0x00CE), QChar(0x010E),  // C8-CF
        QChar(0x0110), QChar(0x0143), QChar(0x0147), QChar(0x00D3), QChar(0x00D4), QChar(0x0150), QChar(0x00D6), QChar(0x00D7),  // D0-D7
        QChar(0x0158), QChar(0x016E), QChar(0x00DA), QChar(0x0170), QChar(0x00DC), QChar(0x00DD), QChar(0x0162), QChar(0x00DF),  // D8-DF
        QChar(0x0155), QChar(0x00E1), QChar(0x00E2), QChar(0x0103), QChar(0x00E4), QChar(0x013A), QChar(0x0107), QChar(0x00E7),  // E0-E7
        QChar(0x010D), QChar(0x00E9), QChar(0x0119), QChar(0x00EB), QChar(0x011B), QChar(0x00ED), QChar(0x00EE), QChar(0x010F),  // E8-EF
        QChar(0x0111), QChar(0x0144), QChar(0x0148), QChar(0x00F3), QChar(0x00F4), QChar(0x0151), QChar(0x00F6), QChar(0x00F7),  // F0=F7
        QChar(0x0159), QChar(0x016F), QChar(0x00FA), QChar(0x0171), QChar(0x00FC), QChar(0x00FD), QChar(0x0163), QChar(0x02D9)}))},// F8-FF
    {QStringLiteral("WINDOWS-1251"),
    qMakePair(tr("WINDOWS-1251 (Cyrillic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x0402), QChar(0x0403), QChar(0x201A), QChar(0x0453), QChar(0x201E), QChar(0x2026), QChar(0x2020), QChar(0x2021),  // 80-87
        QChar(0x20AC), QChar(0x2030), QChar(0x0409), QChar(0x2039), QChar(0x040A), QChar(0x040C), QChar(0x040B), QChar(0x040F),  // 88-8F
        QChar(0x0452), QChar(0x2018), QChar(0x2019), QChar(0x201C), QChar(0x201D), QChar(0x2022), QChar(0x2013), QChar(0x2014),  // 90-97
        QChar(0xFFFD), QChar(0x2122), QChar(0x0459), QChar(0x203A), QChar(0x045A), QChar(0x045C), QChar(0x045B), QChar(0x045F),  // 98-9F
        QChar(0x00A0), QChar(0x040E), QChar(0x045E), QChar(0x0408), QChar(0x00A4), QChar(0x0490), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x0401), QChar(0x00A9), QChar(0x0404), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x0407),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x0406), QChar(0x0456), QChar(0x0491), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x0451), QChar(0x2116), QChar(0x0454), QChar(0x00BB), QChar(0x0458), QChar(0x0405), QChar(0x0455), QChar(0x0457),  // B8-BF
        QChar(0x0410), QChar(0x0411), QChar(0x0412), QChar(0x0413), QChar(0x0414), QChar(0x0415), QChar(0x0416), QChar(0x0417),  // C0-C7
        QChar(0x0418), QChar(0x0419), QChar(0x041A), QChar(0x041B), QChar(0x041C), QChar(0x041D), QChar(0x041E), QChar(0x041F),  // C8-CF
        QChar(0x0420), QChar(0x0421), QChar(0x0422), QChar(0x0423), QChar(0x0424), QChar(0x0425), QChar(0x0426), QChar(0x0427),  // D0-D7
        QChar(0x0428), QChar(0x0429), QChar(0x042A), QChar(0x042B), QChar(0x042C), QChar(0x042D), QChar(0x042E), QChar(0x042F),  // D8-DF
        QChar(0x0430), QChar(0x0431), QChar(0x0432), QChar(0x0433), QChar(0x0434), QChar(0x0435), QChar(0x0436), QChar(0x0437),  // E0-E7
        QChar(0x0438), QChar(0x0439), QChar(0x043A), QChar(0x043B), QChar(0x043C), QChar(0x043D), QChar(0x043E), QChar(0x043F),  // E8-EF
        QChar(0x0440), QChar(0x0441), QChar(0x0442), QChar(0x0443), QChar(0x0444), QChar(0x0445), QChar(0x0446), QChar(0x0447),  // F0=F7
        QChar(0x0448), QChar(0x0449), QChar(0x044A), QChar(0x044B), QChar(0x044C), QChar(0x044D), QChar(0x044E), QChar(0x044F)}))},// F8-FF
    {QStringLiteral("WINDOWS-1252"),
    qMakePair(tr("WINDOWS-1252 (Western)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x20AC), QChar(0xFFFD), QChar(0x201A), QChar(0x0192), QChar(0x201E), QChar(0x2026), QChar(0x2020), QChar(0x2021),  // 80-87
        QChar(0x02C6), QChar(0x2030), QChar(0x0160), QChar(0x2039), QChar(0x0152), QChar(0xFFFD), QChar(0x017D), QChar(0xFFFD),  // 88-8F
        QChar(0xFFFD), QChar(0x2018), QChar(0x2019), QChar(0x201C), QChar(0x201D), QChar(0x2022), QChar(0x2013), QChar(0x2014),  // 90-97
        QChar(0x02DC), QChar(0x2122), QChar(0x0161), QChar(0x203A), QChar(0x0153), QChar(0xFFFD), QChar(0x017E), QChar(0x0178),  // 98-9F
        QChar(0x00A0), QChar(0x00A1), QChar(0x00A2), QChar(0x00A3), QChar(0x00A4), QChar(0x00A5), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x00A9), QChar(0x00AA), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x00AF),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x00B2), QChar(0x00B3), QChar(0x00B4), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x00B8), QChar(0x00B9), QChar(0x00BA), QChar(0x00BB), QChar(0x00BC), QChar(0x00BD), QChar(0x00BE), QChar(0x00BF),  // B8-BF
        QChar(0x00C0), QChar(0x00C1), QChar(0x00C2), QChar(0x00C3), QChar(0x00C4), QChar(0x00C5), QChar(0x00C6), QChar(0x00C7),  // C0-C7
        QChar(0x00C8), QChar(0x00C9), QChar(0x00CA), QChar(0x00CB), QChar(0x00CC), QChar(0x00CD), QChar(0x00CE), QChar(0x00CF),  // C8-CF
        QChar(0x00D0), QChar(0x00D1), QChar(0x00D2), QChar(0x00D3), QChar(0x00D4), QChar(0x00D5), QChar(0x00D6), QChar(0x00D7),  // D0-D7
        QChar(0x00D8), QChar(0x00D9), QChar(0x00DA), QChar(0x00DB), QChar(0x00DC), QChar(0x00DD), QChar(0x00DE), QChar(0x00DF),  // D8-DF
        QChar(0x00E0), QChar(0x00E1), QChar(0x00E2), QChar(0x00E3), QChar(0x00E4), QChar(0x00E5), QChar(0x00E6), QChar(0x00E7),  // E0-E7
        QChar(0x00E8), QChar(0x00E9), QChar(0x00EA), QChar(0x00EB), QChar(0x00EC), QChar(0x00ED), QChar(0x00EE), QChar(0x00EF),  // E8-EF
        QChar(0x00F0), QChar(0x00F1), QChar(0x00F2), QChar(0x00F3), QChar(0x00F4), QChar(0x00F5), QChar(0x00F6), QChar(0x00F7),  // F0=F7
        QChar(0x00F8), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x00FD), QChar(0x00FE), QChar(0x00FF)}))},// F8-FF
    {QStringLiteral("WINDOWS-1253"),
    qMakePair(tr("WINDOWS-1253 (Greek)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x20AC), QChar(0xFFFD), QChar(0x201A), QChar(0x0192), QChar(0x201E), QChar(0x2026), QChar(0x2020), QChar(0x2021),  // 80-87
        QChar(0xFFFD), QChar(0x2030), QChar(0xFFFD), QChar(0x2039), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 88-8F
        QChar(0xFFFD), QChar(0x2018), QChar(0x2019), QChar(0x201C), QChar(0x201D), QChar(0x2022), QChar(0x2013), QChar(0x2014),  // 90-97
        QChar(0xFFFD), QChar(0x2122), QChar(0xFFFD), QChar(0x203A), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 98-9F
        QChar(0x00A0), QChar(0x0385), QChar(0x0386), QChar(0x00A3), QChar(0x00A4), QChar(0x00A5), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x00A9), QChar(0xFFFD), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x2015),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x00B2), QChar(0x00B3), QChar(0x0384), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x0388), QChar(0x0389), QChar(0x038A), QChar(0x00BB), QChar(0x038C), QChar(0x00BD), QChar(0x038E), QChar(0x038F),  // B8-BF
        QChar(0x0390), QChar(0x0391), QChar(0x0392), QChar(0x0393), QChar(0x0394), QChar(0x0395), QChar(0x0396), QChar(0x0397),  // C0-C7
        QChar(0x0398), QChar(0x0399), QChar(0x039A), QChar(0x039B), QChar(0x039C), QChar(0x039D), QChar(0x039E), QChar(0x039F),  // C8-CF
        QChar(0x03A0), QChar(0x03A1), QChar(0xFFFD), QChar(0x03A3), QChar(0x03A4), QChar(0x03A5), QChar(0x03A6), QChar(0x03A7),  // D0-D7
        QChar(0x03A8), QChar(0x03A9), QChar(0x03AA), QChar(0x03AB), QChar(0x03AC), QChar(0x03AD), QChar(0x03AE), QChar(0x03AF),  // D8-DF
        QChar(0x03B0), QChar(0x03B1), QChar(0x03B2), QChar(0x03B3), QChar(0x03B4), QChar(0x03B5), QChar(0x03B6), QChar(0x03B7),  // E0-E7
        QChar(0x03B8), QChar(0x03B9), QChar(0x03BA), QChar(0x03BB), QChar(0x03BC), QChar(0x03BD), QChar(0x03BE), QChar(0x03BF),  // E8-EF
        QChar(0x03C0), QChar(0x03C1), QChar(0x03C2), QChar(0x03C3), QChar(0x03C4), QChar(0x03C5), QChar(0x03C6), QChar(0x03C7),  // F0=F7
        QChar(0x03C8), QChar(0x03C9), QChar(0x03CA), QChar(0x03CB), QChar(0x03CC), QChar(0x03CD), QChar(0x03CE), QChar(0xFFFD)}))},// F8-FF
    {QStringLiteral("WINDOWS-1254"),
    qMakePair(tr("WINDOWS-1254 (Turkish)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x20AC), QChar(0xFFFD), QChar(0x201A), QChar(0x0192), QChar(0x201E), QChar(0x2026), QChar(0x2020), QChar(0x2021),  // 80-87
        QChar(0x02C6), QChar(0x2030), QChar(0x0160), QChar(0x2039), QChar(0x0152), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 88-8F
        QChar(0xFFFD), QChar(0x2018), QChar(0x2019), QChar(0x201C), QChar(0x201D), QChar(0x2022), QChar(0x2013), QChar(0x2014),  // 90-97
        QChar(0x02DC), QChar(0x2122), QChar(0x0161), QChar(0x203A), QChar(0x0153), QChar(0xFFFD), QChar(0xFFFD), QChar(0x0178),  // 98-9F
        QChar(0x00A0), QChar(0x00A1), QChar(0x00A2), QChar(0x00A3), QChar(0x00A4), QChar(0x00A5), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x00A9), QChar(0x00AA), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x00AF),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x00B2), QChar(0x00B3), QChar(0x00B4), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x00B8), QChar(0x00B9), QChar(0x00BA), QChar(0x00BB), QChar(0x00BC), QChar(0x00BD), QChar(0x00BE), QChar(0x00BF),  // B8-BF
        QChar(0x00C0), QChar(0x00C1), QChar(0x00C2), QChar(0x00C3), QChar(0x00C4), QChar(0x00C5), QChar(0x00C6), QChar(0x00C7),  // C0-C7
        QChar(0x00C8), QChar(0x00C9), QChar(0x00CA), QChar(0x00CB), QChar(0x00CC), QChar(0x00CD), QChar(0x00CE), QChar(0x00CF),  // C8-CF
        QChar(0x011E), QChar(0x00D1), QChar(0x00D2), QChar(0x00D3), QChar(0x00D4), QChar(0x00D5), QChar(0x00D6), QChar(0x00D7),  // D0-D7
        QChar(0x00D8), QChar(0x00D9), QChar(0x00DA), QChar(0x00DB), QChar(0x00DC), QChar(0x0130), QChar(0x015E), QChar(0x00DF),  // D8-DF
        QChar(0x00E0), QChar(0x00E1), QChar(0x00E2), QChar(0x00E3), QChar(0x00E4), QChar(0x00E5), QChar(0x00E6), QChar(0x00E7),  // E0-E7
        QChar(0x00E8), QChar(0x00E9), QChar(0x00EA), QChar(0x00EB), QChar(0x00EC), QChar(0x00ED), QChar(0x00EE), QChar(0x00EF),  // E8-EF
        QChar(0x011F), QChar(0x00F1), QChar(0x00F2), QChar(0x00F3), QChar(0x00F4), QChar(0x00F5), QChar(0x00F6), QChar(0x00F7),  // F0=F7
        QChar(0x00F8), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x0131), QChar(0x015F), QChar(0x00FF)}))},// F8-FF
    {QStringLiteral("WINDOWS-1255"),
    qMakePair(tr("WINDOWS-1255 (Hebrew)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x20AC), QChar(0xFFFD), QChar(0x201A), QChar(0x0192), QChar(0x201E), QChar(0x2026), QChar(0x2020), QChar(0x2021),  // 80-87
        QChar(0x02C6), QChar(0x2030), QChar(0xFFFD), QChar(0x2039), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 88-8F
        QChar(0xFFFD), QChar(0x2018), QChar(0x2019), QChar(0x201C), QChar(0x201D), QChar(0x2022), QChar(0x2013), QChar(0x2014),  // 90-97
        QChar(0x02DC), QChar(0x2122), QChar(0xFFFD), QChar(0x203A), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 98-9F
        QChar(0x00A0), QChar(0x00A1), QChar(0x00A2), QChar(0x00A3), QChar(0x20AA), QChar(0x00A5), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x00A9), QChar(0x00D7), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x00AF),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x00B2), QChar(0x00B3), QChar(0x00B4), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x00B8), QChar(0x00B9), QChar(0x00F7), QChar(0x00BB), QChar(0x00BC), QChar(0x00BD), QChar(0x00BE), QChar(0x00BF),  // B8-BF
        QChar(0x05B0), QChar(0x05B1), QChar(0x05B2), QChar(0x05B3), QChar(0x05B4), QChar(0x05B5), QChar(0x05B6), QChar(0x05B7),  // C0-C7
        QChar(0x05B8), QChar(0x05B9), QChar(0xFFFD), QChar(0x05BB), QChar(0x05BC), QChar(0x05BD), QChar(0x05BE), QChar(0x05BF),  // C8-CF
        QChar(0x05C0), QChar(0x05C1), QChar(0x05C2), QChar(0x05C3), QChar(0x05F0), QChar(0x05F1), QChar(0x05F2), QChar(0x05F3),  // D0-D7
        QChar(0x05F4), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // D8-DF
        QChar(0x05D0), QChar(0x05D1), QChar(0x05D2), QChar(0x05D3), QChar(0x05D4), QChar(0x05D5), QChar(0x05D6), QChar(0x05D7),  // E0-E7
        QChar(0x05D8), QChar(0x05D9), QChar(0x05DA), QChar(0x05DB), QChar(0x05DC), QChar(0x05DD), QChar(0x05DE), QChar(0x05DF),  // E8-EF
        QChar(0x05E0), QChar(0x05E1), QChar(0x05E2), QChar(0x05E3), QChar(0x05E4), QChar(0x05E5), QChar(0x05E6), QChar(0x05E7),  // F0=F7
        QChar(0x05E8), QChar(0x05E9), QChar(0x05EA), QChar(0xFFFD), QChar(0xFFFD), QChar(0x200E), QChar(0x200F), QChar(0xFFFD)}))},// F8-FF
    {QStringLiteral("WINDOWS-1256"),
    qMakePair(tr("WINDOWS-1256 (Arabic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x20AC), QChar(0x067E), QChar(0x201A), QChar(0x0192), QChar(0x201E), QChar(0x2026), QChar(0x2020), QChar(0x2021),  // 80-87
        QChar(0x02C6), QChar(0x2030), QChar(0x0679), QChar(0x2039), QChar(0x0152), QChar(0x0686), QChar(0x0698), QChar(0x0688),  // 88-8F
        QChar(0x06AF), QChar(0x2018), QChar(0x2019), QChar(0x201C), QChar(0x201D), QChar(0x2022), QChar(0x2013), QChar(0x2014),  // 90-97
        QChar(0x06A9), QChar(0x2122), QChar(0x0691), QChar(0x203A), QChar(0x0153), QChar(0x200C), QChar(0x200D), QChar(0x06BA),  // 98-9F
        QChar(0x00A0), QChar(0x060C), QChar(0x00A2), QChar(0x00A3), QChar(0x00A4), QChar(0x00A5), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x00A9), QChar(0x06BE), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x00AF),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x00B2), QChar(0x00B3), QChar(0x00B4), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x00B8), QChar(0x00B9), QChar(0x061B), QChar(0x00BB), QChar(0x00BC), QChar(0x00BD), QChar(0x00BE), QChar(0x061F),  // B8-BF
        QChar(0x06C1), QChar(0x0621), QChar(0x0622), QChar(0x0623), QChar(0x0624), QChar(0x0625), QChar(0x0626), QChar(0x0627),  // C0-C7
        QChar(0x0628), QChar(0x0629), QChar(0x062A), QChar(0x062B), QChar(0x062C), QChar(0x062D), QChar(0x062E), QChar(0x062F),  // C8-CF
        QChar(0x0630), QChar(0x0631), QChar(0x0632), QChar(0x0633), QChar(0x0634), QChar(0x0635), QChar(0x0636), QChar(0x00D7),  // D0-D7
        QChar(0x0637), QChar(0x0638), QChar(0x0639), QChar(0x063A), QChar(0x0640), QChar(0x0641), QChar(0x0642), QChar(0x0643),  // D8-DF
        QChar(0x00E0), QChar(0x0644), QChar(0x00E2), QChar(0x0645), QChar(0x0646), QChar(0x0647), QChar(0x0648), QChar(0x00E7),  // E0-E7
        QChar(0x00E8), QChar(0x00E9), QChar(0x00EA), QChar(0x00EB), QChar(0x0649), QChar(0x064A), QChar(0x00EE), QChar(0x00EF),  // E8-EF
        QChar(0x064B), QChar(0x064C), QChar(0x064D), QChar(0x064E), QChar(0x00F4), QChar(0x064F), QChar(0x0650), QChar(0x00F7),  // F0=F7
        QChar(0x0651), QChar(0x00F9), QChar(0x0652), QChar(0x00FB), QChar(0x00FC), QChar(0x200E), QChar(0x200F), QChar(0x06D2)}))},// F8-FF
    {QStringLiteral("WINDOWS-1257"),
    qMakePair(tr("WINDOWS-1257 (Baltic)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x20AC), QChar(0xFFFD), QChar(0x201A), QChar(0xFFFD), QChar(0x201E), QChar(0x2026), QChar(0x2020), QChar(0x2021),  // 80-87
        QChar(0xFFFD), QChar(0x2030), QChar(0xFFFD), QChar(0x2039), QChar(0xFFFD), QChar(0x00A8), QChar(0x02C7), QChar(0x00B8),  // 88-8F
        QChar(0xFFFD), QChar(0x2018), QChar(0x2019), QChar(0x201C), QChar(0x201D), QChar(0x2022), QChar(0x2013), QChar(0x2014),  // 90-97
        QChar(0xFFFD), QChar(0x2122), QChar(0xFFFD), QChar(0x203A), QChar(0xFFFD), QChar(0x00AF), QChar(0x02DB), QChar(0xFFFD),  // 98-9F
        QChar(0x00A0), QChar(0xFFFD), QChar(0x00A2), QChar(0x00A3), QChar(0x00A4), QChar(0xFFFD), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x00D8), QChar(0x00A9), QChar(0x0156), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x00C6),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x00B2), QChar(0x00B3), QChar(0x00B4), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x00F8), QChar(0x00B9), QChar(0x0157), QChar(0x00BB), QChar(0x00BC), QChar(0x00BD), QChar(0x00BE), QChar(0x00E6),  // B8-BF
        QChar(0x0104), QChar(0x012E), QChar(0x0100), QChar(0x0106), QChar(0x00C4), QChar(0x00C5), QChar(0x0118), QChar(0x0112),  // C0-C7
        QChar(0x010C), QChar(0x00C9), QChar(0x0179), QChar(0x0116), QChar(0x0122), QChar(0x0136), QChar(0x012A), QChar(0x013B),  // C8-CF
        QChar(0x0160), QChar(0x0143), QChar(0x0145), QChar(0x00D3), QChar(0x014C), QChar(0x00D5), QChar(0x00D6), QChar(0x00D7),  // D0-D7
        QChar(0x0172), QChar(0x0141), QChar(0x015A), QChar(0x016A), QChar(0x00DC), QChar(0x017B), QChar(0x017D), QChar(0x00DF),  // D8-DF
        QChar(0x0105), QChar(0x012F), QChar(0x0101), QChar(0x0107), QChar(0x00E4), QChar(0x00E5), QChar(0x0119), QChar(0x0113),  // E0-E7
        QChar(0x010D), QChar(0x00E9), QChar(0x017A), QChar(0x0117), QChar(0x0123), QChar(0x0137), QChar(0x012B), QChar(0x013C),  // E8-EF
        QChar(0x0161), QChar(0x0144), QChar(0x0146), QChar(0x00F3), QChar(0x014D), QChar(0x00F5), QChar(0x00F6), QChar(0x00F7),  // F0=F7
        QChar(0x0173), QChar(0x0142), QChar(0x015B), QChar(0x016B), QChar(0x00FC), QChar(0x017C), QChar(0x017E), QChar(0x02D9)}))},// F8-FF
    {QStringLiteral("WINDOWS-1258"),
    qMakePair(tr("WINDOWS-1258 (Vietnamese)", "Keep the English translation intact, so if a user accidentally changes to a language they don't understand, they can change back e.g. ISO 8859-2 (Центральная Европа/Central European)"), QVector<QChar>(
        //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
       {QChar(0x20AC), QChar(0xFFFD), QChar(0x201A), QChar(0x0192), QChar(0x201E), QChar(0x2026), QChar(0x2020), QChar(0x2021),  // 80-87
        QChar(0x02C6), QChar(0x2030), QChar(0xFFFD), QChar(0x2039), QChar(0x0152), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 88-8F
        QChar(0xFFFD), QChar(0x2018), QChar(0x2019), QChar(0x201C), QChar(0x201D), QChar(0x2022), QChar(0x2013), QChar(0x2014),  // 90-97
        QChar(0x02DC), QChar(0x2122), QChar(0xFFFD), QChar(0x203A), QChar(0x0153), QChar(0xFFFD), QChar(0xFFFD), QChar(0x0178),  // 98-9F
        QChar(0x00A0), QChar(0x00A1), QChar(0x00A2), QChar(0x00A3), QChar(0x00A4), QChar(0x00A5), QChar(0x00A6), QChar(0x00A7),  // A0-A7
        QChar(0x00A8), QChar(0x00A9), QChar(0x00AA), QChar(0x00AB), QChar(0x00AC), QChar(0x00AD), QChar(0x00AE), QChar(0x00AF),  // A8-AF
        QChar(0x00B0), QChar(0x00B1), QChar(0x00B2), QChar(0x00B3), QChar(0x00B4), QChar(0x00B5), QChar(0x00B6), QChar(0x00B7),  // B0-B7
        QChar(0x00B8), QChar(0x00B9), QChar(0x00BA), QChar(0x00BB), QChar(0x00BC), QChar(0x00BD), QChar(0x00BE), QChar(0x00BF),  // B8-BF
        QChar(0x00C0), QChar(0x00C1), QChar(0x00C2), QChar(0x0102), QChar(0x00C4), QChar(0x00C5), QChar(0x00C6), QChar(0x00C7),  // C0-C7
        QChar(0x00C8), QChar(0x00C9), QChar(0x00CA), QChar(0x00CB), QChar(0x0300), QChar(0x00CD), QChar(0x00CE), QChar(0x00CF),  // C8-CF
        QChar(0x0110), QChar(0x00D1), QChar(0x0309), QChar(0x00D3), QChar(0x00D4), QChar(0x01A0), QChar(0x00D6), QChar(0x00D7),  // D0-D7
        QChar(0x00D8), QChar(0x00D9), QChar(0x00DA), QChar(0x00DB), QChar(0x00DC), QChar(0x01AF), QChar(0x0303), QChar(0x00DF),  // D8-DF
        QChar(0x00E0), QChar(0x00E1), QChar(0x00E2), QChar(0x0103), QChar(0x00E4), QChar(0x00E5), QChar(0x00E6), QChar(0x00E7),  // E0-E7
        QChar(0x00E8), QChar(0x00E9), QChar(0x00EA), QChar(0x00EB), QChar(0x0301), QChar(0x00ED), QChar(0x00EE), QChar(0x00EF),  // E8-EF
        QChar(0x0111), QChar(0x00F1), QChar(0x0323), QChar(0x00F3), QChar(0x00F4), QChar(0x01A1), QChar(0x00F6), QChar(0x00F7),  // F0=F7
        QChar(0x00F8), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x01B0), QChar(0x20AB), QChar(0x00FF)}))}// F8-FF
};
// clang-format on

// a map of supported MXP elements and attributes
const QMap<QString, QVector<QString>> TBuffer::mSupportedMxpElements = {
    {QStringLiteral("send"), {"href", "hint", "prompt"}},
    {QStringLiteral("br"), {}}
};

TChar::TChar()
{
    fgR = 255;
    fgG = 255;
    fgB = 255;
    bgR = 0;
    bgG = 0;
    bgB = 0;
    flags = 0;
    link = 0;
}

TChar::TChar(int fR, int fG, int fB, int bR, int bG, int bB, bool b, bool i, bool u, bool s, int _link) : fgR(fR), fgG(fG), fgB(fB), bgR(bR), bgG(bG), bgB(bB), link(_link)
{
    flags = 0;
    if (i) {
        flags |= TCHAR_ITALICS;
    }
    if (b) {
        flags |= TCHAR_BOLD;
    }
    if (u) {
        flags |= TCHAR_UNDERLINE;
    }
    if (s) {
        flags |= TCHAR_STRIKEOUT;
    }
}

TChar::TChar(Host* pH)
{
    if (pH) {
        fgR = pH->mFgColor.red();
        fgG = pH->mFgColor.green();
        fgB = pH->mFgColor.blue();
        bgR = pH->mBgColor.red();
        bgG = pH->mBgColor.green();
        bgB = pH->mBgColor.blue();
    } else {
        fgR = 255;
        fgG = 255;
        fgB = 255;
        bgR = 0;
        bgG = 0;
        bgB = 0;
    }
    flags = 0;
    link = 0;
}

bool TChar::operator==(const TChar& c)
{
    if (fgR != c.fgR) {
        return false;
    }
    if (fgG != c.fgG) {
        return false;
    }
    if (fgB != c.fgB) {
        return false;
    }
    if (bgR != c.bgR) {
        return false;
    }
    if (bgG != c.bgG) {
        return false;
    }
    if (bgB != c.bgB) {
        return false;
    }
    if (flags != c.flags) {
        return false;
    }
    if (link != c.link) {
        return false;
    }
    return true;
}

TChar::TChar(const TChar& copy)
{
    fgR = copy.fgR;
    fgG = copy.fgG;
    fgB = copy.fgB;
    bgR = copy.bgR;
    bgG = copy.bgG;
    bgB = copy.bgB;
    flags = copy.flags;
    link = copy.link;
    flags &= ~(TCHAR_INVERSE); //for some reason we always clear the inverse, is this a bug?
}


TBuffer::TBuffer(Host* pH)
: mLinkID(0)
, mLinesLimit(10000)
, mBatchDeleteSize(1000)
, mUntriggered(0)
, mWrapAt(99999999)
, mWrapIndent(0)
, mCursorY(0)
, mMXP(false)
, mAssemblingToken(false)
, currentToken("")
, openT(0)
, closeT(0)
, mMXP_LINK_MODE(false)
, mIgnoreTag(false)
, mSkip("")
, mParsingVar(false)
, mMXP_SEND_NO_REF_MODE(false)
, mGotESC(false)
, mGotCSI(false)
, mGotOSC(false)
, mBlack(pH->mBlack)
, mLightBlack(pH->mLightBlack)
, mRed(pH->mRed)
, mLightRed(pH->mLightRed)
, mLightGreen(pH->mLightGreen)
, mGreen(pH->mGreen)
, mLightBlue(pH->mLightBlue)
, mBlue(pH->mBlue)
, mLightYellow(pH->mLightYellow)
, mYellow(pH->mYellow)
, mLightCyan(pH->mLightCyan)
, mCyan(pH->mCyan)
, mLightMagenta(pH->mLightMagenta)
, mMagenta(pH->mMagenta)
, mLightWhite(pH->mLightWhite)
, mWhite(pH->mWhite)
, mpHost(pH)
, mCursorMoved(false)
, mBold(false)
, mItalics(false)
, mUnderline(false)
, mStrikeOut(false)
, mOpenMainQuote()
, mEchoText()
, mIsDefaultColor()
, isUserScrollBack()
, maxx()
, maxy()
, hadLF()
, lastLoggedFromLine(0)
, lastloggedToLine(0)
, mEncoding()
, mMainIncomingCodec(nullptr)
{
    clear();
    newLines = 0;
    mLastLine = 0;
    updateColors();

    TMxpElement _element;
    _element.name = "SEND";
    _element.href = "";
    _element.hint = "";
    mMXP_Elements["SEND"] = _element;

    // Validate the encoding tables in case there has been an edit which breaks
    // things:
    for (auto table : csmEncodingTable) {
        Q_ASSERT_X(table.second.size() == 128, "TBuffer", "Mis-sized encoding look-up table.");
    }
}

void TBuffer::setBufferSize(int s, int batch)
{
    if (s < 100) {
        s = 100;
    }
    if (batch >= s) {
        batch = s / 10;
    }
    mLinesLimit = s;
    mBatchDeleteSize = batch;
}

void TBuffer::updateColors()
{
    Host* pH = mpHost;
    if (!pH) {
        qWarning() << "TBuffer::updateColors() ERROR - Called when mpHost pointer is nullptr";
        return;
    }

    mBlack = pH->mBlack;
    mBlackR = mBlack.red();
    mBlackG = mBlack.green();
    mBlackB = mBlack.blue();
    mLightBlack = pH->mLightBlack;
    mLightBlackR = mLightBlack.red();
    mLightBlackG = mLightBlack.green();
    mLightBlackB = mLightBlack.blue();
    mRed = pH->mRed;
    mRedR = mRed.red();
    mRedG = mRed.green();
    mRedB = mRed.blue();
    mLightRed = pH->mLightRed;
    mLightRedR = mLightRed.red();
    mLightRedG = mLightRed.green();
    mLightRedB = mLightRed.blue();
    mLightGreen = pH->mLightGreen;
    mLightGreenR = mLightGreen.red();
    mLightGreenG = mLightGreen.green();
    mLightGreenB = mLightGreen.blue();
    mGreen = pH->mGreen;
    mGreenR = mGreen.red();
    mGreenG = mGreen.green();
    mGreenB = mGreen.blue();
    mLightBlue = pH->mLightBlue;
    mLightBlueR = mLightBlue.red();
    mLightBlueG = mLightBlue.green();
    mLightBlueB = mLightBlue.blue();
    mBlue = pH->mBlue;
    mBlueR = mBlue.red();
    mBlueG = mBlue.green();
    mBlueB = mBlue.blue();
    mLightYellow = pH->mLightYellow;
    mLightYellowR = mLightYellow.red();
    mLightYellowG = mLightYellow.green();
    mLightYellowB = mLightYellow.blue();
    mYellow = pH->mYellow;
    mYellowR = mYellow.red();
    mYellowG = mYellow.green();
    mYellowB = mYellow.blue();
    mLightCyan = pH->mLightCyan;
    mLightCyanR = mLightCyan.red();
    mLightCyanG = mLightCyan.green();
    mLightCyanB = mLightCyan.blue();
    mCyan = pH->mCyan;
    mCyanR = mCyan.red();
    mCyanG = mCyan.green();
    mCyanB = mCyan.blue();
    mLightMagenta = pH->mLightMagenta;
    mLightMagentaR = mLightMagenta.red();
    mLightMagentaG = mLightMagenta.green();
    mLightMagentaB = mLightMagenta.blue();
    mMagenta = pH->mMagenta;
    mMagentaR = mMagenta.red();
    mMagentaG = mMagenta.green();
    mMagentaB = mMagenta.blue();
    mLightWhite = pH->mLightWhite;
    mLightWhiteR = mLightWhite.red();
    mLightWhiteG = mLightWhite.green();
    mLightWhiteB = mLightWhite.blue();
    mWhite = pH->mWhite;
    mWhiteR = mWhite.red();
    mWhiteG = mWhite.green();
    mWhiteB = mWhite.blue();
    mCurrentFgColorR = pH->mFgColor.red();
    mCurrentFgColorG = pH->mFgColor.green();
    mCurrentFgColorB = pH->mFgColor.blue();
    mCurrentFgColorLightR = mCurrentFgColorR;
    mCurrentFgColorLightG = mCurrentFgColorG;
    mCurrentFgColorLightB = mCurrentFgColorB;
    mCurrentBgColorR = pH->mBgColor.red();
    mCurrentBgColorG = pH->mBgColor.green();
    mCurrentBgColorB = pH->mBgColor.blue();
}

QPoint TBuffer::getEndPos()
{
    int x = 0;
    int y = 0;
    if (!buffer.empty()) {
        y = buffer.size() - 1;
        if (!buffer.at(y).empty()) {
            x = buffer.at(y).size() - 1;
        }
    }
    QPoint P_end(x, y);
    return P_end;
}

int TBuffer::getLastLineNumber()
{
    if (static_cast<int>(buffer.size()) > 0) {
        return static_cast<int>(buffer.size()) - 1;
    } else {
        return 0; //-1;
    }
}

void TBuffer::addLink(bool trigMode, const QString& text, QStringList& command, QStringList& hint, TChar format)
{
    mLinkID++;
    if (mLinkID > 1000) {
        mLinkID = 1;
    }
    mLinkStore[mLinkID] = command;
    mHintStore[mLinkID] = hint;
    if (!trigMode) {
        append(text,
               0,
               text.length(),
               format.fgR,
               format.fgG,
               format.fgB,
               format.bgR,
               format.bgG,
               format.bgB,
               format.flags & TCHAR_BOLD,
               format.flags & TCHAR_ITALICS,
               format.flags & TCHAR_UNDERLINE,
               format.flags & TCHAR_STRIKEOUT,
               mLinkID);
    } else {
        appendLine(text,
                   0,
                   text.length(),
                   format.fgR,
                   format.fgG,
                   format.fgB,
                   format.bgR,
                   format.bgG,
                   format.bgB,
                   format.flags & TCHAR_BOLD,
                   format.flags & TCHAR_ITALICS,
                   format.flags & TCHAR_UNDERLINE,
                   format.flags & TCHAR_STRIKEOUT,
                   mLinkID);
    }
}

//void TBuffer::appendLink( QString & text,
//                          int sub_start,
//                          int sub_end,
//                          int fgColorR,
//                          int fgColorG,
//                          int fgColorB,
//                          int bgColorR,
//                          int bgColorG,
//                          int bgColorB,
//                          bool bold,
//                          bool italics,
//                          bool underline )
//{
//    if( static_cast<int>(buffer.size()) > mLinesLimit )
//    {
//        shrinkBuffer();
//    }
//    int last = buffer.size()-1;
//    if( last < 0 )
//    {
//        std::deque<TChar> newLine;
//        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline);
//        newLine.push_back( c );
//        buffer.push_back( newLine );
//        lineBuffer.push_back(QString());
//        promptBuffer.push_back(false);
//        timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
//        dirty.push_back(true);
//        last = 0;
//    }
//    bool firstChar = (lineBuffer.back().size() == 0);
//    int length = text.size();
//    if( length < 1 ) return;
//    if( sub_end >= length ) sub_end = text.size()-1;

//    for( int i=sub_start; i<=(sub_start+sub_end); i++ )
//    {
//        if( text.at(i) == '\n' )
//        {
//            std::deque<TChar> newLine;
//            buffer.push_back( newLine );
//            lineBuffer.push_back( QString() );
//            QString time = "-------------";
//            timeBuffer << time;
//            promptBuffer << false;
//            dirty << true;
//            mLastLine++;
//            newLines++;
//            firstChar = true;
//            continue;
//        }
//        if( lineBuffer.back().size() >= mWrapAt )
//        {
//            //assert(lineBuffer.back().size()==buffer.back().size());
//            const QString lineBreaks = ",.- ";
//            const QString nothing = "";
//            for( int i=lineBuffer.back().size()-1; i>=0; i-- )
//            {
//                if( lineBreaks.indexOf( lineBuffer.back().at(i) ) > -1 )
//                {
//                    QString tmp = lineBuffer.back().mid(0,i+1);
//                    QString lineRest = lineBuffer.back().mid(i+1);
//                    lineBuffer.back() = tmp;
//                    std::deque<TChar> newLine;

//                    int k = lineRest.size();
//                    if( k > 0 )
//                    {
//                        while( k > 0 )
//                        {
//                            newLine.push_front(buffer.back().back());
//                            buffer.back().pop_back();
//                            k--;
//                        }
//                    }

//                    buffer.push_back( newLine );
//                    if( lineRest.size() > 0 )
//                        lineBuffer.append( lineRest );
//                    else
//                        lineBuffer.append( nothing );
//                    QString time = "-------------";
//                    timeBuffer << time;
//                    promptBuffer << false;
//                    dirty << true;
//                    mLastLine++;
//                    newLines++;
//                    break;
//                }
//            }
//        }
//        lineBuffer.back().append( text.at( i ) );

//        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline, mLinkID );
//        buffer.back().push_back( c );
//        if( firstChar )
//        {
//            timeBuffer.back() = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
//        }
//    }
//}



/* ANSI color codes: sequence = "ESCAPE + [ code_1; ... ; code_n m"
      -----------------------------------------
      0 reset
      1 intensity bold on
      2 intensity faint on
      3 italics on
      4 underline on
      5 blink on slow
      6 blink on fast
      7 inverse on
      9 strikethrough on
      10 ? TODO
      22 intensity normal (not bold, not faint)
      23 italics off
      24 underline off
      25 blink off
      26 RESERVED (for proportional spacing)
      27 inverse off
      29 strikethrough off
      30 fg black
      31 fg red
      32 fg green
      33 fg yellow
      34 fg blue
      35 fg magenta
      36 fg cyan
      37 fg white
      39 fg default
      40 bg black
      41 bg red
      42 bg green
      43 bg yellow
      44 bg blue
      45 bg magenta
      46 bg cyan
      47 bg white
      49 bg default
      50 RESERVED (for proportional spacing)
      51 framed on
      52 encircled on
      53 overlined on
      54 framed / encircled off
      55 overlined off

      Notes for code 38/48:
      38:0 implementation defined (48:0 is NOT allowed)

      38:1 transparent foreground
      48:1 transparent background

      sequences for 24(32 for '4')-bit Color support:
      38:2:???:0-255:0-255:0-255:XXX:0-255:0-1 (direct) RGB space foreground color
      48:2:???:0-255:0-255:0-255:XXX:0-255:0-1 (direct) RGB space background color
      38:3:???:0-255:0-255:0-255:XXX:0-255:0-1 (direct) CMY space foreground color
      48:3:???:0-255:0-255:0-255:XXX:0-255:0-1 (direct) CMY space background color
      38:4:???:0-255:0-255:0-255:0-255:0-255:0-1 (direct) CMYK space foreground color
      48:4:???:0-255:0-255:0-255:0-255:0-255:0-1 (direct) CMYK space background color
      The third parameter is the color space id but this is expected to be the
      "default" value which is an empty string. The seventh parameter may be used
      to specify a tolerance value (an integer) and parameter eight may be used
      to specify a colour space associated with the tolerance (0 for CIELUV,
      1 for CIELAB).

      sequences for (indexed) 256 Color support:
      38:5:0-256 (indexed) foreground color
      48:5:0-256 (indexed) background color:
          0x00-0x07:   0 -   7 standard colors (as in ESC [ 30–37 m)
          0x08-0x0F:   8 -  15 high intensity colors (as in ESC [ 90–97 m)
          0x10-0xE7:  16 - 231 6 × 6 × 6 = 216 colors: 16 + 36 × r + 6 × g + b (0 ≤ r, g, b ≤ 5)
          0xE8-0xFF: 232 - 255 grayscale from black to white in 24 steps

      Also note that for the 38 and 48 codes the parameter elements SHOULD be
      separated by ':' but some interpretations erroneously use ';'.  Also
      "empty" parameter elements represent a default value and that empty
      elements at the end can be omitted.
 */

void TBuffer::translateToPlainText(std::string& incoming, const bool isFromServer)
{
    // What can appear in a CSI Parameter String (Ps) byte or at least for it
    // to be something we can handle:
    const QByteArray cParameter("0123456789;:");
    // What can appear in the initial position of a CSI Parameter String (Ps) byte:
    const QByteArray cParameterInitial("0123456789;:<=>?");
    // What can appear in a CSI Intermediate byte (includes a quote character in
    // the middle of the text here which has to be escaped with a backslash):
    const QByteArray cIntermediate(" !\"#$%&'()*+,-./");
    // What can appear in a CSI final byte position - (includes a backslash
    // which has to be doubled to include it in here):
    const QByteArray cFinal("@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");

    // As well as enabling the prepending of left-over bytes from last packet
    // from the MUD server this may help in high frequency interactions to
    // protect this process from the supplied string being modified
    // asynchronously by the QNetwork code that runs in another thread:
    std::string localBuffer;

    Host* pHost = mpHost;
    if (!pHost) {
        qWarning() << "TBuffer::translateToPlainText(...) ERROR: Cannot access Host instance at this time - data has been lost.";
        return; // We really have a problem
    }

    // Check this each packet
    QString usedEncoding = mpHost->mTelnet.getEncoding();
    if (mEncoding != usedEncoding) {
        encodingChanged(usedEncoding);
    }

// clang-format off
    if (isFromServer
        && !mIncompleteSequenceBytes.empty()
        && (mEncoding == QLatin1String("UTF-8") || mEncoding == QLatin1String("GBK") || mEncoding == QLatin1String("GB18030"))) {

// clang-format on
#if defined(DEBUG_UTF8_PROCESSING) || defined(DEBUG_GB_PROCESSING)
        qDebug() << "TBuffer::translateToPlainText(...) Prepending residual UTF-8 bytes onto incoming data!";
#endif
        localBuffer = mIncompleteSequenceBytes + incoming;
        mIncompleteSequenceBytes.clear();
    } else {
        localBuffer = incoming;
    }

    const QVector<QChar> encodingLookupTable = csmEncodingTable.value(mEncoding).second;
    // If the encoding is "ASCII", "ISO 8859-1", "UTF-8", "GBK" or "GB18030"
    // (which are not in the table) encodingLookupTable will be empty otherwise
    // the 128 values in the returned table will be used for all the text data
    // that gets through the following ANSI code and other out-of-band data
    // processing - doing this mean that a (fast) lookup in the QVector can be
    // done as opposed to a a repeated switch(...) and branch to one of a series
    // of decoding methods each with another up to 128 value switch()

    mUntriggered = lineBuffer.size() - 1;
    size_t localBufferLength = localBuffer.length();
    size_t localBufferPosition = 0;
    if (!localBufferLength) {
        return;
    }

    while (true) {
        if (localBufferPosition >= localBufferLength) {
            return;
        }

        char& ch = localBuffer[localBufferPosition];
        if (ch == '\033') {
            if (!mGotOSC) {
                // The terminator for an OSC is the String Terminator but that
                // is the ESC character followed by (the single character)
                // '\\' so must not respond to an ESC here - though the code
                // arrangement should avoid looping around this loop whilst
                // seeking this character pair anyhow...
                mGotESC = true;
                ++localBufferPosition;
                continue;
            }
        }

        if (mGotESC && (ch == '[' || ch == ']')) {
            mGotESC = false;
            mGotCSI = (ch == '[');
            mGotOSC = (ch == ']');
            ++localBufferPosition;
            continue;
        }

        if (mGotCSI) {
            // Lookahead and try and see what we are processing
            // At the start of a CSI sequence the only valid character is one of:
            // "0-9:;<=>?" if it is one of "0-9:;" then it is a
            // "parameter-string" ELSE if it is one of '<', '=', '>' or '?' it
            // IS a private/experimental and not covered by the ECMA-48
            // specifications..
            // After the first character the remaining characters of the
            // parameter string will be in the range "0-9:;" only
            size_t spanStart = localBufferPosition;
            size_t spanEnd = spanStart;
            while (spanEnd < localBufferLength
                   && (((spanStart < spanEnd) && cParameterInitial.indexOf(localBuffer[spanEnd]) >= 0)
                      ||(spanStart == spanEnd) && cParameter.indexOf(localBuffer[spanEnd]) >= 0)) {
                ++spanEnd;
            }

            // Test whether the first byte is within the usable subset of the
            // allowed value - or not:
            if (cParameter.indexOf(localBuffer[spanStart]) == -1) {
                // Oh dear, the CSI parameter string sequence begins with one of
                // the reserved characters ('<', '=', '>' or '?') which we
                // can/do not handle

                qDebug().noquote().nospace() << "TBuffer::translateToPlainText(...) INFO - detected a private/reserved CSI sequence beginning with \"CSI" << localBuffer.substr(spanStart, spanEnd - spanStart).c_str() << "\" which Mudlet cannot interpret.";
                // So skip over it as far as we can - will still possible have
                // garbage beyond the end which will still be shown...
                localBufferPosition += 1 + spanEnd - spanStart;
                mGotCSI = false;
                // Go around while loop again:
                continue;
            }

            if (cParameter.indexOf(localBuffer[spanEnd]) >=0) {
                // The last character in the buffer is still within a CSI
                // sequence - therefore we have probably got a split between
                // data packets and are not in a position to process the
                // current line further...

                mIncompleteSequenceBytes = localBuffer.substr(spanStart);
                return;
            }

            // Now we can take a peak at what the next character is, it could
            // be an optional (and we are not expecting this) "intermediate
            // byte" which is space or one of "!"#$%&'()*+,-./" or a "final
            // byte" which is what determines what on earth the CSI is for, it
            // should be in the (ASCII) range '@' to '~' and the end of that
            // range 'p' to '~' is for "private" or "experimental" use.

            if (cIntermediate.indexOf(localBuffer[spanEnd]) >= 0) {
                // We do not handle any sequences with intermediate bytes
                // Report it and then ignore it, try and find out what the byte
                // afterwards is as it might help to debug things
                if (spanEnd + 1 < localBufferLength) {
                    // Yeah there is another byte we can report as the final byte
                    qDebug().noquote().nospace() << "TBuffer::translateToPlainText(...) INFO - detected a CSI sequence with an 'intermediate' byte ('" << localBuffer[spanEnd] << "') and a 'final' byte ('" << localBuffer[spanEnd+1] << "') which Mudlet cannot interpret.";
                } else {
                    qDebug().noquote().nospace() << "TBuffer::translateToPlainText(...) INFO - detected a CSI sequence with an 'intermediate' byte ('" << localBuffer[spanEnd] << "') which Mudlet cannot interpret.";
                }
                // So skip over it as far as we can - will still be possible to
                // have garbage beyond the end which will still be shown...
                localBufferPosition += 1 + spanEnd - spanStart;
                mGotCSI = false;
                // Go around while loop again:
                continue;
            }

            const quint8 modeChar = static_cast<unsigned char>(localBuffer[spanEnd]);
            if (cFinal.indexOf(localBuffer[spanEnd]) >= 0) {
                // We have a valid CSI sequence - but is it one we handle?
                // We currently only handle the 'm' for SGR and the 'z' for
                // Zuggsoft's MXP protocol:
                switch (modeChar) {
                case static_cast<quint8>('m'):
                    // We have a complete SGR sequence:
#if defined(DEBUG_SGR_PROCESSING)
                    qDebug().nospace().noquote() << "    Consider the SGR sequence: \"" << localBuffer.substr(localBufferPosition, spanEnd - spanStart).c_str() << "\"";
#endif
                    decodeSGR(QString(localBuffer.substr(localBufferPosition, spanEnd - spanStart).c_str()));
                    break;

                case static_cast<quint8>('z'):
                    // We have a control sequence for MXP
                    qDebug().nospace().noquote() << "    Consider the MXP control sequence: \"" << localBuffer.substr(localBufferPosition, spanEnd - spanStart).c_str() << "\"";
                    if (!mpHost->mFORCE_MXP_NEGOTIATION_OFF) {
                        mGotCSI = false;

                        bool isOk = false;
                        QString code = QString(localBuffer.substr(localBufferPosition, spanEnd - spanStart).c_str());
                        if (isOk) {
                            // we really do not handle these well...

                            // The wrong layout and odd code here (rather than
                            // using a switch () with an integer) is to enable
                            // the same code in this area as was previously use
                            // so that the diff for the git commit is two
                            // smaller chunks that are slightly easier to read.
                            // clang-format off

                    // MXP line modes

                    // locked mode
                    if (code == "7" || code == "2") {
                        mMXP = false;
                    }
                    // secure mode
                    if (code == "1" || code == "6" || code == "4") {
                        mMXP = true;
                    }
                    // reset
                    if (code == "3") {
                        closeT = 0;
                        openT = 0;
                        mAssemblingToken = false;
                        currentToken.clear();
                        mParsingVar = false;
                    }

                    // 0 and 5 are not even handled in current code
                    if (code == "0" || code == "5" || code.toInt() > 7) {
                        qDebug().noquote().nospace() << "TBuffer::translateToPlainText(...) INFO - Unhandled MXP control sequence CSI " << code << " z received, Mudlet will ignore it.";
                    }

                    // clang-format on
                    // Code above here is deliberately misaligned
                        } else {
                            // isOk is false here as toInt(...) failed
                            qDebug().noquote().nospace() << "TBuffer::translateToPlainText(...) INFO - Non-mumeric MXP control sequence CSI " << code << " z received, Mudlet will ignore it.";
                        }

                    }
                    // end of if (!mpHost->mFORCE_MXP_NEGOTIATION_OFF)
                    // We have manually disabled MXP negotiation
                    mCode.clear();
                    break;

                default: // Unhandled other (valid) CSI final byte sequences will end up here
                    qDebug().noquote().nospace() << "TBuffer::translateToPlainText(...) INFO - Unhandled sequence of form CSI..." << localBuffer[spanEnd] << " received, Mudlet will ignore it.";

                } // End of switch(modeChar) {}
            } else {
                qDebug().noquote().nospace() << "TBuffer::translateToPlainText(...) INFO - detected an invalid CSI sequence beginning with \"CSI" << localBuffer.substr(spanStart, spanEnd - spanStart).c_str() << " which Mudlet will ignore.";
            }  // End of (isAValidFinalByte) {}

            mGotCSI = false;
            localBufferPosition += 1 + spanEnd - spanStart;
            // Go around while loop again:
            continue;

        } // End of if (mGotCSI)

        if (mGotOSC) {
            // Lookahead and find end of sequence (the ST string terminator)
            // DANGER, WILL ROBINSON! Should an OSC be recieved without a
            // terminator then all data will just be swallowed into the buffer

            // Valid characters inside an OSC are: a "command string" or a
            // "character string".
            // A "command string" is a sequence of bit combinations in the range
            // <BS><TAB><LF><VT><FF><CR> and ASCII printables from Space to '~'
            // A "character string" is a sequence of any character except Start
            // of String (SOS) or String Terminator (ST) and the latter is ESC
            // followed by '\\' (a single \ BTW) in the 7-bit code case (the
            // former is encoded as ESC followed by 'X'):
            size_t spanStart = localBufferPosition;
            size_t spanEnd = spanStart;
            // It is safe to look at spanEnd-1 even at the starting position
            // because we already know that the localBuffer extends backwards
            // that far (it will be the ']' character!)
            while (spanEnd < localBufferLength
                   && (localBuffer[spanEnd-1] != '\033')
                   && (localBuffer[spanEnd] != '\\')) {
                ++spanEnd;
            }

            if (localBuffer[spanEnd] != '\\') {
                // The last character in the buffer is NOT the expected ST
                // - therefore we have probably got a split between
                // data packets and are not in a position to process the
                // current line further...

                mIncompleteSequenceBytes = localBuffer.substr(spanStart);
                return;
            }

            decodeOSC(QString(localBuffer.substr(localBufferPosition, spanEnd - spanStart - 1).c_str()));
            mGotOSC = false;
            localBufferPosition += 1 + spanEnd - spanStart;
            // Go around while loop again:
            continue;
        }

        // We are outside of a CSI or OSC sequence if we get to here:

        if (mMXP) {

            // ignore < and > inside of parameter strings
            if (openT == 1) {
                if (ch == '\'' || ch == '\"') {
                    if (!mParsingVar) {
                        mOpenMainQuote = ch;
                        mParsingVar = true;
                    } else {
                        if (ch == mOpenMainQuote) {
                            mParsingVar = false;
                        }
                    }
                }
            }

            if (ch == '<') {
                if (!mParsingVar) {
                    openT++;
                    if (!currentToken.empty()) {
                        currentToken += ch;
                    }
                    mAssemblingToken = true;
                    ++localBufferPosition;
                    continue;
                }
            }

            if (ch == '>') {
                if (!mParsingVar) {
                    closeT++;
                }

                // sanity check
                if (closeT > openT) {
                    closeT = 0;
                    openT = 0;
                    mAssemblingToken = false;
                    currentToken.clear();
                    mParsingVar = false;
                }

                if ((openT > 0) && (closeT == openT)) {
                    mAssemblingToken = false;
                    std::string::size_type _pfs = currentToken.find_first_of(' ');
                    QString _tn;
                    if (_pfs == std::string::npos) {
                        _tn = currentToken.c_str();
                    } else {
                        _tn = currentToken.substr(0, _pfs).c_str();
                    }
                    _tn = _tn.toUpper();
                    if (_tn == "VERSION") {
                        QString payload = QStringLiteral("\n\x1b[1z<VERSION MXP=1.0 CLIENT=Mudlet VERSION=%1%2>\n").arg(APP_VERSION, APP_BUILD);
                        mpHost->mTelnet.sendData(payload);
                    } else if (_tn == QLatin1String("SUPPORT")) {
                        auto response = processSupportsRequest(currentToken.c_str());
                        QString payload = QStringLiteral("\n\x1b[1z<SUPPORTS %1>\n").arg(response);
                        mpHost->mTelnet.sendData(payload);
                    }
                    if (_tn == "BR") {
                        /*
                         * FIXME: The Zuggsoft MXP specification states:
                         * "<BR> = Line break.  Forces a line break inside or
                         * outside of a paragraph.  Note that <BR> is NOT parsed
                         * as a newline from the MUD as far as mode changes are
                         * concerned."
                         * This does not appear to be how it is done here...!
                         */
                        ch = '\n';
                        openT = 0;
                        closeT = 0;
                        currentToken.clear();
                        goto COMMIT_LINE;
                    }
                    if (_tn.startsWith("!EL")) {
                        QString _tp = currentToken.substr(currentToken.find_first_of(' ')).c_str();
                        _tn = _tp.section(' ', 1, 1).toUpper();
                        _tp = _tp.section(' ', 2).toUpper();
                        if ((_tp.indexOf("SEND") != -1)) {
                            QString _t2 = _tp;
                            int pRef = _t2.indexOf("HREF=");
                            bool _got_ref = false;
                            // wenn kein href angegeben ist, dann gilt das 1. parameter als href
                            if (pRef == -1) {
                                pRef = _t2.indexOf("SEND ");
                            } else {
                                _got_ref = true;
                            }

                            if (pRef == -1) {
                                return;
                            }
                            pRef += 5;

                            QChar _quote_type = _t2[pRef];
                            int pRef2;
                            if (_quote_type != '&') {
                                pRef2 = _t2.indexOf(_quote_type, pRef + 1); //' ', pRef );
                            } else {
                                pRef2 = _t2.indexOf(' ', pRef + 1);
                            }
                            QString _ref = _t2.mid(pRef, pRef2 - pRef);

                            // gegencheck, ob es keine andere variable ist

                            if (_ref.startsWith('\'')) {
                                int pRef3 = _t2.indexOf('\'', _t2.indexOf('\'', pRef) + 1);
                                int pRef4 = _t2.indexOf('=');
                                if (((pRef4 == -1) || (pRef4 != 0 && pRef4 > pRef3)) || (_got_ref)) {
                                    _ref = _t2.mid(pRef, pRef2 - pRef);
                                }
                            } else if (_ref.startsWith('\"')) {
                                int pRef3 = _t2.indexOf('\"', _t2.indexOf('\"', pRef) + 1);
                                int pRef4 = _t2.indexOf('=');
                                if (((pRef4 == -1) || (pRef4 != 0 && pRef4 > pRef3)) || (_got_ref)) {
                                    _ref = _t2.mid(pRef, pRef2 - pRef);
                                }
                            } else if (_ref.startsWith('&')) {
                                _ref = _t2.mid(pRef, _t2.indexOf(' ', pRef + 1) - pRef);
                            } else {
                                _ref = "";
                            }
                            _ref = _ref.replace(';', "");
                            _ref = _ref.replace("&quot", "");
                            _ref = _ref.replace("&amp", "&");
                            _ref = _ref.replace('\'', ""); //NEU
                            _ref = _ref.replace('\"', ""); //NEU
                            _ref = _ref.replace("&#34", R"(")");

                            pRef = _t2.indexOf("HINT=");
                            QString _hint;
                            if (pRef != -1) {
                                pRef += 5;
                                int pRef2 = _t2.indexOf(' ', pRef);
                                _hint = _t2.mid(pRef, pRef2 - pRef);
                                if (_hint.startsWith('\'') || pRef2 < 0) {
                                    pRef2 = _t2.indexOf('\'', _t2.indexOf('\'', pRef) + 1);
                                    _hint = _t2.mid(pRef, pRef2 - pRef);
                                } else if (_hint.startsWith('\"') || pRef2 < 0) {
                                    pRef2 = _t2.indexOf('\"', _t2.indexOf('\"', pRef) + 1);
                                    _hint = _t2.mid(pRef, pRef2 - pRef);
                                }
                                _hint = _hint.replace(';', "");
                                _hint = _hint.replace("&quot", "");
                                _hint = _hint.replace("&amp", "&");
                                _hint = _hint.replace('\'', ""); //NEU
                                _hint = _hint.replace('\"', ""); //NEU
                                _hint = _hint.replace("&#34", R"(")");
                            }
                            TMxpElement _element;
                            _element.name = _tn;
                            _element.href = _ref;
                            _element.hint = _hint;
                            mMXP_Elements[_tn] = _element;
                        }
                        openT = 0;
                        closeT = 0;
                        currentToken.clear();
                        ++localBufferPosition;
                        continue;
                    }


                    if (mMXP_LINK_MODE) {
                        if (_tn.indexOf('/') != -1) {
                            mMXP_LINK_MODE = false;
                        }
                    }

                    if (mMXP_SEND_NO_REF_MODE) {
                        if (_tn.indexOf('/') != -1) {
                            mMXP_SEND_NO_REF_MODE = false;
                            if (mLinkStore[mLinkID].front() == "send([[]])") {
                                QString _t_ref = "send([[";
                                _t_ref.append(mAssembleRef.c_str());
                                _t_ref.append("]])");
                                QStringList _t_ref_list;
                                _t_ref_list << _t_ref;
                                mLinkStore[mLinkID] = _t_ref_list;
                            } else {
                                mLinkStore[mLinkID].replaceInStrings("&text;", mAssembleRef.c_str());
                            }
                            mAssembleRef.clear();
                        }
                    } else if (mMXP_Elements.contains(_tn)) {
                        QString _tp;
                        std::string::size_type _fs = currentToken.find_first_of(' ');
                        if (_fs != std::string::npos) {
                            _tp = currentToken.substr(_fs).c_str();
                        }
                        QString _t1 = _tp.toUpper();
                        const TMxpElement& _element =  mMXP_Elements[_tn];
                        QString _t2 = _element.href;
                        QString _t3 = _element.hint;
                        bool _userTag = true;
                        if (_t2.size() < 1) {
                            _userTag = false;
                        }
                        QRegularExpression _rex;
                        QStringList _rl1, _rl2;
                        int _ki1 = _tp.indexOf('\'');
                        int _ki2 = _tp.indexOf('\"');
                        int _ki3 = _tp.indexOf('=');
                        // is the first parameter to send given in the form
                        // send "what" hint="bla" or send href="what" hint="bla"

                        // handle the first case without a variable assignment
                        if ((_ki3 == -1)                                           // no = whatsoever
                            || ((_ki3 != -1) && ((_ki2 < _ki3) || (_ki1 < _ki3)))) // first parameter is given without =
                        {
                            if ((_ki1 < _ki2 && _ki1 != -1) || (_ki2 == -1 && _ki1 != -1)) {
                                if (_ki1 < _ki3 || _ki3 == -1) {
                                    _rl1 << "HREF";
                                    int _cki1 = _tp.indexOf('\'', _ki1 + 1);
                                    if (_cki1 > -1) {
                                        _rl2 << _tp.mid(_ki1 + 1, _cki1 - (_ki1 + 1));
                                    }
                                }
                            } else if ((_ki2 < _ki1 && _ki2 != -1) || (_ki1 == -1 && _ki2 != -1)) {
                                if (_ki2 < _ki3 || _ki3 == -1) {
                                    _rl1 << "HREF";
                                    int _cki2 = _tp.indexOf('\"', _ki2 + 1);
                                    if (_cki2 > -1) {
                                        _rl2 << _tp.mid(_ki2 + 1, _cki2 - (_ki2 + 1));
                                    }
                                }
                            }
                        }
                        // parse parameters in the form var="val" or var='val' where val can be given in the form "foo'b'ar" or 'foo"b"ar'
                        if (_tp.contains(QStringLiteral(R"(=')"))) {
                            _rex = QRegularExpression(QStringLiteral(R"(\b(\w+)=\'([^\']*) ?)"));
                        } else {
                            _rex = QRegularExpression(QStringLiteral(R"(\b(\w+)=\"([^\"]*) ?)"));
                        }

                        int _rpos = 0;
                        QRegularExpressionMatch match = _rex.match(_tp, _rpos);
                        while ((_rpos = match.capturedStart()) != -1) {
                            _rl1 << match.captured(1).toUpper();
                            _rl2 << match.captured(2);
                            _rpos += match.capturedLength();

                            match = _rex.match(_tp, _rpos);
                        }

                        if ((_rl1.size() == _rl2.size()) && (!_rl1.empty())) {
                            for (int i = 0; i < _rl1.size(); i++) {
                                QString _var = _rl1[i];
                                _var.prepend('&');
                                if (_userTag || _t2.indexOf(_var) != -1) {
                                    _t2 = _t2.replace(_var, _rl2[i]);
                                    _t3 = _t3.replace(_var, _rl2[i]);
                                } else {
                                    if (_rl1[i] == QStringLiteral("HREF")) {
                                        _t2 = _rl2[i];
                                    }
                                    if (_rl1[i] == QStringLiteral("HINT")) {
                                        _t3 = _rl2[i];
                                    }
                                }
                            }
                        }

                        // handle print to prompt feature PROMPT
                        bool _send_to_command_line = false;
                        if (_t1.endsWith("PROMPT")) {
                            _send_to_command_line = true;
                        }


                        mMXP_LINK_MODE = true;
                        if (_t2.size() < 1 || _t2.contains("&text;")) {
                            mMXP_SEND_NO_REF_MODE = true;
                        }
                        mLinkID++;
                        if (mLinkID > 1000) {
                            mLinkID = 1;
                        }
                        QStringList _tl = _t2.split('|');
                        for (int i = 0; i < _tl.size(); i++) {
                            _tl[i].replace("|", "");
                            if (!_send_to_command_line) {
                                _tl[i] = "send([[" + _tl[i] + "]])";
                            } else {
                                _tl[i] = "printCmdLine([[" + _tl[i] + "]])";
                            }
                        }

                        mLinkStore[mLinkID] = _tl;

                        _t3 = _t3.replace("&quot;", R"(")");
                        _t3 = _t3.replace("&amp;", "&");
                        _t3 = _t3.replace("&apos;", "'");
                        _t3 = _t3.replace("&#34;", R"(")");

                        QStringList _tl2 = _t3.split('|');
                        _tl2.replaceInStrings("|", "");
                        if (_tl2.size() >= _tl.size() + 1) {
                            _tl2.pop_front();
                        }
                        mHintStore[mLinkID] = _tl2;
                    }
                    openT = 0;
                    closeT = 0;
                    currentToken.clear();
                }
                ++localBufferPosition;
                continue;
            }

            if (mAssemblingToken) {
                if (ch == '\n') {
                    closeT = 0;
                    openT = 0;
                    mAssemblingToken = false;
                    currentToken.clear();
                    mParsingVar = false;
                } else {
                    currentToken += ch;
                    ++localBufferPosition;
                    continue;
                }
            }

            if (ch == '&' || mIgnoreTag) {
                if ((localBufferPosition + 4 < localBufferLength) && (mSkip.empty())) {
                    if (localBuffer.substr(localBufferPosition, 4) == "&gt;") {
                        localBufferPosition += 3;
                        ch = '>';
                        mIgnoreTag = false;
                    } else if (localBuffer.substr(localBufferPosition, 4) == "&lt;") {
                        localBufferPosition += 3;
                        ch = '<';
                        mIgnoreTag = false;
                    } else if (localBuffer.substr(localBufferPosition, 5) == "&amp;") {
                        mIgnoreTag = false;
                        localBufferPosition += 4;
                        ch = '&';
                    } else if (localBuffer.substr(localBufferPosition, 6) == "&quot;") {
                        localBufferPosition += 5;
                        mIgnoreTag = false;
                        mSkip.clear();
                        ch = '"';
                    }
                }
                // if the content is split across package borders
                else if (mSkip == "&gt" && ch == ';') {
                    mIgnoreTag = false;
                    mSkip.clear();
                    ch = '>';
                } else if (mSkip == "&lt" && ch == ';') {
                    mIgnoreTag = false;
                    mSkip.clear();
                    ch = '<';
                } else if (mSkip == "&amp" && ch == ';') {
                    mIgnoreTag = false;
                    mSkip.clear();
                    ch = '&';
                } else if (mSkip == "&quot;" && ch == ';') {
                    mIgnoreTag = false;
                    mSkip.clear();
                    ch = '"';
                } else {
                    mIgnoreTag = true;
                    mSkip += ch;
                    // sanity check
                    if (mSkip.size() > 7) {
                        mIgnoreTag = false;
                        mSkip.clear();
                    }
                    ++localBufferPosition;
                    continue;
                }
            }
        }


        if (mMXP_SEND_NO_REF_MODE) {
            mAssembleRef += ch;
        }

    COMMIT_LINE:
        if ((ch == '\n') || (ch == '\xff') || (ch == '\r')) {
            // DE: MUD Zeilen werden immer am Zeilenanfang geschrieben
            // EN: MUD lines are always written at the beginning of the line

            // FIXME: This is the point where we should renormalise the new text
            // data - of course there is the theoretical chance that the new
            // text would alter the prior contents but as that is on a separate
            // line there should not be any changes to text before a line feed
            // which sort of seems to be implied by the current value of ch:

            if (static_cast<size_t>(mMudLine.size()) != mMudBuffer.size()) {
                qWarning() << "TBuffer::translateToPlainText(...) WARNING: mismatch in new text "
                              "data character and attribute data items!";
            }

            if (!lineBuffer.back().isEmpty()) {
                if (mMudLine.size() > 0) {
                    lineBuffer << mMudLine;
                } else {
                    if (ch == '\r') {
                        ++localBufferPosition;
                        continue; //empty timer posting
                    }
                    lineBuffer << QString();
                }
                buffer.push_back(mMudBuffer);
                dirty << true;
                timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
                if (ch == '\xff') {
                    promptBuffer.append(true);
                } else {
                    promptBuffer.append(false);
                }
            } else {
                if (mMudLine.size() > 0) {
                    lineBuffer.back().append(mMudLine);
                } else {
                    if (ch == '\r') {
                        ++localBufferPosition;
                        continue; //empty timer posting
                    }
                    lineBuffer.back().append(QString());
                }
                buffer.back() = mMudBuffer;
                dirty.back() = true;
                timeBuffer.back() = QTime::currentTime().toString("hh:mm:ss.zzz") + "   ";
                if (ch == '\xff') {
                    promptBuffer.back() = true;
                } else {
                    promptBuffer.back() = false;
                }
            }

            mMudLine.clear();
            mMudBuffer.clear();
            int line = lineBuffer.size() - 1;
            mpHost->mpConsole->runTriggers(line);
            // Only use of TBuffer::wrap(), breaks up new text
            // NOTE: it MAY have been clobbered by the trigger engine!
            wrap(lineBuffer.size() - 1);

            // Start a new, but empty line in the various buffers
            ++localBufferPosition;
            std::deque<TChar> newLine;
            buffer.push_back(newLine);
            lineBuffer.push_back(QString());
            timeBuffer.push_back("   ");
            promptBuffer << false;
            dirty << true;
            if (static_cast<int>(buffer.size()) > mLinesLimit) {
                shrinkBuffer();
            }
            continue;
        }

        // PLACEMARKER: Incoming text decoding
        // Used to double up the TChars for Utf-8 byte sequences that produce
        // a surrogate pair (non-BMP):
        bool isTwoTCharsNeeded = false;

        if (!encodingLookupTable.isEmpty()) {
            auto index = static_cast<quint8>(ch);
            if (index < 128) {
                mMudLine.append(QChar::fromLatin1(ch));
            } else {
                mMudLine.append(encodingLookupTable.at(index - 128));
            }
        } else if (mEncoding == QLatin1String("ISO 8859-1")) {
            mMudLine.append(QString(QChar::fromLatin1(ch)));
        } else if (mEncoding == QLatin1String("GBK")) {
            if (!processGBSequence(localBuffer, isFromServer, false, localBufferLength, localBufferPosition, isTwoTCharsNeeded)) {
                // We have run out of bytes and we have stored the unprocessed
                // ones but we need to bail out NOW!
                return;
            }
        } else if (mEncoding == QLatin1String("GB18030")) {
            if (!processGBSequence(localBuffer, isFromServer, true, localBufferLength, localBufferPosition, isTwoTCharsNeeded)) {
                // We have run out of bytes and we have stored the unprocessed
                // ones but we need to bail out NOW!
                return;
            }
        } else if (mEncoding == QLatin1String("Big5")) {
            if (!processBig5Sequence(localBuffer, isFromServer, localBufferLength, localBufferPosition, isTwoTCharsNeeded)) {
                // We have run out of bytes and we have stored the unprocessed
                // ones but we need to bail out NOW!
                return;
            }
        } else if (mEncoding == QLatin1String("UTF-8")) {
            if (!processUtf8Sequence(localBuffer, isFromServer, localBufferLength, localBufferPosition, isTwoTCharsNeeded)) {
                // We have run out of bytes and we have stored the unprocessed
                // ones but we need to bail out NOW!
                return;
            }
        } else {
            // Default - no encoding case - reject anything that has MS Bit set
            // as that isn't ASCII which is what no encoding specifies!
            if (ch & 0x80) {
                // Was going to ignore this byte, not add a TChar instance
                // either and move on:
                // ++localBufferPosition;
                // continue;
                // but instead insert the "Replacement Character Marker"
                mMudLine.append(QChar::ReplacementCharacter);
            } else {
                mMudLine.append(ch);
            }
        }

        TChar c(!mIsDefaultColor && mBold ? mCurrentFgColorLightR : mCurrentFgColorR,
                !mIsDefaultColor && mBold ? mCurrentFgColorLightG : mCurrentFgColorG,
                !mIsDefaultColor && mBold ? mCurrentFgColorLightB : mCurrentFgColorB,
                mCurrentBgColorR,
                mCurrentBgColorG,
                mCurrentBgColorB,
                mIsDefaultColor ? mBold : false,
                mItalics,
                mUnderline,
                mStrikeOut);

        if (mMXP_LINK_MODE) {
            c.link = mLinkID;
            c.flags |= TCHAR_UNDERLINE;
        }

        mMudBuffer.push_back(c);

        if (isTwoTCharsNeeded) {
            TChar c2(!mIsDefaultColor && mBold ? mCurrentFgColorLightR : mCurrentFgColorR,
                     !mIsDefaultColor && mBold ? mCurrentFgColorLightG : mCurrentFgColorG,
                     !mIsDefaultColor && mBold ? mCurrentFgColorLightB : mCurrentFgColorB,
                     mCurrentBgColorR,
                     mCurrentBgColorG,
                     mCurrentBgColorB,
                     mIsDefaultColor ? mBold : false,
                     mItalics,
                     mUnderline,
                     mStrikeOut);

            // CHECK: Do we need to duplicate stuff for mMXP_LINK_MODE?
            mMudBuffer.push_back(c2);
        }

        ++localBufferPosition;
    }
}

void TBuffer::decodeSGR38(QStringList parameters, bool isColonSeparated)
{
#if defined(DEBUG_SGR_PROCESSING)
    qDebug() << "    TBuffer::decodeSGR38(" << parameters << "," << isColonSeparated <<") INFO - called";
#endif
    if (parameters.at(1) == QLatin1String("5")) {
        int tag = 0;
        if (parameters.count() > 2) {
            bool isOk = false;
            tag = parameters.at(2).toInt(&isOk);
#if defined(DEBUG_SGR_PROCESSING)
            if (!isOk) {
                if (isColonSeparated) {
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR38(...) ERROR - failed to parse color index parameter element (the third part) in a SGR...;38:5:" << parameters.at(2) << ":...;...m sequence treating it as a zero!";
                } else {
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR38(...) ERROR - failed to parse color index parameter string (the third part) in a SGR...;38;5;" << parameters.at(2) << ";...m sequence treating it as a zero!";
                }
            }
#endif
        } else {
            // Missing last parameter - so it is treated as a zero
#if defined(DEBUG_SGR_PROCESSING)
            if (!isOk) {
                if (isColonSeparated) {
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR38(...) ERROR - missing color index parameter element (the third part) in a SGR...;38:5;...m sequence treating it as a zero!";
                } else {
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR38(...) ERROR - missing color index parameter string (the third part) in a SGR...;38;5;;m sequence treating it as a zero!";
                }
            }
#endif
        }

        if (tag < 16) {
            if (tag >= 8) {
                tag -= 8;
                mBold = true;
            } else {
                mBold = false;
            }
            mIsDefaultColor = false;

            switch (tag) {
            case 0:
                mCurrentFgColorR = mBlackR;
                mCurrentFgColorG = mBlackG;
                mCurrentFgColorB = mBlackB;
                mCurrentFgColorLightR = mLightBlackR;
                mCurrentFgColorLightG = mLightBlackG;
                mCurrentFgColorLightB = mLightBlackB;
                break;
            case 1:
                mCurrentFgColorR = mRedR;
                mCurrentFgColorG = mRedG;
                mCurrentFgColorB = mRedB;
                mCurrentFgColorLightR = mLightRedR;
                mCurrentFgColorLightG = mLightRedG;
                mCurrentFgColorLightB = mLightRedB;
                break;
            case 2:
                mCurrentFgColorR = mGreenR;
                mCurrentFgColorG = mGreenG;
                mCurrentFgColorB = mGreenB;
                mCurrentFgColorLightR = mLightGreenR;
                mCurrentFgColorLightG = mLightGreenG;
                mCurrentFgColorLightB = mLightGreenB;
                break;
            case 3:
                mCurrentFgColorR = mYellowR;
                mCurrentFgColorG = mYellowG;
                mCurrentFgColorB = mYellowB;
                mCurrentFgColorLightR = mLightYellowR;
                mCurrentFgColorLightG = mLightYellowG;
                mCurrentFgColorLightB = mLightYellowB;
                break;
            case 4:
                mCurrentFgColorR = mBlueR;
                mCurrentFgColorG = mBlueG;
                mCurrentFgColorB = mBlueB;
                mCurrentFgColorLightR = mLightBlueR;
                mCurrentFgColorLightG = mLightBlueG;
                mCurrentFgColorLightB = mLightBlueB;
                break;
            case 5:
                mCurrentFgColorR = mMagentaR;
                mCurrentFgColorG = mMagentaG;
                mCurrentFgColorB = mMagentaB;
                mCurrentFgColorLightR = mLightMagentaR;
                mCurrentFgColorLightG = mLightMagentaG;
                mCurrentFgColorLightB = mLightMagentaB;
                break;
            case 6:
                mCurrentFgColorR = mCyanR;
                mCurrentFgColorG = mCyanG;
                mCurrentFgColorB = mCyanB;
                mCurrentFgColorLightR = mLightCyanR;
                mCurrentFgColorLightG = mLightCyanG;
                mCurrentFgColorLightB = mLightCyanB;
                break;
            case 7:
                mCurrentFgColorR = mWhiteR;
                mCurrentFgColorG = mWhiteG;
                mCurrentFgColorB = mWhiteB;
                mCurrentFgColorLightR = mLightWhiteR;
                mCurrentFgColorLightG = mLightWhiteG;
                mCurrentFgColorLightB = mLightWhiteB;
                break;
            }

        } else if (tag < 232) {
            // because color 1-15 behave like normal ANSI colors
            tag -= 16;
            // 6x6x6 RGB color space
            quint8 r = tag / 36;
            quint8 g = (tag - (r * 36)) / 6;
            quint8 b = (tag - (r * 36)) - (g * 6);
            // Did use 42 as a factor but that isn't right
            // as it yields:
            // 0:0; 1:42; 2:84; 3:126; 4:168; 5:210
            // 6 x 42 DOES equal 252 BUT IT IS OUT OF RANGE
            // Instead we use 51:
            // 0:0; 1:51; 2:102; 3:153; 4:204: 5:255
            mCurrentFgColorR = r * 51;
            mCurrentFgColorLightR = r * 51;
            mCurrentFgColorG = g * 51;
            mCurrentFgColorLightG = g * 51;
            mCurrentFgColorB = b * 51;
            mCurrentFgColorLightB = b * 51;

        } else {
            // black + 23 tone grayscale from dark to light
            // gray. Similar to RGB case the multiplier was
            // a bit off we had been using 10 but:
            // 23 x 10 = 230
            // whereas 23 should map to 255, this requires
            // a non-integer multiplier, instead of
            // multiplying and rounding we, for speed, can
            // use a look-up table:
            int value = 0;
            // clang-format off
            switch (tag) {
                case 232:   value =   0; break; //   0.000
                case 233:   value =  11; break; //  11.087
                case 234:   value =  22; break; //  22.174
                case 235:   value =  33; break; //  33.261
                case 236:   value =  44; break; //  44.348
                case 237:   value =  55; break; //  55.435
                case 238:   value =  67; break; //  66.522
                case 239:   value =  78; break; //  77.609
                case 240:   value =  89; break; //  88.696
                case 241:   value = 100; break; //  99.783
                case 242:   value = 111; break; // 110.870
                case 243:   value = 122; break; // 121.957
                case 244:   value = 133; break; // 133.043
                case 245:   value = 144; break; // 144.130
                case 246:   value = 155; break; // 155.217
                case 247:   value = 166; break; // 166.304
                case 248:   value = 177; break; // 177.391
                case 249:   value = 188; break; // 188.478
                case 250:   value = 200; break; // 199.565
                case 251:   value = 211; break; // 210.652
                case 252:   value = 222; break; // 221.739
                case 253:   value = 233; break; // 232.826
                case 254:   value = 244; break; // 243.913
                case 255:   value = 255; break; // 255.000
                default:
                    value = 192;
#if defined(DEBUG_SGR_PROCESSING)
                    if (isColonSeparated) {
                        qDebug().noquote().nospace() << "TBuffer::decodeSGR38(...) ERROR - unexpected color index parameter element (the third part) in a SGR...;38:5:" << parameters.at(2) << ";..m sequence treating it as 192!";
                    } else {
                        qDebug().noquote().nospace() << "TBuffer::decodeSGR38(...) ERROR - unexpected color index parameter element (the third part) in a SGR...;38;5;" << parameters.at(2) << ";..m sequence treating it as 192!";
                    }
#endif
            }

             // clang-format on
            mCurrentFgColorR = value;
            mCurrentFgColorLightR = value;
            mCurrentFgColorG = value;
            mCurrentFgColorLightG = value;
            mCurrentFgColorB = value;
            mCurrentFgColorLightB = value;
        }

    } else if (parameters.at(1) == QLatin1String("2")) {
        if (parameters.count() >= 6) {
            // Have enough for all three colour
            // components
            mCurrentFgColorR = qBound(0, parameters.at(3).toInt(), 255);
            mCurrentFgColorG = qBound(0, parameters.at(4).toInt(), 255);
            mCurrentFgColorB = qBound(0, parameters.at(5).toInt(), 255);
        } else if (parameters.count() >= 5) {
            // Have enough for two colour
            // components, but blue component is
            // zero
            mCurrentFgColorR = qBound(0, parameters.at(3).toInt(), 255);
            mCurrentFgColorG = qBound(0, parameters.at(4).toInt(), 255);
            mCurrentFgColorB = 0;
        } else if (parameters.count() >= 4) {
            // Have enough for one colour component,
            // but green and blue components are
            // zero
            mCurrentFgColorR = qBound(0, parameters.at(3).toInt(), 255);
            mCurrentFgColorG = 0;
            mCurrentFgColorB = 0;
        } else  {
            // No codes left for any colour
            // components so colour must be black,
            // as all of red, green and blue
            // components are zero
            mCurrentFgColorR = 0;
            mCurrentFgColorG = 0;
            mCurrentFgColorB = 0;
        }

        if (parameters.count() >= 3 && !parameters.at(2).isEmpty()) {
            if (!isColonSeparated) {
#if ! defined(DEBUG_SGR_PROCESSING)
                qDebug() << "Unhandled color space identifier in a SGR...;38;2;" << parameters.at(2) << ";...m sequence - if 16M colors items are missing blue elements you may have checked the \"Expect Color Space Id in SGR...(3|4)8;2;....m codes\" option on the Special Options tab of the preferences when it is not needed!";
#else
                qDebug().noquote().nospace() << "TBuffer::decodeSGR38(...) WARNING - unhandled color space identifier in a SGR...;38;2;" << parameters.at(2) << ";...m sequence treating it as the default (empty) case!";
            } else {
                qDebug().noquote().nospace() << "TBuffer::decodeSGR38(...) WARNING - unhandled color space identifier in a SGR...;38:2:" << parameters.at(2) << ":...;...m sequence treating it as the default (empty) case!";
#endif
            }
        }
        mCurrentFgColorLightR = mCurrentFgColorR;
        mCurrentFgColorLightG = mCurrentFgColorG;
        mCurrentFgColorLightB = mCurrentFgColorB;

    } else if (parameters.at(1) == QLatin1String("4")
            || parameters.at(1) == QLatin1String("3")
            || parameters.at(1) == QLatin1String("1")
            || parameters.at(1) == QLatin1String("0")) {

#if defined(DEBUG_SGR_PROCESSING)
        if (isColonSeparated) {
            qDebug().noquote().nospace() << "TBuffer::decodeSGR38(...) WARNING - unhandled SGR code: SGR...;38:" << parameters.at(1) << ":...;...m ignoring sequence!";
        } else {
            qDebug().noquote().nospace() << "TBuffer::decodeSGR38(...) WARNING - unhandled SGR code: SGR...;38;" << parameters.at(1) << ";...m ignoring sequence!";
        }
#endif

    } else {

#if defined(DEBUG_SGR_PROCESSING)
        if (isColonSeparated) {
            qDebug().noquote().nospace() << "TBuffer::decodeSGR38(...) WARNING - unexpect SGR code: SGR...;38:" << parameters.at(1) << ":...;...m ignoring sequence!";
        } else {
            qDebug().noquote().nospace() << "TBuffer::decodeSGR38(...) WARNING - unexpect SGR code: SGR...;38;" << parameters.at(1) << ";...m ignoring sequence!";
        }
#endif

    }
}

void TBuffer::decodeSGR48(QStringList parameters, bool isColonSeparated)
{
#if defined(DEBUG_SGR_PROCESSING)
    qDebug() << "    TBuffer::decodeSGR48(" << parameters << "," << isColonSeparated <<") INFO - called";
#endif
    bool useLightColor = false;

    if (parameters.at(1) == QLatin1String("5")) {
        int tag = 0;
        if (parameters.count() > 2) {
            bool isOk = false;
            tag = parameters.at(2).toInt(&isOk);
#if defined(DEBUG_SGR_PROCESSING)
            if (!isOk) {
                if (isColonSeparated) {
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR48(...) ERROR - failed to parse color index parameter element (the third part) in a SGR...;48:5:" << parameters.at(2) << ":...;...m sequence treating it as a zero!";
                } else {
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR48(...) ERROR - failed to parse color index parameter string (the third part) in a SGR...;48;5;" << parameters.at(2) << ";...m sequence treating it as a zero!";
                }
            }
#endif
        } else {
            // Missing last parameter - so it is treated as a zero
#if defined(DEBUG_SGR_PROCESSING)
            if (!isOk) {
                if (isColonSeparated) {
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR48(...) ERROR - missing color index parameter element (the third part) in a SGR...;48:5;...m sequence treating it as a zero!";
                } else {
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR48(...) ERROR - missing color index parameter string (the third part) in a SGR...;48;5;;m sequence treating it as a zero!";
                }
            }
#endif
        }

        if (tag < 16) {
            if (tag >= 8) {
                tag -= 8;
                useLightColor = true;
            } else {
                useLightColor = false;
            }
            mIsDefaultColor = false;
            int bgColorLightR = 0;
            int bgColorLightG = 0;
            int bgColorLightB = 0;

            switch (tag) {
            case 0:
                mCurrentBgColorR = mBlackR;
                mCurrentBgColorG = mBlackG;
                mCurrentBgColorB = mBlackB;
                bgColorLightR = mLightBlackR;
                bgColorLightG = mLightBlackG;
                bgColorLightB = mLightBlackB;
                break;
            case 1:
                mCurrentBgColorR = mRedR;
                mCurrentBgColorG = mRedG;
                mCurrentBgColorB = mRedB;
                bgColorLightR = mLightRedR;
                bgColorLightG = mLightRedG;
                bgColorLightB = mLightRedB;
                break;
            case 2:
                mCurrentBgColorR = mGreenR;
                mCurrentBgColorG = mGreenG;
                mCurrentBgColorB = mGreenB;
                bgColorLightR = mLightGreenR;
                bgColorLightG = mLightGreenG;
                bgColorLightB = mLightGreenB;
                break;
            case 3:
                mCurrentBgColorR = mYellowR;
                mCurrentBgColorG = mYellowG;
                mCurrentBgColorB = mYellowB;
                bgColorLightR = mLightYellowR;
                bgColorLightG = mLightYellowG;
                bgColorLightB = mLightYellowB;
                break;
            case 4:
                mCurrentBgColorR = mBlueR;
                mCurrentBgColorG = mBlueG;
                mCurrentBgColorB = mBlueB;
                bgColorLightR = mLightBlueR;
                bgColorLightG = mLightBlueG;
                bgColorLightB = mLightBlueB;
                break;
            case 5:
                mCurrentBgColorR = mMagentaR;
                mCurrentBgColorG = mMagentaG;
                mCurrentBgColorB = mMagentaB;
                bgColorLightR = mLightMagentaR;
                bgColorLightG = mLightMagentaG;
                bgColorLightB = mLightMagentaB;
                break;
            case 6:
                mCurrentBgColorR = mCyanR;
                mCurrentBgColorG = mCyanG;
                mCurrentBgColorB = mCyanB;
                bgColorLightR = mLightCyanR;
                bgColorLightG = mLightCyanG;
                bgColorLightB = mLightCyanB;
                break;
            case 7:
                mCurrentBgColorR = mWhiteR;
                mCurrentBgColorG = mWhiteG;
                mCurrentBgColorB = mWhiteB;
                bgColorLightR = mLightWhiteR;
                bgColorLightG = mLightWhiteG;
                bgColorLightB = mLightWhiteB;
                break;
            }
            if (useLightColor) {
                mCurrentBgColorR = bgColorLightR;
                mCurrentBgColorG = bgColorLightG;
                mCurrentBgColorB = bgColorLightB;
            }

        } else if (tag < 232) {
            // because color 1-15 behave like normal ANSI colors
            tag -= 16;
            // 6x6x6 RGB color space
            quint8 r = tag / 36;
            quint8 g = (tag - (r * 36)) / 6;
            quint8 b = (tag - (r * 36)) - (g * 6);
            // Did use 42 as a factor but that isn't right
            // as it yields:
            // 0:0; 1:42; 2:84; 3:126; 4:168; 5:210
            // 6 x 42 DOES equal 252 BUT IT IS OUT OF RANGE
            // Instead we use 51:
            // 0:0; 1:51; 2:102; 3:153; 4:204: 5:255
            mCurrentBgColorR = r * 51;
            mCurrentBgColorG = g * 51;
            mCurrentBgColorB = b * 51;

        } else {
            // black + 23 tone grayscale from dark to light
            // gray. Similar to RGB case the multiplier was
            // a bit off we had been using 10 but:
            // 23 x 10 = 230
            // whereas 23 should map to 255, this requires
            // a non-integer multiplier, instead of
            // multiplying and rounding we, for speed, can
            // use a look-up table:
            int value = 0;
            // clang-format off
            switch (tag) {
                case 232:   value =   0; break; //   0.000
                case 233:   value =  11; break; //  11.087
                case 234:   value =  22; break; //  22.174
                case 235:   value =  33; break; //  33.261
                case 236:   value =  44; break; //  44.348
                case 237:   value =  55; break; //  55.435
                case 238:   value =  67; break; //  66.522
                case 239:   value =  78; break; //  77.609
                case 240:   value =  89; break; //  88.696
                case 241:   value = 100; break; //  99.783
                case 242:   value = 111; break; // 110.870
                case 243:   value = 122; break; // 121.957
                case 244:   value = 133; break; // 133.043
                case 245:   value = 144; break; // 144.130
                case 246:   value = 155; break; // 155.217
                case 247:   value = 166; break; // 166.304
                case 248:   value = 177; break; // 177.391
                case 249:   value = 188; break; // 188.478
                case 250:   value = 200; break; // 199.565
                case 251:   value = 211; break; // 210.652
                case 252:   value = 222; break; // 221.739
                case 253:   value = 233; break; // 232.826
                case 254:   value = 244; break; // 243.913
                case 255:   value = 255; break; // 255.000
                default:
                    value = 64;
#if defined(DEBUG_SGR_PROCESSING)
                    if (isColonSeparated) {
                        qDebug().noquote().nospace() << "TBuffer::decodeSGR48(...) ERROR - unexpected color index parameter element (the third part) in a SGR...;48:5:" << parameters.at(2) << ";..m sequence treating it as 64!";
                    } else {
                        qDebug().noquote().nospace() << "TBuffer::decodeSGR48(...) ERROR - unexpected color index parameter element (the third part) in a SGR...;48;5;" << parameters.at(2) << ";..m sequence treating it as 64!";
                    }
#endif
            }
             // clang-format on
            mCurrentBgColorR = value;
            mCurrentBgColorG = value;
            mCurrentBgColorB = value;
        }

    } else if (parameters.at(1) == QLatin1String("2")) {
        if (parameters.count() >= 6) {
            // Have enough for all three colour
            // components
            mCurrentBgColorR = qBound(0, parameters.at(3).toInt(), 255);
            mCurrentBgColorG = qBound(0, parameters.at(4).toInt(), 255);
            mCurrentBgColorB = qBound(0, parameters.at(5).toInt(), 255);

        } else if (parameters.count() >= 5) {
            // Have enough for two colour
            // components, but blue component is
            // zero
            mCurrentBgColorR = qBound(0, parameters.at(3).toInt(), 255);
            mCurrentBgColorG = qBound(0, parameters.at(4).toInt(), 255);
            mCurrentBgColorB = 0;

        } else if (parameters.count() >= 4) {
            // Have enough for one colour component,
            // but green and blue components are
            // zero
            mCurrentBgColorR = qBound(0, parameters.at(3).toInt(), 255);
            mCurrentBgColorG = 0;
            mCurrentBgColorB = 0;

        } else  {
            // No codes left for any colour
            // components so colour must be black,
            // as all of red, green and blue
            // components are zero
            mCurrentBgColorR = 0;
            mCurrentBgColorG = 0;
            mCurrentBgColorB = 0;
        }

        if (parameters.count() >= 3 && !parameters.at(2).isEmpty()) {
            if (!isColonSeparated) {
#if ! defined(DEBUG_SGR_PROCESSING)
                qDebug() << "Unhandled color space identifier in a SGR...;48;2;" << parameters.at(2) << ";...m sequence - if 16M colors items are missing blue elements you may have checked the \"Expect Color Space Id in SGR...(3|4)8;2;....m codes\" option on the Special Options tab of the preferences when it is not needed!";
#else
                qDebug().noquote().nospace() << "TBuffer::decodeSGR48(...) WARNING - unhandled color space identifier in a SGR...;48;2;" << parameters.at(2) << ";...m sequence treating it as the default (empty) case!";
            } else {
                qDebug().noquote().nospace() << "TBuffer::decodeSGR48(...) WARNING - unhandled color space identifier in a SGR...;48:2:" << parameters.at(2) << ":...;...m sequence treating it as the default (empty) case!";
#endif
            }
        }

    } else if (parameters.at(1) == QLatin1String("4")
            || parameters.at(1) == QLatin1String("3")
            || parameters.at(1) == QLatin1String("1")
            || parameters.at(1) == QLatin1String("0")) {

#if defined(DEBUG_SGR_PROCESSING)
        if (isColonSeparated) {
            qDebug().noquote().nospace() << "TBuffer::decodeSGR48(...) WARNING - unhandled SGR code: SGR...;48:" << parameters.at(1) << ":...;...m ignoring sequence!";
        } else {
            qDebug().noquote().nospace() << "TBuffer::decodeSGR48(...) WARNING - unhandled SGR code: SGR...;48;" << parameters.at(1) << ";...m ignoring sequence!";
        }
#endif

    } else {

#if defined(DEBUG_SGR_PROCESSING)
        if (isColonSeparated) {
            qDebug().noquote().nospace() << "TBuffer::decodeSGR48(...) WARNING - unexpect SGR code: SGR...;48:" << parameters.at(1) << ":...;...m ignoring sequence!";
        } else {
            qDebug().noquote().nospace() << "TBuffer::decodeSGR48(...) WARNING - unexpect SGR code: SGR...;48;" << parameters.at(1) << ";...m ignoring sequence!";
        }
#endif
    }

}

void TBuffer::decodeSGR(const QString& sequence)
{
    Host* pHost = mpHost;
    if (!pHost) {
        qWarning() << "TBuffer::decodeSGR(...) ERROR - Called when mpHost pointer is nullptr";
        return;
    }

    bool haveColorSpaceId = pHost->getHaveColorSpaceId();

    QStringList parameterStrings = sequence.split(QChar(';'));
    for (int paraIndex = 0, total = parameterStrings.count(); paraIndex < total; ++paraIndex) {
        QString allParameterElements = parameterStrings.at(paraIndex);
        if (allParameterElements.contains(QLatin1String(":"))) {
            // We have colon separated parameter elements, so we must have at least 2 members
            QStringList parameterElements(allParameterElements.split(QChar(':')));
            if (parameterElements.at(0) == QLatin1String("38")) {
                if (parameterElements.count() >= 2) {
                    decodeSGR38(parameterElements, true);

                } else {
                    // We only have a single element in this parameterString,
                    // so we will need to steal the needed number from the
                    // remainder - this is falling back to using a semicolon
                    // separated list rather than a colon separated one
                    if (paraIndex + 1 >= total) {
                        // Oh dear we are out of parameters to examine, so bail
                        // out:
                        return;
                    }

                    // Okay we have one more parameter at least - so examine it
                    // and grab the needed number of arguments:
                    QStringList madeElements;
                    madeElements << parameterStrings.at(paraIndex); // "38"
                    madeElements << parameterStrings.at(paraIndex + 1); // "2" or "5" hopefully
                    bool isOk = false;
                    int sgr38_type = madeElements.at(1).toInt(&isOk);
                    if (madeElements.at(1).isEmpty() || !isOk || sgr38_type == 0) {
                        // Oh dear that parameter is empty or equivalent to zero
                        // so we cannot do anything more
                        return;
                    }

                    switch (sgr38_type) {
                    case 5: // Needs one more number
                        if (paraIndex + 2 < total) {
                            // We have the parameter needed
                            madeElements << parameterStrings.at(paraIndex + 2);
                        }
                        decodeSGR38(madeElements, false);
                        // Move the index to consume the used values less
                        // the one that the for loop will handled - even if it
                        // goes past end
                        paraIndex += 1;
                        break;
                    case 4: // Not handled but we still should skip its arguments
                            // Uses four or five depending on whether there is
                            // the colour space id first
                            // Move the index to consume the used values less
                            // the one that the for loop will handled - even if it
                            // goes past end
                        paraIndex += (haveColorSpaceId ? 6 : 5);
                        break;
                    case 3: // Not handled but we still should skip its arguments
                            // Move the index to consume the used values less
                            // the one that the for loop will handled - even if it
                            // goes past end
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 2: // Need three or four depending on whether there is
                            // the colour space id first
                        if (haveColorSpaceId) {
                            if (paraIndex + 2 < total) {
                                // We have the color space id
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                            if (paraIndex + 5 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 5);
                            }
                        } else {
                            // Fake an empty colour space id
                            madeElements << QString();
                            if (paraIndex + 2 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                        }

                        decodeSGR38(madeElements, false);
                        // Move the index to consume the used values less
                        // the one that the for loop will handled - even if it
                        // goes past end
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 1: // This uses no extra arguments and, as it means
                            // transparent, is no use to us
                        [[clang::fallthrough]];
                    default:
                        break;
                    }

                }
            // End of if (parameterElements.at(0) == QLatin1String("38"))
            } else if (parameterElements.at(0) == QLatin1String("48")) {
                if (parameterElements.count() >= 2) {
                    decodeSGR48(parameterElements, true);

                } else {
                    // We only have a single element in this parameterString,
                    // so we will need to steal the needed number from the
                    // remainder - this is falling back to using a semicolon
                    // separated list rather than a colon separated one
                    if (paraIndex + 1 >= total) {
                        // Oh dear we are out of parameters to examine, so bail
                        // out:
                        return;
                    }

                    // Okay we have one more parameter at least - so examine it
                    // and grab the needed number of arguments:
                    QStringList madeElements;
                    madeElements << parameterStrings.at(paraIndex);
                    madeElements << parameterStrings.at(paraIndex + 1);
                    bool isOk = false;
                    int sgr48_type = madeElements.at(1).toInt(&isOk);
                    if (madeElements.at(1).isEmpty() || !isOk || sgr48_type == 0) {
                        // Oh dear that parameter is empty or equivalent to zero
                        // so we cannot do anything more
                        return;
                    }

                    switch (sgr48_type) {
                    case 5: // Needs one more number
                        if (paraIndex + 2 < total) {
                            // We have the parameter needed
                            madeElements << parameterStrings.at(paraIndex + 2);
                        }
                        // Move the index to consume the used values less
                        // the one that the for loop will handled - even if it
                        // goes past end
                        decodeSGR48(madeElements, false);
                        paraIndex += 1;
                        break;
                    case 4: // Not handled but we still should skip its arguments
                            // Uses four or five depending on whether there is
                            // the colour space id first
                            // Move the index to consume the used values less
                            // the one that the for loop will handled - even if it
                            // goes past end
                        paraIndex += (haveColorSpaceId ? 6 : 5);
                        break;
                    case 3: // Not handled but we still should skip its arguments
                            // Move the index to consume the used values less
                            // the one that the for loop will handled - even if it
                            // goes past end
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 2: // Need three or four depending on whether there is
                            // the colour space id first
                        if (haveColorSpaceId) {
                            if (paraIndex + 2 < total) {
                                // We have the color space id
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                            if (paraIndex + 5 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 5);
                            }
                        } else {
                            // Fake an empty colour space id
                            madeElements << QString();
                            if (paraIndex + 2 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                        }

                        // Move the index to consume the used values less
                        // the one that the for loop will handled - even if it
                        // goes past end
                        decodeSGR48(madeElements, false);
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 1: // This uses no extra arguments and, as it means
                            // transparent, is no use to us
                        [[clang::fallthrough]];
                    default:
                        break;
                    }

                }
            // End of if (parameterElements.at(0) == QLatin1String("48"))
            } else if (parameterElements.at(0) == QLatin1String("4")) {
                // New way of controlling underline
                bool isOk = false;
                int value = parameterElements.at(1).toInt(&isOk);
                if (!isOk) {
                    // missing value
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence << "\") ERROR - failed to detect underline parameter element (the second part) in a SGR...;4:?;..m sequence assuming it is a zero!";
                }
                switch (value) {
                case 0: // Underline off
                    mUnderline = false;
                    break;
                case 1: // Underline on
                    mUnderline = true;
                    break;
                case 2: // Double underline - not supported, treat as single
                    [[clang::fallthrough]];
                case 3: // Wavey underline - not supported, treat as single
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence << "\") ERROR - unsupported underline parameter element (the second part) in a SGR...;4:" << parameterElements.at(1) << ";../m sequence treating it as a one!";
                    mUnderline = true;
                    break;
                default: // Something unexpected
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence << "\") ERROR - unexpected underline parameter element (the second part) in a SGR...;4:" << parameterElements.at(1) << ";../m sequence treating it as a one!";
                    mUnderline = true;
                    break;
                }
            } else if (parameterElements.at(0) == QLatin1String("3")) {
                // New way of controlling italics
                bool isOk = false;
                int value = parameterElements.at(1).toInt(&isOk);
                if (!isOk) {
                    // missing value
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence << "\") ERROR - failed to detect italic parameter element (the second part) in a SGR...;3:?;../m sequence assuming it is a zero!";
                }
                switch (value) {
                case 0: // Italics/Slant off
                    mItalics = false;
                    break;
                case 1: // Italics on
                    mItalics = true;
                    break;
                case 2: // Slant on - not supported, treat as italics
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence << "\") ERROR - unsupported italic parameter element (the second part) in a SGR...;3:" << parameterElements.at(1) << ";../m sequence treating it as a one!";
                    mUnderline = true;
                    break;
                default: // Something unexpected
                    qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence << "\") ERROR - unexpected italic parameter element (the second part) in a SGR...;3:" << parameterElements.at(1) << ";../m sequence treating it as a one!";
                    mUnderline = true;
                    break;
                }
            } else {
                qDebug().noquote().nospace() << "TBuffer::decodeSGR(\"" << sequence << "\") ERROR - parameter string with an unexpected initial parameter element in a SGR...;" << parameterElements.at(0) << ":" << parameterElements.at(1) << "...;.../m sequence, ignoring it!";
            }
        } else {
            // We do not have a colon separated string so we must just have a
            // number:
            bool isOk = false;
            int tag = allParameterElements.toInt(&isOk);
            if (isOk) {
                switch (tag) {
                case 0:
                    mIsDefaultColor = true;
                    mCurrentFgColorR = pHost->mFgColor.red();
                    mCurrentFgColorG = pHost->mFgColor.green();
                    mCurrentFgColorB = pHost->mFgColor.blue();
                    mCurrentBgColorR = pHost->mBgColor.red();
                    mCurrentBgColorG = pHost->mBgColor.green();
                    mCurrentBgColorB = pHost->mBgColor.blue();
                    mBold = false;
                    mItalics = false;
                    mUnderline = false;
                    mStrikeOut = false;
                    break;
                case 1:
                    mBold = true;
                    break;
                case 2:
                    // Technically this should be faint (i.e. decreased
                    // intensity compared to normal and 22 should be
                    // the reset to "normal" intensity):
                    mBold = false;
                    break;
                case 3:
                    // There is a proposal by the "VTE" terminal
                    // emulator to use a (sub)parameter entry to
                    // destinguish between italics and slanted text by
                    // using ESC[...;3:1;...m and ESC[...;3:2;...m
                    // respectively:
                    mItalics = true;
                    break;
                case 4:
                    mUnderline = true;
                    // There is a implimention by some terminal
                    // emulators ("Kitty" and "VTE") to use a
                    // (sub)parameter entry of 3 for a wavy underline
                    // {presumably 2 would be a double underline and 1
                    // the normal single underline) by sending e.g.:
                    // ESC[...;4:3;...m
                    break;
                case 5:
                    // TODO:
                    break; //slow-blinking
                case 6:
                    // TODO:
                    break; //fast blinking
                case 7:
                    // TODO:
                    break; //inverse
                // case 8: // Concealed characters (set foreground to be the same as background?)
                //    break;
                case 9:
                    mStrikeOut = true;
                    break; //strikethrough
                case 10:
                    break; //default font
                // case 21: // Double underline according to specs
                //    break;
                case 22:
                    mBold = false;
                    break;
                case 23:
                    mItalics = false;
                    break;
                case 24:
                    mUnderline = false;
                    break;
                case 25:
                    break; // blink off
                case 27:
                    // TODO:
                    break; //inverse off
                // case 28: // Revealed characters (undoes the effect of "8")
                //    break;
                case 29:
                    mStrikeOut = false;
                    break; //not crossed out (strikethrough) text
                case 30:
                    mCurrentFgColorR = mBlackR;
                    mCurrentFgColorG = mBlackG;
                    mCurrentFgColorB = mBlackB;
                    mCurrentFgColorLightR = mLightBlackR;
                    mCurrentFgColorLightG = mLightBlackG;
                    mCurrentFgColorLightB = mLightBlackB;
                    mIsDefaultColor = false;
                    break;
                case 31:
                    mCurrentFgColorR = mRedR;
                    mCurrentFgColorG = mRedG;
                    mCurrentFgColorB = mRedB;
                    mCurrentFgColorLightR = mLightRedR;
                    mCurrentFgColorLightG = mLightRedG;
                    mCurrentFgColorLightB = mLightRedB;
                    mIsDefaultColor = false;
                    break;
                case 32:
                    mCurrentFgColorR = mGreenR;
                    mCurrentFgColorG = mGreenG;
                    mCurrentFgColorB = mGreenB;
                    mCurrentFgColorLightR = mLightGreenR;
                    mCurrentFgColorLightG = mLightGreenG;
                    mCurrentFgColorLightB = mLightGreenB;
                    mIsDefaultColor = false;
                    break;
                case 33:
                    mCurrentFgColorR = mYellowR;
                    mCurrentFgColorG = mYellowG;
                    mCurrentFgColorB = mYellowB;
                    mCurrentFgColorLightR = mLightYellowR;
                    mCurrentFgColorLightG = mLightYellowG;
                    mCurrentFgColorLightB = mLightYellowB;
                    mIsDefaultColor = false;
                    break;
                case 34:
                    mCurrentFgColorR = mBlueR;
                    mCurrentFgColorG = mBlueG;
                    mCurrentFgColorB = mBlueB;
                    mCurrentFgColorLightR = mLightBlueR;
                    mCurrentFgColorLightG = mLightBlueG;
                    mCurrentFgColorLightB = mLightBlueB;
                    mIsDefaultColor = false;
                    break;
                case 35:
                    mCurrentFgColorR = mMagentaR;
                    mCurrentFgColorG = mMagentaG;
                    mCurrentFgColorB = mMagentaB;
                    mCurrentFgColorLightR = mLightMagentaR;
                    mCurrentFgColorLightG = mLightMagentaG;
                    mCurrentFgColorLightB = mLightMagentaB;
                    mIsDefaultColor = false;
                    break;
                case 36:
                    mCurrentFgColorR = mCyanR;
                    mCurrentFgColorG = mCyanG;
                    mCurrentFgColorB = mCyanB;
                    mCurrentFgColorLightR = mLightCyanR;
                    mCurrentFgColorLightG = mLightCyanG;
                    mCurrentFgColorLightB = mLightCyanB;
                    mIsDefaultColor = false;
                    break;
                case 37:
                    mCurrentFgColorR = mWhiteR;
                    mCurrentFgColorG = mWhiteG;
                    mCurrentFgColorB = mWhiteB;
                    mCurrentFgColorLightR = mLightWhiteR;
                    mCurrentFgColorLightG = mLightWhiteG;
                    mCurrentFgColorLightB = mLightWhiteB;
                    mIsDefaultColor = false;
                    break;
                case 38: {
                    // We only have single elements so we will need to steal the
                    // needed number from the remainder:
                    if (paraIndex + 1 >= total) {
                        // Oh dear we are out of parameters to examine, so bail
                        // out:
                        return;
                    }

                    // Okay we have one more parameter at least - so examine it
                    // and grab the needed number of arguments:
                    QStringList madeElements;
                    madeElements << parameterStrings.at(paraIndex);
                    madeElements << parameterStrings.at(paraIndex + 1);
                    bool isOk = false;
                    int sgr38_type = madeElements.at(1).toInt(&isOk);
                    if (madeElements.at(1).isEmpty() || !isOk || sgr38_type == 0) {
                        // Oh dear that parameter is empty or equivalent to zero
                        // so we cannot do anything more
                        return;
                    }

                    switch (sgr38_type) {
                    case 5: // Needs one more number
                        if (paraIndex + 2 < total) {
                            // We have the parameter needed
                            madeElements << parameterStrings.at(paraIndex + 2);
                        }
                        // Move the index to consume the used values less
                        // the one that the for loop will handled - even if it
                        // goes past end
                        decodeSGR38(madeElements, false);
                        paraIndex += 1;
                        break;
                    case 4: // Not handled but we still should skip its arguments
                            // Uses four or five depending on whether there is
                            // the colour space id first
                            // Move the index to consume the used values less
                            // the one that the for loop will handled - even if it
                            // goes past end
                        paraIndex += (haveColorSpaceId ? 6 : 5);
                        break;
                    case 3: // Not handled but we still should skip its arguments
                            // Move the index to consume the used values less
                            // the one that the for loop will handled - even if it
                            // goes past end
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 2: // Need three or four depending on whether there is
                            // the colour space id first
                        if (haveColorSpaceId) {
                            if (paraIndex + 2 < total) {
                                // We have the color space id
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                            if (paraIndex + 5 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 5);
                            }
                        } else {
                            // Fake an empty colour space id
                            madeElements << QString();
                            if (paraIndex + 2 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                        }

                        // Move the index to consume the used values LESS
                        // the one that the for loop will handle - even if it
                        // goes past end
                        decodeSGR38(madeElements, false);
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 1: // This uses no extra arguments and, as it means
                            // transparent, is no use to us
                        [[clang::fallthrough]];
                    default:
                        break;
                    }
                }
                    break;
                case 39: //default foreground color
                    mCurrentFgColorR = pHost->mFgColor.red();
                    mCurrentFgColorG = pHost->mFgColor.green();
                    mCurrentFgColorB = pHost->mFgColor.blue();
                    break;
                case 40:
                    mCurrentBgColorR = mBlackR;
                    mCurrentBgColorG = mBlackG;
                    mCurrentBgColorB = mBlackB;
                    break;
                case 41:
                    mCurrentBgColorR = mRedR;
                    mCurrentBgColorG = mRedG;
                    mCurrentBgColorB = mRedB;
                    break;
                case 42:
                    mCurrentBgColorR = mGreenR;
                    mCurrentBgColorG = mGreenG;
                    mCurrentBgColorB = mGreenB;
                    break;
                case 43:
                    mCurrentBgColorR = mYellowR;
                    mCurrentBgColorG = mYellowG;
                    mCurrentBgColorB = mYellowB;
                    break;
                case 44:
                    mCurrentBgColorR = mBlueR;
                    mCurrentBgColorG = mBlueG;
                    mCurrentBgColorB = mBlueB;
                    break;
                case 45:
                    mCurrentBgColorR = mMagentaR;
                    mCurrentBgColorG = mMagentaG;
                    mCurrentBgColorB = mMagentaB;
                    break;
                case 46:
                    mCurrentBgColorR = mCyanR;
                    mCurrentBgColorG = mCyanG;
                    mCurrentBgColorB = mCyanB;
                    break;
                case 47:
                    mCurrentBgColorR = mWhiteR;
                    mCurrentBgColorG = mWhiteG;
                    mCurrentBgColorB = mWhiteB;
                    break;
                case 48: {
                    // We only have single elements so we will need to steal the
                    // needed number from the remainder:
                    if (paraIndex + 1 >= total) {
                        // Oh dear we are out of parameters to examine, so bail
                        // out:
                        return;
                    }

                    // Okay we have one more parameter at least - so examine it
                    // and grab the needed number of arguments:
                    QStringList madeElements;
                    madeElements << parameterStrings.at(paraIndex);
                    madeElements << parameterStrings.at(paraIndex + 1);
                    bool isOk = false;
                    int sgr48_type = madeElements.at(1).toInt(&isOk);
                    if (madeElements.at(1).isEmpty() || !isOk || sgr48_type == 0) {
                        // Oh dear that parameter is empty or equivalent to zero
                        // so we cannot do anything more
                        return;
                    }

                    switch (sgr48_type) {
                    case 5: // Needs one more number
                        if (paraIndex + 2 < total) {
                            // We have the parameter needed
                            madeElements << parameterStrings.at(paraIndex + 2);
                        }
                        // Move the index to consume the used values less
                        // the one that the for loop will handled - even if it
                        // goes past end
                        decodeSGR48(madeElements, false);
                        paraIndex += 1;
                        break;
                    case 4: // Not handled but we still should skip its arguments
                            // Uses four or five depending on whether there is
                            // the colour space id first
                            // Move the index to consume the used values less
                            // the one that the for loop will handled - even if it
                            // goes past end
                        paraIndex += (haveColorSpaceId ? 6 : 5);
                        break;
                    case 3: // Not handled but we still should skip its arguments
                            // Move the index to consume the used values less
                            // the one that the for loop will handled - even if it
                            // goes past end
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 2: // Need three or four depending on whether there is
                            // the colour space id first
                        if (haveColorSpaceId) {
                            if (paraIndex + 2 < total) {
                                // We have the color space id
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                            if (paraIndex + 5 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 5);
                            }
                        } else {
                            // Fake an empty colour space id
                            madeElements << QString();
                            if (paraIndex + 2 < total) {
                                // We have the red component
                                madeElements << parameterStrings.at(paraIndex + 2);
                            }
                            if (paraIndex + 3 < total) {
                                // We have the green component
                                madeElements << parameterStrings.at(paraIndex + 3);
                            }
                            if (paraIndex + 4 < total) {
                                // We have the blue component
                                madeElements << parameterStrings.at(paraIndex + 4);
                            }
                        }

                        // Move the index to consume the used values less
                        // the one that the for loop will handled - even if it
                        // goes past end
                        decodeSGR48(madeElements, false);
                        paraIndex += (haveColorSpaceId ? 5 : 4);
                        break;
                    case 1: // This uses no extra arguments and, as it means
                            // transparent, is no use to us
                        [[clang::fallthrough]];
                    default:
                        break;
                    }
                }
                    break;
                case 49: // default background color
                    mCurrentBgColorR = pHost->mBgColor.red();
                    mCurrentBgColorG = pHost->mBgColor.green();
                    mCurrentBgColorB = pHost->mBgColor.blue();
                    break;
                // case 51: // Framed
                //    break;
                // case 52: // Encircled
                //    break;
                case 53: // overline on
                    // TODO:
                    break;
                // case 54: // Not framed, not encircled
                //    break;
                case 55: // overline off
                    // TODO:
                    break;
                // 56 to 59 reserved for future standardization
                // case 60: // ideogram underline or right side line
                //    break;
                // case 61: // ideogram double underline or double right side line
                //    break;
                // case 62: // ideogram overline or left side line
                //    break;
                // case 63: // ideogram double overline or double left side line
                //    break;
                // case 64: // ideogram stress marking
                //    break;
                // case 65: // cancels the effects of 60 to 64
                //    break;
                };
            }
        }
    }
}

void TBuffer::decodeOSC(const QString& sequence)
{
    Host* pHost = mpHost;
    if (!pHost) {
        qWarning() << "TBuffer::decodeOSC(...) ERROR - Called when mpHost pointer is nullptr";
        return;
    }

    bool serverMayRedefineDefaultColors = pHost->getMayRedefineColors();
#if defined(DEBUG_OSC_PROCESSING)
    qDebug().nospace().noquote() << "    Consider the OSC sequence: \"" << sequence << "\"";
#endif
    unsigned short ch = sequence.at(0).unicode();
    switch (ch) {
    case static_cast<quint8>('P'):
        if (serverMayRedefineDefaultColors) {
            if (sequence.size() == 8) {
                // Should be a 8 byte Hex number in form PIRRGGBB - including the 'P'
                bool isOk = false;
                // Uses mid(...) rather than at(...) because we want the return to
                // be a (single character) QString and not a QChar so we can use
                // QString::toUInt(...):
                quint8 colorNumber = sequence.mid(1,1).toUInt(&isOk, 16);
                quint8 rr = 0;
                if (isOk) {
                    rr = sequence.mid(2, 2).toUInt(&isOk, 16);
                }
                quint8 gg = 0;
                if (isOk) {
                    gg = sequence.mid(4, 2).toUInt(&isOk, 16);
                }
                quint8 bb = 0;
                if (isOk) {
                    bb = sequence.mid(6, 2).toUInt(&isOk, 16);
                }
                if (isOk) {
                    switch (colorNumber) {
                    case 0: // Black
                        pHost->mBlack = QColor(rr, gg, bb);
                        break;
                    case 1: // Red
                        pHost->mRed = QColor(rr, gg, bb);
                        break;
                    case 2: // Green
                        pHost->mGreen = QColor(rr, gg, bb);
                        break;
                    case 3: // Yellow
                        pHost->mYellow = QColor(rr, gg, bb);
                        break;
                    case 4: // Blue
                        pHost->mBlue = QColor(rr, gg, bb);
                        break;
                    case 5: // Magenta
                        pHost->mMagenta = QColor(rr, gg, bb);
                        break;
                    case 6: // Cyan
                        pHost->mCyan = QColor(rr, gg, bb);
                        break;
                    case 7: // Light gray
                        pHost->mWhite = QColor(rr, gg, bb);
                        break;
                    case 8: // Dark gray
                        pHost->mLightBlack = QColor(rr, gg, bb);
                        break;
                    case 9: // Light Red
                        pHost->mLightRed = QColor(rr, gg, bb);
                        break;
                    case 10: // Light Green
                        pHost->mLightGreen = QColor(rr, gg, bb);
                        break;
                    case 11: // Light Yellow
                        pHost->mLightYellow = QColor(rr, gg, bb);
                        break;
                    case 12: // Light Blue
                        pHost->mLightBlue = QColor(rr, gg, bb);
                        break;
                    case 13: // Light Magenta
                        pHost->mLightMagenta = QColor(rr, gg, bb);
                        break;
                    case 14: // Light Cyan
                        pHost->mLightCyan = QColor(rr, gg, bb);
                        break;
                    case 15: // Light gray
                        pHost->mLightWhite = QColor(rr, gg, bb);
                        break;
                    }
                } else {
#if defined(DEBUG_OSC_PROCESSING)
                    qDebug().noquote().nospace() << "TBuffer::decodeOSC(\"" << sequence << "\") ERROR - Unable to parse this as a <OSC>P<I><RR><GG><BB><ST> string to redefined one of the 16 ANSI colors.";
#endif
                }
            } else {
#if defined(DEBUG_OSC_PROCESSING)
                qDebug().noquote().nospace() << "TBuffer::decodeOSC(\"" << sequence << "\") ERROR - Wrong length of string, unable to decode this as a <OSC>P<I><RR><GG><BB><ST> string to redefined one of the 16 ANSI colors.";
#endif
            }
        }
        break;
    case static_cast<quint8>('R'):
        if (serverMayRedefineDefaultColors) {
            resetColors();
        }
        break;
    default:
        qDebug().noquote().nospace() << "TBuffer::decodeOSC(\"" << sequence << "\") ERROR - Unhandled <OSC>?...<ST> code, Mudlet will ignore it.";
    }
}

void TBuffer::resetColors()
{
    Host* pHost = mpHost;
    if (!pHost) {
        qWarning() << "TBuffer::resetColors(...) ERROR - Called when mpHost pointer is nullptr";
        return;
    }

    // These should match the corresponding settings in
    // dlgProfilePreferences::resetColors() :
    pHost->mBlack = Qt::black;
    pHost->mLightBlack = Qt::darkGray;
    pHost->mRed = Qt::darkRed;
    pHost->mLightRed = Qt::red;
    pHost->mGreen = Qt::darkGreen;
    pHost->mLightGreen = Qt::green;
    pHost->mBlue = Qt::darkBlue;
    pHost->mLightBlue = Qt::blue;
    pHost->mYellow = Qt::darkYellow;
    pHost->mLightYellow = Qt::yellow;
    pHost->mCyan = Qt::darkCyan;
    pHost->mLightCyan = Qt::cyan;
    pHost->mMagenta = Qt::darkMagenta;
    pHost->mLightMagenta = Qt::magenta;
    pHost->mWhite = Qt::lightGray;
    pHost->mLightWhite = Qt::white;

    // This will refresh the "main" console as it is only this class instance
    // associated with that one that will call this method from the
    // decodeOSC(...) method:
    if (mudlet::self()->mConsoleMap.contains(pHost)) {
        mudlet::self()->mConsoleMap[pHost]->changeColors();
    }
}

void TBuffer::append(const QString& text,
                     int sub_start,
                     int sub_end,
                     int fgColorR,
                     int fgColorG,
                     int fgColorB,
                     int bgColorR,
                     int bgColorG,
                     int bgColorB,
                     bool bold,
                     bool italics,
                     bool underline,
                     bool strikeout,
                     int linkID)
{
    // CHECK: What about other Unicode line breaks, e.g. soft-hyphen:
    const QString lineBreaks = QStringLiteral(",.- ");

    if (static_cast<int>(buffer.size()) > mLinesLimit) {
        shrinkBuffer();
    }
    int last = buffer.size() - 1;
    if (last < 0) {
        std::deque<TChar> newLine;
        TChar c(fgColorR, fgColorG, fgColorB, bgColorR, bgColorG, bgColorB, bold, italics, underline, strikeout);
        if (mEchoText) {
            c.flags |= TCHAR_ECHO;
        }
        newLine.push_back(c);
        buffer.push_back(newLine);
        lineBuffer.push_back(QString());
        timeBuffer << QTime::currentTime().toString(QStringLiteral("hh:mm:ss.zzz   "));
        promptBuffer << false;
        dirty << true;
        last = 0;
    }
    bool firstChar = (lineBuffer.back().size() == 0);
    int length = text.size();
    if (length < 1) {
        return;
    }
    if (sub_end >= length) {
        sub_end = text.size() - 1;
    }

    for (int i = sub_start; i < length; i++) { //FIXME <=substart+sub_end muss nachsehen, ob wirklich noch teilbereiche gebraucht werden
        if (text.at(i) == '\n') {
            log(size() - 1, size() - 1);
            std::deque<TChar> newLine;
            buffer.push_back(newLine);
            lineBuffer.push_back(QString());
            timeBuffer << QStringLiteral("-------------");
            promptBuffer << false;
            dirty << true;
            mLastLine++;
            newLines++;
            firstChar = true;
            continue;
        }

        // FIXME: (I18n) Need to measure painted line width and compare that
        // to "unit" character width (whatever we work THAT out to be)
        // multiplied by mWrap:
        if (lineBuffer.back().size() >= mWrapAt) {
            for (int i = lineBuffer.back().size() - 1; i >= 0; i--) {
                if (lineBreaks.indexOf(lineBuffer.back().at(i)) > -1) {
                    QString tmp = lineBuffer.back().mid(0, i + 1);
                    QString lineRest = lineBuffer.back().mid(i + 1);
                    lineBuffer.back() = tmp;
                    std::deque<TChar> newLine;

                    int k = lineRest.size();
                    if (k > 0) {
                        while (k > 0) {
                            newLine.push_front(buffer.back().back());
                            buffer.back().pop_back();
                            k--;
                        }
                    }

                    buffer.push_back(newLine);
                    if (lineRest.size() > 0) {
                        lineBuffer.append(lineRest);
                    } else {
                        lineBuffer.append(QString());
                    }
                    timeBuffer << QStringLiteral("-------------");
                    promptBuffer << false;
                    dirty << true;
                    mLastLine++;
                    newLines++;
                    log(size() - 2, size() - 2);
                    // Was absent causing loss of all but last line of wrapped
                    // long lines of user input and some other console displayed
                    // text from log file.
                    break;
                }
            }
        }
        lineBuffer.back().append(text.at(i));
        TChar c(fgColorR, fgColorG, fgColorB, bgColorR, bgColorG, bgColorB, bold, italics, underline, strikeout, linkID);
        if (mEchoText) {
            c.flags |= TCHAR_ECHO;
        }
        buffer.back().push_back(c);
        if (firstChar) {
            timeBuffer.back() = QTime::currentTime().toString(QStringLiteral("hh:mm:ss.zzz   "));
            firstChar = false;
        }
    }
}

void TBuffer::appendLine(const QString& text,
                         int sub_start,
                         int sub_end,
                         int fgColorR,
                         int fgColorG,
                         int fgColorB,
                         int bgColorR,
                         int bgColorG,
                         int bgColorB,
                         bool bold,
                         bool italics,
                         bool underline,
                         bool strikeout,
                         int linkID)
{
    if (sub_end < 0) {
        return;
    }
    if (static_cast<int>(buffer.size()) > mLinesLimit) {
        shrinkBuffer();
    }
    int last = buffer.size() - 1;
    if (last < 0) {
        std::deque<TChar> newLine;
        TChar c(fgColorR, fgColorG, fgColorB, bgColorR, bgColorG, bgColorB, bold, italics, underline, strikeout);
        if (mEchoText) {
            c.flags |= TCHAR_ECHO;
        }
        newLine.push_back(c);
        buffer.push_back(newLine);
        lineBuffer.push_back(QString());
        timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
        promptBuffer << false;
        dirty << true;
        last = 0;
    }
    bool firstChar = (lineBuffer.back().size() == 0);
    int length = text.size();
    if (length < 1) {
        return;
    }
    if (sub_end >= length) {
        sub_end = text.size() - 1;
    }

    for (int i = sub_start; i <= (sub_start + sub_end); i++) {
        lineBuffer.back().append(text.at(i));
        TChar c(fgColorR, fgColorG, fgColorB, bgColorR, bgColorG, bgColorB, bold, italics, underline, strikeout, linkID);
        if (mEchoText) {
            c.flags |= TCHAR_ECHO;
        }
        buffer.back().push_back(c);
        if (firstChar) {
            timeBuffer.back() = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
        }
    }
}

QPoint TBuffer::insert(QPoint& where, const QString& text, int fgColorR, int fgColorG, int fgColorB, int bgColorR, int bgColorG, int bgColorB, bool bold, bool italics, bool underline, bool strikeout)
{
    QPoint P(-1, -1);

    int x = where.x();
    int y = where.y();

    if (y < 0) {
        return P;
    }
    if (y >= static_cast<int>(buffer.size())) {
        return P;
    }


    for (auto character : text) {
        if (character == QChar('\n')) {
            std::deque<TChar> newLine;
            TChar c(fgColorR, fgColorG, fgColorB, bgColorR, bgColorG, bgColorB, bold, italics, underline, strikeout);
            newLine.push_back(c);
            buffer.push_back(newLine);
            promptBuffer.insert(y, false);
            const QString nothing = "";
            lineBuffer.insert(y, nothing);
            timeBuffer << "-->"; //this is intentional -> faster
            dirty.insert(y, true);
            mLastLine++;
            newLines++;
            x = 0;
            y++;
            continue;
        }
        lineBuffer[y].insert(x, character);
        TChar c(fgColorR, fgColorG, fgColorB, bgColorR, bgColorG, bgColorB, bold, italics, underline, strikeout);
        auto it = buffer[y].begin();
        buffer[y].insert(it + x, c);
    }
    dirty[y] = true;
    P.setX(x);
    P.setY(y);
    return P;
}


bool TBuffer::insertInLine(QPoint& P, const QString& text, TChar& format)
{
    if (text.size() < 1) {
        return false;
    }
    int x = P.x();
    int y = P.y();
    if ((y >= 0) && (y < static_cast<int>(buffer.size()))) {
        if (x < 0) {
            return false;
        }
        if (x >= static_cast<int>(buffer[y].size())) {
            TChar c;
            expandLine(y, x - buffer[y].size(), c);
        }
        for (int i = 0; i < text.size(); i++) {
            lineBuffer[y].insert(x + i, text.at(i));
            TChar c = format;
            auto it = buffer[y].begin();
            buffer[y].insert(it + x + i, c);
        }
    } else {
        appendLine(text,
                   0,
                   text.size(),
                   format.fgR,
                   format.fgG,
                   format.fgB,
                   format.bgR,
                   format.bgG,
                   format.bgB,
                   format.flags & TCHAR_BOLD,
                   format.flags & TCHAR_ITALICS,
                   format.flags & TCHAR_UNDERLINE,
                   format.flags & TCHAR_STRIKEOUT);
    }
    return true;
}

TBuffer TBuffer::copy(QPoint& P1, QPoint& P2)
{
    TBuffer slice(mpHost);
    slice.clear();
    int y = P1.y();
    int x = P1.x();
    if (y < 0 || y >= static_cast<int>(buffer.size())) {
        return slice;
    }

    if ((x < 0) || (x >= static_cast<int>(buffer[y].size())) || (P2.x() < 0) || (P2.x() > static_cast<int>(buffer[y].size()))) {
        x = 0;
    }
    for (; x < P2.x(); x++) {
        QString s(lineBuffer[y][x]);
        slice.append(s,
                     0,
                     1,
                     buffer[y][x].fgR,
                     buffer[y][x].fgG,
                     buffer[y][x].fgB,
                     buffer[y][x].bgR,
                     buffer[y][x].bgG,
                     buffer[y][x].bgB,
                     (buffer[y][x].flags & TCHAR_BOLD),
                     (buffer[y][x].flags & TCHAR_ITALICS),
                     (buffer[y][x].flags & TCHAR_UNDERLINE),
                     (buffer[y][x].flags & TCHAR_STRIKEOUT));
    }
    return slice;
}

TBuffer TBuffer::cut(QPoint& P1, QPoint& P2)
{
    TBuffer slice = copy(P1, P2);
    QString nothing = "";
    TChar format;
    replaceInLine(P1, P2, nothing, format);
    return slice;
}

void TBuffer::paste(QPoint& P, TBuffer chunk)
{
    bool needAppend = false;
    bool hasAppended = false;
    int y = P.y();
    int x = P.x();
    if (chunk.buffer.empty()) {
        return;
    }
    if (y < 0 || y > getLastLineNumber()) {
        y = getLastLineNumber();
    }
    if (y == -1) {
        needAppend = true;
    } else {
        if (x < 0 || x >= static_cast<int>(buffer[y].size())) {
            return;
        }
    }
    for (int cx = 0; cx < static_cast<int>(chunk.buffer[0].size()); cx++) {
        QPoint P_current(cx, y);
        if ((y < getLastLineNumber()) && (!needAppend)) {
            TChar& format = chunk.buffer[0][cx];
            QString s = QString(chunk.lineBuffer[0][cx]);
            insertInLine(P_current, s, format);
        } else {
            hasAppended = true;
            QString s(chunk.lineBuffer[0][cx]);
            append(s,
                   0,
                   1,
                   chunk.buffer[0][cx].fgR,
                   chunk.buffer[0][cx].fgG,
                   chunk.buffer[0][cx].fgB,
                   chunk.buffer[0][cx].bgR,
                   chunk.buffer[0][cx].bgG,
                   chunk.buffer[0][cx].bgB,
                   (chunk.buffer[0][cx].flags & TCHAR_BOLD),
                   (chunk.buffer[0][cx].flags & TCHAR_ITALICS),
                   (chunk.buffer[0][cx].flags & TCHAR_UNDERLINE),
                   (chunk.buffer[0][cx].flags & TCHAR_STRIKEOUT));
        }
    }
    if (hasAppended) {
        TChar format;
        if (y == -1) {
        } else {
            wrapLine(y, mWrapAt, mWrapIndent, format);
        }
    }
}

void TBuffer::appendBuffer(const TBuffer& chunk)
{
    if (chunk.buffer.empty()) {
        return;
    }
    for (int cx = 0; cx < static_cast<int>(chunk.buffer[0].size()); cx++) {
        QString s(chunk.lineBuffer[0][cx]);
        append(s,
               0,
               1,
               chunk.buffer[0][cx].fgR,
               chunk.buffer[0][cx].fgG,
               chunk.buffer[0][cx].fgB,
               chunk.buffer[0][cx].bgR,
               chunk.buffer[0][cx].bgG,
               chunk.buffer[0][cx].bgB,
               (chunk.buffer[0][cx].flags & TCHAR_BOLD),
               (chunk.buffer[0][cx].flags & TCHAR_ITALICS),
               (chunk.buffer[0][cx].flags & TCHAR_UNDERLINE),
               (chunk.buffer[0][cx].flags & TCHAR_STRIKEOUT));
    }
    QString lf = "\n";
    append(lf, 0, 1, 0, 0, 0, 0, 0, 0, false, false, false, false);
}

int TBuffer::calcWrapPos(int line, int begin, int end)
{
    const QString lineBreaks = ",.- \n";
    if (lineBuffer.size() < line) {
        return 0;
    }
    int lineSize = static_cast<int>(lineBuffer[line].size()) - 1;
    if (lineSize < end) {
        end = lineSize;
    }
    for (int i = end; i >= begin; i--) {
        if (lineBreaks.indexOf(lineBuffer[line].at(i)) > -1) {
            return i;
        }
    }
    return 0;
}

inline int TBuffer::skipSpacesAtBeginOfLine(int i, int i2)
{
    int offset = 0;
    int i_end = lineBuffer[i].size();
    QChar space = ' ';
    while (i2 < i_end) {
        if (buffer[i][i2].flags & TCHAR_ECHO) {
            break;
        }
        if (lineBuffer[i][i2] == space) {
            offset++;
        } else {
            break;
        }
        i2++;
    }
    return offset;
}

inline int TBuffer::wrap(int startLine)
{
    if (static_cast<int>(buffer.size()) < startLine || startLine < 0) {
        return 0;
    }
    std::queue<std::deque<TChar>> queue;
    QStringList tempList;
    QStringList timeList;
    QList<bool> promptList;
    int lineCount = 0;
    for (int i = startLine; i < static_cast<int>(buffer.size()); i++) {
        bool isPrompt = promptBuffer[i];
        std::deque<TChar> newLine;
        QString lineText = "";
        QString time = timeBuffer[i];
        int indent = 0;
        if (static_cast<int>(buffer[i].size()) >= mWrapAt) {
            for (int i3 = 0; i3 < mWrapIndent; i3++) {
                TChar pSpace;
                newLine.push_back(pSpace);
                lineText.append(" ");
            }
            indent = mWrapIndent;
        }
        int lastSpace = 0;
        int wrapPos = 0;
        int length = buffer[i].size();
        if (length == 0) {
            tempList.append(QString());
            std::deque<TChar> emptyLine;
            queue.push(emptyLine);
            timeList.append(time);
        }
        for (int i2 = 0; i2 < static_cast<int>(buffer[i].size());) {
            if (length - i2 > mWrapAt - indent) {
                wrapPos = calcWrapPos(i, i2, i2 + mWrapAt - indent);
                lastSpace = qMax(0, wrapPos);
            } else {
                lastSpace = 0;
            }
            int wrapPosition = (lastSpace) ? lastSpace : (mWrapAt - indent);
            for (int i3 = 0; i3 < wrapPosition; i3++) {
                if (lastSpace > 0) {
                    if (i2 > lastSpace) {
                        break;
                    }
                }
                if (i2 >= static_cast<int>(buffer[i].size())) {
                    break;
                }
                if (lineBuffer[i].at(i2) == '\n') {
                    i2++;
                    break;
                }
                newLine.push_back(buffer[i][i2]);
                lineText.append(lineBuffer[i].at(i2));
                i2++;
            }
            if (newLine.empty()) {
                tempList.append(QString());
                std::deque<TChar> emptyLine;
                queue.push(emptyLine);
                timeList.append(QString());
                promptList.append(false);
            } else {
                queue.push(newLine);
                tempList.append(lineText);
                timeList.append(time);
                promptList.append(isPrompt);
            }
            newLine.clear();
            lineText = "";
            indent = 0;
            i2 += skipSpacesAtBeginOfLine(i, i2);
        }
        lineCount++;
    }
    for (int i = 0; i < lineCount; i++) {
        buffer.pop_back();
        lineBuffer.pop_back();
        timeBuffer.pop_back();
        promptBuffer.pop_back();
        dirty.pop_back();
    }

    newLines -= lineCount;
    newLines += queue.size();
    int insertedLines = queue.size() - 1;
    while (!queue.empty()) {
        buffer.push_back(queue.front());
        queue.pop();
    }
    for (int i = 0; i < tempList.size(); i++) {
        if (tempList[i].size() < 1) {
            lineBuffer.append(QString());
            timeBuffer.append(QString());
            promptBuffer.push_back(false);
        } else {
            lineBuffer.append(tempList[i]);
            timeBuffer.append(timeList[i]);
            promptBuffer.push_back(promptList[i]);
        }
        dirty.push_back(true);
    }

    log(startLine, startLine + tempList.size());
    return insertedLines > 0 ? insertedLines : 0;
}

void TBuffer::log(int fromLine, int toLine)
{
    TBuffer* pB = &mpHost->mpConsole->buffer;
    if (pB != this || !mpHost->mpConsole->mLogToLogFile) {
        return;
    }

    if (fromLine >= size() || fromLine < 0) {
        return;
    }
    if (toLine >= size()) {
        toLine = size() - 1;
    }
    if (toLine < 0) {
        return;
    }

    // if we've been called to log the same line - which can happen when the user
    // enters a command after in-game text - then skip recording the last line
    if (fromLine != lastLoggedFromLine && toLine != lastloggedToLine) {
        mpHost->mpConsole->mLogStream << lastTextToLog;
        mpHost->mpConsole->mLogStream.flush();
    }

    QStringList linesToLog;
    for (int i = fromLine; i <= toLine; i++) {
        if (mpHost->mIsCurrentLogFileInHtmlFormat) {
            // This only handles a single line of logged text at a time:
            linesToLog << bufferToHtml(QPoint(0, i), QPoint(buffer.at(i).size(), i), mpHost->mIsLoggingTimestamps);
        } else {
            linesToLog << ((mpHost->mIsLoggingTimestamps && !timeBuffer.at(i).isEmpty()) ? timeBuffer.at(i).left(13) : QString()) % lineBuffer.at(i) % QChar::LineFeed;
        }
    }

    // record the last log call into a temporary buffer - we'll actually log
    // on the next iteration after duplication detection has run
    lastTextToLog = std::move(linesToLog.join(QString()));
    lastLoggedFromLine = fromLine;
    lastloggedToLine = toLine;
}

// logs the remaining output when logging gets stopped, without duplication checks
void TBuffer::logRemainingOutput()
{
    mpHost->mpConsole->mLogStream << lastTextToLog;
    mpHost->mpConsole->mLogStream.flush();
}

// returns how many new lines have been inserted by the wrapping action
int TBuffer::wrapLine(int startLine, int screenWidth, int indentSize, TChar& format)
{
    if (startLine < 0) {
        return 0;
    }
    if (static_cast<int>(buffer.size()) <= startLine) {
        return 0;
    }
    std::queue<std::deque<TChar>> queue;
    QStringList tempList;
    int lineCount = 0;

    for (int i = startLine; i < static_cast<int>(buffer.size()); i++) {
        if (i > startLine) {
            break; //only wrap one line of text
        }
        std::deque<TChar> newLine;
        QString lineText;

        int indent = 0;
        if (static_cast<int>(buffer[i].size()) >= screenWidth) {
            for (int i3 = 0; i3 < indentSize; i3++) {
                TChar pSpace = format;
                newLine.push_back(pSpace);
                lineText.append(" ");
            }
            indent = indentSize;
        }
        int lastSpace = -1;
        int wrapPos = -1;
        auto length = static_cast<int>(buffer[i].size());

        for (int i2 = 0; i2 < static_cast<int>(buffer[i].size());) {
            if (length - i2 > screenWidth - indent) {
                wrapPos = calcWrapPos(i, i2, i2 + screenWidth - indent);
                lastSpace = qMax(-1, wrapPos);
            } else {
                lastSpace = -1;
            }
            for (int i3 = 0; i3 < screenWidth - indent; i3++) {
                if (lastSpace > 0) {
                    if (i2 >= lastSpace) {
                        i2++;
                        break;
                    }
                }
                if (i2 >= static_cast<int>(buffer[i].size())) {
                    break;
                }
                if (lineBuffer[i][i2] == QChar('\n')) {
                    i2++;

                    if (newLine.empty()) {
                        tempList.append(QString());
                        std::deque<TChar> emptyLine;
                        queue.push(emptyLine);
                    } else {
                        queue.push(newLine);
                        tempList.append(lineText);
                    }
                    goto OPT_OUT_CLEAN;
                }
                newLine.push_back(buffer[i][i2]);
                lineText.append(lineBuffer[i].at(i2));
                i2++;
            }
            queue.push(newLine);
            tempList.append(lineText);

        OPT_OUT_CLEAN:
            newLine.clear();
            lineText.clear();
            indent = 0;
        }
        lineCount++;
    }

    if (lineCount < 1) {
        log(startLine, startLine);
        return 0;
    }

    buffer.erase(buffer.begin() + startLine);
    lineBuffer.removeAt(startLine);
    QString time = timeBuffer.at(startLine);
    timeBuffer.removeAt(startLine);
    bool isPrompt = promptBuffer.at(startLine);
    promptBuffer.removeAt(startLine);
    dirty.removeAt(startLine);

    int insertedLines = queue.size() - 1;
    int i = 0;
    while (!queue.empty()) {
        buffer.insert(buffer.begin() + startLine + i, queue.front());
        queue.pop();
        i++;
    }

    for (int i = 0; i < tempList.size(); i++) {
        lineBuffer.insert(startLine + i, tempList[i]);
        timeBuffer.insert(startLine + i, time);
        promptBuffer.insert(startLine + i, isPrompt);
        dirty.insert(startLine + i, true);
    }
    log(startLine, startLine + tempList.size() - 1);
    return insertedLines > 0 ? insertedLines : 0;
}


bool TBuffer::moveCursor(QPoint& where)
{
    int x = where.x();
    int y = where.y();
    if (y < 0) {
        return false;
    }
    if (y >= static_cast<int>(buffer.size())) {
        return false;
    }

    if (static_cast<int>(buffer[y].size()) - 1 > x) {
        TChar c;
        expandLine(y, x - buffer[y].size() - 1, c);
    }
    return true;
}

QString badLineError = QString("ERROR: invalid line number");


QString& TBuffer::line(int n)
{
    if ((n >= lineBuffer.size()) || (n < 0)) {
        return badLineError;
    }
    return lineBuffer[n];
}


int TBuffer::find(int line, const QString& what, int pos = 0)
{
    if (lineBuffer[line].size() >= pos) {
        return -1;
    }
    if (pos < 0) {
        return -1;
    }
    if ((line >= static_cast<int>(buffer.size())) || (line < 0)) {
        return -1;
    }
    return lineBuffer[line].indexOf(what, pos);
}


QStringList TBuffer::split(int line, const QString& splitter)
{
    if ((line >= static_cast<int>(buffer.size())) || (line < 0)) {
        return QStringList();
    }
    return lineBuffer[line].split(splitter);
}


QStringList TBuffer::split(int line, QRegularExpression splitter)
{
    if ((line >= static_cast<int>(buffer.size())) || (line < 0)) {
        return QStringList();
    }
    return lineBuffer[line].split(splitter);
}


void TBuffer::expandLine(int y, int count, TChar& pC)
{
    int size = buffer[y].size() - 1;
    for (int i = size; i < size + count; i++) {
        buffer[y].push_back(pC);
        lineBuffer[y].append(" ");
    }
}

bool TBuffer::replaceInLine(QPoint& P_begin, QPoint& P_end, const QString& with, TChar& format)
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();
    if ((y1 >= static_cast<int>(buffer.size())) || (y2 >= static_cast<int>(buffer.size()))) {
        return false;
    }
    if ((x2 > static_cast<int>(buffer[y2].size())) || (x1 > static_cast<int>(buffer[y1].size()))) {
        return false;
    }
    if (x1 < 0 || x2 < 0) {
        return false;
    }

    int xb, xe, yb, ye;
    if (y1 <= y2) {
        yb = y1;
        ye = y2;
        xb = x1;
        xe = x2;
    } else {
        yb = y2;
        ye = y1;
        xb = x2;
        xe = x1;
    }

    for (int y = yb; y <= ye; y++) {
        int x = 0;
        if (y == yb) {
            x = xb;
        }
        int x_end = buffer[y].size() - 1;
        if (y == ye) {
            x_end = xe;
        }
        lineBuffer[y].remove(x, x_end - x);
        auto it1 = buffer[y].begin() + x;
        auto it2 = buffer[y].begin() + x_end;
        buffer[y].erase(it1, it2);
    }

    // insert replacement
    insertInLine(P_begin, with, format);
    return true;
}

void TBuffer::clear()
{
    while (!buffer.empty()) {
        if (!deleteLines(0, 0)) {
            break;
        }
    }
    std::deque<TChar> newLine;
    buffer.push_back(newLine);
    lineBuffer << QString();
    timeBuffer << QString();
    promptBuffer.push_back(false);
    dirty.push_back(true);
}

bool TBuffer::deleteLine(int y)
{
    return deleteLines(y, y);
}

void TBuffer::shrinkBuffer()
{
    for (int i = 0; i < mBatchDeleteSize; i++) {
        lineBuffer.pop_front();
        promptBuffer.pop_front();
        timeBuffer.pop_front();
        dirty.pop_front();
        buffer.pop_front();
        mCursorY--;
    }
}

bool TBuffer::deleteLines(int from, int to)
{
    if ((from >= 0) && (from < static_cast<int>(buffer.size())) && (from <= to) && (to >= 0) && (to < static_cast<int>(buffer.size()))) {
        int delta = to - from + 1;

        for (int i = from; i < from + delta; i++) {
            lineBuffer.removeAt(i);
            timeBuffer.removeAt(i);
            promptBuffer.removeAt(i);
            dirty.removeAt(i);
        }

        buffer.erase(buffer.begin() + from, buffer.begin() + to + 1);
        return true;
    } else {
        return false;
    }
}


bool TBuffer::applyFormat(QPoint& P_begin, QPoint& P_end, TChar& format)
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();

    if ((x1 >= 0) && ((y2 < static_cast<int>(buffer.size())) && (y2 >= 0)) && ((x2 > x1) || (y2 > y1)) && (x1 < static_cast<int>(buffer[y1].size())))
    // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
    // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for (int y = y1; y <= y2; y++) {
            int x = 0;
            if (y == y1) {
                x = x1;
            }
            while (x < static_cast<int>(buffer[y].size())) {
                if (y >= y2) {
                    if (x >= x2) {
                        return true;
                    }
                }

                buffer[y][x] = format;
                x++;
            }
        }
        return true;
    } else {
        return false;
    }
}

bool TBuffer::applyLink(QPoint& P_begin, QPoint& P_end, const QString& linkText, QStringList& linkFunction, QStringList& linkHint)
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();
    bool incLinkID = false;
    int linkID = 0;
    if ((x1 >= 0) && ((y2 < static_cast<int>(buffer.size())) && (y2 >= 0)) && ((x2 > x1) || (y2 > y1)) && (x1 < static_cast<int>(buffer[y1].size())))
    // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
    // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for (int y = y1; y <= y2; y++) {
            int x = 0;
            if (y == y1) {
                x = x1;
            }
            while (x < static_cast<int>(buffer[y].size())) {
                if (y >= y2) {
                    if (x >= x2) {
                        return true;
                    }
                }
                if (!incLinkID) {
                    incLinkID = true;
                    mLinkID++;
                    linkID = mLinkID;
                    if (mLinkID > 1000) {
                        mLinkID = 1;
                    }
                    mLinkStore[mLinkID] = linkFunction;
                    mHintStore[mLinkID] = linkHint;
                }
                buffer[y][x].link = linkID;
                x++;
            }
        }
        return true;
    } else {
        return false;
    }
}

bool TBuffer::applyBold(QPoint& P_begin, QPoint& P_end, bool bold)
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();

    if ((x1 >= 0) && ((y2 < static_cast<int>(buffer.size())) && (y2 >= 0)) && ((x2 > x1) || (y2 > y1)) && (x1 < static_cast<int>(buffer[y1].size())))
    // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
    // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for (int y = y1; y <= y2; y++) {
            int x = 0;
            if (y == y1) {
                x = x1;
            }
            while (x < static_cast<int>(buffer[y].size())) {
                if (y >= y2) {
                    if (x >= x2) {
                        return true;
                    }
                }
                if (bold) {
                    buffer[y][x].flags |= TCHAR_BOLD;
                } else {
                    buffer[y][x].flags &= ~(TCHAR_BOLD);
                }
                x++;
            }
        }
        return true;
    } else {
        return false;
    }
}

bool TBuffer::applyItalics(QPoint& P_begin, QPoint& P_end, bool bold)
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();

    if ((x1 >= 0) && ((y2 < static_cast<int>(buffer.size())) && (y2 >= 0)) && ((x2 > x1) || (y2 > y1)) && (x1 < static_cast<int>(buffer[y1].size())))
    // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
    // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for (int y = y1; y <= y2; y++) {
            int x = 0;
            if (y == y1) {
                x = x1;
            }
            while (x < static_cast<int>(buffer[y].size())) {
                if (y >= y2) {
                    if (x >= x2) {
                        return true;
                    }
                }
                if (bold) {
                    buffer[y][x].flags |= TCHAR_ITALICS;
                } else {
                    buffer[y][x].flags &= ~(TCHAR_ITALICS);
                }
                x++;
            }
        }
        return true;
    } else {
        return false;
    }
}

bool TBuffer::applyUnderline(QPoint& P_begin, QPoint& P_end, bool bold)
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();

    if ((x1 >= 0) && ((y2 < static_cast<int>(buffer.size())) && (y2 >= 0)) && ((x2 > x1) || (y2 > y1)) && (x1 < static_cast<int>(buffer[y1].size())))
    // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
    // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for (int y = y1; y <= y2; y++) {
            int x = 0;
            if (y == y1) {
                x = x1;
            }
            while (x < static_cast<int>(buffer[y].size())) {
                if (y >= y2) {
                    if (x >= x2) {
                        return true;
                    }
                }

                if (bold) {
                    buffer[y][x].flags |= TCHAR_UNDERLINE;
                } else {
                    buffer[y][x].flags &= ~(TCHAR_UNDERLINE);
                }
                x++;
            }
        }
        return true;
    } else {
        return false;
    }
}

bool TBuffer::applyStrikeOut(QPoint& P_begin, QPoint& P_end, bool strikeout)
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();

    if ((x1 >= 0) && ((y2 < static_cast<int>(buffer.size())) && (y2 >= 0)) && ((x2 > x1) || (y2 > y1)) && (x1 < static_cast<int>(buffer[y1].size())))
    // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
    // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for (int y = y1; y <= y2; y++) {
            int x = 0;
            if (y == y1) {
                x = x1;
            }
            while (x < static_cast<int>(buffer[y].size())) {
                if (y >= y2) {
                    if (x >= x2) {
                        return true;
                    }
                }

                if (strikeout) {
                    buffer[y][x].flags |= TCHAR_STRIKEOUT;
                } else {
                    buffer[y][x].flags &= ~(TCHAR_STRIKEOUT);
                }
                x++;
            }
        }
        return true;
    } else {
        return false;
    }
}

bool TBuffer::applyFgColor(QPoint& P_begin, QPoint& P_end, int fgColorR, int fgColorG, int fgColorB)
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();

    if ((x1 >= 0) && ((y2 < static_cast<int>(buffer.size())) && (y2 >= 0)) && ((x2 > x1) || (y2 > y1)) && (x1 < static_cast<int>(buffer[y1].size())))
    // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
    // && ( x2 < static_cast<int>(buffer[y2].size()) ) )

    {
        for (int y = y1; y <= y2; y++) {
            int x = 0;
            if (y == y1) {
                x = x1;
            }
            while (x < static_cast<int>(buffer[y].size())) {
                if (y >= y2) {
                    if (x >= x2) {
                        return true;
                    }
                }

                buffer[y][x].fgR = fgColorR;
                buffer[y][x].fgG = fgColorG;
                buffer[y][x].fgB = fgColorB;
                x++;
            }
        }
        return true;
    } else {
        return false;
    }
}

bool TBuffer::applyBgColor(QPoint& P_begin, QPoint& P_end, int bgColorR, int bgColorG, int bgColorB)
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();
    if ((x1 >= 0) && ((y2 < static_cast<int>(buffer.size())) && (y2 >= 0)) && ((x2 > x1) || (y2 > y1)) && (x1 < static_cast<int>(buffer[y1].size())))
    // even if the end selection is out of bounds we still apply the format until the end of the line to simplify and ultimately speed up user scripting (no need to calc end of line)
    // && ( x2 < static_cast<int>(buffer[y2].size()) ) )
    {
        for (int y = y1; y <= y2; y++) {
            int x = 0;
            if (y == y1) {
                x = x1;
            }
            while (x < static_cast<int>(buffer[y].size())) {
                if (y >= y2) {
                    if (x >= x2) {
                        return true;
                    }
                }

                (buffer[y][x]).bgR = bgColorR;
                (buffer[y][x]).bgG = bgColorG;
                (buffer[y][x]).bgB = bgColorB;
                x++;
            }
        }
        return true;
    } else {
        return false;
    }
}

QStringList TBuffer::getEndLines(int n)
{
    QStringList linesList;
    for (int i = getLastLineNumber() - n; i < getLastLineNumber(); i++) {
        linesList << line(i);
    }
    return linesList;
}


QString TBuffer::bufferToHtml(QPoint P1, QPoint P2, bool allowedTimestamps, int spacePadding)
{
    int y = P1.y();
    int x = P1.x();
    QString s;
    if (y < 0 || y >= static_cast<int>(buffer.size())) {
        return s;
    }

    if ((x < 0) || (x >= static_cast<int>(buffer[y].size())) || (P2.x() >= static_cast<int>(buffer[y].size()))) {
        x = 0;
    }
    if (P2.x() < 0) {
        P2.setX(buffer[y].size());
    }

    bool bold = false;
    bool italics = false;
    bool underline = false;
    //    bool overline = false;
    bool strikeout = false;
    //    bool inverse = false;
    int fgR = 0;
    int fgG = 0;
    int fgB = 0;
    int bgR = 0;
    int bgG = 0;
    int bgB = 0;
    // This combination of color values (black on black) cannot usefully be used in practice
    // - so use as initialization values
    QString fontWeight;
    QString fontStyle;
    QString textDecoration;
    bool firstSpan = true;
    if (allowedTimestamps && !timeBuffer[y].isEmpty()) {
        firstSpan = false;
        // formatting according to TTextEdit.cpp: if( i2 < timeOffset )
        s.append(R"(<span style="color: rgb(200,150,0); background: rgb(22,22,22); )");
        s.append(R"(font-weight: normal; font-style: normal; text-decoration: normal">)");
        s.append(timeBuffer[y].leftRef(13));
    }
    if (spacePadding > 0) {
        // used for "copy HTML", first line of selection
        if (firstSpan) {
            firstSpan = false;
        } else {
            s.append("</span>");
        }
        s.append("<span>");
        s.append(QString(spacePadding, QLatin1Char(' ')));
        // Pad out with spaces to the right so a partial first line lines up
    }
    for (; x < P2.x(); x++) {
        if (x >= static_cast<int>(buffer[y].size())) {
            break;
        }
        if (firstSpan || buffer[y][x].fgR != fgR || buffer[y][x].fgG != fgG || buffer[y][x].fgB != fgB || buffer[y][x].bgR != bgR || buffer[y][x].bgG != bgG || buffer[y][x].bgB != bgB
            || bool(buffer[y][x].flags & TCHAR_BOLD) != bold
            || bool(buffer[y][x].flags & TCHAR_UNDERLINE) != underline
            || bool(buffer[y][x].flags & TCHAR_ITALICS) != italics
            || bool(buffer[y][x].flags & TCHAR_STRIKEOUT) != strikeout
            //            || bool( buffer[y][x].flags & TCHAR_OVERLINE ) != overline
            //            || bool( buffer[y][x].flags & TCHAR_INVERSE ) != inverse
            ) { // Can leave this on a separate line until line above uncommented.
            if (firstSpan) {
                firstSpan = false; // The first span won't need to close the previous one
            } else {
                s += "</span>";
            }
            fgR = buffer[y][x].fgR;
            fgG = buffer[y][x].fgG;
            fgB = buffer[y][x].fgB;
            bgR = buffer[y][x].bgR;
            bgG = buffer[y][x].bgG;
            bgB = buffer[y][x].bgB;
            bold = buffer[y][x].flags & TCHAR_BOLD;
            italics = buffer[y][x].flags & TCHAR_ITALICS;
            underline = buffer[y][x].flags & TCHAR_UNDERLINE;
            strikeout = buffer[y][x].flags & TCHAR_STRIKEOUT;
            //            overline = buffer[y][x].flags & TCHAR_OVERLINE;
            //            inverse = buffer[y][x].flags & TCHAR_INVERSE;
            if (bold) {
                fontWeight = "bold";
            } else {
                fontWeight = "normal";
            }
            if (italics) {
                fontStyle = "italic";
            } else {
                fontStyle = "normal";
            }
            if (!(underline || strikeout /* || overline */)) {
                textDecoration = "normal";
            } else {
                textDecoration = "";
                if (underline) {
                    textDecoration += "underline ";
                }
                if (strikeout) {
                    textDecoration += "line-through ";
                }
                //                if( overline ) {
                //                    textDecoration += "overline ";
                //                }
                textDecoration = textDecoration.trimmed();
            }
            s += R"(<span style=")";
            //            if( inverse )
            //            {
            //                s += "color: rgb(" + QString::number(bgR) + ","
            //                                   + QString::number(bgG) + ","
            //                                   + QString::number(bgB) + ");";
            //                s += " background: rgb(" + QString::number(fgR) + ","
            //                                         + QString::number(fgG) + ","
            //                                         + QString::number(fgB) + ");";
            //            }
            //            else
            //            {
            s += "color: rgb(" + QString::number(fgR) + "," + QString::number(fgG) + "," + QString::number(fgB) + ");";
            s += " background: rgb(" + QString::number(bgR) + "," + QString::number(bgG) + "," + QString::number(bgB) + ");";
            //            }
            s += " font-weight: " + fontWeight + "; font-style: " + fontStyle + "; text-decoration: " + textDecoration + R"(">)";
        }
        if (lineBuffer[y][x] == '<') {
            s.append("&lt;");
        } else if (lineBuffer[y][x] == '>') {
            s.append("&gt;");
        } else {
            s.append(lineBuffer[y][x]);
        }
    }
    if (s.size() > 0) {
        s.append("</span>");
        // Needed to balance the very first open <span>, but only if we have
        // included anything. the previously appearing <br /> is an XML tag, NOT
        // a (strict) HTML 4 one
    }

    s.append(QStringLiteral("<br>\n"));
    // Needed to reproduce empty lines in capture, as this method is called for
    // EACH line, even the empty ones, the spans are styled as "pre" so literal
    // linefeeds would be treated as such THERE but we deliberately place the
    // line-feeds OUTSIDE so they come under the <body>s no wrap and as such
    // line-feeds can be used to break the HTML over lots of lines (which is
    // easier to hand edit and examine afterwards) without impacting the
    // formatting. To get the line feeds at the end of displayed HTML lines the
    // <br> is used.  This slightly weird way of doing things is so that some
    // on-line tools preserve the formatting when the HTML-lised selection is
    // pasted to them AND retain the ability to paste the HTML from the
    // clipboard into a plain text editor and not have everything on one line in
    // that editor!

    return s;
}

const QList<QString> TBuffer::getFriendlyEncodingNames() {
    QList<QString> encodings;
    for (auto pair: csmEncodingTable) {
        encodings << pair.first;
    }

    return encodings;
}

// returns the computer encoding name given a human-friendly one
const QString& TBuffer::getComputerEncoding(const QString& encoding) {
    QMapIterator<QString, QPair<QString, QVector<QChar>>> iterator(csmEncodingTable);
    while (iterator.hasNext()) {
        iterator.next();
        // check the friendly name (stored as the map value pair's first item)
        // against the input - if found, return the map key which is the computer
        // encoding name
        if (iterator.value().first == encoding) {
            return iterator.key();
        }
    }

    // return the original encoding if none is found
    return encoding;
}

bool TBuffer::processUtf8Sequence(const std::string& bufferData, const bool isFromServer, const size_t len, size_t& pos, bool& isNonBMPCharacter)
{
    // In Utf-8 mode we have to process the data more than one byte at a
    // time because there is not necessarily a one-byte to one TChar
    // mapping, instead we use one TChar per QChar - and that has to be
    // tweaked for non-BMP characters that use TWO QChars per codepoint.
    if (bufferData.at(pos) & 0x80) {
        // MSB is set, so if this is Utf-8 then assume this is the first byte
        size_t utf8SequenceLength = 1;
        if ((bufferData.at(pos) & 0xE0) == 0xC0) {
            // 2 byte sequence - Unicode code-points: U+00000080 to U+000007FF
            utf8SequenceLength = 2;
        } else if ((bufferData.at(pos) & 0xF0) == 0xE0) {
            // 3 byte sequence - Unicode code-points: U+00000800 to U+0000FFFF
            utf8SequenceLength = 3;
        } else if ((bufferData.at(pos) & 0xF8) == 0xF0) {
            // 4 byte sequence - Unicode code-points: U+00010000 to U+001FFFFF (<= U+0010FFF LEGAL)
            utf8SequenceLength = 4;
        } else if ((bufferData.at(pos) & 0xFC) == 0xF8) {
            // 5 byte sequence - Unicode code-points: U+00200000 to U+03FFFFFF (ALL ILLEGAL)
            utf8SequenceLength = 5;
        } else if ((bufferData.at(pos) & 0xFE) == 0xFC) {
            // 6 byte sequence - Unicode code-points: U+04000000 to U+7FFFFFFF (ALL ILLEGAL)
            utf8SequenceLength = 6;
        }

        if ((pos + utf8SequenceLength) > len) {
            // Not enough bytes left in bufferData to complete the utf-8
            // sequence - need to save and prepend onto incoming data next
            // time around.
            // The absence of a second argument takes all the available
            // bytes - this is only for data from the Server NOT from
            // locally generated material from Lua feedTriggers(...)
            if (isFromServer) {
#if defined(DEBUG_UTF8_PROCESSING)
                qDebug() << "TBuffer::processUtf8Sequence(...) Insufficent bytes in buffer to complate UTF-8 sequence, need:" << utf8SequenceLength
                         << " but we currently only have: " << bufferData.substr(pos).length() << " bytes (which we will store for next call to this method)...";
#endif
                mIncompleteSequenceBytes = bufferData.substr(pos);
            }
            return false; // Bail out
        }

        // If we have got here we have enough bytes to work with:
        bool isValid = true;
        bool isToUseReplacementMark = false;
        bool isToUseByteOrderMark = false; // When BOM seen in stream it transcodes as zero characters
        switch (utf8SequenceLength) {
        case 4:
            if ((bufferData.at(pos + 3) & 0xC0) != 0x80) {
#if defined(DEBUG_UTF8_PROCESSING)
                qDebug() << "TBuffer::processUtf8Sequence(...) 4th byte in UTF-8 sequence is invalid!";
#endif
                isValid = false;
                isToUseReplacementMark = true;
            } else if (((bufferData.at(pos) & 0x07) > 0x04) || (((bufferData.at(pos) & 0x07) == 0x04) && ((bufferData.at(pos + 1) & 0x3F) > 0x0F))) {
// For 4 byte values the bits are distributed:
//  Byte 1    Byte 2    Byte 3    Byte 4
// 11110ABC  10DEFGHI  10JKLMNO  10PQRSTU   A is MSB
// U+10FFFF in binary is: 1 0000 1111 1111 1111 1111
// So this (the maximum valid character) is:
//      ABC    DEFGHI    JKLMNO    PQRSTU
//      100    001111    111111    111111
// So if the first byte bufferData.at(pos] & 0x07 is:
//  < 0x04 then must be in range
//  > 0x04 then must be out of range
// == 0x04 then consider bufferData.at(pos+1] & 0x3F:
//     <= 001111 0x0F then must be in range
//      > 001111 0x0F then must be out of range

#if defined(DEBUG_UTF8_PROCESSING)
                qDebug() << "TBuffer::processUtf8Sequence(...) 4 byte UTF-8 sequence is valid but is beyond range of legal codepoints!";
#endif
                isValid = false;
                isToUseReplacementMark = true;
            }

        // Fall-through
            [[clang::fallthrough]];
        case 3:
            if ((bufferData.at(pos + 2) & 0xC0) != 0x80) {
#if defined(DEBUG_UTF8_PROCESSING)
                qDebug() << "TBuffer::processUtf8Sequence(...) 3rd byte in UTF-8 sequence is invalid!";
#endif
                isValid = false;
                isToUseReplacementMark = true;
            } else if ((bufferData.at(pos) & 0x0F) == 0x0D) {
// For 3 byte values the bits are distributed:
//  Byte 1    Byte 2    Byte 3
// 1110ABCD  10DEFGHI  10JKLMNO   A is MSB
// First High surrogate 0xed 0xa0 0x80 (U+D800)
// 1101 1000 0000 0000
// ----1101  --100000  --000000
// Last Low surrogate 0xed 0xbf 0xbf (U+DFFF)
// 1101 1111 1111 1111
// ----1101  --111111  --111111
/*
    * As per Wikipedia {https://en.wikipedia.org/wiki/UTF-16#U.2BD800_to_U.2BDFFF}
    * "The Unicode standard permanently reserves these code point values for UTF-16
    * encoding of the high and low surrogates, and they will never be assigned a
    * character, so there should be no reason to encode them. The official Unicode
    * standard says that no UTF forms, including UTF-16, can encode these code
    * points.
    *
    * However UCS-2, UTF-8, and UTF-32 can encode these code points in trivial and
    * obvious ways, and large amounts of software does so even though the standard
    * states that such arrangements should be treated as encoding errors. It is
    * possible to unambiguously encode them in UTF-16 by using a code unit equal to
    * the code point, as long as no sequence of two code units can be interpreted
    * as a legal surrogate pair (that is, as long as a high surrogate is never
    * followed by a low surrogate). The majority of UTF-16 encoder and decoder
    * implementations translate between encodings as though this were the case
    * and Windows allows such sequences in filenames."
    */
// So test for and reject if LSN of first byte is 0xD!
#if defined(DEBUG_UTF8_PROCESSING)
                qDebug() << "TBuffer::processUtf8Sequence(...) 3 byte UTF-8 sequence is a High or Low UTF-16 Surrogate and is not valid in UTF-8!";
#endif
                isValid = false;
                isToUseReplacementMark = true;
            } else if (   (static_cast<quint8>(bufferData.at(pos + 2)) == 0xBF)
                       && (static_cast<quint8>(bufferData.at(pos + 1)) == 0xBB)
                       && (static_cast<quint8>(bufferData.at(pos    )) == 0xEF)) {
// Got caught out by this one - it is the UTF-8 BOM and
// needs to be ignored as it transcodes to NO codepoints!
#if defined(DEBUG_UTF8_PROCESSING)
                qDebug() << "TBuffer::processUtf8Sequence(...) UTF-8 BOM sequence seen and handled!";
#endif
                isValid = false;
                isToUseByteOrderMark = true;
            }

        // Fall-through
            [[clang::fallthrough]];
        case 2:
            if ((static_cast<quint8>(bufferData.at(pos + 1)) & 0xC0) != 0x80) {
#if defined(DEBUG_UTF8_PROCESSING)
                qDebug() << "TBuffer::processUtf8Sequence(...) 2nd byte in UTF-8 sequence is invalid!";
#endif
                isValid = false;
                isToUseReplacementMark = true;
            }

            // clang-format off
            // Disable code reformatting as it would destroy layout that helps
            // to explain the grouping of the tests
            // Also test for (and reject) overlong sequences - don't
            // need to check 5 or 6 ones as those are already rejected:
            if (  ( ((static_cast<quint8>(bufferData.at(pos    )) & 0xFE) == 0xC0) && ( ( static_cast<quint8>(bufferData.at(pos + 1)) & 0xC0) == 0x80) )
                ||( ( static_cast<quint8>(bufferData.at(pos    )        ) == 0xE0) && ( ( static_cast<quint8>(bufferData.at(pos + 1)) & 0xE0) == 0x80) )
                ||( ( static_cast<quint8>(bufferData.at(pos    )        ) == 0xF0) && ( ( static_cast<quint8>(bufferData.at(pos + 1)) & 0xF0) == 0x80) ) ) {
// clang-format on

#if defined(DEBUG_UTF8_PROCESSING)
                qDebug().nospace() << "TBuffer::processUtf8Sequence(...) Overlong " << utf8SequenceLength << "-byte sequence as UTF-8 rejected!";
#endif
                isValid = false;
                isToUseReplacementMark = true;
            }
            break;

        default:
#if defined(DEBUG_UTF8_PROCESSING)
            qDebug().nospace() << "TBuffer::processUtf8Sequence(...) " << utf8SequenceLength << "-byte sequence as UTF-8 rejected!";
#endif
            isValid = false;
            isToUseReplacementMark = true;
        }

        // Will be one (BMP codepoint) or two (non-BMP codepoints) QChar(s)
        if (isValid) {
            QString codePoint = QString(bufferData.substr(pos, utf8SequenceLength).c_str());
            switch (codePoint.size()) {
            default:
                Q_UNREACHABLE(); // This can't happen, unless we got start or length wrong in std::string::substr()
                qWarning().nospace() << "TBuffer::processUtf8Sequence(...) " << utf8SequenceLength << "-byte UTF-8 sequence accepted, and it encoded to " << codePoint.size()
                                     << " QChars which does not make sense!!!";
                isValid = false;
                isToUseReplacementMark = true;
                break;
            case 2:
                isNonBMPCharacter = true;
                // Fall-through
                [[clang::fallthrough]];
            case 1:
#if defined(DEBUG_UTF8_PROCESSING)
                qDebug().nospace() << "TBuffer::processUtf8Sequence(...) " << utf8SequenceLength << "-byte UTF-8 sequence accepted, it was " << codePoint.size() << " QChar(s) long [" << codePoint
                                   << "]";
#endif
                mMudLine.append(codePoint);
                break;
            case 0:
                qWarning().nospace() << "TBuffer::processUtf8Sequence(...) " << utf8SequenceLength << "-byte UTF-8 sequence accepted, but it did not encode to "
                                                                                                      "ANY QChar(s)!!!";
                isValid = false;
                isToUseReplacementMark = true;
            }
        }

        if (!isValid) {
#if defined(DEBUG_UTF8_PROCESSING)
            QString debugMsg;
            for (size_t i = 0; i < utf8SequenceLength; ++i) {
                debugMsg.append(QStringLiteral("<%1>").arg(static_cast<quint8>(bufferData.at(pos + i)), 2, 16, QChar('0')));
            }
            qDebug().nospace() << "    Sequence bytes are: " << debugMsg.toLatin1().constData();
#endif
            if (isToUseReplacementMark) {
                mMudLine.append(QChar::ReplacementCharacter);
            } else if (isToUseByteOrderMark) {
                mMudLine.append(QChar::ByteOrderMark);
            }
        }

        // As there is already a unit increment at the bottom of loop
        // add one less than the sequence length:
        pos += utf8SequenceLength - 1;
    } else {
        // Single byte character i.e. Unicode points: U+00000000 to U+0000007F
        mMudLine.append(bufferData.at(pos));
    }

    return true;
}

bool TBuffer::processGBSequence(const std::string& bufferData, const bool isFromServer, const bool isGB18030, const size_t len, size_t& pos, bool& isNonBmpCharacter)
{
// In GBK/GB18030 mode we have to process the data more than one byte at a
// time because there is not necessarily a one-byte to one TChar
// mapping, instead we use one TChar per QChar - and that has to be
// tweaked for non-BMP characters that use TWO QChars per codepoint.
// GB2312 is the predecessor to both and - according to Wikipedia (EN) covers
// over 99% of the characters of contempory usage.
// GBK is a sub-set of GB18030 so can be processed in the same method
// Assume we are at the first byte of a single (ASCII), pair (GBK/GB18030)
// or four byte (GB18030) sequence

#if defined(DEBUG_GB_PROCESSING)
    std::string dataIdentity;
#endif

    // The range deductions for two-byte sequences are take from:
    // https://en.wikipedia.org/wiki/GBK#Encoding
    size_t gbSequenceLength = 1;
    bool isValid = true;
    bool isToUseReplacementMark = false;
    // Only set this if we are adding more than one code-point to
    // mCurrentLineCharacters:
    isNonBmpCharacter = false;
    if (static_cast<quint8>(bufferData.at(pos)) < 0x80) {
        // Is ASCII - single byte character, straight forward for a "first" byte case
        mMudLine.append(bufferData.at(pos));
        // As there is already a unit increment at the bottom of caller's loop
        // there is no need to tweak pos in THIS case

        return true;
    } else if (static_cast<quint8>(bufferData.at(pos)) == 0x80) {
        // Invalid as first byte
        isValid = false;
        isToUseReplacementMark = true;
#if defined(DEBUG_GB_PROCESSING)
        qDebug().nospace() << "TBuffer::processGBSequence(...) 1-byte sequence as " << (isGB18030 ? "GB18030" : "GB2312/GBK") << " rejected!";
#endif

        // Proceed to handle 1 byte (as GB2312/GBK/GB18030 data) outside of checks...

    } else if (!isGB18030) {
        // Could be two byte GBK - but do we have a second byte?
        // As we are not in GB18030 mode treat it as if it is a 2 byte sequence
        gbSequenceLength = 2;
        if ((pos + gbSequenceLength - 1) < len) {
            // We have enough bytes to look at the second one - lets see which
            // range it is in:
            // clang-format off
            if        (  (static_cast<quint8>(bufferData.at(pos    )) >= 0x81) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xA0)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0x40) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xFE)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) != 0x7F) ) {
// clang-format on
// GBK Area 3 sequence

#if defined(DEBUG_GB_PROCESSING)
                dataIdentity = "GBK Area 3";
#endif

                // clang-format off
            } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xA9)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xFE) ) {
// clang-format on
// GBK Area 1 (& GB2312) sequence

#if defined(DEBUG_GB_PROCESSING)
                dataIdentity = "GBK Area 1 (or GB2312)";
#endif

                // clang-format off
            } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xB0) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xF7)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xFE) ) {
// clang-format on
// GBK Area 2 (& GB2312) sequence

#if defined(DEBUG_GB_PROCESSING)
                dataIdentity = "GBK Area 2 (or GB2312)";
#endif

                // clang-format off
            } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xA8) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xA9)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0x40) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xA0)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) != 0x7F) ) {
// clang-format on
// GBK/5 sequence

#if defined(DEBUG_GB_PROCESSING)
                dataIdentity = "GBK Area 5";
#endif

                // clang-format off
            } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xAA) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xFE)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0x40) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xA0)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) != 0x7F) ) {
// clang-format on
// GBK/4 sequence

#if defined(DEBUG_GB_PROCESSING)
                dataIdentity = "GBK Area 4";
#endif

                // clang-format off
            } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xAA) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xAF)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xFE) ) {
// clang-format on
// User Defined 1 sequence - possibly invalid for us but if the
// MUD supplies their own font it could be used:

#if defined(DEBUG_GB_PROCESSING)
                dataIdentity = "User Defined (PU) Area 1";
#endif

                // clang-format off
            } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xF8) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xFE)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xFE) ) {
// clang-format on
// User Defined 2 sequence - possibly invalid for us but if the
// MUD supplies their own font it could be used:

#if defined(DEBUG_GB_PROCESSING)
                dataIdentity = "User Defined (PU) Area 2";
#endif

                // clang-format off
            } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xA7)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xFE)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) != 0x7F) ) {
// clang-format on
// User Defined 3 sequence - possibly invalid for us but if the
// MUD supplies their own font it could be used:

#if defined(DEBUG_GB_PROCESSING)
                dataIdentity = "User Defined (PU) Area 3";
#endif

                // clang-format off
            } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0x90) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xE3)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0x30) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0x39) ) {
// clang-format on
// First two bytes of a 4-byte GB18030 sequence - for a non-BMP mapped Unicode
// codepoint if byte 3 is within 0x81-00xFE and byte 4 is within 0x30-0x39
                isValid = false;
                isToUseReplacementMark = true;
#if defined(DEBUG_GB_PROCESSING)
                qDebug().nospace() << "TBuffer::processGBSequence(...) 2-byte sequence as "
                                      "GB2312/GBK rejected because it is seems to be the "
                                      "first pair of a 4-byte GB18030 non-BMP Unicode sequence!";
#endif
                // clang-format off
            } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xFD) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xFE)
                      && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0x30) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0x39) ) {
// clang-format on
// First two bytes of a 4-byte GB18030 sequence - for a non-BMP mapped Unicode
// codepoint if byte 3 is within 0x81-00xFE and byte 4 is within 0x30-0x39
                isValid = false;
                isToUseReplacementMark = true;
#if defined(DEBUG_GB_PROCESSING)
                qDebug().nospace() << "TBuffer::processGBSequence(...) 2-byte sequence as "
                                      "GB2312/GBK rejected because it is seems to be the "
                                      "first pair of a 4-byte GB18030 Private Use sequence!";
#endif
            } else {
                // Outside expected ranges

                isValid = false;
                isToUseReplacementMark = true;
#if defined(DEBUG_GB_PROCESSING)
                qDebug().nospace() << "TBuffer::processGBSequence(...) 2-byte sequence as "
                                      "GB2312/GBK rejected!";
#endif
            }

            // Proceed to handle 2 bytes (of GB2312/GBK data) outside of checks...

        } else {
            // Not enough bytes to process yet - so store what we have and return
            if (isFromServer) {
#if defined(DEBUG_GB_PROCESSING)
                qDebug().nospace() << "TBuffer::processGBSequence(...) Insufficent bytes in buffer to "
                                      "complate GB2312/GBK sequence, need at least: "
                                   << gbSequenceLength << " but we currently only have: " << bufferData.substr(pos).length() << " bytes (which we will store for next call to this method)...";
#endif
                mIncompleteSequenceBytes = bufferData.substr(pos);
            }
            return false; // Bail out
        }

    } else {
        // isGB18030 is true!
        // Could be two bytes or four bytes - but do we have at least a second
        // byte? Treat it as if it is a 2 byte sequence until we know we have a
        // four byte one - from examining the second byte and it is in range
        // 0x30 to 0x39 inclusive:

        gbSequenceLength = 2;
        if ((pos + gbSequenceLength - 1) < len) {
            // We have enough bytes to look at the second one - lets see which
            // range it is in:
            // clang-format off
            if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0x81) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xFE)
               && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0x30) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0x39) ) {
                // clang-format on
                // This IS a 4-byte sequence

                gbSequenceLength = 4;

                if ((pos + gbSequenceLength - 1) >= len) {
                    // Not enough bytes to process yet - so store what we have and return
                    if (isFromServer) {
#if defined(DEBUG_GB_PROCESSING)
                        qDebug().nospace() << "TBuffer::processGBSequence(...) Insufficent bytes in buffer to "
                                              "complate GB18030 sequence, need at least: "
                                           << gbSequenceLength << " but we currently only have: " << bufferData.substr(pos).length() << " bytes (which we will store for next call to this method)...";
#endif
                        mIncompleteSequenceBytes = bufferData.substr(pos);
                    }

                    return false; // Bail out
                }

                // clang-format off
                // Continue with four-byte sequence validation processing as we
                // have all four bytes to work with:
                if (   ((  /* 1st group low limit 0x81 is already done*/        (static_cast<quint8>(bufferData.at(pos    )) <= 0x84))
                        ||((static_cast<quint8>(bufferData.at(pos)) >= 0x90) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xE3)))
                    /*
                     * Only the above 1st byte ranges are currently used - others are reserved
                     * Second byte range is 0x30-0x39 for all and has already been checked
                     */
                    && (static_cast<quint8>(bufferData.at(pos + 2)) >= 0x81) && (static_cast<quint8>(bufferData.at(pos + 2)) <= 0xFE)
                    && (static_cast<quint8>(bufferData.at(pos + 3)) >= 0x30) && (static_cast<quint8>(bufferData.at(pos + 3)) <= 0x39) ) {

                    // Okay we should have a valid four byte sequence now
#if defined(DEBUG_GB_PROCESSING)
                    dataIdentity = "Non-BMP mapped Unicode";
#endif
                } else {
                    // if first byte was < 0x90 then it would have been a BMP
                    // unicode codepoint but it is academic as it is not
                    // currently defined as a valid codepoint value and will be
                    // substituted with the replacement character anyway:
                    // clang-format on

                    isValid = false;
                    isToUseReplacementMark = true;

#if defined(DEBUG_GB_PROCESSING)
                    qDebug().nospace() << "TBuffer::processGBSequence(...) 4-byte sequence as "
                                          "GB18030 rejected!";
#endif
                }

                // Proceed to handle 4 bytes (as GB18030 data) outside of checks...

            } else {
                // Looks as though it is a two-byte sequence after all - so
                // validate it as that:
                // clang-format off
                if        (  (static_cast<quint8>(bufferData.at(pos    )) >= 0x81) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xA0)
                          && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0x40) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xFE)
                          && (static_cast<quint8>(bufferData.at(pos + 1)) != 0x7F) ) {
                    // clang-format on
                    // GBK/3 sequence

#if defined(DEBUG_GB_PROCESSING)
                    dataIdentity = "GBK Area 3";
#endif

                // clang-format off
                } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xA9)
                          && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xFE) ) {
                    // clang-format on
                    // GBK/1 (& GB2312) sequence

#if defined(DEBUG_GB_PROCESSING)
                    dataIdentity = "GBK Area 1";
#endif

                // clang-format off
                } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xB0) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xF7)
                          && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xFE) ) {
                    // clang-format on
                    // GBK/2 (& GB2312) sequence

#if defined(DEBUG_GB_PROCESSING)
                    dataIdentity = "GBK Area 2";
#endif

                // clang-format off
                } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xA8) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xA9)
                          && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0x40) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xA0)
                          && (static_cast<quint8>(bufferData.at(pos + 1)) != 0x7F) ) {
                    // clang-format on
                    // GBK/5 sequence

#if defined(DEBUG_GB_PROCESSING)
                    dataIdentity = "GBK Area 5";
#endif

                // clang-format off
                } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xAA) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xFE)
                          && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0x40) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xA0)
                          && (static_cast<quint8>(bufferData.at(pos + 1)) != 0x7F) ) {
                    // clang-format on
                    // GBK/4 sequence

#if defined(DEBUG_GB_PROCESSING)
                    dataIdentity = "GBK Area 4";
#endif

                // clang-format off
                } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xAA) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xAF)
                          && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xFE) ) {
                    // clang-format on
                    // User Defined 1 sequence - possibly invalid for us but if the
                    // MUD supplies their own font it could be used:

#if defined(DEBUG_GB_PROCESSING)
                    dataIdentity = "User Defined (PU) Area 1";
#endif

                // clang-format off
                } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xF8) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xFE)
                          && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xFE) ) {
                    // clang-format on
                    // User Defined 2 sequence - possibly invalid for us but if the
                    // MUD supplies their own font it could be used:

#if defined(DEBUG_GB_PROCESSING)
                    dataIdentity = "User Defined (PU) Area 2";
#endif

                // clang-format off
                } else if (  (static_cast<quint8>(bufferData.at(pos    )) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos    )) <= 0xA7)
                          && (static_cast<quint8>(bufferData.at(pos + 1)) >= 0xA1) && (static_cast<quint8>(bufferData.at(pos + 1)) <= 0xFE)
                          && (static_cast<quint8>(bufferData.at(pos + 1)) != 0x7F) ) {
                    // clang-format on
                    // User Defined 3 sequence - possibly invalid for us but if the
                    // MUD supplies their own font it could be used:

#if defined(DEBUG_GB_PROCESSING)
                    dataIdentity = "User Defined (PU) Area 3";
#endif

                } else {
                    // Outside expected range

                    isValid = false;
                    isToUseReplacementMark = true;
#if defined(DEBUG_GB_PROCESSING)
                    qDebug().nospace() << "TBuffer::processGBSequence(...) 2-byte sequence as GB18030 rejected!";
#endif
                }

            } // End of IF is a four-byte ELSE is a two-byte sequence...

            // Proceed to handle 2 bytes (of GB18030 data) outside of checks...

        } else {
            // Not enough bytes to process yet could be we need two OR four but
            // we only have one - so store what we have and return
            if (isFromServer) {
#if defined(DEBUG_GB_PROCESSING)
                qDebug().nospace() << "TBuffer::processGBSequence(...) Insufficent bytes in buffer to complate GB18030 sequence, need at least:"
                                   << gbSequenceLength << " but we currently only have: " << bufferData.substr(pos).length()
                                   << " bytes (which we will store for next call to this method)...";
#endif
                mIncompleteSequenceBytes = bufferData.substr(pos);
            }
            return false; // Bail out
        }
    }

    // At this point we know how many bytes to consume, and whether they are in
    // the right ranges of individual values to be valid

    if (isValid) {
        // Try and convert two or four byte sequence to Unicode using Qts own
        // decoder - and check number of codepoints returned

        QString codePoint;
        if (mMainIncomingCodec) {
            // Third argument is 0 to indicate we do NOT wish to store the state:
            codePoint = mMainIncomingCodec->toUnicode(bufferData.substr(pos, gbSequenceLength).c_str(), static_cast<int>(gbSequenceLength),
                                                      nullptr);
            switch (codePoint.size()) {
            default:
                Q_UNREACHABLE(); // This can't happen, unless we got start or length wrong in std::string::substr()
                qWarning().nospace() << "TBuffer::processGBSequence(...) " << gbSequenceLength << "-byte " << (isGB18030 ? "GB18030" : "GB2312/GBK") << " sequence accepted, and it encoded to "
                                     << codePoint.size() << " QChars which does not make sense!!!";
                isValid = false;
                isToUseReplacementMark = true;
                break;
            case 2:
                isNonBmpCharacter = true;
            // Fall-through
                [[clang::fallthrough]];
            case 1:
#if defined(DEBUG_GB_PROCESSING)
                qDebug().nospace() << "TBuffer::processGBSequence(...) " << gbSequenceLength << "-byte " << (isGB18030 ? "GB18030" : "GB2312/GBK") << " sequence accepted, it is " << codePoint.size()
                                   << " QChar(s) long [" << codePoint << "] and is in the " << dataIdentity.c_str() << " range";
#endif
                mMudLine.append(codePoint);
                break;
            case 0:
                qWarning().nospace() << "TBuffer::processGBSequence(...) " << gbSequenceLength << "-byte " << (isGB18030 ? "GB18030" : "GB2312/GBK")
                                     << " sequence accepted, but it did not encode to ANY QChar(s)!!!";
                isValid = false;
                isToUseReplacementMark = true;
            }
        } else {
            // Unable to decode it - no Qt decoder...!
            isValid = false;
            isToUseReplacementMark = true;
        }
    }

    if (!isValid) {
#if defined(DEBUG_GB_PROCESSING)
        QString debugMsg;
        for (size_t i = 0; i < gbSequenceLength; ++i) {
            debugMsg.append(QStringLiteral("<%1>").arg(static_cast<quint8>(bufferData.at(pos + i)), 2, 16, QChar('0')));
        }
        qDebug().nospace() << "    Sequence bytes are: " << debugMsg.toLatin1().constData();
#endif
        if (isToUseReplacementMark) {
            mMudLine.append(QChar::ReplacementCharacter);
        }
    }

    // As there is already a unit increment at the bottom of loop
    // add one less than the sequence length:
    pos += gbSequenceLength - 1;

    return true;
}

bool TBuffer::processBig5Sequence(const std::string& bufferData, const bool isFromServer, const size_t len, size_t& pos, bool& isNonBmpCharacter)
{
#if defined(DEBUG_BIG5_PROCESSING)
    std::string dataIdentity;
#endif

    // The encoding standard are taken from https://en.wikipedia.org/wiki/Big5
    size_t big5SequenceLength = 1;
    bool isValid = true;
    bool isToUseReplacementMark = false;
    // Only set this if we are adding more than one code-point to
    // mCurrentLineCharacters:
    isNonBmpCharacter = false;
    if (static_cast<quint8>(bufferData.at(pos)) < 0x80) {
        // Is ASCII - single byte character, straight forward for a "first" byte case
        mMudLine.append(bufferData.at(pos));
        // As there is already a unit increment at the bottom of caller's loop
        // there is no need to tweak pos in THIS case

        return true;
    } else if (static_cast<quint8>(bufferData.at(pos)) == 0x80 || static_cast<quint8>(bufferData.at(pos)) > 0xFE) {
        // Invalid as first byte
        isValid = false;
        isToUseReplacementMark = true;
#if defined(DEBUG_BIG5_PROCESSING)
        qDebug().nospace() << "TBuffer::processBig5Sequence(...) 1-byte sequence as Big5 rejected!";
#endif
    } else {
        // We have two bytes
        big5SequenceLength = 2;
        if ((pos + big5SequenceLength - 1) >= len) {
            // Not enough bytes to process yet - so store what we have and return
            if (isFromServer) {
#if defined(DEBUG_BIG5_PROCESSING)
                qDebug().nospace() << "TBuffer::processBig5Sequence(...) Insufficent bytes in buffer to "
                                      "complete Big5 sequence, need at least: "
                                   << big5SequenceLength << " but we currently only have: " << bufferData.substr(pos).length() << " bytes (which we will store for next call to this method)...";
#endif
                mIncompleteSequenceBytes = bufferData.substr(pos);
            }
            return false; // Bail out
        } else {
            // check if second byte range is valid
            auto val2 = static_cast<quint8>(bufferData.at(pos + 1));
            if (val2 < 0x40 || (val2 > 0x7E && val2 < 0xA1) || val2 > 0xFE) {
                // second byte range is invalid
                isValid = false;
                isToUseReplacementMark = true;
            }
        }

    }

    // At this point we know how many bytes to consume, and whether they are in
    // the right ranges of individual values to be valid

    if (isValid) {
        // Try and convert two byte sequence to Unicode using Qts own
        // decoder - and check number of codepoints returned

        QString codePoint;
        if (mMainIncomingCodec) {
            // Third argument is 0 to indicate we do NOT wish to store the state:
            codePoint = mMainIncomingCodec->toUnicode(bufferData.substr(pos, big5SequenceLength).c_str(), static_cast<int>(big5SequenceLength),
                                                      nullptr);
            switch (codePoint.size()) {
                default:
                    Q_UNREACHABLE(); // This can't happen, unless we got start or length wrong in std::string::substr()
                    qWarning().nospace() << "TBuffer::processBig5Sequence(...) " << big5SequenceLength << "-byte Big5 sequence accepted, and it encoded to "
                                         << codePoint.size() << " QChars which does not make sense!!!";
                    isValid = false;
                    isToUseReplacementMark = true;
                    break;
                case 2:
                    // Fall-through
                    [[clang::fallthrough]];
                case 1:
                    // If Qt's decoder found bad characters, update status flags to reflect that.
                    if (codePoint.contains(QChar::ReplacementCharacter)) {
                        isValid = false;
                        isToUseReplacementMark = true;
                        break;
                    }
#if defined(DEBUG_BIG5_PROCESSING)
                    qDebug().nospace() << "TBuffer::processBig5Sequence(...) " << big5SequenceLength << "-byte Big5 sequence accepted, it is " << codePoint.size()
                                   << " QChar(s) long [" << codePoint << "] and is in the " << dataIdentity.c_str() << " range";
#endif
                    mMudLine.append(codePoint);
                    break;
                case 0:
                    qWarning().nospace() << "TBuffer::processBig5Sequence(...) " << big5SequenceLength << "-byte Big5"
                                   << "sequence accepted, but it did not encode to ANY QChar(s)!!!";
                    isValid = false;
                    isToUseReplacementMark = true;
            }
        } else {
            // Unable to decode it - no Qt decoder...!
#if defined(DEBUG_BIG5_PROCESSING)
            qDebug().nospace() << "No Qt decoder found...";
#endif
            isValid = false;
            isToUseReplacementMark = true;
        }
    }

    if (!isValid) {
#if defined(DEBUG_BIG5_PROCESSING)
        QString debugMsg;
        for (size_t i = 0; i < big5SequenceLength; ++i) {
            debugMsg.append(QStringLiteral("<%1>").arg(static_cast<quint8>(bufferData.at(pos + i)), 2, 16, QChar('0')));
        }
        qDebug().nospace() << "    Invalid.  Sequence bytes are: " << debugMsg.toLatin1().constData();
#endif
        if (isToUseReplacementMark) {
            mMudLine.append(QChar::ReplacementCharacter);
        }
    }

    // As there is already a unit increment at the bottom of loop
    // add one less than the sequence length:
    pos += big5SequenceLength - 1;

    return true;
}

void TBuffer::encodingChanged(const QString& newEncoding)
{
    if (mEncoding != newEncoding) {
        mEncoding = newEncoding;
        if (mEncoding == QLatin1String("GBK") || mEncoding == QLatin1String("GB18030")
                || mEncoding == QLatin1String("Big5")) {
            mMainIncomingCodec = QTextCodec::codecForName(mEncoding.toLatin1().constData());
            if (!mMainIncomingCodec) {
                qCritical().nospace() << "encodingChanged(" << newEncoding << ") ERROR: This encoding cannot be handled as a required codec was not found in the system!";
            } else {
                qDebug().nospace() << "encodingChanged(" << newEncoding << ") INFO: Installing a codec that can handle:" << mMainIncomingCodec->aliases();
            }
        } else if (mMainIncomingCodec) {
            qDebug().nospace() << "encodingChanged(" << newEncoding << ") INFO: Uninstall a codec that can handle:" << mMainIncomingCodec->aliases() << " as the new encoding setting of: "
                               << mEncoding << " does not need a dedicated one explicitly set...";
            mMainIncomingCodec = nullptr;
        }
    }
}

QString TBuffer::processSupportsRequest(const QString& elements)
{
    // strip initial SUPPORT and tokenize all of the requested elements
    auto elementsList = elements.trimmed().remove(0, 7).split(QStringLiteral(" "), QString::SkipEmptyParts);
    QStringList result;

    auto reportEntireElement = [](auto element, auto& result) {
        result.append("+" + element);

        for (const auto& attribute : mSupportedMxpElements.value(element)) {
            result.append("+" + element + QStringLiteral(".") + attribute);
        }

        return result;
    };

    auto reportAllElements = [reportEntireElement](auto& result) {
        auto elementsIterator = mSupportedMxpElements.constBegin();
        while (elementsIterator != mSupportedMxpElements.constEnd()) {
            result = reportEntireElement(elementsIterator.key(), result);
            ++elementsIterator;
        }

        return result;
    };

    // empty <SUPPORT> - report all known elements
    if (elementsList.isEmpty()) {
        result = reportAllElements(result);
    } else {
        // otherwise it's <SUPPORT element1 element2 element3>
        for (auto& element : elementsList) {
            // prune any enclosing quotes
            if (element.startsWith(QChar('"'))) {
                element = element.remove(0, 1);
            }
            if (element.endsWith(QChar('"'))) {
                element.chop(1);
            }

            if (!element.contains(QChar('.'))) {
                if (mSupportedMxpElements.contains(element)) {
                    result = reportEntireElement(element, result);
                } else {
                    result.append("-" + element);
                }
            } else {
                auto elementName = element.section(QChar('.'), 0, 0);
                auto attributeName = element.section(QChar('.'), 1, 1);

                if (!mSupportedMxpElements.contains(elementName)) {
                    result.append("-" + element);
                } else if (attributeName == QLatin1String("*")) {
                    result = reportEntireElement(elementName, result);
                } else {
                    if (mSupportedMxpElements.value(elementName).contains(attributeName)) {
                        result.append("+" + element + "." + attributeName);
                    } else {
                        result.append("-" + element + "." + attributeName);
                    }
                }
            }
        }
    }

    return result.join(QLatin1String(" "));
}
