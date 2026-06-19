/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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

#include <TEncodingHelper.h>

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
};

QTEST_GUILESS_MAIN(TEncodingHelperTest)

#include "TEncodingHelperTest.moc"
