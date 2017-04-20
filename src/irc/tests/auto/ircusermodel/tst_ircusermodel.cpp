/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "ircusermodel.h"
#include "ircconnection.h"
#include "ircbuffermodel.h"
#include "ircchannel.h"
#include "ircuser.h"
#include "irc.h"

#include "tst_ircdata.h"
#include "tst_ircclientserver.h"

#include <QtTest/QtTest>

static bool caseInsensitiveLessThan(const QString& s1, const QString& s2)
{
    return s1.compare(s2, Qt::CaseInsensitive) < 0;
}

static bool caseInsensitiveGreaterThan(const QString& s1, const QString& s2)
{
    return s1.compare(s2, Qt::CaseInsensitive) > 0;
}

class tst_IrcUserModel : public tst_IrcClientServer
{
    Q_OBJECT

public:
    tst_IrcUserModel();

private slots:
    void testDefaults();
    void testClear();
    void testSorting_data();
    void testSorting();
    void testActivity_freenode();
    void testActivity_ircnet();
    void testActivity_euirc();
    void testChanges();
    void testRoles();
    void testAIM();
    void testUser();
};

Q_DECLARE_METATYPE(QModelIndex)
tst_IrcUserModel::tst_IrcUserModel()
{
    Irc::registerMetaTypes();
    qRegisterMetaType<QModelIndex>();
    qRegisterMetaType<IrcUser*>("IrcUser*");
    qRegisterMetaType<IrcChannel*>("IrcChannel*");
    qRegisterMetaType<QList<IrcUser*> >("QList<IrcUser*>");
}

void tst_IrcUserModel::testDefaults()
{
    IrcUserModel model;
    QCOMPARE(model.count(), 0);
    QVERIFY(model.isEmpty());
    QVERIFY(model.names().isEmpty());
    QVERIFY(model.users().isEmpty());
    QCOMPARE(model.displayRole(), Irc::TitleRole);
    QVERIFY(!model.channel());
    QCOMPARE(model.sortMethod(), Irc::SortByHand);
    QCOMPARE(model.sortOrder(), Qt::AscendingOrder);
}

void tst_IrcUserModel::testClear()
{
    IrcBufferModel bufferModel;
    bufferModel.setConnection(connection);

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(waitForWritten(tst_IrcData::welcome()));
    QCOMPARE(bufferModel.count(), 0);
    waitForWritten(":communi!communi@hidd.en JOIN :#channel");
    QCOMPARE(bufferModel.count(), 1);

    QPointer<IrcChannel> channel = bufferModel.get(0)->toChannel();
    QVERIFY(channel);

    IrcUserModel userModel;
    userModel.setChannel(channel);
    waitForWritten(":irc.ser.ver 353 communi = #channel :a @b +c");
    waitForWritten(":irc.ser.ver 366 communi #channel :End of /NAMES list.");
    QCOMPARE(userModel.count(), 3);

    QPointer<IrcUser> a = userModel.find("a");
    QPointer<IrcUser> b = userModel.find("b");
    QPointer<IrcUser> c = userModel.find("c");

    QVERIFY(a);
    QVERIFY(b);
    QVERIFY(c);

    QSignalSpy countSpy(&userModel, SIGNAL(countChanged(int)));
    QSignalSpy emptySpy(&userModel, SIGNAL(emptyChanged(bool)));
    QSignalSpy usersSpy(&userModel, SIGNAL(usersChanged(QList<IrcUser*>)));
    QSignalSpy namesSpy(&userModel, SIGNAL(namesChanged(QStringList)));
    QSignalSpy modelAboutToBeResetSpy(&userModel, SIGNAL(modelAboutToBeReset()));
    QSignalSpy modelResetSpy(&userModel, SIGNAL(modelReset()));

    QVERIFY(countSpy.isValid());
    QVERIFY(emptySpy.isValid());
    QVERIFY(usersSpy.isValid());
    QVERIFY(namesSpy.isValid());
    QVERIFY(modelAboutToBeResetSpy.isValid());
    QVERIFY(modelResetSpy.isValid());

    userModel.clear();

    QCOMPARE(userModel.count(), 0);
    QVERIFY(userModel.isEmpty());
    QCOMPARE(userModel.users(), QList<IrcUser*>());
    QCOMPARE(userModel.names(), QStringList());

    QVERIFY(!userModel.get(0));
    QVERIFY(!userModel.find("a"));
    QVERIFY(!userModel.find("b"));
    QVERIFY(!userModel.find("c"));
    QVERIFY(!userModel.contains("a"));
    QVERIFY(!userModel.contains("b"));
    QVERIFY(!userModel.contains("c"));

    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(countSpy.last().at(0).toInt(), 0);

    QCOMPARE(emptySpy.count(), 1);
    QCOMPARE(emptySpy.last().at(0).toBool(), true);

    QCOMPARE(usersSpy.count(), 1);
    QCOMPARE(usersSpy.last().at(0).value<QList<IrcUser*> >(), QList<IrcUser*>());

    QCOMPARE(namesSpy.count(), 1);
    QCOMPARE(namesSpy.last().at(0).toStringList(), QStringList());

    QCOMPARE(modelAboutToBeResetSpy.count(), 1);
    QCOMPARE(modelResetSpy.count(), 1);

    QVERIFY(a);
    QVERIFY(b);
    QVERIFY(c);

    bufferModel.clear();

    QVERIFY(!a);
    QVERIFY(!b);
    QVERIFY(!c);
}

void tst_IrcUserModel::testSorting_data()
{
    QTest::addColumn<QByteArray>("welcomeData");
    QTest::addColumn<QByteArray>("joinData");
    QTest::addColumn<QStringList>("names");
    QTest::addColumn<QStringList>("admins");
    QTest::addColumn<QStringList>("ops");
    QTest::addColumn<QStringList>("halfops");
    QTest::addColumn<QStringList>("voices");

    foreach (const QByteArray& key, tst_IrcData::keys()) {
        QTest::newRow(key)
            << tst_IrcData::welcome(key)
            << tst_IrcData::join(key)
            << tst_IrcData::names(key)
            << tst_IrcData::admins(key)
            << tst_IrcData::ops(key)
            << tst_IrcData::halfops(key)
            << tst_IrcData::voices(key);
    }
}

void tst_IrcUserModel::testSorting()
{
    QFETCH(QByteArray, welcomeData);
    QFETCH(QByteArray, joinData);
    QFETCH(QStringList, names);
    QFETCH(QStringList, admins);
    QFETCH(QStringList, ops);
    QFETCH(QStringList, halfops);
    QFETCH(QStringList, voices);

    IrcBufferModel bufferModel;
    bufferModel.setConnection(connection);

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(waitForWritten(welcomeData));
    QCOMPARE(bufferModel.count(), 0);

    QVERIFY(waitForWritten(joinData));

    QCOMPARE(bufferModel.count(), 1);
    IrcChannel* channel = bufferModel.get(0)->toChannel();
    QVERIFY(channel);

    IrcUserModel staticModel(channel);
    QCOMPARE(staticModel.count(), names.count());
    for (int i = 0; i < staticModel.count(); ++i) {
        IrcUser* user = staticModel.get(i);
        QCOMPARE(user->name(), names.at(i));
        if (admins.contains(user->name())) {
            QCOMPARE(user->mode(), QString("a"));
            QCOMPARE(user->prefix(), QString("!"));
        } else if (ops.contains(user->name())) {
            QCOMPARE(user->mode(), QString("o"));
            QCOMPARE(user->prefix(), QString("@"));
        } else if (halfops.contains(user->name())) {
            QCOMPARE(user->mode(), QString("h"));
            QCOMPARE(user->prefix(), QString("%"));
        } else if (voices.contains(user->name())) {
            QCOMPARE(user->mode(), QString("v"));
            QCOMPARE(user->prefix(), QString("+"));
        }
    }

    QStringList sorted = names;
    qSort(sorted);
    QCOMPARE(staticModel.names(), sorted);

    // IGNORE INVALID COLUMNS
    staticModel.sort(-1, Qt::AscendingOrder);
    for (int i = 0; i < staticModel.count(); ++i)
        QCOMPARE(staticModel.get(i)->name(), names.at(i));

    staticModel.sort(1, Qt::AscendingOrder);
    for (int i = 0; i < staticModel.count(); ++i)
        QCOMPARE(staticModel.get(i)->name(), names.at(i));

    // STATIC - BY NAME - ASCENDING
    staticModel.setSortMethod(Irc::SortByName);
    staticModel.sort(0, Qt::AscendingOrder);

    QStringList nasc = names;
    qSort(nasc.begin(), nasc.end(), caseInsensitiveLessThan);

    for (int i = 0; i < staticModel.count(); ++i)
        QCOMPARE(staticModel.get(i)->name(), nasc.at(i));

    // STATIC - BY NAME - DESCENDING
    staticModel.setSortMethod(Irc::SortByName);
    staticModel.sort(0, Qt::DescendingOrder);

    QStringList ndesc = names;
    qSort(ndesc.begin(), ndesc.end(), caseInsensitiveGreaterThan);

    for (int i = 0; i < staticModel.count(); ++i)
        QCOMPARE(staticModel.get(i)->name(), ndesc.at(i));

    // STATIC - BY TITLE - ASCENDING
    staticModel.setSortMethod(Irc::SortByTitle);
    staticModel.sort(0, Qt::AscendingOrder);

    QStringList aasc = admins;
    qSort(aasc.begin(), aasc.end(), caseInsensitiveLessThan);

    QStringList oasc = ops;
    qSort(oasc.begin(), oasc.end(), caseInsensitiveLessThan);

    QStringList hasc = halfops;
    qSort(hasc.begin(), hasc.end(), caseInsensitiveLessThan);

    QStringList vasc = voices;
    qSort(vasc.begin(), vasc.end(), caseInsensitiveLessThan);

    QStringList tasc = aasc + oasc + hasc + vasc + nasc;
    // remove duplicates
    foreach (const QString& voice, voices)
        tasc.removeAt(tasc.lastIndexOf(voice));
    foreach (const QString& halfop, halfops)
        tasc.removeAt(tasc.lastIndexOf(halfop));
    foreach (const QString& op, ops)
        tasc.removeAt(tasc.lastIndexOf(op));
    foreach (const QString& admin, admins)
        tasc.removeAt(tasc.lastIndexOf(admin));

    for (int i = 0; i < staticModel.count(); ++i)
        QCOMPARE(staticModel.get(i)->name(), tasc.at(i));

    // STATIC - BY TITLE - DESCENDING
    staticModel.setSortMethod(Irc::SortByTitle);
    staticModel.sort(0, Qt::DescendingOrder);

    QStringList adesc = admins;
    qSort(adesc.begin(), adesc.end(), caseInsensitiveGreaterThan);

    QStringList odesc = ops;
    qSort(odesc.begin(), odesc.end(), caseInsensitiveGreaterThan);

    QStringList hdesc = halfops;
    qSort(hdesc.begin(), hdesc.end(), caseInsensitiveGreaterThan);

    QStringList vdesc = voices;
    qSort(vdesc.begin(), vdesc.end(), caseInsensitiveGreaterThan);

    QStringList tdesc = ndesc + vdesc + hdesc + odesc + adesc;
    // remove duplicates
    foreach (const QString& voice, voices)
        tdesc.removeAt(tdesc.indexOf(voice));
    foreach (const QString& halfop, halfops)
        tdesc.removeAt(tdesc.indexOf(halfop));
    foreach (const QString& op, ops)
        tdesc.removeAt(tdesc.indexOf(op));
    foreach (const QString& admin, admins)
        tdesc.removeAt(tdesc.indexOf(admin));

    for (int i = 0; i < staticModel.count(); ++i)
        QCOMPARE(staticModel.get(i)->name(), tdesc.at(i));

    // DYNAMIC - BY NAME, TITLE & ACTIVITY - ASCENDING
    IrcUserModel dynamicModel(channel);

    dynamicModel.setSortMethod(Irc::SortByName);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), nasc.at(i));

    dynamicModel.setSortMethod(Irc::SortByTitle);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), tasc.at(i));

    dynamicModel.setSortMethod(Irc::SortByActivity);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), names.at(i));

    // DYNAMIC - BY NAME, TITLE & ACTIVITY - DESCENDING
    dynamicModel.setSortOrder(Qt::DescendingOrder);

    dynamicModel.setSortMethod(Irc::SortByName);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), ndesc.at(i));

    dynamicModel.setSortMethod(Irc::SortByTitle);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), tdesc.at(i));

    dynamicModel.setSortMethod(Irc::SortByActivity);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), names.at(names.count() - 1 - i));

    // RESTORE USERS IN ASCENDING ORDER
    dynamicModel.setSortOrder(Qt::AscendingOrder);

    dynamicModel.setChannel(0);
    dynamicModel.setSortMethod(Irc::SortByName);
    dynamicModel.setChannel(channel);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), nasc.at(i));

    dynamicModel.setChannel(0);
    dynamicModel.setSortMethod(Irc::SortByTitle);
    dynamicModel.setChannel(channel);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), tasc.at(i));

    dynamicModel.setChannel(0);
    dynamicModel.setSortMethod(Irc::SortByActivity);
    dynamicModel.setChannel(channel);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), names.at(i));

    // RESTORE USERS IN DESCENDING ORDER
    dynamicModel.setSortOrder(Qt::DescendingOrder);

    dynamicModel.setChannel(0);
    dynamicModel.setSortMethod(Irc::SortByName);
    dynamicModel.setChannel(channel);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), ndesc.at(i));

    dynamicModel.setChannel(0);
    dynamicModel.setSortMethod(Irc::SortByTitle);
    dynamicModel.setChannel(channel);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), tdesc.at(i));

    dynamicModel.setChannel(0);
    dynamicModel.setSortMethod(Irc::SortByActivity);
    dynamicModel.setChannel(channel);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), names.at(names.count() - 1 - i));

    // DO NOTHING
    dynamicModel.sort(Irc::SortByHand);
    for (int i = 0; i < dynamicModel.count(); ++i)
        QCOMPARE(dynamicModel.get(i)->name(), names.at(names.count() - 1 - i));
}

void tst_IrcUserModel::testActivity_freenode()
{
    IrcBufferModel bufferModel;
    bufferModel.setConnection(connection);

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(waitForWritten(tst_IrcData::welcome("freenode")));
    QCOMPARE(bufferModel.count(), 0);

    QVERIFY(waitForWritten(tst_IrcData::join("freenode")));

    QCOMPARE(bufferModel.count(), 1);
    IrcChannel* channel = bufferModel.get(0)->toChannel();
    QVERIFY(channel);

    QStringList names = tst_IrcData::names("freenode");

    IrcUserModel activityModel(channel);
    activityModel.setSortMethod(Irc::SortByActivity);

    int count = names.count();

    QVERIFY(waitForWritten(":smurfy!~smurfy@hidd.en PART #freenode"));
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("smurfy"));

    QVERIFY(waitForWritten(":ToApolytoXaos!~ToApolyto@hidd.en QUIT :Quit: Leaving"));
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("ToApolytoXaos"));

    QVERIFY(waitForWritten(":agsrv!~guest@hidd.en JOIN #freenode"));
    QCOMPARE(activityModel.count(), ++count);
    QCOMPARE(activityModel.indexOf(activityModel.find("agsrv")), 0);

    QVERIFY(waitForWritten(":Hello71!~Hello71@hidd.en PRIVMSG #freenode :straterra: there are many users on it"));
    QCOMPARE(activityModel.count(), count);
    QCOMPARE(activityModel.indexOf(activityModel.find("Hello71")), 0);
    QCOMPARE(activityModel.indexOf(activityModel.find("straterra")), 1);

    QVERIFY(waitForWritten(":straterra!straterra@hidd.en PRIVMSG #freenode :what?"));
    QCOMPARE(activityModel.count(), count);
    QCOMPARE(activityModel.indexOf(activityModel.find("straterra")), 0);
    QCOMPARE(activityModel.indexOf(activityModel.find("Hello71")), 1);

    QVERIFY(waitForWritten(":JuxTApose!~indigital@hidd.en NICK :JuxTApose_afk"));
    QCOMPARE(activityModel.count(), count);
    QVERIFY(!activityModel.contains("JuxTApose"));
    QCOMPARE(activityModel.indexOf(activityModel.find("JuxTApose_afk")), 0);
    QCOMPARE(activityModel.indexOf(activityModel.find("straterra")), 1);
    QCOMPARE(activityModel.indexOf(activityModel.find("Hello71")), 2);

    QVERIFY(waitForWritten(":communi!communi@hidd.en PRIVMSG #freenode :+tomaw: ping"));
    QCOMPARE(activityModel.count(), count);
    QCOMPARE(activityModel.indexOf(activityModel.find("communi")), 0);
    QCOMPARE(activityModel.indexOf(activityModel.find("tomaw")), 1);

    QVERIFY(waitForWritten(":Hello71!~Hello71@hidd.en NOTICE #freenode :just testing..."));
    QCOMPARE(activityModel.count(), count);
    QCOMPARE(activityModel.indexOf(activityModel.find("Hello71")), 0);
}

void tst_IrcUserModel::testActivity_ircnet()
{
    IrcBufferModel bufferModel;
    bufferModel.setConnection(connection);

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(waitForWritten(tst_IrcData::welcome("ircnet")));
    QCOMPARE(bufferModel.count(), 0);

    QVERIFY(waitForWritten(tst_IrcData::join("ircnet")));

    QCOMPARE(bufferModel.count(), 1);
    IrcChannel* channel = bufferModel.get(0)->toChannel();
    QVERIFY(channel);

    QStringList names = tst_IrcData::names("ircnet");

    IrcUserModel activityModel(channel);
    activityModel.setSortMethod(Irc::SortByActivity);

    int count = names.count();

    QVERIFY(waitForWritten(":_box!~box@hidd.en QUIT :Broken pipe"));
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("_box"));

    QVERIFY(waitForWritten(":ip!~v6@hidd.en QUIT :Connection reset by peer"));
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("ip"));

    QVERIFY(waitForWritten(":[m]!m@hidd.en MODE #uptimed +b *!*x@does.matter.not*"));
    QCOMPARE(activityModel.count(), count);

    QVERIFY(waitForWritten(":[m]!m@hidd.en KICK #uptimed \\x00 :lame exit sorry :P"));
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("\\x00"));

    QVERIFY(waitForWritten(":_box!~box@hidd.en JOIN :#uptimed"));
    QCOMPARE(activityModel.count(), ++count);
    QCOMPARE(activityModel.indexOf(activityModel.find("_box")), 0);

    QVERIFY(waitForWritten(":Voicer!mrozu@hidd.en MODE #uptimed +v _box"));
    QCOMPARE(activityModel.count(), count);

    QVERIFY(waitForWritten(":t0r-!t0r@hidd.en PRIVMSG #uptimed :there is no sense for _box and ip to join the contest"));
    QCOMPARE(activityModel.count(), count);
    QCOMPARE(activityModel.indexOf(activityModel.find("t0r-")), 0);

    QVERIFY(waitForWritten(":ip!~v6@hidd.en JOIN :#uptimed"));
    QCOMPARE(activityModel.count(), ++count);
    QCOMPARE(activityModel.indexOf(activityModel.find("ip")), 0);

    QVERIFY(waitForWritten(":Voicer!mrozu@hidd.en MODE #uptimed +v ip"));
    QCOMPARE(activityModel.count(), count);

    QVERIFY(waitForWritten(":[m]!m@hidd.en MODE #uptimed +b *!*v6@*.does.matter.not"));
    QCOMPARE(activityModel.count(), count);

    QVERIFY(waitForWritten(":[m]!m@hidd.en KICK #uptimed ip :no reason"));
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("ip"));

    QVERIFY(waitForWritten(":t0r-!t0r@hidd.en PRIVMSG #uptimed :they are going down every second"));
    QCOMPARE(activityModel.count(), count);
    QCOMPARE(activityModel.indexOf(activityModel.find("t0r-")), 0);

    QVERIFY(waitForWritten(":t0r-!t0r@hidd.en PRIVMSG #uptimed :yeah"));
    QCOMPARE(activityModel.count(), count);
    QCOMPARE(activityModel.indexOf(activityModel.find("t0r-")), 0);

    QVERIFY(waitForWritten(":[m]!m@hidd.en MODE #uptimed -b *!*box@*.does.not.matter"));
    QCOMPARE(activityModel.count(), count);

    QVERIFY(waitForWritten(":[m]!m@hidd.en KICK #uptimed _box :no reason"));
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("_box"));
}

void tst_IrcUserModel::testActivity_euirc()
{
    IrcBufferModel bufferModel;
    bufferModel.setConnection(connection);

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(waitForWritten(tst_IrcData::welcome("euirc")));
    QCOMPARE(bufferModel.count(), 0);

    QVERIFY(waitForWritten(tst_IrcData::join("euirc")));

    QCOMPARE(bufferModel.count(), 1);
    IrcChannel* channel = bufferModel.get(0)->toChannel();
    QVERIFY(channel);

    QStringList names = tst_IrcData::names("euirc");

    IrcUserModel activityModel(channel);
    activityModel.setSortMethod(Irc::SortByActivity);

    int count = names.count();

    QVERIFY(waitForWritten(":Marko10_000!~marko@hidd.en JOIN :#euirc\n"));
    QCOMPARE(activityModel.count(), ++count);
    QCOMPARE(activityModel.indexOf(activityModel.find("Marko10_000")), 0);

    QVERIFY(waitForWritten(":Marko10_000!~marko@hidd.en NICK :Guest775\n"));
    QCOMPARE(activityModel.count(), count);
    QVERIFY(!activityModel.contains("Marko10_000"));
    QCOMPARE(activityModel.indexOf(activityModel.find("Guest775")), 0);

    QVERIFY(waitForWritten(":Guest775!~marko@hidd.en QUIT :Quit: Verlassend\n"));
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("Guest775"));

    QVERIFY(waitForWritten(":Marko10_000!~marko@hidd.en JOIN :#euirc\n"));
    QCOMPARE(activityModel.count(), ++count);
    QCOMPARE(activityModel.indexOf(activityModel.find("Marko10_000")), 0);

    QVERIFY(waitForWritten(":Guest774!absk007@hidd.en QUIT :Quit: Good Bye. I Quit...\n"));
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("Guest774"));

    QVERIFY(waitForWritten(":absk007!absk007@hidd.en JOIN :#euirc\n"));
    QCOMPARE(activityModel.count(), ++count);
    QCOMPARE(activityModel.indexOf(activityModel.find("absk007")), 0);

    QVERIFY(waitForWritten(":charly6!~Miranda@hidd.en QUIT :Client exited\n"));
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("charly6"));

    QVERIFY(waitForWritten(":absk007!absk007@hidd.en NICK :Guest776\n"));
    QCOMPARE(activityModel.count(), count);
    QVERIFY(!activityModel.contains("absk007"));
    QCOMPARE(activityModel.indexOf(activityModel.find("Guest776")), 0);

    QVERIFY(waitForWritten(":Tina-chan_onAir!~kvirc@hidd.en NICK :Tina-chan\n"));
    QCOMPARE(activityModel.count(), count);
    QVERIFY(!activityModel.contains("Tina-chan_onAir"));
    QCOMPARE(activityModel.indexOf(activityModel.find("Tina-chan")), 0);

    QVERIFY(waitForWritten(":Guest776!absk007@hidd.en NICK :absk007\n"));
    QCOMPARE(activityModel.count(), count);
    QVERIFY(!activityModel.contains("Guest776"));
    QCOMPARE(activityModel.indexOf(activityModel.find("absk007")), 0);

    QVERIFY(waitForWritten(":aleksandr!~aleksandr@hidd.en PRIVMSG #euirc :absk007, last warning. fix your client/script\n"));
    QCOMPARE(activityModel.count(), count);
    QCOMPARE(activityModel.indexOf(activityModel.find("aleksandr")), 0);
    QCOMPARE(activityModel.indexOf(activityModel.find("absk007")), 1);

    QVERIFY(waitForWritten(":charly6!~Miranda@hidd.en JOIN :#euirc\n"));
    QCOMPARE(activityModel.count(), ++count);
    QCOMPARE(activityModel.indexOf(activityModel.find("charly6")), 0);

    QVERIFY(waitForWritten(":absk007!absk007@hidd.en PRIVMSG #euirc :aleksandr, what did i do this time?\n"));
    QCOMPARE(activityModel.count(), count);
    QCOMPARE(activityModel.indexOf(activityModel.find("absk007")), 0);
    QCOMPARE(activityModel.indexOf(activityModel.find("aleksandr")), 1);

    QVERIFY(waitForWritten(":aleksandr!~aleksandr@hidd.en PRIVMSG #euirc :if you need help, join #opers\n"));
    QCOMPARE(activityModel.count(), count);
    QCOMPARE(activityModel.indexOf(activityModel.find("aleksandr")), 0);

    QVERIFY(waitForWritten(":charly6!~Miranda@hidd.en QUIT :Client exited\n"));
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("charly6"));

    QVERIFY(waitForWritten(":icefly!~icefly@hidd.en PART #euirc :Once you know what it is you want to be true, instinct is a very useful device for enabling you to know that it is\n"));
    QCOMPARE(activityModel.count(), --count);
    QVERIFY(!activityModel.contains("icefly"));

    QVERIFY(waitForWritten(":icefly!~icefly@hidd.en JOIN :#euirc\n"));
    QCOMPARE(activityModel.count(), ++count);
    QCOMPARE(activityModel.indexOf(activityModel.find("icefly")), 0);
}

void tst_IrcUserModel::testChanges()
{
    IrcUserModel userModel;

    // IrcUserModel signals
    QSignalSpy addedSpy(&userModel, SIGNAL(added(IrcUser*)));
    QSignalSpy removedSpy(&userModel, SIGNAL(removed(IrcUser*)));
    QSignalSpy aboutToBeAddedSpy(&userModel, SIGNAL(aboutToBeAdded(IrcUser*)));
    QSignalSpy aboutToBeRemovedSpy(&userModel, SIGNAL(aboutToBeRemoved(IrcUser*)));
    QSignalSpy countChangedSpy(&userModel, SIGNAL(countChanged(int)));
    QSignalSpy namesChangedSpy(&userModel, SIGNAL(namesChanged(QStringList)));
    QSignalSpy usersChangedSpy(&userModel, SIGNAL(usersChanged(QList<IrcUser*>)));
    QSignalSpy channelChangedSpy(&userModel, SIGNAL(channelChanged(IrcChannel*)));

    QVERIFY(addedSpy.isValid());
    QVERIFY(removedSpy.isValid());
    QVERIFY(aboutToBeAddedSpy.isValid());
    QVERIFY(aboutToBeRemovedSpy.isValid());
    QVERIFY(countChangedSpy.isValid());
    QVERIFY(namesChangedSpy.isValid());
    QVERIFY(usersChangedSpy.isValid());
    QVERIFY(channelChangedSpy.isValid());

    int aboutToBeAddedCount = 0, addedCount = 0;
    int aboutToBeRemovedCount = 0, removedCount = 0;
    int countChangedCount = 0;
    int namesChangedCount = 0;
    int usersChangedCount = 0;
    int channelChangedCount = 0;

    // relevant QAbstractItemModel signals
    QSignalSpy dataChangedSpy(&userModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)));
    QSignalSpy modelAboutToBeResetSpy(&userModel, SIGNAL(modelAboutToBeReset()));
    QSignalSpy modelResetSpy(&userModel, SIGNAL(modelReset()));
    QSignalSpy layoutAboutToBeChangedSpy(&userModel, SIGNAL(layoutAboutToBeChanged()));
    QSignalSpy layoutChangedSpy(&userModel, SIGNAL(layoutChanged()));
    QSignalSpy rowsAboutToBeInsertedSpy(&userModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)));
    QSignalSpy rowsInsertedSpy(&userModel, SIGNAL(rowsInserted(QModelIndex,int,int)));
    QSignalSpy rowsAboutToBeRemovedSpy(&userModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)));
    QSignalSpy rowsRemovedSpy(&userModel, SIGNAL(rowsRemoved(QModelIndex,int,int)));

    QVERIFY(dataChangedSpy.isValid());
    QVERIFY(modelAboutToBeResetSpy.isValid());
    QVERIFY(modelResetSpy.isValid());
    QVERIFY(layoutAboutToBeChangedSpy.isValid());
    QVERIFY(layoutChangedSpy.isValid());
    QVERIFY(rowsAboutToBeInsertedSpy.isValid());
    QVERIFY(rowsInsertedSpy.isValid());
    QVERIFY(rowsAboutToBeRemovedSpy.isValid());
    QVERIFY(rowsRemovedSpy.isValid());

    int dataChangedCount = 0;
    int modelAboutToBeResetCount = 0, modelResetCount = 0;
    int layoutAboutToBeChangedCount = 0, layoutChangedCount = 0;
    int rowsAboutToBeInsertedCount = 0, rowsInsertedCount = 0;
    int rowsAboutToBeRemovedCount = 0, rowsRemovedCount = 0;

    // ### setup #communi (5): communi @ChanServ +qtassistant Guest1234 +qout
    IrcBufferModel bufferModel;
    bufferModel.setConnection(connection);

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(waitForWritten(tst_IrcData::welcome()));
    QVERIFY(waitForWritten(":communi!~communi@hidd.en JOIN :#communi"));
    QVERIFY(waitForWritten(":irc.ifi.uio.no 353 communi = #communi :communi @ChanServ +qtassistant Guest1234 +qout"));
    QVERIFY(waitForWritten(":irc.ifi.uio.no 366 communi #communi :End of NAMES list."));
    QCOMPARE(bufferModel.count(), 1);
    IrcChannel* channel = bufferModel.get(0)->toChannel();
    QVERIFY(channel);
    QCOMPARE(channel->title(), QString("#communi"));

    // ### ready to go!
    userModel.setChannel(channel);
    QCOMPARE(channelChangedSpy.count(), ++channelChangedCount);
    QCOMPARE(channelChangedSpy.last().at(0).value<IrcChannel*>(), channel);
    QCOMPARE(modelAboutToBeResetSpy.count(), ++modelAboutToBeResetCount);
    QCOMPARE(modelResetSpy.count(), ++modelResetCount);

    QStringList names = QStringList() << "communi" << "ChanServ" << "qtassistant" << "Guest1234" << "qout";
    QStringList titles = QStringList() << "communi" << "@ChanServ" << "+qtassistant" << "Guest1234" << "+qout";
    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
    }

    QPointer<IrcUser> communi = userModel.get(0);
    QVERIFY(communi);
    QCOMPARE(communi->name(), QString("communi"));
    QCOMPARE(communi->mode(), QString());
    QCOMPARE(communi->prefix(), QString());

    QPointer<IrcUser> ChanServ = userModel.get(1);
    QVERIFY(ChanServ);
    QCOMPARE(ChanServ->name(), QString("ChanServ"));
    QCOMPARE(ChanServ->mode(), QString("o"));
    QCOMPARE(ChanServ->prefix(), QString("@"));

    QPointer<IrcUser> qtassistant = userModel.get(2);
    QVERIFY(qtassistant);
    QCOMPARE(qtassistant->name(), QString("qtassistant"));
    QCOMPARE(qtassistant->mode(), QString("v"));
    QCOMPARE(qtassistant->prefix(), QString("+"));

    QPointer<IrcUser> Guest1234 = userModel.get(3);
    QVERIFY(Guest1234);
    QCOMPARE(Guest1234->name(), QString("Guest1234"));
    QCOMPARE(Guest1234->mode(), QString());
    QCOMPARE(Guest1234->prefix(), QString());

    QPointer<IrcUser> qout = userModel.get(4);
    QVERIFY(qout);
    QCOMPARE(qout->name(), QString("qout"));
    QCOMPARE(qout->mode(), QString("v"));
    QCOMPARE(qout->prefix(), QString("+"));

    QCOMPARE(countChangedSpy.count(), ++countChangedCount);
    QCOMPARE(countChangedSpy.last().at(0).toInt(), 5);

    QCOMPARE(namesChangedSpy.count(), ++namesChangedCount);
    QCOMPARE(namesChangedSpy.last().at(0).toStringList(), QStringList() << "ChanServ" << "Guest1234" << "communi" << "qout" << "qtassistant");

    QList<IrcUser*> users = QList<IrcUser*>() << communi << ChanServ << qtassistant << Guest1234 << qout;
    QCOMPARE(usersChangedSpy.count(), ++usersChangedCount);
    QCOMPARE(usersChangedSpy.last().at(0).value<QList<IrcUser*> >(), users);

    // ### trigger sort -> layout change
    userModel.setSortMethod(Irc::SortByTitle);
    QCOMPARE(layoutAboutToBeChangedSpy.count(), ++layoutAboutToBeChangedCount);
    QCOMPARE(layoutChangedSpy.count(), ++layoutChangedCount);

    // Irc::SortByTitle
    users = QList<IrcUser*>() << ChanServ << qout << qtassistant << communi << Guest1234;
    names = QStringList() << "ChanServ" << "qout" << "qtassistant" << "communi" << "Guest1234";
    titles = QStringList() << "@ChanServ" << "+qout" << "+qtassistant" << "communi" << "Guest1234";

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    // ### trigger sort -> layout change
    userModel.setSortMethod(Irc::SortByName);
    QCOMPARE(layoutAboutToBeChangedSpy.count(), ++layoutAboutToBeChangedCount);
    QCOMPARE(layoutChangedSpy.count(), ++layoutChangedCount);

    // Irc::SortByName
    users = QList<IrcUser*>() << ChanServ << communi << Guest1234 << qout << qtassistant;
    names = QStringList() << "ChanServ" << "communi" << "Guest1234" << "qout" << "qtassistant";
    titles = QStringList() << "@ChanServ" << "communi" << "Guest1234" << "+qout" << "+qtassistant";

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    // ### trigger model reset
    userModel.setChannel(0);
    QCOMPARE(channelChangedSpy.count(), ++channelChangedCount);
    QCOMPARE(channelChangedSpy.last().at(0).value<IrcChannel*>(), static_cast<IrcChannel*>(0));
    QCOMPARE(modelAboutToBeResetSpy.count(), ++modelAboutToBeResetCount);
    QCOMPARE(modelResetSpy.count(), ++modelResetCount);

    QCOMPARE(countChangedSpy.count(), ++countChangedCount);
    QCOMPARE(countChangedSpy.last().at(0).toInt(), 0);

    QCOMPARE(namesChangedSpy.count(), ++namesChangedCount);
    QCOMPARE(namesChangedSpy.last().at(0).toStringList(), QStringList());

    QCOMPARE(usersChangedSpy.count(), ++usersChangedCount);
    QCOMPARE(usersChangedSpy.last().at(0).value<QList<IrcUser*> >(), QList<IrcUser*>());

    // ### empty model -> no layout change
    userModel.setSortMethod(Irc::SortByActivity);
    QCOMPARE(layoutAboutToBeChangedSpy.count(), layoutAboutToBeChangedCount);
    QCOMPARE(layoutChangedSpy.count(), layoutChangedCount);

    // ### reset again
    userModel.setChannel(channel);
    QCOMPARE(modelAboutToBeResetSpy.count(), ++modelAboutToBeResetCount);
    QCOMPARE(modelResetSpy.count(), ++modelResetCount);

    // Irc::SortByActivity
    users = QList<IrcUser*>() << communi << ChanServ << qtassistant << Guest1234 << qout;
    names = QStringList() << "communi" << "ChanServ" << "qtassistant" << "Guest1234" << "qout";
    titles = QStringList() << "communi" << "@ChanServ" << "+qtassistant" << "Guest1234" << "+qout";

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    QCOMPARE(countChangedSpy.count(), ++countChangedCount);
    QCOMPARE(countChangedSpy.last().at(0).toInt(), users.count());

    QCOMPARE(namesChangedSpy.count(), ++namesChangedCount);
    QCOMPARE(namesChangedSpy.last().at(0).toStringList(), QStringList() << "ChanServ" << "Guest1234" << "communi" << "qout" << "qtassistant");

    QCOMPARE(usersChangedSpy.count(), ++usersChangedCount);
    QCOMPARE(usersChangedSpy.last().at(0).value<QList<IrcUser*> >(), users);

    QSignalSpy guestTitleChangedSpy(Guest1234, SIGNAL(titleChanged(QString)));
    QSignalSpy guestNameChangedSpy(Guest1234, SIGNAL(nameChanged(QString)));
    QSignalSpy guestPrefixChangedSpy(Guest1234, SIGNAL(prefixChanged(QString)));
    QSignalSpy guestModeChangedSpy(Guest1234, SIGNAL(modeChanged(QString)));
    QVERIFY(guestTitleChangedSpy.isValid());
    QVERIFY(guestNameChangedSpy.isValid());
    QVERIFY(guestPrefixChangedSpy.isValid());
    QVERIFY(guestModeChangedSpy.isValid());

    int guestTitleChangedCount = 0;
    int guestNameChangedCount = 0;
    int guestPrefixChangedCount = 0;
    int guestModeChangedCount = 0;

    // ### sorted by activity -> trigger a change in names & users, count remains intact
    QVERIFY(waitForWritten(":Guest1234!~Guest1234@hidd.en NICK :Guest5678"));

    QCOMPARE(Guest1234->name(), QString("Guest5678"));
    QCOMPARE(Guest1234->title(), QString("Guest5678"));

    QCOMPARE(guestTitleChangedSpy.count(), ++guestTitleChangedCount);
    QCOMPARE(guestTitleChangedSpy.last().at(0).toString(), QString("Guest5678"));

    QCOMPARE(guestNameChangedSpy.count(), ++guestNameChangedCount);
    QCOMPARE(guestNameChangedSpy.last().at(0).toString(), QString("Guest5678"));

    QCOMPARE(guestPrefixChangedSpy.count(), guestPrefixChangedCount);
    QCOMPARE(guestModeChangedSpy.count(), guestModeChangedCount);

    int previousIndex = users.indexOf(Guest1234);

    // Irc::SortByActivity
    users = QList<IrcUser*>() << Guest1234 << communi << ChanServ << qtassistant << qout;
    names = QStringList() << "Guest5678" << "communi" << "ChanServ" << "qtassistant" << "qout";
    titles = QStringList() << "Guest5678" << "communi" << "@ChanServ" << "+qtassistant" << "+qout";

    int nextIndex = users.indexOf(Guest1234);

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    QCOMPARE(countChangedSpy.count(), countChangedCount);

    QCOMPARE(namesChangedSpy.count(), ++namesChangedCount);
    QCOMPARE(namesChangedSpy.last().at(0).toStringList(), QStringList() << "ChanServ" << "Guest5678" << "communi" << "qout" << "qtassistant");

    QCOMPARE(usersChangedSpy.count(), ++usersChangedCount);
    QCOMPARE(usersChangedSpy.last().at(0).value<QList<IrcUser*> >(), users);

    QCOMPARE(dataChangedSpy.count(), ++dataChangedCount);
    QModelIndex topLeft = dataChangedSpy.last().at(0).value<QModelIndex>();
    QModelIndex bottomRight = dataChangedSpy.last().at(0).value<QModelIndex>();
    QVERIFY(topLeft.isValid());
    QVERIFY(bottomRight.isValid());
    QVERIFY(topLeft == bottomRight);
    QCOMPARE(topLeft.row(), previousIndex);
    QCOMPARE(topLeft.column(), 0);

    // TODO: nick change AND activity promotion
    //       => would ideally still result to just one change...
    rowsAboutToBeRemovedCount += 2;
    rowsRemovedCount += 2;
    rowsAboutToBeInsertedCount += 2;
    rowsInsertedCount += 2;

    QCOMPARE(rowsAboutToBeRemovedSpy.count(), rowsAboutToBeRemovedCount);
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(1).toInt(), previousIndex);
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(2).toInt(), previousIndex);

    QCOMPARE(rowsRemovedSpy.count(), rowsRemovedCount);
    QCOMPARE(rowsRemovedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsRemovedSpy.last().at(1).toInt(), previousIndex);
    QCOMPARE(rowsRemovedSpy.last().at(2).toInt(), previousIndex);

    QCOMPARE(rowsAboutToBeInsertedSpy.count(), rowsAboutToBeInsertedCount);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(2).toInt(), nextIndex);

    QCOMPARE(rowsInsertedSpy.count(), rowsInsertedCount);
    QCOMPARE(rowsInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsInsertedSpy.last().at(2).toInt(), nextIndex);

    // ### trigger sort -> layout change
    userModel.setSortMethod(Irc::SortByTitle);
    QCOMPARE(layoutAboutToBeChangedSpy.count(), ++layoutAboutToBeChangedCount);
    QCOMPARE(layoutChangedSpy.count(), ++layoutChangedCount);

    // Irc::SortByTitle
    users = QList<IrcUser*>() << ChanServ << qout << qtassistant << communi << Guest1234;
    names = QStringList() << "ChanServ" << "qout" << "qtassistant" << "communi" << "Guest5678";
    titles = QStringList() << "@ChanServ" << "+qout" << "+qtassistant" << "communi" << "Guest5678";

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    // ### sorted by title -> trigger a change in names & users, count remains intact
    QVERIFY(waitForWritten(":Guest5678!~Guest1234@hidd.en NICK :Guest1234"));

    QCOMPARE(Guest1234->name(), QString("Guest1234"));
    QCOMPARE(Guest1234->title(), QString("Guest1234"));

    QCOMPARE(guestTitleChangedSpy.count(), ++guestTitleChangedCount);
    QCOMPARE(guestTitleChangedSpy.last().at(0).toString(), QString("Guest1234"));

    QCOMPARE(guestNameChangedSpy.count(), ++guestNameChangedCount);
    QCOMPARE(guestNameChangedSpy.last().at(0).toString(), QString("Guest1234"));

    QCOMPARE(guestPrefixChangedSpy.count(), guestPrefixChangedCount);
    QCOMPARE(guestModeChangedSpy.count(), guestModeChangedCount);

    previousIndex = users.indexOf(Guest1234);

    // Irc::SortByTitle
    users = QList<IrcUser*>() << ChanServ << qout << qtassistant << communi << Guest1234;
    names = QStringList() << "ChanServ" << "qout" << "qtassistant" << "communi" << "Guest1234";
    titles = QStringList() << "@ChanServ" << "+qout" << "+qtassistant" << "communi" << "Guest1234";

    nextIndex = users.indexOf(Guest1234);

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    QCOMPARE(countChangedSpy.count(), countChangedCount);

    QCOMPARE(namesChangedSpy.count(), ++namesChangedCount);
    QCOMPARE(namesChangedSpy.last().at(0).toStringList(), QStringList() << "ChanServ" << "Guest1234" << "communi" << "qout" << "qtassistant");

    QCOMPARE(usersChangedSpy.count(), usersChangedCount);

    QCOMPARE(dataChangedSpy.count(), ++dataChangedCount);
    topLeft = dataChangedSpy.last().at(0).value<QModelIndex>();
    bottomRight = dataChangedSpy.last().at(1).value<QModelIndex>();
    QVERIFY(topLeft.isValid());
    QVERIFY(bottomRight.isValid());
    QVERIFY(topLeft == bottomRight);
    QCOMPARE(topLeft.row(), 4);
    QCOMPARE(topLeft.column(), 0);

    // TODO: nick change without index change
    //       => would ideally result to merely a data change...
    QCOMPARE(previousIndex, nextIndex);

    QCOMPARE(rowsAboutToBeRemovedSpy.count(), ++rowsAboutToBeRemovedCount);
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(1).toInt(), previousIndex);
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(2).toInt(), previousIndex);

    QCOMPARE(rowsRemovedSpy.count(), ++rowsRemovedCount);
    QCOMPARE(rowsRemovedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsRemovedSpy.last().at(1).toInt(), previousIndex);
    QCOMPARE(rowsRemovedSpy.last().at(2).toInt(), previousIndex);

    QCOMPARE(rowsAboutToBeInsertedSpy.count(), ++rowsAboutToBeInsertedCount);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(2).toInt(), nextIndex);

    QCOMPARE(rowsInsertedSpy.count(), ++rowsInsertedCount);
    QCOMPARE(rowsInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsInsertedSpy.last().at(2).toInt(), nextIndex);

    // ### sorted by title -> trigger a change in users, count & names remain intact
    QVERIFY(waitForWritten(":ChanServ!ChanServ@services. MODE #communi +v Guest1234"));

    QCOMPARE(Guest1234->name(), QString("Guest1234"));
    QCOMPARE(Guest1234->title(), QString("+Guest1234"));
    QCOMPARE(Guest1234->prefix(), QString("+"));
    QCOMPARE(Guest1234->mode(), QString("v"));

    QCOMPARE(guestTitleChangedSpy.count(), ++guestTitleChangedCount);
    QCOMPARE(guestTitleChangedSpy.last().at(0).toString(), QString("+Guest1234"));

    QCOMPARE(guestNameChangedSpy.count(), guestNameChangedCount);

    QCOMPARE(guestPrefixChangedSpy.count(), ++guestPrefixChangedCount);
    QCOMPARE(guestPrefixChangedSpy.last().at(0).toString(), QString("+"));

    QCOMPARE(guestModeChangedSpy.count(), ++guestModeChangedCount);
    QCOMPARE(guestModeChangedSpy.last().at(0).toString(), QString("v"));

    previousIndex = users.indexOf(Guest1234);

    // Irc::SortByTitle
    users = QList<IrcUser*>() << ChanServ << Guest1234 << qout << qtassistant << communi;
    names = QStringList() << "ChanServ" << "Guest1234" << "qout" << "qtassistant" << "communi";
    titles = QStringList() << "@ChanServ" << "+Guest1234" << "+qout" << "+qtassistant" << "communi";

    nextIndex = users.indexOf(Guest1234);

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    QCOMPARE(countChangedSpy.count(), countChangedCount);

    QCOMPARE(namesChangedSpy.count(), namesChangedCount);

    QCOMPARE(usersChangedSpy.count(), ++usersChangedCount);
    QCOMPARE(usersChangedSpy.last().at(0).value<QList<IrcUser*> >(), users);

    QCOMPARE(dataChangedSpy.count(), ++dataChangedCount);
    topLeft = dataChangedSpy.last().at(0).value<QModelIndex>();
    bottomRight = dataChangedSpy.last().at(1).value<QModelIndex>();
    QVERIFY(topLeft.isValid());
    QVERIFY(bottomRight.isValid());
    QVERIFY(topLeft == bottomRight);
    QCOMPARE(topLeft.row(), previousIndex);
    QCOMPARE(topLeft.column(), 0);

    QCOMPARE(rowsAboutToBeRemovedSpy.count(), ++rowsAboutToBeRemovedCount);
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(1).toInt(), previousIndex);
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(2).toInt(), previousIndex);

    QCOMPARE(rowsRemovedSpy.count(), ++rowsRemovedCount);
    QCOMPARE(rowsRemovedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsRemovedSpy.last().at(1).toInt(), previousIndex);
    QCOMPARE(rowsRemovedSpy.last().at(2).toInt(), previousIndex);

    QCOMPARE(rowsAboutToBeInsertedSpy.count(), ++rowsAboutToBeInsertedCount);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(2).toInt(), nextIndex);

    QCOMPARE(rowsInsertedSpy.count(), ++rowsInsertedCount);
    QCOMPARE(rowsInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsInsertedSpy.last().at(2).toInt(), nextIndex);

    // ### sorted by title -> trigger a change in users, count & names remain intact
    QVERIFY(waitForWritten(":ChanServ!ChanServ@services. MODE #communi -v Guest1234"));

    QCOMPARE(Guest1234->name(), QString("Guest1234"));
    QCOMPARE(Guest1234->title(), QString("Guest1234"));
    QCOMPARE(Guest1234->prefix(), QString());
    QCOMPARE(Guest1234->mode(), QString());

    QCOMPARE(guestTitleChangedSpy.count(), ++guestTitleChangedCount);
    QCOMPARE(guestTitleChangedSpy.last().at(0).toString(), QString("Guest1234"));

    QCOMPARE(guestNameChangedSpy.count(), guestNameChangedCount);

    QCOMPARE(guestPrefixChangedSpy.count(), ++guestPrefixChangedCount);
    QCOMPARE(guestPrefixChangedSpy.last().at(0).toString(), QString());

    QCOMPARE(guestModeChangedSpy.count(), ++guestModeChangedCount);
    QCOMPARE(guestModeChangedSpy.last().at(0).toString(), QString());

    previousIndex = users.indexOf(Guest1234);

    // Irc::SortByTitle
    users = QList<IrcUser*>() << ChanServ << qout << qtassistant << communi << Guest1234;
    names = QStringList() << "ChanServ" << "qout" << "qtassistant" << "communi" << "Guest1234";
    titles = QStringList() << "@ChanServ" << "+qout" << "+qtassistant" << "communi" << "Guest1234";

    nextIndex = users.indexOf(Guest1234);

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    QCOMPARE(countChangedSpy.count(), countChangedCount);

    QCOMPARE(namesChangedSpy.count(), namesChangedCount);

    QCOMPARE(usersChangedSpy.count(), ++usersChangedCount);
    QCOMPARE(usersChangedSpy.last().at(0).value<QList<IrcUser*> >(), users);

    QCOMPARE(dataChangedSpy.count(), ++dataChangedCount);
    topLeft = dataChangedSpy.last().at(0).value<QModelIndex>();
    bottomRight = dataChangedSpy.last().at(1).value<QModelIndex>();
    QVERIFY(topLeft.isValid());
    QVERIFY(bottomRight.isValid());
    QVERIFY(topLeft == bottomRight);
    QCOMPARE(topLeft.row(), previousIndex);
    QCOMPARE(topLeft.column(), 0);

    QCOMPARE(rowsAboutToBeRemovedSpy.count(), ++rowsAboutToBeRemovedCount);
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(1).toInt(), previousIndex);
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(2).toInt(), previousIndex);

    QCOMPARE(rowsRemovedSpy.count(), ++rowsRemovedCount);
    QCOMPARE(rowsRemovedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsRemovedSpy.last().at(1).toInt(), previousIndex);
    QCOMPARE(rowsRemovedSpy.last().at(2).toInt(), previousIndex);

    QCOMPARE(rowsAboutToBeInsertedSpy.count(), ++rowsAboutToBeInsertedCount);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(2).toInt(), nextIndex);

    QCOMPARE(rowsInsertedSpy.count(), ++rowsInsertedCount);
    QCOMPARE(rowsInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsInsertedSpy.last().at(2).toInt(), nextIndex);

    // ### sorted by title -> trigger a change in count, users & names
    QVERIFY(waitForWritten(":Guest1234!~Guest1234@hidd.en PART #communi"));

    QVERIFY(Guest1234); // deleteLater()'d

    QCOMPARE(aboutToBeRemovedSpy.count(), ++aboutToBeRemovedCount);
    QCOMPARE(aboutToBeRemovedSpy.last().at(0).value<IrcUser*>(), Guest1234.data());

    QCOMPARE(removedSpy.count(), ++removedCount);
    QCOMPARE(removedSpy.last().at(0).value<IrcUser*>(), Guest1234.data());

    previousIndex = users.indexOf(Guest1234);

    QCoreApplication::sendPostedEvents(Guest1234, QEvent::DeferredDelete);
    QVERIFY(!Guest1234);

    // Irc::SortByTitle
    users = QList<IrcUser*>() << ChanServ << qout << qtassistant << communi;
    names = QStringList() << "ChanServ" << "qout" << "qtassistant" << "communi";
    titles = QStringList() << "@ChanServ" << "+qout" << "+qtassistant" << "communi";

    nextIndex = users.indexOf(Guest1234);

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    QCOMPARE(countChangedSpy.count(), ++countChangedCount);
    QCOMPARE(countChangedSpy.last().at(0).toInt(), users.count());

    QCOMPARE(namesChangedSpy.count(), ++namesChangedCount);
    QCOMPARE(namesChangedSpy.last().at(0).toStringList(), QStringList() << "ChanServ" << "communi" << "qout" << "qtassistant");

    QCOMPARE(usersChangedSpy.count(), ++usersChangedCount);
    QCOMPARE(usersChangedSpy.last().at(0).value<QList<IrcUser*> >(), users);

    QCOMPARE(dataChangedSpy.count(), dataChangedCount);

    QCOMPARE(rowsAboutToBeRemovedSpy.count(), ++rowsAboutToBeRemovedCount);
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(1).toInt(), previousIndex);
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(2).toInt(), previousIndex);

    QCOMPARE(rowsRemovedSpy.count(), ++rowsRemovedCount);
    QCOMPARE(rowsRemovedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsRemovedSpy.last().at(1).toInt(), previousIndex);
    QCOMPARE(rowsRemovedSpy.last().at(2).toInt(), previousIndex);

    QCOMPARE(rowsAboutToBeInsertedSpy.count(), rowsAboutToBeInsertedCount);

    QCOMPARE(rowsInsertedSpy.count(), rowsInsertedCount);

    // ### sorted by title -> trigger a change in count, users & names
    QVERIFY(waitForWritten(":Guest1234!~Guest1234@hidd.en JOIN #communi"));

    Guest1234 = userModel.find("Guest1234");
    QVERIFY(Guest1234);

    QCOMPARE(aboutToBeAddedSpy.count(), ++aboutToBeAddedCount);
    QCOMPARE(aboutToBeAddedSpy.last().at(0).value<IrcUser*>(), Guest1234.data());

    QCOMPARE(addedSpy.count(), ++addedCount);
    QCOMPARE(addedSpy.last().at(0).value<IrcUser*>(), Guest1234.data());

    previousIndex = users.indexOf(Guest1234);

    // Irc::SortByTitle
    users = QList<IrcUser*>() << ChanServ << qout << qtassistant << communi << Guest1234;
    names = QStringList() << "ChanServ" << "qout" << "qtassistant" << "communi" << "Guest1234";
    titles = QStringList() << "@ChanServ" << "+qout" << "+qtassistant" << "communi" << "Guest1234";

    nextIndex = users.indexOf(Guest1234);

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    QCOMPARE(countChangedSpy.count(), ++countChangedCount);
    QCOMPARE(countChangedSpy.last().at(0).toInt(), users.count());

    QCOMPARE(namesChangedSpy.count(), ++namesChangedCount);
    QCOMPARE(namesChangedSpy.last().at(0).toStringList(), QStringList() << "ChanServ" << "Guest1234" << "communi" << "qout" << "qtassistant");

    QCOMPARE(usersChangedSpy.count(), ++usersChangedCount);
    QCOMPARE(usersChangedSpy.last().at(0).value<QList<IrcUser*> >(), users);

    QCOMPARE(dataChangedSpy.count(), dataChangedCount);

    QCOMPARE(rowsAboutToBeRemovedSpy.count(), rowsAboutToBeRemovedCount);

    QCOMPARE(rowsRemovedSpy.count(), rowsRemovedCount);

    QCOMPARE(rowsAboutToBeInsertedSpy.count(), ++rowsAboutToBeInsertedCount);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(2).toInt(), nextIndex);

    QCOMPARE(rowsInsertedSpy.count(), ++rowsInsertedCount);
    QCOMPARE(rowsInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsInsertedSpy.last().at(2).toInt(), nextIndex);

    QPointer<IrcUser> Bot = Guest1234;

    QSignalSpy botTitleChangedSpy(Guest1234, SIGNAL(titleChanged(QString)));
    QSignalSpy botNameChangedSpy(Guest1234, SIGNAL(nameChanged(QString)));
    QSignalSpy botPrefixChangedSpy(Guest1234, SIGNAL(prefixChanged(QString)));
    QSignalSpy botModeChangedSpy(Guest1234, SIGNAL(modeChanged(QString)));
    QVERIFY(botTitleChangedSpy.isValid());
    QVERIFY(botNameChangedSpy.isValid());
    QVERIFY(botPrefixChangedSpy.isValid());
    QVERIFY(botModeChangedSpy.isValid());

    int botTitleChangedCount = 0;
    int botNameChangedCount = 0;
    int botPrefixChangedCount = 0;
    int botModeChangedCount = 0;

    // ### sorted by title -> trigger a change in users & names, count remains intact
    QVERIFY(waitForWritten(":Guest1234!~Guest1234@hidd.en NICK :Bot"));

    QCOMPARE(Bot->name(), QString("Bot"));
    QCOMPARE(Bot->title(), QString("Bot"));

    QCOMPARE(botTitleChangedSpy.count(), ++botTitleChangedCount);
    QCOMPARE(botTitleChangedSpy.last().at(0).toString(), QString("Bot"));

    QCOMPARE(botNameChangedSpy.count(), ++botNameChangedCount);
    QCOMPARE(botNameChangedSpy.last().at(0).toString(), QString("Bot"));

    QCOMPARE(botPrefixChangedSpy.count(), botPrefixChangedCount);
    QCOMPARE(botModeChangedSpy.count(), botModeChangedCount);

    previousIndex = users.indexOf(Bot);

    // Irc::SortByTitle
    users = QList<IrcUser*>() << ChanServ << qout << qtassistant << Bot << communi;
    names = QStringList() << "ChanServ" << "qout" << "qtassistant" << "Bot" <<  "communi";
    titles = QStringList() << "@ChanServ" << "+qout" << "+qtassistant" << "Bot" << "communi";

    nextIndex = users.indexOf(Bot);

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    QCOMPARE(countChangedSpy.count(), countChangedCount);

    QCOMPARE(namesChangedSpy.count(), ++namesChangedCount);
    QCOMPARE(namesChangedSpy.last().at(0).toStringList(), QStringList() << "Bot" << "ChanServ" << "communi" << "qout" << "qtassistant");

    QCOMPARE(usersChangedSpy.count(), ++usersChangedCount);
    QCOMPARE(usersChangedSpy.last().at(0).value<QList<IrcUser*> >(), users);

    QCOMPARE(dataChangedSpy.count(), ++dataChangedCount);
    topLeft = dataChangedSpy.last().at(0).value<QModelIndex>();
    bottomRight = dataChangedSpy.last().at(0).value<QModelIndex>();
    QVERIFY(topLeft.isValid());
    QVERIFY(bottomRight.isValid());
    QVERIFY(topLeft == bottomRight);
    QCOMPARE(topLeft.row(), previousIndex);
    QCOMPARE(topLeft.column(), 0);

    QCOMPARE(rowsAboutToBeRemovedSpy.count(), ++rowsAboutToBeRemovedCount);
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(1).toInt(), previousIndex);
    QCOMPARE(rowsAboutToBeRemovedSpy.last().at(2).toInt(), previousIndex);

    QCOMPARE(rowsRemovedSpy.count(), ++rowsRemovedCount);
    QCOMPARE(rowsRemovedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsRemovedSpy.last().at(1).toInt(), previousIndex);
    QCOMPARE(rowsRemovedSpy.last().at(2).toInt(), previousIndex);

    QCOMPARE(rowsAboutToBeInsertedSpy.count(), ++rowsAboutToBeInsertedCount);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(2).toInt(), nextIndex);

    QCOMPARE(rowsInsertedSpy.count(), ++rowsInsertedCount);
    QCOMPARE(rowsInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsInsertedSpy.last().at(2).toInt(), nextIndex);

    // ### sorted by name, descending -> trigger a change in count, users & names
    userModel.setSortOrder(Qt::DescendingOrder);
    userModel.setSortMethod(Irc::SortByName);
    QVERIFY(waitForWritten(":fake!fake@hidd.en JOIN #communi"));

    QPointer<IrcUser> fake = userModel.find("fake");
    QVERIFY(fake);

    QCOMPARE(aboutToBeAddedSpy.count(), ++aboutToBeAddedCount);
    QCOMPARE(aboutToBeAddedSpy.last().at(0).value<IrcUser*>(), fake.data());

    QCOMPARE(addedSpy.count(), ++addedCount);
    QCOMPARE(addedSpy.last().at(0).value<IrcUser*>(), fake.data());

    previousIndex = users.indexOf(fake);

    // Irc::SortByName - descending
    users = QList<IrcUser*>() << qtassistant << qout << fake << communi << ChanServ << Bot;
    names = QStringList() << "qtassistant" << "qout" << "fake" << "communi" << "ChanServ" << "Bot";
    titles = QStringList() << "+qtassistant" << "+qout" << "fake" << "communi" << "@ChanServ" << "Bot";

    nextIndex = users.indexOf(fake);

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    QCOMPARE(countChangedSpy.count(), ++countChangedCount);
    QCOMPARE(countChangedSpy.last().at(0).toInt(), users.count());

    QCOMPARE(namesChangedSpy.count(), ++namesChangedCount);
    QCOMPARE(namesChangedSpy.last().at(0).toStringList(), QStringList() << "Bot" << "ChanServ" << "communi" << "fake" << "qout" << "qtassistant");

    QCOMPARE(usersChangedSpy.count(), ++usersChangedCount);
    QCOMPARE(usersChangedSpy.last().at(0).value<QList<IrcUser*> >(), users);

    QCOMPARE(dataChangedSpy.count(), dataChangedCount);

    QCOMPARE(rowsAboutToBeRemovedSpy.count(), rowsAboutToBeRemovedCount);

    QCOMPARE(rowsRemovedSpy.count(), rowsRemovedCount);

    QCOMPARE(rowsAboutToBeInsertedSpy.count(), ++rowsAboutToBeInsertedCount);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsAboutToBeInsertedSpy.last().at(2).toInt(), nextIndex);

    QCOMPARE(rowsInsertedSpy.count(), ++rowsInsertedCount);
    QCOMPARE(rowsInsertedSpy.last().at(0).value<QModelIndex>(), topLeft.parent());
    QCOMPARE(rowsInsertedSpy.last().at(1).toInt(), nextIndex);
    QCOMPARE(rowsInsertedSpy.last().at(2).toInt(), nextIndex);

    // QUIT -> no changes
    QVERIFY(waitForWritten(":communi!communi@hidd.en QUIT :bye"));

    QCOMPARE(userModel.count(), names.count());
    for (int i = 0; i < userModel.count(); ++i) {
        QCOMPARE(userModel.get(i)->name(), names.at(i));
        QCOMPARE(userModel.get(i)->title(), titles.at(i));
        QCOMPARE(userModel.get(i), users.at(i));
    }

    QCOMPARE(countChangedSpy.count(), countChangedCount);
    QCOMPARE(namesChangedSpy.count(), namesChangedCount);
    QCOMPARE(usersChangedSpy.count(), usersChangedCount);
    QCOMPARE(dataChangedSpy.count(), dataChangedCount);
    QCOMPARE(rowsAboutToBeRemovedSpy.count(), rowsAboutToBeRemovedCount);
    QCOMPARE(rowsRemovedSpy.count(), rowsRemovedCount);
    QCOMPARE(rowsAboutToBeInsertedSpy.count(), rowsAboutToBeInsertedCount);
    QCOMPARE(rowsInsertedSpy.count(), rowsInsertedCount);
    QCOMPARE(modelAboutToBeResetSpy.count(), modelAboutToBeResetCount);
    QCOMPARE(modelResetSpy.count(), modelResetCount);
}

void tst_IrcUserModel::testRoles()
{
    IrcUserModel model;
    QHash<int, QByteArray> roles = model.roleNames();
    QCOMPARE(roles.take(Qt::DisplayRole), QByteArray("display"));
    QCOMPARE(roles.take(Irc::UserRole), QByteArray("user"));
    QCOMPARE(roles.take(Irc::NameRole), QByteArray("name"));
    QCOMPARE(roles.take(Irc::PrefixRole), QByteArray("prefix"));
    QCOMPARE(roles.take(Irc::ModeRole), QByteArray("mode"));
    QCOMPARE(roles.take(Irc::TitleRole), QByteArray("title"));
    QVERIFY(roles.isEmpty());
}

void tst_IrcUserModel::testAIM()
{
    IrcBufferModel bufferModel;
    bufferModel.setConnection(connection);

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(waitForWritten(tst_IrcData::welcome()));
    QCOMPARE(bufferModel.count(), 0);
    waitForWritten(":communi!communi@hidd.en JOIN :#channel");
    QCOMPARE(bufferModel.count(), 1);

    IrcUserModel userModel(bufferModel.get(0));
    waitForWritten(":irc.ser.ver 353 communi = #channel :a @b +c");
    waitForWritten(":irc.ser.ver 366 communi #channel :End of /NAMES list.");
    QCOMPARE(userModel.count(), 3);

    IrcUser* a = userModel.find("a");
    IrcUser* b = userModel.find("b");
    IrcUser* c = userModel.find("c");
    IrcUser* o = 0;

    QVERIFY(a);
    QVERIFY(b);
    QVERIFY(c);

    QAbstractItemModel* aim = &userModel;
    QModelIndex ai = aim->index(0, 0);
    QModelIndex bi = aim->index(1, 0);
    QModelIndex ci = aim->index(2, 0);
    QModelIndex oi = aim->index(100, 100);

    QVERIFY(ai.isValid());
    QVERIFY(bi.isValid());
    QVERIFY(ci.isValid());
    QVERIFY(!oi.isValid());

    QCOMPARE(aim->rowCount(QModelIndex()), 3);
    QCOMPARE(aim->rowCount(ai), 0);

    QCOMPARE(aim->columnCount(QModelIndex()), 1);
    QCOMPARE(aim->columnCount(ai), 0);

    QCOMPARE(userModel.index(a), ai);
    QCOMPARE(userModel.index(b), bi);
    QCOMPARE(userModel.index(c), ci);
    QVERIFY(!userModel.index(o).isValid());

    QCOMPARE(userModel.user(ai), a);
    QCOMPARE(userModel.user(bi), b);
    QCOMPARE(userModel.user(ci), c);
    QVERIFY(!userModel.user(oi));

    userModel.setDisplayRole(Irc::TitleRole);
    QCOMPARE(aim->data(ai, Qt::DisplayRole).toString(), a->title());
    QCOMPARE(aim->data(bi, Qt::DisplayRole).toString(), b->title());
    QCOMPARE(aim->data(ci, Qt::DisplayRole).toString(), c->title());
    QVERIFY(aim->data(oi, Qt::DisplayRole).toString().isEmpty());

    userModel.setDisplayRole(Irc::UserRole);
    QCOMPARE(aim->data(ai, Qt::DisplayRole).value<IrcUser*>(), a);
    QCOMPARE(aim->data(bi, Qt::DisplayRole).value<IrcUser*>(), b);
    QCOMPARE(aim->data(ci, Qt::DisplayRole).value<IrcUser*>(), c);
    QVERIFY(!aim->data(oi, Qt::DisplayRole).value<IrcUser*>());

    QCOMPARE(aim->data(ai, Irc::UserRole).value<IrcUser*>(), a);
    QCOMPARE(aim->data(bi, Irc::UserRole).value<IrcUser*>(), b);
    QCOMPARE(aim->data(ci, Irc::UserRole).value<IrcUser*>(), c);
    QVERIFY(!aim->data(oi, Irc::UserRole).value<IrcUser*>());

    QCOMPARE(aim->data(ai, Irc::TitleRole).toString(), a->title());
    QCOMPARE(aim->data(bi, Irc::TitleRole).toString(), b->title());
    QCOMPARE(aim->data(ci, Irc::TitleRole).toString(), c->title());
    QVERIFY(aim->data(oi, Irc::TitleRole).toString().isEmpty());

    QCOMPARE(aim->data(ai, Irc::NameRole).toString(), a->name());
    QCOMPARE(aim->data(bi, Irc::NameRole).toString(), b->name());
    QCOMPARE(aim->data(ci, Irc::NameRole).toString(), c->name());
    QVERIFY(aim->data(oi, Irc::NameRole).toString().isEmpty());

    QVERIFY(aim->data(ai, Irc::PrefixRole).toString().isEmpty());
    QCOMPARE(aim->data(bi, Irc::PrefixRole).toString(), QString("@"));
    QCOMPARE(aim->data(ci, Irc::PrefixRole).toString(), QString("+"));
    QVERIFY(aim->data(oi, Irc::PrefixRole).toString().isEmpty());

    QVERIFY(aim->data(ai, Irc::ModeRole).toString().isEmpty());
    QCOMPARE(aim->data(bi, Irc::ModeRole).toString(), QString("o"));
    QCOMPARE(aim->data(ci, Irc::ModeRole).toString(), QString("v"));
    QVERIFY(aim->data(oi, Irc::ModeRole).toString().isEmpty());
}

void tst_IrcUserModel::testUser()
{
    IrcUserModel userModel;

    // ### setup #communi (5): communi @ChanServ +qtassistant Guest1234 +qout
    IrcBufferModel bufferModel;
    bufferModel.setConnection(connection);

    connection->open();
    QVERIFY(waitForOpened());

    QVERIFY(waitForWritten(tst_IrcData::welcome()));
    QVERIFY(waitForWritten(":communi!~communi@hidd.en JOIN :#communi"));
    QVERIFY(waitForWritten(":irc.ifi.uio.no 353 communi = #communi :communi @ChanServ +qtassistant Guest1234 +qout"));
    QVERIFY(waitForWritten(":irc.ifi.uio.no 366 communi #communi :End of NAMES list."));
    QCOMPARE(bufferModel.count(), 1);
    IrcChannel* channel = bufferModel.get(0)->toChannel();
    QVERIFY(channel);
    QCOMPARE(channel->title(), QString("#communi"));

    // ### ready to go!
    userModel.setChannel(channel);
    QCOMPARE(userModel.count(), 5);

    QPointer<IrcUser> communi = userModel.get(0);
    QVERIFY(communi);
    QCOMPARE(communi->name(), QString("communi"));
    QVERIFY(!communi->isAway());
    QVERIFY(!communi->isServOp());

    QPointer<IrcUser> ChanServ = userModel.get(1);
    QVERIFY(ChanServ);
    QCOMPARE(ChanServ->name(), QString("ChanServ"));
    QVERIFY(!ChanServ->isAway());
    QVERIFY(!ChanServ->isServOp());

    QPointer<IrcUser> qtassistant = userModel.get(2);
    QVERIFY(qtassistant);
    QCOMPARE(qtassistant->name(), QString("qtassistant"));
    QVERIFY(!qtassistant->isAway());
    QVERIFY(!qtassistant->isServOp());

    QPointer<IrcUser> Guest1234 = userModel.get(3);
    QVERIFY(Guest1234);
    QCOMPARE(Guest1234->name(), QString("Guest1234"));
    QVERIFY(!Guest1234->isAway());
    QVERIFY(!Guest1234->isServOp());

    QPointer<IrcUser> qout = userModel.get(4);
    QVERIFY(qout);
    QCOMPARE(qout->name(), QString("qout"));
    QVERIFY(!qout->isAway());
    QVERIFY(!qout->isServOp());

    QSignalSpy communiAwaySpy(communi.data(), SIGNAL(awayChanged(bool)));
    QSignalSpy ChanServAwaySpy(ChanServ.data(), SIGNAL(awayChanged(bool)));
    QSignalSpy qtassistantAwaySpy(qtassistant.data(), SIGNAL(awayChanged(bool)));
    QSignalSpy Guest1234AwaySpy(Guest1234.data(), SIGNAL(awayChanged(bool)));
    QSignalSpy qoutAwaySpy(qout.data(), SIGNAL(awayChanged(bool)));

    QVERIFY(communiAwaySpy.isValid());
    QVERIFY(ChanServAwaySpy.isValid());
    QVERIFY(qtassistantAwaySpy.isValid());
    QVERIFY(Guest1234AwaySpy.isValid());
    QVERIFY(qoutAwaySpy.isValid());

    QSignalSpy communiServOpSpy(communi.data(), SIGNAL(servOpChanged(bool)));
    QSignalSpy ChanServServOpSpy(ChanServ.data(), SIGNAL(servOpChanged(bool)));
    QSignalSpy qtassistantServOpSpy(qtassistant.data(), SIGNAL(servOpChanged(bool)));
    QSignalSpy Guest1234ServOpSpy(Guest1234.data(), SIGNAL(servOpChanged(bool)));
    QSignalSpy qoutServOpSpy(qout.data(), SIGNAL(servOpChanged(bool)));

    QVERIFY(communiServOpSpy.isValid());
    QVERIFY(ChanServServOpSpy.isValid());
    QVERIFY(qtassistantServOpSpy.isValid());
    QVERIFY(Guest1234ServOpSpy.isValid());
    QVERIFY(qoutServOpSpy.isValid());

    // first round

    QVERIFY(waitForWritten(":irc.ifi.uio.no 352 communi #communi ~communi hidd.en irc.ifi.uio.no communi H* :0 Communi"));
    QVERIFY(!communi->isAway());
    QVERIFY(communi->isServOp());
    QCOMPARE(communiAwaySpy.count(), 0);
    QCOMPARE(communiServOpSpy.count(), 1);

    QVERIFY(waitForWritten(":irc.ifi.uio.no 352 communi #communi ChanServ services. irc.ifi.uio.no ChanServ H@ :0 ChanServ"));
    QVERIFY(!ChanServ->isAway());
    QVERIFY(!ChanServ->isServOp());
    QCOMPARE(ChanServAwaySpy.count(), 0);
    QCOMPARE(ChanServServOpSpy.count(), 0);

    QVERIFY(waitForWritten(":irc.ifi.uio.no 352 communi #communi ~qtassistant hidd.en irc.ifi.uio.no qtassistant G+ :0 Qt Assistant"));
    QVERIFY(qtassistant->isAway());
    QVERIFY(!qtassistant->isServOp());
    QCOMPARE(qtassistantAwaySpy.count(), 1);
    QCOMPARE(qtassistantServOpSpy.count(), 0);

    QVERIFY(waitForWritten(":irc.ifi.uio.no 352 communi #communi ~guest hidd.en irc.ifi.uio.no Guest1234 H :0 Just a guest..."));
    QVERIFY(!Guest1234->isAway());
    QVERIFY(!Guest1234->isServOp());
    QCOMPARE(Guest1234AwaySpy.count(), 0);
    QCOMPARE(Guest1234ServOpSpy.count(), 0);

    QVERIFY(waitForWritten(":irc.ifi.uio.no 352 communi #communi ~qout hidd.en irc.ifi.uio.no qout G+ :0"));
    QVERIFY(qout->isAway());
    QVERIFY(!qout->isServOp());
    QCOMPARE(qoutAwaySpy.count(), 1);
    QCOMPARE(qoutServOpSpy.count(), 0);

    // second round

    QVERIFY(waitForWritten(":irc.ifi.uio.no 352 communi #communi ~communi hidd.en irc.ifi.uio.no communi G@ :0 Communi"));
    QVERIFY(communi->isAway());
    QVERIFY(!communi->isServOp());
    QCOMPARE(communiAwaySpy.count(), 1);
    QCOMPARE(communiServOpSpy.count(), 2);

    QVERIFY(waitForWritten(":irc.ifi.uio.no 352 communi #communi ChanServ services. irc.ifi.uio.no ChanServ H@ :0 ChanServ"));
    QVERIFY(!ChanServ->isAway());
    QVERIFY(!ChanServ->isServOp());
    QCOMPARE(ChanServAwaySpy.count(), 0);
    QCOMPARE(ChanServServOpSpy.count(), 0);

    QVERIFY(waitForWritten(":irc.ifi.uio.no 352 communi #communi ~qtassistant hidd.en irc.ifi.uio.no qtassistant H+ :0 Qt Assistant"));
    QVERIFY(!qtassistant->isAway());
    QVERIFY(!qtassistant->isServOp());
    QCOMPARE(qtassistantAwaySpy.count(), 2);
    QCOMPARE(qtassistantServOpSpy.count(), 0);

    QVERIFY(waitForWritten(":irc.ifi.uio.no 352 communi #communi ~guest hidd.en irc.ifi.uio.no Guest1234 G :0 Just a guest..."));
    QVERIFY(Guest1234->isAway());
    QVERIFY(!Guest1234->isServOp());
    QCOMPARE(Guest1234AwaySpy.count(), 1);
    QCOMPARE(Guest1234ServOpSpy.count(), 0);

    QVERIFY(waitForWritten(":irc.ifi.uio.no 352 communi #communi ~qout hidd.en irc.ifi.uio.no qout G+ :0"));
    QVERIFY(qout->isAway());
    QVERIFY(!qout->isServOp());
    QCOMPARE(qoutAwaySpy.count(), 1);
    QCOMPARE(qoutServOpSpy.count(), 0);
}

QTEST_MAIN(tst_IrcUserModel)

#include "tst_ircusermodel.moc"
