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

#include "userlistview.h"
#include "sortedusermodel.h"
#include "usermodel.h"
#include "session.h"
#include <QItemSelectionModel>
#include <QContextMenuEvent>
#include <QScrollBar>
#include <QAction>
#include <QMenu>

UserListView::UserListView(QWidget* parent) : QListView(parent)
{
    d.userModel = new UserModel(this);
    connect(this, SIGNAL(doubleClicked(QModelIndex)), SLOT(onDoubleClicked(QModelIndex)));
}

UserListView::~UserListView()
{
}

QSize UserListView::sizeHint() const
{
    return QSize(16 * fontMetrics().width('#') + verticalScrollBar()->sizeHint().width(), QListView::sizeHint().height());
}

Session* UserListView::session() const
{
    return d.userModel->session();
}

void UserListView::setSession(Session* session)
{
    delete model();
    if (session)
        setModel(new SortedUserModel(session->prefixModes(), d.userModel));

    d.userModel->setSession(session);
}

QString UserListView::channel() const
{
    return d.userModel->channel();
}

void UserListView::setChannel(const QString &channel)
{
    d.userModel->setChannel(channel);
}

UserModel* UserListView::userModel() const
{
    return d.userModel;
}

bool UserListView::hasUser(const QString &user) const
{
    return d.userModel->hasUser(user);
}

void UserListView::processMessage(IrcMessage* message)
{
    d.userModel->processMessage(message);
}

void UserListView::contextMenuEvent(QContextMenuEvent* event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.isValid())
    {
        QMenu* menu = createMenu(index, this);
        menu->exec(event->globalPos());
        menu->deleteLater();
    }
}

void UserListView::mousePressEvent(QMouseEvent* event)
{
    QListView::mousePressEvent(event);
    if (!indexAt(event->pos()).isValid())
        selectionModel()->clear();
}

void UserListView::onDoubleClicked(const QModelIndex& index)
{
    if (index.isValid())
        emit doubleClicked(index.data(Qt::EditRole).toString());
}

void UserListView::onWhoisTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
    {
        IrcCommand* command = IrcCommand::createWhois(action->data().toString());
        emit commandRequested(command);
        command->deleteLater();
    }
}

void UserListView::onQueryTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
        emit queried(action->data().toString());
}

void UserListView::onModeTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
    {
        QStringList params = action->data().toStringList();
        IrcCommand* command = IrcCommand::createMode(channel(), params.at(1), params.at(0));
        emit commandRequested(command);
        command->deleteLater();
    }
}

void UserListView::onKickTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
    {
        IrcCommand* command = IrcCommand::createKick(channel(), action->data().toString());
        emit commandRequested(command);
        command->deleteLater();
    }
}

void UserListView::onBanTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action)
    {
        IrcCommand* command = IrcCommand::createMode(channel(), "+b", action->data().toString() + "!*@*");
        emit commandRequested(command);
        command->deleteLater();
    }
}

QMenu* UserListView::createMenu(const QModelIndex& index, QWidget* parent)
{
    QMenu* menu = new QMenu(parent);

    QAction* action = 0;
    QString user = index.data(Qt::EditRole).toString();
    QString modes = index.data(Qt::UserRole).toString();

    action = menu->addAction(tr("Whois"), this, SLOT(onWhoisTriggered()));
    action->setData(user);

    action = menu->addAction(tr("Query"), this, SLOT(onQueryTriggered()));
    action->setData(user);

    menu->addSeparator();

    if (modes.contains("@"))
    {
        action = menu->addAction(tr("Deop"), this, SLOT(onModeTriggered()));
        action->setData(QStringList() << user << "-o");
    }
    else
    {
        action = menu->addAction(tr("Op"), this, SLOT(onModeTriggered()));
        action->setData(QStringList() << user << "+o");
    }

    if (modes.contains("+"))
    {
        action = menu->addAction(tr("Devoice"), this, SLOT(onModeTriggered()));
        action->setData(QStringList() << user << "-v");
    }
    else
    {
        action = menu->addAction(tr("Voice"), this, SLOT(onModeTriggered()));
        action->setData(QStringList() << user << "+v");
    }

    menu->addSeparator();

    action = menu->addAction(tr("Kick"), this, SLOT(onKickTriggered()));
    action->setData(user);

    action = menu->addAction(tr("Ban"), this, SLOT(onBanTriggered()));
    action->setData(user);

    return menu;
}
