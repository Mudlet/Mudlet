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

#include "TMxpProcessor.h"
#include "TMxpStubClient.h"
#include "TMxpTagParser.h"
#include <QTest>
#include <string>

/**
 * Test the MXP mode security and tag validation implementation
 *
 * According to the MXP spec (https://www.zuggsoft.com/zmud/mxp.htm):
 * - OPEN mode (default after negotiation): Only OPEN-category tags allowed
 * - SECURE mode: All MXP tags allowed
 * - LOCKED mode: No tags at all (verbatim text)
 *
 * Tags that don't match known MXP spec tags or user-defined elements
 * are treated as literal text, preventing false positives from characters
 * like < in normal game text.
 */
class TMxpModeSecurityTest : public QObject {
  Q_OBJECT

private:
  /**
   * Helper: feed a string through the MXP processor character by character
   * and collect all entity values that are output (rejected/literal text).
   */
  static QString processAndCollectOutput(TMxpProcessor &processor,
                                         const std::string &input) {
    QString output;
    for (char ch : input) {
      TMxpProcessingResult result = processor.processMxpInput(ch, true);
      if (result == HANDLER_INSERT_ENTITY_SYS) {
        output += processor.getEntityValue();
      }
    }
    return output;
  }

private slots:

  // ---------------------------------------------------------------
  // Mode defaults and transitions
  // ---------------------------------------------------------------

  void testDefaultModeIsOpen() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    // MXP spec: "OPEN MODE starts as the Default mode"
    QCOMPARE(processor.mode(), MXP_MODE_OPEN);
  }

  void testModeTransitions() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    processor.setMode(MXP_MODE_CODE_SECURE);
    QCOMPARE(processor.mode(), MXP_MODE_SECURE);

    processor.setMode(MXP_MODE_CODE_OPEN);
    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    processor.setMode(MXP_MODE_CODE_LOCKED);
    QCOMPARE(processor.mode(), MXP_MODE_LOCKED);
  }

  // ---------------------------------------------------------------
  // Tag recognition (static sets from the MXP spec)
  // ---------------------------------------------------------------

  void testOpenModeTagsRecognized() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    // All formatting tags should be recognized
    QVERIFY(processor.isRecognizedMxpTag(qsl("B")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("BOLD")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("I")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("COLOR")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("FONT")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("BR")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("HR")));

    // Case insensitive
    QVERIFY(processor.isRecognizedMxpTag(qsl("b")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("Color")));
  }

  void testSecureModeTagsRecognized() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    QVERIFY(processor.isRecognizedMxpTag(qsl("SEND")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("A")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("VERSION")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("SUPPORT")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("SOUND")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("IMAGE")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("FRAME")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("DEST")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("!ELEMENT")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("!ENTITY")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("VAR")));
  }

  void testUnknownTagsNotRecognized() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    QVERIFY(!processor.isRecognizedMxpTag(qsl("test")));
    QVERIFY(!processor.isRecognizedMxpTag(qsl("villains")));
    QVERIFY(!processor.isRecognizedMxpTag(qsl("notmxp")));
    QVERIFY(!processor.isRecognizedMxpTag(qsl("echo")));
  }

  // ---------------------------------------------------------------
  // OPEN mode: only OPEN tags allowed
  // ---------------------------------------------------------------

  void testSendTagBlockedInOpenMode() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // SEND is a SECURE tag - should be rejected in OPEN mode and
    // output as literal text
    QString output =
        processAndCollectOutput(processor, "<SEND href=\"cmd\">click</SEND>");

    // The SEND tag should have been output as literal text
    QVERIFY(output.contains(qsl("<SEND")));

    // No link should have been created
    QCOMPARE(client.mHrefs.size(), 0);
  }

  void testFormattingTagsAllowedInOpenMode() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // Process formatting tags - these should all work in OPEN mode
    processAndCollectOutput(processor, "<B>bold</B>");

    // Bold should have been activated (not rejected)
    // The stub client tracks bold state
    // After </B>, boldCounter should be back to 0
    QCOMPARE(client.boldCounter, static_cast<unsigned int>(0));
  }

  void testColorTagAllowedInOpenMode() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // COLOR is an OPEN tag - process only the opening tag so that
    // pushColor is called but popColor hasn't happened yet
    processAndCollectOutput(processor, "<COLOR red>text");

    // Should have set the color (not rejected)
    QVERIFY(!client.fgColor.isEmpty());
  }

  // ---------------------------------------------------------------
  // SECURE mode: all MXP tags allowed
  // ---------------------------------------------------------------

  void testSendTagAllowedInSecureMode() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    processor.setMode(MXP_MODE_CODE_SECURE);
    QCOMPARE(processor.mode(), MXP_MODE_SECURE);

    processAndCollectOutput(processor,
                            "<SEND href=\"test command\">click me</SEND>");

    // A link should have been created
    QVERIFY(client.mHrefs.size() > 0);
  }

  void testElementDefinitionAllowedInSecureMode() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    processor.setMode(MXP_MODE_CODE_SECURE);
    QCOMPARE(processor.mode(), MXP_MODE_SECURE);

    // !ELEMENT is a SECURE tag
    processAndCollectOutput(
        processor, "<!ELEMENT RName '<FONT COLOR=Red><B>' FLAG=\"RoomName\">");

    // Should have registered the element
    QVERIFY(processor.getMxpTagProcessor().getElementRegistry().containsElement(
        qsl("RName")));
  }

  // ---------------------------------------------------------------
  // LOCKED mode: no tags at all
  // ---------------------------------------------------------------

  void testNoTagsAllowedInLockedMode() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    processor.setMode(MXP_MODE_CODE_LOCKED);
    QCOMPARE(processor.mode(), MXP_MODE_LOCKED);

    // In LOCKED mode, text passes through without parsing
    std::string input = "<B>bold</B>";

    for (char ch : input) {
      TMxpProcessingResult result = processor.processMxpInput(ch, true);
      // All characters should fall through (no tag parsing)
      QCOMPARE(result, HANDLER_FALL_THROUGH);
    }
  }

  // ---------------------------------------------------------------
  // False positive prevention: non-MXP text with < characters
  // ---------------------------------------------------------------

  void testLessThanInTextOutputAsLiteral() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // "< villains>" - the '<' starts tag parsing, ' villains'
    // accumulates, '>' closes it. "villains" is not a known OPEN tag.
    QString output = processAndCollectOutput(processor, "< villains>");

    // Should be output as literal text since "villains" is not an MXP tag
    QVERIFY(output.contains(qsl("villains")));
  }

  void testUnknownTagNameRejectedImmediately() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // <xyz is not a known MXP tag - should be rejected early via prefix check
    // 'x' doesn't start any OPEN mode tag
    QString output = processAndCollectOutput(processor, "<xyz something>");

    // Should show the original text as literal
    QVERIFY(output.contains(qsl("<xyz")));
  }

  void testSecureTagInTextRejectedInOpenMode() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // Even though IMAGE is a real MXP tag, it's SECURE - not allowed in OPEN
    // mode
    QString output = processAndCollectOutput(processor, "<IMAGE map.jpg>");

    // Should be output as literal text
    QVERIFY(output.contains(qsl("<IMAGE")));
  }

  // ---------------------------------------------------------------
  // User-defined elements
  // ---------------------------------------------------------------

  void testUserDefinedOpenElementAllowedInOpenMode() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    // First, register a user-defined OPEN element (normally done in SECURE
    // mode)
    processor.setMode(MXP_MODE_CODE_SECURE);
    processAndCollectOutput(processor,
                            "<!ELEMENT Auction '<FONT COLOR=red>' OPEN>");

    // Switch back to OPEN mode
    processor.setMode(MXP_MODE_CODE_OPEN);
    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // The OPEN user-defined element should be recognized
    QVERIFY(processor.isTagAllowedInCurrentMode(qsl("Auction")));
  }

  void testUserDefinedSecureElementBlockedInOpenMode() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    // Register a SECURE user-defined element (no OPEN keyword)
    processor.setMode(MXP_MODE_CODE_SECURE);
    processAndCollectOutput(processor,
                            "<!ELEMENT ImmChan '<FONT COLOR=Red,Blink>'>");

    // Switch back to OPEN mode
    processor.setMode(MXP_MODE_CODE_OPEN);
    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // The SECURE user-defined element should NOT be allowed in OPEN mode
    QVERIFY(!processor.isTagAllowedInCurrentMode(qsl("ImmChan")));
  }

  // ---------------------------------------------------------------
  // Prefix validation (early rejection during tag name accumulation)
  // ---------------------------------------------------------------

  void testValidPrefixNotRejected() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // "B" is a valid OPEN tag prefix
    QVERIFY(processor.couldBeValidMxpTag(qsl("B")));

    // "CO" is a prefix of "COLOR" (OPEN tag)
    QVERIFY(processor.couldBeValidMxpTag(qsl("CO")));

    // "FO" is a prefix of "FONT" (OPEN tag)
    QVERIFY(processor.couldBeValidMxpTag(qsl("FO")));
  }

  void testInvalidPrefixRejected() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // "X" doesn't start any OPEN mode tag
    QVERIFY(!processor.couldBeValidMxpTag(qsl("X")));

    // "VIL" doesn't start any OPEN mode tag
    QVERIFY(!processor.couldBeValidMxpTag(qsl("VIL")));

    // "S" starts "S", "SMALL", "SBR", "STRIKEOUT" in OPEN mode
    QVERIFY(processor.couldBeValidMxpTag(qsl("S")));
    // But "SE" doesn't start any OPEN mode tag
    QVERIFY(!processor.couldBeValidMxpTag(qsl("SE")));
  }

  void testPrefixValidationInSecureMode() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    processor.setMode(MXP_MODE_CODE_SECURE);
    QCOMPARE(processor.mode(), MXP_MODE_SECURE);

    // "SE" starts "SEND" in SECURE mode
    QVERIFY(processor.couldBeValidMxpTag(qsl("SE")));

    // "IM" starts "IMAGE" in SECURE mode
    QVERIFY(processor.couldBeValidMxpTag(qsl("IM")));

    // "ZZ" doesn't start any tag
    QVERIFY(!processor.couldBeValidMxpTag(qsl("ZZ")));
  }

  // ---------------------------------------------------------------
  // Integration: MXP spec example should work in SECURE mode
  // ---------------------------------------------------------------

  void testMxpSpecExampleInSecureMode() {
    TMxpStubClient client;
    TMxpProcessor processor(&client);

    processor.setMode(MXP_MODE_CODE_LOCK_SECURE);
    QCOMPARE(processor.mode(), MXP_MODE_SECURE);

    // From the MXP spec example
    processAndCollectOutput(
        processor, "<!ELEMENT RName '<FONT COLOR=Red><B>' FLAG=\"RoomName\">");
    processAndCollectOutput(processor, "<!ELEMENT Ex '<SEND>'>");

    QVERIFY(processor.getMxpTagProcessor().getElementRegistry().containsElement(
        qsl("RName")));
    QVERIFY(processor.getMxpTagProcessor().getElementRegistry().containsElement(
        qsl("Ex")));

    // The user-defined elements should be recognized
    QVERIFY(processor.isRecognizedMxpTag(qsl("RName")));
    QVERIFY(processor.isRecognizedMxpTag(qsl("Ex")));
  }
};

QTEST_MAIN(TMxpModeSecurityTest)
#include "TMxpModeSecurityTest.moc"
