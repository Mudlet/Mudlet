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

    property IrcChannel channel

    signal queried(IrcUser user)

    color: "#edf3fe"

    Rectangle {
        id: frame
        anchors.fill: parent
        color: "transparent"
        border.color: "#aaa"
        anchors.topMargin: -1
        anchors.leftMargin: -1
        anchors.bottomMargin: -1
    }

    ScrollView {
        id: scrollView

        anchors.fill: parent
        anchors.topMargin: -1
        anchors.bottomMargin: -1

        ListView {
            id: listView

            model: IrcUserModel {
                id: userModel
                sortMethod: Irc.SortByTitle
                channel: background.channel
                onChannelChanged: listView.currentIndex = -1
            }

            delegate: Rectangle {
                width: parent.width
                height: Math.max(20, label.implicitHeight + 4)
                color: ListView.isCurrentItem ? "#b5d5ff" : "transparent"
                Label {
                    id: label
                    text: model.title
                    anchors.margins: 2
                    anchors.leftMargin: 6
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                }
                MouseArea {
                    anchors.fill: parent
                    onPressed: listView.currentIndex = index
                    onDoubleClicked: queried(model.user)
                }
            }
        }
    }
}
