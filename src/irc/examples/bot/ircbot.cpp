/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This example is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "ircbot.h"
#include <IrcMessage>
#include <IrcCommand>
#include <QCoreApplication>
#include <QTimer>

IrcBot::IrcBot(QObject* parent) : IrcConnection(parent)
{
//! [messages]
    connect(this, SIGNAL(privateMessageReceived(IrcPrivateMessage*)), this, SLOT(processMessage(IrcPrivateMessage*)));
//! [messages]

//! [commands]
    parser.addCommand(IrcCommand::CtcpAction, "ACT [target] <message...>");
    parser.addCommand(IrcCommand::Custom, "HELP (<command...>)");
    parser.addCommand(IrcCommand::Nick, "NICK <nick>");
    parser.addCommand(IrcCommand::Join, "JOIN <#channel> (<key>)");
    parser.addCommand(IrcCommand::Part, "PART (<#channel>) (<message...>)");
    parser.addCommand(IrcCommand::Quit, "QUIT (<message...>)");
    parser.addCommand(IrcCommand::Message, "SAY [target] <message...>");
//! [commands]

    bufferModel.setConnection(this);
//! [channels]
    connect(&bufferModel, SIGNAL(channelsChanged(QStringList)), &parser, SLOT(setChannels(QStringList)));
//! [channels]
}

void IrcBot::join(QString channel)
{
    sendCommand(IrcCommand::createJoin(channel));
}

//![receive]
void IrcBot::processMessage(IrcPrivateMessage* message)
{
    if (message->isPrivate()) {
        // private message: reply to the message sender
        // => triggers: "!<cmd> <params>" and "<cmd> <params>"
        parser.setTarget(message->nick());
        parser.setTriggers(QStringList() << "!" << "");
    } else {
        // channel message: reply to the target channel
        // => triggers: "!<cmd> <params>" and "bot: <cmd> <params>"
        parser.setTarget(message->target());
        parser.setTriggers(QStringList() << "!" << nickName().append(":"));
    }

    IrcCommand* cmd = parser.parse(message->content());
    if (cmd) {
        if (cmd->type() == IrcCommand::Custom && cmd->parameters().value(0) == "HELP") {
            help(cmd->parameters().mid(1));
        } else {
            sendCommand(cmd);

            if (cmd->type() == IrcCommand::Quit) {
                connect(this, SIGNAL(disconnected()), qApp, SLOT(quit()));
                QTimer::singleShot(1000, qApp, SLOT(quit()));
            }
        }
    }
}
//![receive]

void IrcBot::help(QStringList commands)
{
    if (commands.isEmpty())
        commands = parser.commands();

    QString target = parser.target();
    foreach (const QString& command, commands) {
        QString syntax = parser.syntax(command);
        sendCommand(IrcCommand::createMessage(target, syntax));
    }
}
