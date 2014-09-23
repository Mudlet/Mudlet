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

#ifndef USERMODEL_H
#define USERMODEL_H

#include <QStringList>
#include <QAbstractListModel>

class Session;
class IrcMessage;

class UserModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString channel READ channel WRITE setChannel)

public:
    explicit UserModel(QObject* parent = 0);
    virtual ~UserModel();

    Session* session() const;
    void setSession(Session* session);

    QString channel() const;

    QStringList users() const;
    bool hasUser(const QString& user) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

public slots:
    void setChannel(const QString& channel);
    void processMessage(IrcMessage* message);

protected:
    void addUser(const QString& user);
    void addUsers(const QStringList& users);
    void removeUser(const QString& user);
    void clearUsers();
    void renameUser(const QString& from, const QString& to);
    void setUserMode(const QString& user, const QString& mode);

private:
    struct Private
    {
        QString channel;
        Session* session;
        QStringList names;
        QHash<QString, QString> modes;
    } d;
};

#endif // USERMODEL_H
