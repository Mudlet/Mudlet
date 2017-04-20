/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "ircuser.h"
#include <QtTest/QtTest>
#include <QtCore/QRegExp>
#ifdef Q_OS_LINUX
#include "ircuser_p.h"
#endif // Q_OS_LINUX

class tst_IrcUser : public QObject
{
    Q_OBJECT

private slots:
    void testDefaults();
    void testSignals();
    void testDebug();
};

void tst_IrcUser::testDefaults()
{
    IrcUser user;
    QVERIFY(user.title().isEmpty());
    QVERIFY(user.name().isEmpty());
    QVERIFY(user.prefix().isEmpty());
    QVERIFY(user.mode().isEmpty());
    QVERIFY(!user.isServOp());
    QVERIFY(!user.isAway());
    QVERIFY(!user.channel());
}

void tst_IrcUser::testSignals()
{
    IrcUser user;
    QSignalSpy titleSpy(&user, SIGNAL(titleChanged(QString)));
    QSignalSpy nameSpy(&user, SIGNAL(nameChanged(QString)));
    QSignalSpy prefixSpy(&user, SIGNAL(prefixChanged(QString)));
    QSignalSpy modeSpy(&user, SIGNAL(modeChanged(QString)));
    QSignalSpy servOpSpy(&user, SIGNAL(servOpChanged(bool)));
    QSignalSpy awaySpy(&user, SIGNAL(awayChanged(bool)));
    QVERIFY(titleSpy.isValid());
    QVERIFY(nameSpy.isValid());
    QVERIFY(prefixSpy.isValid());
    QVERIFY(modeSpy.isValid());
    QVERIFY(servOpSpy.isValid());
    QVERIFY(awaySpy.isValid());
}

void tst_IrcUser::testDebug()
{
    QString str;
    QDebug dbg(&str);

    dbg << static_cast<IrcUser*>(0);
    QCOMPARE(str.trimmed(), QString::fromLatin1("IrcUser(0x0)"));
    str.clear();

    IrcUser user;
    dbg << &user;
    QVERIFY(QRegExp("IrcUser\\(0x[0-9A-Fa-f]+\\) ").exactMatch(str));
    str.clear();

    user.setObjectName("obj");
    dbg << &user;
    QVERIFY(QRegExp("IrcUser\\(0x[0-9A-Fa-f]+, name=obj\\) ").exactMatch(str));
    str.clear();

#ifdef Q_OS_LINUX
    // others have problems with symbols (win) or private headers (osx frameworks)
    IrcUserPrivate::get(&user)->setName("usr");
    dbg << &user;
    QVERIFY(QRegExp("IrcUser\\(0x[0-9A-Fa-f]+, name=obj, user=usr\\) ").exactMatch(str));
    str.clear();
#endif // Q_OS_LINUX
}

QTEST_MAIN(tst_IrcUser)

#include "tst_ircuser.moc"
