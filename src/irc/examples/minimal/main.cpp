/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This example is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include <QtCore>
#include <IrcConnection>
#include <IrcCommand>
#include <Irc>

#ifndef IRC_DOXYGEN
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    // enable debug output
    qputenv("IRC_DEBUG", "1");
    qsrand(QTime::currentTime().msec());

//! [minimal]
    IrcConnection connection("irc.freenode.net");
    connection.setUserName("communi");
    connection.setNickName(QString("Minimal%1").arg(qrand() % 9999));
    connection.setRealName(QString("Communi %1 minimal example").arg(Irc::version()));
    connection.sendCommand(IrcCommand::createJoin("#botwar"));
    connection.sendCommand(IrcCommand::createMessage("#botwar", "Hi, kthxbye!"));
    connection.sendCommand(IrcCommand::createQuit());
    connection.open();
//! [minimal]

    QObject::connect(&connection, SIGNAL(disconnected()), &app, SLOT(quit()));
    return app.exec();
}
#endif
