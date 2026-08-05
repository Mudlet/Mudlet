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
#include <utils.h>
#include <QFile>
#include <QtTest/QtTest>

class DiscordTest : public QObject
{
    Q_OBJECT

private slots:

    void initTestCase() {}

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
        // Discord documents details as holding 128 bytes, so a longer string is
        // cut down to exactly that - the buffer allows for its own terminator
        // rather than spending one of those 128 bytes on it (#9634).
        QString longString(200, QChar('A'));
        presence.setDetailText(longString);

        DiscordRichPresence converted = presence.convert();
        QVERIFY(converted.details != nullptr);
        QCOMPARE(strlen(converted.details), size_t{128});
    }

    // A field of exactly the documented length has to arrive whole: an asset key
    // that loses its last character resolves to no icon at all (#9634).
    void testFullLengthFieldsSurviveWhole()
    {
        localDiscordPresence presence;
        presence.setLargeImageKey(QString(32, QChar('a')));
        presence.setStateText(QString(128, QChar('s')));

        DiscordRichPresence converted = presence.convert();
        QCOMPARE(strlen(converted.largeImageKey), size_t{32});
        QCOMPARE(strlen(converted.state), size_t{128});
    }

    // Truncation has to fall between characters. A field cut through the middle
    // of a multi-byte one is no longer valid UTF-8, and Discord discards the
    // whole presence frame carrying it rather than just that field (#9634).
    void testTruncationKeepsUtf8Intact()
    {
        localDiscordPresence presence;
        // 65 two-byte characters: 130 bytes, so the cut has to fall inside the
        // 65th and take all of it.
        presence.setDetailText(QString(65, QChar(0x00E9)));
        // 17 of the same in a 32 byte field, which holds 16 of them.
        presence.setLargeImageKey(QString(17, QChar(0x00E9)));

        DiscordRichPresence converted = presence.convert();
        QCOMPARE(QByteArray(converted.details), QString(64, QChar(0x00E9)).toUtf8());
        QCOMPARE(QByteArray(converted.largeImageKey), QString(16, QChar(0x00E9)).toUtf8());
        // A three-byte character has two ways to be cut in half, so check the
        // other one too: 43 of them are 129 bytes.
        presence.setStateText(QString(43, QChar(0x4F60)));
        converted = presence.convert();
        QCOMPARE(QByteArray(converted.state), QString(42, QChar(0x4F60)).toUtf8());

        // And an emoji, the four-byte case, where the walk-back has to step
        // over three continuation bytes: 33 of them are 132 bytes.
        const char32_t grinningFace = 0x1F600;
        const QString emoji = QString::fromUcs4(&grinningFace, 1);
        presence.setDetailText(emoji.repeated(33));
        converted = presence.convert();
        QCOMPARE(QByteArray(converted.details), emoji.repeated(32).toUtf8());
    }

    // The truncation itself, at boundaries the fixed-size presence fields
    // cannot reach.
    void testCopyUtf8StringEdgeCases()
    {
        char buffer[8];
        // Nothing to copy, and a destination too small even to terminate:
        QCOMPARE(utils::copyUtf8String(buffer, sizeof(buffer), "", 0), size_t{0});
        QCOMPARE(utils::copyUtf8String(buffer, 0, "abc", 3), size_t{0});
        // Exactly filling the usable space is not a truncation, so there is
        // nothing to walk back from:
        QCOMPARE(utils::copyUtf8String(buffer, sizeof(buffer), "abcdefg", 7), size_t{7});
        QCOMPARE(QByteArray(buffer), QByteArray("abcdefg"));
        // One byte too many, cut between characters:
        QCOMPARE(utils::copyUtf8String(buffer, sizeof(buffer), "abcdefgh", 8), size_t{7});
        // Input that is nothing but continuation bytes cannot be cut anywhere
        // valid, so an empty field is what comes out - never a broken sequence.
        const char continuationBytes[] = "\x80\x80\x80\x80\x80\x80\x80\x80\x80";
        QCOMPARE(utils::copyUtf8String(buffer, sizeof(buffer), continuationBytes, 9), size_t{0});
        QCOMPARE(QByteArray(buffer), QByteArray());
    }

    // Test that Discord username comparison is case-insensitive.
    // This mirrors the logic in Host::discordUserIdMatch without
    // constructing a Host (which has heavy dependencies).
    void testUserNameComparisonCaseInsensitive()
    {
        // The comparison logic from Host::discordUserIdMatch:
        // if userName and required are both non-empty, compare toLower()
        auto matchesRequired = [](const QString& loggedInUser, const QString& requiredUser) -> bool {
            if (!loggedInUser.isEmpty() && !requiredUser.isEmpty() && loggedInUser.toLower() != requiredUser.toLower()) {
                return false;
            }
            return true;
        };

        // No restriction - should always match
        QVERIFY(matchesRequired(qsl("anyuser"), QString()));
        QVERIFY(matchesRequired(QString(), QString()));

        // Exact lowercase match
        QVERIFY(matchesRequired(qsl("morquin"), qsl("morquin")));
        // Mixed case should still match
        QVERIFY(matchesRequired(qsl("Morquin"), qsl("morquin")));
        QVERIFY(matchesRequired(qsl("MORQUIN"), qsl("morquin")));
        QVERIFY(matchesRequired(qsl("morquin"), qsl("Morquin")));
        // Wrong user should not match
        QVERIFY(!matchesRequired(qsl("someone_else"), qsl("morquin")));
        // Empty logged-in user (not connected yet) should match
        QVERIFY(matchesRequired(QString(), qsl("morquin")));
    }

    // The Lua API permission gating contract: mutators (setDiscord* and
    // resetDiscordData) must require write access - discordApiEnabled(L, true)
    // denies them while the API is read-only because the logged-in Discord
    // user differs from the profile's restriction - while getters only need
    // read access. Exercising the real functions needs a live profile (see
    // TDiscordModeTest); this scans the source instead, like
    // CMakeListsConsistencyTest does, so a swapped flag fails here too.
    void testLuaApiGatingContract()
    {
        QFile source(qsl(MUDLET_SRC_DIR "/TLuaInterpreterDiscord.cpp"));
        QVERIFY2(source.open(QIODevice::ReadOnly | QIODevice::Text), "cannot open TLuaInterpreterDiscord.cpp");
        const QString text = QString::fromUtf8(source.readAll());

        const QStringList chunks = text.split(qsl("int TLuaInterpreter::"));
        int checked = 0;
        for (int i = 1; i < chunks.size(); ++i) {
            const QString& chunk = chunks.at(i);
            const QString name = chunk.left(chunk.indexOf(QLatin1Char('(')));
            const bool readGate = chunk.contains(qsl("discordApiEnabled(L)"));
            const bool writeGate = chunk.contains(qsl("discordApiEnabled(L, true)"));

            if (name == qsl("setDiscordGameUrl")) {
                // Intentionally ungated: the invite URL is not part of rich
                // presence (see the comment in the function itself)
                QVERIFY2(!readGate && !writeGate, qPrintable(qsl("%1 is documented as exempt from the Discord API gate").arg(name)));
                ++checked;
            } else if (name.startsWith(qsl("setDiscord")) || name == qsl("resetDiscordData")) {
                QVERIFY2(writeGate && !readGate, qPrintable(qsl("%1 mutates Discord data so it must call discordApiEnabled(L, true)").arg(name)));
                ++checked;
            } else if (name.startsWith(qsl("getDiscord")) || name == qsl("usingMudletsDiscordID")) {
                QVERIFY2(readGate && !writeGate, qPrintable(qsl("%1 only reads Discord data so it must call discordApiEnabled(L)").arg(name)));
                ++checked;
            }
        }
        // All 22 Discord Lua API functions should have been categorised:
        QVERIFY2(checked >= 22, qPrintable(qsl("only categorised %1 Discord Lua functions - has the source moved?").arg(checked)));
    }

    void cleanupTestCase() {}
};

#include "DiscordTest.moc"
QTEST_MAIN(DiscordTest)
