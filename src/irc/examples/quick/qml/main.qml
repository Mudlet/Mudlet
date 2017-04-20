/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This example is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

import QtQuick 2.1
import QtQuick.Controls 1.0
import Communi 3.0

ApplicationWindow {
    id: window

    visible: true
    title: qsTr("Communi %1 QtQuick example").arg(irc.version())

    width: 800
    height: 480

    minimumWidth: connectPage.implicitWidth
    minimumHeight: connectPage.implicitHeight

    color: Qt.darker(palette.base, 1.06)

    SystemPalette { id: palette }

    Irc { id: irc }
    IrcCommand { id: cmd }

    ConnectPage {
        id: connectPage
        anchors.fill: parent
        visible: !connection.active
        onAccepted: {
            chatPage.currentBuffer = serverBuffer
            connection.sendCommand(cmd.createJoin(channel))
            connection.open()
        }
        onRejected: Qt.quit()
    }

    ChatPage {
        id: chatPage
        anchors.fill: parent
        visible: connection.active
        bufferModel: IrcBufferModel {
            id: bufferModel
            sortMethod: Irc.SortByTitle
            connection: IrcConnection {
                id: connection
                host: connectPage.host
                port: connectPage.port
                secure: connectPage.secure
                saslMechanism: connectPage.sasl ? supportedSaslMechanisms[0] : ""
                nickName: connectPage.nickName
                realName: connectPage.realName
                userName: connectPage.userName
                password: connectPage.password
            }
            onMessageIgnored: serverBuffer.receiveMessage(message)
            function quit() {
                bufferModel.clear()
                connection.quit(qsTr("Communi %1 QtQuick example").arg(irc.version()))
                connection.close()
            }
        }
        serverBuffer: IrcBuffer {
            id: serverBuffer
            sticky: true
            persistent: true
            name: connection.displayName
            Component.onCompleted: bufferModel.add(serverBuffer)
        }
    }

    Component.onDestruction: bufferModel.quit()
}
