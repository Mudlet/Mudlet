/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This example is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#ifndef IRCMESSAGEFORMATTER_H
#define IRCMESSAGEFORMATTER_H

#include <IrcMessage>

class IrcMessageFormatter
{
public:
    static QString formatMessage(IrcMessage* message);
    static QString formatMessage(const QString& message);

private:
    static QString formatJoinMessage(IrcJoinMessage* message);
    static QString formatModeMessage(IrcModeMessage* message);
    static QString formatNamesMessage(IrcNamesMessage* message);
    static QString formatNickMessage(IrcNickMessage* message);
    static QString formatPartMessage(IrcPartMessage* message);
    static QString formatPrivateMessage(IrcPrivateMessage* message);
    static QString formatQuitMessage(IrcQuitMessage* message);
};

#endif // IRCMESSAGEFORMATTER_H
