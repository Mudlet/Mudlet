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

#ifndef IRCBUFFER_P_H
#define IRCBUFFER_P_H

#include "ircbuffer.h"
#include "ircmessage.h"
#include <qstringlist.h>
#include <qdatetime.h>
#include <qlist.h>
#include <qmap.h>

IRC_BEGIN_NAMESPACE

class IrcUser;
class IrcUserModel;

class IrcBufferPrivate
{
    Q_DECLARE_PUBLIC(IrcBuffer)

public:
    IrcBufferPrivate();
    virtual ~IrcBufferPrivate();

    virtual void init(const QString& title, IrcBufferModel* model);
    virtual void connected();
    virtual void disconnected();

    void setName(const QString& name);
    void setPrefix(const QString& prefix);
    void setModel(IrcBufferModel* model);

    enum MonitorStatus { MonitorUnknown, MonitorOffline, MonitorOnline };
    void setMonitorStatus(MonitorStatus status);
    bool isMonitorable() const;

    bool processMessage(IrcMessage* message);

    virtual bool processAwayMessage(IrcAwayMessage* message);
    virtual bool processJoinMessage(IrcJoinMessage* message);
    virtual bool processKickMessage(IrcKickMessage* message);
    virtual bool processModeMessage(IrcModeMessage* message);
    virtual bool processNamesMessage(IrcNamesMessage* message);
    virtual bool processNickMessage(IrcNickMessage* message);
    virtual bool processNoticeMessage(IrcNoticeMessage* message);
    virtual bool processNumericMessage(IrcNumericMessage* message);
    virtual bool processPartMessage(IrcPartMessage* message);
    virtual bool processPrivateMessage(IrcPrivateMessage* message);
    virtual bool processQuitMessage(IrcQuitMessage* message);
    virtual bool processTopicMessage(IrcTopicMessage* message);
    virtual bool processWhoReplyMessage(IrcWhoReplyMessage* message);

    static IrcBufferPrivate* get(IrcBuffer* buffer)
    {
        return buffer->d_func();
    }

    IrcBuffer* q_ptr;
    IrcBufferModel* model;
    QString name;
    QString prefix;
    bool persistent;
    bool sticky;
    QVariantMap userData;
    QDateTime activity;
    MonitorStatus monitorStatus;
};

IRC_END_NAMESPACE

#endif // IRCBUFFER_P_H
