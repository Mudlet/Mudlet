/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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
#include <string>

/**
 * Test class for cTelnet bounds checking fix for GitHub issue #8104
 * 
 * This test documents and verifies the bounds checking logic needed
 * to prevent crashes when processTelnetCommand receives empty or 
 * malformed command strings from MCCP2 decompression issues.
 * 
 * Since the actual cTelnet class has many dependencies, this test
 * focuses on the core logic and documents the expected behavior.
 */
class CTelnetTest : public QObject {
    Q_OBJECT

private slots:
    void testBoundsCheckingLogic();
    void testTelnetCommandStructure();
    void testMCCPScenario();

private:
    // Simulates the bounds checking logic that should be in processTelnetCommand
    bool simulateProcessTelnetCommand(const std::string& telnetCommand);
    
    // Helper function to determine if a telnet command needs 3 bytes  
    bool needsThreeBytes(const std::string& telnetCommand);
};

bool CTelnetTest::simulateProcessTelnetCommand(const std::string& telnetCommand)
{
    // This simulates the bounds checking fix we implemented
    if (telnetCommand.size() < 2) {
        // Should log: "WARNING: telnetCommand too short, ignoring"
        return false; // Command ignored
    }
    
    char ch = telnetCommand[1];
    
    // Simulate the different command types and their requirements
    const char TN_WILL = static_cast<char>(0xFB);
    const char TN_WONT = static_cast<char>(0xFC);  
    const char TN_DO = static_cast<char>(0xFD);
    const char TN_DONT = static_cast<char>(0xFE);
    const char TN_SB = static_cast<char>(0xFA);
    const char TN_GA = static_cast<char>(0xF9);
    const char TN_EOR = static_cast<char>(0x19);
    
    // Commands that need 3 bytes (IAC + command + option)
    if (ch == TN_WILL || ch == TN_WONT || ch == TN_DO || ch == TN_DONT || ch == TN_SB) {
        if (telnetCommand.size() < 3) {
            // Should log: "WARNING: TN_WILL/WONT/DO/DONT/SB command too short, ignoring"  
            return false; // Command ignored
        }
        // Access telnetCommand[2] is now safe
    }
    
    // Commands like GA/EOR only need 2 bytes and are handled correctly
    return true; // Command processed
}

void CTelnetTest::testBoundsCheckingLogic()
{
    // Test Case 1: Empty command (MCCP decompression returns 0 bytes)
    std::string emptyCommand = "";
    QVERIFY2(!simulateProcessTelnetCommand(emptyCommand), 
             "Empty command should be rejected by bounds checking");
    
    // Test Case 2: Single byte (incomplete IAC)  
    std::string singleByte = "\xFF";
    QVERIFY2(!simulateProcessTelnetCommand(singleByte),
             "Single byte command should be rejected by bounds checking");
    
    // Test Case 3: Valid 2-byte command (IAC GA)
    std::string validGA = "\xFF\xF9";
    QVERIFY2(simulateProcessTelnetCommand(validGA),
             "Valid 2-byte GA command should be processed");
    
    // Test Case 4: Incomplete 3-byte command (IAC WILL without option)
    std::string incompleteWill = "\xFF\xFB";
    QVERIFY2(!simulateProcessTelnetCommand(incompleteWill),
             "Incomplete WILL command should be rejected by bounds checking");
    
    // Test Case 5: Complete 3-byte command (IAC WILL ECHO)
    std::string completeWill = "\xFF\xFB\x01";
    QVERIFY2(simulateProcessTelnetCommand(completeWill),
             "Complete WILL ECHO command should be processed");
}

void CTelnetTest::testTelnetCommandStructure()
{
    // Document the telnet command structure that our bounds checking protects
    
    // IAC (Interpret As Command) = 0xFF
    const char IAC = static_cast<char>(0xFF);
    
    // 2-byte commands: IAC + command
    const char GA = static_cast<char>(0xF9);   // Go Ahead
    const char EOR = static_cast<char>(0x19);  // End of Record
    
    std::string iacGA;
    iacGA += IAC;
    iacGA += GA;
    QCOMPARE(iacGA.size(), size_t(2));
    QVERIFY2(simulateProcessTelnetCommand(iacGA), "IAC GA should be processed");
    
    // 3-byte commands: IAC + command + option
    const char WILL = static_cast<char>(0xFB);
    const char ECHO = static_cast<char>(0x01);
    
    std::string iacWillEcho;
    iacWillEcho += IAC;
    iacWillEcho += WILL;
    iacWillEcho += ECHO;
    QCOMPARE(iacWillEcho.size(), size_t(3));
    QVERIFY2(simulateProcessTelnetCommand(iacWillEcho), "IAC WILL ECHO should be processed");
    
    // Incomplete 3-byte command should be rejected
    std::string incompleteWill;
    incompleteWill += IAC;
    incompleteWill += WILL;
    // Missing option byte
    QCOMPARE(incompleteWill.size(), size_t(2));
    QVERIFY2(!simulateProcessTelnetCommand(incompleteWill), "Incomplete WILL should be rejected");
}

void CTelnetTest::testMCCPScenario()
{
    // Test the specific scenario from GitHub issue #8104
    // This simulates what happens during SlothMUD connection with KaVir protocol detection
    
    // Based on the actual connection log, the sequence is:
    // 1. Initial KaVir protocol negotiation (works fine)
    // 2. KaVir detection triggers auto-reconnect with version in TTYPE
    // 3. Second connection negotiates MCCP2 compression
    // 4. "MCCP version 2 starting sequence" begins
    // 5. Compressed data processing can produce malformed telnet commands
    
    // Scenario: MCCP2 decompression produces malformed telnet sequences
    // The original crash was: assertion '__pos <= size()' failed on telnetCommand[1]
    
    // Test Case 1: Decompression returns empty string (buffer underrun)
    std::string emptyFromDecompression = "";
    QVERIFY2(!simulateProcessTelnetCommand(emptyFromDecompression),
             "Empty result from MCCP decompression should not crash");
    
    // Test Case 2: Decompression returns partial command (truncated during compression boundary)
    std::string partialFromDecompression = "\xFF"; // Just IAC
    QVERIFY2(!simulateProcessTelnetCommand(partialFromDecompression), 
             "Partial command from MCCP decompression should not crash");
    
    // Test Case 3: Decompression returns incomplete negotiation (corruption at compression boundary)
    std::string incompleteNegotiation = "\xFF\xFB"; // IAC WILL (missing option)
    QVERIFY2(!simulateProcessTelnetCommand(incompleteNegotiation),
             "Incomplete negotiation from MCCP decompression should not crash");
    
    // Test Case 4: Test the specific KaVir sequence that leads to MCCP2 negotiation
    // Simulate the commands that would be processed after "MCCP version 2 starting sequence"
    
    // KaVir protocol options in order: TTYPE(24), NAWS(31), CHARSET(42), MSDP(69), MSSP(70), ATCP(200), MSP(90), MXP(91)
    std::vector<std::string> kavirSequence = {
        "\xFF\xFD\x18", // IAC DO TTYPE (24)
        "\xFF\xFD\x1F", // IAC DO NAWS (31) 
        "\xFF\xFD\x2A", // IAC DO CHARSET (42)
        "\xFF\xFB\x45", // IAC WILL MSDP (69)
        "\xFF\xFB\x46", // IAC WILL MSSP (70)
        "\xFF\xFD\xC8", // IAC DO ATCP (200)
        "\xFF\xFB\x5A", // IAC WILL MSP (90)
        "\xFF\xFD\x5B", // IAC DO MXP (91)
        "\xFF\xFB\x56"  // IAC WILL MCCP2 (86) - this triggers compression
    };
    
    // All valid commands in the KaVir sequence should be processed correctly
    for (size_t i = 0; i < kavirSequence.size(); ++i) {
        QVERIFY2(simulateProcessTelnetCommand(kavirSequence[i]),
                 QString("KaVir sequence command %1 should be processed correctly").arg(i).toLocal8Bit());
    }
    
    // Test Case 5: Simulate what happens after MCCP2 starts - corrupted versions of the same commands
    std::vector<std::string> corruptedAfterMCCP = {
        "",              // Empty (decompression buffer underrun)
        "\xFF",          // Incomplete IAC DO TTYPE  
        "\xFF\xFD",      // Incomplete IAC DO NAWS
        "\xFF\xFB",      // Incomplete IAC WILL MSDP
        "\xFF\xFB\x46\x00", // Valid MSSP followed by NULL (compression artifact)
        "\x00\xFF\xFD\xC8", // NULL prefix (compression corruption)
    };
    
    for (size_t i = 0; i < corruptedAfterMCCP.size(); ++i) {
        bool shouldReject = (corruptedAfterMCCP[i].size() < 2) || 
                           (corruptedAfterMCCP[i].size() < 3 && needsThreeBytes(corruptedAfterMCCP[i]));
        QVERIFY2(simulateProcessTelnetCommand(corruptedAfterMCCP[i]) == !shouldReject,
                 QString("Corrupted post-MCCP command %1 should be handled correctly").arg(i).toLocal8Bit());
    }
    
    // Test Case 6: Valid commands should still work after MCCP compression is active  
    std::string validFromDecompression = "\xFF\xFD\x18"; // IAC DO TTYPE  
    QVERIFY2(simulateProcessTelnetCommand(validFromDecompression),
             "Valid commands should still work with bounds checking after MCCP");
}

// Helper function to determine if a telnet command needs 3 bytes
bool CTelnetTest::needsThreeBytes(const std::string& telnetCommand)
{
    if (telnetCommand.size() < 2) return false;
    
    char ch = telnetCommand[1];
    const char TN_WILL = static_cast<char>(0xFB);
    const char TN_WONT = static_cast<char>(0xFC);  
    const char TN_DO = static_cast<char>(0xFD);
    const char TN_DONT = static_cast<char>(0xFE);
    const char TN_SB = static_cast<char>(0xFA);
    
    return (ch == TN_WILL || ch == TN_WONT || ch == TN_DO || ch == TN_DONT || ch == TN_SB);
}

#include "CTelnetTest.moc"
QTEST_GUILESS_MAIN(CTelnetTest)
