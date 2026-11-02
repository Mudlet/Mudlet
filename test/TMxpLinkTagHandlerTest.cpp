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

#include "GroupedTest.h"
#include "LuaLiteral.h"
#include "TMxpProcessor.h"
#include "TMxpStubClient.h"
#include "UntrustedText.h"

#include <QTest>

class TMxpLinkTagHandlerTest : public QObject {
  Q_OBJECT

  static void feed(TMxpStubClient &stub, const std::string &input) {
    TMxpProcessor processor(&stub);
    processor.setMode(MXP_MODE_CODE_LOCK_SECURE);
    std::string bytes = input;
    for (char &ch : bytes) {
      processor.processMxpInput(ch, true);
    }
  }

  static QString refusal(const QString &href) {
    return qsl("Mudlet does not open this kind of address: %1")
        .arg(UntrustedText::forTarget(href));
  }

  static QString opens(const QString &href) {
    return qsl("openUrl(%1)").arg(LuaLiteral::quote(href));
  }

  static QString connects(const QString &href) {
    return qsl("connectFromGameLink(%1)").arg(LuaLiteral::quote(href));
  }

private slots:
  // The tooltip, which a screen reader also reads, has to say a refused link
  // will not open, rather than show its address as if it would
  void testRefusedHrefSaysItWillNotOpen() {
    TMxpStubClient stub;
    feed(stub, "<A href=\"file:///etc/hosts\">hosts</A>");

    QCOMPARE(stub.mHrefs, QStringList(QString()));
    QCOMPARE(stub.mHints, QStringList(refusal(qsl("file:///etc/hosts"))));
  }

  // A game's own hint could claim the link opens something, so it gives way
  void testRefusedHrefReplacesTheGamesHint() {
    TMxpStubClient stub;
    feed(stub, "<A href=\"steam://run/1\" hint=\"Open the map\">map</A>");

    QCOMPARE(stub.mHrefs, QStringList(QString()));
    QCOMPARE(stub.mHints, QStringList(refusal(qsl("steam://run/1"))));
  }

  void testAllowedHrefKeepsTheGamesHint() {
    TMxpStubClient stub;
    feed(stub, "<A href=\"https://wiki.mudlet.org/\" hint=\"Visit the wiki\">wiki</A>");

    QCOMPARE(stub.mHrefs, QStringList(opens(qsl("https://wiki.mudlet.org/"))));
    QCOMPARE(stub.mHints, QStringList(qsl("Visit the wiki")));
  }

  // A bare A's address is only known at the end tag, and its tooltip must not
  // be left as the placeholder it was made with
  void testBareAShowsTheAddressItWraps() {
    TMxpStubClient stub;
    feed(stub, "<A>https://www.mudlet.org/</A>");

    QCOMPARE(stub.mHrefs, QStringList(opens(qsl("https://www.mudlet.org/"))));
    QCOMPARE(stub.mHints, QStringList(qsl("https://www.mudlet.org/")));
  }

  void testRefusedBareASaysItWillNotOpen() {
    TMxpStubClient stub;
    feed(stub, "<A>file:///etc/hosts</A>");

    QCOMPARE(stub.mHrefs, QStringList(QString()));
    QCOMPARE(stub.mHints, QStringList(refusal(qsl("file:///etc/hosts"))));
  }

  void testFilledAddressKeepsTheGamesHint() {
    TMxpStubClient stub;
    feed(stub, "<A href=\"https://wiki.example/&text;\" hint=\"Wiki page\">Page</A>");

    QCOMPARE(stub.mHrefs, QStringList(opens(qsl("https://wiki.example/Page"))));
    QCOMPARE(stub.mHints, QStringList(qsl("Wiki page")));
  }

  // The tooltip must show the mail address that will open, not the attachment the game asked for
  void testMailHintShowsTheAddressThatOpens() {
    TMxpStubClient stub;
    feed(stub, "<A href=\"mailto:x@example.invalid?attachment=/etc/hostname\">Mail</A>");

    QCOMPARE(stub.mHrefs, QStringList(opens(qsl("mailto:x@example.invalid"))));
    QCOMPARE(stub.mHints, QStringList(UntrustedText::forTarget(qsl("mailto:x@example.invalid"))));
  }

  void testBareMailAShowsTheAddressThatOpens() {
    TMxpStubClient stub;
    feed(stub, "<A>mailto:x@example.invalid?attachment=/etc/hostname</A>");

    QCOMPARE(stub.mHrefs, QStringList(opens(qsl("mailto:x@example.invalid"))));
    QCOMPARE(stub.mHints, QStringList(UntrustedText::forTarget(qsl("mailto:x@example.invalid"))));
  }

  void testTelnetHrefConnectsAfterAsking() {
    TMxpStubClient stub;
    feed(stub, "<A href=\"telnet://games.example:4000\">Visit</A>");

    QCOMPARE(stub.mHrefs, QStringList(connects(qsl("telnet://games.example:4000"))));
    QCOMPARE(stub.mHints, QStringList(qsl("Connect to: telnet://games.example:4000")));
  }

  void testTelnetsTextOfABareAConnectsAfterAsking() {
    TMxpStubClient stub;
    feed(stub, "<A>telnets://games.example:4000</A>");

    QCOMPARE(stub.mHrefs, QStringList(connects(qsl("telnets://games.example:4000"))));
    QCOMPARE(stub.mHints, QStringList(qsl("Connect to: telnets://games.example:4000")));
  }

  void testTelnetLinkKeepsTheGamesHint() {
    TMxpStubClient stub;
    feed(stub, "<A href=\"telnet://games.example:4000\" hint=\"Our sister game\">Visit</A>");

    QCOMPARE(stub.mHrefs, QStringList(connects(qsl("telnet://games.example:4000"))));
    QCOMPARE(stub.mHints, QStringList(qsl("Our sister game")));
  }

  // A link that could never connect says so, rather than asking nothing when clicked
  void testTelnetLinkWithoutAHostIsRefused() {
    TMxpStubClient stub;
    feed(stub, "<A href=\"telnet://:4000\">Visit</A>");

    QCOMPARE(stub.mHrefs, QStringList(QString()));
    QCOMPARE(stub.mHints, QStringList(qsl("Mudlet does not open this kind of address: telnet://:4000")));
  }
};

#include "TMxpLinkTagHandlerTest.moc"
MUDLET_GROUPED_TEST_MAIN(TMxpLinkTagHandlerTest)
