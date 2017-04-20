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

    color: "#ddd"
    height: Math.max(20, label.implicitHeight + 4)

    IrcTextFormat {
        id: textFormat
    }

    Label {
        id: label

        anchors.margins: 2
        anchors.leftMargin: 4
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        wrapMode: Text.Wrap
        text: channel && channel.topic ? textFormat.toHtml(channel.topic) : "-"
    }

    Rectangle {
        id: frame

        color: "transparent"
        anchors.fill: parent
        anchors.topMargin: -1
        anchors.leftMargin: -1
        border.color: "#aaa"
    }
}
