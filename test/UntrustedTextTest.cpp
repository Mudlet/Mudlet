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
 */
class UntrustedTextTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}

    void testOrdinaryTextUnchanged()
    {
        QCOMPARE(UntrustedText::forDisplay(QStringLiteral("Open browser to: https://www.mudlet.org")), QStringLiteral("Open browser to: https://www.mudlet.org"));
    }

    void testNonLatinTextUnchanged()
    {
        // Sanitization must not damage legitimate non-Latin tooltips.
        const QString text = QStringLiteral("你好 مرحبا Здравствуй");
        QCOMPARE(UntrustedText::forDisplay(text), text);
    }

    void testAstralPlaneTextUnchanged()
    {
        // Surrogate pairs must round-trip: an emoji is two QChars, one code point.
        const QString text = QStringLiteral("a\U0001F600b");
        QCOMPARE(UntrustedText::forDisplay(text), text);
    }

    void testRightToLeftOverrideEscaped()
    {
        QCOMPARE(UntrustedText::forDisplay(QStringLiteral("mudlet‮gro.live")), QStringLiteral("mudlet\\u{202E}gro.live"));
    }

    void testZeroWidthEscaped()
    {
        QCOMPARE(UntrustedText::forDisplay(QStringLiteral("mud​let.org")), QStringLiteral("mud\\u{200B}let.org"));
    }

    void testByteOrderMarkEscaped()
    {
        QCOMPARE(UntrustedText::forDisplay(QStringLiteral("a﻿b")), QStringLiteral("a\\u{FEFF}b"));
    }

    void testControlCharactersEscaped()
    {
        // A newline in a tooltip creates a second visual line that can forge
        // trusted-looking UI text below the real target.
        QCOMPARE(UntrustedText::forDisplay(QStringLiteral("a\nb")), QStringLiteral("a\\u{A}b"));
        QCOMPARE(UntrustedText::forDisplay(QStringLiteral("ab")), QStringLiteral("a\\u{85}b"));
    }

    void testLineSeparatorEscaped()
    {
        QCOMPARE(UntrustedText::forDisplay(QStringLiteral("a b")), QStringLiteral("a\\u{2028}b"));
    }

    void testEmptyText()
    {
        QCOMPARE(UntrustedText::forDisplay(QString()), QString());
    }

    void testClassification()
    {
        QVERIFY(UntrustedText::unsafeCharacter(0x202E));
        QVERIFY(UntrustedText::unsafeCharacter(0x200B));
        QVERIFY(UntrustedText::unsafeCharacter(0x0000));
        QVERIFY(UntrustedText::unsafeCharacter(0x009F));
        QVERIFY(!UntrustedText::unsafeCharacter(0x0041));
        QVERIFY(!UntrustedText::unsafeCharacter(0x00A0));
        QVERIFY(!UntrustedText::unsafeCharacter(0x4F60));
    }
};

QTEST_MAIN(UntrustedTextTest)
#include "UntrustedTextTest.moc"
