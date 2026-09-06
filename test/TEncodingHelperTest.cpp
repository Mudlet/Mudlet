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

    inline static const QList<QByteArray> csmOwnCodePages{"CP437", "CP667", "CP737", "CP869", "MEDIEVIA"};

    static QByteArray upperHalf()
    {
        QByteArray bytes;
        for (int byte = 0x80; byte <= 0xFF; ++byte) {
            bytes.append(static_cast<char>(byte));
        }
        return bytes;
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
    // Mudlet's own code pages - CP437, CP667, CP737, CP869 and MEDIEVIA are
    // answered from the tables in TEncodingTable.cpp ahead of anything Qt
    // supplies under the same name. The "M_" prefix they once registered under
    // is still accepted.
    // -------------------------------------------------------------------------

    void encode_cp737_lowercaseGreek()
    {
        // 0xA0 ι, 0xA1 κ, 0xF0 Ώ
        QCOMPARE(TEncodingHelper::encode(fromCodepoints({0x03B9, 0x03BA, 0x038F}), "CP737"), QByteArray::fromHex("a0a1f0"));
    }

    void canEncode_cp737_lowercaseGreek_true() { QVERIFY(TEncodingHelper::canEncode(fromCodepoints({0x03B9}), "CP737")); }

    // CP737 has no accented Latin letters: a table that accepts Ü has CP437's
    // 0x98-0xAF row in place of CP737's lowercase Greek.
    void canEncode_cp737_capitalUUmlaut_false() { QVERIFY(!TEncodingHelper::canEncode(fromCodepoints({0x00DC}), "CP737")); }

    void encode_cp667_oAcuteAndTheRunAfterIt()
    {
        // 0xA3 Ó, 0xA4 ń, 0xA5 Ń, 0xA6 ź, 0xA7 ż
        QCOMPARE(TEncodingHelper::encode(fromCodepoints({0x00D3, 0x0144, 0x0143, 0x017A, 0x017C}), "CP667"), QByteArray::fromHex("a3a4a5a6a7"));
    }

    // 0x87 is the euro sign in Mudlet's CP869 and undefined in the ICU table Qt
    // offers under that name, so this holds only while Mudlet's own table
    // answers ahead of Qt.
    void decode_cp869_euroFromMudletsOwnTable() { QCOMPARE(TEncodingHelper::decode(QByteArray::fromHex("87"), "CP869"), fromCodepoints({0x20AC})); }

    void prefixedName_reachesTheSameTable()
    {
        const QByteArray upper = upperHalf();
        for (const QByteArray& name : csmOwnCodePages) {
            const QByteArray prefixed = "M_" + name;
            const QString decoded = TEncodingHelper::decode(upper, name);
            // went through a table, not the Latin-1 fallthrough
            QVERIFY2(decoded != QString::fromLatin1(upper), name.constData());
            QCOMPARE(TEncodingHelper::decode(upper, prefixed), decoded);
            QCOMPARE(TEncodingHelper::encode(decoded, prefixed), TEncodingHelper::encode(decoded, name));
            QCOMPARE(TEncodingHelper::canEncode(decoded, prefixed), TEncodingHelper::canEncode(decoded, name));
            QVERIFY2(TEncodingHelper::isEncodingAvailable(prefixed), prefixed.constData());
        }
    }

    // Every byte a code page defines has to come back as itself after decoding
    // and encoding, which also proves no two bytes share a code point.
    void roundTrip_ownCodePages_everyDefinedByte()
    {
        for (const QByteArray& name : csmOwnCodePages) {
            for (int byte = 0x80; byte <= 0xFF; ++byte) {
                const QByteArray original(1, static_cast<char>(byte));
                const QString character = TEncodingHelper::decode(original, name);
                if (character == QString(QChar(0xFFFD))) {
                    continue;
                }
                QVERIFY2(TEncodingHelper::encode(character, name) == original, (name + " 0x" + QByteArray::number(byte, 16)).constData());
            }
        }
    }
};

QTEST_GUILESS_MAIN(TEncodingHelperTest)

#include "TEncodingHelperTest.moc"
