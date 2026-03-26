/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

/*
 * Tests for OSC (Operating System Command) escape sequence handling.
 *
 * Run with: ctest -R TOscTest -V
 */

#include <QtTest/QtTest>

#include "THyperlinkStyling.h"
#include "TLinkStore.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForOscTest();

class TOscTest : public QObject {
  Q_OBJECT

private:
  TelnetServerStub *mpServer = nullptr;
  const QString mHostname = "OSC-Test-Host";
  const QString mPort = "4002";
  const QString mLocalhost = "localhost";

  // Scans backward through the buffer to find the first hyperlink and returns
  // its command list from the link store.
  QStringList findFirstLinkCommands() {
    auto *host = mudlet::self()->getActiveHost();
    if (!host || !host->mpConsole) {
      return {};
    }
    TMainConsole *console = host->mpConsole;
    int linkId = 0;
    for (int line = console->buffer.getLastLineNumber();
         line >= 0 && linkId == 0; --line) {
      linkId = console->buffer.getLinkIndexAt(line, 0);
    }
    if (linkId <= 0) {
      return {};
    }
    return console->getLinkStore().getLinksConst(linkId);
  }

  // Scans backward through the buffer to find the first hyperlink and returns
  // its HyperlinkStyling from the link store.
  Mudlet::HyperlinkStyling findFirstLinkStyling() {
    auto *host = mudlet::self()->getActiveHost();
    if (!host || !host->mpConsole) {
      return {};
    }
    TMainConsole *console = host->mpConsole;
    int linkId = 0;
    for (int line = console->buffer.getLastLineNumber();
         line >= 0 && linkId == 0; --line) {
      linkId = console->buffer.getLinkIndexAt(line, 0);
    }
    if (linkId <= 0) {
      return {};
    }
    return console->getLinkStore().getStyling(linkId);
  }

  // Builds an OSC 8 hyperlink message with optional JSON config in the URL.
  // The config is appended as a ?config= query parameter.
  QString buildOsc8WithConfig(const QString &baseUrl, const QString &linkText,
                              const QString &jsonConfig) {
    QString url = baseUrl;
    if (!jsonConfig.isEmpty()) {
      url += qsl("?config=") + jsonConfig;
    }
    return qsl("\x1b]8;;") + url + qsl("\x1b\\") + linkText +
           qsl("\x1b]8;;\x1b\\");
  }

private slots:
  void initTestCase() { initializeQRCResourcesForOscTest(); }

  void init() {
    mpServer = new TelnetServerStub(qApp);
    mpServer->start(mLocalhost, mPort.toUShort());
    mudlet::start();
    mudlet::self()->setupConfig();
    mudlet::self()->takeOwnershipOfInstanceCoordinator(
        std::make_unique<MudletInstanceCoordinator>(
            "MudletInstanceCoordinator"));
    mudlet::self()->init();
    mudlet::self()->setStorePasswordsSecurely(false);
    deleteProfileDirectory(mHostname);
  }

  // Test that BEL-terminated OSC 2 (window title) doesn't swallow subsequent
  // text
  void test_BelTerminatedOsc2_DoesNotSwallowText() {
    // OSC 2 (set window title) with BEL terminator, followed by regular text
    // ESC ] 2 ; Window Title BEL Hello World
    QString messageFromMud = QString("\x1b]2;Window Title\x07Hello World");
    QString expectedText = "Hello World";

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QString actualText =
        mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
    QCOMPARE(actualText, expectedText);
  }

  // Test that ST-terminated OSC sequences still work (regression test)
  void test_StTerminatedOsc_StillWorks() {
    // OSC with ST terminator (ESC \), followed by regular text
    // ESC ] P 0 F F 0 0 0 0 ESC \ Hello
    // Note: OSC P redefines colors, but we're just checking text after isn't
    // swallowed
    QString messageFromMud = QString("\x1b]P0FF0000\x1b\\Hello");
    QString expectedText = "Hello";

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QString actualText =
        mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
    QCOMPARE(actualText, expectedText);
  }

  // Test that text after BEL-terminated OSC 8 (hyperlink) is displayed
  void test_BelTerminatedOsc8_TextDisplayed() {
    // OSC 8 hyperlink with BEL, some text, close OSC 8, more text
    // ESC ] 8 ; ; http://example.com BEL Link Text ESC ] 8 ; ; BEL After Link
    QString messageFromMud = QString(
        "\x1b]8;;http://example.com\x07Link Text\x1b]8;;\x07 After Link");
    // The hyperlink text and subsequent text should both appear
    QString expectedContains = "After Link";

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QString actualText =
        mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
    QVERIFY2(actualText.contains(expectedContains),
             qPrintable(QString("Expected text to contain '%1' but got '%2'")
                            .arg(expectedContains, actualText)));
  }

  // Test that BEL-terminated OSC P (color redefinition) works
  void test_BelTerminatedOscP_TextDisplayed() {
    // OSC P redefines color palette, with BEL terminator, followed by text
    // ESC ] P 0 F F 0 0 0 0 BEL Hello
    QString messageFromMud = QString("\x1b]P0FF0000\x07Hello");
    QString expectedText = "Hello";

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QString actualText =
        mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
    QCOMPARE(actualText, expectedText);
  }

  // Test that empty OSC sequence doesn't crash (edge case)
  void test_EmptyOscSequence_DoesNotCrash() {
    // OSC with immediate BEL terminator (empty content)
    // ESC ] BEL Normal text
    QString messageFromMud = QString("\x1b]\x07Normal text");
    QString expectedText = "Normal text";

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QString actualText =
        mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
    QCOMPARE(actualText, expectedText);
  }

  // Test that OSC sequence exceeding length limit doesn't hang and recovers
  // gracefully
  void test_OscExceedsLengthLimit_DoesNotHang() {
    // Create an OSC sequence longer than 4096 bytes, followed by a BEL and
    // normal text
    QString longContent = QString(5000, 'A');
    // ESC ] 2 ; <5000 A's> BEL Normal text
    QString messageFromMud =
        QString("\x1b]2;") + longContent + QString("\x07Normal text");
    QString expectedText = "Normal text";

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QString actualText =
        mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
    // When length limit is exceeded, the parser scans forward for a terminator
    // and skips the malformed sequence. Text after the terminator should
    // display normally.
    QCOMPARE(actualText, expectedText);
  }

  // ═══════════════════════════════════════════════════════════════════
  // OSC 8 Hyperlink URL Parameter Tests
  // ═══════════════════════════════════════════════════════════════════

  void test_Osc8WebUrl_PreservesQueryParameters() {
    QString url = "https://example.com/?id=42&lang=en";
    QString messageFromMud =
        QString("\x1b]8;;%1\x1b\\Link\x1b]8;;\x1b\\").arg(url);

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QStringList commands = findFirstLinkCommands();
    QVERIFY2(!commands.isEmpty(), "No hyperlink found in buffer");
    QVERIFY2(
        commands.first().contains("id=42&lang=en"),
        qPrintable(
            QString("Query params missing from: %1").arg(commands.first())));
  }

  void test_Osc8WebUrl_StripsConfigParameter() {
    QString messageFromMud = "\x1b]8;;https://example.com/"
                             "?config=%7B%22style%22%3A%7B%22color%22%3A%22red%"
                             "22%7D%7D\x1b\\Styled\x1b]8;;\x1b\\";

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QStringList commands = findFirstLinkCommands();
    QVERIFY2(!commands.isEmpty(), "No hyperlink found in buffer");
    QVERIFY2(!commands.first().contains("config="),
             qPrintable(QString("Reserved 'config' param not stripped: %1")
                            .arg(commands.first())));
  }

  void test_Osc8WebUrl_StripsPresetParameter() {
    QString messageFromMud = "\x1b]8;;https://example.com/"
                             "?page=1&preset=danger\x1b\\Link\x1b]8;;\x1b\\";

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QStringList commands = findFirstLinkCommands();
    QVERIFY2(!commands.isEmpty(), "No hyperlink found in buffer");
    QVERIFY2(!commands.first().contains("preset="),
             qPrintable(QString("Reserved 'preset' param not stripped: %1")
                            .arg(commands.first())));
    QVERIFY2(commands.first().contains("page=1"),
             qPrintable(QString("Non-reserved 'page' param was stripped: %1")
                            .arg(commands.first())));
  }

  void test_Osc8WebUrl_StripsPresetWithEncodedEquals() {
    // Tests that preset parameter is stripped even when = is percent-encoded as
    // %3D, while preserving non-reserved parameters
    QString messageFromMud = "\x1b]8;;https://example.com/"
                             "?preset%3Ddefault&page=1\x1b\\Link\x1b]8;;\x1b\\";

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QStringList commands = findFirstLinkCommands();
    QVERIFY2(!commands.isEmpty(), "No hyperlink found in buffer");
    QVERIFY2(!commands.first().contains("preset%3D"),
             qPrintable(QString("Preset param with encoded = not stripped: %1")
                            .arg(commands.first())));
    QVERIFY2(commands.first().contains("page=1"),
             qPrintable(QString("Non-reserved 'page' param was stripped: %1")
                            .arg(commands.first())));
  }

  void test_Osc8WebUrl_StripsConfigWithEncodedEquals() {
    // Tests that config parameter is stripped even when = is percent-encoded as
    // %3d (lowercase variant of %3D), while preserving non-reserved parameters
    QString messageFromMud = "\x1b]8;;https://example.com/"
                             "?config%3dvalue&foo=bar\x1b\\Link\x1b]8;;\x1b\\";

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QStringList commands = findFirstLinkCommands();
    QVERIFY2(!commands.isEmpty(), "No hyperlink found in buffer");
    QVERIFY2(!commands.first().contains("config%3d"),
             qPrintable(QString("Config param with encoded = not stripped: %1")
                            .arg(commands.first())));
    QVERIFY2(commands.first().contains("foo=bar"),
             qPrintable(QString("Non-reserved 'foo' param was stripped: %1")
                            .arg(commands.first())));
  }

  void test_Osc8WebUrl_PreservesPercentEncodedReservedNames() {
    // %63%6F%6E%66%69%67 is "config" percent-encoded — should NOT be stripped
    QString messageFromMud =
        "\x1b]8;;https://example.com/"
        "?%63%6F%6E%66%69%67=value\x1b\\Link\x1b]8;;\x1b\\";

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QStringList commands = findFirstLinkCommands();
    QVERIFY2(!commands.isEmpty(), "No hyperlink found in buffer");
    QVERIFY2(
        commands.first().contains("%63%6F%6E%66%69%67=value"),
        qPrintable(
            QString("Percent-encoded 'config' key was incorrectly stripped: %1")
                .arg(commands.first())));
  }

  void test_Osc8SendUrl_StripsAllQueryParameters() {
    QString messageFromMud =
        "\x1b]8;;send:attack?config=%7B%22style%22%3A%7B%22color%22%3A%22red%"
        "22%7D%7D\x1b\\Attack\x1b]8;;\x1b\\";

    mpServer->setWelcomeMessage(messageFromMud);
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QStringList commands = findFirstLinkCommands();
    QVERIFY2(!commands.isEmpty(), "No hyperlink found in buffer");
    QVERIFY2(commands.first().contains("attack"),
             qPrintable(QString("Command missing: %1").arg(commands.first())));
    QVERIFY2(!commands.first().contains("config="),
             qPrintable(QString("Query params leaked into send command: %1")
                            .arg(commands.first())));
  }

  // ═══════════════════════════════════════════════════════════════════
  // OSC 8 Hyperlink Context Menu Title Tests
  // ═══════════════════════════════════════════════════════════════════

  void test_Osc8Title_SimpleStringTitle() {
    QString config = qsl(
        R"({"title":"Lamb and Barley Stew","menu":[{"View Details":"send:look stew"},{"Buy":"send:buy stew"},{"Taste":"send:taste stew"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:look stew"), qsl("[Stew]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Lamb and Barley Stew"));
  }

  void test_Osc8Title_StyledObjectBoldAndColor() {
    QString config = qsl(
        R"({"title":{"text":"Magic Shop - Potions","style":{"color":"#ffd700","bold":true}},"menu":[{"Buy":"send:buy potion"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:buy potion"), qsl("[Shop]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Magic Shop - Potions"));
    QVERIFY(styling.menuTitleStyle.isBold);
    QVERIFY(styling.menuTitleStyle.hasForegroundColor);
    QCOMPARE(styling.menuTitleStyle.foregroundColor, QColor("#ffd700"));
  }

  void test_Osc8Title_StyledObjectItalicAndBackground() {
    QString config = qsl(
        R"({"title":{"text":"Sir Galahad the Brave","style":{"color":"#ffffff","bg":"#333333","italic":true}},"menu":[{"Talk":"send:talk galahad"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:talk galahad"), qsl("[Galahad]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Sir Galahad the Brave"));
    QVERIFY(styling.menuTitleStyle.isItalic);
    QVERIFY(styling.menuTitleStyle.hasForegroundColor);
    QCOMPARE(styling.menuTitleStyle.foregroundColor, QColor("#ffffff"));
    QVERIFY(styling.menuTitleStyle.hasBackgroundColor);
    QCOMPARE(styling.menuTitleStyle.backgroundColor, QColor("#333333"));
  }

  void test_Osc8Title_CompactSyntaxTiShorthand() {
    QString config = qsl(
        R"({"ti":"Rusty Sword","m":[{"Equip":"send:wield sword"},{"Drop":"send:drop sword"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:wield sword"), qsl("[Sword]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Rusty Sword"));
  }

  void test_Osc8Title_MenuWithoutTitle() {
    QString config = qsl(
        R"({"menu":[{"North":"send:north"},{"South":"send:south"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:north"), qsl("[Exits]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QVERIFY2(styling.menuTitle.isEmpty(),
             qPrintable(qsl("Expected empty menuTitle but got '%1'")
                            .arg(styling.menuTitle)));
  }

  void test_Osc8Title_TitleWithoutMenu() {
    QString config = qsl(R"({"title":"Lonely Title"})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:test"), qsl("[No Menu]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Lonely Title"));
  }

  void test_Osc8Title_AllTextDecorations() {
    QString config = qsl(
        R"({"title":{"text":"Decorated Title","style":{"color":"#ff0000","bold":true,"italic":true,"underline":true,"strikethrough":true}},"menu":[{"Action":"send:action"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:action"), qsl("[Decorated]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Decorated Title"));
    QVERIFY(styling.menuTitleStyle.isBold);
    QVERIFY(styling.menuTitleStyle.isItalic);
    QVERIFY(styling.menuTitleStyle.isUnderlined);
    QVERIFY(styling.menuTitleStyle.isStrikeOut);
    QVERIFY(styling.menuTitleStyle.hasForegroundColor);
    QCOMPARE(styling.menuTitleStyle.foregroundColor, QColor("#ff0000"));
  }

  void test_Osc8Title_WavyUnderlineStyle() {
    QString config = qsl(
        R"({"title":{"text":"Wavy Title","style":{"color":"#00ff00","underline":"wavy"}},"menu":[{"Test":"send:test"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:test"), qsl("[Wavy]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Wavy Title"));
    QVERIFY(styling.menuTitleStyle.isUnderlined);
    QCOMPARE(styling.menuTitleStyle.underlineStyle,
             Mudlet::HyperlinkStyling::UnderlineWavy);
  }

  void test_Osc8Title_DottedUnderlineStyle() {
    QString config = qsl(
        R"({"title":{"text":"Dotted Title","style":{"color":"#00ff00","underline":"dotted"}},"menu":[{"Test":"send:test"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:test"), qsl("[Dotted]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Dotted Title"));
    QVERIFY(styling.menuTitleStyle.isUnderlined);
    QCOMPARE(styling.menuTitleStyle.underlineStyle,
             Mudlet::HyperlinkStyling::UnderlineDotted);
  }

  void test_Osc8Title_DashedUnderlineStyle() {
    QString config = qsl(
        R"({"title":{"text":"Dashed Title","style":{"color":"#00ff00","underline":"dashed"}},"menu":[{"Test":"send:test"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:test"), qsl("[Dashed]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Dashed Title"));
    QVERIFY(styling.menuTitleStyle.isUnderlined);
    QCOMPARE(styling.menuTitleStyle.underlineStyle,
             Mudlet::HyperlinkStyling::UnderlineDashed);
  }

  void test_Osc8Title_BackgroundColorCssProperty() {
    QString config = qsl(
        R"({"title":{"text":"CSS BG Title","style":{"color":"#ffffff","background-color":"#660000"}},"menu":[{"Action":"send:action"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:action"), qsl("[CSS BG]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("CSS BG Title"));
    QVERIFY(styling.menuTitleStyle.hasBackgroundColor);
    QCOMPARE(styling.menuTitleStyle.backgroundColor, QColor("#660000"));
  }

  void test_Osc8Title_TextDecorationColor() {
    QString config = qsl(
        R"({"title":{"text":"Color Decoration","style":{"color":"#ffffff","underline":true,"text-decoration-color":"#ff00ff"}},"menu":[{"Action":"send:action"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:action"), qsl("[Deco Color]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Color Decoration"));
    QVERIFY(styling.menuTitleStyle.hasUnderlineColor);
    QCOMPARE(styling.menuTitleStyle.underlineColor, QColor("#ff00ff"));
  }

  void test_Osc8Title_EmptyStringTitle() {
    QString config = qsl(R"({"title":"","menu":[{"Action":"send:action"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:action"), qsl("[Empty]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QVERIFY(styling.menuTitle.isEmpty());
  }

  void test_Osc8Title_ObjectWithEmptyText() {
    QString config = qsl(
        R"({"title":{"text":"","style":{"color":"#ff0000"}},"menu":[{"Action":"send:action"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:action"), qsl("[Empty Obj]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QVERIFY(styling.menuTitle.isEmpty());
  }

  void test_Osc8Title_ObjectWithoutTextKey() {
    QString config = qsl(
        R"({"title":{"style":{"color":"#ff0000"}},"menu":[{"Action":"send:action"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:action"), qsl("[No Text]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QVERIFY(styling.menuTitle.isEmpty());
  }

  void test_Osc8Title_ObjectWithoutStyleKey() {
    QString config = qsl(
        R"({"title":{"text":"Style-less Title"},"menu":[{"Action":"send:action"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:action"), qsl("[No Style]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Style-less Title"));
    QVERIFY(!styling.menuTitleStyle.hasCustomStyling);
  }

  void test_Osc8Title_UnicodeCharacters() {
    QString config = qsl(
        R"({"title":"Potion du Guerrier","menu":[{"Drink":"send:drink potion"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:drink potion"), qsl("[Potion]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Potion du Guerrier"));
  }

  void test_Osc8Title_SpecialCharacters() {
    QString config = qsl(
        R"({"title":"Item <Rare> [+5] & More!","menu":[{"Use":"send:use item"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:use item"), qsl("[Special]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Item <Rare> [+5] & More!"));
  }

  void test_Osc8Title_AlongsideOtherConfigProperties() {
    QString config = qsl(
        R"({"title":"Full Config","tooltip":"A helpful tooltip","style":{"color":"#00ffff"},"menu":[{"Action 1":"send:action1"},{"Action 2":"send:action2"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:action1"), qsl("[Full Config]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, qsl("Full Config"));
  }

  void test_Osc8Title_NumericValueIgnored() {
    QString config = qsl(R"({"title":42,"menu":[{"Action":"send:action"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:action"), qsl("[Num]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QVERIFY2(styling.menuTitle.isEmpty(),
             qPrintable(qsl("Numeric title should be ignored but got '%1'")
                            .arg(styling.menuTitle)));
  }

  void test_Osc8Title_BooleanValueIgnored() {
    QString config =
        qsl(R"({"title":true,"menu":[{"Action":"send:action"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:action"), qsl("[Bool]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QVERIFY2(styling.menuTitle.isEmpty(),
             qPrintable(qsl("Boolean title should be ignored but got '%1'")
                            .arg(styling.menuTitle)));
  }

  void test_Osc8Title_ArrayValueIgnored() {
    QString config =
        qsl(R"({"title":["a","b"],"menu":[{"Action":"send:action"}]})");
    mpServer->setWelcomeMessage(
        buildOsc8WithConfig(qsl("send:action"), qsl("[Array]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    auto styling = findFirstLinkStyling();
    QVERIFY2(styling.menuTitle.isEmpty(),
             qPrintable(qsl("Array title should be ignored but got '%1'")
                            .arg(styling.menuTitle)));
  }

  void test_Osc8Title_LinkTextDisplayedInBuffer() {
    QString config = qsl(
        R"({"title":"Lamb and Barley Stew","menu":[{"View Details":"send:look stew"}]})");
    mpServer->setWelcomeMessage(buildOsc8WithConfig(
        qsl("send:look stew"), qsl("[Lamb and Barley Stew]"), config));
    startProfile(mHostname, mLocalhost, mPort);
    QSignalSpy spy(mudlet::self()->getActiveHost()->mpConsole,
                   &TMainConsole::signal_newDataAlert);
    QVERIFY(spy.wait(200));

    QString actualText =
        mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
    QVERIFY2(
        actualText.contains(qsl("[Lamb and Barley Stew]")),
        qPrintable(qsl("Expected link text in buffer but got '%1'")
                       .arg(actualText)));
  }

  void cleanup() {
    delete mpServer;
    mpServer = nullptr;
    deleteProfileDirectory(mHostname);
    delete mudlet::self();
  }

  void startProfile(const QString &hostname, const QString &address,
                    const QString &port) {
    QTimer::singleShot(0, qApp, [hostname, address, port]() {
      mudlet::self()->startAutoLogin({});
      QTest::qWait(100);
      QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button,
                        Qt::LeftButton);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), hostname);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), address);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), port);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
    });

    QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
    if (!spy.wait(1000)) {
      QFAIL("Profile took too long to load.");
    }
    auto host = mudlet::self()->getActiveHost();
    if (!host) {
      QFAIL("No active host available for the test.");
    }

    QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
    if (!spy2.wait(500)) {
      QFAIL("Could not connect with the host.");
    }
  }

  void deleteProfileDirectory(const QString &profileName) {
    const QString path =
        mudlet::getMudletPath(enums::profileHomePath, profileName);
    QDir dir(path);

    if (!dir.exists()) {
      return;
    }
    dir.removeRecursively();
  }
};

void initializeQRCResourcesForOscTest() {
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
  qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
  qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
  qInitResources_mudlet_fonts_posix();
#endif
#endif
  qInitResources_mudlet();
  qInitResources_qm();
}

#include "TOscTest.moc"
QTEST_MAIN(TOscTest)
