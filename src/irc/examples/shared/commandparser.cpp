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

#include "commandparser.h"
#include <IrcCommand>
#include <QDateTime>
#include <QHash>
#include <QMap>

static QMap<QString, QString>& command_syntaxes()
{
    static QMap<QString, QString> syntaxes;
    if (syntaxes.isEmpty())
    {
        syntaxes.insert("AWAY", "(<reason>)");
        syntaxes.insert("INVITE", "<user>");
        syntaxes.insert("JOIN", "<channel> (<key>)");
        syntaxes.insert("KICK", "<user> (<reason>)");
        syntaxes.insert("ME", "<message>");
        syntaxes.insert("MODE", "<channel/user> <mode> (<arg>)");
        syntaxes.insert("NAMES", "");
        syntaxes.insert("NICK", "<nick>");
        syntaxes.insert("NOTICE", "<channel/user> <message>");
        syntaxes.insert("PART", "(<reason>)");
        syntaxes.insert("PING", "(<user>)");
        syntaxes.insert("QUIT" , "(<message>)");
        syntaxes.insert("QUOTE" , "<command> (<parameters>)");
        syntaxes.insert("TIME", "(<user>)");
        syntaxes.insert("TOPIC", "(<topic>)");
        syntaxes.insert("VERSION", "(<user>)");
        syntaxes.insert("WHOIS", "<user>");
        syntaxes.insert("WHOWAS", "<user>");
    }
    return syntaxes;
}

CommandParser::CommandParser(QObject* parent) : QObject(parent)
{
}

CommandParser::~CommandParser()
{
}

QStringList CommandParser::availableCommands()
{
    return command_syntaxes().keys();
}

QStringList CommandParser::suggestedCommands(const QString& command, const QStringList& params)
{
    QStringList suggestions;
    foreach (const QString& available, availableCommands())
        if (!command.compare(available, Qt::CaseInsensitive) || (params.isEmpty() && available.startsWith(command, Qt::CaseInsensitive)))
            suggestions += available;
    return suggestions;
}

QString CommandParser::syntax(const QString& command)
{
    if (command_syntaxes().contains(command.toUpper()))
        return command.toUpper() + " " + command_syntaxes().value(command.toUpper());
    return QString();
}

void CommandParser::addCustomCommand(const QString& command, const QString& syntax)
{
    command_syntaxes().insert(command.toUpper(), syntax);
}

void CommandParser::removeCustomCommand(const QString& command)
{
    command_syntaxes().remove(command.toUpper());
}

Q_GLOBAL_STATIC(bool, has_error)

bool CommandParser::hasError() const
{
    return *has_error();
}

IrcCommand* CommandParser::parseCommand(const QString& receiver, const QString& text)
{
    *has_error() = false;
    if (text.startsWith("/"))
    {
        typedef IrcCommand*(*ParseFunc)(const QString&, const QStringList&);

        static QHash<QString, ParseFunc> parseFunctions;
        if (parseFunctions.isEmpty())
        {
            parseFunctions.insert("AWAY", &CommandParser::parseAway);
            parseFunctions.insert("INVITE", &CommandParser::parseInvite);
            parseFunctions.insert("JOIN", &CommandParser::parseJoin);
            parseFunctions.insert("KICK", &CommandParser::parseKick);
            parseFunctions.insert("ME", &CommandParser::parseMe);
            parseFunctions.insert("MODE", &CommandParser::parseMode);
            parseFunctions.insert("NAMES", &CommandParser::parseNames);
            parseFunctions.insert("NICK", &CommandParser::parseNick);
            parseFunctions.insert("NOTICE", &CommandParser::parseNotice);
            parseFunctions.insert("PART", &CommandParser::parsePart);
            parseFunctions.insert("PING", &CommandParser::parsePing);
            parseFunctions.insert("QUIT", &CommandParser::parseQuit);
            parseFunctions.insert("QUOTE", &CommandParser::parseQuote);
            parseFunctions.insert("TIME", &CommandParser::parseTime);
            parseFunctions.insert("TOPIC", &CommandParser::parseTopic);
            parseFunctions.insert("VERSION", &CommandParser::parseVersion);
            parseFunctions.insert("WHOIS", &CommandParser::parseWhois);
            parseFunctions.insert("WHOWAS", &CommandParser::parseWhowas);
        }

        const QStringList words = text.mid(1).split(" ", QString::SkipEmptyParts);
        const QString command = words.value(0).toUpper();
        ParseFunc parseFunc = parseFunctions.value(command);
        if (parseFunc)
        {
            IrcCommand* cmd = parseFunc(receiver, words.mid(1));
            if (!cmd)
                *has_error() = true;
            return cmd;
        }
        if (command_syntaxes().contains(command.toUpper()))
        {
            emit customCommand(command, words.mid(1));
            return 0;
        }
    }
    else
    {
        return IrcCommand::createMessage(receiver, text);
    }

    // unknown command
    *has_error() = true;
    return 0;
}

IrcCommand* CommandParser::parseAway(const QString& receiver, const QStringList& params)
{
    Q_UNUSED(receiver);
    return IrcCommand::createAway(params.value(0));
}

IrcCommand* CommandParser::parseInvite(const QString& receiver, const QStringList& params)
{
    if (params.count() == 1)
        return IrcCommand::createInvite(params.at(0), receiver);
    return 0;
}

IrcCommand* CommandParser::parseJoin(const QString& receiver, const QStringList& params)
{
    Q_UNUSED(receiver);
    if (params.count() == 1 || params.count() == 2)
        return IrcCommand::createJoin(params.at(0), params.value(1));
    return 0;
}

IrcCommand* CommandParser::parseKick(const QString& receiver, const QStringList& params)
{
    if (params.count() >= 1)
        return IrcCommand::createKick(receiver, params.at(0), QStringList(params.mid(1)).join(" "));
    return 0;
}

IrcCommand* CommandParser::parseMe(const QString& receiver, const QStringList& params)
{
    if (!params.isEmpty())
        return IrcCommand::createCtcpAction(receiver, params.join(" "));
    return 0;
}

IrcCommand* CommandParser::parseMode(const QString& receiver, const QStringList& params)
{
    Q_UNUSED(receiver);
    if (params.count() >= 2 && params.count() <= 4)
        return IrcCommand::createMode(params.at(0), params.at(1), params.value(2));
    return 0;
}

IrcCommand* CommandParser::parseNames(const QString& receiver, const QStringList& params)
{
    if (params.isEmpty())
        return IrcCommand::createNames(receiver);
    return 0;
}

IrcCommand* CommandParser::parseNick(const QString& receiver, const QStringList& params)
{
    Q_UNUSED(receiver);
    if (params.count() == 1)
        return IrcCommand::createNick(params.at(0));
    return 0;
}

IrcCommand* CommandParser::parseNotice(const QString& receiver, const QStringList& params)
{
    Q_UNUSED(receiver);
    if (params.count() >= 2)
        return IrcCommand::createNotice(params.at(0), QStringList(params.mid(1)).join(" "));
    return 0;
}

IrcCommand* CommandParser::parsePart(const QString& receiver, const QStringList& params)
{
    return IrcCommand::createPart(receiver, params.join(" "));
}

IrcCommand* CommandParser::parsePing(const QString& receiver, const QStringList& params)
{
    Q_UNUSED(receiver);
    QString time = QString::number(QDateTime::currentDateTime().toTime_t());
    if (params.isEmpty())
        return IrcCommand::createQuote(QStringList() << "PING" << time);
    return IrcCommand::createCtcpRequest(params.at(0), "PING " + time);
}

IrcCommand* CommandParser::parseQuit(const QString& receiver, const QStringList& params)
{
    Q_UNUSED(receiver);
    return IrcCommand::createQuit(params.join(" "));
}

IrcCommand* CommandParser::parseQuote(const QString& receiver, const QStringList& params)
{
    Q_UNUSED(receiver);
    return IrcCommand::createQuote(params);
}

IrcCommand* CommandParser::parseTime(const QString& receiver, const QStringList& params)
{
    Q_UNUSED(receiver);
    if (params.isEmpty())
        return IrcCommand::createQuote(QStringList() << "TIME");
    return IrcCommand::createCtcpRequest(params.at(0), "TIME");
}

IrcCommand* CommandParser::parseTopic(const QString& receiver, const QStringList& params)
{
    return IrcCommand::createTopic(receiver, params.join(" "));
}

IrcCommand* CommandParser::parseVersion(const QString& receiver, const QStringList& params)
{
    Q_UNUSED(receiver);
    if (params.isEmpty())
        return IrcCommand::createQuote(QStringList() << "VERSION");
    return IrcCommand::createCtcpRequest(params.at(0), "VERSION");
}

IrcCommand* CommandParser::parseWhois(const QString& receiver, const QStringList& params)
{
    Q_UNUSED(receiver);
    if (params.count() == 1)
        return IrcCommand::createWhois(params.at(0));
    return 0;
}

IrcCommand* CommandParser::parseWhowas(const QString& receiver, const QStringList& params)
{
    Q_UNUSED(receiver);
    if (params.count() == 1)
        return IrcCommand::createWhowas(params.at(0));
    return 0;
}
