/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef MESSAGEFORMATTER_H
#define MESSAGEFORMATTER_H

#include <QHash>
#include <QObject>
#include <IrcMessage>

class UserModel;

class MessageFormatter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList highlights READ highlights WRITE setHighlights)
    Q_PROPERTY(bool timeStamp READ timeStamp WRITE setTimeStamp)
    Q_PROPERTY(bool stripNicks READ stripNicks WRITE setStripNicks)
    Q_PROPERTY(QString timeStampFormat READ timeStampFormat WRITE setTimeStampFormat)
    Q_PROPERTY(QString messageFormat READ messageFormat WRITE setMessageFormat)
    Q_PROPERTY(QString eventFormat READ eventFormat WRITE setEventFormat)
    Q_PROPERTY(QString noticeFormat READ noticeFormat WRITE setNoticeFormat)
    Q_PROPERTY(QString actionFormat READ actionFormat WRITE setActionFormat)
    Q_PROPERTY(QString unknownFormat READ unknownFormat WRITE setUnknownFormat)
    Q_PROPERTY(QString highlightFormat READ highlightFormat WRITE setHighlightFormat)

public:
    explicit MessageFormatter(QObject* parent = 0);
    virtual ~MessageFormatter();

    QStringList highlights() const;
    void setHighlights(const QStringList& highlights);

    bool timeStamp() const;
    void setTimeStamp(bool timeStamp);

    bool stripNicks() const;
    void setStripNicks(bool strip);

    QString timeStampFormat() const;
    void setTimeStampFormat(const QString& format);

    QString messageFormat() const;
    void setMessageFormat(const QString& format);

    QString eventFormat() const;
    void setEventFormat(const QString& format);

    QString noticeFormat() const;
    void setNoticeFormat(const QString& format);

    QString actionFormat() const;
    void setActionFormat(const QString& format);

    QString unknownFormat() const;
    void setUnknownFormat(const QString& format);

    QString highlightFormat() const;
    void setHighlightFormat(const QString& format);

    Q_INVOKABLE QString formatMessage(IrcMessage* message, UserModel* userModel = 0) const;
    Q_INVOKABLE QString formatMessage(const QString& message) const;

protected:
    QString formatInviteMessage(IrcInviteMessage* message) const;
    QString formatJoinMessage(IrcJoinMessage* message) const;
    QString formatKickMessage(IrcKickMessage* message) const;
    QString formatModeMessage(IrcModeMessage* message) const;
    QString formatNickMessage(IrcNickMessage* message) const;
    QString formatNoticeMessage(IrcNoticeMessage* message) const;
    QString formatNumericMessage(IrcNumericMessage* message) const;
    QString formatPartMessage(IrcPartMessage* message) const;
    QString formatPongMessage(IrcPongMessage* message) const;
    QString formatPrivateMessage(IrcPrivateMessage* message) const;
    QString formatQuitMessage(IrcQuitMessage* message) const;
    QString formatTopicMessage(IrcTopicMessage* message) const;
    QString formatUnknownMessage(IrcMessage* message) const;

    static QString formatPingReply(const IrcSender& sender, const QString& arg);

    static QString formatSender(const IrcSender& sender, bool strip = true);
    static QString formatUser(const QString& user, bool strip = true);

    static QString formatIdleTime(int secs);

    QString formatHtml(const QString& message) const;

private:
    mutable struct Private
    {
        bool highlight;
        bool timeStamp;
        bool stripNicks;
        UserModel* userModel;
        QStringList highlights;
        QString timeStampFormat;
        QString messageFormat;
        QString highlightFormat;
        QHash<QString, QString> prefixedFormats;
        QList<int> receivedCodes;
    } d;
};

#endif // MESSAGEFORMATTER_H
