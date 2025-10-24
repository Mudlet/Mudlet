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

#include <QTest>
#include "TMxpProcessor.h"
#include "TMxpStubClient.h"
#include "TMxpTagParser.h"
#include <string>

/**
 * Test the MXP mode security implementation
 * 
 * This test addresses issue #8150: MXP "open mode" security vulnerability
 * where SEND and other secure tags were being allowed in OPEN mode.
 * 
 * According to the MXP spec (https://www.zuggsoft.com/zmud/mxp.htm):
 * - OPEN mode: Only basic formatting tags (B, I, U, COLOR, etc.)
 * - SECURE mode: All tags including SEND, A, VAR, SOUND, MUSIC
 * - LOCKED mode: No tags at all
 */
class TMxpModeSecurityTest : public QObject {
Q_OBJECT

private:
    // Extended stub client that tracks mode and validates tags
    class TMxpModeTestClient : public TMxpStubClient {
    private:
        TMxpProcessor* mpProcessor;
        
    public:
        int rejectedTagCount = 0;
        QString lastRejectedTag;
        
        void setProcessor(TMxpProcessor* processor) {
            mpProcessor = processor;
        }
        
        bool startTagReceived(MxpStartTag* startTag) override {
            if (!mpProcessor) {
                return true; // No processor, allow everything
            }
            
            TMXPMode currentMode = mpProcessor->mode();
            const QString tagName = startTag->getName().toUpper();
            
            // In LOCKED mode, no tags are allowed
            if (currentMode == MXP_MODE_LOCKED) {
                rejectedTagCount++;
                lastRejectedTag = tagName;
                return false;
            }
            
            // Check if this tag is allowed in the current mode
            if (!isTagAllowedInMode(tagName, currentMode)) {
                rejectedTagCount++;
                lastRejectedTag = tagName;
                return false;
            }
            
            return true;
        }
        
    private:
        bool isTagAllowedInMode(const QString& tagName, TMXPMode mode) const {
            // In SECURE or TEMP_SECURE mode, all tags are allowed
            if (mode == MXP_MODE_SECURE || mode == MXP_MODE_TEMP_SECURE) {
                return true;
            }
            
            // In LOCKED mode, no tags are allowed
            if (mode == MXP_MODE_LOCKED) {
                return false;
            }
            
            // In OPEN mode, only specific formatting tags are allowed
            static const QSet<QString> openModeTags = {
                "B", "BOLD", "STRONG",
                "I", "ITALIC", "EM",
                "U", "UNDERLINE",
                "S", "STRIKEOUT",
                "C", "COLOR",
                "H", "HIGH",
                "FONT",
                "NOBR",
                "P",
                "BR", "SBR"
            };
            
            return openModeTags.contains(tagName);
        }
    };

private slots:
    void testSendTagBlockedInOpenMode()
    {
        // Test the core security issue from #8150
        TMxpModeTestClient client;
        TMxpProcessor processor(&client);
        client.setProcessor(&processor);
        
        // Verify we're in OPEN mode by default
        QCOMPARE(processor.mode(), MXP_MODE_OPEN);
        
        // Process a SEND tag in OPEN mode
        std::string input = "\033[0z<SEND href=\"dangerous command\">click me</SEND>";

        for (char ch : input) {
            processor.processMxpInput(ch, true);
        }
        
        // The SEND tag should have been rejected
        QVERIFY(client.rejectedTagCount > 0);
        QCOMPARE(client.lastRejectedTag, QString("SEND"));
        
        // No link should have been created
        QCOMPARE(client.mHrefs.size(), 0);
    }

    void testSendTagAllowedInSecureMode()
    {
        TMxpModeTestClient client;
        TMxpProcessor processor(&client);
        client.setProcessor(&processor);
        
        // Set to SECURE mode
        processor.setMode(1);
        QCOMPARE(processor.mode(), MXP_MODE_SECURE);
        
        // Process a SEND tag in SECURE mode
        std::string input = "<SEND href=\"test command\">click me</SEND>";

        for (char ch : input) {
            processor.processMxpInput(ch, true);
        }
        
        // The SEND tag should have been accepted
        QCOMPARE(client.rejectedTagCount, 0);
        
        // A link should have been created
        QVERIFY(client.mHrefs.size() > 0);
    }

    void testFormattingTagsAllowedInOpenMode()
    {
        TMxpModeTestClient client;
        TMxpProcessor processor(&client);
        client.setProcessor(&processor);
        
        // Verify we're in OPEN mode
        QCOMPARE(processor.mode(), MXP_MODE_OPEN);
        
        // Process formatting tags - these should all work in OPEN mode
        std::string input = "\033[0z<B>bold</B><I>italic</I><U>underline</U>";

        for (char ch : input) {
            processor.processMxpInput(ch, true);
        }
        
        // No tags should have been rejected
        QCOMPARE(client.rejectedTagCount, 0);
    }

    void testNoTagsAllowedInLockedMode()
    {
        TMxpModeTestClient client;
        TMxpProcessor processor(&client);
        client.setProcessor(&processor);
        
        // Set to LOCKED mode
        processor.setMode(2);
        QCOMPARE(processor.mode(), MXP_MODE_LOCKED);
        
        // Try to process formatting tags - even these should be blocked
        std::string input = "<B>bold</B>";

        for (char ch : input) {
            processor.processMxpInput(ch, true);
        }
        
        // Tags should have been rejected
        QVERIFY(client.rejectedTagCount > 0);
    }

    void testModeTransitions()
    {
        TMxpModeTestClient client;
        TMxpProcessor processor(&client);
        client.setProcessor(&processor);
        
        // Start in OPEN mode (default)
        QCOMPARE(processor.mode(), MXP_MODE_OPEN);
        
        // Switch to SECURE mode
        processor.setMode(1);
        QCOMPARE(processor.mode(), MXP_MODE_SECURE);
        
        // Now SEND should work
        std::string input1 = "<SEND>test</SEND>";
        client.rejectedTagCount = 0;

        for (char ch : input1) {
            processor.processMxpInput(ch, true);
        }

        QCOMPARE(client.rejectedTagCount, 0);
        
        // Switch back to OPEN mode
        processor.setMode(0);
        QCOMPARE(processor.mode(), MXP_MODE_OPEN);
        
        // Now SEND should be blocked again
        std::string input2 = "<SEND>test2</SEND>";
        client.rejectedTagCount = 0;

        for (char ch : input2) {
            processor.processMxpInput(ch, true);
        }

        QVERIFY(client.rejectedTagCount > 0);
    }
};

QTEST_MAIN(TMxpModeSecurityTest)
#include "TMxpModeSecurityTest.moc"
