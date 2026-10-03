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

    void join_ownJoinSaysWhichNickItWasAs() { QCOMPARE(forLua(":me!u@h JOIN #mudlet"), QStringLiteral("! You have joined #mudlet as me")); }

    void part_withoutAReason() { QCOMPARE(forLua(":bob!u@h PART #mudlet"), QStringLiteral("! bob has left #mudlet")); }

    void part_withAReason() { QCOMPARE(forLua(":bob!u@h PART #mudlet :getting dinner"), QStringLiteral("! bob has left #mudlet (getting dinner)")); }

    void quit_withoutAReason() { QCOMPARE(forLua(":bob!u@h QUIT"), QStringLiteral("! bob has quit")); }

    void quit_withAReason() { QCOMPARE(forLua(":bob!u@h QUIT :connection reset"), QStringLiteral("! bob has quit (connection reset)")); }

    void nick_namesBothTheOldAndTheNewNick() { QCOMPARE(forLua(":old!u@h NICK :new"), QStringLiteral("! old has changed nick to new")); }

    void mode_namesWhoSetWhatOnWhom() { QCOMPARE(forLua(":bob!u@h MODE #mudlet +o alice"), QStringLiteral("! bob sets mode #mudlet +o alice")); }

    // The 324 reply to asking what a channel's modes are is composed by communi
    void mode_replySaysWhatTheChannelsModesAre()
    {
        auto* message = new IrcModeMessage(&mConnection);
        message->setCommand(QString::number(Irc::RPL_CHANNELMODEIS));
        message->setParameters({QStringLiteral("#mudlet"), QStringLiteral("+l"), QStringLiteral("10")});
        QCOMPARE(IrcMessageFormatter::formatMessage(message, true), QStringLiteral("! #mudlet mode is +l 10"));
    }

    void topic_changed() { QCOMPARE(forLua(":bob!u@h TOPIC #mudlet :a new topic"), QStringLiteral("! bob changed topic")); }

    void topic_cleared() { QCOMPARE(forLua(":bob!u@h TOPIC #mudlet :"), QStringLiteral("! bob cleared topic")); }

    void away_withAReason() { QCOMPARE(forLua(":bob!u@h AWAY :at lunch"), QStringLiteral("! bob is away (at lunch)")); }

    void away_withoutAReasonMeansBack() { QCOMPARE(forLua(":bob!u@h AWAY"), QStringLiteral("! bob is back")); }

    // Being marked away ourselves is shown in the server's own words
    void away_ownIsTheServersWordsAlone() { QCOMPARE(forLua(":me!u@h AWAY :You have been marked as being away"), QStringLiteral("! You have been marked as being away")); }

    void away_reasonIsEscapedForTheWindow()
    {
        const QString html = forWindow(":bob!u@h AWAY :<u>not underlined</u>");
        QVERIFY2(!html.contains(QStringLiteral("<u>")), qPrintable(html));
        QVERIFY2(html.contains(QStringLiteral("! bob is away (&lt;u")), qPrintable(html));
    }

    void invite_namesWhoWasInvitedWhere() { QCOMPARE(forLua(":bob!u@h INVITE alice #mudlet"), QStringLiteral("! bob invited to #mudlet")); }

    // The 341 reply confirming an invitation this connection sent
    void invite_replyNamesWhoWasInvited()
    {
        auto* message = new IrcInviteMessage(&mConnection);
        message->setCommand(QString::number(Irc::RPL_INVITING));
        message->setParameters({QStringLiteral("alice"), QStringLiteral("#mudlet")});
        QCOMPARE(IrcMessageFormatter::formatMessage(message, true), QStringLiteral("! invited alice to #mudlet"));
    }

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

    // A CTCP PING carries the time it was sent, and the reply echoes it back
    void notice_ctcpPingReplyIsReportedAsTheRoundTrip()
    {
        // however long a slow machine takes over it, the round trip lies between
        // the five seconds ago it was sent and the time it was formatted by
        const qint64 sent = QDateTime::currentSecsSinceEpoch() - 5;
        const QString text = forLua(":bob!u@h NOTICE me :\001PING " + QByteArray::number(sent) + "\001");
        const qint64 latest = QDateTime::currentSecsSinceEpoch() - sent;
        const QRegularExpressionMatch match = QRegularExpression(QStringLiteral("^! bob replied in (\\d+)s$")).match(text);
        QVERIFY2(match.hasMatch(), qPrintable(text));
        const qint64 seconds = match.captured(1).toLongLong();
        QVERIFY2(seconds >= 5 && seconds <= latest, qPrintable(text));
    }

    void notice_toThisConnectionNamesTheSenderInTheWindow()
    {
        const QString html = forWindow(":bob!u@h NOTICE me :<u>just for you</u>");
        QVERIFY2(html.contains(QStringLiteral("[bob] &lt;u")), qPrintable(html));
        QVERIFY2(!html.contains(QStringLiteral("<u>")), qPrintable(html));
    }

    void numeric_belowThreeHundredIsInformation() { QCOMPARE(forLua(":server 001 me :Welcome to the network"), QStringLiteral("[INFO] Welcome to the network")); }

    // Everything the server sends is put into an HTML document, so the markup
    // characters in it have to be escaped on the way - in every field of every
    // line, not just the ones that are obviously free text
    void window_escapesMarkupInEveryField_data()
    {
        QTest::addColumn<QByteArray>("raw");

        QTest::newRow("information numeric") << QByteArrayLiteral(":server 001 me :<b>not bold</b>");
        QTest::newRow("kick reason") << QByteArrayLiteral(":bob!u@h KICK #mudlet alice :<b>not bold</b>");
        // a channel name may hold anything but NUL, BEL, CR, LF, space, comma
        // and colon, so it is markup a channel operator can choose
        QTest::newRow("kick channel") << QByteArrayLiteral(":bob!u@h KICK #<b>bold</b> alice :hi");
        QTest::newRow("part reason") << QByteArrayLiteral(":bob!u@h PART #mudlet :<b>not bold</b>");
        QTest::newRow("quit reason") << QByteArrayLiteral(":bob!u@h QUIT :<b>not bold</b>");
    }

    void window_escapesMarkupInEveryField()
    {
        QFETCH(QByteArray, raw);
        const QString html = forWindow(raw);
        QVERIFY2(!html.contains(QStringLiteral("<b>")), qPrintable(html));
        QVERIFY2(html.contains(QStringLiteral("&lt;b")), qPrintable(html));
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

    void numeric_serverVersionReply() { QCOMPARE(forLua(":server 351 me 2.11 irc.example.org :comments"), QStringLiteral("! server version is 2.11")); }

    void numeric_serverTimeReply() { QCOMPARE(forLua(":server 391 me irc.example.org :Friday September 25 2026"), QStringLiteral("! irc.example.org time is Friday September 25 2026")); }

    // A numeric that communi folds into a composed message (here one line of
    // the MOTD) is shown through that message, so it says nothing of its own
    void numeric_partOfAComposedReplyIsNotShownTwice()
    {
        QVERIFY(forLua(":server 372 me :- a line of the MOTD").isEmpty());
        QVERIFY(forWindow(":server 372 me :- a line of the MOTD").isEmpty());
    }

    void numeric_implicitIsNotShown()
    {
        const QByteArray raw = ":server 333 me #mudlet bob :1234567890";
        QVERIFY2(!forLua(raw).isEmpty(), "the same numeric should be shown when it is not implicit");
        IrcMessage* message = fromRaw(raw);
        message->setFlag(IrcMessage::Implicit);
        QVERIFY(IrcMessageFormatter::formatMessage(message, true).isEmpty());
        QVERIFY(IrcMessageFormatter::formatMessage(message, false).isEmpty());
    }

    void numeric_channelUrlIsALinkInTheWindow()
    {
        const QString html = forWindow(":server 328 me #mudlet :https://www.mudlet.org/");
        QVERIFY2(html.contains(QStringLiteral("[Channel URL] #mudlet <a href='https://www.mudlet.org/'>")), qPrintable(html));
    }

    void numeric_errorAndOtherCodesAreEscapedForTheWindow()
    {
        const QString error = forWindow(":server 401 me <u>nick</u> :No such nick");
        QVERIFY2(error.contains(QStringLiteral("[ERROR] &lt;u")) && !error.contains(QStringLiteral("<u>")), qPrintable(error));
        const QString other = forWindow(":server 333 me #<u>mudlet</u> bob :1234567890");
        QVERIFY2(other.contains(QStringLiteral("[333] #&lt;u")) && !other.contains(QStringLiteral("<u>")), qPrintable(other));
    }

    void error_isMarkedAsAnError() { QCOMPARE(forLua("ERROR :Closing link"), QStringLiteral("[ERROR] Closing link")); }

    void unknown_isShownVerbatimRatherThanDropped() { QCOMPARE(forLua(":bob!u@h FROBNICATE one two"), QStringLiteral("? bob FROBNICATE one two")); }

    // A message type the formatter has no line for, such as the server's PING,
    // comes out empty, which is what tells the IRC window to leave it out
    void ping_isNotShown()
    {
        QVERIFY(forLua(":server PING :irc.example.org").isEmpty());
        QVERIFY(forWindow(":server PING :irc.example.org").isEmpty());
    }

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

    void topic_replyWithNoTopicSaysSo()
    {
        auto* message = new IrcTopicMessage(&mConnection);
        message->setCommand(QString::number(Irc::RPL_NOTOPIC));
        message->setParameters({QStringLiteral("#mudlet"), QStringLiteral("No topic is set")});
        QCOMPARE(IrcMessageFormatter::formatMessage(message, true), QStringLiteral("! no topic"));
    }

    void topic_replyIsColouredAsATopicInTheWindow()
    {
        auto* message = new IrcTopicMessage(&mConnection);
        message->setCommand(QString::number(Irc::RPL_TOPIC));
        message->setParameters({QStringLiteral("#mudlet"), QStringLiteral("<u>not underlined</u>")});
        const QString html = IrcMessageFormatter::formatMessage(message, false);
        QVERIFY2(html.startsWith(QStringLiteral("<font color='#3283bc'>")), qPrintable(html));
        QVERIFY2(html.contains(QStringLiteral("[TOPIC] &lt;u")) && !html.contains(QStringLiteral("<u>")), qPrintable(html));
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

    void whowas_reportsTheAccountTheyWereLoggedInAs()
    {
        auto* message = new IrcWhowasMessage(&mConnection);
        message->setPrefix(QStringLiteral("bob!ident@example.org"));
        message->setParameters({QStringLiteral("Bob Smith"), QStringLiteral("irc.example.org"), QStringLiteral("Example Network"), QStringLiteral("bobaccount")});
        const QString text = IrcMessageFormatter::formatMessage(message, true);
        QVERIFY2(text.contains(QStringLiteral("[WHOWAS] bob was logged in as bobaccount")), qPrintable(text));
    }

    void whois_reportsWhyTheyAreAway()
    {
        auto* message = new IrcWhoisMessage(&mConnection);
        message->setPrefix(QStringLiteral("bob!ident@example.org"));
        QStringList parameters(10);
        parameters[0] = QStringLiteral("Bob Smith");
        parameters[9] = QStringLiteral("gone fishing");
        message->setParameters(parameters);
        const QString text = IrcMessageFormatter::formatMessage(message, true);
        QVERIFY2(text.contains(QStringLiteral("[WHOIS] bob is away: gone fishing")), qPrintable(text));
    }

    void whowas_reportsWhoTheyWere()
    {
        auto* message = new IrcWhowasMessage(&mConnection);
        message->setPrefix(QStringLiteral("bob!ident@example.org"));
        message->setParameters({QStringLiteral("Bob Smith"), QStringLiteral("irc.example.org"), QStringLiteral("Example Network")});
        const QString text = IrcMessageFormatter::formatMessage(message, true);
        QVERIFY2(text.contains(QStringLiteral("[WHOWAS] bob was ident@example.org (Bob Smith)")), qPrintable(text));
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
