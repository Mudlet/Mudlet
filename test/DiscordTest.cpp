/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Makers                                   *
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

#include <discord.h>
#include <Host.h>
#include <QtTest/QtTest>

class DiscordTest : public QObject {
    Q_OBJECT

private slots:

    void initTestCase()
    {
    }

    // Test that convert() returns nullptr for empty string fields
    void testConvertNullIfEmpty()
    {
        localDiscordPresence presence;
        // All strings default to empty char arrays
        DiscordRichPresence converted = presence.convert();

        QVERIFY2(converted.state == nullptr, "Empty state should convert to nullptr");
        QVERIFY2(converted.details == nullptr, "Empty details should convert to nullptr");
        QVERIFY2(converted.largeImageKey == nullptr, "Empty largeImageKey should convert to nullptr");
        QVERIFY2(converted.largeImageText == nullptr, "Empty largeImageText should convert to nullptr");
        QVERIFY2(converted.smallImageKey == nullptr, "Empty smallImageKey should convert to nullptr");
        QVERIFY2(converted.smallImageText == nullptr, "Empty smallImageText should convert to nullptr");
        QVERIFY2(converted.partyId == nullptr, "Empty partyId should convert to nullptr");
        QVERIFY2(converted.matchSecret == nullptr, "Empty matchSecret should convert to nullptr");
        QVERIFY2(converted.joinSecret == nullptr, "Empty joinSecret should convert to nullptr");
        QVERIFY2(converted.spectateSecret == nullptr, "Empty spectateSecret should convert to nullptr");
    }

    // Test that convert() returns valid pointers for non-empty strings
    void testConvertNonEmpty()
    {
        localDiscordPresence presence;
        presence.setDetailText(qsl("test detail"));
        presence.setStateText(qsl("test state"));
        presence.setLargeImageKey(qsl("icon-key"));

        DiscordRichPresence converted = presence.convert();

        QVERIFY(converted.details != nullptr);
        QCOMPARE(QString::fromUtf8(converted.details), qsl("test detail"));
        QVERIFY(converted.state != nullptr);
        QCOMPARE(QString::fromUtf8(converted.state), qsl("test state"));
        QVERIFY(converted.largeImageKey != nullptr);
        QCOMPARE(QString::fromUtf8(converted.largeImageKey), qsl("icon-key"));
        // Fields we didn't set should still be nullptr
        QVERIFY(converted.smallImageKey == nullptr);
    }

    // Test that timestamps are correctly set and cleared
    void testConvertTimestamps()
    {
        localDiscordPresence presence;

        // Default timestamps should be 0
        DiscordRichPresence converted = presence.convert();
        QCOMPARE(converted.startTimestamp, static_cast<int64_t>(0));
        QCOMPARE(converted.endTimestamp, static_cast<int64_t>(0));

        // Set a start timestamp
        presence.setStartTimeStamp(1234567890);
        converted = presence.convert();
        QCOMPARE(converted.startTimestamp, static_cast<int64_t>(1234567890));
        QCOMPARE(converted.endTimestamp, static_cast<int64_t>(0));

        // Clear it back to 0
        presence.setStartTimeStamp(0);
        converted = presence.convert();
        QCOMPARE(converted.startTimestamp, static_cast<int64_t>(0));
    }

    // Test that party size/max are correctly set
    void testConvertParty()
    {
        localDiscordPresence presence;

        DiscordRichPresence converted = presence.convert();
        QCOMPARE(converted.partySize, 0);
        QCOMPARE(converted.partyMax, 0);

        presence.setPartySize(3);
        presence.setPartyMax(10);
        converted = presence.convert();
        QCOMPARE(converted.partySize, 3);
        QCOMPARE(converted.partyMax, 10);

        // Clear party
        presence.setPartySize(0);
        presence.setPartyMax(0);
        converted = presence.convert();
        QCOMPARE(converted.partySize, 0);
        QCOMPARE(converted.partyMax, 0);
    }

    // Test that setDetailText with empty string produces nullptr in convert
    void testEmptyDetailProducesNull()
    {
        localDiscordPresence presence;
        presence.setDetailText(QString());

        DiscordRichPresence converted = presence.convert();
        // Empty string should convert to nullptr via nullIfEmpty
        QVERIFY2(converted.details == nullptr, "Empty detail text should convert to nullptr");
    }

    // Test DiscordMode enum values match expected integers for serialization
    void testDiscordModeValues()
    {
        QCOMPARE(static_cast<int>(Host::DiscordDisabled), 0);
        QCOMPARE(static_cast<int>(Host::DiscordShowMudletOnly), 1);
        QCOMPARE(static_cast<int>(Host::DiscordShowGameDetails), 2);
    }

    // Test DiscordOptionFlag values for server-origin tracking compatibility
    void testDiscordOptionFlagValues()
    {
        // Verify the sub-mask covers all individual field flags
        QVERIFY(Host::DiscordSetSubMask & Host::DiscordSetDetail);
        QVERIFY(Host::DiscordSetSubMask & Host::DiscordSetState);
        QVERIFY(Host::DiscordSetSubMask & Host::DiscordSetLargeIcon);
        QVERIFY(Host::DiscordSetSubMask & Host::DiscordSetLargeIconText);
        QVERIFY(Host::DiscordSetSubMask & Host::DiscordSetSmallIcon);
        QVERIFY(Host::DiscordSetSubMask & Host::DiscordSetSmallIconText);
        QVERIFY(Host::DiscordSetSubMask & Host::DiscordSetPartyInfo);
        QVERIFY(Host::DiscordSetSubMask & Host::DiscordSetTimeInfo);

        // DiscordLuaAccessEnabled should NOT be in the sub-mask
        QVERIFY(!(Host::DiscordSetSubMask & Host::DiscordLuaAccessEnabled));
    }

    // Test that string truncation works for fields exceeding buffer size
    void testStringTruncation()
    {
        localDiscordPresence presence;
        // Details buffer is 128 bytes - test with a string longer than that
        QString longString(200, QChar('A'));
        presence.setDetailText(longString);

        DiscordRichPresence converted = presence.convert();
        QVERIFY(converted.details != nullptr);
        // Should be truncated but not crash
        QVERIFY(strlen(converted.details) < 128);
    }

    void cleanupTestCase()
    {
    }
};

#include "DiscordTest.moc"
QTEST_MAIN(DiscordTest)
