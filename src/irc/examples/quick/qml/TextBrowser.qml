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

Item {
    id: background

    property IrcBuffer buffer

    implicitWidth: textArea.implicitWidth
    implicitHeight: textArea.implicitHeight

    MessageFormatter {
        id: formatter
    }

    Connections {
        target: buffer
        onMessageReceived: {
            var line = formatter.formatMessage(message)
            if (line)
                textArea.append(line)
        }
    }

    TextArea {
        id: textArea

        anchors.fill: parent
        anchors.topMargin: -1
        anchors.leftMargin: -1
        anchors.bottomMargin: -1

        readOnly: true
        textFormat: Qt.RichText
        frameVisible: false
    }
}
