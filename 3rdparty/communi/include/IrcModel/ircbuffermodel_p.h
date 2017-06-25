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

#ifndef IRCBUFFERMODEL_P_H
#define IRCBUFFERMODEL_P_H

#include "ircbuffer.h"
#include "ircfilter.h"
#include "ircbuffermodel.h"
#include <qpointer.h>

IRC_BEGIN_NAMESPACE

class IrcBufferModelPrivate : public QObject, public IrcMessageFilter, public IrcCommandFilter
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(IrcBufferModel)
    Q_INTERFACES(IrcMessageFilter IrcCommandFilter)

public:
    IrcBufferModelPrivate();

    bool messageFilter(IrcMessage* message);
    bool commandFilter(IrcCommand* command);

    IrcBuffer* createBufferHelper(const QString& title);
    IrcChannel* createChannelHelper(const QString& title);

    IrcBuffer* createBuffer(const QString& title);
    void destroyBuffer(const QString& title, bool force = false);

    void addBuffer(IrcBuffer* buffer, bool notify = true);
    void insertBuffer(int index, IrcBuffer* buffer, bool notify = true);
    void removeBuffer(IrcBuffer* buffer, bool notify = true);
    bool renameBuffer(const QString& from, const QString& to);
    void promoteBuffer(IrcBuffer* buffer);

    void restoreBuffer(IrcBuffer* buffer);
    QVariantMap saveBuffer(IrcBuffer* buffer) const;

    bool processMessage(const QString& title, IrcMessage* message, bool create = false);

    void _irc_connected();
    void _irc_initialized();
    void _irc_disconnected();
    void _irc_bufferDestroyed(IrcBuffer* buffer);

    void _irc_restoreBuffers();
    void _irc_monitorStatus();

    static IrcBufferModelPrivate* get(IrcBufferModel* model)
    {
        return model->d_func();
    }

    IrcBufferModel* q_ptr;
    Irc::DataRole role;
    QPointer<IrcConnection> connection;
    QList<IrcBuffer*> bufferList;
    QMap<QString, IrcBuffer*> bufferMap;
    QHash<QString, QString> keys;
    QVariantMap bufferStates;
    QStringList channels;
    Irc::SortMethod sortMethod;
    Qt::SortOrder sortOrder;
    IrcBuffer* bufferProto;
    IrcChannel* channelProto;
    bool persistent;
    int joinDelay;
    bool monitorEnabled;
    bool monitorPending;
};

IRC_END_NAMESPACE

#endif // IRCBUFFERMODEL_P_H
