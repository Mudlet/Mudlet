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

Rectangle {
    id: background

    property alias bufferModel: listView.model
    property IrcBuffer currentBuffer

    signal closed(IrcBuffer buffer)

    color: "#edf3fe"

    Rectangle {
        id: frame
        anchors.fill: parent
        anchors.topMargin: -1
        color: "transparent"
        border.color: "#aaa"
    }

    Menu {
        id: menu
        MenuItem {
            text: qsTr("Close")
            shortcut: qsTr("Ctrl+W")
            enabled: !!currentBuffer
            onTriggered: closed(currentBuffer)
        }
    }

    ScrollView {
        id: scrollView

        anchors.fill: parent
        anchors.topMargin: -1

        ListView {
            id: listView

            delegate: Rectangle {
                property bool first: index === 0
                property bool current: model.buffer === currentBuffer
                anchors.left: parent ? parent.left : undefined
                anchors.right: parent ? parent.right : undefined
                anchors.margins: 1
                height: Math.max(21, label.implicitHeight + 5)
                color: first ? "#ddd" : current ? "#b5d5ff" : "transparent"
                Rectangle {
                    visible: first
                    width: parent.width
                    height: 1
                    anchors.bottom: parent.bottom
                    anchors.fill: parent
                    anchors.leftMargin: -1
                    anchors.rightMargin: -1
                    color: "transparent"
                    border.color: "#aaa"
                }
                Label {
                    id: label
                    text: model.title
                    font.bold: first
                    anchors.margins: 2
                    anchors.leftMargin: 6
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onPressed: {
                        currentBuffer = model.buffer
                        if (mouse.button === Qt.RightButton)
                            menu.popup()
                    }
                }
            }
        }
    }
}
