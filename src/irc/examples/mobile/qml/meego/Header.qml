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
import com.nokia.extras 1.0
import "UIConstants.js" as UI

BorderImage {
    id: header

    property alias title: label.text
    property alias subtitle: sublabel.text
    property bool busy: false
    property bool active: true
    property bool expanded: false
    property bool pressed: mouseArea.pressed && mouseArea.containsMouse
    property alias icon: icon

    signal clicked()

    property real baseHeight: screen.currentOrientation === Screen.Landscape ? UI.HTB_LANDSCAPE_HEIGHT : UI.HTB_PORTRAIT_HEIGHT

    width: parent.width
    height: baseHeight + (expanded && subtitle ? sublabel.height + UI.DEFAULT_SPACING : 0)
    source: "image://theme/meegotouch-sheet-header-background"

    Connections {
        target: Qt.application
        onActiveChanged: {
            if (!Qt.application.active)
                header.busy = false;
        }
    }

    Behavior on height { NumberAnimation { duration: 100 } }

    border {
        top: 10
        left: 10
        right: 10
        bottom: 2
    }

    MouseArea {
        id: mouseArea
        clip: true
        anchors.fill: parent
        onClicked: {
            header.busy = false;
            header.clicked();
        }

        Label {
            id: label

            elide: Text.ElideRight
            font.pixelSize: UI.LARGE_FONT
            verticalAlignment: Text.AlignVCenter
            color: header.active ? UI.TITLE_COLOR : UI.INACTIVE_COLOR

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: busyIndicator.left
            anchors.leftMargin: UI.PAGE_MARGIN
            anchors.rightMargin: UI.DEFAULT_SPACING
            height: header.baseHeight
        }

        BusyIndicator {
            id: busyIndicator

            running: header.busy
            visible: header.busy

            anchors.right: parent.right
            anchors.rightMargin: UI.PAGE_MARGIN
            anchors.verticalCenter: label.verticalCenter
        }

        Image {
            id: icon
            anchors.centerIn: busyIndicator
        }

        Label {
            id: sublabel

            font.pixelSize: UI.SMALL_FONT
            color: header.active ? UI.SUBTITLE_COLOR : UI.INACTIVE_COLOR

            anchors.top: label.bottom
            anchors.left: label.left
            anchors.right: busyIndicator.right

            onLinkActivated: {
                header.busy = true;
                Qt.openUrlExternally(link);
            }
        }
    }
}
