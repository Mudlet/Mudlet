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

#include "UntrustedText.h"

#include <QtTest/QtTest>

/*
 * A game server chooses the tooltip, menu labels and menu title of an OSC 8
 * link, and Mudlet shows the link's target URL in the default hint. Bidi
 * overrides and zero-width characters let that text claim one target while the
 * link carries another, so they are escaped into a visible form before display.
 *
 * Test inputs spell invisible code points as \uXXXX escapes rather than
 * embedding them raw: editors and review tools that strip invisible characters
 * have silently deleted them from source before, which turns a real assertion
 * into one that passes for the wrong reason.
 */
class UntrustedTextTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}

    void testOrdinaryTextUnchanged() { QCOMPARE(UntrustedText::forTarget(QStringLiteral("Open browser to: https://www.mudlet.org")), QStringLiteral("Open browser to: https://www.mudlet.org")); }

    void testNonLatinTextUnchanged()
    {
        // Sanitization must not damage legitimate non-Latin tooltips.
        const QString text = QStringLiteral("你好 مرحبا Здравствуй");
        QCOMPARE(UntrustedText::forTarget(text), text);
    }

    void testAstralPlaneTextUnchanged()
    {
        // Surrogate pairs must round-trip: an emoji is two QChars, one code point.
        const QString text = QStringLiteral("a\U0001F600b");
        QCOMPARE(UntrustedText::forTarget(text), text);
    }

    void testRightToLeftOverrideEscaped() { QCOMPARE(UntrustedText::forTarget(QStringLiteral("mudlet\u202Egro.live")), QStringLiteral("mudlet\\u{202E}gro.live")); }

    void testZeroWidthEscaped() { QCOMPARE(UntrustedText::forTarget(QStringLiteral("mud\u200Blet.org")), QStringLiteral("mud\\u{200B}let.org")); }

    void testByteOrderMarkEscaped() { QCOMPARE(UntrustedText::forTarget(QStringLiteral("a\uFEFFb")), QStringLiteral("a\\u{FEFF}b")); }

    void testControlCharactersEscaped()
    {
        // A newline in a tooltip creates a second visual line that can forge
        // trusted-looking UI text below the real target.
        QCOMPARE(UntrustedText::forTarget(QStringLiteral("a\nb")), QStringLiteral("a\\u{A}b"));
        QCOMPARE(UntrustedText::forTarget(QStringLiteral("a\u0085b")), QStringLiteral("a\\u{85}b"));
    }

    void testLineSeparatorEscaped() { QCOMPARE(UntrustedText::forTarget(QStringLiteral("a\u2028b")), QStringLiteral("a\\u{2028}b")); }

    void testEmptyText() { QCOMPARE(UntrustedText::forTarget(QString()), QString()); }

    void testLiteralEscapeSequenceDisambiguated()
    {
        // Server text containing the six characters \u{202E} must not display
        // the same as a sanitized real U+202E.
        QCOMPARE(UntrustedText::forTarget(QStringLiteral("a\\u{202E}b")), QStringLiteral("a\\u{5C}u{202E}b"));
        // A backslash not starting a \u{ sequence is left alone.
        QCOMPARE(UntrustedText::forTarget(QStringLiteral("C:\\mud\\maps")), QStringLiteral("C:\\mud\\maps"));
    }

    // Emoji in tooltips and menu labels are a documented OSC 8 feature and are
    // used in the wild. The strict policy escapes the joiners and tag
    // characters they are assembled from, so authored text must not use it.
    void testAuthoredTextKeepsEmoji_data()
    {
        QTest::addColumn<QString>("emoji");

        QTest::newRow("rainbow flag") << QStringLiteral("\U0001F3F3\uFE0F\u200D\U0001F308");
        QTest::newRow("pirate flag") << QStringLiteral("\U0001F3F4\u200D☠\uFE0F");
        QTest::newRow("man cook") << QStringLiteral("\U0001F468\u200D\U0001F373");
        QTest::newRow("family") << QStringLiteral("\U0001F468\u200D\U0001F469\u200D\U0001F467");
        QTest::newRow("scotland flag") << QStringLiteral("\U0001F3F4\U000E0067\U000E0062\U000E0073\U000E0063\U000E0074\U000E007F");
        QTest::newRow("plain emoji") << QStringLiteral("⚔\uFE0F");
        QTest::newRow("skin tone") << QStringLiteral("\U0001F44D\U0001F3FD");
        QTest::newRow("regional flag") << QStringLiteral("\U0001F1EC\U0001F1E7");
    }

    void testAuthoredTextKeepsEmoji()
    {
        QFETCH(QString, emoji);
        QCOMPARE(UntrustedText::forAuthoredText(emoji), emoji);
    }

    void testAuthoredTextKeepsPersianShaping()
    {
        // ZWNJ separates the prefix in this Persian verb; escaping it changes
        // how the word is shaped and read.
        const QString text = QStringLiteral("می\u200Cرود");
        QCOMPARE(UntrustedText::forAuthoredText(text), text);
    }

    void testAuthoredTextEscapesUnusableTagCharacters()
    {
        // The exception covers only what an emoji flag needs. The deprecated
        // language tag and the unassigned code points below it are not that.
        QCOMPARE(UntrustedText::forAuthoredText(QStringLiteral("a\U000E0001b")), QStringLiteral("a\\u{E0001}b"));
        QCOMPARE(UntrustedText::forAuthoredText(QStringLiteral("a\U000E0000b")), QStringLiteral("a\\u{E0000}b"));
        QCOMPARE(UntrustedText::forAuthoredText(QStringLiteral("a\U000E001Fb")), QStringLiteral("a\\u{E001F}b"));
        // The first assigned tag character is where the exception starts.
        QCOMPARE(UntrustedText::forAuthoredText(QStringLiteral("a\U000E0020b")), QStringLiteral("a\U000E0020b"));
    }

    void testTargetStillEscapesWhatAuthoredTextKeeps()
    {
        // The same characters must not survive in a link target, where they
        // would hide part of what the user is being asked to trust.
        QCOMPARE(UntrustedText::forTarget(QStringLiteral("a\u200Db")), QStringLiteral("a\\u{200D}b"));
        QCOMPARE(UntrustedText::forTarget(QStringLiteral("a\u200Cb")), QStringLiteral("a\\u{200C}b"));
        QCOMPARE(UntrustedText::forTarget(QStringLiteral("a\U000E0067b")), QStringLiteral("a\\u{E0067}b"));
    }

    void testAuthoredTextStillEscapesReordering()
    {
        // Relaxing the joiners must not relax the characters that let a label
        // misrepresent itself or forge a second line of UI.
        QCOMPARE(UntrustedText::forAuthoredText(QStringLiteral("mudlet\u202Egro.live")), QStringLiteral("mudlet\\u{202E}gro.live"));
        QCOMPARE(UntrustedText::forAuthoredText(QStringLiteral("a\u2066b")), QStringLiteral("a\\u{2066}b"));
        QCOMPARE(UntrustedText::forAuthoredText(QStringLiteral("a\nb")), QStringLiteral("a\\u{A}b"));
        QCOMPARE(UntrustedText::forAuthoredText(QStringLiteral("a\u2028b")), QStringLiteral("a\\u{2028}b"));
        QCOMPARE(UntrustedText::forAuthoredText(QStringLiteral("a\u200Bb")), QStringLiteral("a\\u{200B}b"));
        QCOMPARE(UntrustedText::forAuthoredText(QStringLiteral("a\uFEFFb")), QStringLiteral("a\\u{FEFF}b"));
    }

    void testClassification()
    {
        QVERIFY(UntrustedText::unsafeCharacter(0x202E));
        QVERIFY(UntrustedText::unsafeCharacter(0x200B));
        QVERIFY(UntrustedText::unsafeCharacter(0x0000));
        QVERIFY(UntrustedText::unsafeCharacter(0x009F));
        // Bidi embeddings and overrides, and the isolates that replaced them.
        QVERIFY(UntrustedText::unsafeCharacter(0x202A));
        QVERIFY(UntrustedText::unsafeCharacter(0x202D));
        QVERIFY(UntrustedText::unsafeCharacter(0x2066));
        QVERIFY(UntrustedText::unsafeCharacter(0x2069));
        // Arabic letter mark, zero-width joiners and the word joiner.
        QVERIFY(UntrustedText::unsafeCharacter(0x061C));
        QVERIFY(UntrustedText::unsafeCharacter(0x200C));
        QVERIFY(UntrustedText::unsafeCharacter(0x200D));
        QVERIFY(UntrustedText::unsafeCharacter(0x2060));
        // Both ends of the invisible-by-design tag character block.
        QVERIFY(UntrustedText::unsafeCharacter(0xE0000));
        QVERIFY(UntrustedText::unsafeCharacter(0xE007F));
        QVERIFY(!UntrustedText::unsafeCharacter(0x0041));
        QVERIFY(!UntrustedText::unsafeCharacter(0x00A0));
        QVERIFY(!UntrustedText::unsafeCharacter(0x4F60));
    }

    void testAuthoredClassificationDiffersOnlyWhereIntended()
    {
        // The authored policy is the strict one minus exactly three things.
        for (char32_t codePoint = 0; codePoint <= 0xE0100; ++codePoint) {
            const bool relaxed = UntrustedText::unsafeCharacter(codePoint) && !UntrustedText::unsafeAuthoredCharacter(codePoint);
            const bool expected = codePoint == 0x200C || codePoint == 0x200D || (codePoint >= 0xE0020 && codePoint <= 0xE007F);
            if (relaxed != expected) {
                QFAIL(qPrintable(QStringLiteral("policies diverge unexpectedly at U+%1").arg(QString::number(static_cast<uint>(codePoint), 16).toUpper())));
            }
            // Authored text may never mark something unsafe that strict does not.
            QVERIFY(!(UntrustedText::unsafeAuthoredCharacter(codePoint) && !UntrustedText::unsafeCharacter(codePoint)));
        }
    }
};

QTEST_MAIN(UntrustedTextTest)
#include "UntrustedTextTest.moc"
