/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "ircmessageformatter.h"

#include <IrcConnection>
#include <ircnetwork_p.h>
#include <QtTest/QtTest>

/*
 * Unit tests for IrcMessageFormatter, which turns what the IRC server says into
 * the two forms Mudlet shows it in: HTML for the IRC window, and plain text for
 * the sysIrcMessage event a script sees.
 *
 * The formatter is static and depends only on communi, so these tests need
 * neither a Host nor the Mudlet application stack.
 *
 * The window form carries the wall-clock time it was formatted at, so the cases
 * that need an exact string ask for the Lua form, which does not.
 */
class IrcMessageFormatterTest : public QObject
{
    Q_OBJECT

private:
    // Never opened: a connection is only wanted because communi decodes a
    // message against one, parents the message to it, and answers
    // IrcNoticeMessage::isPrivate() by comparing against its nickname
    IrcConnection mConnection;

    IrcMessage* fromRaw(const QByteArray& raw) { return IrcMessage::fromData(raw, &mConnection); }

    QString forLua(const QByteArray& raw) { return IrcMessageFormatter::formatMessage(fromRaw(raw), true); }

    QString forWindow(const QByteArray& raw) { return IrcMessageFormatter::formatMessage(fromRaw(raw), false); }

private slots:
    void initTestCase() { mConnection.setNickName(QStringLiteral("me")); }

    void join_namesWhoJoinedAndWhere() { QCOMPARE(forLua(":bob!u@h JOIN #mudlet"), QStringLiteral("! bob has joined #mudlet")); }

    void part_withoutAReason() { QCOMPARE(forLua(":bob!u@h PART #mudlet"), QStringLiteral("! bob has left #mudlet")); }

    void part_withAReason() { QCOMPARE(forLua(":bob!u@h PART #mudlet :getting dinner"), QStringLiteral("! bob has left #mudlet (getting dinner)")); }

    void quit_withoutAReason() { QCOMPARE(forLua(":bob!u@h QUIT"), QStringLiteral("! bob has quit")); }

    void quit_withAReason() { QCOMPARE(forLua(":bob!u@h QUIT :connection reset"), QStringLiteral("! bob has quit (connection reset)")); }

    void nick_namesBothTheOldAndTheNewNick() { QCOMPARE(forLua(":old!u@h NICK :new"), QStringLiteral("! old has changed nick to new")); }

    void mode_namesWhoSetWhatOnWhom() { QCOMPARE(forLua(":bob!u@h MODE #mudlet +o alice"), QStringLiteral("! bob sets mode #mudlet +o alice")); }

    void topic_changed() { QCOMPARE(forLua(":bob!u@h TOPIC #mudlet :a new topic"), QStringLiteral("! bob changed topic")); }

    void topic_cleared() { QCOMPARE(forLua(":bob!u@h TOPIC #mudlet :"), QStringLiteral("! bob cleared topic")); }

    void away_withAReason() { QCOMPARE(forLua(":bob!u@h AWAY :at lunch"), QStringLiteral("! bob is away (at lunch)")); }

    void away_withoutAReasonMeansBack() { QCOMPARE(forLua(":bob!u@h AWAY"), QStringLiteral("! bob is back")); }

    void invite_namesWhoWasInvitedWhere() { QCOMPARE(forLua(":bob!u@h INVITE alice #mudlet"), QStringLiteral("! bob invited to #mudlet")); }

    // A kick is the one thing that happens to a player without their asking, so
    // silently dropping it leaves them looking at a channel they are no longer in.
    // With no reason given there are no empty parentheses either.
    void kick_namesWhoWasKickedByWhom() { QCOMPARE(forLua(":bob!u@h KICK #mudlet alice"), QStringLiteral("! bob kicked alice from #mudlet")); }

    // The reason is the one part of a kick a player cannot work out for themselves
    void kick_withAReason() { QCOMPARE(forLua(":bob!u@h KICK #mudlet alice :behave"), QStringLiteral("! bob kicked alice from #mudlet (behave)")); }

    // The formatter does not special-case a kick of this connection; the copy
    // dlgIRC puts in the server tab relies on that, since the line it copies is
    // the same one the channel tab got
    void kick_ofThisConnectionIsWordedNoDifferently() { QCOMPARE(forLua(":bob!u@h KICK #mudlet me :get out"), QStringLiteral("! bob kicked me from #mudlet (get out)")); }

    // Escaping the fields must not cost the window the line itself, and a kick
    // with no reason must not render the empty parentheses the other branch avoids
    void kick_readsTheSameInTheWindowAsItDoesForLua()
    {
        const QString html = forWindow(":bob!u@h KICK #mudlet alice :behave");
        QVERIFY2(html.contains(QStringLiteral("! bob kicked alice from #mudlet (behave)")), qPrintable(html));

        const QString reasonless = forWindow(":bob!u@h KICK #mudlet alice");
        QVERIFY2(reasonless.contains(QStringLiteral("! bob kicked alice from #mudlet")), qPrintable(reasonless));
        QVERIFY2(!reasonless.contains(QStringLiteral("()")), qPrintable(reasonless));
    }

    void privateMessage_forLuaIsTheTextAlone() { QCOMPARE(forLua(":bob!u@h PRIVMSG #mudlet :hello there"), QStringLiteral("hello there")); }

    void privateMessage_forTheWindowCarriesTheNickInBold()
    {
        const QString html = forWindow(":bob!u@h PRIVMSG #mudlet :hello there");
        QVERIFY2(html.contains(QStringLiteral("<b>&lt;bob&gt;</b> hello there")), qPrintable(html));
    }

    // A CTCP ACTION is what /me sends, and reads as narration rather than speech
    void privateMessage_actionReadsAsNarration() { QCOMPARE(forLua(":bob!u@h PRIVMSG #mudlet :\001ACTION waves\001"), QStringLiteral("* bob waves")); }

    void notice_toTheChannelIsTheTextAloneForLua() { QCOMPARE(forLua(":bob!u@h NOTICE #mudlet :heads up"), QStringLiteral("heads up")); }

    void notice_toTheChannelNamesTheSenderAndTargetForTheWindow()
    {
        const QString html = forWindow(":bob!u@h NOTICE #mudlet :heads up");
        QVERIFY2(html.contains(QStringLiteral("&lt;bob&gt; [#mudlet] heads up")), qPrintable(html));
    }

    // A notice addressed to this connection's own nickname is a private one, and
    // is marked as such in both forms so it is not mistaken for channel traffic
    void notice_toThisConnectionIsMarkedPrivate() { QCOMPARE(forLua(":bob!u@h NOTICE me :just for you"), QStringLiteral("[bob] just for you")); }

    void notice_ctcpVersionReplyIsReportedAsAVersion() { QCOMPARE(forLua(":bob!u@h NOTICE me :\001VERSION Mudlet 5.0\001"), QStringLiteral("! bob version is Mudlet 5.0")); }

    void notice_ctcpTimeReplyIsReportedAsATime() { QCOMPARE(forLua(":bob!u@h NOTICE me :\001TIME Tue Jan 1 00:00:00 2030\001"), QStringLiteral("! bob time is Tue Jan 1 00:00:00 2030")); }

    void numeric_belowThreeHundredIsInformation() { QCOMPARE(forLua(":server 001 me :Welcome to the network"), QStringLiteral("[INFO] Welcome to the network")); }

    // Everything the server sends is put into an HTML document, so the markup
    // characters in it have to be escaped on the way - in every field of every
    // line, not just the ones that are obviously free text. The window links
    // anchors through QDesktopServices::openUrl(), so markup that got through
    // could plant a link to anything. <u> is used because the formatter adds
    // <b> of its own around a channel message's sender.
    void window_escapesMarkupInEveryField_data()
    {
        QTest::addColumn<QByteArray>("raw");

        QTest::newRow("information numeric") << QByteArrayLiteral(":server 001 me :<u>not underlined</u>");
        QTest::newRow("kick reason") << QByteArrayLiteral(":bob!u@h KICK #mudlet alice :<u>not underlined</u>");
        // a channel name may hold anything but NUL, BEL, CR, LF, space, comma
        // and colon, so it is markup a channel operator can choose
        QTest::newRow("kick channel") << QByteArrayLiteral(":bob!u@h KICK #<u>underlined</u> alice :hi");
        QTest::newRow("part reason") << QByteArrayLiteral(":bob!u@h PART #mudlet :<u>not underlined</u>");
        QTest::newRow("quit reason") << QByteArrayLiteral(":bob!u@h QUIT :<u>not underlined</u>");
        QTest::newRow("join channel") << QByteArrayLiteral(":bob!u@h JOIN #<u>underlined</u>");
        QTest::newRow("own join channel") << QByteArrayLiteral(":me!u@h JOIN #<u>underlined</u>");
        QTest::newRow("part channel") << QByteArrayLiteral(":bob!u@h PART #<u>underlined</u>");
        QTest::newRow("part channel with a reason") << QByteArrayLiteral(":bob!u@h PART #<u>underlined</u> :bye");
        QTest::newRow("invite channel") << QByteArrayLiteral(":bob!u@h INVITE me #<u>underlined</u>");
        QTest::newRow("mode target") << QByteArrayLiteral(":bob!u@h MODE #<u>underlined</u> +o alice");
        // a ban mask is whatever the channel operator typed
        QTest::newRow("mode argument") << QByteArrayLiteral(":bob!u@h MODE #mudlet +b <u>underlined</u>!*@*");
        QTest::newRow("notice target") << QByteArrayLiteral(":bob!u@h NOTICE #<u>underlined</u> :hi");
        // the text of a CTCP reply is whatever the other user's client says
        QTest::newRow("CTCP VERSION reply") << QByteArrayLiteral(":bob!u@h NOTICE me :\001VERSION <u>not underlined</u>\001");
        QTest::newRow("CTCP TIME reply") << QByteArrayLiteral(":bob!u@h NOTICE me :\001TIME <u>not underlined</u>\001");
        QTest::newRow("version numeric") << QByteArrayLiteral(":server 351 me <u>not underlined</u> irc.example.org :comments");
        QTest::newRow("time numeric") << QByteArrayLiteral(":server 391 me irc.example.org :<u>not underlined</u>");
        QTest::newRow("error") << QByteArrayLiteral("ERROR :<u>not underlined</u>");
        QTest::newRow("unknown command") << QByteArrayLiteral(":bob!u@h FROBNICATE :<u>not underlined</u>");
        // nick rules are the server's to enforce, and it is the server that is not trusted
        QTest::newRow("new nick") << QByteArrayLiteral(":old!u@h NICK :<u>underlined</u>");
        QTest::newRow("channel message sender") << QByteArrayLiteral(":<u>underlined</u>!u@h PRIVMSG #mudlet :hi");
        QTest::newRow("away nick") << QByteArrayLiteral(":<u>underlined</u>!u@h AWAY :brb");
        QTest::newRow("kicked nick") << QByteArrayLiteral(":bob!u@h KICK #mudlet <u>underlined</u> :bye");
        QTest::newRow("quitting nick") << QByteArrayLiteral(":<u>underlined</u>!u@h QUIT");
        QTest::newRow("topic setter") << QByteArrayLiteral(":<u>underlined</u>!u@h TOPIC #mudlet :new");
        QTest::newRow("pong sender") << QByteArrayLiteral(":<u>underlined</u>!u@h PONG me :1");
        QTest::newRow("inviter") << QByteArrayLiteral(":<u>underlined</u>!u@h INVITE me #mudlet");
        QTest::newRow("joining nick") << QByteArrayLiteral(":<u>underlined</u>!u@h JOIN #mudlet");
        QTest::newRow("kicker") << QByteArrayLiteral(":<u>underlined</u>!u@h KICK #mudlet alice :bye");
        QTest::newRow("mode") << QByteArrayLiteral(":bob!u@h MODE #mudlet +<u>underlined</u>");
        QTest::newRow("mode setter") << QByteArrayLiteral(":<u>underlined</u>!u@h MODE #mudlet +o alice");
        QTest::newRow("old nick") << QByteArrayLiteral(":<u>underlined</u>!u@h NICK :new");
        QTest::newRow("notice sender") << QByteArrayLiteral(":<u>underlined</u>!u@h NOTICE #mudlet :hi");
        QTest::newRow("CTCP VERSION replier") << QByteArrayLiteral(":<u>underlined</u>!u@h NOTICE me :\001VERSION Mudlet\001");
        QTest::newRow("version numeric sender") << QByteArrayLiteral(":<u>underlined</u> 351 me 1.0 irc.example.org :comments");
        QTest::newRow("time numeric server") << QByteArrayLiteral(":server 391 me <u>underlined</u> :Tue Jan 1 00:00:00 2030");
        QTest::newRow("parting nick") << QByteArrayLiteral(":<u>underlined</u>!u@h PART #mudlet");
        QTest::newRow("quitting nick with a reason") << QByteArrayLiteral(":<u>underlined</u>!u@h QUIT :bye");
        QTest::newRow("topic clearer") << QByteArrayLiteral(":<u>underlined</u>!u@h TOPIC #mudlet :");
        QTest::newRow("unknown command sender") << QByteArrayLiteral(":<u>underlined</u>!u@h FROBNICATE :hi");
        QTest::newRow("unknown command") << QByteArrayLiteral(":bob!u@h FROB<u>underlined</u> :hi");
    }

    // Names are escaped rather than formatted, so a channel that looks like a
    // web address is not made into a link to it
    void window_leavesANameUnlinked()
    {
        const QString join = forWindow(":bob!u@h JOIN #www.example.org");
        QVERIFY2(join.contains(QStringLiteral("! bob has joined #www.example.org")) && !join.contains(QStringLiteral("href")), qPrintable(join));
        const QString kick = forWindow(":bob!u@h KICK #www.example.org alice");
        QVERIFY2(!kick.contains(QStringLiteral("href")), qPrintable(kick));
    }

    void window_escapesMarkupInEveryField()
    {
        QFETCH(QByteArray, raw);
        const QString html = forWindow(raw);
        QVERIFY2(!html.contains(QStringLiteral("<u>")), qPrintable(html));
        QVERIFY2(html.contains(QStringLiteral("&lt;u")), qPrintable(html));
    }

    // The composed replies carry the fields any user fills in about themselves,
    // and the nick, ident and host every line of them repeats. Each row marks up
    // one field only, so a field left unescaped cannot hide behind another.
    void window_escapesMarkupInEachFieldOfAComposedReply_data()
    {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<QString>("prefix");
        QTest::addColumn<QStringList>("parameters");

        const QString markup = QStringLiteral("<u>x</u>");
        const QString prefix = QStringLiteral("bob!ident@example.org");
        // realName, server, info, account, address, connected since, idle, secure, channels, away reason
        const QStringList whois{QStringLiteral("Bob"),
                                QStringLiteral("irc.example.org"),
                                QStringLiteral("Example Network"),
                                QStringLiteral("bob"),
                                QStringLiteral("192.0.2.1"),
                                QStringLiteral("0"),
                                QStringLiteral("5"),
                                QString(),
                                QStringLiteral("#mudlet"),
                                QStringLiteral("brb")};
        const QStringList whowas = whois.mid(0, 4);
        const QStringList who{QStringLiteral("ident@example.org"), QStringLiteral("irc.example.org"), QStringLiteral("H"), QStringLiteral("Bob")};

        QTest::newRow("WHOIS nick") << "WHOIS" << QStringLiteral("%1!ident@example.org").arg(markup) << whois;
        QTest::newRow("WHOIS ident") << "WHOIS" << QStringLiteral("bob!%1@example.org").arg(markup) << whois;
        QTest::newRow("WHOIS host") << "WHOIS" << QStringLiteral("bob!ident@%1").arg(markup) << whois;
        const QStringList whoisFields{QStringLiteral("real name"), QStringLiteral("server"), QStringLiteral("server info"), QStringLiteral("account"), QStringLiteral("address")};
        for (int i = 0; i < whoisFields.size(); ++i) {
            QStringList parameters = whois;
            parameters[i] = markup;
            QTest::addRow("WHOIS %s", qPrintable(whoisFields.at(i))) << "WHOIS" << prefix << parameters;
        }
        QStringList parameters = whois;
        parameters[8] = QStringLiteral("#") + markup;
        QTest::newRow("WHOIS channels") << "WHOIS" << prefix << parameters;
        parameters = whois;
        parameters[9] = markup;
        QTest::newRow("WHOIS away reason") << "WHOIS" << prefix << parameters;

        QTest::newRow("WHOWAS nick") << "WHOWAS" << QStringLiteral("%1!ident@example.org").arg(markup) << whowas;
        QTest::newRow("WHOWAS ident") << "WHOWAS" << QStringLiteral("bob!%1@example.org").arg(markup) << whowas;
        QTest::newRow("WHOWAS host") << "WHOWAS" << QStringLiteral("bob!ident@%1").arg(markup) << whowas;
        for (int i = 0; i < whowas.size(); ++i) {
            parameters = whowas;
            parameters[i] = markup;
            QTest::addRow("WHOWAS %s", qPrintable(whoisFields.at(i))) << "WHOWAS" << prefix << parameters;
        }

        QTest::newRow("WHO nick") << "WHO" << QStringLiteral("%1!ident@example.org").arg(markup) << who;
        parameters = who;
        parameters[3] = markup;
        QTest::newRow("WHO real name") << "WHO" << prefix << parameters;

        QTest::newRow("NAMES channel") << "NAMES" << QString() << QStringList{QStringLiteral("#") + markup, QStringLiteral("alice")};
    }

    void window_escapesMarkupInEachFieldOfAComposedReply()
    {
        QFETCH(QString, kind);
        QFETCH(QString, prefix);
        QFETCH(QStringList, parameters);

        IrcMessage* message = nullptr;
        if (kind == QLatin1String("WHOIS")) {
            message = new IrcWhoisMessage(&mConnection);
        } else if (kind == QLatin1String("WHOWAS")) {
            message = new IrcWhowasMessage(&mConnection);
        } else if (kind == QLatin1String("WHO")) {
            message = new IrcWhoReplyMessage(&mConnection);
        } else {
            message = new IrcNamesMessage(&mConnection);
        }
        message->setPrefix(prefix);
        message->setParameters(parameters);
        const QString html = IrcMessageFormatter::formatMessage(message, false);
        QVERIFY2(!html.contains(QStringLiteral("<u>")), qPrintable(html));
        QVERIFY2(html.contains(QStringLiteral("&lt;u")), qPrintable(html));
    }

    // The 341 reply confirming an invitation this connection sent names whoever
    // was invited, as the server spelled them
    void window_escapesMarkupInAnInvitationReply()
    {
        auto* message = new IrcInviteMessage(&mConnection);
        message->setCommand(QString::number(Irc::RPL_INVITING));
        message->setParameters({QStringLiteral("<u>underlined</u>"), QStringLiteral("#mudlet")});
        const QString html = IrcMessageFormatter::formatMessage(message, false);
        QVERIFY2(!html.contains(QStringLiteral("<u>")), qPrintable(html));
        QVERIFY2(html.contains(QStringLiteral("&lt;u")), qPrintable(html));
    }

    // A server picks which characters address a notice to the channel's
    // operators or voiced users alone, and so what the prefix can be
    void window_escapesMarkupInANoticeStatusPrefix()
    {
        IrcNetworkPrivate* network = IrcNetworkPrivate::get(mConnection.network());
        network->setInfo({{QStringLiteral("STATUSMSG"), QStringLiteral("<u>")}});
        const QString html = forWindow(":bob!u@h NOTICE <u>#mudlet :hi");
        network->setInfo({{QStringLiteral("STATUSMSG"), QString()}});
        QVERIFY2(!html.contains(QStringLiteral("<u>")), qPrintable(html));
        QVERIFY2(html.contains(QStringLiteral(":&lt;u&gt;")), qPrintable(html));
    }

    // Anyone in a channel can type a URL of any scheme and have it made a link,
    // so only one to a web page may be opened when it is clicked
    void link_opensInABrowserOnlyForAWebPage_data()
    {
        QTest::addColumn<QString>("link");
        QTest::addColumn<bool>("opens");

        QTest::newRow("http") << QStringLiteral("http://www.example.org/") << true;
        QTest::newRow("https") << QStringLiteral("https://example.org/a?b=c") << true;
        QTest::newRow("upper case scheme") << QStringLiteral("HTTPS://example.org/") << true;
        QTest::newRow("local file") << QStringLiteral("file:///C:/x.exe") << false;
        QTest::newRow("file on a share") << QStringLiteral("file://attacker/share/x.exe") << false;
        QTest::newRow("SMB share") << QStringLiteral("smb://attacker/share") << false;
        QTest::newRow("settings app") << QStringLiteral("ms-settings:privacy") << false;
        QTest::newRow("script") << QStringLiteral("javascript:alert(1)") << false;
        QTest::newRow("e-mail") << QStringLiteral("mailto:bob@example.org") << false;
        QTest::newRow("no scheme") << QStringLiteral("www.example.org") << false;
        QTest::newRow("web scheme with no host") << QStringLiteral("http:x.exe") << false;
    }

    void link_opensInABrowserOnlyForAWebPage()
    {
        QFETCH(QString, link);
        QFETCH(bool, opens);
        QCOMPARE(IrcMessageFormatter::linkOpensInBrowser(QUrl(link)), opens);
    }

    // What the window makes a link of is what reaches the check above
    void link_madeOfWhatAnyoneTypesKeepsItsScheme()
    {
        const QString html = forWindow(":bob!u@h PRIVMSG #mudlet :see file:///C:/x.exe");
        QVERIFY2(html.contains(QStringLiteral("href='file:///C:/x.exe'")), qPrintable(html));
    }

    // communi escapes & and < for the window before it strips the formatting
    // codes, and the plain text it hands back for a script still has those
    // entities in it - so they have to be undone, on every path that reaches Lua
    void lua_getsTheTextAsItWasSent_data()
    {
        QTest::addColumn<QByteArray>("raw");
        QTest::addColumn<QString>("expected");

        QTest::newRow("information numeric") << QByteArrayLiteral(":server 002 me :Fish & Chips <here>") << QStringLiteral("[INFO] Fish & Chips <here>");
        QTest::newRow("error numeric") << QByteArrayLiteral(":server 401 me nick :Fish & Chips <here>") << QStringLiteral("[ERROR] nick Fish & Chips <here>");
        QTest::newRow("other numeric") << QByteArrayLiteral(":server 333 me #mudlet :Fish & Chips <here>") << QStringLiteral("[333] #mudlet Fish & Chips <here>");
        QTest::newRow("channel message") << QByteArrayLiteral(":bob!u@h PRIVMSG #mudlet :Fish & Chips <here>") << QStringLiteral("Fish & Chips <here>");
        QTest::newRow("action") << QByteArrayLiteral(":bob!u@h PRIVMSG #mudlet :\001ACTION likes Fish & Chips <here>\001") << QStringLiteral("* bob likes Fish & Chips <here>");
        QTest::newRow("notice") << QByteArrayLiteral(":bob!u@h NOTICE #mudlet :Fish & Chips <here>") << QStringLiteral("Fish & Chips <here>");
        QTest::newRow("kick reason") << QByteArrayLiteral(":bob!u@h KICK #mudlet alice :Fish & Chips <here>") << QStringLiteral("! bob kicked alice from #mudlet (Fish & Chips <here>)");
        // the channel is escaped for the window too, so it has to come back
        // unescaped here - a script addresses the channel it is given
        QTest::newRow("kick channel") << QByteArrayLiteral(":bob!u@h KICK #a&b<c> alice :hi") << QStringLiteral("! bob kicked alice from #a&b<c> (hi)");
        QTest::newRow("part reason") << QByteArrayLiteral(":bob!u@h PART #mudlet :Fish & Chips <here>") << QStringLiteral("! bob has left #mudlet (Fish & Chips <here>)");
        QTest::newRow("quit reason") << QByteArrayLiteral(":bob!u@h QUIT :Fish & Chips <here>") << QStringLiteral("! bob has quit (Fish & Chips <here>)");
        QTest::newRow("join channel") << QByteArrayLiteral(":bob!u@h JOIN #a&b<c>") << QStringLiteral("! bob has joined #a&b<c>");
        // a name is given as the server spelled it, formatting codes and all,
        // so that it still matches the channel a script is told the line is for
        QTest::newRow("join channel with formatting codes") << QByteArrayLiteral(":bob!u@h JOIN #\002a\002") << QStringLiteral("! bob has joined #\002a\002");
        QTest::newRow("CTCP VERSION reply") << QByteArrayLiteral(":bob!u@h NOTICE me :\001VERSION Fish & Chips <here>\001") << QStringLiteral("! bob version is Fish & Chips <here>");
        QTest::newRow("ban mask") << QByteArrayLiteral(":bob!u@h MODE #mudlet +b a&b<c>!*@*") << QStringLiteral("! bob sets mode #mudlet +b a&b<c>!*@*");
        QTest::newRow("error") << QByteArrayLiteral("ERROR :Fish & Chips <here>") << QStringLiteral("[ERROR] Fish & Chips <here>");
        // Somebody typing an entity by hand must see it come out as typed
        QTest::newRow("a literal entity survives") << QByteArrayLiteral(":bob!u@h PRIVMSG #mudlet :&amp; &lt; &amp;lt;") << QStringLiteral("&amp; &lt; &amp;lt;");
        // The formatting codes are still stripped, which is why the plain text
        // comes from communi in the first place
        QTest::newRow("formatting codes are still stripped") << QByteArrayLiteral(":bob!u@h PRIVMSG #mudlet :\002Fish\002 & \00304Chips\003 <here>") << QStringLiteral("Fish & Chips <here>");
        QTest::newRow("formatting codes are stripped from a part reason too")
                << QByteArrayLiteral(":bob!u@h PART #mudlet :\002Fish\002 & \00304Chips\003 <here>") << QStringLiteral("! bob has left #mudlet (Fish & Chips <here>)");
    }

    void lua_getsTheTextAsItWasSent()
    {
        QFETCH(QByteArray, raw);
        QFETCH(QString, expected);
        QCOMPARE(forLua(raw), expected);
    }

    void lua_getsComposedRepliesAsSent()
    {
        auto* who = new IrcWhoReplyMessage(&mConnection);
        who->setPrefix(QStringLiteral("bob!ident@example.org"));
        who->setParameters({QStringLiteral("ident@example.org"), QStringLiteral("irc.example.org"), QStringLiteral("H"), QStringLiteral("Fish & Chips <here>")});
        QCOMPARE(IrcMessageFormatter::formatMessage(who, true), QStringLiteral("[WHO] bob (Fish & Chips <here>)"));

        auto* whois = new IrcWhoisMessage(&mConnection);
        whois->setPrefix(QStringLiteral("bob!ident@example.org"));
        whois->setParameters({QStringLiteral("Fish & Chips <here>"), QStringLiteral("irc.example.org"), QStringLiteral("Example Network")});
        const QString text = IrcMessageFormatter::formatMessage(whois, true);
        QVERIFY2(text.contains(QStringLiteral("[WHOIS] bob is ident@example.org (Fish & Chips <here>)")), qPrintable(text));
    }

    // The same characters still have to be escaped on their way into the window
    void window_stillEscapesWhatLuaGetsRaw()
    {
        const QString html = forWindow(":bob!u@h PRIVMSG #mudlet :Fish & Chips <here>");
        QVERIFY2(html.contains(QStringLiteral("Fish &amp; Chips &lt;here>")), qPrintable(html));
    }

    void numeric_errorCodesAreMarkedAsErrors() { QCOMPARE(forLua(":server 401 me nosuchnick :No such nick"), QStringLiteral("[ERROR] nosuchnick No such nick")); }

    // The error colour has to be picked from the numeric's own code, since a
    // numeric error arrives as an ordinary numeric rather than an ERROR command
    void numeric_errorCodesAreColouredAsErrorsInTheWindow()
    {
        const QString html = forWindow(":server 401 me nosuchnick :No such nick");
        QVERIFY2(html.contains(QStringLiteral("indianred")), qPrintable(html));
    }

    void numeric_channelUrlIsLabelled() { QCOMPARE(forLua(":server 328 me #mudlet :https://www.mudlet.org/"), QStringLiteral("[Channel URL] #mudlet https://www.mudlet.org/")); }

    void numeric_anythingElseFallsBackToItsOwnCode() { QCOMPARE(forLua(":server 333 me #mudlet bob :1234567890"), QStringLiteral("[333] #mudlet bob 1234567890")); }

    void error_isMarkedAsAnError() { QCOMPARE(forLua("ERROR :Closing link"), QStringLiteral("[ERROR] Closing link")); }

    void unknown_isShownVerbatimRatherThanDropped() { QCOMPARE(forLua(":bob!u@h FROBNICATE one two"), QStringLiteral("? bob FROBNICATE one two")); }

    void pong_reportsHowLongTheReplyTook()
    {
        const QString text = forLua(":server PONG server :1234");
        QVERIFY2(text.contains(QStringLiteral("replied in")), qPrintable(text));
    }

    // The composed message types are assembled by communi out of several
    // numerics, so they are built here rather than parsed from one line
    void names_countsTheUsersForTheWindowAndListsThemForLua()
    {
        auto* message = new IrcNamesMessage(&mConnection);
        message->setParameters({QStringLiteral("#mudlet"), QStringLiteral("alice"), QStringLiteral("bob")});
        QCOMPARE(IrcMessageFormatter::formatMessage(message, true), QStringLiteral("! #mudlet has 2 users: alice bob"));
        QVERIFY2(IrcMessageFormatter::formatMessage(message, false).contains(QStringLiteral("! #mudlet has 2 users")), "the window form should carry the count");
    }

    void motd_putsEveryLineOnItsOwn()
    {
        auto* message = new IrcMotdMessage(&mConnection);
        message->setParameters({QStringLiteral("me"), QStringLiteral("first line"), QStringLiteral("second line")});
        QCOMPARE(IrcMessageFormatter::formatMessage(message, true), QStringLiteral("[MOTD] first line\n[MOTD] second line\n"));
    }

    // A topic reply is composed by communi from the 332 numeric, so it has to
    // be built by hand here
    void topic_replyReachesLuaAsItWasSent()
    {
        auto* message = new IrcTopicMessage(&mConnection);
        message->setCommand(QString::number(Irc::RPL_TOPIC));
        message->setParameters({QStringLiteral("#mudlet"), QStringLiteral("Fish & Chips <here>")});
        QCOMPARE(IrcMessageFormatter::formatMessage(message, true), QStringLiteral("[TOPIC] Fish & Chips <here>"));
    }

    void motd_reachesLuaAsItWasSent()
    {
        auto* message = new IrcMotdMessage(&mConnection);
        message->setParameters({QStringLiteral("me"), QStringLiteral("Fish & Chips <here>")});
        QCOMPARE(IrcMessageFormatter::formatMessage(message, true), QStringLiteral("[MOTD] Fish & Chips <here>\n"));
    }

    void motd_breaksItsLinesWithMarkupForTheWindow()
    {
        auto* message = new IrcMotdMessage(&mConnection);
        message->setParameters({QStringLiteral("me"), QStringLiteral("first line"), QStringLiteral("second line")});
        QVERIFY2(IrcMessageFormatter::formatMessage(message, false).contains(QStringLiteral("<br />")), "the window form should break its lines");
    }

    void whois_reportsTheFieldsTheServerFilledIn()
    {
        auto* message = new IrcWhoisMessage(&mConnection);
        message->setPrefix(QStringLiteral("bob!ident@example.org"));
        // realName, server, info, account, address, connected since, idle, secure, channels
        message->setParameters({QStringLiteral("Bob Smith"),
                                QStringLiteral("irc.example.org"),
                                QStringLiteral("Example Network"),
                                QStringLiteral("bobaccount"),
                                QStringLiteral("192.0.2.1"),
                                QStringLiteral("0"),
                                QStringLiteral("3661"),
                                QStringLiteral("secure"),
                                QStringLiteral("#mudlet #other")});
        const QString text = IrcMessageFormatter::formatMessage(message, true);
        QVERIFY2(text.contains(QStringLiteral("[WHOIS] bob is ident@example.org (Bob Smith)")), qPrintable(text));
        QVERIFY2(text.contains(QStringLiteral("[WHOIS] bob is logged in as bobaccount")), qPrintable(text));
        QVERIFY2(text.contains(QStringLiteral("[WHOIS] bob is connected from 192.0.2.1")), qPrintable(text));
        QVERIFY2(text.contains(QStringLiteral("[WHOIS] bob is using a secure connection")), qPrintable(text));
        QVERIFY2(text.contains(QStringLiteral("[WHOIS] bob is on #mudlet #other")), qPrintable(text));
        QVERIFY2(text.contains(QStringLiteral("idle 1 hours 1 mins 1 secs")), qPrintable(text));
    }

    // Everything the server did not fill in is left out rather than shown empty
    void whois_leavesOutTheFieldsTheServerDidNotFillIn()
    {
        auto* message = new IrcWhoisMessage(&mConnection);
        message->setPrefix(QStringLiteral("bob!ident@example.org"));
        message->setParameters({QStringLiteral("Bob Smith"), QStringLiteral("irc.example.org"), QStringLiteral("Example Network")});
        const QString text = IrcMessageFormatter::formatMessage(message, true);
        QVERIFY2(!text.contains(QStringLiteral("logged in as")), qPrintable(text));
        QVERIFY2(!text.contains(QStringLiteral("secure connection")), qPrintable(text));
        QVERIFY2(!text.contains(QStringLiteral("is on")), qPrintable(text));
    }

    // The slots communi lays a WHOIS out in, with only what RPL_WHOISIDLE
    // (317) fills in - the sign-on time and the idle seconds - left to the case
    static QStringList whoisParameters(const QString& signOn, const QString& idle)
    {
        return {QStringLiteral("Bob Smith"), QStringLiteral("irc.example.org"), QStringLiteral("Example Network"), QString(), QString(), signOn, idle, QString(), QString()};
    }

    // A server need not send 317 at all, and communi then reports a sign-on
    // at the epoch and an idle time of none, neither of which is so
    void whois_withoutAnIdleReplySaysNothingAboutIdleTime()
    {
        auto* message = new IrcWhoisMessage(&mConnection);
        message->setPrefix(QStringLiteral("bob!ident@example.org"));
        message->setParameters(whoisParameters(QString(), QString()));
        const QString text = IrcMessageFormatter::formatMessage(message, true);
        QVERIFY2(!text.contains(QStringLiteral("idle")), qPrintable(text));
        QVERIFY2(!text.contains(QStringLiteral("connected since")), qPrintable(text));
        QCOMPARE(text.count(QLatin1Char('\n')), 1);
    }

    // RFC 1459's 317 carries only the idle seconds - "bob 42 :seconds idle" -
    // so what lands in the sign-on slot is that trailing text, not a time
    void whois_idleReplyWithoutASignOnTimeShowsOnlyTheIdleTime()
    {
        auto* message = new IrcWhoisMessage(&mConnection);
        message->setPrefix(QStringLiteral("bob!ident@example.org"));
        message->setParameters(whoisParameters(QStringLiteral("seconds idle"), QStringLiteral("42")));
        const QString text = IrcMessageFormatter::formatMessage(message, true);
        QVERIFY2(text.contains(QStringLiteral("[WHOIS] bob has been idle 42 secs")), qPrintable(text));
        QVERIFY2(!text.contains(QStringLiteral("connected since")), qPrintable(text));
    }

    void whois_idleReplyWithASignOnTimeShowsBoth()
    {
        auto* message = new IrcWhoisMessage(&mConnection);
        message->setPrefix(QStringLiteral("bob!ident@example.org"));
        message->setParameters(whoisParameters(QStringLiteral("1700000000"), QStringLiteral("0")));
        const QString text = IrcMessageFormatter::formatMessage(message, true);
        const QString since = QDateTime::fromSecsSinceEpoch(1700000000).toString();
        QVERIFY2(text.contains(QStringLiteral("[WHOIS] bob is connected since %1 (idle 0 secs)").arg(since)), qPrintable(text));
    }

    void whowas_reportsWhoTheyWere()
    {
        auto* message = new IrcWhowasMessage(&mConnection);
        message->setPrefix(QStringLiteral("bob!ident@example.org"));
        message->setParameters({QStringLiteral("Bob Smith"), QStringLiteral("irc.example.org"), QStringLiteral("Example Network")});
        const QString text = IrcMessageFormatter::formatMessage(message, true);
        QVERIFY2(text.contains(QStringLiteral("[WHOWAS] bob was ident@example.org (Bob Smith)")), qPrintable(text));
    }

    // Each thing a WHOIS or WHOWAS reply says is a line of its own, not run
    // on into the one before it
    void whois_putsEveryLineOnItsOwn()
    {
        auto* message = new IrcWhoisMessage(&mConnection);
        message->setPrefix(QStringLiteral("bob!ident@example.org"));
        message->setParameters({QStringLiteral("Bob Smith"), QStringLiteral("irc.example.org"), QStringLiteral("Example Network"), QStringLiteral("bobaccount")});
        const QStringList lines = IrcMessageFormatter::formatMessage(message, true).split(QLatin1Char('\n'));
        QCOMPARE(lines.size(), 3);
        for (const QString& line : lines) {
            QVERIFY2(line.startsWith(QStringLiteral("[WHOIS] bob ")) && line.count(QStringLiteral("[WHOIS]")) == 1, qPrintable(line));
        }
        const QString html = IrcMessageFormatter::formatMessage(message, false);
        QVERIFY2(html.contains(QStringLiteral("(Bob Smith)<br />\n[WHOIS] bob is connected via")), qPrintable(html));
    }

    void whowas_putsEveryLineOnItsOwn()
    {
        auto* message = new IrcWhowasMessage(&mConnection);
        message->setPrefix(QStringLiteral("bob!ident@example.org"));
        message->setParameters({QStringLiteral("Bob Smith"), QStringLiteral("irc.example.org"), QStringLiteral("Example Network"), QStringLiteral("bobaccount")});
        QCOMPARE(IrcMessageFormatter::formatMessage(message, true),
                 QStringLiteral("[WHOWAS] bob was ident@example.org (Bob Smith)\n[WHOWAS] bob was connected via irc.example.org (Example Network)\n[WHOWAS] bob was logged in as bobaccount"));
        const QString html = IrcMessageFormatter::formatMessage(message, false);
        QVERIFY2(html.contains(QStringLiteral("(Bob Smith)<br />\n[WHOWAS] bob was connected via")), qPrintable(html));
    }

    void whoReply_marksAnAwayUser()
    {
        auto* message = new IrcWhoReplyMessage(&mConnection);
        message->setPrefix(QStringLiteral("bob!ident@example.org"));
        // mask, server, the flags that say away and server operator, real name
        message->setParameters({QStringLiteral("ident@example.org"), QStringLiteral("irc.example.org"), QStringLiteral("G*"), QStringLiteral("Bob Smith")});
        const QString text = IrcMessageFormatter::formatMessage(message, true);
        QCOMPARE(text, QStringLiteral("[WHO] bob (Bob Smith) - away - server operator"));
    }

    // -------------------------------------------------------------------------
    // The wrapper every formatted message goes through, which is also what
    // dlgIRC calls directly for the client's own status lines.
    // -------------------------------------------------------------------------

    void wrapper_prependsTheTimeForTheWindow()
    {
        const QString html = IrcMessageFormatter::formatMessage(QStringLiteral("plain status line"));
        QVERIFY2(QRegularExpression(QStringLiteral("^\\[\\d\\d:\\d\\d:\\d\\d\\] plain status line$")).match(html).hasMatch(), qPrintable(html));
    }

    void wrapper_leavesTheLuaFormAlone() { QCOMPARE(IrcMessageFormatter::formatMessage(QStringLiteral("! bob has quit"), QStringLiteral("#f29010"), true), QStringLiteral("! bob has quit")); }

    void wrapper_saysNothingAboutAnEmptyMessage() { QVERIFY(IrcMessageFormatter::formatMessage(QString()).isEmpty()); }

    // The leading character is how the client tells its four kinds of line
    // apart, and is the only thing that picks their colour
    void wrapper_coloursByTheLeadingCharacter()
    {
        QVERIFY(IrcMessageFormatter::formatMessage(QStringLiteral("! a server event")).startsWith(QStringLiteral("<font color='gray'>")));
        QVERIFY(IrcMessageFormatter::formatMessage(QStringLiteral("* an action")).startsWith(QStringLiteral("<font color='maroon'>")));
        QVERIFY(IrcMessageFormatter::formatMessage(QStringLiteral("$ a client notice")).startsWith(QStringLiteral("<font color='#3cc46e'>")));
        QVERIFY(IrcMessageFormatter::formatMessage(QStringLiteral("[TOPIC] something"), QStringLiteral("#3283bc")).startsWith(QStringLiteral("<font color='#3283bc'>")));
    }

    void wrapper_fallsBackToTheDefaultColourForABracketedLine()
    {
        QVERIFY(IrcMessageFormatter::formatMessage(QStringLiteral("[TOPIC] something"), QString()).contains(QStringLiteral("<font color='#f29010'>")));
    }

    // -------------------------------------------------------------------------
    // The two duration helpers, which are also what the whois idle time uses.
    // -------------------------------------------------------------------------

    void formatDuration_secondsOnly() { QCOMPARE(IrcMessageFormatter::formatDuration(59), QStringLiteral("59 secs")); }

    void formatDuration_zeroStillSaysSeconds() { QCOMPARE(IrcMessageFormatter::formatDuration(0), QStringLiteral("0 secs")); }

    void formatDuration_minutesAndSeconds() { QCOMPARE(IrcMessageFormatter::formatDuration(61), QStringLiteral("1 mins 1 secs")); }

    void formatDuration_hoursMinutesAndSeconds() { QCOMPARE(IrcMessageFormatter::formatDuration(3661), QStringLiteral("1 hours 1 mins 1 secs")); }

    void formatDuration_daysDownToSeconds() { QCOMPARE(IrcMessageFormatter::formatDuration(90061), QStringLiteral("1 days 1 hours 1 mins 1 secs")); }

    // The argument is when the ping was sent, so what comes back is how long ago
    // that was - allowing for the clock ticking over between the two readings
    void formatSeconds_countsBackFromNow()
    {
        const QString elapsed = IrcMessageFormatter::formatSeconds(static_cast<int>(QDateTime::currentSecsSinceEpoch()) - 5);
        QVERIFY2(elapsed == QStringLiteral("5s") || elapsed == QStringLiteral("6s"), qPrintable(elapsed));
    }
};

QTEST_GUILESS_MAIN(IrcMessageFormatterTest)
#include "IrcMessageFormatterTest.moc"
