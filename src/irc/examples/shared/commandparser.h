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

#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

#include <QObject>
#include <QStringList>

class IrcCommand;

class CommandParser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool error READ hasError)
    Q_PROPERTY(QStringList availableCommands READ availableCommands)

public:
    explicit CommandParser(QObject* parent = 0);
    virtual ~CommandParser();

    static QStringList availableCommands();
    static Q_INVOKABLE QStringList suggestedCommands(const QString& command, const QStringList& params);
    static Q_INVOKABLE QString syntax(const QString& command);

    static Q_INVOKABLE void addCustomCommand(const QString& command, const QString& syntax);
    static Q_INVOKABLE void removeCustomCommand(const QString& command);

    bool hasError() const;
    Q_INVOKABLE IrcCommand* parseCommand(const QString& receiver, const QString& text);

signals:
    void customCommand(const QString& command, const QStringList& params);

private:
    static IrcCommand* parseAway(const QString& receiver, const QStringList& params);
    static IrcCommand* parseInvite(const QString& receiver, const QStringList& params);
    static IrcCommand* parseJoin(const QString& receiver, const QStringList& params);
    static IrcCommand* parseKick(const QString& receiver, const QStringList& params);
    static IrcCommand* parseMe(const QString& receiver, const QStringList& params);
    static IrcCommand* parseMode(const QString& receiver, const QStringList& params);
    static IrcCommand* parseNames(const QString& receiver, const QStringList& params);
    static IrcCommand* parseNick(const QString& receiver, const QStringList& params);
    static IrcCommand* parseNotice(const QString& receiver, const QStringList& params);
    static IrcCommand* parsePart(const QString& receiver, const QStringList& params);
    static IrcCommand* parsePing(const QString& receiver, const QStringList& params);
    static IrcCommand* parseQuit(const QString& receiver, const QStringList& params);
    static IrcCommand* parseQuote(const QString& receiver, const QStringList& params);
    static IrcCommand* parseTime(const QString& receiver, const QStringList& params);
    static IrcCommand* parseTopic(const QString& receiver, const QStringList& params);
    static IrcCommand* parseVersion(const QString& receiver, const QStringList& params);
    static IrcCommand* parseWhois(const QString& receiver, const QStringList& params);
    static IrcCommand* parseWhowas(const QString& receiver, const QStringList& params);
};

#endif // COMMANDPARSER_H
