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
 * Uses loopbackTest() to inject data directly into the telnet processing
 * pipeline, avoiding per-test TCP connections and profile creation.
 * Mudlet is started once in initTestCase and reused across all tests.
 *
 * Run with: ctest -R TOscTest -V
 */

#include <QKeyEvent>
#include <QtTest/QtTest>

#include "MudletInstanceCoordinator.h"
#include "TAccessibleTextEdit.h"
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
  Host *mpHost = nullptr;
  const QString mHostname = "OSC-Test-Host";
  const QString mPort = "4002";
  const QString mLocalhost = "localhost";

  // Injects raw telnet data into the processing pipeline via loopback and
  // waits for the buffer to process it.
  void injectData(const QString &message) {
    QByteArray data = (message + qsl("\r\n")).toUtf8();
    mpHost->mTelnet.loopbackTest(data);
    QTest::qWait(50);
  }

  // Scans backward through the buffer to find the first hyperlink and returns
  // its command list from the link store.
  QStringList findFirstLinkCommands() {
    if (!mpHost || !mpHost->mpConsole) {
      return {};
    }
    TMainConsole *console = mpHost->mpConsole;
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
    if (!mpHost || !mpHost->mpConsole) {
      return {};
    }
    TMainConsole *console = mpHost->mpConsole;
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

  // Scans forward through the buffer to find the first hyperlink and returns
  // its (line, column) position, or std::nullopt if none found.
  std::optional<std::pair<int, int>> findFirstLinkPosition() {
    if (!mpHost || !mpHost->mpConsole) {
      return std::nullopt;
    }
    TMainConsole *console = mpHost->mpConsole;
    for (int line = 0; line <= console->buffer.getLastLineNumber(); ++line) {
      for (int col = 0; col < console->buffer.line(line).length(); ++col) {
        if (console->buffer.getLinkIndexAt(line, col) > 0) {
          return std::pair{line, col};
        }
      }
    }
    return std::nullopt;
  }

  // Scans forward through the buffer to find the first hyperlink and returns
  // its (line, column, linkId), or std::nullopt if none found.
  std::optional<std::tuple<int, int, int>> findFirstLinkPositionWithId() {
    if (!mpHost || !mpHost->mpConsole) {
      return std::nullopt;
    }
    TMainConsole *console = mpHost->mpConsole;
    for (int line = 0; line <= console->buffer.getLastLineNumber(); ++line) {
      for (int col = 0; col < console->buffer.line(line).length(); ++col) {
        int lid = console->buffer.getLinkIndexAt(line, col);
        if (lid > 0) {
          return std::tuple{line, col, lid};
        }
      }
    }
    return std::nullopt;
  }

  // Scans forward through the buffer to find the nth distinct hyperlink
  // (1-based) and returns its (line, column, linkId), or std::nullopt.
  std::optional<std::tuple<int, int, int>> findNthDistinctLinkPosition(int n) {
    if (!mpHost || !mpHost->mpConsole) {
      return std::nullopt;
    }
    TMainConsole *console = mpHost->mpConsole;
    QSet<int> seenIds;
    for (int line = 0; line <= console->buffer.getLastLineNumber(); ++line) {
      for (int col = 0; col < console->buffer.line(line).length(); ++col) {
        int lid = console->buffer.getLinkIndexAt(line, col);
        if (lid > 0 && !seenIds.contains(lid)) {
          seenIds.insert(lid);
          if (seenIds.size() == n) {
            return std::tuple{line, col, lid};
          }
        }
      }
    }
    return std::nullopt;
  }

private slots:
  // Start mudlet and create a profile once for all tests.
  void initTestCase() {
    initializeQRCResourcesForOscTest();

    mpServer = new TelnetServerStub(qApp);
    mpServer->start(mLocalhost, mPort.toUShort());
    mudlet::start();
    mudlet::self()->setupConfig();
    mudlet::self()->takeOwnershipOfInstanceCoordinator(
        std::make_unique<MudletInstanceCoordinator>(
            "MudletInstanceCoordinator"));
    mudlet::self()->init();
    mudlet::self()->setStorePasswordsSecurely(false);

    const QString path =
        mudlet::getMudletPath(enums::profileHomePath, mHostname);
    QDir(path).removeRecursively();

    QTimer::singleShot(0, qApp, [this]() {
      mudlet::self()->startAutoLogin({});
      QTest::qWait(100);
      QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button,
                        Qt::LeftButton);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), mHostname);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
      QTest::qWait(100);
      QTest::keyClicks(QApplication::focusWidget(), mPort);
      QTest::qWait(100);
      QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
    });

    QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
    if (!spy.wait(1000)) {
      QFAIL("Profile took too long to load.");
    }
    mpHost = mudlet::self()->getActiveHost();
    if (!mpHost) {
      QFAIL("No active host available for the test.");
    }

    QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
    if (!spy2.wait(500)) {
      QFAIL("Could not connect with the host.");
    }
  }

  // Clear buffer and link state before each test for isolation.
  void init() {
    QVERIFY(mpHost);
    QVERIFY(mpHost->mpConsole);
    mpHost->mpConsole->buffer.clear();
  }

  // Data-driven test: verifies text after various OSC sequences is displayed
  // correctly (not swallowed by the parser).
  void test_OscTextDisplay_data() {
    QTest::addColumn<QString>("message");
    QTest::addColumn<QString>("expectedText");
    // BEL-terminated OSC 8 splits link text across buffer lines in loopback
    // mode, so exact match on the joined text isn't possible - use contains.
    QTest::addColumn<bool>("exactMatch");

    QTest::newRow("BEL-terminated OSC 2 (window title)")
        << QString("\x1b]2;Window Title\x07Hello World") << qsl("Hello World")
        << true;
    QTest::newRow("ST-terminated OSC P (color redefine)")
        << QString("\x1b]P0FF0000\x1b\\Hello") << qsl("Hello") << true;
    QTest::newRow("BEL-terminated OSC 8 (hyperlink)")
        << QString(
               "\x1b]8;;http://example.com\x07Link Text\x1b]8;;\x07 After Link")
        << qsl("Link Text After Link") << false;
    QTest::newRow("BEL-terminated OSC P")
        << QString("\x1b]P0FF0000\x07Hello") << qsl("Hello") << true;
    QTest::newRow("empty OSC sequence")
        << QString("\x1b]\x07Normal text") << qsl("Normal text") << true;
    QTest::newRow("OSC exceeds length limit")
        << QString("\x1b]2;") + QString(5000, 'A') + QString("\x07Normal text")
        << qsl("Normal text") << true;
  }

  void test_OscTextDisplay() {
    QFETCH(QString, message);
    QFETCH(QString, expectedText);
    QFETCH(bool, exactMatch);

    injectData(message);

    // Join all buffer lines since cursor position varies with loopback.
    TMainConsole *console = mpHost->mpConsole;
    QString allText;
    for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
      allText += console->buffer.line(i);
    }
    if (exactMatch) {
      QCOMPARE(allText.trimmed(), expectedText);
    } else {
      QVERIFY2(allText.contains(expectedText),
               qPrintable(qsl("Expected buffer to contain '%1' but got '%2'")
                              .arg(expectedText, allText)));
    }
  }

  // =====================================================================
  // OSC 8 Hyperlink URL Parameter Tests
  // =====================================================================

  // Data-driven test: verifies OSC 8 URL query parameter handling - reserved
  // params (config, preset) are stripped while user params are preserved.
  void test_Osc8UrlParams_data() {
    QTest::addColumn<QString>("message");
    QTest::addColumn<QStringList>("mustContain");
    QTest::addColumn<QStringList>("mustNotContain");

    QTest::newRow("preserves web URL query params")
        << qsl("\x1b]8;;https://example.com/"
               "?id=42&lang=en\x1b\\Link\x1b]8;;\x1b\\")
        << QStringList{"id=42&lang=en"} << QStringList{};
    QTest::newRow("strips config param")
        << qsl("\x1b]8;;https://example.com/"
               "?config=%7B%22style%22%3A%7B%22color%22%3A%22red%22%7D%"
               "7D\x1b\\Styled\x1b]8;;\x1b\\")
        << QStringList{} << QStringList{"config="};
    QTest::newRow("strips preset, preserves other params")
        << qsl("\x1b]8;;https://example.com/"
               "?page=1&preset=danger\x1b\\Link\x1b]8;;\x1b\\")
        << QStringList{"page=1"} << QStringList{"preset="};
    QTest::newRow("strips preset with encoded equals")
        << qsl("\x1b]8;;https://example.com/"
               "?preset%3Ddefault&page=1\x1b\\Link\x1b]8;;\x1b\\")
        << QStringList{"page=1"} << QStringList{"preset%3D"};
    QTest::newRow("strips config with encoded equals (lowercase)")
        << qsl("\x1b]8;;https://example.com/"
               "?config%3dvalue&foo=bar\x1b\\Link\x1b]8;;\x1b\\")
        << QStringList{"foo=bar"} << QStringList{"config%3d"};
    QTest::newRow("preserves percent-encoded reserved names")
        << qsl("\x1b]8;;https://example.com/"
               "?%63%6F%6E%66%69%67=value\x1b\\Link\x1b]8;;\x1b\\")
        << QStringList{"%63%6F%6E%66%69%67=value"} << QStringList{};
    QTest::newRow("send URL strips all query params")
        << qsl("\x1b]8;;send:attack?config=%7B%22style%22%3A%7B%22color%22%3A%"
               "22red%22%7D%7D\x1b\\Attack\x1b]8;;\x1b\\")
        << QStringList{"attack"} << QStringList{"config="};
  }

  void test_Osc8UrlParams() {
    QFETCH(QString, message);
    QFETCH(QStringList, mustContain);
    QFETCH(QStringList, mustNotContain);

    injectData(message);

    QStringList commands = findFirstLinkCommands();
    QVERIFY2(!commands.isEmpty(), "No hyperlink found in buffer");
    for (const auto &expected : mustContain) {
      QVERIFY2(
          commands.first().contains(expected),
          qPrintable(
              qsl("Expected '%1' in: %2").arg(expected, commands.first())));
    }
    for (const auto &forbidden : mustNotContain) {
      QVERIFY2(!commands.first().contains(forbidden),
               qPrintable(qsl("Did not expect '%1' in: %2")
                              .arg(forbidden, commands.first())));
    }
  }

  // =====================================================================
  // OSC 8 Hyperlink Context Menu Title Tests
  // =====================================================================

  // Data-driven test: verifies menuTitle text is parsed correctly from various
  // config JSON shapes (simple string, object, compact syntax, invalid types).
  void test_Osc8Title_TextParsing_data() {
    QTest::addColumn<QString>("config");
    QTest::addColumn<QString>("expectedTitle");
    QTest::addColumn<bool>("hasCustomStyling");

    QTest::newRow("simple string")
        << qsl(R"({"title":"Lamb and Barley Stew","menu":[{"View Details":"send:look stew"}]})")
        << qsl("Lamb and Barley Stew") << false;
    QTest::newRow("compact ti shorthand")
        << qsl(R"({"ti":"Rusty Sword","m":[{"Equip":"send:wield sword"}]})")
        << qsl("Rusty Sword") << false;
    QTest::newRow("title without menu")
        << qsl(R"({"title":"Lonely Title"})") << qsl("Lonely Title") << false;
    QTest::newRow("menu without title")
        << qsl(R"({"menu":[{"North":"send:north"}]})") << QString() << false;
    QTest::newRow("empty string")
        << qsl(R"({"title":"","menu":[{"Action":"send:action"}]})") << QString()
        << false;
    QTest::newRow("object with empty text")
        << qsl(R"({"title":{"text":"","style":{"color":"#ff0000"}},"menu":[{"Action":"send:action"}]})")
        << QString() << true;
    QTest::newRow("object without text key")
        << qsl(R"({"title":{"style":{"color":"#ff0000"}},"menu":[{"Action":"send:action"}]})")
        << QString() << true;
    QTest::newRow("object without style key")
        << qsl(R"({"title":{"text":"Style-less Title"},"menu":[{"Action":"send:action"}]})")
        << qsl("Style-less Title") << false;
    QTest::newRow("unicode characters")
        << qsl(R"({"title":"Potion du Guerrier","menu":[{"Drink":"send:drink potion"}]})")
        << qsl("Potion du Guerrier") << false;
    QTest::newRow("special characters")
        << qsl(R"({"title":"Item <Rare> [+5] & More!","menu":[{"Use":"send:use item"}]})")
        << qsl("Item <Rare> [+5] & More!") << false;
    QTest::newRow("alongside other config")
        << qsl(R"({"title":"Full Config","tooltip":"A helpful tooltip","style":{"color":"#00ffff"},"menu":[{"Action 1":"send:action1"}]})")
        << qsl("Full Config") << true;
    QTest::newRow("numeric value ignored")
        << qsl(R"({"title":42,"menu":[{"Action":"send:action"}]})") << QString()
        << false;
    QTest::newRow("boolean value ignored")
        << qsl(R"({"title":true,"menu":[{"Action":"send:action"}]})")
        << QString() << false;
    QTest::newRow("array value ignored")
        << qsl(R"({"title":["a","b"],"menu":[{"Action":"send:action"}]})")
        << QString() << false;
  }

  void test_Osc8Title_TextParsing() {
    QFETCH(QString, config);
    QFETCH(QString, expectedTitle);
    QFETCH(bool, hasCustomStyling);

    injectData(buildOsc8WithConfig(qsl("send:action"), qsl("[Link]"), config));

    // Verify a link was actually created in the buffer - prevents silent
    // passes when the injection pipeline is broken entirely.
    QStringList commands = findFirstLinkCommands();
    QVERIFY2(!commands.isEmpty(), "No hyperlink found in buffer");

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, expectedTitle);
    if (!hasCustomStyling) {
      QVERIFY2(!styling.menuTitleStyle.hasCustomStyling,
               "Expected no custom styling on menu title");
    }
  }

  // Data-driven test: verifies menuTitleStyle properties are parsed correctly.
  void test_Osc8Title_StyleParsing_data() {
    QTest::addColumn<QString>("config");
    QTest::addColumn<QString>("expectedTitle");
    QTest::addColumn<bool>("bold");
    QTest::addColumn<bool>("italic");
    QTest::addColumn<bool>("underlined");
    QTest::addColumn<bool>("strikeOut");
    QTest::addColumn<QString>("fgColor");
    QTest::addColumn<QString>("bgColor");
    QTest::addColumn<int>("underlineStyle");
    QTest::addColumn<QString>("underlineColor");

    QTest::newRow("bold and color")
        << qsl(R"({"title":{"text":"Magic Shop - Potions","style":{"color":"#ffd700","bold":true}},"menu":[{"Buy":"send:buy potion"}]})")
        << qsl("Magic Shop - Potions") << true << false << false << false
        << qsl("#ffd700") << QString() << -1 << QString();
    QTest::newRow("italic and background")
        << qsl(R"({"title":{"text":"Sir Galahad the Brave","style":{"color":"#ffffff","bg":"#333333","italic":true}},"menu":[{"Talk":"send:talk galahad"}]})")
        << qsl("Sir Galahad the Brave") << false << true << false << false
        << qsl("#ffffff") << qsl("#333333") << -1 << QString();
    QTest::newRow("all text decorations")
        << qsl(R"({"title":{"text":"Decorated Title","style":{"color":"#ff0000","bold":true,"italic":true,"underline":true,"strikethrough":true}},"menu":[{"Action":"send:action"}]})")
        << qsl("Decorated Title") << true << true << true << true
        << qsl("#ff0000") << QString() << -1 << QString();
    QTest::newRow("wavy underline")
        << qsl(R"({"title":{"text":"Wavy Title","style":{"color":"#00ff00","underline":"wavy"}},"menu":[{"Test":"send:test"}]})")
        << qsl("Wavy Title") << false << false << true << false
        << qsl("#00ff00") << QString()
        << static_cast<int>(Mudlet::HyperlinkStyling::UnderlineWavy)
        << QString();
    QTest::newRow("dotted underline")
        << qsl(R"({"title":{"text":"Dotted Title","style":{"color":"#00ff00","underline":"dotted"}},"menu":[{"Test":"send:test"}]})")
        << qsl("Dotted Title") << false << false << true << false
        << qsl("#00ff00") << QString()
        << static_cast<int>(Mudlet::HyperlinkStyling::UnderlineDotted)
        << QString();
    QTest::newRow("dashed underline")
        << qsl(R"({"title":{"text":"Dashed Title","style":{"color":"#00ff00","underline":"dashed"}},"menu":[{"Test":"send:test"}]})")
        << qsl("Dashed Title") << false << false << true << false
        << qsl("#00ff00") << QString()
        << static_cast<int>(Mudlet::HyperlinkStyling::UnderlineDashed)
        << QString();
    QTest::newRow("background-color CSS property")
        << qsl(R"({"title":{"text":"CSS BG Title","style":{"color":"#ffffff","background-color":"#660000"}},"menu":[{"Action":"send:action"}]})")
        << qsl("CSS BG Title") << false << false << false << false
        << qsl("#ffffff") << qsl("#660000") << -1 << QString();
    QTest::newRow("text-decoration-color")
        << qsl(R"({"title":{"text":"Color Decoration","style":{"color":"#ffffff","underline":true,"text-decoration-color":"#ff00ff"}},"menu":[{"Action":"send:action"}]})")
        << qsl("Color Decoration") << false << false << true << false
        << qsl("#ffffff") << QString() << -1 << qsl("#ff00ff");
  }

  void test_Osc8Title_StyleParsing() {
    QFETCH(QString, config);
    QFETCH(QString, expectedTitle);
    QFETCH(bool, bold);
    QFETCH(bool, italic);
    QFETCH(bool, underlined);
    QFETCH(bool, strikeOut);
    QFETCH(QString, fgColor);
    QFETCH(QString, bgColor);
    QFETCH(int, underlineStyle);
    QFETCH(QString, underlineColor);

    injectData(buildOsc8WithConfig(qsl("send:action"), qsl("[Link]"), config));

    auto styling = findFirstLinkStyling();
    QCOMPARE(styling.menuTitle, expectedTitle);
    QCOMPARE(styling.menuTitleStyle.isBold, bold);
    QCOMPARE(styling.menuTitleStyle.isItalic, italic);
    QCOMPARE(styling.menuTitleStyle.isUnderlined, underlined);
    QCOMPARE(styling.menuTitleStyle.isStrikeOut, strikeOut);
    if (!fgColor.isEmpty()) {
      QVERIFY(styling.menuTitleStyle.hasForegroundColor);
      QCOMPARE(styling.menuTitleStyle.foregroundColor, QColor(fgColor));
    }
    if (!bgColor.isEmpty()) {
      QVERIFY(styling.menuTitleStyle.hasBackgroundColor);
      QCOMPARE(styling.menuTitleStyle.backgroundColor, QColor(bgColor));
    }
    if (underlineStyle >= 0) {
      QCOMPARE(styling.menuTitleStyle.underlineStyle,
               static_cast<Mudlet::HyperlinkStyling::UnderlineStyle>(
                   underlineStyle));
    }
    if (!underlineColor.isEmpty()) {
      QVERIFY(styling.menuTitleStyle.hasUnderlineColor);
      QCOMPARE(styling.menuTitleStyle.underlineColor, QColor(underlineColor));
    }
  }

  void test_Osc8Title_LinkTextDisplayedInBuffer() {
    QString config = qsl(
        R"({"title":"Lamb and Barley Stew","menu":[{"View Details":"send:look stew"}]})");
    injectData(buildOsc8WithConfig(qsl("send:look stew"),
                                   qsl("[Lamb and Barley Stew]"), config));

    TMainConsole *console = mpHost->mpConsole;
    QString allText;
    for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
      allText += console->buffer.line(i);
    }
    QVERIFY2(
        allText.contains(qsl("[Lamb and Barley Stew]")),
        qPrintable(
            qsl("Expected link text in buffer but got '%1'").arg(allText)));
  }

  // =====================================================================
  // OSC 8 Hyperlink Navigation Tests (Tab/Shift+Tab support)
  // =====================================================================

  void test_FindNextLink_MultipleLinks() {
    // Inject two distinct links on the same line
    injectData(qsl("\x1b]8;;send:north\x1b\\North\x1b]8;;\x1b\\ - "
                   "\x1b]8;;send:south\x1b\\South\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;

    // Starting from position 0 (on the first link), findNextLink should find
    // the second
    int outLine = -1, outCol = -1;
    bool found = console->buffer.findNextLink(0, 0, outLine, outCol);
    QVERIFY2(found, "Expected to find a second link after the first");

    // The second link should be at a column past "North - "
    int secondLinkIndex = console->buffer.getLinkIndexAt(outLine, outCol);
    QVERIFY2(secondLinkIndex > 0,
             "Expected a valid link index at the found position");

    // Verify it's actually a different link from the first
    int firstLinkIndex = console->buffer.getLinkIndexAt(0, 0);
    QVERIFY2(
        firstLinkIndex != secondLinkIndex,
        "findNextLink should return a different link than the starting one");
  }

  void test_FindNextLink_NoLinks() {
    // Inject plain text with no links
    injectData(qsl("Just some plain text with no links"));

    TMainConsole *console = mpHost->mpConsole;

    int outLine = -1, outCol = -1;
    bool found = console->buffer.findNextLink(0, 0, outLine, outCol);
    QVERIFY2(!found, "findNextLink should return false when no links exist");
  }

  void test_FindNextLink_SkipsCurrentLink() {
    // Inject a single link
    injectData(qsl("\x1b]8;;send:look\x1b\\Look around\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;

    // Find where the link starts
    auto pos = findFirstLinkPosition();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol] = *pos;

    // Starting from the link, there should be no "next" link
    int outLine = -1, outCol = -1;
    bool found2 =
        console->buffer.findNextLink(linkLine, linkCol, outLine, outCol);
    QVERIFY2(!found2,
             "findNextLink should skip the current link and find nothing else");
  }

  void test_FindPreviousLink_MultipleLinks() {
    // Inject two distinct links
    injectData(qsl("\x1b]8;;send:north\x1b\\North\x1b]8;;\x1b\\ - "
                   "\x1b]8;;send:south\x1b\\South\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;

    // Find the first and second distinct links
    auto first = findNthDistinctLinkPosition(1);
    QVERIFY2(first.has_value(), "Expected to find a first link in the buffer");
    auto [firstLine, firstCol, firstLinkId] = *first;

    auto second = findNthDistinctLinkPosition(2);
    QVERIFY2(second.has_value(),
             "Expected to find a second link in the buffer");
    auto [secondLine, secondCol, secondLinkId] = *second;

    // From the second link, findPreviousLink should find the first
    int outLine = -1, outCol = -1;
    bool found = console->buffer.findPreviousLink(secondLine, secondCol,
                                                  outLine, outCol);
    QVERIFY2(found, "Expected to find the first link when scanning backward "
                    "from the second");

    int foundLinkId = console->buffer.getLinkIndexAt(outLine, outCol);
    QCOMPARE(foundLinkId, firstLinkId);
  }

  void test_FindPreviousLink_AtStartOfBuffer() {
    // Inject a single link
    injectData(qsl("\x1b]8;;send:look\x1b\\Look\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;

    // Starting from 0,0 there should be no previous link
    int outLine = -1, outCol = -1;
    bool found = console->buffer.findPreviousLink(0, 0, outLine, outCol);
    QVERIFY2(!found,
             "findPreviousLink should return false at the start of the buffer");
  }

  // =====================================================================
  // OSC 8 Hyperlink Tooltip Tests
  // =====================================================================

  void test_GetLinkTooltip_data() {
    QTest::addColumn<QString>("config");
    QTest::addColumn<QString>("expectedTooltip");

    QTest::newRow("tooltip from config")
        << qsl(R"({"tooltip":"A magical sword","style":{"color":"#ff0000"}})")
        << qsl("A magical sword");
    QTest::newRow("no tooltip in config - uses default send hint")
        << qsl(R"({"style":{"color":"#00ff00"}})") << qsl("Send: look sword");
    QTest::newRow("empty config - uses default send hint")
        << QString() << qsl("Send: look sword");
  }

  void test_GetLinkTooltip() {
    QFETCH(QString, config);
    QFETCH(QString, expectedTooltip);

    injectData(
        buildOsc8WithConfig(qsl("send:look sword"), qsl("[Sword]"), config));

    TMainConsole *console = mpHost->mpConsole;
    int linkId = 0;
    for (int line = console->buffer.getLastLineNumber();
         line >= 0 && linkId == 0; --line) {
      linkId = console->buffer.getLinkIndexAt(line, 0);
    }
    QVERIFY2(linkId > 0, "Expected to find a link in the buffer");

    QString tooltip = console->buffer.getLinkTooltip(linkId);
    QCOMPARE(tooltip, expectedTooltip);
  }

  void test_GetLinkTooltip_InvalidIndex() {
    TMainConsole *console = mpHost->mpConsole;

    // Index 0 and negative should return empty
    QCOMPARE(console->buffer.getLinkTooltip(0), QString());
    QCOMPARE(console->buffer.getLinkTooltip(-1), QString());
    // Non-existent positive index should return empty
    QCOMPARE(console->buffer.getLinkTooltip(99999), QString());
  }

  // =====================================================================
  // OSC 8 Hyperlink Visited State Tests
  // =====================================================================

  void test_LinkVisitedState() {
    injectData(qsl("\x1b]8;;send:look\x1b\\Look\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;
    int linkId = 0;
    for (int line = console->buffer.getLastLineNumber();
         line >= 0 && linkId == 0; --line) {
      linkId = console->buffer.getLinkIndexAt(line, 0);
    }
    QVERIFY2(linkId > 0, "Expected to find a link in the buffer");

    // Should not be visited initially
    QVERIFY2(!console->buffer.isLinkVisited(linkId),
             "Link should not be visited initially");

    // Mark as visited
    console->buffer.markLinkAsVisited(linkId);
    QVERIFY2(console->buffer.isLinkVisited(linkId),
             "Link should be visited after markLinkAsVisited");
  }

  // =====================================================================
  // OSC 8 Hyperlink Accessible Attributes Tests
  // =====================================================================

  void test_LinkAttributes_ExposedInAccessible() {
    injectData(qsl("\x1b]8;;https://example.com\x1b\\Click me\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;

    // Find the link position in the buffer
    auto pos = findFirstLinkPosition();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol] = *pos;

    // Create the accessible interface for the upper pane
    TTextEdit *pane = console->mUpperPane;
    QVERIFY2(pane, "Expected upper pane to exist");

    TAccessibleTextEdit accessible(pane);

    // Compute the offset for the link position
    int offset = 0;
    for (int i = 0; i < linkLine; ++i) {
      offset += console->buffer.line(i).length() + 1; // +1 for \n
    }
    offset += linkCol;

    // Query attributes at the link offset
    int startOffset = 0, endOffset = 0;
    QString attrs = accessible.attributes(offset, &startOffset, &endOffset);

    // Verify link-specific attributes are present
    QVERIFY2(
        attrs.contains(qsl("text-link:")),
        qPrintable(
            qsl("Expected 'text-link:' in attributes but got: %1").arg(attrs)));
    QVERIFY2(
        attrs.contains(qsl("example.com")),
        qPrintable(qsl("Expected URL in attributes but got: %1").arg(attrs)));
  }

  void test_LinkAttributes_NotPresentOnPlainText() {
    injectData(qsl("Just plain text here"));

    TMainConsole *console = mpHost->mpConsole;
    TTextEdit *pane = console->mUpperPane;
    QVERIFY2(pane, "Expected upper pane to exist");

    TAccessibleTextEdit accessible(pane);

    // Query attributes at position 0 (plain text)
    int startOffset = 0, endOffset = 0;
    QString attrs = accessible.attributes(0, &startOffset, &endOffset);

    // Verify no link attributes are present
    QVERIFY2(!attrs.contains(qsl("text-link:")),
             qPrintable(
                 qsl("Did not expect 'text-link:' in plain text attributes: %1")
                     .arg(attrs)));
  }

  void test_LinkAttributes_VisitedState() {
    injectData(qsl("\x1b]8;;send:look\x1b\\Look\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;

    // Find the link
    auto pos = findFirstLinkPositionWithId();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol, linkId] = *pos;

    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    TAccessibleTextEdit accessible(pane);

    int offset = 0;
    for (int i = 0; i < linkLine; ++i) {
      offset += console->buffer.line(i).length() + 1;
    }
    offset += linkCol;

    // Before visiting: should not have visited attribute
    int s = 0, e = 0;
    QString attrsBefore = accessible.attributes(offset, &s, &e);
    QVERIFY2(!attrsBefore.contains(qsl("text-link-visited:true")),
             "Link should not show visited before being visited");

    // Mark as visited
    console->buffer.markLinkAsVisited(linkId);

    // After visiting: should have visited attribute
    QString attrsAfter = accessible.attributes(offset, &s, &e);
    QVERIFY2(
        attrsAfter.contains(qsl("text-link-visited:true")),
        qPrintable(
            qsl("Expected 'text-link-visited:true' after visiting but got: %1")
                .arg(attrsAfter)));
  }

  void test_LinkAttributes_DisabledState() {
    injectData(qsl("\x1b]8;;send:look\x1b\\Disabled Link\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;

    auto pos = findFirstLinkPositionWithId();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol, linkId] = *pos;

    // Set link state to disabled
    console->buffer.setLinkState(linkId,
                                 Mudlet::HyperlinkStyling::StateDisabled);

    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    TAccessibleTextEdit accessible(pane);

    int offset = 0;
    for (int i = 0; i < linkLine; ++i) {
      offset += console->buffer.line(i).length() + 1;
    }
    offset += linkCol;

    int s = 0, e = 0;
    QString attrs = accessible.attributes(offset, &s, &e);
    QVERIFY2(
        attrs.contains(qsl("text-link-disabled:true;")),
        qPrintable(
            qsl("Expected 'text-link-disabled:true;' in attributes but got: %1")
                .arg(attrs)));
  }

  void test_LinkAttributes_SelectedState() {
    injectData(qsl("\x1b]8;;send:look\x1b\\Select Me\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;

    auto pos = findFirstLinkPositionWithId();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol, linkId] = *pos;

    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    TAccessibleTextEdit accessible(pane);

    int offset = 0;
    for (int i = 0; i < linkLine; ++i) {
      offset += console->buffer.line(i).length() + 1;
    }
    offset += linkCol;

    // Before selecting: should not have selected attribute
    int s = 0, e = 0;
    QString attrsBefore = accessible.attributes(offset, &s, &e);
    QVERIFY2(!attrsBefore.contains(qsl("text-link-selected:true")),
             "Link should not show selected before being selected");

    // Mark as selected
    console->buffer.setLinkSelected(linkId, true);

    // After selecting: should have selected attribute
    QString attrsAfter = accessible.attributes(offset, &s, &e);
    QVERIFY2(attrsAfter.contains(qsl("text-link-selected:true;")),
             qPrintable(qsl("Expected 'text-link-selected:true;' after "
                            "selecting but got: %1")
                            .arg(attrsAfter)));
  }

  void test_LinkAttributes_UrlSanitization() {
    injectData(
        qsl("\x1b]8;;https://example.com:8080/path\x1b\\Look\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;

    auto pos = findFirstLinkPosition();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol] = *pos;

    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    TAccessibleTextEdit accessible(pane);

    int offset = 0;
    for (int i = 0; i < linkLine; ++i) {
      offset += console->buffer.line(i).length() + 1;
    }
    offset += linkCol;

    int s = 0, e = 0;
    QString attrs = accessible.attributes(offset, &s, &e);
    // Colons in the URL should be escaped as \:
    QVERIFY2(attrs.contains(qsl("\\:")),
             qPrintable(qsl("Expected escaped colons (\\:) in text-link "
                            "attribute but got: %1")
                            .arg(attrs)));
    // Verify the attribute does not contain an unescaped colon after
    // "text-link:" The "text-link:" prefix itself has an unescaped colon, but
    // colons within the URL value must be escaped
    int linkAttrStart = attrs.indexOf(qsl("text-link:"));
    QVERIFY(linkAttrStart >= 0);
    QString linkValue = attrs.mid(linkAttrStart + 10); // after "text-link:"
    int semiPos = linkValue.indexOf(QLatin1Char(';'));
    QVERIFY(semiPos > 0);
    linkValue = linkValue.left(semiPos);
    // Every colon in the value should be preceded by a backslash
    for (int i = 0; i < linkValue.size(); ++i) {
      if (linkValue.at(i) == QLatin1Char(':')) {
        QVERIFY2(i > 0 && linkValue.at(i - 1) == QLatin1Char('\\'),
                 qPrintable(qsl("Found unescaped colon in text-link value: %1")
                                .arg(linkValue)));
      }
    }
  }

  void test_LinkAttributes_SemicolonAndBackslashEscaping() {
    // URL contains semicolons and backslashes that must be escaped in
    // IAccessible2 text attributes
    injectData(qsl("\x1b]8;;https://example.com/"
                   "path;param\\extra\x1b\\Look\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;

    auto pos = findFirstLinkPosition();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol] = *pos;

    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    TAccessibleTextEdit accessible(pane);

    int offset = 0;
    for (int i = 0; i < linkLine; ++i) {
      offset += console->buffer.line(i).length() + 1;
    }
    offset += linkCol;

    int s = 0, e = 0;
    QString attrs = accessible.attributes(offset, &s, &e);

    // Extract the text-link value, skipping escaped semicolons (\;)
    int linkAttrStart = attrs.indexOf(qsl("text-link:"));
    QVERIFY(linkAttrStart >= 0);
    QString linkValue = attrs.mid(linkAttrStart + 10);
    int semiEnd = -1;
    for (int i = 0; i < linkValue.length(); ++i) {
      if (linkValue.at(i) == QLatin1Char(';') &&
          (i == 0 || linkValue.at(i - 1) != QLatin1Char('\\'))) {
        semiEnd = i;
        break;
      }
    }
    QVERIFY(semiEnd > 0);
    linkValue = linkValue.left(semiEnd);

    // Semicolons within the URL value should be escaped as \;
    QVERIFY2(linkValue.contains(qsl("\\;")),
             qPrintable(qsl("Expected escaped semicolons in text-link "
                            "value but got: %1")
                            .arg(linkValue)));
    // Backslashes within the URL value should be escaped as double backslash
    const QString escapedBackslash = qsl("\\\\");
    QVERIFY2(linkValue.contains(escapedBackslash),
             qPrintable(qsl("Expected escaped backslashes in text-link "
                            "value but got: %1")
                            .arg(linkValue)));
  }

  void test_LinkAttributes_GroupAttribute() {
    // Link with a selection group should expose text-link-group attribute
    QString config = qsl(
        R"({"selection":{"group":"combat","value":"sword","toggle":true}})");
    injectData(
        buildOsc8WithConfig(qsl("send:equip sword"), qsl("[Sword]"), config));

    TMainConsole *console = mpHost->mpConsole;

    auto pos = findFirstLinkPosition();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol] = *pos;

    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    TAccessibleTextEdit accessible(pane);

    int offset = 0;
    for (int i = 0; i < linkLine; ++i) {
      offset += console->buffer.line(i).length() + 1;
    }
    offset += linkCol;

    int s = 0, e = 0;
    QString attrs = accessible.attributes(offset, &s, &e);

    QVERIFY2(attrs.contains(qsl("text-link-group:")),
             qPrintable(qsl("Expected 'text-link-group:' in attributes "
                            "but got: %1")
                            .arg(attrs)));
    // The group value should contain the group name (may be escaped)
    QVERIFY2(attrs.contains(qsl("combat")),
             qPrintable(qsl("Expected group name 'combat' in attributes "
                            "but got: %1")
                            .arg(attrs)));
  }

  // =====================================================================
  // OSC 8 Hyperlink QAccessible::Description Tests
  // =====================================================================

  void test_Description_CaretOnLink() {
    QString config = qsl(
        R"({"tooltip":"A magical sword","menu":[{"View Details":"send:look sword"}]})");
    injectData(
        buildOsc8WithConfig(qsl("send:look sword"), qsl("[Sword]"), config));

    TMainConsole *console = mpHost->mpConsole;

    // Find the link position
    auto pos = findFirstLinkPosition();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol] = *pos;

    // Enable caret mode and move caret to the link
    mpHost->setCaretEnabled(true);
    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    pane->setCaretPosition(linkLine, linkCol);

    TAccessibleTextEdit accessible(pane);
    QString desc = accessible.text(QAccessible::Description);

    QVERIFY2(
        desc.contains(qsl("A magical sword")),
        qPrintable(
            qsl("Expected tooltip in Description but got: '%1'").arg(desc)));

    // Cleanup: disable caret mode
    mpHost->setCaretEnabled(false);
  }

  void test_Description_CaretNotOnLink() {
    injectData(qsl("Plain text with no links"));

    TMainConsole *console = mpHost->mpConsole;

    // Enable caret mode, caret at line 0 col 0 (plain text)
    mpHost->setCaretEnabled(true);
    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    pane->setCaretPosition(0, 0);

    TAccessibleTextEdit accessible(pane);
    QString desc = accessible.text(QAccessible::Description);

    QVERIFY(desc.isEmpty());

    // Cleanup
    mpHost->setCaretEnabled(false);
  }

  // =====================================================================
  // OSC 8 attributes() run-range / caret-cell isolation Tests
  // =====================================================================

  // Helper: compute the offset of a (line, column) position the same way the
  // accessible interface does it (one '\n' per line break).
  int offsetForPosition(TMainConsole *console, int line, int column) {
    int offset = 0;
    for (int i = 0; i < line; ++i) {
      offset += console->buffer.line(i).length() + 1;
    }
    return offset + column;
  }

  void test_Attributes_RunRangeCoversFullLink() {
    // A whole link run should be returned in a single attributes() call
    // (start..end describe the contiguous range with these attributes).
    injectData(qsl("\x1b]8;;https://example.com\x1b\\Click me\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;
    auto pos = findFirstLinkPositionWithId();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol, linkId] = *pos;

    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    TAccessibleTextEdit accessible(pane);

    const int offset = offsetForPosition(console, linkLine, linkCol);
    int startOffset = -1, endOffset = -1;
    QString attrs = accessible.attributes(offset, &startOffset, &endOffset);

    QVERIFY2(attrs.contains(qsl("text-link:")),
             qPrintable(qsl("Expected link attributes; got: %1").arg(attrs)));

    // The reported range must include the queried offset and stay within the
    // link span on this line.
    QVERIFY2(startOffset <= offset && offset < endOffset,
             qPrintable(qsl("Range [%1,%2) must include offset %3")
                            .arg(startOffset)
                            .arg(endOffset)
                            .arg(offset)));

    // Every column inside [startCol, endCol) on this line must belong to the
    // same linkId, otherwise the run-expansion crossed a boundary.
    const int lineStart = offsetForPosition(console, linkLine, 0);
    for (int o = startOffset; o < endOffset; ++o) {
      const int col = o - lineStart;
      QVERIFY2(col >= 0 && col < console->buffer.line(linkLine).length(),
               "run range should not cross a line boundary");
      QCOMPARE(console->buffer.getLinkIndexAt(linkLine, col), linkId);
    }
  }

  void test_Attributes_RunRangeStopsAtLinkBoundary() {
    // " " between "X" and the link must not be merged with the link's run.
    injectData(qsl("X \x1b]8;;https://example.com\x1b\\L\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;
    auto pos = findFirstLinkPositionWithId();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol, linkId] = *pos;
    QVERIFY2(linkCol >= 1, "Link should be preceded by plain text");

    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    TAccessibleTextEdit accessible(pane);

    // Query attributes at the plain-text character immediately before the link.
    const int plainOffset = offsetForPosition(console, linkLine, linkCol - 1);
    int s = -1, e = -1;
    QString plainAttrs = accessible.attributes(plainOffset, &s, &e);
    QVERIFY2(!plainAttrs.contains(qsl("text-link:")),
             "Plain text before link should not have link attributes");
    // The run from the plain-text query must end at or before the link starts.
    const int linkOffset = offsetForPosition(console, linkLine, linkCol);
    QVERIFY2(e <= linkOffset,
             qPrintable(qsl("Plain run [%1,%2) leaked into link starting at %3")
                            .arg(s)
                            .arg(e)
                            .arg(linkOffset)));
  }

  void test_Attributes_CaretCellIsolatedFromRun() {
    // The caret cell renders inverted; it must not be merged into a
    // neighbouring run even when they otherwise share styling.
    injectData(qsl("\x1b]8;;https://example.com\x1b\\Hello\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;
    auto pos = findFirstLinkPositionWithId();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol, linkId] = *pos;

    mpHost->setCaretEnabled(true);
    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    // Park the caret on the second character of the link.
    pane->setCaretPosition(linkLine, linkCol + 1);

    TAccessibleTextEdit accessible(pane);

    const int caretOffset = offsetForPosition(console, linkLine, linkCol + 1);
    int sCaret = -1, eCaret = -1;
    QString attrsCaret = accessible.attributes(caretOffset, &sCaret, &eCaret);
    QVERIFY2(attrsCaret.contains(qsl("text-link:")),
             "Caret-on-link should still report link attributes");
    // The caret cell's range must be just the caret cell, not the whole link.
    QCOMPARE(eCaret - sCaret, 1);

    // The character before the caret must form its own run that ends at the
    // caret cell (does not engulf it).
    const int beforeOffset = offsetForPosition(console, linkLine, linkCol);
    int sBefore = -1, eBefore = -1;
    accessible.attributes(beforeOffset, &sBefore, &eBefore);
    QVERIFY2(eBefore == caretOffset,
             qPrintable(qsl("Run before caret should end at caret offset %1, "
                            "got [%2,%3)")
                            .arg(caretOffset)
                            .arg(sBefore)
                            .arg(eBefore)));

    mpHost->setCaretEnabled(false);
  }

  // =====================================================================
  // OSC 8 link navigation key Tests
  // =====================================================================

  void test_FindLink_WrapForward() {
    // With only one link in the buffer, starting from after it we should wrap
    // to the start and rediscover the same link (and report wrapped == true).
    injectData(qsl("\x1b]8;;send:look\x1b\\Look\x1b]8;;\x1b\\ trailing text"));

    TMainConsole *console = mpHost->mpConsole;
    auto pos = findFirstLinkPositionWithId();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol, linkId] = *pos;
    Q_UNUSED(linkLine);
    Q_UNUSED(linkCol);

    // Scan forward starting at a position past the only link: the search must
    // fall through to the wrap-around branch and rediscover the same link.
    const int lastLine = console->buffer.getLastLineNumber();
    const int lastCol = console->buffer.line(lastLine).length() - 1;
    int outLine2 = -1, outCol2 = -1;
    bool wrapped2 = false;
    bool found2 = console->buffer.findNextLink(lastLine, lastCol, outLine2,
                                               outCol2, &wrapped2);
    QVERIFY2(found2, "findNextLink with wrap should rediscover the only link");
    QVERIFY2(wrapped2, "wrapped should be set to true when the search wrapped");
    QCOMPARE(console->buffer.getLinkIndexAt(outLine2, outCol2), linkId);
  }

  void test_FindLink_WrapBackward() {
    // Place the link on a later line so that a backward search from line 0
    // exhausts the pre-start range and falls through to the wrap path.
    injectData(qsl("plain leading line"));
    injectData(qsl("\x1b]8;;send:look\x1b\\Look\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;
    auto pos = findFirstLinkPositionWithId();
    QVERIFY2(pos.has_value(), "Expected to find a link in the buffer");
    auto [linkLine, linkCol, linkId] = *pos;
    QVERIFY2(linkLine > 0,
             "Test setup expects the link on a line after the leading text");
    Q_UNUSED(linkCol);

    // Searching backward from the very start of the buffer must wrap to find
    // the only link further down the buffer.
    int outLine = -1, outCol = -1;
    bool wrapped = false;
    bool found =
        console->buffer.findPreviousLink(0, 0, outLine, outCol, &wrapped);
    QVERIFY2(found,
             "findPreviousLink with wrap should rediscover the only link");
    QVERIFY2(wrapped,
             "wrapped should be set to true when backward search wrapped");
    QCOMPARE(outLine, linkLine);
    QCOMPARE(console->buffer.getLinkIndexAt(outLine, outCol), linkId);
  }

  void test_KeyNav_CtrlBracketRight_MovesToNextLink() {
    injectData(qsl("\x1b]8;;send:north\x1b\\North\x1b]8;;\x1b\\ - "
                   "\x1b]8;;send:south\x1b\\South\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;
    auto first = findNthDistinctLinkPosition(1);
    auto second = findNthDistinctLinkPosition(2);
    QVERIFY(first.has_value() && second.has_value());
    auto [firstLine, firstCol, firstLinkId] = *first;
    auto [secondLine, secondCol, secondLinkId] = *second;
    Q_UNUSED(firstLinkId);

    mpHost->setCaretEnabled(true);
    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    pane->setCaretPosition(firstLine, firstCol);

    QKeyEvent press(QEvent::KeyPress, Qt::Key_BracketRight,
                    Qt::ControlModifier);
    QApplication::sendEvent(pane, &press);

    QCOMPARE(pane->mCaretLine, secondLine);
    QCOMPARE(pane->mCaretColumn, secondCol);
    QCOMPARE(console->buffer.getFocusedLink(), secondLinkId);

    mpHost->setCaretEnabled(false);
  }

  void test_KeyNav_CtrlBracketLeft_MovesToPreviousLink() {
    injectData(qsl("\x1b]8;;send:north\x1b\\North\x1b]8;;\x1b\\ - "
                   "\x1b]8;;send:south\x1b\\South\x1b]8;;\x1b\\"));

    TMainConsole *console = mpHost->mpConsole;
    auto first = findNthDistinctLinkPosition(1);
    auto second = findNthDistinctLinkPosition(2);
    QVERIFY(first.has_value() && second.has_value());
    auto [firstLine, firstCol, firstLinkId] = *first;
    auto [secondLine, secondCol, secondLinkId] = *second;
    Q_UNUSED(secondLinkId);

    mpHost->setCaretEnabled(true);
    TTextEdit *pane = console->mUpperPane;
    QVERIFY(pane);
    pane->setCaretPosition(secondLine, secondCol);

    QKeyEvent press(QEvent::KeyPress, Qt::Key_BracketLeft, Qt::ControlModifier);
    QApplication::sendEvent(pane, &press);

    QCOMPARE(pane->mCaretLine, firstLine);
    QCOMPARE(pane->mCaretColumn, firstCol);
    QCOMPARE(console->buffer.getFocusedLink(), firstLinkId);

    mpHost->setCaretEnabled(false);
  }

  void cleanupTestCase() {
    delete mpServer;
    mpServer = nullptr;
    mpHost = nullptr;
    const QString path =
        mudlet::getMudletPath(enums::profileHomePath, mHostname);
    QDir(path).removeRecursively();
    delete mudlet::self();
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
