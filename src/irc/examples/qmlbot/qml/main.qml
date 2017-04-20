/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This example is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

import QtQml 2.0
import Communi 3.0

QtObject {
    id: bot

    property string channel: "#botwar"

    property Irc irc: Irc { id: irc }
    property IrcCommand command: IrcCommand { id: command }

    property IrcBufferModel model: IrcBufferModel {
        id: model

//! [connection]
        connection: IrcConnection {
            id: connection

            host: "irc.freenode.net"
            userName: "communi"
            nickName: "QmlBot" + Math.round(Math.random() * 9999)
            realName: qsTr("Communi %1 QML bot example").arg(irc.version())

            Component.onCompleted: {
                // queue a command to automatically join a channel when connected
                sendCommand(command.createJoin(channel))
                open()
            }
//! [connection]

//! [receive]
            onMessageReceived: {
                if (message.type === IrcMessage.Private) {
                    // - in private, reply to the message sender
                    // - on channel, reply to the target channel
                    parser.target = message.private ? message.nick : message.target

                    var command = parser.parse(message.content)
                    if (command) {
                        // send the command to the IRC server
                        sendCommand(command)
                        if (command.type === IrcCommand.Quit) {
                            // close the connection & quit the app when a !quit command was received
                            close()
                            Qt.quit()
                        }
                    }
                }
            }
//! [receive]
        }
    }

//! [parser]
    property IrcCommandParser parser: IrcCommandParser {
        id: parser

        // keep the parser aware of the context
        channels: model.channels

        // - on channel, respond to: "!<command> <params>" and "bot: <command> <params>"
        // - in query, respond to: "!<command> <params>" and "<command> <params>"
        triggers: connection.network.isChannel(target) ? ["!", connection.nickName + ":"] : ["!", ""]

        Component.onCompleted: {
            // teach the bot some commands
            parser.addCommand(IrcCommand.Nick, "NICK <nick>");
            parser.addCommand(IrcCommand.Join, "JOIN <#channel> (<key>)");
            parser.addCommand(IrcCommand.Part, "PART (<#channel>) (<message...>)");
            parser.addCommand(IrcCommand.Quit, "QUIT (<message...>)");
            parser.addCommand(IrcCommand.Message, "SAY [target] <message...>");
        }
    }
//! [parser]
}
