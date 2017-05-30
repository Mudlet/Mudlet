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
    static QString formatMessage(IrcMessage* message);
    static QString formatMessage(const QString& message);
    static QString formatSeconds(int secs);
    static QString formatDuration(int secs);
private:
    static QString formatAwayMessage(IrcAwayMessage* message);
    static QString formatInviteMessage(IrcInviteMessage* message);
    static QString formatJoinMessage(IrcJoinMessage* message);
    static QString formatKickMessage(IrcKickMessage* message);
    static QString formatModeMessage(IrcModeMessage* message);
    static QString formatMotdMessage(IrcMotdMessage* message);
    static QString formatNamesMessage(IrcNamesMessage* message);
    static QString formatNickMessage(IrcNickMessage* message);
    static QString formatNoticeMessage(IrcNoticeMessage* message);
    static QString formatNumericMessage(IrcNumericMessage* message);
    static QString formatErrorMessage(IrcErrorMessage* message);
    static QString formatPartMessage(IrcPartMessage* message);
    static QString formatPongMessage(IrcPongMessage* message);
    static QString formatPrivateMessage(IrcPrivateMessage* message);
    static QString formatQuitMessage(IrcQuitMessage* message);
    static QString formatTopicMessage(IrcTopicMessage* message);
    static QString formatUnknownMessage(IrcMessage* message);
    static QString formatWhoisMessage(IrcWhoisMessage* message);
    static QString formatWhowasMessage(IrcWhowasMessage* message);
    static QString formatWhoReplyMessage(IrcWhoReplyMessage* message);
};

#endif // IRCMESSAGEFORMATTER_H
