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
    // silently dropping it leaves them looking at a channel they are no longer in
    void kick_namesWhoWasKickedByWhom() { QCOMPARE(forLua(":bob!u@h KICK #mudlet alice :behave"), QStringLiteral("! bob kicked alice")); }

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
    // characters in it have to be escaped on the way - an information numeric no
    // less than any other line
    void numeric_informationIsEscapedBeforeItReachesTheWindow()
    {
        const QString html = forWindow(":server 001 me :<b>not bold</b>");
        QVERIFY2(!html.contains(QStringLiteral("<b>")), qPrintable(html));
        QVERIFY2(html.contains(QStringLiteral("&lt;b")), qPrintable(html));
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
