/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2022 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2016 by Eric Wallace - eewallace@gmail.com              *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2022-2023 by Lecker Kebap - Leris@mudlet.org            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "TLuaInterpreter.h"

#include "EAction.h"
#include "Host.h"
#include "TAlias.h"
#include "TArea.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TEvent.h"
#include "TFlipButton.h"
#include "TForkedProcess.h"
#include "TLabel.h"
#include "TMapLabel.h"
#include "TMedia.h"
#include "TRoomDB.h"
#include "TTabBar.h"
#include "TTextEdit.h"
#include "TTimer.h"
#include "dlgComposer.h"
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgModuleManager.h"
#include "dlgTriggerEditor.h"
#include "mapInfoContributorManager.h"
#include "mudlet.h"
#if defined(INCLUDE_3DMAPPER)
#include "glwidget_integration.h"
#endif

#include <limits>
#include <math.h>

#include <QtConcurrent>
#include <QCollator>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QTableWidget>
#include <QToolTip>
#include <QFileInfo>
#include <QMovie>
#include <QVector>
#ifdef QT_TEXTTOSPEECH_LIB
#include <QTextToSpeech>
#endif // QT_TEXTTOSPEECH_LIB


// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#connectToServer
int TLuaInterpreter::connectToServer(lua_State* L)
{
    // The lua_tointeger(...) call can return a 64-bit integer number, on
    // Windows Platform that is bigger than the int32_t type (a.k.a. "int" AND
    // "long" types on that platform)! 8-O
    lua_Integer port = 23;
    bool isToSaveToProfile = false;

    Host& host = getHostFromLua(L);
    const QString url = getVerifiedString(L, __func__, 1, "url");

    if (!lua_isnoneornil(L, 2)) {
        port = getVerifiedInt(L, __func__, 2, "port number {default = 23}", true);
        if (port > 65535 || port < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "invalid port number %1 given, if supplied it must be in range 1 to 65535, {defaults to 23 if not provided}").arg(port));
        }
    }

    // Optional argument to save this new connection to disk for this profile.
    if (!lua_isnoneornil(L, 3)) {
        isToSaveToProfile = getVerifiedBool(L, __func__, 3, "save host name and port number", true);
    }

    if (isToSaveToProfile) {
        QPair<bool, QString> result = host.writeProfileData(QLatin1String("url"), url);
        if (!result.first) {
            return warnArgumentValue(L, __func__, qsl("unable to save host name, reason: %1").arg(result.second));
        }

        result = host.writeProfileData(QLatin1String("port"), QString::number(port));
        if (!result.first) {
            return warnArgumentValue(L, __func__, qsl("unable to save port number, reason: %1").arg(result.second));
        }
    }

    host.mTelnet.connectIt(url, port);

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disconnect
int TLuaInterpreter::disconnect(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.mTelnet.disconnectIt();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#downloadFile
int TLuaInterpreter::downloadFile(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const QString localFile = getVerifiedString(L, __func__, 1, "local filename");
    const QString urlString = getVerifiedString(L, __func__, 2, "remote url");
    const QUrl url = QUrl::fromUserInput(urlString);
    if (!url.isValid()) {
        return warnArgumentValue(L, __func__, qsl("url is invalid, reason: %1").arg(url.errorString()));
    }

    QNetworkRequest request = QNetworkRequest(url);
    mudlet::self()->setNetworkRequestDefaults(url, request);

    host.updateProxySettings(host.mLuaInterpreter.mpFileDownloader);
    QNetworkReply* reply = host.mLuaInterpreter.mpFileDownloader->get(request);
    host.mLuaInterpreter.downloadMap.insert(reply, localFile);
    connect(reply, &QNetworkReply::downloadProgress, reply, [=](qint64 bytesDownloaded, qint64 totalBytes) {
        raiseDownloadProgressEvent(L, urlString, bytesDownloaded, totalBytes);
        if (mudlet::smDebugMode) {
            auto& lHost = getHostFromLua(L);
            TDebug(Qt::white, Qt::blue) << "downloadFile: " << bytesDownloaded << "/" << totalBytes
                                        << " bytes ready for " << reply->url().toString() << "\n" >> &lHost;
        }
    });

    if (mudlet::smDebugMode) {
        TDebug(Qt::white, Qt::blue) << "downloadFile: start download from " << reply->url().toString() << "\n" >> &host;
    }

    lua_pushboolean(L, true);
    lua_pushstring(L, reply->url().toString().toUtf8().constData()); // Returns the Url that was ACTUALLY used
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getConnectionInfo
int TLuaInterpreter::getConnectionInfo(lua_State *L)
{
    const Host& host = getHostFromLua(L);

    auto [hostName, hostPort, connected] = host.mTelnet.getConnectionInfo();
    lua_pushstring(L, hostName.toUtf8().constData());
    lua_pushnumber(L, hostPort);
    lua_pushboolean(L, connected);
    return 3;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getIrcChannels
int TLuaInterpreter::getIrcChannels(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    QStringList channels;
    if (pHost->mpDlgIRC) {
        channels = pHost->mpDlgIRC->getChannels();
    } else {
        channels = dlgIRC::readIrcChannels(pHost);
    }

    lua_newtable(L);
    const int total = channels.count();
    for (int i = 0; i < total; ++i) {
        lua_pushnumber(L, i + 1);
        lua_pushstring(L, channels[i].toUtf8().data());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getIrcConnectedHost
int TLuaInterpreter::getIrcConnectedHost(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    QString cHostName;
    QString error = qsl("no client active");
    if (pHost->mpDlgIRC) {
        cHostName = pHost->mpDlgIRC->getConnectedHost();

        if (cHostName.isEmpty()) {
            error = qsl("not yet connected");
        }
    }

    if (cHostName.isEmpty()) {
        return warnArgumentValue(L, __func__, error, true);
    } else {
        lua_pushboolean(L, true);
        lua_pushstring(L, cHostName.toUtf8().constData());
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getIrcNick
int TLuaInterpreter::getIrcNick(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    QString nick;
    if (pHost->mpDlgIRC) {
        nick = pHost->mpDlgIRC->getNickName();
    } else {
        nick = dlgIRC::readIrcNickName(pHost);
    }

    lua_pushstring(L, nick.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getIrcServer
int TLuaInterpreter::getIrcServer(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    QString hname;
    int hport = 0;
    bool hsecure = false;
    if (pHost->mpDlgIRC) {
        hname = pHost->mpDlgIRC->getHostName();
        hport = pHost->mpDlgIRC->getHostPort();
        hsecure = pHost->mpDlgIRC->getHostSecure();
    } else {
        hname = dlgIRC::readIrcHostName(pHost);
        hport = dlgIRC::readIrcHostPort(pHost);
        hsecure = dlgIRC::readIrcHostSecure(pHost);
    }

    lua_pushstring(L, hname.toUtf8().constData());
    lua_pushinteger(L, hport);
    lua_pushboolean(L, hsecure);
    return 3;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getNetworkLatency
int TLuaInterpreter::getNetworkLatency(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.mTelnet.networkLatencyTime);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#openUrl
int TLuaInterpreter::openUrl(lua_State* L)
{
    const QString url = getVerifiedString(L, __func__, 1, "url");
    QDesktopServices::openUrl(url);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#reconnect
int TLuaInterpreter::reconnect(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.mTelnet.reconnect();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#restartIrc
int TLuaInterpreter::restartIrc(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    bool rv = false;
    if (pHost->mpDlgIRC) {
        pHost->mpDlgIRC->ircRestart();
        rv = true;
    }

    lua_pushboolean(L, rv);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendATCP
int TLuaInterpreter::sendATCP(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "sendATCP: bad argument #1 type (message as string expected, got %1!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    const std::string msg = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 1));

    std::string what;
    if (lua_gettop(L) > 1) {
        if (!lua_isstring(L, 2)) {
            lua_pushfstring(L, "sendATCP: bad argument #2 type (what as string is optional, got %1!)", luaL_typename(L, 2));
            return lua_error(L);
        }
        what = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 2));
    }

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_ATCP;
    output += msg;
    if (!what.empty()) {
        output += " ";
        output += what;
    }
    output += TN_IAC;
    output += TN_SE;

    if (!host.mTelnet.isATCPEnabled()) {
        return warnArgumentValue(L, __func__, "ATCP is not currently enabled");
    }

    // output is in Mud Server Encoding form here:
    if (!host.mTelnet.socketOutRaw(output)) {
        return warnArgumentValue(L, __func__, "unable to send all of the ATCP message");
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendGMCP
int TLuaInterpreter::sendGMCP(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "sendGMCP: bad argument #1 type (message as string expected, got %1!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    const std::string msg = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 1));

    std::string what;
    if (lua_gettop(L) > 1) {
        if (!lua_isstring(L, 2)) {
            lua_pushfstring(L, "sendGMCP: bad argument #2 type (what as string is optional, got %1!)", luaL_typename(L, 2));
            return lua_error(L);
        }
        what = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 2));
    }

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_GMCP;
    output += msg;
    if (!what.empty()) {
        output += " ";
        output += what;
    }
    output += TN_IAC;
    output += TN_SE;

    if (!host.mTelnet.isGMCPEnabled()) {
        return warnArgumentValue(L, __func__, "GMCP is not currently enabled");
    }

    // output is in Mud Server Encoding form here:
    if (!host.mTelnet.socketOutRaw(output)) {
        return warnArgumentValue(L, __func__, "unable to send all of the GMCP message");
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendIrc
int TLuaInterpreter::sendIrc(lua_State* L)
{
    const QString target = getVerifiedString(L, __func__, 1, "target");
    const QString msg = getVerifiedString(L, __func__, 2, "message");

    Host* pHost = &getHostFromLua(L);
    if (!pHost->mpDlgIRC) {
        // create a new irc client if one isn't ready.
        pHost->mpDlgIRC = new dlgIRC(pHost);
        pHost->mpDlgIRC->raise();
        pHost->mpDlgIRC->show();
    }

    // wait for our client to be ready before sending messages.
    if (!pHost->mpDlgIRC->mReadyForSending) {
        return warnArgumentValue(L, __func__, "not ready to send just yet");
    }

    const auto result = pHost->mpDlgIRC->sendMsg(target, msg);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendMSDP
int TLuaInterpreter::sendMSDP(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const int n = lua_gettop(L);

    if (n < 1) {
        lua_pushstring(L, "sendMSDP: bad argument #1 type (variable name as string expected, got nil!)");
        return lua_error(L);
    }

    for (int i = 1; i <= n; ++i) {
        if (!lua_isstring(L, i)) {
            lua_pushfstring(L, "sendMSDP: bad argument #%d type (%s as string expected, got %s!)", i, (i == 1 ? "variable name" : "value"), luaL_typename(L, i));
            return lua_error(L);
        }
    }

    const std::string variable = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 1));

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_MSDP;
    output += MSDP_VAR;
    output += variable;

    for (int i = 2; i <= n; ++i) {
        output += MSDP_VAL;
        output += host.mTelnet.encodeAndCookBytes(lua_tostring(L, i));
    }

    output += TN_IAC;
    output += TN_SE;

    // output is in Mud Server Encoding form here:
    host.mTelnet.socketOutRaw(output);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendTelnetChannel102
int TLuaInterpreter::sendTelnetChannel102(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "sendTelnetChannel102: bad argument #1 type (message bytes {2 characters} as string expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }
    const std::string msg = lua_tostring(L, 1);
    if (msg.length() != 2) {
        return warnArgumentValue(L, __func__, qsl(
            "invalid message of length %1 supplied, it should be two bytes (may use lua \\### for each byte where ### is a number between 1 and 254)")
            .arg(msg.length()));
    }

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_102;
    output += msg;
    output += TN_IAC;
    output += TN_SE;

    Host& host = getHostFromLua(L);
    if (!host.mTelnet.isChannel102Enabled()) {
        return warnArgumentValue(L, __func__, "unable to send message as the 102 subchannel support has not been enabled by the game server");
    }
    // We have already validated output to contain a 2 byte payload so we
    // should not need to worry about the "encoding" in this use of
    // socketOutRaw(...) - with the exception of handling any occurrence of
    // 0xFF as either of the bytes to send - however Aardwolf does not use
    // *THAT* value so, though it is probably okay to not worry about the
    // need to "escape" it to get it through the telnet protocol unscathed
    // it is trivial to fix:
    output = mudlet::replaceString(output, "\xff", "\xff\xff");
    host.mTelnet.socketOutRaw(output);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setIrcChannels
int TLuaInterpreter::setIrcChannels(lua_State* L)
{
    QStringList newchannels;
    if (!lua_istable(L, 1)) {
        lua_pushfstring(L, "setIrcChannels: bad argument #1 type (channels as table expected, got %s!)", lua_typename(L, lua_type(L, 1)));
        return lua_error(L);
    }
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        if (lua_type(L, -1) == LUA_TSTRING) {
            const QString c = lua_tostring(L, -1);
            if (!c.isEmpty() && (c.startsWith(QLatin1String("#")) || c.startsWith(QLatin1String("&")) || c.startsWith(QLatin1String("+")))) {
                newchannels << c;
            }
        }
        lua_pop(L, 1);
    }

    if (newchannels.empty()) {
        return warnArgumentValue(L, __func__, "no (valid) channel names provided");
    }

    Host* pHost = &getHostFromLua(L);
    QPair<bool, QString> const result = dlgIRC::writeIrcChannels(pHost, newchannels);
    if (!result.first) {
        return warnArgumentValue(L, __func__, qsl("unable to save channels, reason: %1").arg(result.second));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setIrcNick
int TLuaInterpreter::setIrcNick(lua_State* L)
{
    const QString nick = getVerifiedString(L, __func__, 1, "nick");
    if (nick.isEmpty()) {
        return warnArgumentValue(L, __func__, "nick must not be empty");
    }

    Host* pHost = &getHostFromLua(L);
    QPair<bool, QString> const result = dlgIRC::writeIrcNickName(pHost, nick);
    if (!result.first) {
        return warnArgumentValue(L, __func__, qsl("unable to save nick name, reason: %1").arg(result.second));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setIrcServer
int TLuaInterpreter::setIrcServer(lua_State* L)
{
    const int args = lua_gettop(L);
    int secure = false;
    int port = 6667;
    QString password;
    const std::string addr = getVerifiedString(L, __func__, 1, "hostname").toStdString();
    if (addr.empty()) {
        return warnArgumentValue(L, __func__, "hostname must not be empty");
    }
    if (!lua_isnoneornil(L, 2)) {
        port = getVerifiedInt(L, __func__, 2, "port number {default = 6667}", true);
        if (port > 65535 || port < 1) {
            return warnArgumentValue(L, __func__, qsl("invalid port number %1 given, if supplied it must be in range 1 to 65535").arg(port));
        }
    }
    if (args > 2) {
        secure = getVerifiedBool(L, __func__, 3, "secure {default = false}", true);
    }
    if (args > 3) {
            password = getVerifiedString(L, __func__, 4, "server password", true);
    }

    Host* pHost = &getHostFromLua(L);
    QPair<bool, QString> result = dlgIRC::writeIrcHostName(pHost, QString::fromStdString(addr));
    if (!result.first) {
        return warnArgumentValue(L, __func__, qsl("unable to save hostname, reason: %1").arg(result.second));
    }

    result = dlgIRC::writeIrcHostPort(pHost, port);
    if (!result.first) {
        return warnArgumentValue(L, __func__, qsl("unable to save port, reason: %1").arg(result.second));
    }

    result = dlgIRC::writeIrcHostSecure(pHost, secure);
    if (!result.first) {
        return warnArgumentValue(L, __func__, qsl("unable to save secure, reason: %1").arg(result.second));
    }

    result = dlgIRC::writeIrcPassword(pHost, password);
    if (!result.first) {
        return warnArgumentValue(L, __func__, qsl("unable to save password, reason: %1").arg(result.second));
    }

    lua_pushboolean(L, true);
    lua_pushnil(L);
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getHTTP
int TLuaInterpreter::getHTTP(lua_State* L)
{
    auto& host = getHostFromLua(L);
    const QString urlString = getVerifiedString(L, __func__, 1, "remote url");
    const QUrl url = QUrl::fromUserInput(urlString);
    if (!url.isValid()) {
        return warnArgumentValue(L, __func__, qsl("url is invalid, reason: %1").arg(url.errorString()));
    }

    QNetworkRequest request = QNetworkRequest(url);
    mudlet::self()->setNetworkRequestDefaults(url, request);

    if (!lua_istable(L, 2) && !lua_isnoneornil(L, 2)) {
        lua_pushfstring(L, "getHTTP: bad argument #2 type (headers as a table expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    if (lua_istable(L, 2)) {
        lua_pushnil(L);
        while (lua_next(L, 2) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING && lua_type(L, -2) == LUA_TSTRING) {
                request.setRawHeader(QByteArray(lua_tostring(L, -2)), QByteArray(lua_tostring(L, -1)));
            } else {
                lua_pushfstring(L,
                                "getHTTP: bad argument #2 type (custom headers must be strings, got header: %s (should be string) and value: %s (should be string))",
                                luaL_typename(L, -2),
                                luaL_typename(L, -1));
                return lua_error(L);
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
    }

    host.updateProxySettings(host.mLuaInterpreter.mpFileDownloader);
    QNetworkReply* reply = host.mLuaInterpreter.mpFileDownloader->get(request);

    if (mudlet::smDebugMode) {
        TDebug(Qt::white, Qt::blue) << qsl("getHTTP: script is getting data from %1\n").arg(reply->url().toString()) >> &host;
    }

    lua_pushboolean(L, true);
    lua_pushstring(L, reply->url().toString().toUtf8().constData()); // Returns the Url that was ACTUALLY used
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#postHTTP
int TLuaInterpreter::postHTTP(lua_State* L)
{
    return performHttpRequest(L, __func__, 0, QNetworkAccessManager::PostOperation, qsl("post"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#putHTTP
int TLuaInterpreter::putHTTP(lua_State* L)
{
    return performHttpRequest(L, __func__, 0, QNetworkAccessManager::PutOperation, qsl("put"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Networking_Functions#deleteHTTP
int TLuaInterpreter::deleteHTTP(lua_State *L)
{
    auto& host = getHostFromLua(L);
    const QString urlString = getVerifiedString(L, __func__, 1, "remote url");
    const QUrl url = QUrl::fromUserInput(urlString);
    if (!url.isValid()) {
        return warnArgumentValue(L, __func__, qsl("url is invalid, reason: %1").arg(url.errorString()));
    }

    QNetworkRequest request = QNetworkRequest(url);
    mudlet::self()->setNetworkRequestDefaults(url, request);

    if (!lua_istable(L, 2) && !lua_isnoneornil(L, 2)) {
        lua_pushfstring(L, "deleteHTTP: bad argument #2 type (headers as a table expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    if (lua_istable(L, 2)) {
        lua_pushnil(L);
        while (lua_next(L, 2) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING && lua_type(L, -2) == LUA_TSTRING) {

                request.setRawHeader(QByteArray(lua_tostring(L, -2)), QByteArray(lua_tostring(L, -1)));
            } else {
                lua_pushfstring(L,
                                "deleteHTTP: bad argument #2 type (custom headers must be strings, got header: %s (should be string) and value: %s (should be string))",
                                luaL_typename(L, -2),
                                luaL_typename(L, -1));
                return lua_error(L);
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
    }

    host.updateProxySettings(host.mLuaInterpreter.mpFileDownloader);
    QNetworkReply* reply = host.mLuaInterpreter.mpFileDownloader->deleteResource(request);

    if (mudlet::smDebugMode) {
        TDebug(Qt::white, Qt::blue) << qsl("deleteHTTP: script is sending delete request for %1\n").arg(reply->url().toString()) >> &host;
    }

    lua_pushboolean(L, true);
    lua_pushstring(L, reply->url().toString().toUtf8().constData()); // Returns the Url that was ACTUALLY used
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#customHTTP
int TLuaInterpreter::customHTTP(lua_State* L)
{
    auto customMethod = getVerifiedString(L, __func__, 1, "http method");
    return performHttpRequest(L, __func__, 1, QNetworkAccessManager::CustomOperation, customMethod);
}

