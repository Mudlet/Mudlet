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

#ifndef IRCMESSAGE_H
#define IRCMESSAGE_H

#include <Irc>
#include <IrcGlobal>
#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qstringlist.h>

IRC_BEGIN_NAMESPACE

class IrcCommand;
class IrcNetwork;
class IrcConnection;
class IrcMessagePrivate;

class IRC_CORE_EXPORT IrcMessage : public QObject
{
    Q_OBJECT
    Q_PROPERTY(IrcConnection* connection READ connection)
    Q_PROPERTY(IrcNetwork* network READ network)
    Q_PROPERTY(Type type READ type)
    Q_PROPERTY(bool own READ isOwn)
    Q_PROPERTY(bool implicit READ isImplicit)
    Q_PROPERTY(Flags flags READ flags)
    Q_PROPERTY(bool valid READ isValid)
    Q_PROPERTY(QString command READ command)
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix)
    Q_PROPERTY(QString nick READ nick)
    Q_PROPERTY(QString ident READ ident)
    Q_PROPERTY(QString host READ host)
    Q_PROPERTY(QString account READ account)
    Q_PROPERTY(QStringList parameters READ parameters WRITE setParameters)
    Q_PROPERTY(QDateTime timeStamp READ timeStamp WRITE setTimeStamp)
    Q_PROPERTY(QVariantMap tags READ tags WRITE setTags)
    Q_ENUMS(Type Flag)
    Q_FLAGS(Flags)

public:
    enum Type {
        Unknown,
        Capability,
        Error,
        Invite,
        Join,
        Kick,
        Mode,
        Motd,
        Names,
        Nick,
        Notice,
        Numeric,
        Part,
        Ping,
        Pong,
        Private,
        Quit,
        Topic,
        WhoReply,
        Account,
        Away,
        Whois,
        Whowas,
        HostChange,
        Batch
    };

    enum Flag {
        None = 0x00,
        Own = 0x01,
        Identified = 0x02,
        Unidentified = 0x04,
        Playback = 0x08,
        Implicit = 0x10
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    Q_INVOKABLE explicit IrcMessage(IrcConnection* connection);
    virtual ~IrcMessage();

    IrcConnection* connection() const;
    IrcNetwork* network() const;

    Type type() const;

    bool isOwn() const;
    bool isImplicit() const;

    Flags flags() const;
    void setFlags(Flags flags);

    Q_INVOKABLE bool testFlag(Flag flag) const;
    Q_INVOKABLE void setFlag(Flag flag, bool on = true);

    QString command() const;
    void setCommand(const QString& command);

    QString prefix() const;
    void setPrefix(const QString& prefix);

    QString nick() const;
    QString ident() const;
    QString host() const;
    QString account() const;

    QStringList parameters() const;
    void setParameters(const QStringList& parameters);

    QString parameter(int index) const;
    void setParameter(int index, const QString& parameter);

    virtual bool isValid() const;

    QDateTime timeStamp() const;
    void setTimeStamp(const QDateTime& timeStamp);

    QByteArray encoding() const;
    void setEncoding(const QByteArray& encoding);

    QVariantMap tags() const;
    void setTags(const QVariantMap& tags);

    QVariant tag(const QString& name) const;
    void setTag(const QString& name, const QVariant& tag);

    Q_INVOKABLE QByteArray toData() const;
    Q_INVOKABLE static IrcMessage* fromData(const QByteArray& data, IrcConnection* connection);
    Q_INVOKABLE static IrcMessage* fromParameters(const QString& prefix, const QString& command, const QStringList& parameters, IrcConnection* connection);
    Q_INVOKABLE IrcMessage* clone(QObject *parent = 0) const;

protected:
    QScopedPointer<IrcMessagePrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcMessage)
    Q_DISABLE_COPY(IrcMessage)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(IrcMessage::Flags)

class IRC_CORE_EXPORT IrcAccountMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString account READ account)

public:
    Q_INVOKABLE explicit IrcAccountMessage(IrcConnection* connection);

    QString account() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcAccountMessage)
};

class IRC_CORE_EXPORT IrcAwayMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString content READ content)
    Q_PROPERTY(bool reply READ isReply)
    Q_PROPERTY(bool away READ isAway)

public:
    Q_INVOKABLE explicit IrcAwayMessage(IrcConnection* connection);

    QString content() const;
    bool isReply() const;
    bool isAway() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcAwayMessage)
};

class IRC_CORE_EXPORT IrcBatchMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString tag READ tag)
    Q_PROPERTY(QString batch READ batch)
    Q_PROPERTY(QList<IrcMessage*> messages READ messages)

public:
    Q_INVOKABLE explicit IrcBatchMessage(IrcConnection* connection);

    QString tag() const;
    QString batch() const;

    QList<IrcMessage*> messages() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcBatchMessage)
};

class IRC_CORE_EXPORT IrcCapabilityMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString subCommand READ subCommand)
    Q_PROPERTY(QStringList capabilities READ capabilities)

public:
    Q_INVOKABLE explicit IrcCapabilityMessage(IrcConnection* connection);

    QString subCommand() const;
    QStringList capabilities() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcCapabilityMessage)
};

class IRC_CORE_EXPORT IrcErrorMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString error READ error)

public:
    Q_INVOKABLE explicit IrcErrorMessage(IrcConnection* connection);

    QString error() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcErrorMessage)
};

class IRC_CORE_EXPORT IrcHostChangeMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString user READ user)
    Q_PROPERTY(QString host READ host)

public:
    Q_INVOKABLE explicit IrcHostChangeMessage(IrcConnection* connection);

    QString user() const;
    QString host() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcHostChangeMessage)
};

class IRC_CORE_EXPORT IrcInviteMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString user READ user)
    Q_PROPERTY(QString channel READ channel)
    Q_PROPERTY(bool reply READ isReply)

public:
    Q_INVOKABLE explicit IrcInviteMessage(IrcConnection* connection);

    QString user() const;
    QString channel() const;
    bool isReply() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcInviteMessage)
};

class IRC_CORE_EXPORT IrcJoinMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel)
    Q_PROPERTY(QString account READ account)
    Q_PROPERTY(QString realName READ realName)

public:
    Q_INVOKABLE explicit IrcJoinMessage(IrcConnection* connection);

    QString channel() const;
    QString account() const;
    QString realName() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcJoinMessage)
};

class IRC_CORE_EXPORT IrcKickMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel)
    Q_PROPERTY(QString user READ user)
    Q_PROPERTY(QString reason READ reason)

public:
    Q_INVOKABLE explicit IrcKickMessage(IrcConnection* connection);

    QString channel() const;
    QString user() const;
    QString reason() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcKickMessage)
};

class IRC_CORE_EXPORT IrcModeMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString target READ target)
    Q_PROPERTY(QString mode READ mode)
    Q_PROPERTY(QString argument READ argument)
    Q_PROPERTY(QStringList arguments READ arguments)
    Q_PROPERTY(bool reply READ isReply)
    Q_PROPERTY(Kind kind READ kind)
    Q_ENUMS(Kind)

public:
    Q_INVOKABLE explicit IrcModeMessage(IrcConnection* connection);

    QString target() const;
    QString mode() const;
    QString argument() const;
    QStringList arguments() const;
    bool isReply() const;

    enum Kind { Channel, User };
    Kind kind() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcModeMessage)
};

class IRC_CORE_EXPORT IrcMotdMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QStringList lines READ lines)

public:
    Q_INVOKABLE explicit IrcMotdMessage(IrcConnection* connection);

    QStringList lines() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcMotdMessage)
};

class IRC_CORE_EXPORT IrcNamesMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel)
    Q_PROPERTY(QStringList names READ names)

public:
    Q_INVOKABLE explicit IrcNamesMessage(IrcConnection* connection);

    QString channel() const;
    QStringList names() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcNamesMessage)
};

class IRC_CORE_EXPORT IrcNickMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString oldNick READ oldNick)
    Q_PROPERTY(QString newNick READ newNick)

public:
    Q_INVOKABLE explicit IrcNickMessage(IrcConnection* connection);

    QString oldNick() const;
    QString newNick() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcNickMessage)
};

class IRC_CORE_EXPORT IrcNoticeMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString target READ target)
    Q_PROPERTY(QString content READ content)
    Q_PROPERTY(QString statusPrefix READ statusPrefix)
    Q_PROPERTY(bool private READ isPrivate)
    Q_PROPERTY(bool reply READ isReply)

public:
    Q_INVOKABLE explicit IrcNoticeMessage(IrcConnection* connection);

    QString target() const;
    QString content() const;
    QString statusPrefix() const;
    bool isPrivate() const;
    bool isReply() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcNoticeMessage)
};

class IRC_CORE_EXPORT IrcNumericMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(int code READ code)
    Q_PROPERTY(bool composed READ isComposed)

public:
    Q_INVOKABLE explicit IrcNumericMessage(IrcConnection* connection);

    int code() const;
    bool isComposed() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcNumericMessage)
};

class IRC_CORE_EXPORT IrcPartMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel)
    Q_PROPERTY(QString reason READ reason)

public:
    Q_INVOKABLE explicit IrcPartMessage(IrcConnection* connection);

    QString channel() const;
    QString reason() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcPartMessage)
};

class IRC_CORE_EXPORT IrcPingMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString argument READ argument)

public:
    Q_INVOKABLE explicit IrcPingMessage(IrcConnection* connection);

    QString argument() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcPingMessage)
};

class IRC_CORE_EXPORT IrcPongMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString argument READ argument)

public:
    Q_INVOKABLE explicit IrcPongMessage(IrcConnection* connection);

    QString argument() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcPongMessage)
};

class IRC_CORE_EXPORT IrcPrivateMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString target READ target)
    Q_PROPERTY(QString content READ content)
    Q_PROPERTY(QString statusPrefix READ statusPrefix)
    Q_PROPERTY(bool private READ isPrivate)
    Q_PROPERTY(bool action READ isAction)
    Q_PROPERTY(bool request READ isRequest)

public:
    Q_INVOKABLE explicit IrcPrivateMessage(IrcConnection* connection);

    QString target() const;
    QString content() const;
    QString statusPrefix() const;
    bool isPrivate() const;
    bool isAction() const;
    bool isRequest() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcPrivateMessage)
};

class IRC_CORE_EXPORT IrcQuitMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString reason READ reason)

public:
    Q_INVOKABLE explicit IrcQuitMessage(IrcConnection* connection);

    QString reason() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcQuitMessage)
};

class IRC_CORE_EXPORT IrcTopicMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel)
    Q_PROPERTY(QString topic READ topic)
    Q_PROPERTY(bool reply READ isReply)

public:
    Q_INVOKABLE explicit IrcTopicMessage(IrcConnection* connection);

    QString channel() const;
    QString topic() const;
    bool isReply() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcTopicMessage)
};

class IRC_CORE_EXPORT IrcWhoisMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString realName READ realName)
    Q_PROPERTY(QString server READ server)
    Q_PROPERTY(QString info READ info)
    Q_PROPERTY(QString account READ account)
    Q_PROPERTY(QString address READ address)
    Q_PROPERTY(QDateTime since READ since)
    Q_PROPERTY(int idle READ idle)
    Q_PROPERTY(bool secure READ isSecure)
    Q_PROPERTY(QStringList channels READ channels)
    Q_PROPERTY(QString awayReason READ awayReason)

public:
    Q_INVOKABLE explicit IrcWhoisMessage(IrcConnection* connection);

    QString realName() const;
    QString server() const;
    QString info() const;
    QString account() const;
    QString address() const;
    QDateTime since() const;
    int idle() const;
    bool isSecure() const;
    QStringList channels() const;
    QString awayReason() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcWhoisMessage)
};

class IRC_CORE_EXPORT IrcWhowasMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString realName READ realName)
    Q_PROPERTY(QString server READ server)
    Q_PROPERTY(QString info READ info)
    Q_PROPERTY(QString account READ account)

public:
    Q_INVOKABLE explicit IrcWhowasMessage(IrcConnection* connection);

    QString realName() const;
    QString server() const;
    QString info() const;
    QString account() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcWhowasMessage)
};

class IRC_CORE_EXPORT IrcWhoReplyMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString mask READ mask)
    Q_PROPERTY(QString server READ server)
    Q_PROPERTY(bool away READ isAway)
    Q_PROPERTY(bool servOp READ isServOp)
    Q_PROPERTY(QString realName READ realName)

public:
    Q_INVOKABLE explicit IrcWhoReplyMessage(IrcConnection* connection);

    QString mask() const;
    QString server() const;
    bool isAway() const;
    bool isServOp() const;
    QString realName() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcWhoReplyMessage)
};

#ifndef QT_NO_DEBUG_STREAM
IRC_CORE_EXPORT QDebug operator<<(QDebug debug, IrcMessage::Type type);
IRC_CORE_EXPORT QDebug operator<<(QDebug debug, IrcMessage::Flag flag);
IRC_CORE_EXPORT QDebug operator<<(QDebug debug, IrcMessage::Flags flags);
IRC_CORE_EXPORT QDebug operator<<(QDebug debug, IrcModeMessage::Kind kind);
IRC_CORE_EXPORT QDebug operator<<(QDebug debug, const IrcMessage* message);
#endif // QT_NO_DEBUG_STREAM

IRC_END_NAMESPACE

Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcMessage::Type))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcAccountMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcAwayMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcBatchMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcCapabilityMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcErrorMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcHostChangeMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcInviteMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcJoinMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcKickMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcModeMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcMotdMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcNamesMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcNickMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcNoticeMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcNumericMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcPartMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcPingMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcPongMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcPrivateMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcQuitMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcTopicMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcWhoisMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcWhowasMessage*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcWhoReplyMessage*))

#endif // IRCMESSAGE_H
