/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
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


#include "TEncodingTable.h"
#include "TTextCodec.h"

const TEncodingTable TEncodingTable::csmDefaultInstance = TEncodingTable(csmEncodings);

QList<QByteArray> TEncodingTable::getEncodingNames() const
{
    static QByteArrayList results;
    if (results.isEmpty()) {
        // First call, list is empty - so work them out, just this once:
        results = QByteArrayList{mEncodingMap.keys()};

        QMutableByteArrayListIterator itEncoding(results);
        while (itEncoding.hasNext()) {
            QByteArray encoding{itEncoding.next()};
            QTextCodec* pEncoding = QTextCodec::codecForName(encoding);
            if (!pEncoding) {
                // We do not have that encoder available after all
                itEncoding.remove();
                if (encoding == "CP437") {
                    // Okay to insert our replacement TTextCodex_XXXX into the
                    // system we must instantiate them once:
                    auto* pTTextCodec_437 = new TTextCodec_437();
                    // Now that it has been instantiated, the system knows about
                    // it - indeed it takes possession of it and we must NOT
                    // delete it ourselves!
                    if (pTTextCodec_437) {
                        itEncoding.insert(pTTextCodec_437->name());
                    }
                } else if (encoding == "CP667") {
                    auto* pTTextCodec_667 = new TTextCodec_667();
                    if (pTTextCodec_667) {
                        itEncoding.insert(pTTextCodec_667->name());
                    }
                } else if (encoding == "CP737") {
                    auto* pTTextCodec_737 = new TTextCodec_737();
                    if (pTTextCodec_737) {
                        itEncoding.insert(pTTextCodec_737->name());
                    }
                } else if (encoding == "CP869") {
                    auto* pTTextCodec_869 = new TTextCodec_869();
                    if (pTTextCodec_869) {
                        itEncoding.insert(pTTextCodec_869->name());
                    }
                }
            }
        }
    }
    return results;
}

const QVector<QChar>& TEncodingTable::getLookupTable(const QByteArray& encoding) const
{
    const auto ptr = mEncodingMap.find(encoding);
    return ptr != mEncodingMap.end() ? ptr.value() : csmEmptyLookupTable;
}


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
const QMap<QByteArray, QVector<QChar>> TEncodingTable::csmEncodings = {
        {"ISO 8859-2",
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
          QChar(0x0159), QChar(0x016F), QChar(0x00FA), QChar(0x0171), QChar(0x00FC), QChar(0x00FD), QChar(0x0163), QChar(0x02D9)}}, // F8-FF
        {"ISO 8859-3",
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
          QChar(0x011D), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x016D), QChar(0x015D), QChar(0x02D9)}},// F8-FF
        {"ISO 8859-4",
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
          QChar(0x00F8), QChar(0x0173), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x0169), QChar(0x016B), QChar(0x02D9)}},// F8-FF
        {"ISO 8859-5",
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
          QChar(0x0458), QChar(0x0459), QChar(0x045A), QChar(0x045B), QChar(0x045C), QChar(0x00A7), QChar(0x045E), QChar(0x045F)}},// F8-FF
        {"ISO 8859-6",
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
          QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD)}},// F8-FF
        {"ISO 8859-7",
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
          QChar(0x03C8), QChar(0x03C9), QChar(0x03CA), QChar(0x03CB), QChar(0x03CC), QChar(0x03CD), QChar(0x03CE), QChar(0xFFFD)}},// F8-FF
        {"ISO 8859-8",
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
          QChar(0x05E8), QChar(0x05E9), QChar(0x05EA), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD)}},// F8-FF
        {"ISO 8859-9",
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
          QChar(0x00F8), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x0131), QChar(0x015F), QChar(0x00FF)}},// F8-FF
        {"ISO 8859-10",
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
          QChar(0x00F8), QChar(0x0173), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x00FD), QChar(0x00FE), QChar(0x0138)}},// F8-FF
        {"ISO 8859-11",
                //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
         {QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 80-87
          QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 88-8F
          QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD),  // 90-97
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
          QChar(0x0E58), QChar(0x0E59), QChar(0x0E5A), QChar(0x0E5B), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD)}},// F8-FF
        {"ISO 8859-13",
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
          QChar(0x0173), QChar(0x0142), QChar(0x015B), QChar(0x016B), QChar(0x00FC), QChar(0x017C), QChar(0x017E), QChar(0x2019)}},// F8-FF
        {"ISO 8859-14",
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
          QChar(0x00F8), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x00FD), QChar(0x0177), QChar(0x00FF)}},// F8-FF
        {"ISO 8859-15",
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
          QChar(0x00F8), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x00FD), QChar(0x00FE), QChar(0x00FF)}},// F8-FF
        {"ISO 8859-16",
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
          QChar(0x0171), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x0119), QChar(0x021B), QChar(0x00FF)}},// F8-FF
        {"CP437", // Our alternative is M_CP437
                //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
         {QChar(0x00C7), QChar(0x00FC), QChar(0x00E9), QChar(0x00E2), QChar(0x00E4), QChar(0x00E0), QChar(0x00E5), QChar(0x00E7),  // 80-87
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
          QChar(0x00B0), QChar(0x2219), QChar(0x00B7), QChar(0x221A), QChar(0x207F), QChar(0x00B2), QChar(0x25A0), QChar(0x00A0)}},// F8-FF
        {"CP667", // Our alternative is M_CP667
                //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
         {QChar(0x00C7), QChar(0x00FC), QChar(0x00E9), QChar(0x00E2), QChar(0x00E4), QChar(0x00E0), QChar(0x0105), QChar(0x00E7),  // 80-87
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
          QChar(0x00B0), QChar(0x2219), QChar(0x00B7), QChar(0x221A), QChar(0x207F), QChar(0x00B2), QChar(0x25A0), QChar(0x00A0)}},// F8-FF
        {"CP737", // Our alternative is M_CP737
                //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
         {QChar(0x0391), QChar(0x0392), QChar(0x0393), QChar(0x0394), QChar(0x0395), QChar(0x0396), QChar(0x0397), QChar(0x0398),  // 80-87
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
          QChar(0x00B0), QChar(0x2219), QChar(0x00B7), QChar(0x221A), QChar(0x207F), QChar(0x00B2), QChar(0x25A0), QChar(0x00A0)}},// F8-FF
        {"CP850",
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
          QChar(0x00B0), QChar(0x00A8), QChar(0x00B7), QChar(0x00B9), QChar(0x00B3), QChar(0x00B2), QChar(0x25A0), QChar(0x00A0)}},// F8-FF
        {"CP866",
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
          QChar(0x00B0), QChar(0x2219), QChar(0x00B7), QChar(0x221A), QChar(0x2116), QChar(0x00A4), QChar(0x25A0), QChar(0x00A0)}},// F8-FF
        {"CP869", // Our alternative is M_CP869
                //      x0/x8          x1/x9          x2/xA          x3/xB          x4/xC          x5/xD          x6/xE          x7/xF
         {QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0x0386), QChar(0x20AC),  // 80-87
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
          QChar(0x00B0), QChar(0x00A8), QChar(0x03C9), QChar(0x03CB), QChar(0x03B0), QChar(0x03CE), QChar(0x25A0), QChar(0x00A0)}},// F8-FF
        {"CP1161",
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
          QChar(0x0E58), QChar(0x0E59), QChar(0x0E5A), QChar(0x0E5B), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD), QChar(0xFFFD)}},// F8-FF
        {"KOI8-R",
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
          QChar(0x042C), QChar(0x042B), QChar(0x0417), QChar(0x0428), QChar(0x042D), QChar(0x0429), QChar(0x0427), QChar(0x042A)}},// F8-FF
        {"KOI8-U",
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
          QChar(0x042C), QChar(0x042B), QChar(0x0417), QChar(0x0428), QChar(0x042D), QChar(0x0429), QChar(0x0427), QChar(0x042A)}},// F8-FF
        {"MACINTOSH",
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
          QChar(0x00AF), QChar(0x02D8), QChar(0x02D9), QChar(0x02DA), QChar(0x00B8), QChar(0x02DD), QChar(0x02DB), QChar(0x02C7)}},// F8-FF
        {"WINDOWS-1250",
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
          QChar(0x0159), QChar(0x016F), QChar(0x00FA), QChar(0x0171), QChar(0x00FC), QChar(0x00FD), QChar(0x0163), QChar(0x02D9)}},// F8-FF
        {"WINDOWS-1251",
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
          QChar(0x0448), QChar(0x0449), QChar(0x044A), QChar(0x044B), QChar(0x044C), QChar(0x044D), QChar(0x044E), QChar(0x044F)}},// F8-FF
        {"WINDOWS-1252",
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
          QChar(0x00F8), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x00FD), QChar(0x00FE), QChar(0x00FF)}},// F8-FF
        {"WINDOWS-1253",
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
          QChar(0x03C8), QChar(0x03C9), QChar(0x03CA), QChar(0x03CB), QChar(0x03CC), QChar(0x03CD), QChar(0x03CE), QChar(0xFFFD)}},// F8-FF
        {"WINDOWS-1254",
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
          QChar(0x00F8), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x0131), QChar(0x015F), QChar(0x00FF)}},// F8-FF
        {"WINDOWS-1255",
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
          QChar(0x05E8), QChar(0x05E9), QChar(0x05EA), QChar(0xFFFD), QChar(0xFFFD), QChar(0x200E), QChar(0x200F), QChar(0xFFFD)}},// F8-FF
        {"WINDOWS-1256",
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
          QChar(0x0651), QChar(0x00F9), QChar(0x0652), QChar(0x00FB), QChar(0x00FC), QChar(0x200E), QChar(0x200F), QChar(0x06D2)}},// F8-FF
        {"WINDOWS-1257",
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
          QChar(0x0173), QChar(0x0142), QChar(0x015B), QChar(0x016B), QChar(0x00FC), QChar(0x017C), QChar(0x017E), QChar(0x02D9)}},// F8-FF
        {"WINDOWS-1258",
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
          QChar(0x00F8), QChar(0x00F9), QChar(0x00FA), QChar(0x00FB), QChar(0x00FC), QChar(0x01B0), QChar(0x20AB), QChar(0x00FF)}}};// F8-FF
// clang-format on