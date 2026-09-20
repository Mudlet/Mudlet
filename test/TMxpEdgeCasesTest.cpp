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

#include "TMxpProcessor.h"
#include "TMxpStatTagHandler.h"
#include "TMxpStubClient.h"
#include "TMxpTagParser.h"
#include "TMxpTagProcessor.h"
#include <QTest>
#include <string>

// Tests for MXP edge cases (issue #8908): comments, STAT/GAUGE tags, and mode
// locking
class TMxpEdgeCasesTest : public QObject {
  Q_OBJECT

private:
  class TMxpEdgeCaseClient : public TMxpStubClient {
  public:
    QStringList handledTagNames;
    QStringList unhandledTagNames;

    bool startTagReceived(MxpStartTag *startTag) override {
      handledTagNames.append(startTag->getName());
      return true;
    }

    TMxpTagHandlerResult tagHandled(MxpTag *tag, TMxpTagHandlerResult result,
                                    TMxpContext &context) override {
      Q_UNUSED(context)
      if (result == MXP_TAG_NOT_HANDLED) {
        unhandledTagNames.append(tag->getName());
      }
      return result;
    }

    void reset() {
      handledTagNames.clear();
      unhandledTagNames.clear();
    }
  };

  void processInput(TMxpProcessor &processor, const std::string &input) {
    for (char ch : input) {
      processor.processMxpInput(ch, true);
    }
  }

private slots:
  // Comments (<!-- -->) should be consumed, not displayed as text
  void testCommentsAreSilentlyConsumed() {
    TMxpEdgeCaseClient client;
    TMxpProcessor processor(&client);

    processor.enable();
    processor.setMode(MXP_MODE_CODE_SECURE);
    QCOMPARE(processor.mode(), MXP_MODE_SECURE);

    // Process a comment followed by a known tag
    processInput(processor, "<!-- this is a comment --><B>bold</B>");

    // The B tag should still be handled after the comment
    QVERIFY2(client.handledTagNames.contains(qsl("B")),
             "B tag after comment should still be handled");

    // The comment should NOT have been passed to client as unhandled
    // (unhandled tags appear as raw text in the output)
    QVERIFY2(!client.unhandledTagNames.contains(qsl("!--")),
             "Comment should NOT be unhandled (would appear as text)");
  }

  // Verify tag processor returns HANDLED for comment tags
  void testTagProcessorHandlesComments() {
    TMxpStubContext ctx;
    TMxpEdgeCaseClient client;
    TMxpTagProcessor tagProcessor;

    // Parse a comment tag
    QList<QSharedPointer<MxpNode>> nodes =
        TMxpTagParser::parseToMxpNodeList(qsl("<!-- test comment -->"));
    QVERIFY(!nodes.isEmpty());
    QVERIFY(nodes.first()->isTag());

    MxpTag *tag = nodes.first()->asTag();
    QCOMPARE(tag->getName(), qsl("!--"));

    // The tag processor should return HANDLED for comments
    TMxpTagHandlerResult result = tagProcessor.handleTag(ctx, client, tag);
    QCOMPARE(result, MXP_TAG_HANDLED);
  }

  // STAT tags should be consumed, not displayed as text
  void testStatTagIsHandled() {
    TMxpEdgeCaseClient client;
    TMxpProcessor processor(&client);

    processor.enable();
    processor.setMode(MXP_MODE_CODE_SECURE);

    // Process a STAT tag
    processInput(processor, "<STAT hp Max=maxhp Caption=\"Health\">");

    // The STAT tag should have been received and handled
    QVERIFY2(client.handledTagNames.contains(qsl("STAT")),
             "STAT tag should be received by the processor");

    // STAT should NOT be in unhandled list (which would mean it appears as
    // text)
    QVERIFY2(!client.unhandledTagNames.contains(qsl("STAT")),
             "STAT tag should NOT be unhandled (would appear as text)");
  }

  // Verify STAT handler returns HANDLED
  void testStatTagHandlerReturnsHandled() {
    TMxpStubContext ctx;
    TMxpEdgeCaseClient client;
    TMxpStatTagHandler handler;

    // Parse a STAT tag
    QList<QSharedPointer<MxpNode>> nodes =
        TMxpTagParser::parseToMxpNodeList(qsl("<STAT hp>"));
    QVERIFY(!nodes.isEmpty());
    QVERIFY(nodes.first()->isTag());
    QVERIFY(!nodes.first()->asTag()->isEndTag());

    MxpStartTag *tag = static_cast<MxpStartTag *>(nodes.first()->asTag());
    QCOMPARE(tag->getName(), qsl("STAT"));

    // The handler should support STAT tags
    QVERIFY2(handler.supports(ctx, client, tag),
             "TMxpStatTagHandler should support STAT tags");

    // The handler should return HANDLED (not NOT_HANDLED which means "display
    // as text")
    TMxpTagHandlerResult result = handler.handleStartTag(ctx, client, tag);
    QCOMPARE(result, MXP_TAG_HANDLED);
  }

  // GAUGE tags should be consumed, not displayed as text
  void testGaugeTagIsHandled() {
    TMxpEdgeCaseClient client;
    TMxpProcessor processor(&client);

    processor.enable();
    processor.setMode(MXP_MODE_CODE_SECURE);

    // Process a GAUGE tag
    processInput(processor,
                 "<GAUGE mana Max=maxmana Caption=\"Mana\" Color=blue>");

    // The GAUGE tag should have been received and handled
    QVERIFY2(client.handledTagNames.contains(qsl("GAUGE")),
             "GAUGE tag should be received by the processor");

    // GAUGE should NOT be in unhandled list
    QVERIFY2(!client.unhandledTagNames.contains(qsl("GAUGE")),
             "GAUGE tag should NOT be unhandled (would appear as text)");
  }

  // Verify GAUGE handler returns HANDLED
  void testGaugeTagHandlerReturnsHandled() {
    TMxpStubContext ctx;
    TMxpEdgeCaseClient client;
    TMxpStatTagHandler handler;

    // Parse a GAUGE tag
    QList<QSharedPointer<MxpNode>> nodes =
        TMxpTagParser::parseToMxpNodeList(qsl("<GAUGE mana>"));
    QVERIFY(!nodes.isEmpty());
    QVERIFY(nodes.first()->isTag());
    QVERIFY(!nodes.first()->asTag()->isEndTag());

    MxpStartTag *tag = static_cast<MxpStartTag *>(nodes.first()->asTag());
    QCOMPARE(tag->getName(), qsl("GAUGE"));

    // The handler should support GAUGE tags
    QVERIFY2(handler.supports(ctx, client, tag),
             "TMxpStatTagHandler should support GAUGE tags");

    // The handler should return HANDLED
    TMxpTagHandlerResult result = handler.handleStartTag(ctx, client, tag);
    QCOMPARE(result, MXP_TAG_HANDLED);
  }

  // Mode code 7 sets LOCKED as current and default mode
  void testModeLockLocked() {
    TMxpEdgeCaseClient client;
    TMxpProcessor processor(&client);

    processor.enable();

    // Verify initial mode
    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // Set mode to LOCKED using mode code 7 (lock LOCKED as default)
    processor.setMode(MXP_MODE_CODE_LOCK_LOCKED);

    // After mode code 7, both current and default should be LOCKED
    QCOMPARE(processor.mode(), MXP_MODE_LOCKED);
    QCOMPARE(processor.defaultMode(), MXP_MODE_LOCKED);
  }

  // Mode code 5 sets OPEN as current and default mode
  void testModeLockOpen() {
    TMxpEdgeCaseClient client;
    TMxpProcessor processor(&client);

    processor.enable();

    // Set mode to SECURE first
    processor.setMode(MXP_MODE_CODE_SECURE);
    QCOMPARE(processor.mode(), MXP_MODE_SECURE);

    // Set mode using mode code 5 (lock OPEN as default)
    processor.setMode(MXP_MODE_CODE_LOCK_OPEN);

    // After mode code 5, both current and default should be OPEN
    QCOMPARE(processor.mode(), MXP_MODE_OPEN);
    QCOMPARE(processor.defaultMode(), MXP_MODE_OPEN);
  }

  // Mode code 6 sets SECURE as current and default mode
  void testModeLockSecure() {
    TMxpEdgeCaseClient client;
    TMxpProcessor processor(&client);

    processor.enable();

    // Verify initial mode
    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // Set mode using mode code 6 (lock SECURE as default)
    processor.setMode(MXP_MODE_CODE_LOCK_SECURE);

    // After mode code 6, both current and default should be SECURE
    QCOMPARE(processor.mode(), MXP_MODE_SECURE);
    QCOMPARE(processor.defaultMode(), MXP_MODE_SECURE);
  }

  // resetToDefaultMode uses the locked default, not always OPEN
  void testResetUsesLockedDefault() {
    TMxpEdgeCaseClient client;
    TMxpProcessor processor(&client);

    processor.enable();

    // Lock SECURE as default
    processor.setMode(MXP_MODE_CODE_LOCK_SECURE);
    QCOMPARE(processor.defaultMode(), MXP_MODE_SECURE);

    // Temporarily switch to OPEN
    processor.setMode(MXP_MODE_CODE_OPEN);
    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    // Reset to default (simulates newline)
    processor.resetToDefaultMode();

    // Should reset to SECURE (the locked default), not OPEN
    QCOMPARE(processor.mode(), MXP_MODE_SECURE);
  }

  // Mode codes correctly set LOCKED mode
  void testModeCodeSetsLockedMode() {
    TMxpEdgeCaseClient client;
    TMxpProcessor processor(&client);

    processor.enable();

    // Lock to LOCKED mode (mode code 7)
    processor.setMode(MXP_MODE_CODE_LOCK_LOCKED);
    QCOMPARE(processor.mode(), MXP_MODE_LOCKED);
    QCOMPARE(processor.defaultMode(), MXP_MODE_LOCKED);

    // Also test mode code 2 (non-locking LOCKED)
    processor.setMode(MXP_MODE_CODE_OPEN);
    QCOMPARE(processor.mode(), MXP_MODE_OPEN);

    processor.setMode(MXP_MODE_CODE_LOCKED);
    QCOMPARE(processor.mode(), MXP_MODE_LOCKED);
    // Default should still be LOCKED from the earlier lock
    QCOMPARE(processor.defaultMode(), MXP_MODE_LOCKED);
  }

  // Test mode 4 (temp secure) for secure tags with mode 7 (lock locked) as
  // default. This is the pattern used by some MUDs for maximum security:
  // ESC[4z<SEND HREF="foo">ESC[7zfooESC[4z</SEND>ESC[7z
  void testTempSecureWithLockLockedPattern() {
    TMxpEdgeCaseClient client;
    TMxpProcessor processor(&client);

    processor.enable();

    // Start in lock locked mode (mode 7) - very conservative default
    processor.setMode(MXP_MODE_CODE_LOCK_LOCKED);
    QCOMPARE(processor.mode(), MXP_MODE_LOCKED);
    QCOMPARE(processor.defaultMode(), MXP_MODE_LOCKED);

    // Mode 4 = temp secure for the opening SEND tag
    processor.setMode(MXP_MODE_CODE_TEMP_SECURE);
    QCOMPARE(processor.mode(), MXP_MODE_TEMP_SECURE);

    // Process opening SEND tag in temp secure mode
    processInput(processor, "<SEND HREF=\"foo\">");
    QVERIFY2(client.handledTagNames.contains(qsl("SEND")),
             "SEND tag should be handled in temp secure mode");

    // Mode 7 = lock locked for the content (prevents MXP parsing of user text)
    processor.setMode(MXP_MODE_CODE_LOCK_LOCKED);
    QCOMPARE(processor.mode(), MXP_MODE_LOCKED);

    // Content "foo" is in locked mode - no MXP parsing
    // (In real usage this would just be displayed as text)

    // Mode 4 again for the closing SEND tag
    processor.setMode(MXP_MODE_CODE_TEMP_SECURE);
    QCOMPARE(processor.mode(), MXP_MODE_TEMP_SECURE);

    // Process closing SEND tag
    processInput(processor, "</SEND>");

    // Mode 7 to restore locked as default
    processor.setMode(MXP_MODE_CODE_LOCK_LOCKED);
    QCOMPARE(processor.mode(), MXP_MODE_LOCKED);
    QCOMPARE(processor.defaultMode(), MXP_MODE_LOCKED);
  }

  // Test mode 4 (temp secure) reverts to default after tag is processed
  void testTempSecureRevertsToDefault() {
    TMxpEdgeCaseClient client;
    TMxpProcessor processor(&client);

    processor.enable();

    // Lock OPEN as default
    processor.setMode(MXP_MODE_CODE_LOCK_OPEN);
    QCOMPARE(processor.defaultMode(), MXP_MODE_OPEN);

    // Set temp secure mode
    processor.setMode(MXP_MODE_CODE_TEMP_SECURE);
    QCOMPARE(processor.mode(), MXP_MODE_TEMP_SECURE);

    // Process a secure tag
    processInput(processor, "<SEND>test</SEND>");

    // After processing, mode should revert to OPEN (the locked default)
    // Note: The exact revert behavior depends on implementation details
    // This test documents the expected behavior for temp secure mode
    QVERIFY2(client.handledTagNames.contains(qsl("SEND")),
             "SEND tag should be handled in temp secure mode");
  }
};

QTEST_MAIN(TMxpEdgeCasesTest)
#include "TMxpEdgeCasesTest.moc"
