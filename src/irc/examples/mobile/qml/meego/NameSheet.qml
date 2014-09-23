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

    property alias name: nameField.text

    acceptable: !nameField.errorHighlight
    onStatusChanged: if (status === DialogStatus.Open) nameField.forceActiveFocus()

    Column {
        id: column
        width: parent.width

        Label { text: qsTr("Nick name") }
        TextField {
            id: nameField
            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            errorHighlight: !text.length
            width: parent.width
            platformSipAttributes: SipAttributes {
                actionKeyEnabled: sheet.acceptable
                actionKeyHighlighted: true
                actionKeyLabel: qsTr("Ok")
            }
            Keys.onReturnPressed: sheet.accept()
        }
    }
}
