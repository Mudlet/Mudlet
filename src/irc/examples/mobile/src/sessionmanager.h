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

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QDeclarativeContext>
class SessionItem;
class Session;

class SessionManager : public QObject
{
    Q_OBJECT

public:
    SessionManager(QDeclarativeContext* context);

    Q_INVOKABLE void addSession(Session* session);
    Q_INVOKABLE void removeSession(Session* session);

public slots:
    void restore();
    void save();

signals:
    void alert(QObject* item, const QString& text);
    void channelKeyRequired(Session* session, const QString& channel);

private slots:
    void updateModel();

private:
    QObjectList m_items;
    QDeclarativeContext* m_context;
};

#endif // SESSIONMANAGER_H
