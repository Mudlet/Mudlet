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
      auto* skipBtn = mudlet::self()->mpConnectionDialog->findChild<QPushButton*>(qsl("skipToGamesButton"));
      if (skipBtn && skipBtn->isVisible()) {
          QTest::mouseClick(skipBtn, Qt::LeftButton);
          QTest::qWait(100);
      }
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
        << QString("\x1b]2;Window Title\x07Hello World")
        << qsl("Hello World") << true;
    QTest::newRow("ST-terminated OSC P (color redefine)")
        << QString("\x1b]P0FF0000\x1b\\Hello")
        << qsl("Hello") << true;
    QTest::newRow("BEL-terminated OSC 8 (hyperlink)")
        << QString("\x1b]8;;http://example.com\x07Link Text\x1b]8;;\x07 After Link")
        << qsl("Link Text After Link") << false;
    QTest::newRow("BEL-terminated OSC P")
        << QString("\x1b]P0FF0000\x07Hello")
        << qsl("Hello") << true;
    QTest::newRow("empty OSC sequence")
        << QString("\x1b]\x07Normal text")
        << qsl("Normal text") << true;
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
        << qsl("\x1b]8;;https://example.com/?id=42&lang=en\x1b\\Link\x1b]8;;\x1b\\")
        << QStringList{"id=42&lang=en"}
        << QStringList{};
    QTest::newRow("strips config param")
        << qsl("\x1b]8;;https://example.com/?config=%7B%22style%22%3A%7B%22color%22%3A%22red%22%7D%7D\x1b\\Styled\x1b]8;;\x1b\\")
        << QStringList{}
        << QStringList{"config="};
    QTest::newRow("strips preset, preserves other params")
        << qsl("\x1b]8;;https://example.com/?page=1&preset=danger\x1b\\Link\x1b]8;;\x1b\\")
        << QStringList{"page=1"}
        << QStringList{"preset="};
    QTest::newRow("strips preset with encoded equals")
        << qsl("\x1b]8;;https://example.com/?preset%3Ddefault&page=1\x1b\\Link\x1b]8;;\x1b\\")
        << QStringList{"page=1"}
        << QStringList{"preset%3D"};
    QTest::newRow("strips config with encoded equals (lowercase)")
        << qsl("\x1b]8;;https://example.com/?config%3dvalue&foo=bar\x1b\\Link\x1b]8;;\x1b\\")
        << QStringList{"foo=bar"}
        << QStringList{"config%3d"};
    QTest::newRow("preserves percent-encoded reserved names")
        << qsl("\x1b]8;;https://example.com/?%63%6F%6E%66%69%67=value\x1b\\Link\x1b]8;;\x1b\\")
        << QStringList{"%63%6F%6E%66%69%67=value"}
        << QStringList{};
    QTest::newRow("send URL strips all query params")
        << qsl("\x1b]8;;send:attack?config=%7B%22style%22%3A%7B%22color%22%3A%22red%22%7D%7D\x1b\\Attack\x1b]8;;\x1b\\")
        << QStringList{"attack"}
        << QStringList{"config="};
  }

  void test_Osc8UrlParams() {
    QFETCH(QString, message);
    QFETCH(QStringList, mustContain);
    QFETCH(QStringList, mustNotContain);

    injectData(message);

    QStringList commands = findFirstLinkCommands();
    QVERIFY2(!commands.isEmpty(), "No hyperlink found in buffer");
    for (const auto &expected : mustContain) {
      QVERIFY2(commands.first().contains(expected),
               qPrintable(qsl("Expected '%1' in: %2").arg(expected, commands.first())));
    }
    for (const auto &forbidden : mustNotContain) {
      QVERIFY2(!commands.first().contains(forbidden),
               qPrintable(qsl("Did not expect '%1' in: %2").arg(forbidden, commands.first())));
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
        << qsl(R"({"title":"Lonely Title"})")
        << qsl("Lonely Title") << false;
    QTest::newRow("menu without title")
        << qsl(R"({"menu":[{"North":"send:north"}]})")
        << QString() << false;
    QTest::newRow("empty string")
        << qsl(R"({"title":"","menu":[{"Action":"send:action"}]})")
        << QString() << false;
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
        << qsl(R"({"title":42,"menu":[{"Action":"send:action"}]})")
        << QString() << false;
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
        << qsl("Magic Shop - Potions")
        << true << false << false << false
        << qsl("#ffd700") << QString() << -1 << QString();
    QTest::newRow("italic and background")
        << qsl(R"({"title":{"text":"Sir Galahad the Brave","style":{"color":"#ffffff","bg":"#333333","italic":true}},"menu":[{"Talk":"send:talk galahad"}]})")
        << qsl("Sir Galahad the Brave")
        << false << true << false << false
        << qsl("#ffffff") << qsl("#333333") << -1 << QString();
    QTest::newRow("all text decorations")
        << qsl(R"({"title":{"text":"Decorated Title","style":{"color":"#ff0000","bold":true,"italic":true,"underline":true,"strikethrough":true}},"menu":[{"Action":"send:action"}]})")
        << qsl("Decorated Title")
        << true << true << true << true
        << qsl("#ff0000") << QString() << -1 << QString();
    QTest::newRow("wavy underline")
        << qsl(R"({"title":{"text":"Wavy Title","style":{"color":"#00ff00","underline":"wavy"}},"menu":[{"Test":"send:test"}]})")
        << qsl("Wavy Title")
        << false << false << true << false
        << qsl("#00ff00") << QString()
        << static_cast<int>(Mudlet::HyperlinkStyling::UnderlineWavy) << QString();
    QTest::newRow("dotted underline")
        << qsl(R"({"title":{"text":"Dotted Title","style":{"color":"#00ff00","underline":"dotted"}},"menu":[{"Test":"send:test"}]})")
        << qsl("Dotted Title")
        << false << false << true << false
        << qsl("#00ff00") << QString()
        << static_cast<int>(Mudlet::HyperlinkStyling::UnderlineDotted) << QString();
    QTest::newRow("dashed underline")
        << qsl(R"({"title":{"text":"Dashed Title","style":{"color":"#00ff00","underline":"dashed"}},"menu":[{"Test":"send:test"}]})")
        << qsl("Dashed Title")
        << false << false << true << false
        << qsl("#00ff00") << QString()
        << static_cast<int>(Mudlet::HyperlinkStyling::UnderlineDashed) << QString();
    QTest::newRow("background-color CSS property")
        << qsl(R"({"title":{"text":"CSS BG Title","style":{"color":"#ffffff","background-color":"#660000"}},"menu":[{"Action":"send:action"}]})")
        << qsl("CSS BG Title")
        << false << false << false << false
        << qsl("#ffffff") << qsl("#660000") << -1 << QString();
    QTest::newRow("text-decoration-color")
        << qsl(R"({"title":{"text":"Color Decoration","style":{"color":"#ffffff","underline":true,"text-decoration-color":"#ff00ff"}},"menu":[{"Action":"send:action"}]})")
        << qsl("Color Decoration")
        << false << false << true << false
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
               static_cast<Mudlet::HyperlinkStyling::UnderlineStyle>(underlineStyle));
    }
    if (!underlineColor.isEmpty()) {
      QVERIFY(styling.menuTitleStyle.hasUnderlineColor);
      QCOMPARE(styling.menuTitleStyle.underlineColor, QColor(underlineColor));
    }
  }

  void test_Osc8Title_LinkTextDisplayedInBuffer() {
    QString config = qsl(
        R"({"title":"Lamb and Barley Stew","menu":[{"View Details":"send:look stew"}]})");
    injectData(buildOsc8WithConfig(
        qsl("send:look stew"), qsl("[Lamb and Barley Stew]"), config));

    TMainConsole *console = mpHost->mpConsole;
    QString allText;
    for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
      allText += console->buffer.line(i);
    }
    QVERIFY2(
        allText.contains(qsl("[Lamb and Barley Stew]")),
        qPrintable(qsl("Expected link text in buffer but got '%1'")
                       .arg(allText)));
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
