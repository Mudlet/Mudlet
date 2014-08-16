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

Sheet {
    id: sheet

    property bool acceptable: false
    default property alias content: contentItem.data
    property alias title: title.text

    acceptButtonText: acceptable ? qsTr("Ok") : ""
    rejectButtonText: qsTr("Cancel")

    content: Rectangle {
        id: background
        anchors.fill: parent
        color: UI.BACKGROUND_COLOR

        Flickable {
            id: flickable
            anchors.fill: parent
            anchors.margins: UI.PAGE_MARGIN
            contentHeight: contentColumn.height

            Column {
                id: contentColumn
                spacing: UI.DEFAULT_SPACING
                Label {
                    id: title
                    color: UI.TITLE_COLOR
                    font.pixelSize: UI.LARGE_FONT
                }
                Rectangle {
                    color: UI.SEPARATOR_COLOR
                    height: 1
                    width: flickable.width
                }
                Item {
                    id: contentItem
                    width: flickable.width
                    height: childrenRect.height
                }
            }
        }
    }
}
