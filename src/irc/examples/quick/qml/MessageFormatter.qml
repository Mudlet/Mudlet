/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This example is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

import QtQuick 2.1
import Communi 3.0

QtObject {
    id: root

    property IrcTextFormat textFormat: IrcTextFormat {
        id: textFormat

        palette.gray: "#606060"
        palette.lightGray: "#808080"

        // http://ethanschoonover.com/solarized
        palette.blue: "#268bd2"
        palette.green: "#859900"
        palette.red: "#dc322f"
        palette.brown: "#cb4b16"
        palette.purple: "#6c71c4"
        palette.orange: "#cb4b16"
        palette.yellow: "#b58900"
        palette.lightGreen: "#859900"
        palette.cyan: "#2aa198"
        palette.lightCyan: "#2aa198"
        palette.lightBlue: "#268bd2"
        palette.pink: "#6c71c4"
    }

    function formatMessage(message) {
        var formatted
        switch (message.type) {
            case IrcMessage.Invite:  formatted = formatInviteMessage(message); break
            case IrcMessage.Join:    formatted = formatJoinMessage(message); break
            case IrcMessage.Kick:    formatted = formatKickMessage(message); break
            case IrcMessage.Mode:    formatted = formatModeMessage(message); break
            case IrcMessage.Names:   formatted = formatNamesMessage(message); break
            case IrcMessage.Nick:    formatted = formatNickMessage(message); break
            case IrcMessage.Notice:  formatted = formatNoticeMessage(message); break
            case IrcMessage.Numeric: formatted = formatNumericMessage(message); break
            case IrcMessage.Part:    formatted = formatPartMessage(message); break
            case IrcMessage.Private: formatted = formatPrivateMessage(message); break
            case IrcMessage.Quit:    formatted = formatQuitMessage(message); break
            case IrcMessage.Topic:   formatted = formatTopicMessage(message); break
        }
        return formatText(formatted, message.timeStamp)
    }

    function formatText(text, timeStamp) {
        if (text) {
            switch (text[0]) {
                case '!': text = qsTr("<font color='gray'>%1</font>").arg(text); break;
                case '[': text = qsTr("<font color='brown'>%1</font>").arg(text); break;
                case '*': text = qsTr("<font color='darkmagenta'>%1</font>").arg(text); break;
                case '?': text = qsTr("<font color='brown'>%1</font>").arg(text); break;
                default: break;
            }

            return qsTr("<font color='gray'>[%1]</font> %2").arg(Qt.formatTime(timeStamp, "hh:mm:ss")).arg(text)
        }
    }

    function formatInviteMessage(message) {
        var sender = formatName(message.nick)
        return qsTr("! %1 invited to %3").arg(sender).arg(message.channel)
    }

    function formatJoinMessage(message) {
        var sender = formatName(message.nick)
        return qsTr("! %1 joined %2").arg(sender).arg(message.channel)
    }

    function formatKickMessage(message) {
        var sender = formatName(message.nick)
        var user = formatName(message.user)
        if (message.reason.length)
            return qsTr("! %1 kicked %2 (%3)").arg(sender).arg(user).arg(message.reason)
        return qsTr("! %1 kicked %2").arg(sender).arg(user)
    }

    function formatModeMessage(message) {
        var sender = formatName(message.nick)
        if (message.reply)
            return qsTr("! %1 mode is %2 %3").arg(message.target).arg(message.mode).arg(message.argument)
        return qsTr("! %1 sets mode %2 %3").arg(sender).arg(message.mode).arg(message.argument)
    }

    function formatNamesMessage(message) {
        return qsTr("! %1 has %2 users").arg(message.channel).arg(message.names.length)
    }

    function formatNickMessage(message) {
        var sender = formatName(message.nick)
        var nick = formatName(message.newNick)
        return qsTr("! %1 changed nick to %2").arg(sender).arg(nick)
    }

    function formatNoticeMessage(message) {
        var sender = formatName(message.nick)
        var content = formatHtml(message.content)
        return qsTr("[%1] %2").arg(sender).arg(content)
    }

    function formatNumericMessage(message) {
        switch (message.code) {
        case Irc.RPL_TOPIC:
        case Irc.RPL_TOPICWHOTIME:
        case Irc.RPL_CHANNEL_URL:
        case Irc.RPL_NAMREPLY:
        case Irc.RPL_ENDOFNAMES:
            return // ignore
        default:
            return qsTr("[%1] %2").arg(message.code).arg(message.parameters.slice(1).join(" "))
        }
    }

    function formatPartMessage(message) {
        var sender = formatName(message.nick)
        if (message.reason.length)
            return qsTr("! %1 parted %2 (%3)").arg(sender).arg(message.channel).arg(formatHtml(message.reason))
        return qsTr("! %1 parted %2").arg(sender).arg(message.channel)
    }

    function formatPrivateMessage(message) {
        var sender = formatName(message.nick)
        var content = formatHtml(message.content)
        if (message.action)
            return qsTr("* %1 %2").arg(sender).arg(content)
        if (message.request)
            return qsTr("! %1 requested %2").arg(sender).arg(content.split(" ")[0].toLowerCase())
        return qsTr("&lt;%1&gt; %2").arg(sender).arg(content)
    }

    function formatQuitMessage(message) {
        var sender = formatName(message.nick)
        if (message.reason.length)
            return qsTr("! %1 has quit (%2)").arg(sender).arg(formatHtml(message.reason))
        return qsTr("! %1 has quit").arg(sender)
    }

    function formatTopicMessage(message) {
        var sender = formatName(message.nick)
        var topic = formatHtml(message.topic)
        var channel = message.channel
        if (message.reply)
            return qsTr("! %1 topic is \"%2\"").arg(channel).arg(topic)
        return qsTr("! %1 sets topic \"%2\" on %3").arg(sender).arg(topic).arg(channel)
    }

    function formatHtml(message) {
        return textFormat.toHtml(message)
    }

    function formatName(name) {
        var color = hslToRgb((hashCode(name) % 359)/359, 0.5, 0.25)
        var r = ("0" + Math.round(Math.abs(color[0])).toString(16)).substr(-2)
        var g = ("0" + Math.round(Math.abs(color[1])).toString(16)).substr(-2)
        var b = ("0" + Math.round(Math.abs(color[2])).toString(16)).substr(-2)
        return qsTr("<b><font color='#%1'>%2</font></b>").arg(r+g+b).arg(name);
    }

    function hashCode(str) {
        var hash = 0;
        if (str.length == 0) return hash;
        for (var i = 0; i < str.length; i++) {
            var chr = str.charCodeAt(i);
            hash = ((hash<<5)-hash)+chr;
            hash = hash & hash; // Convert to 32bit integer
        }
        return hash;
    }

    /**
     * Converts an HSL color value to RGB. Conversion formula
     * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
     * Assumes h, s, and l are contained in the set [0, 1] and
     * returns r, g, and b in the set [0, 255].
     *
     * @param   Number  h       The hue
     * @param   Number  s       The saturation
     * @param   Number  l       The lightness
     * @return  Array           The RGB representation
     */
    function hslToRgb(h, s, l){
        var r, g, b;

        function hue2rgb(p, q, t){
            if(t < 0) t += 1;
            if(t > 1) t -= 1;
            if(t < 1/6) return p + (q - p) * 6 * t;
            if(t < 1/2) return q;
            if(t < 2/3) return p + (q - p) * (2/3 - t) * 6;
            return p;
        }

        if(s == 0){
            r = g = b = l; // achromatic
        }else{
            var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
            var p = 2 * l - q;
            r = hue2rgb(p, q, h + 1/3);
            g = hue2rgb(p, q, h);
            b = hue2rgb(p, q, h - 1/3);
        }

        return [r * 255, g * 255, b * 255];
    }
}
