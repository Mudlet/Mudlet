/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#ifndef TST_IRCCLIENTSERVER_H
#define TST_IRCCLIENTSERVER_H

#include <IrcConnection>

#include <QtTest/QtTest>
#include <QtCore/QPointer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#if QT_VERSION >= 0x050000
#define Q4SKIP(description) QSKIP(description)
#else
#define Q4SKIP(description) QSKIP(description, SkipAll)
#endif

class tst_IrcClientServer : public QObject
{
    Q_OBJECT

public:
    tst_IrcClientServer();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

protected:
    bool waitForOpened(int timeout = 200);
    bool waitForWritten(const QByteArray& data, int timeout = 1000);

    QPointer<QTcpServer> server;
    QPointer<QTcpSocket> serverSocket;
    QPointer<IrcConnection> connection;
    QPointer<QAbstractSocket> clientSocket;
};

#endif // TST_IRCCLIENTSERVER_H
