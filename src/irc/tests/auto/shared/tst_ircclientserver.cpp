/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "tst_ircclientserver.h"

tst_IrcClientServer::tst_IrcClientServer()
{
    server = new QTcpServer(this);
}

void tst_IrcClientServer::initTestCase()
{
    QVERIFY(server->listen());
}

void tst_IrcClientServer::cleanupTestCase()
{
    server->close();
}

void tst_IrcClientServer::init()
{
    connection = new IrcConnection(this);
    connection->setUserName("user");
    connection->setNickName("nick");
    connection->setRealName("real");
    connection->setPassword("secret");
    connection->setHost("127.0.0.1");
    connection->setPort(server->serverPort());
}

void tst_IrcClientServer::cleanup()
{
    delete connection;
}

bool tst_IrcClientServer::waitForOpened(int timeout)
{
    if (!server->waitForNewConnection(timeout))
        return false;
    serverSocket = server->nextPendingConnection();
    clientSocket = connection->socket();
    return serverSocket && clientSocket && clientSocket->waitForConnected(1000);
}

bool tst_IrcClientServer::waitForWritten(const QByteArray& data, int timeout)
{
    if (!data.isNull()) {
        if (data.count('\n') > 1) {
            bool success = true;
            foreach (const QByteArray& line, data.split('\n'))
                success &= waitForWritten(line + '\n', timeout);
            return success;
        }
        if (data.endsWith('\r') || data.endsWith('\n'))
            serverSocket->write(data);
        else
            serverSocket->write(data + "\r\n");
    }
    return serverSocket->waitForBytesWritten(timeout) && clientSocket->waitForReadyRead(timeout);
}
