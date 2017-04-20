/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This example is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#ifndef IRCBOT_H
#define IRCBOT_H

#include <IrcConnection>
#include <IrcBufferModel>
#include <IrcCommandParser>

class IrcBot : public IrcConnection
{
    Q_OBJECT

public:
    IrcBot(QObject* parent = 0);

public slots:
    void join(QString channel);

private slots:
    void processMessage(IrcPrivateMessage* message);

private:
    void help(QStringList commands);

    IrcCommandParser parser;
    IrcBufferModel bufferModel;
};

#endif // IRCBOT_H
