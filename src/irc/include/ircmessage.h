/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
* License for more details.
*/

#ifndef IRCMESSAGE_H
#define IRCMESSAGE_H

#include <IrcGlobal>
#include <IrcSender>
#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>

class IrcCommand;
class IrcMessagePrivate;

class COMMUNI_EXPORT IrcMessage : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Type type READ type)
    Q_PROPERTY(Flags flags READ flags)
    Q_PROPERTY(bool own READ isOwn)
    Q_PROPERTY(bool valid READ isValid)
    Q_PROPERTY(IrcSender sender READ sender)
    Q_PROPERTY(QString command READ command)
    Q_PROPERTY(QStringList parameters READ parameters)
    Q_ENUMS(Type Flag)
    Q_FLAGS(Flags)

public:
    enum Type
    {
        Unknown,
        Nick,
        Quit,
        Join,
        Part,
        Topic,
        Invite,
        Kick,
        Mode,
        Private,
        Notice,
        Ping,
        Pong,
        Error,
        Numeric,
        Capability
    };

    enum Flag
    {
        None = 0x0,
        Own = 0x1,
        Identified = 0x2,
        Unidentified = 0x4
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    Q_INVOKABLE explicit IrcMessage(QObject* parent = 0);
    virtual ~IrcMessage();

    Type type() const;
    Flags flags() const;

    IrcSender sender() const;
    QString command() const;
    QStringList parameters() const;

    bool isOwn() const;
    virtual bool isValid() const;

    QByteArray encoding() const;
    void setEncoding(const QByteArray& encoding);

    Q_INVOKABLE QByteArray toData() const;
    Q_INVOKABLE static IrcMessage* fromData(const QByteArray& data, QObject* parent = 0);
    Q_INVOKABLE static IrcMessage* fromData(const QByteArray& data, const QByteArray& encoding, QObject* parent = 0);

    Q_DECL_DEPRECATED QString toString() const;
    Q_DECL_DEPRECATED static IrcMessage* fromString(const QString& str, QObject* parent = 0);

    Q_INVOKABLE static IrcMessage* fromCommand(const QString& sender, IrcCommand* command, QObject* parent = 0);

protected:
    QScopedPointer<IrcMessagePrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcMessage)
    Q_DISABLE_COPY(IrcMessage)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(IrcMessage::Flags)

class COMMUNI_EXPORT IrcNickMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString nick READ nick)

public:
    Q_INVOKABLE explicit IrcNickMessage(QObject* parent = 0);

    QString nick() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcNickMessage)
};

class COMMUNI_EXPORT IrcQuitMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString reason READ reason)

public:
    Q_INVOKABLE explicit IrcQuitMessage(QObject* parent = 0);

    QString reason() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcQuitMessage)
};

class COMMUNI_EXPORT IrcJoinMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel)

public:
    Q_INVOKABLE explicit IrcJoinMessage(QObject* parent = 0);

    QString channel() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcJoinMessage)
};

class COMMUNI_EXPORT IrcPartMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel)
    Q_PROPERTY(QString reason READ reason)

public:
    Q_INVOKABLE explicit IrcPartMessage(QObject* parent = 0);

    QString channel() const;
    QString reason() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcPartMessage)
};

class COMMUNI_EXPORT IrcTopicMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel)
    Q_PROPERTY(QString topic READ topic)

public:
    Q_INVOKABLE explicit IrcTopicMessage(QObject* parent = 0);

    QString channel() const;
    QString topic() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcTopicMessage)
};

class COMMUNI_EXPORT IrcInviteMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString user READ user)
    Q_PROPERTY(QString channel READ channel)

public:
    Q_INVOKABLE explicit IrcInviteMessage(QObject* parent = 0);

    QString user() const;
    QString channel() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcInviteMessage)
};

class COMMUNI_EXPORT IrcKickMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel)
    Q_PROPERTY(QString user READ user)
    Q_PROPERTY(QString reason READ reason)

public:
    Q_INVOKABLE explicit IrcKickMessage(QObject* parent = 0);

    QString channel() const;
    QString user() const;
    QString reason() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcKickMessage)
};

class COMMUNI_EXPORT IrcModeMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString target READ target)
    Q_PROPERTY(QString mode READ mode)
    Q_PROPERTY(QString argument READ argument)

public:
    Q_INVOKABLE explicit IrcModeMessage(QObject* parent = 0);

    QString target() const;
    QString mode() const;
    QString argument() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcModeMessage)
};

class COMMUNI_EXPORT IrcPrivateMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString target READ target)
    Q_PROPERTY(QString message READ message)
    Q_PROPERTY(bool action READ isAction)
    Q_PROPERTY(bool request READ isRequest)

public:
    Q_INVOKABLE explicit IrcPrivateMessage(QObject* parent = 0);

    QString target() const;
    QString message() const;
    bool isAction() const;
    bool isRequest() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcPrivateMessage)
};

class COMMUNI_EXPORT IrcNoticeMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString target READ target)
    Q_PROPERTY(QString message READ message)
    Q_PROPERTY(bool reply READ isReply)

public:
    Q_INVOKABLE explicit IrcNoticeMessage(QObject* parent = 0);

    QString target() const;
    QString message() const;
    bool isReply() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcNoticeMessage)
};

class COMMUNI_EXPORT IrcPingMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString argument READ argument)

public:
    Q_INVOKABLE explicit IrcPingMessage(QObject* parent = 0);

    QString argument() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcPingMessage)
};

class COMMUNI_EXPORT IrcPongMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString argument READ argument)

public:
    Q_INVOKABLE explicit IrcPongMessage(QObject* parent = 0);

    QString argument() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcPongMessage)
};

class COMMUNI_EXPORT IrcErrorMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString error READ error)

public:
    Q_INVOKABLE explicit IrcErrorMessage(QObject* parent = 0);

    QString error() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcErrorMessage)
};

class COMMUNI_EXPORT IrcNumericMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(int code READ code)

public:
    Q_INVOKABLE explicit IrcNumericMessage(QObject* parent = 0);

    int code() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcNumericMessage)
};

class COMMUNI_EXPORT IrcCapabilityMessage : public IrcMessage
{
    Q_OBJECT
    Q_PROPERTY(QString subCommand READ subCommand)
    Q_PROPERTY(QStringList capabilities READ capabilities)

public:
    Q_INVOKABLE explicit IrcCapabilityMessage(QObject* parent = 0);

    QString subCommand() const;
    QStringList capabilities() const;

    bool isValid() const;

private:
    Q_DISABLE_COPY(IrcCapabilityMessage)
};

#ifndef QT_NO_DEBUG_STREAM
COMMUNI_EXPORT QDebug operator<<(QDebug debug, const IrcMessage* message);
#endif // QT_NO_DEBUG_STREAM

#endif // IRCMESSAGE_H
