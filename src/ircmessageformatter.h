#ifndef IRCMESSAGEFORMATTER_H
#define IRCMESSAGEFORMATTER_H

/***************************************************************************
 *   Copyright (C) 2008-2017 The Communi Project                           *
 *   Copyright (C) 2017 by Fae - itsthefae@gmail.com                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "pre_guard.h"
#include <IrcMessage>
#include <QString>
#include "post_guard.h"

class IrcMessageFormatter
{
public:
    static QString formatMessage(IrcMessage* message, bool isForLua = false);
    static QString formatMessage(const QString& message, QString color = "#f29010", bool isForLua = false);
    static QString formatSeconds(int secs);
    static QString formatDuration(int secs);

private:
    static QString formatAwayMessage(IrcAwayMessage* message, bool isForLua = false);
    static QString formatInviteMessage(IrcInviteMessage* message, bool isForLua = false);
    static QString formatJoinMessage(IrcJoinMessage* message, bool isForLua = false);
    static QString formatKickMessage(IrcKickMessage* message, bool isForLua = false);
    static QString formatModeMessage(IrcModeMessage* message, bool isForLua = false);
    static QString formatMotdMessage(IrcMotdMessage* message, bool isForLua = false);
    static QString formatNamesMessage(IrcNamesMessage* message, bool isForLua = false);
    static QString formatNickMessage(IrcNickMessage* message, bool isForLua = false);
    static QString formatNoticeMessage(IrcNoticeMessage* message, bool isForLua = false);
    static QString formatNumericMessage(IrcNumericMessage* message, bool isForLua = false);
    static QString formatErrorMessage(IrcErrorMessage* message, bool isForLua = false);
    static QString formatPartMessage(IrcPartMessage* message, bool isForLua = false);
    static QString formatPongMessage(IrcPongMessage* message, bool isForLua = false);
    static QString formatPrivateMessage(IrcPrivateMessage* message, bool isForLua = false);
    static QString formatQuitMessage(IrcQuitMessage* message, bool isForLua = false);
    static QString formatTopicMessage(IrcTopicMessage* message, bool isForLua = false);
    static QString formatUnknownMessage(IrcMessage* message, bool isForLua = false);
    static QString formatWhoisMessage(IrcWhoisMessage* message, bool isForLua = false);
    static QString formatWhowasMessage(IrcWhowasMessage* message, bool isForLua = false);
    static QString formatWhoReplyMessage(IrcWhoReplyMessage* message, bool isForLua = false);
};

#endif // IRCMESSAGEFORMATTER_H
