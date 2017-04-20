/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This example is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

import QtQuick 2.1
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0
import Qt.labs.settings 1.0
import Communi 3.0

Item {
    id: page

    property string host: hostField.text || hostField.placeholderText
    property int port: portField.value
    property bool secure: secureField.checked
    property bool sasl: saslField.checked
    property string nickName: nickNameField.text || nickNameField.placeholderText
    property string realName: realNameField.text || realNameField.placeholderText
    property string userName: userNameField.text || userNameField.placeholderText
    property string password: passwordField.text
    property string channel: channelField.text || channelField.placeholderText

    Settings {
        id: settings
        property alias host: hostField.text
        property alias port: portField.value
        property alias secure: secureField.checked
        property alias sasl: saslField.checked
        property alias nickName: nickNameField.text
        property alias realName: realNameField.text
        property alias userName: userNameField.text
        property alias password: passwordField.text
        property alias channel: channelField.text
    }

    signal accepted()
    signal rejected()

    implicitWidth: grid.implicitWidth + row.implicitWidth
    implicitHeight: grid.implicitHeight + row.implicitHeight + 48

    GridLayout {
        id: grid

        width: page.width * 2 / 3
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -row.height + 12

        columns: 2
        rowSpacing: 12
        columnSpacing: 12

        Label { text: qsTr("Host:") }
        TextField {
            id: hostField
            focus: true
            Layout.fillWidth: true
            placeholderText: "irc.freenode.net"
        }

        Label { text: qsTr("Port:") }
        RowLayout {
            SpinBox {
                id: portField
                value: 6667
                minimumValue: 1024
                maximumValue: 65535
                Layout.fillWidth: true
            }
            CheckBox {
                id: secureField
                text: qsTr("SSL")
            }
            CheckBox {
                id: saslField
                text: qsTr("SASL")
            }
        }

        Label { text: qsTr("Nick name:") }
        TextField {
            id: nickNameField
            Layout.fillWidth: true
            placeholderText: "Quick" + Math.round(Math.random() * 9999)
        }

        Label { text: qsTr("Real name:") }
        TextField {
            id: realNameField
            Layout.fillWidth: true
            placeholderText: qsTr("Communi %1 QtQuick example").arg(irc.version())
        }

        Label { text: qsTr("User name:") }
        TextField {
            id: userNameField
            Layout.fillWidth: true
            placeholderText: "communi"
        }

        Label { text: qsTr("Password:") }
        TextField {
            id: passwordField
            echoMode: TextInput.Password
            Layout.fillWidth: true
        }

        Label { text: qsTr("Channel:") }
        TextField {
            id: channelField
            Layout.fillWidth: true
            placeholderText: "#freenode"
        }
    }

    Keys.onReturnPressed: {
        if (okButton.enabled)
            page.accepted()
    }

    Keys.onEnterPressed: {
        if (okButton.enabled)
            page.accepted()
    }

    Keys.onEscapePressed: {
        if (cancelButton.enabled)
            page.rejected()
    }

    RowLayout {
        id: row

        anchors.margins: 12
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        Item { Layout.fillWidth: true }

        Button {
            id: okButton
            text: qsTr("Ok")
            enabled: page.visible
            onClicked: page.accepted()
        }

        Button {
            id: cancelButton
            text: qsTr("Cancel")
            onClicked: page.rejected()
        }
    }
}
