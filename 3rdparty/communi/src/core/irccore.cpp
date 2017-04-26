/*
  Copyright (C) 2008-2016 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "irccore.h"

IRC_BEGIN_NAMESPACE

/*!
    \file irccore.h
    \brief \#include &lt;IrcCore&gt;
 */

/*!
    \namespace IrcCore
    \ingroup core
    \brief Module meta-type registration.
 */

namespace IrcCore {

    /*!
        Registers IrcCore types to the %Qt meta-system.

        \sa IrcModel::registerMetaTypes(), IrcUtil::registerMetaTypes(), qRegisterMetaType()
     */
    void registerMetaTypes()
    {
        qRegisterMetaType<Irc::Color>("Irc::Color");
        qRegisterMetaType<Irc::DataRole>("Irc::DataRole");
        qRegisterMetaType<Irc::SortMethod>("Irc::SortMethod");
        qRegisterMetaType<Irc::Code>("Irc::Code");

        qRegisterMetaType<IrcConnection*>("IrcConnection*");
        qRegisterMetaType<IrcConnection::Status>("IrcConnection::Status");

        qRegisterMetaType<IrcNetwork*>("IrcNetwork*");

        qRegisterMetaType<IrcCommand*>("IrcCommand*");
        qRegisterMetaType<IrcCommand::Type>("IrcCommand::Type");

        qRegisterMetaType<IrcMessage*>("IrcMessage*");
        qRegisterMetaType<IrcMessage::Type>("IrcMessage::Type");

        qRegisterMetaType<IrcAccountMessage*>("IrcAccountMessage*");
        qRegisterMetaType<IrcAwayMessage*>("IrcAwayMessage*");
        qRegisterMetaType<IrcBatchMessage*>("IrcBatchMessage*");
        qRegisterMetaType<IrcCapabilityMessage*>("IrcCapabilityMessage*");
        qRegisterMetaType<IrcErrorMessage*>("IrcErrorMessage*");
        qRegisterMetaType<IrcInviteMessage*>("IrcHostChangeMessage*");
        qRegisterMetaType<IrcInviteMessage*>("IrcInviteMessage*");
        qRegisterMetaType<IrcJoinMessage*>("IrcJoinMessage*");
        qRegisterMetaType<IrcKickMessage*>("IrcKickMessage*");
        qRegisterMetaType<IrcModeMessage*>("IrcModeMessage*");
        qRegisterMetaType<IrcNamesMessage*>("IrcNamesMessage*");
        qRegisterMetaType<IrcNickMessage*>("IrcNickMessage*");
        qRegisterMetaType<IrcNoticeMessage*>("IrcNoticeMessage*");
        qRegisterMetaType<IrcNumericMessage*>("IrcNumericMessage*");
        qRegisterMetaType<IrcMotdMessage*>("IrcMotdMessage*");
        qRegisterMetaType<IrcPartMessage*>("IrcPartMessage*");
        qRegisterMetaType<IrcPingMessage*>("IrcPingMessage*");
        qRegisterMetaType<IrcPongMessage*>("IrcPongMessage*");
        qRegisterMetaType<IrcPrivateMessage*>("IrcPrivateMessage*");
        qRegisterMetaType<IrcQuitMessage*>("IrcQuitMessage*");
        qRegisterMetaType<IrcTopicMessage*>("IrcTopicMessage*");
        qRegisterMetaType<IrcWhoisMessage*>("IrcWhoisMessage*");
        qRegisterMetaType<IrcWhowasMessage*>("IrcWhowasMessage*");
        qRegisterMetaType<IrcWhoReplyMessage*>("IrcWhoReplyMessage*");
    }
}

IRC_END_NAMESPACE
