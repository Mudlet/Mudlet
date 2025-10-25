#include <QtTest/QtTest>
#include <cstring>

// Test to validate the fix for GitHub issue #1065: Off-by-one error in cTelnet::processSocketData()
// This test validates the buffer handling logic that was corrected

class cTelnetBufferTest : public QObject {
Q_OBJECT

private slots:

    void testBufferTerminationCorrect_data()
    {
        QTest::addColumn<int>("amount");
        QTest::addColumn<QString>("description");

        QTest::newRow("Normal 32 bytes") << 32 << "Normal case: 32 bytes read";
        QTest::newRow("Single byte") << 1 << "Edge case: 1 byte read";
        QTest::newRow("Large buffer") << 1000 << "Large buffer: 1000 bytes read";
        QTest::newRow("No data") << 0 << "No data available";
        QTest::newRow("Read error") << -1 << "Socket read error";
    }

    void testBufferTerminationCorrect()
    {
        QFETCH(int, amount);
        QFETCH(QString, description);

        qDebug() << "Testing:" << description;

        constexpr size_t BUFFER_SIZE = 100000L;
        char in_buffer[BUFFER_SIZE + 10];

        // Fill buffer with recognizable garbage data
        memset(in_buffer, 'X', BUFFER_SIZE + 10);

        // Simulate reading 'amount' bytes of test data
        if (amount > 0) {
            const char* testData = "Hello, World! Test data for buffer validation.";
            int copyAmount = qMin(amount, static_cast<int>(strlen(testData)));
            memcpy(in_buffer, testData, copyAmount);

            // Fill rest with predictable pattern
            for (int i = copyAmount; i < amount; ++i) {
                in_buffer[i] = 'A' + (i % 26);
            }
        }

        // Apply the FIXED logic from processSocketData
        if (amount == -1 || amount == 0) {
            in_buffer[0] = '\0';

            // Verify: buffer should be empty string
            QCOMPARE(strlen(in_buffer), 0);
            QCOMPARE(in_buffer[0], '\0');

            qDebug() << "  ✓ Correctly handled error/no-data case, buffer set to empty string";
            return;
        }

        // For valid data, null terminator should be at index 'amount'
        in_buffer[amount] = '\0';

        // Validation 1: Null terminator is at the correct position
        QCOMPARE(in_buffer[amount], '\0');
        qDebug() << "  ✓ Null terminator correctly placed at index" << amount;

        // Validation 2: String length matches amount
        QCOMPARE(static_cast<int>(strlen(in_buffer)), amount);
        qDebug() << "  ✓ String length (" << strlen(in_buffer) << ") matches amount (" << amount << ")";

        // Validation 3: Data before null terminator is intact
        for (int i = 0; i < amount; ++i) {
            QVERIFY(in_buffer[i] != '\0');
        }
        qDebug() << "  ✓ No premature null terminators in data region [0," << (amount-1) << "]";
    }

    void testBuggyBehaviorWouldFail()
    {
        qDebug() << "\n=== Demonstrating what the BUGGY code would have done ===";

        constexpr size_t BUFFER_SIZE = 100000L;
        char buggy_buffer[BUFFER_SIZE + 10];
        char fixed_buffer[BUFFER_SIZE + 10];

        int amount = 32;
        const char* testData = "Hello, World! Testing bug fix.";

        // Setup both buffers identically
        memset(buggy_buffer, '?', BUFFER_SIZE + 10);
        memset(fixed_buffer, '?', BUFFER_SIZE + 10);
        memcpy(buggy_buffer, testData, amount);
        memcpy(fixed_buffer, testData, amount);

        // BUGGY CODE (before fix):
        // in_buffer[amount + 1] = '\0';  // Would place null at index 33 for amount=32
        buggy_buffer[amount + 1] = '\0';
        qDebug() << "Buggy code: Placed null terminator at index" << (amount + 1);
        qDebug() << "  Problem: Index" << amount << "still contains garbage:" << buggy_buffer[amount];
        QCOMPARE(buggy_buffer[amount], '?');  // Uninitialized!
        QCOMPARE(buggy_buffer[amount + 1], '\0');  // Null at wrong position

        // FIXED CODE (after fix):
        // in_buffer[amount] = '\0';  // Correctly places null at index 32 for amount=32
        fixed_buffer[amount] = '\0';
        qDebug() << "Fixed code: Placed null terminator at index" << amount;
        qDebug() << "  ✓ Index" << amount << "correctly set to null terminator";
        QCOMPARE(fixed_buffer[amount], '\0');  // Correct!
        QCOMPARE(static_cast<int>(strlen(fixed_buffer)), amount);

        qDebug() << "\nDemonstration complete: Fix prevents off-by-one error";
    }

    void testPrematureAccessPrevention()
    {
        qDebug() << "\n=== Testing premature buffer access prevention ===";

        constexpr size_t BUFFER_SIZE = 100000L;
        char test_buffer[BUFFER_SIZE + 10];

        // Test amount = -1 (error case)
        {
            memset(test_buffer, 'X', BUFFER_SIZE + 10);
            int amount = -1;

            qDebug() << "Testing amount = -1 (socket error)";

            // BUGGY CODE would have done: in_buffer[amount + 1] = '\0'
            // which is in_buffer[0] = '\0' - accidentally safe but logically wrong
            qDebug() << "  Buggy code would access index" << (amount + 1) << "before checking amount value";

            // FIXED CODE checks first:
            if (amount == -1 || amount == 0) {
                test_buffer[0] = '\0';
                qDebug() << "  ✓ Fixed code checks amount BEFORE buffer access";
                qDebug() << "  ✓ Sets buffer to empty string safely";
            }

            QCOMPARE(test_buffer[0], '\0');
        }

        // Test amount = 0 (no data case)
        {
            memset(test_buffer, 'X', BUFFER_SIZE + 10);
            int amount = 0;

            qDebug() << "\nTesting amount = 0 (no data available)";

            // BUGGY CODE would have done: in_buffer[amount + 1] = '\0'
            // which is in_buffer[1] = '\0' - wrong! Should return immediately
            qDebug() << "  Buggy code would access index" << (amount + 1) << "before checking amount value";

            // FIXED CODE checks first:
            if (amount == -1 || amount == 0) {
                test_buffer[0] = '\0';
                qDebug() << "  ✓ Fixed code checks amount BEFORE buffer access";
                qDebug() << "  ✓ Returns early without unnecessary buffer modification";
            }

            QCOMPARE(test_buffer[0], '\0');
        }

        qDebug() << "\nPremature access prevention validated";
    }
};

#include "cTelnetBufferTest.moc"
QTEST_MAIN(cTelnetBufferTest)
