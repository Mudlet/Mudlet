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

#ifndef IRCCOMMAND_H
#define IRCCOMMAND_H

#include <IrcGlobal>
#include <QtCore/qobject.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qstringlist.h>

IRC_BEGIN_NAMESPACE

class IrcMessage;
class IrcNetwork;
class IrcConnection;
class IrcCommandPrivate;

class IRC_CORE_EXPORT IrcCommand : public QObject
{
    Q_OBJECT
    Q_PROPERTY(IrcConnection* connection READ connection)
    Q_PROPERTY(IrcNetwork* network READ network)
    Q_PROPERTY(QStringList parameters READ parameters WRITE setParameters)
    Q_PROPERTY(QByteArray encoding READ encoding WRITE setEncoding)
    Q_PROPERTY(Type type READ type WRITE setType)
    Q_ENUMS(Type)

public:
    enum Type {
        Admin,
        Away,
        Capability,
        CtcpAction,
        CtcpReply,
        CtcpRequest,
        Custom,
        Info,
        Invite,
        Join,
        Kick,
        Knock,
        List,
        Message,
        Mode,
        Motd,
        Names,
        Nick,
        Notice,
        Part,
        Ping,
        Pong,
        Quit,
        Quote,
        Stats,
        Time,
        Topic,
        Trace,
        Users,
        Version,
        Who,
        Whois,
        Whowas,
        Monitor
    };

    explicit IrcCommand(QObject* parent = 0);
    virtual ~IrcCommand();

    IrcConnection* connection() const;
    IrcNetwork* network() const;

    Type type() const;
    void setType(Type type);

    QStringList parameters() const;
    void setParameters(const QStringList& parameters);

    QByteArray encoding() const;
    void setEncoding(const QByteArray& encoding);

    virtual QString toString() const;

    Q_INVOKABLE IrcMessage* toMessage(const QString& prefix, IrcConnection* connection) const;

    Q_INVOKABLE static IrcCommand* createAdmin(const QString& server = QString());
    Q_INVOKABLE static IrcCommand* createAway(const QString& reason = QString());
    Q_INVOKABLE static IrcCommand* createCapability(const QString& subCommand, const QString& capability);
    Q_INVOKABLE static IrcCommand* createCapability(const QString& subCommand, const QStringList& capabilities = QStringList());
    Q_INVOKABLE static IrcCommand* createCtcpAction(const QString& target, const QString& action);
    Q_INVOKABLE static IrcCommand* createCtcpReply(const QString& target, const QString& reply);
    Q_INVOKABLE static IrcCommand* createCtcpRequest(const QString& target, const QString& request);
    Q_INVOKABLE static IrcCommand* createInfo(const QString& server = QString());
    Q_INVOKABLE static IrcCommand* createInvite(const QString& user, const QString& channel);
    Q_INVOKABLE static IrcCommand* createJoin(const QString& channel, const QString& key = QString());
    Q_INVOKABLE static IrcCommand* createJoin(const QStringList& channels, const QStringList& keys = QStringList());
    Q_INVOKABLE static IrcCommand* createKick(const QString& channel, const QString& user, const QString& reason = QString());
    Q_INVOKABLE static IrcCommand* createKnock(const QString& channel, const QString& message = QString());
    Q_INVOKABLE static IrcCommand* createList(const QStringList& channels = QStringList(), const QString& server = QString());
    Q_INVOKABLE static IrcCommand* createMessage(const QString& target, const QString& message);
    Q_INVOKABLE static IrcCommand* createMode(const QString& target, const QString& mode = QString(), const QString& arg = QString());
    Q_INVOKABLE static IrcCommand* createMonitor(const QString& command, const QString& target = QString());
    Q_INVOKABLE static IrcCommand* createMonitor(const QString& command, const QStringList& targets);
    Q_INVOKABLE static IrcCommand* createMotd(const QString& server = QString());
    Q_INVOKABLE static IrcCommand* createNames(const QString& channel = QString(), const QString& server = QString());
    Q_INVOKABLE static IrcCommand* createNames(const QStringList& channels, const QString& server = QString());
    Q_INVOKABLE static IrcCommand* createNick(const QString& nick);
    Q_INVOKABLE static IrcCommand* createNotice(const QString& target, const QString& notice);
    Q_INVOKABLE static IrcCommand* createPart(const QString& channel, const QString& reason = QString());
    Q_INVOKABLE static IrcCommand* createPart(const QStringList& channels, const QString& reason = QString());
    Q_INVOKABLE static IrcCommand* createPing(const QString& argument);
    Q_INVOKABLE static IrcCommand* createPong(const QString& argument);
    Q_INVOKABLE static IrcCommand* createQuit(const QString& reason = QString());
    Q_INVOKABLE static IrcCommand* createQuote(const QString& raw);
    Q_INVOKABLE static IrcCommand* createQuote(const QStringList& parameters);
    Q_INVOKABLE static IrcCommand* createStats(const QString& query, const QString& server = QString());
    Q_INVOKABLE static IrcCommand* createTime(const QString& server = QString());
    Q_INVOKABLE static IrcCommand* createTopic(const QString& channel, const QString& topic = QString());
    Q_INVOKABLE static IrcCommand* createTrace(const QString& target = QString());
    Q_INVOKABLE static IrcCommand* createUsers(const QString& server = QString());
    Q_INVOKABLE static IrcCommand* createVersion(const QString& user = QString());
    Q_INVOKABLE static IrcCommand* createWho(const QString& mask, bool operators = false);
    Q_INVOKABLE static IrcCommand* createWhois(const QString& user);
    Q_INVOKABLE static IrcCommand* createWhowas(const QString& user, int count = 1);

private:
    QScopedPointer<IrcCommandPrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcCommand)
    Q_DISABLE_COPY(IrcCommand)
};

#ifndef QT_NO_DEBUG_STREAM
IRC_CORE_EXPORT QDebug operator<<(QDebug debug, IrcCommand::Type type);
IRC_CORE_EXPORT QDebug operator<<(QDebug debug, const IrcCommand* command);
#endif // QT_NO_DEBUG_STREAM

IRC_END_NAMESPACE

Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcCommand*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcCommand::Type))

#endif // IRCCOMMAND_H
