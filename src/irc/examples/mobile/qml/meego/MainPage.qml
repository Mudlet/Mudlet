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
import QtMultimediaKit 1.1
import QtMobility.feedback 1.1
import com.nokia.meego 1.0
import com.nokia.extras 1.0
import "UIConstants.js" as UI

CommonPage {
    id: root

    property alias bouncer: bouncer

    function showAbout() {
        var dialog = about.createObject(root, {showPolicy: !Settings.policyAgreed});
        dialog.open();
    }

    header: Header {
        id: header
        title: qsTr("Communi")
        icon.visible: pressed
        icon.source: "../images/about.png"
        onClicked: root.showAbout()
    }

    tools: ToolBarLayout {
        ToolIcon {
            iconId: "toolbar-add"
            anchors.right: parent.right
            onClicked: connectionSheet.open()
        }
    }

    ListView {
        id: listView

        property QtObject currentSessionItem
        property QtObject currentSession
        property QtObject currentChildItem

        anchors.fill: parent
        model: SessionModel
        delegate: Column {
            width: parent.width
            ListItem {
                title: modelData.title
                subtitle: modelData.subtitle
                iconSource: modelData.session.currentLag > 10000 ? "../images/unknown.png" :
                            modelData.session.active && !modelData.session.connected ? "../images/server.png" :
                            modelData.session.connected ? "../images/connected.png" : "../images/disconnected.png"
                highlighted: modelData.highlighted
                active: modelData.session.active
                unreadCount: modelData.unreadCount
                busy: modelData.busy
                onClicked: chatPage.push(modelData)
                onPressAndHold: {
                    channelSheet.session = modelData.session;
                    listView.currentSessionItem = modelData;
                    listView.currentSession = modelData.session;
                    if (modelData.session.active)
                        activeSessionMenu.open();
                    else
                        inactiveSessionMenu.open();
                }
            }
            Repeater {
                id: repeater
                model: modelData.childItems
                ListItem {
                    title: modelData.title
                    subtitle: modelData.subtitle
                    iconSource: modelData.channel ? "../images/channel.png" : "../images/query.png"
                    highlighted: modelData.highlighted
                    active: modelData.session.active
                    unreadCount: modelData.unreadCount
                    busy: modelData.busy
                    onClicked: chatPage.push(modelData)
                    onPressAndHold: {
                        listView.currentChildItem = modelData;
                        listView.currentSession = modelData.session;
                        childMenu.open();
                    }
                }
            }
            Rectangle {
                color: UI.SEPARATOR_COLOR
                height: 1
                visible: index < listView.count - 1
                anchors {
                    left: parent.left
                    right: parent.right
                    leftMargin: UI.PAGE_MARGIN
                    rightMargin: UI.PAGE_MARGIN
                }
            }
        }
    }

    ScrollDecorator {
        flickableItem: listView
    }

    Component {
        id: bannerComponent
        InfoBanner {
            id: banner
            timerShowTime: 5000
            property QtObject item
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    bouncer.bounce(item, null);
                    banner.hide();
                }
            }
            Connections {
                target: root.pageStack
                onCurrentPageChanged: banner.hide()
            }
            onVisibleChanged: {
                if (!banner.visible)
                    banner.destroy();
            }
        }
    }

    FileEffect {
        id: vibraEffect
        source: "file:///usr/share/sounds/vibra/tct_chat.ivt"
    }

    SoundEffect {
        id: soundEffect
        source: "file:///usr/share/sounds/ui-tones/snd_chat_fg.wav"
    }

    Connections {
        target: SessionManager
        onAlert: {
            var banner = bannerComponent.createObject(pageStack.currentPage.__headerItem);
            banner.text = text;
            banner.item = item;
            banner.show();
            if (Qt.application.active)
                soundEffect.play();
            else
                vibraEffect.start();
        }
    }

    ChatPage {
        id: chatPage
        function push(data) {
            modelData = data;
            root.pageStack.push(chatPage);
        }
        onStatusChanged: {
            if (modelData) {
                modelData.current = (chatPage.status === PageStatus.Active);
                if (status === PageStatus.Inactive) {
                    modelData.unseenIndex = chatPage.count - 1;
                    modelData = null;
                }
                else if (status === PageStatus.Active && bouncer.cmd) {
                    modelData.session.sendUiCommand(bouncer.cmd);
                    bouncer.cmd = null;
                }
            }
            if (status === PageStatus.Inactive && bouncer.item)
                bouncer.start();
        }
    }

    Timer {
        id: bouncer
        interval: 50
        property QtObject item
        property QtObject cmd
        function bounce(item, cmd) {
            bouncer.cmd = cmd;
            if (root.status === PageStatus.Active) {
                bouncer.item = null;
                chatPage.push(item);
            } else {
                bouncer.item = item;
                pageStack.pop();
            }
        }
        onTriggered: {
            chatPage.push(bouncer.item);
            bouncer.item = null;
        }
    }

    ContextMenu {
        id: activeSessionMenu

        MenuLayout {
            MenuItem {
                text: qsTr("Join channel")
                onClicked: channelSheet.open()
            }
            MenuItem {
                text: qsTr("Open query")
                onClicked: querySheet.open()
            }
            MenuItem {
                text: qsTr("Set nick")
                onClicked: {
                    nickSheet.name = listView.currentSession.nickName;
                    nickSheet.open();
                }
            }
            MenuItem {
                text: qsTr("Disconnect")
                onClicked: {
                    listView.currentSession.quit(ApplicationName);
                }
            }
        }
    }

    ContextMenu {
        id: inactiveSessionMenu

        MenuLayout {
            MenuItem {
                text: qsTr("Reconnect")
                onClicked: {
                    listView.currentSession.reconnect();
                }
            }
            MenuItem {
                text: qsTr("Close")
                onClicked: {
                    SessionManager.removeSession(listView.currentSession);
                    listView.currentSession.destructLater();
                }
            }
        }
    }

    ConnectionSheet {
        id: connectionSheet
        title: qsTr("Add connection")

        Component.onCompleted: {
            connectionSheet.host = Settings.host;
            connectionSheet.port = Settings.port;
            connectionSheet.name = Settings.name;
            if (Settings.user !== "") connectionSheet.user = Settings.user;
            if (Settings.real !== "") connectionSheet.real = Settings.real;
            connectionSheet.channel = Settings.channel;
            connectionSheet.secure = Settings.secure;
        }

        Component {
            id: sessionComponent
            Session { }
        }

        onAccepted: {
            var session = sessionComponent.createObject(root);
            session.nickName = connectionSheet.name;
            session.userName = connectionSheet.user.length ? connectionSheet.user : "communi";
            session.realName = connectionSheet.real.length ? connectionSheet.real : ApplicationName;
            session.host = connectionSheet.host;
            session.port = connectionSheet.port;
            session.password = connectionSheet.password;
            session.secure = connectionSheet.secure;
            if (connectionSheet.channel.length)
                session.addChannel(connectionSheet.channel);

            SessionManager.addSession(session);
            session.reconnect();

            connectionSheet.password = "";
            Settings.host = session.host;
            Settings.port = session.port;
            Settings.name = session.nickName;
            Settings.user = session.userName;
            Settings.real = session.realName;
            Settings.channel = connectionSheet.channel;
            Settings.secure = session.secure;
        }
    }

    IrcCommand {
        id: ircCommand
    }

    ChannelSheet {
        id: channelSheet
        title: qsTr("Join channel")
        onAccepted: {
            var child = listView.currentSessionItem.addChild(channel);
            var cmd = ircCommand.createJoin(channel, password);
            listView.currentSession.sendUiCommand(cmd);
            bouncer.bounce(child, null);
        }
        Connections {
            target: SessionManager
            onChannelKeyRequired: {
                channelSheet.session = session;
                channelSheet.channel = channel;
                channelSheet.passwordRequired = true;
                channelSheet.open();
            }
        }
    }

    NameSheet {
        id: querySheet
        title: qsTr("Open query")
        onAccepted: {
            var child = listView.currentSessionItem.addChild(name);
            bouncer.bounce(child, null);
        }
    }

    NameSheet {
        id: nickSheet
        title: qsTr("Set nick")
        onAccepted: {
            listView.currentSession.nickName = name;
        }
    }

    ContextMenu {
        id: childMenu

        MenuLayout {
            MenuItem {
                text: listView.currentChildItem && listView.currentChildItem.channel && listView.currentSession && listView.currentSession.connected ? qsTr("Part") : qsTr("Close")
                onClicked: {
                    var item = listView.currentChildItem;
                    if (item.channel) {
                        var cmd = ircCommand.createPart(item.title, ApplicationName);
                        item.session.sendUiCommand(cmd);
                    }
                    item.sessionItem.removeChild(item.title);
                }
            }
        }
    }

    Component {
        id: about
        QueryDialog {
            id: dialog

            property bool showPolicy: false
            property string policy: qsTr("<p>PLEASE REVIEW THE <a href='http://github.com/communi/communi/wiki/Privacy-Policy'>PRIVACY POLICY</a>.</p>")
            property string license: qsTr("<p><small>This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.</small></p>" +
                                          "<p><small>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.</small></p>")

            titleText: ApplicationName
            message: qsTr("<p>Communi is an IRC (Internet Relay Chat) client used to communicate with others on IRC networks around the world.</p>" +
                          "<p>Copyright (C) 2012 J-P Nurmi <a href=\"mailto:jpnurmi@gmail.com\">jpnurmi@gmail.com</a><br/></p>" +
                          "%1").arg(showPolicy ? policy : license)
            onLinkActivated: Qt.openUrlExternally(link)

            acceptButtonText: showPolicy ? qsTr("I AGREE") : qsTr("OK")
            onAccepted: { Settings.policyAgreed = true; dialog.destroy(1000) }

            rejectButtonText: showPolicy ? qsTr("I DISAGREE") : ""
            onRejected: { showPolicy ? Qt.quit() : dialog.destroy(1000) }
        }
    }
}
