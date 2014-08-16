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

#include "usermodel.h"
#include "session.h"
#include <ircmessage.h>
#include <ircsender.h>
#include <irc.h>

UserModel::UserModel(QObject* parent) : QAbstractListModel(parent)
{
    d.session = qobject_cast<Session*>(parent);
}

UserModel::~UserModel()
{
}

Session* UserModel::session() const
{
    return d.session;
}

void UserModel::setSession(Session* session)
{
    d.session = session;
    clearUsers();
}

QString UserModel::channel() const
{
    return d.channel;
}

void UserModel::setChannel(const QString &channel)
{
    d.channel = channel.toLower();
}

QStringList UserModel::users() const
{
    return d.names;
}

bool UserModel::hasUser(const QString &user) const
{
    return d.names.contains(user, Qt::CaseInsensitive);
}

void UserModel::addUser(const QString& user)
{
    addUsers(QStringList() << user);
}

void UserModel::addUsers(const QStringList& users)
{
    QStringList unique;
    QSet<QString> nameSet = d.names.toSet();
    foreach (const QString& user, users)
    {
        QString name = d.session->unprefixedUser(user);
        if (!nameSet.contains(name))
            unique += user;
    }

    if (!unique.isEmpty())
    {
        beginInsertRows(QModelIndex(), rowCount(), rowCount() + unique.count() - 1);
        foreach (const QString& user, unique)
        {
            QString name = d.session->unprefixedUser(user);
            d.names += name;
            d.modes.insert(name, d.session->userPrefix(user));
        }
        endInsertRows();
    }
}

void UserModel::removeUser(const QString& user)
{
    QString name = d.session->unprefixedUser(user);
    int idx = d.names.indexOf(name);
    if (idx != -1)
    {
        beginRemoveRows(QModelIndex(), idx, idx);
        d.names.removeAt(idx);
        d.modes.remove(name);
        endRemoveRows();
    }
}

void UserModel::clearUsers()
{
    if (!d.names.isEmpty())
    {
        beginResetModel();
        d.names.clear();
        endResetModel();
    }
}

void UserModel::renameUser(const QString& from, const QString& to)
{
    int idx = d.names.indexOf(from);
    if (idx != -1)
    {
        d.names[idx] = to;
        d.modes[to] = d.modes.take(from);
        emit dataChanged(index(idx, 0), index(idx, 0));
    }
}

void UserModel::setUserMode(const QString& user, const QString& mode)
{
    int idx = d.names.indexOf(user);
    if (idx != -1)
    {
        bool add = true;
        QString updated = d.modes.value(user);
        for (int i = 0; i < mode.size(); ++i)
        {
            QChar c = mode.at(i);
            QString m = d.session->prefixTypeToMode(c);
            switch (c.unicode())
            {
                case '+':
                    add = true;
                    break;
                case '-':
                    add = false;
                    break;
                default:
                    if (add)
                    {
                        if (!updated.contains(m))
                            updated += m;
                    }
                    else
                    {
                        updated.remove(m);
                    }
                    break;
            }
        }
        d.modes[user] = updated;
        emit dataChanged(index(idx, 0), index(idx, 0));
    }
}

int UserModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return d.names.count();
}

QVariant UserModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= d.names.count())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::UserRole)
    {
        QString name = d.names.at(index.row());
        if (role == Qt::DisplayRole)
            return d.modes.value(name).left(1) + name;
        if (role == Qt::UserRole)
            return d.modes.value(name);
        return name;
    }

    return QVariant();
}

void UserModel::processMessage(IrcMessage* message)
{
    if (!d.session)
    {
        qWarning() << "UserModel::processMessage(): session is null!";
        return;
    }

    if (message->type() == IrcMessage::Nick)
    {
        QString nick = message->sender().name().toLower();
        renameUser(nick, static_cast<IrcNickMessage*>(message)->nick());
    }
    else if (message->type() == IrcMessage::Join)
    {
        if (message->isOwn())
            clearUsers();
        else
            addUser(message->sender().name());
    }
    else if (message->type() == IrcMessage::Part)
    {
        if (message->isOwn())
            clearUsers();
        else
            removeUser(message->sender().name());
    }
    else if (message->type() == IrcMessage::Kick)
    {
        removeUser(static_cast<IrcKickMessage*>(message)->user());
    }
    else if (message->type() == IrcMessage::Quit)
    {
        removeUser(message->sender().name());
    }
    else if (message->type() == IrcMessage::Mode)
    {
        IrcModeMessage* modeMsg = static_cast<IrcModeMessage*>(message);
        if (modeMsg->sender().name() != modeMsg->target() && !modeMsg->argument().isEmpty())
            setUserMode(modeMsg->argument(), modeMsg->mode());
    }
    else if (message->type() == IrcMessage::Numeric)
    {
        if (static_cast<IrcNumericMessage*>(message)->code() == Irc::RPL_NAMREPLY)
        {
            int count = message->parameters().count();
            if (!d.channel.isNull() && d.channel == message->parameters().value(count - 2).toLower())
            {
                QString names = message->parameters().value(count - 1);
                addUsers(names.split(" ", QString::SkipEmptyParts));
            }
        }
    }
}
