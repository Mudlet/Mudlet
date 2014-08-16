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

import QtQuick 1.1
import com.nokia.meego 1.0
import "UIConstants.js" as UI

CommonSheet {
    id: sheet

    property alias host: hostField.text
    property alias port: portField.text
    property alias password: passField.text
    property alias name: nameField.text
    property alias user: userField.text
    property alias real: realField.text
    property alias channel: chanField.text
    property alias secure: secureBox.checked

    acceptable: name != "" && host != "" && port != ""

    onStatusChanged: if (status == DialogStatus.Open) hostField.forceActiveFocus()

    SipAttributes {
        id: sipAttributes
        actionKeyHighlighted: true
        actionKeyLabel: qsTr("Next")
    }

    Column {
        width: parent.width
        Column {
            width: parent.width
            spacing: UI.DEFAULT_SPACING

            Row {
                width: parent.width
                spacing: UI.DEFAULT_SPACING
                Column {
                    width: parent.width * 3/4 - UI.DEFAULT_SPACING/2
                    Label { text: qsTr("Host") }
                    TextField {
                        id: hostField
                        text: "irc.freenode.net"
                        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhUrlCharactersOnly
                        width: parent.width
                        errorHighlight: !text.length
                        platformSipAttributes: sipAttributes
                        Keys.onReturnPressed: portField.forceActiveFocus()
                    }
                }
                Column {
                    width: parent.width * 1/4 - UI.DEFAULT_SPACING/2
                    Label { text: qsTr("Port") }
                    TextField {
                        id: portField
                        text: "6667"
                        inputMethodHints: Qt.ImhDigitsOnly
                        width: parent.width
                        errorHighlight: !text.length
                        platformSipAttributes: sipAttributes
                        Keys.onReturnPressed: passField.forceActiveFocus()
                    }
                }
            }

            Row {
                width: parent.width
                spacing: UI.DEFAULT_SPACING
                Column {
                    id: passColumn
                    width: hostField.width
                    Label { text: qsTr("Password") }
                    TextField {
                        id: passField
                        echoMode: TextInput.PasswordEchoOnEdit
                        width: parent.width
                        platformSipAttributes: sipAttributes
                        Keys.onReturnPressed: nameField.forceActiveFocus()
                    }
                }
                CheckBox {
                    id: secureBox
                    text: qsTr("SSL")
                    anchors.bottom: passColumn.bottom
                    width: portField.width
                }
            }

            Row {
                width: parent.width
                spacing: UI.DEFAULT_SPACING
                Column {
                    width: parent.width / 2 - UI.DEFAULT_SPACING/2
                    Label { text: qsTr("Nick name") }
                    TextField {
                        id: nameField
                        text: "Guest" + Math.floor(Math.random() * 12345)
                        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                        width: parent.width
                        errorHighlight: !text.length
                        platformSipAttributes: sipAttributes
                        Keys.onReturnPressed: chanField.forceActiveFocus()
                    }
                }
                Column {
                    width: parent.width / 2 - UI.DEFAULT_SPACING/2
                    Label { text: qsTr("Channel") }
                    TextField {
                        id: chanField
                        text: "#freenode"
                        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                        width: parent.width
                        platformSipAttributes: sipAttributes
                        Keys.onReturnPressed: advancedColumn.collapsed ? hostField.forceActiveFocus() : userField.forceActiveFocus()
                    }
                }
            }

            Item {
                clip: true
                width: parent.width
                height: advancedColumn.collapsed ? 0 : (advancedColumn.implicitHeight + UI.DEFAULT_SPACING)
                Behavior on height { NumberAnimation { } }

                Rectangle {
                    color: UI.SEPARATOR_COLOR
                    height: 1
                    width: parent.width
                }

                Column {
                    id: advancedColumn
                    width: parent.width
                    y: UI.DEFAULT_SPACING
                    spacing: UI.DEFAULT_SPACING

                    property bool collapsed: true

                    Column {
                        width: parent.width
                        Label { text: qsTr("User name") }
                        TextField {
                            id: userField
                            text: "communi"
                            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                            width: parent.width
                            errorHighlight: !text.length
                            platformSipAttributes: sipAttributes
                            Keys.onReturnPressed: realField.forceActiveFocus()
                        }
                    }

                    Column {
                        width: parent.width
                        Label { text: qsTr("Real name") }
                        TextField {
                            id: realField
                            text: ApplicationName
                            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                            width: parent.width
                            errorHighlight: !text.length
                            platformSipAttributes: sipAttributes
                            Keys.onReturnPressed: hostField.forceActiveFocus()
                        }
                    }
                }
            }
        }

        ToolIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            iconId: advancedColumn.collapsed ? "icon-m-common-expand" : "icon-m-common-collapse"
            onClicked: advancedColumn.collapsed = !advancedColumn.collapsed
        }
    }
}
