/*
 * Copyright (C) 2008-2009 J-P Nurmi jpnurmi@gmail.com
 *
 * This example is free, and not covered by LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially. By using it you may give me some credits in your
 * program, but you don't have to.
 */

#include <QtCore>
#include "session.h"

int main (int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc < 4)
    {
        qDebug("Usage: %s <server> <nick> <channels...>", argv[0]);
        return 1;
    }

    QStringList channels;
    for (int i = 3; i < argc; ++i)
    {
        channels.append(argv[i]);
    }

    MyIrcSession session;
    session.setNick(argv[2]);
    session.setAutoJoinChannels(channels);
    session.connectToServer(argv[1], 6667);

    return app.exec();
}
