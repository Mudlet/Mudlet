/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Authors                                  *
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

#include <QtTest/QtTest>
#include <cstring>

/*
 * Test for issue #1065: Off-by-one error in cTelnet::processSocketData()
 *
 * The original buggy code was:
 *     in_buffer[amount + 1] = '\0';
 *     if (amount == -1) { return; }
 *     if (amount == 0) { return; }
 *
 * This had two problems:
 * 1. Null terminator at wrong position (amount + 1 instead of amount)
 * 2. Buffer access before validity check
 *
 * The fix:
 *     if (amount == -1 || amount == 0) { return; }
 *     in_buffer[amount] = '\0';
 *
 * Since cTelnet has many dependencies, this test validates the buffer
 * handling logic pattern in isolation.
 */

class cTelnetBufferTest : public QObject
{
    Q_OBJECT

private:
    // Marker byte to detect unintended writes
    static constexpr char MARKER_BYTE = '\x7F';

    // Simulates the FIXED buffer handling logic from processSocketData
    static bool processBufferFixed(char* buffer, int amount)
    {
        if (amount == -1 || amount == 0) {
            return false;
        }
        buffer[amount] = '\0';
        return true;
    }

    // Simulates the BUGGY buffer handling logic (for comparison)
    static bool processBufferBuggy(char* buffer, int amount)
    {
        buffer[amount + 1] = '\0';  // BUG: wrong position, before validation
        if (amount == -1) {
            return false;
        }
        if (amount == 0) {
            return false;
        }
        return true;
    }

private slots:
    void initTestCase()
    {
    }

    // Test that null terminator is placed at correct position
    void testNullTerminatorPosition()
    {
        constexpr int BUFFER_SIZE = 100;
        char buffer[BUFFER_SIZE];

        // Fill buffer with marker bytes
        std::memset(buffer, MARKER_BYTE, BUFFER_SIZE);

        // Simulate receiving 32 bytes of data
        const char* testData = "01234567890123456789012345678901";
        const int amount = 32;
        std::memcpy(buffer, testData, amount);

        // Process with fixed logic
        bool result = processBufferFixed(buffer, amount);

        QVERIFY(result);
        // Null terminator should be at position 32 (index of first byte after data)
        QCOMPARE(buffer[amount], '\0');
        // Position 33 should still have marker (not touched)
        QCOMPARE(buffer[amount + 1], MARKER_BYTE);
        // Data should be intact
        QCOMPARE(std::strncmp(buffer, testData, amount), 0);
    }

    // Test that buggy code puts null terminator at wrong position
    void testBuggyNullTerminatorPosition()
    {
        constexpr int BUFFER_SIZE = 100;
        char buffer[BUFFER_SIZE];

        std::memset(buffer, MARKER_BYTE, BUFFER_SIZE);

        const char* testData = "01234567890123456789012345678901";
        const int amount = 32;
        std::memcpy(buffer, testData, amount);

        // Process with buggy logic
        processBufferBuggy(buffer, amount);

        // BUG: Null terminator at position 33 instead of 32
        QCOMPARE(buffer[amount], MARKER_BYTE);  // Position 32 still has marker (bug!)
        QCOMPARE(buffer[amount + 1], '\0');     // Null at wrong position
    }

    // Test various buffer boundary sizes
    void testBufferBoundaries_data()
    {
        QTest::addColumn<int>("amount");

        QTest::newRow("31 bytes") << 31;
        QTest::newRow("32 bytes") << 32;
        QTest::newRow("33 bytes") << 33;
        QTest::newRow("63 bytes") << 63;
        QTest::newRow("64 bytes") << 64;
        QTest::newRow("127 bytes") << 127;
        QTest::newRow("128 bytes") << 128;
    }

    void testBufferBoundaries()
    {
        QFETCH(int, amount);

        constexpr int BUFFER_SIZE = 256;
        char buffer[BUFFER_SIZE];

        // Fill with marker bytes
        std::memset(buffer, MARKER_BYTE, BUFFER_SIZE);

        // Fill with test pattern up to amount
        for (int i = 0; i < amount; ++i) {
            buffer[i] = 'A' + (i % 26);
        }

        bool result = processBufferFixed(buffer, amount);

        QVERIFY(result);
        QCOMPARE(buffer[amount], '\0');
        QCOMPARE(buffer[amount + 1], MARKER_BYTE);
    }

    // Test that amount == -1 returns early without buffer access
    void testAmountNegativeOne()
    {
        constexpr int BUFFER_SIZE = 100;
        char buffer[BUFFER_SIZE];

        // Fill with marker bytes - none should be modified
        std::memset(buffer, MARKER_BYTE, BUFFER_SIZE);

        bool result = processBufferFixed(buffer, -1);

        QVERIFY(!result);
        // Buffer should be untouched
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            QCOMPARE(buffer[i], MARKER_BYTE);
        }
    }

    // Test that amount == 0 returns early without buffer access
    void testAmountZero()
    {
        constexpr int BUFFER_SIZE = 100;
        char buffer[BUFFER_SIZE];

        std::memset(buffer, MARKER_BYTE, BUFFER_SIZE);

        bool result = processBufferFixed(buffer, 0);

        QVERIFY(!result);
        // Buffer should be untouched
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            QCOMPARE(buffer[i], MARKER_BYTE);
        }
    }

    // Test that buggy code modifies buffer even on invalid amount
    void testBuggyModifiesBufferOnInvalidAmount()
    {
        constexpr int BUFFER_SIZE = 100;
        char buffer[BUFFER_SIZE];

        std::memset(buffer, MARKER_BYTE, BUFFER_SIZE);

        // Buggy code writes to buffer BEFORE checking amount
        processBufferBuggy(buffer, -1);

        // BUG: buffer[0] was written to (amount + 1 = 0)
        QCOMPARE(buffer[0], '\0');  // This demonstrates the bug
    }

    // Test string length after null termination
    void testStringLengthAfterTermination()
    {
        constexpr int BUFFER_SIZE = 100;
        char buffer[BUFFER_SIZE];

        const char* testData = "Hello, World!";
        const int amount = static_cast<int>(std::strlen(testData));
        std::memcpy(buffer, testData, amount);

        processBufferFixed(buffer, amount);

        // String length should match amount
        QCOMPARE(static_cast<int>(std::strlen(buffer)), amount);
        QCOMPARE(QString::fromLatin1(buffer), QString::fromLatin1(testData));
    }

    void cleanupTestCase()
    {
    }
};

#include "cTelnetBufferTest.moc"
QTEST_MAIN(cTelnetBufferTest)
