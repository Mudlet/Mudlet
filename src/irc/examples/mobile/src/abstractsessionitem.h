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

#ifndef ABSTRACTSESSIONITEM_H
#define ABSTRACTSESSIONITEM_H

#include <QObject>
#include <QStringListModel>
#include "messageformatter.h"
#include "messagereceiver.h"

class IrcMessage;
class Session;

class AbstractSessionItem : public QObject, public MessageReceiver
{
    Q_OBJECT
    Q_PROPERTY(Session* session READ session CONSTANT)
    Q_PROPERTY(QString receiver READ receiver WRITE setReceiver)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString subtitle READ subtitle WRITE setSubtitle NOTIFY subtitleChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(bool busy READ isBusy WRITE setBusy NOTIFY busyChanged)
    Q_PROPERTY(bool current READ isCurrent WRITE setCurrent NOTIFY currentChanged)
    Q_PROPERTY(bool highlighted READ isHighlighted WRITE setHighlighted NOTIFY highlightedChanged)
    Q_PROPERTY(bool timeStamp READ timeStamp WRITE setTimeStamp NOTIFY timeStampChanged)
    Q_PROPERTY(bool stripNicks READ stripNicks WRITE setStripNicks NOTIFY stripNicksChanged)
    Q_PROPERTY(int unreadCount READ unreadCount WRITE setUnreadCount NOTIFY unreadCountChanged)
    Q_PROPERTY(int unseenIndex READ unseenIndex WRITE setUnseenIndex NOTIFY unseenIndexChanged)
    Q_PROPERTY(QObject* messages READ messages CONSTANT)

public:
    explicit AbstractSessionItem(QObject *parent = 0);
    virtual ~AbstractSessionItem();

    Session* session() const;

    QString receiver() const;
    void setReceiver(const QString &receiver);

    QString title() const;
    QString subtitle() const;
    QString description() const;
    bool isBusy() const;
    bool isCurrent() const;
    bool isHighlighted() const;
    bool timeStamp() const;
    bool stripNicks() const;
    int unreadCount() const;
    int unseenIndex() const;

    QObject* messages() const;

    virtual QStringList completions(const QString& prefix, const QString& word) const;
    virtual void updateCurrent(AbstractSessionItem* item) = 0;
    virtual void receiveMessage(IrcMessage* message);
    virtual bool hasUser(const QString& user) const;

public slots:
    void setTitle(const QString& title);
    void setSubtitle(const QString& subtitle);
    void setDescription(const QString& description);
    void setBusy(bool busy);
    void setCurrent(bool current);
    void setHighlighted(bool highlighted);
    void setTimeStamp(bool timeStamp);
    void setStripNicks(bool strip);
    void setUnreadCount(int count);
    void setUnseenIndex(int index);
    void clear();

signals:
    void iconChanged();
    void titleChanged(const QString& title);
    void subtitleChanged();
    void descriptionChanged();
    void busyChanged();
    void currentChanged();
    void highlightedChanged();
    void timeStampChanged();
    void stripNicksChanged();
    void unreadCountChanged();
    void unseenIndexChanged();
    void removed();

protected:
    void setSession(Session* session);
    const MessageFormatter* messageFormatter() const;

protected slots:
    void appendMessage(const QString& message);

private:
    Session* m_session;
    QString m_title;
    QString m_subtitle;
    QString m_description;
    bool m_busy;
    bool m_current;
    bool m_highlighted;
    QStringListModel* m_messages;
    MessageFormatter m_formatter;
    int m_unread;
    int m_unseen;
};

#endif // ABSTRACTSESSIONITEM_H
