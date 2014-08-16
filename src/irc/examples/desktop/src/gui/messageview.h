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

#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#include "ui_messageview.h"
#include "messagereceiver.h"
#include "messageformatter.h"
#include "commandparser.h"
#include "settings.h"

class IrcMessage;
class Session;

class MessageView : public QWidget, public MessageReceiver
{
    Q_OBJECT
    Q_PROPERTY(QString receiver READ receiver WRITE setReceiver)

public:
    enum ViewType { ServerView, ChannelView, QueryView };

    MessageView(ViewType type, Session* session, QWidget* parent = 0);
    ~MessageView();

    ViewType viewType() const;
    Session* session() const;

    QString receiver() const;
    void setReceiver(const QString &receiver);

    QByteArray saveSplitter() const;
    void restoreSplitter(const QByteArray& state);

public slots:
    void showHelp(const QString& text, bool error = false);
    void appendMessage(const QString& message);
    void applySettings(const Settings& settings);

signals:
    void highlighted(IrcMessage* message);
    void alerted(IrcMessage* message);
    void queried(const QString& user);
    void splitterChanged(const QByteArray& state);

protected:
    void hideEvent(QHideEvent *event);

    void receiveMessage(IrcMessage* message);
    bool hasUser(const QString& user) const;

private slots:
    void onEscPressed();
    void onSplitterMoved();
    void onSend(const QString& text);
    void onCustomCommand(const QString& command, const QStringList& params);

private:
    struct MessageViewData : public Ui::MessageView
    {
        ViewType viewType;
        QString receiver;
        Session* session;
        CommandParser parser;
        MessageFormatter formatter;
        Settings settings;
    } d;
};

#endif // MESSAGEVIEW_H
