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

#ifndef SESSIONTABWIDGET_H
#define SESSIONTABWIDGET_H

#include "tabwidget.h"
#include "messagehandler.h"
#include "settings.h"
#include <QHash>

class Session;
class IrcMessage;
class MessageView;

class SessionTabWidget : public TabWidget
{
    Q_OBJECT

public:
    SessionTabWidget(Session* session, QWidget* parent = 0);

    Session* session() const;

    QByteArray saveSplitter() const;

public slots:
    void restoreSplitter(const QByteArray& state);

    MessageView* openView(const QString& receiver);
    void removeView(const QString& receiver);
    void closeCurrentView();
    void closeView(int index);
    void renameView(const QString& from, const QString& to);
    void applySettings(const Settings& settings);

signals:
    void alerted(MessageView* view, IrcMessage* message);
    void highlighted(MessageView* view, IrcMessage* message);
    void inactiveStatusChanged(bool inactive);
    void sessionClosed(Session* session);
    void splitterChanged(const QByteArray& state);
    void editSession(Session* session);

    void viewAdded(MessageView* view);
    void viewRemoved(MessageView* view);
    void viewRenamed(const QString& from, const QString& to);
    void viewActivated(MessageView* view);

protected:
    bool event(QEvent* event);

private slots:
    void updateStatus();
    void resetTab(int index);
    void tabActivated(int index);
    void onNewTabRequested();
    void onTabMenuRequested(int index, const QPoint& pos);
    void onTabCloseRequested();
    void delayedTabReset();
    void delayedTabResetTimeout();
    void onTabAlerted(IrcMessage* message);
    void onTabHighlighted(IrcMessage* message);
    void onEditSession();

private:
    struct SessionTabWidgetData
    {
        QList<int> delayedIndexes;
        MessageHandler handler;
        QHash<QString, MessageView*> views;
        Settings settings;
    } d;
};

#endif // SESSIONTABWIDGET_H
