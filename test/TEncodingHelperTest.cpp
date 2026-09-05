/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include "TEncodingHelper.h"

#include <QtTest/QtTest>

/*
 * Unit tests for TEncodingHelper.
 *
 * The class is a pure static utility (no GUI / Host / TMap dependencies), so
 * these tests run without needing the full Mudlet application stack.
 *
 * Primary purpose: regression guard for issue #9344. The 4.21.0 encoding
 * refactor moved Mudlet from Qt5's self-contained QTextCodec to Qt6's
 * QStringConverter, which only offers the multibyte CJK encodings (Big5,
 * Big5-HKSCS, GBK, GB18030, EUC-KR, ...) when Qt is built with ICU. On builds
 * whose Qt lacks runtime ICU this silently broke CJK users (garbled display and
 * "no codec found" send errors). TEncodingHelper now falls back to the
 * Qt5Compat QTextCodec, whose CJK codec tables are ICU-independent.
 *
 * These tests assert the user-facing guarantee - that the CJK encodings remain
 * available and round-trip correctly through TEncodingHelper - regardless of
 * whether the underlying path is QStringConverter (ICU) or the Qt5Compat
 * fallback. On a build lacking ICU, removing the fallback makes these tests
 * fail, which is exactly the regression we want to catch.
 */
class TEncodingHelperTest : public QObject
{
    Q_OBJECT

    // Build a QString from BMP code points so the test does not depend on the
    // source file's own text encoding.
    static QString fromCodepoints(std::initializer_list<char16_t> codepoints)
    {
        QString result;
        for (const char16_t cp : codepoints) {
            result.append(QChar(cp));
        }
        return result;
    }

private slots:

    // -------------------------------------------------------------------------
    // Availability - the core #9344 guarantee
    // -------------------------------------------------------------------------

    void isEncodingAvailable_utf8_true() { QVERIFY(TEncodingHelper::isEncodingAvailable("UTF-8")); }

    void isEncodingAvailable_big5_true() { QVERIFY(TEncodingHelper::isEncodingAvailable("Big5")); }

    void isEncodingAvailable_big5hkscs_true() { QVERIFY(TEncodingHelper::isEncodingAvailable("Big5-HKSCS")); }

    void isEncodingAvailable_gbk_true() { QVERIFY(TEncodingHelper::isEncodingAvailable("GBK")); }

    void isEncodingAvailable_gb18030_true() { QVERIFY(TEncodingHelper::isEncodingAvailable("GB18030")); }

    void isEncodingAvailable_euckr_true() { QVERIFY(TEncodingHelper::isEncodingAvailable("EUC-KR")); }

    void isEncodingAvailable_unknown_false() { QVERIFY(!TEncodingHelper::isEncodingAvailable("not-a-real-codec")); }

    // -------------------------------------------------------------------------
    // Big5 - the headline encoding from issue #9344, asserted against a known
    // byte sequence so the test is meaningful even where ICU happens to work.
    // "中文" -> U+4E2D U+6587 -> Big5 0xA4A4 0xA4E5
    // -------------------------------------------------------------------------

    void decode_big5_knownSequence()
    {
        const QByteArray bytes = QByteArray::fromHex("a4a4a4e5");
        const QString expected = fromCodepoints({0x4E2D, 0x6587});
        QCOMPARE(TEncodingHelper::decode(bytes, "Big5"), expected);
    }

    void encode_big5_knownSequence()
    {
        const QString input = fromCodepoints({0x4E2D, 0x6587});
        const QByteArray expected = QByteArray::fromHex("a4a4a4e5");
        QCOMPARE(TEncodingHelper::encode(input, "Big5"), expected);
    }

    void canEncode_big5_cjk_true()
    {
        const QString input = fromCodepoints({0x4E2D, 0x6587});
        QVERIFY(TEncodingHelper::canEncode(input, "Big5"));
    }

    // Big5 (Traditional Chinese) cannot represent Hangul, so canEncode must
    // report false - the branch behind the "no codec found" send-error in #9344.
    void canEncode_big5_hangul_false()
    {
        const QString input = fromCodepoints({0xD55C, 0xAD6D}); // 한국
        QVERIFY(!TEncodingHelper::canEncode(input, "Big5"));
    }

    void roundTrip_big5()
    {
        const QString input = fromCodepoints({0x4E2D, 0x6587, 0x0041}); // 中文A
        const QByteArray encoded = TEncodingHelper::encode(input, "Big5");
        QCOMPARE(TEncodingHelper::decode(encoded, "Big5"), input);
    }

    // -------------------------------------------------------------------------
    // GBK / GB18030 - simplified Chinese. "中文" -> GBK 0xD6D0 0xCEC4
    // -------------------------------------------------------------------------

    void encode_gbk_knownSequence()
    {
        const QString input = fromCodepoints({0x4E2D, 0x6587});
        const QByteArray expected = QByteArray::fromHex("d6d0cec4");
        QCOMPARE(TEncodingHelper::encode(input, "GBK"), expected);
    }

    void roundTrip_gbk()
    {
        const QString input = fromCodepoints({0x4E2D, 0x6587, 0x0041});
        const QByteArray encoded = TEncodingHelper::encode(input, "GBK");
        QCOMPARE(TEncodingHelper::decode(encoded, "GBK"), input);
    }

    void roundTrip_gb18030()
    {
        const QString input = fromCodepoints({0x4E2D, 0x6587, 0x0041});
        const QByteArray encoded = TEncodingHelper::encode(input, "GB18030");
        QCOMPARE(TEncodingHelper::decode(encoded, "GB18030"), input);
    }

    // -------------------------------------------------------------------------
    // EUC-KR - Korean. "한국" -> U+D55C U+AD6D
    // -------------------------------------------------------------------------

    void roundTrip_euckr()
    {
        const QString input = fromCodepoints({0xD55C, 0xAD6D, 0x0041}); // 한국A
        const QByteArray encoded = TEncodingHelper::encode(input, "EUC-KR");
        QCOMPARE(TEncodingHelper::decode(encoded, "EUC-KR"), input);
    }

    void canEncode_euckr_cjk_true()
    {
        const QString input = fromCodepoints({0xD55C, 0xAD6D});
        QVERIFY(TEncodingHelper::canEncode(input, "EUC-KR"));
    }

    // -------------------------------------------------------------------------
    // Unknown encoding - exercises the final fallthrough of each function once
    // QStringConverter, the lookup tables and QTextCodec have all come up empty.
    // -------------------------------------------------------------------------

    void decode_unknown_fallsBackToLatin1()
    {
        const QByteArray bytes = QByteArrayLiteral("ABC");
        QCOMPARE(TEncodingHelper::decode(bytes, "not-a-real-codec"), QStringLiteral("ABC"));
    }

    void encode_unknown_fallsBackToLatin1()
    {
        const QString input = QStringLiteral("ABC");
        QCOMPARE(TEncodingHelper::encode(input, "not-a-real-codec"), QByteArrayLiteral("ABC"));
    }

    void canEncode_unknown_false() { QVERIFY(!TEncodingHelper::canEncode(QStringLiteral("ABC"), "not-a-real-codec")); }

    // -------------------------------------------------------------------------
    // Controls - encodings that never depended on the fallback, to ensure the
    // fallback chain did not disturb the always-available paths.
    // -------------------------------------------------------------------------

    void roundTrip_utf8()
    {
        const QString input = fromCodepoints({0x4E2D, 0x6587, 0x0041});
        const QByteArray encoded = TEncodingHelper::encode(input, "UTF-8");
        QCOMPARE(TEncodingHelper::decode(encoded, "UTF-8"), input);
    }

    void roundTrip_latin1()
    {
        const QString input = fromCodepoints({0x0048, 0x00E9, 0x006C, 0x006C, 0x006F}); // Héllo
        const QByteArray encoded = TEncodingHelper::encode(input, "ISO 8859-1");
        QCOMPARE(TEncodingHelper::decode(encoded, "ISO 8859-1"), input);
    }

    // -------------------------------------------------------------------------
    // The five custom code pages Mudlet carries itself (M_CP437, M_CP667,
    // M_CP737, M_CP869 and M_MEDIEVIA). TEncodingHelper routes both the bare
    // and the M_-prefixed name to them ahead of everything else, so these are
    // the tables that carry a player's own typing out to the game, and that
    // decode GMCP and the other out-of-band protocols. Game text arriving for
    // the screen is decoded through TEncodingTable's same-named lookup tables
    // instead, so the two have to agree - see Encoding_spec.lua, which watches
    // a character go out through one and come back in through the other.
    // -------------------------------------------------------------------------

    // CP437's upper half is the IBM PC's own: accented Latin at 0x80-0xAF, box
    // drawing at 0xB0-0xDF, mathematical and Greek at 0xE0-0xFF.
    void decode_cp437_accentedLatin()
    {
        const QByteArray bytes = QByteArray::fromHex("8081828384858687");
        const QString expected = fromCodepoints({0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7}); // Çüéâäàåç
        QCOMPARE(TEncodingHelper::decode(bytes, "CP437"), expected);
    }

    void decode_cp437_currencyAndPunctuation()
    {
        const QByteArray bytes = QByteArray::fromHex("9b9c9d9e9fa8a9aa");
        const QString expected = fromCodepoints({0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192, 0x00BF, 0x2310, 0x00AC}); // ¢£¥₧ƒ¿⌐¬
        QCOMPARE(TEncodingHelper::decode(bytes, "CP437"), expected);
    }

    void decode_cp437_boxDrawingAndGreek()
    {
        const QByteArray bytes = QByteArray::fromHex("b0dbe0e1");
        const QString expected = fromCodepoints({0x2591, 0x2588, 0x03B1, 0x00DF}); // ░█αß
        QCOMPARE(TEncodingHelper::decode(bytes, "CP437"), expected);
    }

    // Every byte above ASCII at once. The rows are the standard IBM code page
    // 437, so any one of them being overwritten - by a paste from another code
    // page's table, say - shows up here even where no other test looks.
    void decode_cp437_wholeUpperHalf()
    {
        QByteArray bytes;
        for (int byte = 0x80; byte <= 0xFF; ++byte) {
            bytes.append(static_cast<char>(byte));
        }
        const QString expected = fromCodepoints({0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,   // 80-87
                                                 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,   // 88-8F
                                                 0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,   // 90-97
                                                 0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,   // 98-9F
                                                 0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,   // A0-A7
                                                 0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,   // A8-AF
                                                 0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,   // B0-B7
                                                 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,   // B8-BF
                                                 0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,   // C0-C7
                                                 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,   // C8-CF
                                                 0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,   // D0-D7
                                                 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,   // D8-DF
                                                 0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,   // E0-E7
                                                 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,   // E8-EF
                                                 0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,   // F0-F7
                                                 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0}); // F8-FF
        QCOMPARE(TEncodingHelper::decode(bytes, "CP437"), expected);
    }

    // The M_ prefix is the name TTextCodec_437 reports for itself; both spellings
    // have to reach the same table.
    void decode_cp437_prefixedNameMatchesBareName()
    {
        const QByteArray bytes = QByteArray::fromHex("808182");
        QCOMPARE(TEncodingHelper::decode(bytes, "M_CP437"), TEncodingHelper::decode(bytes, "CP437"));
    }

    void decode_cp437_asciiPassesThrough() { QCOMPARE(TEncodingHelper::decode(QByteArrayLiteral("Hello 42!"), "CP437"), QStringLiteral("Hello 42!")); }

    void roundTrip_cp437()
    {
        const QString input = fromCodepoints({0x00C7, 0x0041, 0x00FC, 0x2591, 0x03B1}); // ÇAü░α
        const QByteArray encoded = TEncodingHelper::encode(input, "CP437");
        QCOMPARE(encoded, QByteArray::fromHex("804181b0e0"));
        QCOMPARE(TEncodingHelper::decode(encoded, "CP437"), input);
    }

    // A character outside the code page is replaced rather than dropped, so the
    // byte count of what is sent still matches the character count typed.
    void encode_cp437_unrepresentableBecomesQuestionMark()
    {
        const QString input = fromCodepoints({0x0041, 0x4E2D, 0x0042}); // A中B
        QCOMPARE(TEncodingHelper::encode(input, "CP437"), QByteArrayLiteral("A?B"));
    }

    void canEncode_cp437_accentedLatin_true() { QVERIFY(TEncodingHelper::canEncode(fromCodepoints({0x00C7, 0x00FC}), "CP437")); }

    void canEncode_cp437_cjk_false() { QVERIFY(!TEncodingHelper::canEncode(fromCodepoints({0x4E2D}), "CP437")); }

    // CP667 (Mazovia) is CP437 with the Polish letters replacing eight of the
    // accented Latin slots - the only thing that tells the two tables apart.
    void decode_cp667_polishLetters()
    {
        const QByteArray bytes = QByteArray::fromHex("868f9092a09c");
        const QString expected = fromCodepoints({0x0105, 0x0104, 0x0118, 0x0142, 0x0179, 0x0141}); // ąĄĘłŹŁ
        QCOMPARE(TEncodingHelper::decode(bytes, "CP667"), expected);
    }

    void roundTrip_cp667()
    {
        const QString input = fromCodepoints({0x0105, 0x0104, 0x0142, 0x0041}); // ąĄłA
        const QByteArray encoded = TEncodingHelper::encode(input, "CP667");
        QCOMPARE(TEncodingHelper::decode(encoded, "CP667"), input);
    }

    // The Polish letters CP667 adds are exactly what CP437 cannot hold, which is
    // the reason both tables exist.
    void canEncode_cp667_polish_true() { QVERIFY(TEncodingHelper::canEncode(fromCodepoints({0x0105, 0x0141}), "CP667")); }

    void canEncode_cp437_polish_false() { QVERIFY(!TEncodingHelper::canEncode(fromCodepoints({0x0105, 0x0141}), "CP437")); }

    // CP737 is the DOS Greek code page: the alphabet starts at 0x80 in capitals.
    void decode_cp737_greekCapitals()
    {
        const QByteArray bytes = QByteArray::fromHex("8081828397");
        const QString expected = fromCodepoints({0x0391, 0x0392, 0x0393, 0x0394, 0x03A9}); // ΑΒΓΔΩ
        QCOMPARE(TEncodingHelper::decode(bytes, "CP737"), expected);
    }

    void roundTrip_cp737()
    {
        const QString input = fromCodepoints({0x0391, 0x03A9, 0x0041}); // ΑΩA
        const QByteArray encoded = TEncodingHelper::encode(input, "CP737");
        QCOMPARE(TEncodingHelper::decode(encoded, "CP737"), input);
    }

    // CP869 is the other Greek code page, and lays the alphabet out differently
    // from CP737 - so the same bytes have to mean different letters.
    void decode_cp869_greek()
    {
        const QByteArray bytes = QByteArray::fromHex("86a4a5de");
        const QString expected = fromCodepoints({0x0386, 0x0391, 0x0392, 0x03B5}); // ΆΑΒε
        QCOMPARE(TEncodingHelper::decode(bytes, "CP869"), expected);
    }

    void decode_cp869_differsFromCp737OnTheSameByte()
    {
        const QByteArray bytes = QByteArray::fromHex("a4");
        QVERIFY(TEncodingHelper::decode(bytes, "CP869") != TEncodingHelper::decode(bytes, "CP737"));
    }

    // 0x96 and 0xD4 are the two places CP869 spells an upsilon; a table that
    // reads them both as the psi at 0xD4 has lost the diaeresis form entirely.
    void decode_cp869_upsilonWithDiaeresisIsNotPsi()
    {
        QCOMPARE(TEncodingHelper::decode(QByteArray::fromHex("96"), "CP869"), fromCodepoints({0x03AB}));
        QCOMPARE(TEncodingHelper::decode(QByteArray::fromHex("d4"), "CP869"), fromCodepoints({0x03A8}));
    }

    // CP869 leaves six of its byte values undefined, and those decode to the
    // replacement character rather than to whatever happens to sit in the slot.
    void decode_cp869_undefinedByteBecomesReplacementCharacter() { QCOMPARE(TEncodingHelper::decode(QByteArray::fromHex("80"), "CP869"), fromCodepoints({0xFFFD})); }

    void roundTrip_cp869()
    {
        const QString input = fromCodepoints({0x0386, 0x0391, 0x0041}); // ΆΑA
        const QByteArray encoded = TEncodingHelper::encode(input, "CP869");
        QCOMPARE(TEncodingHelper::decode(encoded, "CP869"), input);
    }

    // The Medievia code page maps its map-drawing glyphs into the private use
    // area, where the game's own font supplies them.
    void decode_medievia_privateUseGlyphs()
    {
        const QByteArray bytes = QByteArray::fromHex("8084e3");
        const QString expected = fromCodepoints({0x256E, 0xE100, 0x2620});
        QCOMPARE(TEncodingHelper::decode(bytes, "MEDIEVIA"), expected);
    }

    void decode_medievia_prefixedNameMatchesBareName()
    {
        const QByteArray bytes = QByteArray::fromHex("8084e3");
        QCOMPARE(TEncodingHelper::decode(bytes, "M_MEDIEVIA"), TEncodingHelper::decode(bytes, "MEDIEVIA"));
    }

    // Medievia and CP437 share the box-drawing rows but nothing below 0xB0, so a
    // player on the wrong one of the two sees map glyphs where letters belong.
    void decode_medievia_differsFromCp437BelowBoxDrawing()
    {
        const QByteArray bytes = QByteArray::fromHex("80");
        QVERIFY(TEncodingHelper::decode(bytes, "MEDIEVIA") != TEncodingHelper::decode(bytes, "CP437"));
    }

    void roundTrip_medievia()
    {
        const QString input = fromCodepoints({0x256E, 0xE100, 0x0041});
        const QByteArray encoded = TEncodingHelper::encode(input, "MEDIEVIA");
        QCOMPARE(TEncodingHelper::decode(encoded, "MEDIEVIA"), input);
    }

    void isEncodingAvailable_customCodePages_true()
    {
        for (const QByteArray& encoding :
             {QByteArrayLiteral("CP437"), QByteArrayLiteral("M_CP437"), QByteArrayLiteral("CP667"), QByteArrayLiteral("CP737"), QByteArrayLiteral("CP869"), QByteArrayLiteral("MEDIEVIA")}) {
            QVERIFY2(TEncodingHelper::isEncodingAvailable(encoding), encoding.constData());
        }
    }

    // The aliases are what a server's CHARSET negotiation is matched against, so
    // an encoding that answers with an empty list can never be negotiated.
    void aliases_cp437_listsTheIanaNames()
    {
        const QList<QByteArray> expected{"IBM437", "437", "cp437", "csPC8CodePage437"};
        QCOMPARE(TEncodingHelper::aliases("CP437"), expected);
        QCOMPARE(TEncodingHelper::aliases("M_CP437"), expected);
    }

    void aliases_cp869_listsTheIanaNames() { QVERIFY(TEncodingHelper::aliases("CP869").contains(QByteArrayLiteral("IBM869"))); }

    void aliases_nonCustomEncoding_isEmpty() { QVERIFY(TEncodingHelper::aliases("UTF-8").isEmpty()); }

    // -------------------------------------------------------------------------
    // The single-byte encodings Mudlet offers that are not custom code pages.
    // Which mechanism serves one depends on the Qt build - QStringConverter
    // reaches most of them through ICU, and where it cannot, TEncodingTable's
    // own lookup tables do - so these assert the result rather than the route.
    // CP1161 is the exception used below: no QStringConverter build offers it,
    // so it is always the lookup table that answers, on every platform.
    // -------------------------------------------------------------------------

    void decode_koi8r_cyrillic()
    {
        const QByteArray bytes = QByteArray::fromHex("f0d2c9d7c5d4");
        const QString expected = fromCodepoints({0x041F, 0x0440, 0x0438, 0x0432, 0x0435, 0x0442}); // Привет
        QCOMPARE(TEncodingHelper::decode(bytes, "KOI8-R"), expected);
    }

    void roundTrip_koi8r()
    {
        const QString input = fromCodepoints({0x041F, 0x0440, 0x0438, 0x0041});
        const QByteArray encoded = TEncodingHelper::encode(input, "KOI8-R");
        QCOMPARE(TEncodingHelper::decode(encoded, "KOI8-R"), input);
    }

    void roundTrip_iso8859_15()
    {
        const QString input = fromCodepoints({0x20AC, 0x00E9, 0x0041}); // €éA
        const QByteArray encoded = TEncodingHelper::encode(input, "ISO 8859-15");
        QCOMPARE(TEncodingHelper::decode(encoded, "ISO 8859-15"), input);
    }

    // ISO 8859-1 has no euro sign and ISO 8859-15 does, at the byte 8859-1 uses
    // for the currency sign - the pair that makes the two tables worth keeping apart.
    void encode_iso8859_1_hasNoEuroSign() { QVERIFY(!TEncodingHelper::canEncode(fromCodepoints({0x20AC}), "ISO 8859-1")); }

    void canEncode_iso8859_15_euroSign_true() { QVERIFY(TEncodingHelper::canEncode(fromCodepoints({0x20AC}), "ISO 8859-15")); }

    void decode_lookupTable_cp1161Thai()
    {
        const QByteArray bytes = QByteArray::fromHex("41a1a2f0");
        const QString expected = fromCodepoints({0x0041, 0x0E01, 0x0E02, 0x0E50});
        QCOMPARE(TEncodingHelper::decode(bytes, "CP1161"), expected);
    }

    void decode_lookupTable_undefinedByteBecomesReplacementCharacter() { QCOMPARE(TEncodingHelper::decode(QByteArray::fromHex("81"), "CP1161"), fromCodepoints({0xFFFD})); }

    void roundTrip_lookupTable_cp1161()
    {
        const QString input = fromCodepoints({0x0E01, 0x0E02, 0x0041});
        const QByteArray encoded = TEncodingHelper::encode(input, "CP1161");
        QCOMPARE(encoded, QByteArray::fromHex("a1a241"));
        QCOMPARE(TEncodingHelper::decode(encoded, "CP1161"), input);
    }

    void encode_lookupTable_unrepresentableBecomesQuestionMark()
    {
        const QString input = fromCodepoints({0x0041, 0x4E2D, 0x0042}); // A中B
        QCOMPARE(TEncodingHelper::encode(input, "CP1161"), QByteArrayLiteral("A?B"));
    }

    void canEncode_lookupTable_thai_true() { QVERIFY(TEncodingHelper::canEncode(fromCodepoints({0x0E01, 0x0E02}), "CP1161")); }

    void canEncode_lookupTable_cjk_false() { QVERIFY(!TEncodingHelper::canEncode(fromCodepoints({0x4E2D}), "CP1161")); }

    void isEncodingAvailable_lookupTableEncodings_true()
    {
        for (const QByteArray& encoding : {QByteArrayLiteral("KOI8-R"),
                                           QByteArrayLiteral("KOI8-U"),
                                           QByteArrayLiteral("CP850"),
                                           QByteArrayLiteral("CP866"),
                                           QByteArrayLiteral("CP1161"),
                                           QByteArrayLiteral("WINDOWS-1251"),
                                           QByteArrayLiteral("ISO 8859-15")}) {
            QVERIFY2(TEncodingHelper::isEncodingAvailable(encoding), encoding.constData());
        }
    }
};

QTEST_GUILESS_MAIN(TEncodingHelperTest)

#include "TEncodingHelperTest.moc"
