/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "ircmessage.h"
#include "ircconnection.h"
#include "ircprotocol.h"
#include <QtTest/QtTest>
#include <QtCore/QRegExp>
#include <QtCore/QTextCodec>
#include <QtCore/QScopedPointer>

#ifdef Q_OS_LINUX
#include "ircmessagedecoder_p.h"
#endif // Q_OS_LINUX

class tst_IrcMessage : public QObject
{
    Q_OBJECT

private slots:
    void testDefaults();

    void testToData();

    void testPrefix_data();
    void testPrefix();

    void testParameters_data();
    void testParameters();

    void testFlags();

    void testEncoding_data();
    void testEncoding();

    void testDecoder_data();
    void testDecoder();

    void testTags();
    void testServerTime();

    void testAccount_data();
    void testAccount();

    void testAccountMessage_data();
    void testAccountMessage();
    void testAwayMessage_data();
    void testAwayMessage();
    void testCapabilityMessage_data();
    void testCapabilityMessage();
    void testErrorMessage_data();
    void testErrorMessage();
    void testHostChangeMessage_data();
    void testHostChangeMessage();
    void testInviteMessage_data();
    void testInviteMessage();
    void testJoinMessage_data();
    void testJoinMessage();
    void testKickMessage_data();
    void testKickMessage();
    void testNamesMessage();
    void testNickMessage_data();
    void testNickMessage();
    void testNoticeMessage_data();
    void testNoticeMessage();
    void testNumericMessage_data();
    void testNumericMessage();
    void testModeMessage_data();
    void testModeMessage();
    void testMotdMessage();
    void testPartMessage_data();
    void testPartMessage();
    void testPingMessage();
    void testPongMessage();
    void testPrivateMessage_data();
    void testPrivateMessage();
    void testQuitMessage_data();
    void testQuitMessage();
    void testTopicMessage_data();
    void testTopicMessage();
    void testWhoReplyMessage_data();
    void testWhoReplyMessage();

    void testClone();
    void testNullConnection();

    void testDebug();
};

void tst_IrcMessage::testDefaults()
{
    IrcMessage msg(0);
    QVERIFY(!msg.isValid());
    QVERIFY(!msg.connection());
    QCOMPARE(msg.type(), IrcMessage::Unknown);
    QCOMPARE(msg.flags(), IrcMessage::None);
    QCOMPARE(msg.encoding(), QByteArray("ISO-8859-15"));
    QVERIFY(msg.prefix().isNull());
    QVERIFY(msg.nick().isNull());
    QVERIFY(msg.ident().isNull());
    QVERIFY(msg.host().isNull());
    QVERIFY(msg.command().isNull());
    QVERIFY(msg.parameters().isEmpty());
    QVERIFY(msg.toData().isEmpty());
}

void tst_IrcMessage::testToData()
{
    IrcConnection connection;
    IrcMessage* msg = IrcMessage::fromData(":nick!ident@host PRIVMSG user :hello there", &connection);
    QCOMPARE(msg->toData(), QByteArray(":nick!ident@host PRIVMSG user :hello there"));

    msg = IrcMessage::fromData(":nick!ident@host PRIVMSG user ::)", &connection);
    QCOMPARE(msg->toData(), QByteArray(":nick!ident@host PRIVMSG user ::)"));

    msg = IrcMessage::fromData(":nick!ident@host TOPIC #channel :", &connection);
    QCOMPARE(msg->toData(), QByteArray(":nick!ident@host TOPIC #channel :"));

    msg = IrcMessage::fromParameters("nick!ident@host", "PRIVMSG", QStringList() << "user" << "hello there", &connection);
    QCOMPARE(msg->toData(), QByteArray(":nick!ident@host PRIVMSG user :hello there"));

    msg = IrcMessage::fromParameters("nick!ident@host", "PRIVMSG", QStringList() << "user" << ":)", &connection);
    QCOMPARE(msg->toData(), QByteArray(":nick!ident@host PRIVMSG user ::)"));

    msg = IrcMessage::fromParameters("nick!ident@host", "TOPIC", QStringList() << "#channel" << "", &connection);
    QCOMPARE(msg->toData(), QByteArray(":nick!ident@host TOPIC #channel :"));
}

void tst_IrcMessage::testPrefix_data()
{
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<QString>("nick");
    QTest::addColumn<QString>("ident");
    QTest::addColumn<QString>("host");

    QTest::newRow("null") << QString() << QString() << QString() << QString() << QString();
    QTest::newRow("empty") << QString("") << QString("") << QString() << QString() << QString();
    QTest::newRow("space") << QString(" ") << QString(" ") << QString() << QString() << QString();
    QTest::newRow("nick!ident@host") << QString("nick!ident@host") << QString("nick!ident@host") << QString("nick") << QString("ident") << QString("host");
}

void tst_IrcMessage::testPrefix()
{
    QFETCH(QString, prefix);
    QFETCH(QString, expected);
    QFETCH(QString, nick);
    QFETCH(QString, ident);
    QFETCH(QString, host);

    IrcMessage msg(0);
    msg.setPrefix(prefix);
    QCOMPARE(msg.prefix(), expected);
    QCOMPARE(msg.nick(), nick);
    QCOMPARE(msg.ident(), ident);
    QCOMPARE(msg.host(), host);
}

void tst_IrcMessage::testParameters_data()
{
    Irc::registerMetaTypes();

    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QString>("command");
    QTest::addColumn<QStringList>("params");
    QTest::addColumn<IrcMessage::Type>("type");

    QTest::newRow("null") << QString() << QString() << QStringList() << IrcMessage::Unknown;
    QTest::newRow("message") << QString("nick!ident@host") << QString("PRIVMSG") << QStringList("p") << IrcMessage::Private;
    QTest::newRow("notice") << QString("nick!ident@host") << QString("NOTICE") << QStringList("p") << IrcMessage::Notice;
}

void tst_IrcMessage::testParameters()
{
    QFETCH(QString, prefix);
    QFETCH(QString, command);
    QFETCH(QStringList, params);
    QFETCH(IrcMessage::Type, type);

    IrcConnection connection;
    QScopedPointer<IrcMessage> message(IrcMessage::fromParameters(prefix, command, params, &connection));
    QCOMPARE(message->type(), type);
    QCOMPARE(message->prefix(), prefix);
    QCOMPARE(message->command(), command);
    QCOMPARE(message->parameters(), params);

    for (int i = 0; i < params.count(); ++i)
        QCOMPARE(message->parameter(i), params.at(i));

    message->setParameter(5, "foo");
    QCOMPARE(message->parameter(4), QString());
    QCOMPARE(message->parameter(5), QString("foo"));
}

void tst_IrcMessage::testFlags()
{
    IrcMessage msg(0);
    msg.setPrefix("a!b@c");
    QCOMPARE(msg.flags(), IrcMessage::None);

    QVERIFY(!msg.testFlag(IrcMessage::Playback));
    msg.setFlag(IrcMessage::Playback);
    QVERIFY(msg.testFlag(IrcMessage::Playback));
    QCOMPARE(msg.flags(), IrcMessage::Playback);

    QVERIFY(!msg.testFlag(IrcMessage::Implicit));
    msg.setFlag(IrcMessage::Implicit);
    QVERIFY(msg.testFlag(IrcMessage::Implicit));
    QCOMPARE(msg.flags(), IrcMessage::Playback | IrcMessage::Implicit);

    msg.setFlag(IrcMessage::Playback, false);
    QVERIFY(!msg.testFlag(IrcMessage::Playback));
    QCOMPARE(msg.flags(), IrcMessage::Implicit);

    msg.setFlags(IrcMessage::None);
    QVERIFY(!msg.testFlag(IrcMessage::Implicit));
    QVERIFY(!msg.testFlag(IrcMessage::Playback));
    QCOMPARE(msg.flags(), IrcMessage::None);
}

void tst_IrcMessage::testEncoding_data()
{
    QTest::addColumn<QByteArray>("encoding");
    QTest::addColumn<QByteArray>("actual");
    QTest::addColumn<bool>("supported");

    QTest::newRow("null") << QByteArray() << QByteArray("ISO-8859-15") << false;
    QTest::newRow("empty") << QByteArray("") << QByteArray("ISO-8859-15") << false;
    QTest::newRow("space") << QByteArray(" ") << QByteArray("ISO-8859-15") << false;
    QTest::newRow("invalid") << QByteArray("invalid") << QByteArray("ISO-8859-15") << false;
    foreach (const QByteArray& codec, QTextCodec::availableCodecs())
        QTest::newRow(codec) << codec << codec << true;
}

void tst_IrcMessage::testEncoding()
{
    QFETCH(QByteArray, encoding);
    QFETCH(QByteArray, actual);
    QFETCH(bool, supported);

    if (!supported)
        QTest::ignoreMessage(QtWarningMsg, "IrcMessage::setEncoding(): unsupported encoding \"" + encoding + "\" ");

    IrcMessage msg(0);
    msg.setEncoding(encoding);
    QCOMPARE(msg.encoding(), actual);
}

void tst_IrcMessage::testDecoder_data()
{
    QTest::addColumn<QByteArray>("encoding");
    QTest::addColumn<QByteArray>("base64");

    QTest::newRow("windows-1251") << QByteArray("windows-1251") << QByteArray("7+Xt8eju7eXw4Owg7+7k5OXr/O375Q==");
    QTest::newRow("EUC-JP") << QByteArray("EUC-JP") << QByteArray("pKSkxKTHpOKkyaSzpMek4qGhpbml3qXbyMc=");
    QTest::newRow("Shift-JIS") << QByteArray("Shift-JIS") << QByteArray("lbaOmoNSgVuDaJVcg1aDdINn");
    QTest::newRow("ISO-8859-15") << QByteArray("ISO-8859-15") << QByteArray("5Gl0aWVucORpduQ="); // TODO: QByteArray("5OQ=");
}

void tst_IrcMessage::testDecoder()
{
    QFETCH(QByteArray, encoding);
    QFETCH(QByteArray, base64);

#ifdef Q_OS_LINUX
    // others have problems with symbols (win) or private headers (osx frameworks)
    IrcMessageDecoder decoder;
    QString actual = decoder.decode(QByteArray::fromBase64(base64), encoding);
    QString expected = QTextCodec::codecForName(encoding)->toUnicode(QByteArray::fromBase64(base64));
    QCOMPARE(actual, expected);
#endif // Q_OS_LINUX
}

void tst_IrcMessage::testTags()
{
    QVariantMap tags;
    tags.insert("aaa", "bbb");
    tags.insert("ccc", "");
    tags.insert("example.com/ddd", "eee");

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData("@aaa=bbb;ccc;example.com/ddd=eee :nick!ident@host.com PRIVMSG me :Hello", &connection);
    QCOMPARE(message->tags(), tags);

    tags.insert("ccc", "xyz");
    message->setTag("ccc", "xyz");

    QCOMPARE(message->tags(), tags);
    QCOMPARE(message->toData(), QByteArray("@aaa=bbb;ccc=xyz;example.com/ddd=eee :nick!ident@host.com PRIVMSG me Hello"));

    tags.insert("fff", "ggg");
    message->setTag("fff", "ggg");
    QCOMPARE(message->tags(), tags);
    QCOMPARE(message->toData(), QByteArray("@aaa=bbb;ccc=xyz;example.com/ddd=eee;fff=ggg :nick!ident@host.com PRIVMSG me Hello"));

    tags.clear();
    tags.insert("foo", "bar");
    message->setTags(tags);
    QCOMPARE(message->tags(), tags);
    QCOMPARE(message->toData(), QByteArray("@foo=bar :nick!ident@host.com PRIVMSG me Hello"));
}

void tst_IrcMessage::testServerTime()
{
    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData("@time=2011-10-19T16:40:51.620Z :Angel!angel@example.org PRIVMSG Wiz :Hello", &connection);
    QCOMPARE(message->timeStamp(), QDateTime(QDate(2011, 10, 19), QTime(16, 40, 51, 620), Qt::UTC));
}

void tst_IrcMessage::testAccount_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("account");

    QTest::newRow("no account") << QByteArray(":nick!ident@host.com PRIVMSG me :Hello") << QString();
    QTest::newRow("has account") << QByteArray("@account=jpnurmi :nick!ident@host.com PRIVMSG me :Hello") << QString("jpnurmi");
}

void tst_IrcMessage::testAccount()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, account);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->account(), account);
}

void tst_IrcMessage::testAccountMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("account");

    QTest::newRow("no prefix") << false << QByteArray("ACCOUNT") << QString();
    QTest::newRow("empty prefix") << false << QByteArray(": ACCOUNT") << QString();
    QTest::newRow("no params") << false << QByteArray(":nick!ident@host ACCOUNT") << QString();
    QTest::newRow("identified") << true << QByteArray(":nick!ident@host ACCOUNT account") << QString("account");
    QTest::newRow("unidentified") << true << QByteArray(":nick!ident@host ACCOUNT *") << QString();
}

void tst_IrcMessage::testAccountMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, account);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Account);
    QCOMPARE(message->command(), QString("ACCOUNT"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("account").toString(), account);

    IrcAccountMessage* accountMessage = qobject_cast<IrcAccountMessage*>(message);
    QVERIFY(accountMessage);
    QCOMPARE(accountMessage->isValid(), valid);
    QCOMPARE(accountMessage->account(), account);
}

void tst_IrcMessage::testAwayMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("content");

    QTest::newRow("no prefix") << true << QByteArray("AWAY") << QString();
    QTest::newRow("empty prefix") << false << QByteArray(": AWAY") << QString();
    QTest::newRow("away") << true << QByteArray(":nick!ident@host AWAY :gone far away") << QString("gone far away");
    QTest::newRow("back") << true << QByteArray(":nick!ident@host AWAY") << QString();
}

void tst_IrcMessage::testAwayMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, content);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Away);
    QCOMPARE(message->command(), QString("AWAY"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("content").toString(), content);
    QCOMPARE(message->property("away").toBool(), !content.isEmpty());
    QVERIFY(!message->property("reply").toBool());

    IrcAwayMessage* awayMessage = qobject_cast<IrcAwayMessage*>(message);
    QVERIFY(awayMessage);
    QCOMPARE(awayMessage->isValid(), valid);
    QCOMPARE(awayMessage->content(), content);
    QCOMPARE(awayMessage->isAway(), !content.isEmpty());
    QVERIFY(!awayMessage->isReply());
}

void tst_IrcMessage::testCapabilityMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("subCommand");
    QTest::addColumn<QStringList>("capabilities");

    QTest::newRow("no prefix") << true << QByteArray("CAP") << QString() << QStringList();
    QTest::newRow("empty prefix") << false << QByteArray(": CAP") << QString() << QStringList();
    QTest::newRow("no params") << true << QByteArray(":server CAP") << QString() << QStringList();

    QTest::newRow("ls") << true << QByteArray(":server CAP * LS :identify-msg sasl") << QString("LS") << (QStringList() << "identify-msg" << "sasl");
    QTest::newRow("ack") << true << QByteArray(":server CAP communi ACK :identify-msg") << QString("ACK") << (QStringList() << "identify-msg");
    QTest::newRow("nak") << true << QByteArray(":server CAP communi NAK :sasl") << QString("NAK") << (QStringList() << "sasl");
}

void tst_IrcMessage::testCapabilityMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, subCommand);
    QFETCH(QStringList, capabilities);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Capability);
    QCOMPARE(message->command(), QString("CAP"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("subCommand").toString(), subCommand);
    QCOMPARE(message->property("capabilities").toStringList(), capabilities);

    IrcCapabilityMessage* capabilityMessage = qobject_cast<IrcCapabilityMessage*>(message);
    QVERIFY(capabilityMessage);
    QCOMPARE(capabilityMessage->isValid(), valid);
    QCOMPARE(capabilityMessage->subCommand(), subCommand);
    QCOMPARE(capabilityMessage->capabilities(), capabilities);
}

void tst_IrcMessage::testErrorMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("error");

    QTest::newRow("no prefix") << true << QByteArray("ERROR error1") << QString("error1");
    QTest::newRow("empty prefix") << false << QByteArray(": ERROR error1") << QString("error1");
    QTest::newRow("no params") << false << QByteArray(":server ERROR") << QString();
    QTest::newRow("all ok") << true << QByteArray(":server ERROR error1") << QString("error1");
}

void tst_IrcMessage::testErrorMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, error);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Error);
    QCOMPARE(message->command(), QString("ERROR"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("error").toString(), error);

    IrcErrorMessage* errorMessage = qobject_cast<IrcErrorMessage*>(message);
    QVERIFY(errorMessage);
    QCOMPARE(errorMessage->isValid(), valid);
    QCOMPARE(errorMessage->error(), error);
}

void tst_IrcMessage::testHostChangeMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("host");

    QTest::newRow("no prefix") << true << QByteArray("CHGHOST newuser newhost") << QString("newuser") << QString("newhost");
    QTest::newRow("empty prefix") << false << QByteArray(": CHGHOST newuser newhost") << QString("newuser") << QString("newhost");
    QTest::newRow("no params") << false << QByteArray(":nick!user@host CHGHOST") << QString() << QString();
    QTest::newRow("all ok") << true << QByteArray(":nick!user@host CHGHOST newuser newhost") << QString("newuser") << QString("newhost");
}

void tst_IrcMessage::testHostChangeMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, user);
    QFETCH(QString, host);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::HostChange);
    QCOMPARE(message->command(), QString("CHGHOST"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("user").toString(), user);
    QCOMPARE(message->property("host").toString(), host);

    IrcHostChangeMessage* chghostMessage = qobject_cast<IrcHostChangeMessage*>(message);
    QVERIFY(chghostMessage);
    QCOMPARE(chghostMessage->isValid(), valid);
    QCOMPARE(chghostMessage->user(), user);
    QCOMPARE(chghostMessage->host(), host);
}

void tst_IrcMessage::testInviteMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("channel");

    QTest::newRow("no prefix") << true << QByteArray("INVITE Wiz #Dust") << QString("Wiz") << QString("#Dust");
    QTest::newRow("empty prefix") << false << QByteArray(": INVITE Wiz #Dust") << QString("Wiz") << QString("#Dust");
    QTest::newRow("no params") << false << QByteArray(":Angel INVITE") << QString() << QString();
    QTest::newRow("no channel") << false << QByteArray(":Angel INVITE Wiz") << QString("Wiz") << QString();
    QTest::newRow("all ok") << true << QByteArray(":Angel INVITE Wiz #Dust") << QString("Wiz") << QString("#Dust");
}

void tst_IrcMessage::testInviteMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, channel);
    QFETCH(QString, user);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Invite);
    QCOMPARE(message->command(), QString("INVITE"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("channel").toString(), channel);
    QCOMPARE(message->property("user").toString(), user);

    IrcInviteMessage* inviteMessage = qobject_cast<IrcInviteMessage*>(message);
    QVERIFY(inviteMessage);
    QCOMPARE(inviteMessage->isValid(), valid);
    QCOMPARE(inviteMessage->channel(), channel);
    QCOMPARE(inviteMessage->user(), user);
}

void tst_IrcMessage::testJoinMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("channel");
    QTest::addColumn<QString>("account");
    QTest::addColumn<QString>("realName");

    QTest::newRow("no prefix") << true << QByteArray("JOIN #Twilight_zone") << QString("#Twilight_zone") << QString() << QString();
    QTest::newRow("empty prefix") << false << QByteArray(": JOIN #Twilight_zone") << QString("#Twilight_zone") << QString() << QString();
    QTest::newRow("no params") << false << QByteArray(":WiZ JOIN") << QString() << QString() << QString();
    QTest::newRow("all ok") << true << QByteArray(":WiZ JOIN #Twilight_zone") << QString("#Twilight_zone") << QString() << QString();

    QTest::newRow("extended-join") << true << QByteArray(":nick!user@host JOIN #channelname accountname :Real Name") << QString("#channelname") << QString("accountname") << QString("Real Name");
    QTest::newRow("unidentified") << true << QByteArray(":nick!user@host JOIN #channelname * :Real Name") << QString("#channelname") << QString() << QString("Real Name");
}

void tst_IrcMessage::testJoinMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, channel);
    QFETCH(QString, account);
    QFETCH(QString, realName);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Join);
    QCOMPARE(message->command(), QString("JOIN"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("channel").toString(), channel);
    QCOMPARE(message->property("account").toString(), account);
    QCOMPARE(message->property("realName").toString(), realName);

    IrcJoinMessage* joinMessage = qobject_cast<IrcJoinMessage*>(message);
    QVERIFY(joinMessage);
    QCOMPARE(joinMessage->isValid(), valid);
    QCOMPARE(joinMessage->channel(), channel);
    QCOMPARE(joinMessage->account(), account);
    QCOMPARE(joinMessage->realName(), realName);
}

void tst_IrcMessage::testKickMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("channel");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("reason");

    QTest::newRow("no prefix") << true << QByteArray("KICK #Finnish John") << QString("#Finnish") << QString("John") << QString();
    QTest::newRow("empty prefix") << false << QByteArray(": KICK #Finnish John") << QString("#Finnish") << QString("John") << QString();
    QTest::newRow("no params") << false << QByteArray(":WiZ KICK") << QString() << QString() << QString();
    QTest::newRow("no user") << false << QByteArray(":WiZ KICK #Finnish") << QString("#Finnish") << QString() << QString();
    QTest::newRow("no reason") << true << QByteArray(":WiZ KICK #Finnish John") << QString("#Finnish") << QString("John") << QString();
    QTest::newRow("all ok") << true << QByteArray(":WiZ KICK #Finnish John :Another reason") << QString("#Finnish") << QString("John") << QString("Another reason");
}

void tst_IrcMessage::testKickMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, channel);
    QFETCH(QString, user);
    QFETCH(QString, reason);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Kick);
    QCOMPARE(message->command(), QString("KICK"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("channel").toString(), channel);
    QCOMPARE(message->property("user").toString(), user);
    QCOMPARE(message->property("reason").toString(), reason);

    IrcKickMessage* kickMessage = qobject_cast<IrcKickMessage*>(message);
    QVERIFY(kickMessage);
    QCOMPARE(kickMessage->isValid(), valid);
    QCOMPARE(kickMessage->channel(), channel);
    QCOMPARE(kickMessage->user(), user);
    QCOMPARE(kickMessage->reason(), reason);
}

void tst_IrcMessage::testNamesMessage()
{
    IrcConnection connection;
    IrcNamesMessage message(&connection);
    message.setPrefix("nick!ident@host");
    message.setParameters(QStringList() << "chan" << "usr1" << "usr2" << "usr3");
    QVERIFY(message.isValid());
    QCOMPARE(message.type(), IrcMessage::Names);
    QCOMPARE(message.command(), QString("NAMES"));
    QCOMPARE(message.channel(), QString("chan"));
    QCOMPARE(message.names(), QStringList() << "usr1" << "usr2" << "usr3");
}

void tst_IrcMessage::testNickMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("oldNick");
    QTest::addColumn<QString>("newNick");

    QTest::newRow("no prefix") << true << QByteArray("NICK Kilroy") << QString() << QString("Kilroy");
    QTest::newRow("empty prefix") << false << QByteArray(": NICK Kilroy") << QString() << QString("Kilroy");
    QTest::newRow("no params") << false << QByteArray(":WiZ NICK") << QString("WiZ") << QString();
    QTest::newRow("all ok") << true << QByteArray(":WiZ NICK Kilroy") << QString("WiZ") << QString("Kilroy");
}

void tst_IrcMessage::testNickMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, oldNick);
    QFETCH(QString, newNick);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Nick);
    QCOMPARE(message->command(), QString("NICK"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("oldNick").toString(), oldNick);
    QCOMPARE(message->property("newNick").toString(), newNick);

    IrcNickMessage* nickMessage = qobject_cast<IrcNickMessage*>(message);
    QVERIFY(nickMessage);
    QCOMPARE(nickMessage->isValid(), valid);
    QCOMPARE(nickMessage->oldNick(), oldNick);
    QCOMPARE(nickMessage->newNick(), newNick);
}

void tst_IrcMessage::testNoticeMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("target");
    QTest::addColumn<QString>("content");
    QTest::addColumn<bool>("priv");
    QTest::addColumn<bool>("reply");

    QTest::newRow("no prefix") << true << QByteArray("NOTICE Wiz :Hello are you receiving this message ?") << QString("Wiz") << QString("Hello are you receiving this message ?") << false << false;
    QTest::newRow("empty prefix") << false << QByteArray(": NOTICE Wiz :Hello are you receiving this message ?") << QString("Wiz") << QString("Hello are you receiving this message ?") << false << false;
    QTest::newRow("no params") << false << QByteArray(":Angel NOTICE Wiz") << QString("Wiz") << QString() << false << false;
    QTest::newRow("all ok") << true << QByteArray(":Angel NOTICE Wiz :Hello are you receiving this message ?") << QString("Wiz") << QString("Hello are you receiving this message ?") << false << false;
    QTest::newRow("private") << true << QByteArray(":Angel NOTICE communi :Hello are you receiving this message ?") << QString("communi") << QString("Hello are you receiving this message ?") << true << false;
    QTest::newRow("reply") << true << QByteArray(":Angel NOTICE Wiz :\1Hello are you receiving this message ?\1") << QString("Wiz") << QString("Hello are you receiving this message ?") << false << true;
}

void tst_IrcMessage::testNoticeMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, target);
    QFETCH(QString, content);
    QFETCH(bool, priv);
    QFETCH(bool, reply);

    IrcConnection connection;
    connection.setNickName("communi");
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Notice);
    QCOMPARE(message->command(), QString("NOTICE"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("target").toString(), target);
    QCOMPARE(message->property("content").toString(), content);
    QCOMPARE(message->property("private").toBool(), priv);
    QCOMPARE(message->property("reply").toBool(), reply);

    IrcNoticeMessage* noticeMessage = qobject_cast<IrcNoticeMessage*>(message);
    QVERIFY(noticeMessage);
    QCOMPARE(noticeMessage->isValid(), valid);
    QCOMPARE(noticeMessage->target(), target);
    QCOMPARE(noticeMessage->content(), content);
    QCOMPARE(noticeMessage->isPrivate(), priv);
    QCOMPARE(noticeMessage->isReply(), reply);
}

void tst_IrcMessage::testNumericMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<int>("code");
    QTest::addColumn<bool>("composed");

    QTest::newRow("no prefix") << true << QByteArray("123 Kilroy") << 123 << false;
    QTest::newRow("empty prefix") << false << QByteArray(": 123 Kilroy") << 123 << false;
    QTest::newRow("no params") << true << QByteArray(":WiZ 456") << 456 << false;
    QTest::newRow("all ok") << true << QByteArray(":WiZ 789 Kilroy") << 789 << false;
    QTest::newRow("composed") << true << QByteArray(":server 352 me someone ...") << 352 << true;
}

void tst_IrcMessage::testNumericMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(int, code);
    QFETCH(bool, composed);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Numeric);
    QVERIFY(message->command().toInt() > 0);
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("code").toInt(), code);
    QCOMPARE(message->property("composed").toBool(), composed);

    IrcNumericMessage* numericMessage = qobject_cast<IrcNumericMessage*>(message);
    QVERIFY(numericMessage);
    QCOMPARE(numericMessage->isValid(), valid);
    QCOMPARE(numericMessage->code(), code);
    QCOMPARE(numericMessage->isComposed(), composed);
}

void tst_IrcMessage::testModeMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("target");
    QTest::addColumn<QString>("mode");
    QTest::addColumn<QString>("argument");

    QTest::newRow("no prefix") << true << QByteArray("MODE Kilroy -w") << QString("Kilroy") << QString("-w") << QString();
    QTest::newRow("empty prefix") << false << QByteArray(": MODE Kilroy -w") << QString("Kilroy") << QString("-w") << QString();
    QTest::newRow("no params") << false << QByteArray(":WiZ MODE Kilroy") << QString("Kilroy") << QString() << QString();
    QTest::newRow("all ok") << true << QByteArray(":WiZ MODE Kilroy -w") << QString("Kilroy") << QString("-w") << QString();

    QTest::newRow("1") << true << QByteArray(":WiZ MODE #Finnish +im") << QString("#Finnish") << QString("+im") << QString();
    QTest::newRow("2") << true << QByteArray(":Angel MODE #Finnish +o Kilroy") << QString("#Finnish") << QString("+o") << QString("Kilroy");
    QTest::newRow("3") << true << QByteArray(":Kilroy MODE #Finnish +v Wiz") << QString("#Finnish") << QString("+v") << QString("Wiz");
    QTest::newRow("4a") << true << QByteArray("MODE #Fins -s") << QString("#Fins") << QString("-s") << QString();
    QTest::newRow("4b") << false << QByteArray(": MODE #Fins -s") << QString("#Fins") << QString("-s") << QString();
    QTest::newRow("5") << true << QByteArray(":WiZ MODE #42 +k oulu") << QString("#42") << QString("+k") << QString("oulu");
    QTest::newRow("6a") << true << QByteArray("MODE #eu-opers +l 10") << QString("#eu-opers") << QString("+l") << QString("10");
    QTest::newRow("6b") << false << QByteArray(": MODE #eu-opers +l 10") << QString("#eu-opers") << QString("+l") << QString("10");
    QTest::newRow("7") << true << QByteArray(":nobody MODE &oulu +b") << QString("&oulu") << QString("+b") << QString();
    QTest::newRow("8") << true << QByteArray(":someone MODE &oulu +b *!*@*") << QString("&oulu") << QString("+b") << QString("*!*@*");
    QTest::newRow("9") << true << QByteArray(":anyone MODE &oulu +b *!*@*.edu") << QString("&oulu") << QString("+b") << QString("*!*@*.edu");
    QTest::newRow("10a") << true << QByteArray("MODE WiZ -w") << QString("WiZ") << QString("-w") << QString();
    QTest::newRow("10b") << false << QByteArray(": MODE WiZ -w") << QString("WiZ") << QString("-w") << QString();
    QTest::newRow("11") << true << QByteArray(":Angel MODE Angel +i") << QString("Angel") << QString("+i") << QString();
    QTest::newRow("12") << true << QByteArray(":WiZ MODE WiZ -o") << QString("WiZ") << QString("-o") << QString();

    QTest::newRow("args") << true << QByteArray(":someone MODE #chan +lk 10 secret") << QString("#chan") << QString("+lk") << QString("10 secret");
}

void tst_IrcMessage::testModeMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, target);
    QFETCH(QString, mode);
    QFETCH(QString, argument);

    const QString arg = argument.split(" ", QString::SkipEmptyParts).value(0);
    const QStringList args = argument.split(" ", QString::SkipEmptyParts);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Mode);
    QCOMPARE(message->command(), QString("MODE"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("target").toString(), target);
    QCOMPARE(message->property("mode").toString(), mode);
    QCOMPARE(message->property("argument").toString(), arg);
    QCOMPARE(message->property("arguments").toStringList(), args);

    IrcModeMessage* modeMessage = qobject_cast<IrcModeMessage*>(message);
    QVERIFY(modeMessage);
    QCOMPARE(modeMessage->isValid(), valid);
    QCOMPARE(modeMessage->target(), target);
    QCOMPARE(modeMessage->mode(), mode);
    QCOMPARE(modeMessage->argument(), arg);
    QCOMPARE(modeMessage->arguments(), args);
}

void tst_IrcMessage::testMotdMessage()
{
    IrcConnection connection;
    IrcMotdMessage message(&connection);
    message.setPrefix("nick!ident@host");
    QStringList params;
    params += "user";
    params += ":server 375 user :- server Message of the Day";
    params += ":server 372 user :- Welcome...";
    params += ":server 376 user :End of /MOTD command";
    message.setParameters(params);
    QVERIFY(message.isValid());
    QCOMPARE(message.type(), IrcMessage::Motd);
    QCOMPARE(message.command(), QString("MOTD"));
    QCOMPARE(message.lines(), QStringList(params.mid(1)));
}

void tst_IrcMessage::testPartMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("channel");
    QTest::addColumn<QString>("reason");

    QTest::newRow("no prefix") << true << QByteArray("PART #Twilight_zone") << QString("#Twilight_zone") << QString();
    QTest::newRow("empty prefix") << false << QByteArray(": PART #Twilight_zone") << QString("#Twilight_zone") << QString();
    QTest::newRow("no params") << false << QByteArray(":WiZ PART") << QString() << QString();
    QTest::newRow("no reason") << true << QByteArray(":WiZ PART #Twilight_zone") << QString("#Twilight_zone") << QString();
    QTest::newRow("all ok") << true << QByteArray(":WiZ PART #Twilight_zone :Gone to have lunch") << QString("#Twilight_zone") << QString("Gone to have lunch");
}

void tst_IrcMessage::testPartMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, channel);
    QFETCH(QString, reason);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Part);
    QCOMPARE(message->command(), QString("PART"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("channel").toString(), channel);
    QCOMPARE(message->property("reason").toString(), reason);

    IrcPartMessage* partMessage = qobject_cast<IrcPartMessage*>(message);
    QVERIFY(partMessage);
    QCOMPARE(partMessage->isValid(), valid);
    QCOMPARE(partMessage->channel(), channel);
    QCOMPARE(partMessage->reason(), reason);
}

void tst_IrcMessage::testPingMessage()
{
    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData("PING :arg", &connection);
    QCOMPARE(message->type(), IrcMessage::Ping);
    QCOMPARE(message->command(), QString("PING"));
    QCOMPARE(message->property("command").toString(), QString("PING"));
    QVERIFY(message->property("valid").toBool());
    QCOMPARE(message->property("argument").toString(), QString("arg"));

    IrcPingMessage* pingMessage = qobject_cast<IrcPingMessage*>(message);
    QVERIFY(pingMessage);
    QVERIFY(pingMessage->isValid());
    QCOMPARE(pingMessage->argument(), QString("arg"));
}

void tst_IrcMessage::testPongMessage()
{
    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData("PONG tgt :arg", &connection);
    QCOMPARE(message->type(), IrcMessage::Pong);
    QCOMPARE(message->command(), QString("PONG"));
    QCOMPARE(message->property("command").toString(), QString("PONG"));
    QVERIFY(message->property("valid").toBool());
    QCOMPARE(message->property("argument").toString(), QString("arg"));

    IrcPongMessage* pongMessage = qobject_cast<IrcPongMessage*>(message);
    QVERIFY(pongMessage);
    QVERIFY(pongMessage->isValid());
    QCOMPARE(pongMessage->argument(), QString("arg"));
}

void tst_IrcMessage::testPrivateMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QString>("cap");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("target");
    QTest::addColumn<QString>("content");
    QTest::addColumn<bool>("priv");
    QTest::addColumn<bool>("action");
    QTest::addColumn<bool>("request");
    QTest::addColumn<uint>("flags");

    QTest::newRow("no prefix") << true << QString() << QByteArray("PRIVMSG Wiz :Hello are you receiving this message ?") << QString("Wiz") << QString("Hello are you receiving this message ?") << false << false << false << static_cast<uint>(IrcMessage::None);
    QTest::newRow("empty prefix") << false << QString() << QByteArray(": PRIVMSG Wiz :Hello are you receiving this message ?") << QString("Wiz") << QString("Hello are you receiving this message ?") << false << false << false << static_cast<uint>(IrcMessage::None);
    QTest::newRow("no params") << false << QString() << QByteArray(":Angel PRIVMSG Wiz") << QString("Wiz") << QString() << false << false << false << static_cast<uint>(IrcMessage::None);
    QTest::newRow("all ok") << true << QString() << QByteArray(":Angel PRIVMSG Wiz :Hello are you receiving this message ?") << QString("Wiz") << QString("Hello are you receiving this message ?") << false << false << false << static_cast<uint>(IrcMessage::None);
    QTest::newRow("private") << true << QString() << QByteArray(":Angel PRIVMSG communi :Hello are you receiving this message ?") << QString("communi") << QString("Hello are you receiving this message ?") << true << false << false << static_cast<uint>(IrcMessage::None);
    QTest::newRow("action") << true << QString() << QByteArray(":Angel PRIVMSG Wiz :\1ACTION Hello are you receiving this message ?\1") << QString("Wiz") << QString("Hello are you receiving this message ?") << false << true << false << static_cast<uint>(IrcMessage::None);
    QTest::newRow("request") << true << QString() << QByteArray(":Angel PRIVMSG Wiz :\1Hello are you receiving this message ?\1") << QString("Wiz") << QString("Hello are you receiving this message ?") << false << false << true << static_cast<uint>(IrcMessage::None);
}

class TestProtocol : public IrcProtocol
{
public:
    TestProtocol(const QString& cap, IrcConnection* connection) : IrcProtocol(connection)
    {
        QSet<QString> caps;
        caps.insert(cap);
        setAvailableCapabilities(caps);
        setActiveCapabilities(caps);
    }
};

class FriendConnection : public IrcConnection
{
    friend class tst_IrcMessage;
};

void tst_IrcMessage::testPrivateMessage()
{
    QFETCH(bool, valid);
    QFETCH(QString, cap);
    QFETCH(QByteArray, data);
    QFETCH(QString, target);
    QFETCH(QString, content);
    QFETCH(bool, priv);
    QFETCH(bool, action);
    QFETCH(bool, request);
    QFETCH(uint, flags);

    IrcConnection connection;
    connection.setNickName("communi");
    TestProtocol protocol(cap, &connection);
    static_cast<FriendConnection*>(&connection)->setProtocol(&protocol);

    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Private);
    QCOMPARE(message->command(), QString("PRIVMSG"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("target").toString(), target);
    QCOMPARE(message->property("content").toString(), content);
    QCOMPARE(message->property("private").toBool(), priv);
    QCOMPARE(message->property("action").toBool(), action);
    QCOMPARE(message->property("request").toBool(), request);
    QCOMPARE(message->property("flags").toUInt(), flags);

    IrcPrivateMessage* privateMessage = qobject_cast<IrcPrivateMessage*>(message);
    QVERIFY(privateMessage);
    QCOMPARE(privateMessage->isValid(), valid);
    QCOMPARE(privateMessage->target(), target);
    QCOMPARE(privateMessage->content(), content);
    QCOMPARE(privateMessage->isPrivate(), priv);
    QCOMPARE(privateMessage->isAction(), action);
    QCOMPARE(privateMessage->isRequest(), request);
    QCOMPARE(static_cast<uint>(privateMessage->flags()), flags);
}

void tst_IrcMessage::testQuitMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("reason");

    QTest::newRow("no prefix") << true << QByteArray("QUIT :Gone to have lunch") << QString("Gone to have lunch");
    QTest::newRow("empty prefix") << false << QByteArray(": QUIT :Gone to have lunch") << QString("Gone to have lunch");
    QTest::newRow("no params") << true << QByteArray(":WiZ QUIT") << QString();
    QTest::newRow("all ok") << true << QByteArray(":WiZ QUIT :Gone to have lunch") << QString("Gone to have lunch");
}

void tst_IrcMessage::testQuitMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, reason);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Quit);
    QCOMPARE(message->command(), QString("QUIT"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("reason").toString(), reason);

    IrcQuitMessage* quitMessage = qobject_cast<IrcQuitMessage*>(message);
    QVERIFY(quitMessage);
    QCOMPARE(quitMessage->isValid(), valid);
    QCOMPARE(quitMessage->reason(), reason);
}

void tst_IrcMessage::testTopicMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("channel");
    QTest::addColumn<QString>("topic");
    QTest::addColumn<bool>("reply");

    QTest::newRow("no prefix") << true << QByteArray("TOPIC #test") << QString("#test") << QString() << false;
    QTest::newRow("empty prefix") << false << QByteArray(": TOPIC #test") << QString("#test") << QString() << false;
    QTest::newRow("no params") << false << QByteArray(":WiZ TOPIC") << QString() << QString() << false;
    QTest::newRow("no topic") << true << QByteArray(":WiZ TOPIC #test") << QString("#test") << QString() << false;
    QTest::newRow("all ok") << true << QByteArray(":WiZ TOPIC #test :another topic") << QString("#test") << QString("another topic") << false;
    // TODO: QTest::newRow("numeric") << true << QByteArray(":server 332 user #test :foo bar") << QString("#test") << QString("foo bar") << true;
}

void tst_IrcMessage::testTopicMessage()
{
    QFETCH(bool, valid);
    QFETCH(QByteArray, data);
    QFETCH(QString, channel);
    QFETCH(QString, topic);
    QFETCH(bool, reply);

    IrcConnection connection;
    IrcMessage* message = IrcMessage::fromData(data, &connection);
    QCOMPARE(message->type(), IrcMessage::Topic);
    QCOMPARE(message->command(), QString("TOPIC"));
    QCOMPARE(message->property("valid").toBool(), valid);
    QCOMPARE(message->property("channel").toString(), channel);
    QCOMPARE(message->property("topic").toString(), topic);
    QCOMPARE(message->property("reply").toBool(), reply);

    IrcTopicMessage* topicMessage = qobject_cast<IrcTopicMessage*>(message);
    QVERIFY(topicMessage);
    QCOMPARE(topicMessage->isValid(), valid);
    QCOMPARE(topicMessage->channel(), channel);
    QCOMPARE(topicMessage->topic(), topic);
    QCOMPARE(topicMessage->isReply(), reply);
}

void tst_IrcMessage::testWhoReplyMessage_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QStringList>("params");
    QTest::addColumn<QString>("mask");
    QTest::addColumn<QString>("server");
    QTest::addColumn<bool>("away");
    QTest::addColumn<bool>("servOp");
    QTest::addColumn<QString>("realName");

    QTest::newRow("normal") << true << "nick!ident@host"
                            << (QStringList() << "#mask" << "irc.ser.ver" << "H@" << "real name")
                            << "#mask" << "irc.ser.ver" << false << false << "real name";

    QTest::newRow("away") << true << "nick!ident@host"
                          << (QStringList() << "*" << "127.0.0.1" << "G@" << "real name")
                          << "*" << "127.0.0.1" << true << false << "real name";

    QTest::newRow("serv op") << true << "nick!ident@host"
                             << (QStringList() << "*" << "127.0.0.1" << "H*@" << "real name")
                             << "*" << "127.0.0.1" << false << true << "real name";

    QTest::newRow("no name") << true << "nick!ident@host"
                             << (QStringList() << "#mask" << "irc.ser.ver" << "H@" << "")
                             << "#mask" << "irc.ser.ver" << false << false << "";
}

void tst_IrcMessage::testWhoReplyMessage()
{
    QFETCH(bool, valid);
    QFETCH(QString, prefix);
    QFETCH(QStringList, params);
    QFETCH(QString, mask);
    QFETCH(QString, server);
    QFETCH(bool, away);
    QFETCH(bool, servOp);
    QFETCH(QString, realName);

    IrcConnection connection;
    IrcWhoReplyMessage message(&connection);
    message.setPrefix(prefix);
    message.setParameters(params);
    QCOMPARE(message.isValid(), valid);
    QCOMPARE(message.type(), IrcMessage::WhoReply);
    // TODO: QCOMPARE(message.command(), QString::number(Irc::RPL_WHOREPLY));
    QCOMPARE(message.mask(), mask);
    QCOMPARE(message.server(), server);
    QCOMPARE(message.isAway(), away);
    QCOMPARE(message.isServOp(), servOp);
    QCOMPARE(message.realName(), realName);

    QCOMPARE(message.property("valid").toBool(), valid);
    QCOMPARE(message.property("mask").toString(), mask);
    QCOMPARE(message.property("server").toString(), server);
    QCOMPARE(message.property("away").toBool(), away);
    QCOMPARE(message.property("servOp").toBool(), servOp);
    QCOMPARE(message.property("realName").toString(), realName);
}

void tst_IrcMessage::testClone()
{
    IrcConnection connection;
    IrcMessage* pm = IrcMessage::fromData("@a=b;c=d :nick!ident@host PRIVMSG me :hello", &connection);
    QVERIFY(pm);
    IrcMessage* clone = pm->clone(this);
    QVERIFY(clone);
    QVERIFY(clone->isValid());
    QCOMPARE(clone->parent(), this);
    QCOMPARE(clone->connection(), &connection);
    QCOMPARE(clone->type(), pm->type());
    QCOMPARE(clone->flags(), pm->flags());
    QCOMPARE(clone->tags(), pm->tags());
    QCOMPARE(clone->prefix(), pm->prefix());
    QCOMPARE(clone->nick(), pm->nick());
    QCOMPARE(clone->ident(), pm->ident());
    QCOMPARE(clone->host(), pm->host());
    QCOMPARE(clone->command(), pm->command());
    QCOMPARE(clone->parameters(), pm->parameters());
    QCOMPARE(clone->timeStamp(), pm->timeStamp());
    QCOMPARE(clone->account(), pm->account());
}

void tst_IrcMessage::testNullConnection()
{
    IrcMessage* pm = IrcMessage::fromData(":nick!ident@host PRIVMSG me :hello", 0);
    QCOMPARE(pm->type(), IrcMessage::Private);
    QCOMPARE(pm->property("target").toString(), QString("me"));
    QVERIFY(pm->property("statusPrefix").toString().isEmpty());
    QVERIFY(!pm->property("private").toBool());
    QCOMPARE(pm->flags(), IrcMessage::None);
    delete pm;

    IrcMessage* nm = IrcMessage::fromData(":nick!ident@host NOTICE me :hello", 0);
    QCOMPARE(nm->type(), IrcMessage::Notice);
    QCOMPARE(nm->property("target").toString(), QString("me"));
    QVERIFY(nm->property("statusPrefix").toString().isEmpty());
    QVERIFY(!nm->property("private").toBool());
    QCOMPARE(nm->flags(), IrcMessage::None);
    delete nm;
}

void tst_IrcMessage::testDebug()
{
    QString str;
    QDebug dbg(&str);

    dbg << static_cast<IrcMessage*>(0);
    QCOMPARE(str.trimmed(), QString::fromLatin1("IrcMessage(0x0)"));
    str.clear();

    IrcMessage message(0);
    dbg << &message;
    QVERIFY(QRegExp("IrcMessage\\(0x[0-9A-Fa-f]+, flags=\\(None\\)\\) ").exactMatch(str));
    str.clear();

    message.setObjectName("foo");
    dbg << &message;
    QVERIFY(QRegExp("IrcMessage\\(0x[0-9A-Fa-f]+, name=foo, flags=\\(None\\)\\) ").exactMatch(str));
    str.clear();

    message.setPrefix("nick!ident@host");
    dbg << &message;
    QVERIFY(QRegExp("IrcMessage\\(0x[0-9A-Fa-f]+, name=foo, flags=\\(None\\), prefix=nick!ident@host\\) ").exactMatch(str));
    str.clear();

    message.setCommand("COMMAND");
    dbg << &message;
    QVERIFY(QRegExp("IrcMessage\\(0x[0-9A-Fa-f]+, name=foo, flags=\\(None\\), prefix=nick!ident@host, command=COMMAND\\) ").exactMatch(str));
    str.clear();

    message.setFlags(IrcMessage::Own | IrcMessage::Playback | IrcMessage::Implicit);
    dbg << &message;
    QVERIFY(QRegExp("IrcMessage\\(0x[0-9A-Fa-f]+, name=foo, flags=\\(Own\\|Playback\\|Implicit\\), prefix=nick!ident@host, command=COMMAND\\) ").exactMatch(str));
    str.clear();

    dbg << IrcMessage::None;
    QCOMPARE(str.trimmed(), QString::fromLatin1("None"));
    str.clear();

    dbg << IrcMessage::Join;
    QCOMPARE(str.trimmed(), QString::fromLatin1("Join"));
    str.clear();

    dbg << IrcModeMessage::Channel;
    QCOMPARE(str.trimmed(), QString::fromLatin1("Channel"));
    str.clear();
}

QTEST_MAIN(tst_IrcMessage)

#include "tst_ircmessage.moc"
