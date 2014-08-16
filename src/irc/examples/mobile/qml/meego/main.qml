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
import Communi 1.0
import com.nokia.meego 1.0

PageStackWindow {
    id: window

    initialPage: MainPage { id: mainPage }

    style: PageStackWindowStyle {
        background: "../images/meego-background-portrait.png"
        landscapeBackground: "../images/meego-background-landscape.png"
    }

    Component.onCompleted: {
        SessionManager.restore();
        for (var i = 0; i < SessionModel.length; ++i)
            if (!SessionModel[i].session.hasQuit)
                SessionModel[i].session.reconnect();
        if (!Settings.policyAgreed)
            mainPage.showAbout();
    }

    Component.onDestruction: {
        SessionManager.save();
        for (var i = 0; i < SessionModel.length; ++i)
            SessionModel[i].session.quit(ApplicationName);
    }
}
