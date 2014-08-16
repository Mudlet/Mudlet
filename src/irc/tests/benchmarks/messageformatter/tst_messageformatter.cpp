/*
 * Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
 *
 * This test is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include "messageformatter.h"
#include "usermodel.h"
#include "session.h"
#include <QtTest/QtTest>
#include <QtCore/QStringList>

static const QString MSG_32_5("Vestibulum eu libero eget metus.");
static const QString MSG_64_9("Phasellus enim dui, sodales sed tincidunt quis, ultricies metus.");
static const QString MSG_128_19("Ut porttitor volutpat tristique. Aenean semper ligula eget nulla condimentum tempor in quis felis. Sed sem diam, tincidunt amet.");
static const QString MSG_256_37("Vestibulum quis lorem velit, a varius augue. Suspendisse risus augue, ultricies at convallis in, elementum in velit. Fusce fermentum congue augue sit amet dapibus. Fusce ultrices urna ut tortor laoreet a aliquet elit lobortis. Suspendisse volutpat posuere.");
static const QString MSG_512_75("Nam leo risus, accumsan a sagittis eget, posuere eu velit. Morbi mattis auctor risus, vel consequat massa pulvinar nec. Proin aliquam convallis elit nec egestas. Pellentesque accumsan placerat augue, id volutpat nibh dictum vel. Aenean venenatis varius feugiat. Nullam molestie, ipsum id dignissim vulputate, eros urna vestibulum massa, in vehicula lacus nisi vitae risus. Ut nunc nunc, venenatis a mattis auctor, dictum et sem. Nulla posuere libero ut tortor elementum egestas. Aliquam egestas suscipit posuere.");

static const QStringList USERS_100 = QStringList()
        << "jpnurmi" << "Curabitur" << "nORMAL" << "leo" << "luctus"
        << "luctus" << "paraply" << "sapien" << "nope" << "vitae"
        << "where" << "alien" << "urna" << "turpis" << "rea_son"
        << "alone" << "velit" << "jipsu" << "rutrum" << "DiSCO"
        << "abort" << "venue" << "__duis__" << "eros" << "adam"
        << "hendrix" << "liber0" << "Jim" << "Leonardo" << "JiMi"
        << "justin" << "semper" << "fidelis" << "Excelsior" << "parachute"
        << "nam" << "nom" << "Lorem" << "risus" << "stereo"
        << "tv" << "what-ever" << "kill" << "savior" << "[haha]"
        << "null" << "nill" << "will" << "still" << "ill"
        << "fill" << "hill" << "bill" << "Lucas" << "metus"
        << "bitch" << "d0nut" << "perverT" << "br0tha" << "WHYZ"
        << "Amen" << "hero" << "another" << "other" << "augue"
        << "Vestibulum" << "quit" << "quis" << "Luis" << "luiz"
        << "Luigi" << "anus" << "formal" << "f00bar" << "sed"
        << "sodales" << "phasellus" << "port" << "porttitor" << "absolute"
        << "varius" << "Marius" << "access" << "ZzZzZz" << "dust"
        << "up" << "d0wn" << "l3ft" << "r1ght" << "n0ne"
        << "MeSsAgE" << "FoRmAtTeR" << "c00l" << "_[KiDDO]_" << "yes"
        << "no" << "never" << "tincidunt" << "ultricies" << "posuere";

class TestMessageFormatter : public MessageFormatter
{
    friend class tst_MessageFormatter;
};

class TestUserModel : public UserModel
{
    friend class tst_MessageFormatter;
public:
    TestUserModel(Session* session) : UserModel(session) { }
};

class tst_MessageFormatter : public QObject
{
    Q_OBJECT

private slots:
    void testFormatHtml_data();
    void testFormatHtml();

private:
    TestMessageFormatter formatter;
};

Q_DECLARE_METATYPE(QStringList)
void tst_MessageFormatter::testFormatHtml_data()
{
    qRegisterMetaType<QStringList>();

    QStringList USERS_UC;
    foreach (const QString& user, USERS_100)
        USERS_UC += user.toUpper();

    QTest::addColumn<QString>("message");
    QTest::addColumn<QStringList>("users");

    QTest::newRow("empty") << QString() << QStringList();

    QTest::newRow("32 chars / 5 words / 0 users") << MSG_32_5 << QStringList();
    QTest::newRow("32 chars / 5 words / 25 users") << MSG_32_5 << QStringList(USERS_100.mid(0, 25));
    QTest::newRow("32 chars / 5 words / 50 users") << MSG_32_5 << QStringList(USERS_100.mid(0, 50));
    QTest::newRow("32 chars / 5 words / 75 users") << MSG_32_5 << QStringList(USERS_100.mid(0, 75));
    QTest::newRow("32 chars / 5 words / 100 users") << MSG_32_5 << USERS_100;
    QTest::newRow("32 chars / 5 words / 200 users") << MSG_32_5 << USERS_100 + USERS_UC;

    QTest::newRow("64 chars / 9 words / 0 users")  << MSG_64_9 << QStringList();
    QTest::newRow("64 chars / 9 words / 25 users") << MSG_64_9 << QStringList(USERS_100.mid(0, 25));
    QTest::newRow("64 chars / 9 words / 50 users") << MSG_64_9 << QStringList(USERS_100.mid(0, 50));
    QTest::newRow("64 chars / 9 words / 75 users") << MSG_64_9 << QStringList(USERS_100.mid(0, 75));
    QTest::newRow("64 chars / 9 words / 100 users") << MSG_64_9 << USERS_100;
    QTest::newRow("64 chars / 9 words / 200 users") << MSG_64_9 << USERS_100 + USERS_UC;

    QTest::newRow("128 chars / 19 words / 0 users")  << MSG_128_19 << QStringList();
    QTest::newRow("128 chars / 19 words / 25 users") << MSG_128_19 << QStringList(USERS_100.mid(0, 25));
    QTest::newRow("128 chars / 19 words / 50 users") << MSG_128_19 << QStringList(USERS_100.mid(0, 50));
    QTest::newRow("128 chars / 19 words / 75 users") << MSG_128_19 << QStringList(USERS_100.mid(0, 75));
    QTest::newRow("128 chars / 19 words / 100 users") << MSG_128_19 << USERS_100;
    QTest::newRow("128 chars / 19 words / 200 users") << MSG_128_19 << USERS_100 + USERS_UC;

    QTest::newRow("256 chars / 37 words / 0 users")  << MSG_256_37 << QStringList();
    QTest::newRow("256 chars / 37 words / 25 users") << MSG_256_37 << QStringList(USERS_100.mid(0, 25));
    QTest::newRow("256 chars / 37 words / 50 users") << MSG_256_37 << QStringList(USERS_100.mid(0, 50));
    QTest::newRow("256 chars / 37 words / 75 users") << MSG_256_37 << QStringList(USERS_100.mid(0, 75));
    QTest::newRow("256 chars / 37 words / 100 users") << MSG_256_37 << USERS_100;
    QTest::newRow("256 chars / 37 words / 200 users") << MSG_256_37 << USERS_100 + USERS_UC;

    QTest::newRow("512 chars / 75 words / 0 users")  << MSG_512_75 << QStringList();
    QTest::newRow("512 chars / 75 words / 25 users") << MSG_512_75 << QStringList(USERS_100.mid(0, 25));
    QTest::newRow("512 chars / 75 words / 50 users") << MSG_512_75 << QStringList(USERS_100.mid(0, 50));
    QTest::newRow("512 chars / 75 words / 75 users") << MSG_512_75 << QStringList(USERS_100.mid(0, 75));
    QTest::newRow("512 chars / 75 words / 100 users") << MSG_512_75 << USERS_100;
    QTest::newRow("512 chars / 75 words / 200 users") << MSG_512_75 << USERS_100 + USERS_UC;
}

void tst_MessageFormatter::testFormatHtml()
{
    QFETCH(QString, message);
    QFETCH(QStringList, users);

    Session session;
    TestUserModel model(&session);
    model.addUsers(users);
    QCOMPARE(model.rowCount(), users.count());

    IrcMessage dummy;
    formatter.formatMessage(&dummy, &model);

    QBENCHMARK {
        formatter.formatHtml(message);
    }
}

QTEST_MAIN(tst_MessageFormatter)

#include "tst_messageformatter.moc"
