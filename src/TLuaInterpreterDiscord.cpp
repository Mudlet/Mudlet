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

// mapper-specific functions of TLuaInterpreter, split out separately
// for convenience and to keep TLuaInterpreter.cpp size reasonable

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

#include "pre_guard.h"
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
#include "post_guard.h"

// No documentation available in wiki - internal function
std::pair<bool, QString> TLuaInterpreter::discordApiEnabled(lua_State* L, bool writeAccess)
{
    mudlet* pMudlet = mudlet::self();

    if (!pMudlet->mDiscord.libraryLoaded()) {
        return {false, qsl("Discord API is not available")};
    }

    auto& host = getHostFromLua(L);
    if (!(host.mDiscordAccessFlags & Host::DiscordLuaAccessEnabled)) {
        return {false, qsl("Discord API is disabled in settings for privacy")};
    }

    if (writeAccess && !pMudlet->mDiscord.discordUserIdMatch(&host)) {
        return {false, qsl("Discord API is read-only as you're logged in with a different account in Discord compared to the one you entered for this profile")};
    }

    return {true, QString()};
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#usingMudletsDiscordID
int TLuaInterpreter::usingMudletsDiscordID(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }

    lua_pushboolean(L, pMudlet->mDiscord.usingMudletsDiscordID(&host));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordDetail
int TLuaInterpreter::getDiscordDetail(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetDetail)) {
        return warnArgumentValue(L, __func__, "access to Discord detail is disabled in settings for privacy");
    }

    lua_pushfstring(L, pMudlet->mDiscord.getDetailText(&host).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordLargeIcon
int TLuaInterpreter::getDiscordLargeIcon(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetLargeIcon)) {
        return warnArgumentValue(L, __func__, "access to Discord large icon is disabled in settings for privacy");
    }

    lua_pushfstring(L, pMudlet->mDiscord.getLargeImage(&host).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordLargeIconText
int TLuaInterpreter::getDiscordLargeIconText(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L, true);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetLargeIconText)) {
        return warnArgumentValue(L, __func__, "access to Discord large icon text is disabled in settings for privacy");
    }

    lua_pushfstring(L, pMudlet->mDiscord.getLargeImageText(&host).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordParty
int TLuaInterpreter::getDiscordParty(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L, true);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetPartyInfo)) {
        return warnArgumentValue(L, __func__, "access to Discord party info is disabled in settings for privacy");
    }

    QPair<int, int> const partyValues = pMudlet->mDiscord.getParty(&host);
    lua_pushnumber(L, partyValues.first);
    lua_pushnumber(L, partyValues.second);
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordSmallIcon
int TLuaInterpreter::getDiscordSmallIcon(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L, true);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetSmallIcon)) {
        return warnArgumentValue(L, __func__, "access to Discord small icon is disabled in settings for privacy");
    }

    lua_pushfstring(L, pMudlet->mDiscord.getSmallImage(&host).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordSmallIconText
int TLuaInterpreter::getDiscordSmallIconText(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L, true);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetSmallIconText)) {
        return warnArgumentValue(L, __func__, "access to Discord small icon text is disabled in settings for privacy");
    }

    lua_pushfstring(L, pMudlet->mDiscord.getSmallImageText(&host).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordState
int TLuaInterpreter::getDiscordState(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetState)) {
        return warnArgumentValue(L, __func__, "access to Discord state is disabled in settings for privacy");
    }

    lua_pushfstring(L, pMudlet->mDiscord.getStateText(&host).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordTimeStamps
int TLuaInterpreter::getDiscordTimeStamps(lua_State* L)
{
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetTimeInfo)) {
        return warnArgumentValue(L, __func__, "access to Discord time is disabled in settings for privacy");
    }

    QPair<int64_t, int64_t> const timeStamps = mudlet::self()->mDiscord.getTimeStamps(&host);
    lua_pushnumber(L, timeStamps.first);
    lua_pushnumber(L, timeStamps.second);
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetDiscordData
int TLuaInterpreter::resetDiscordData(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    pMudlet->mDiscord.resetData(&host);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordApplicationID
int TLuaInterpreter::setDiscordApplicationID(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L, true);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    }

    if (!lua_gettop(L)) {
        pMudlet->mDiscord.setApplicationID(&host, QString());
        lua_pushboolean(L, true);
        return 1;
    }
    const QString inputText = getVerifiedString(L, __func__, 1, "Discord application ID").trimmed();
    // Treat it as a UTF-8 string because although it is likely to be an
    // unsigned long long integer (0 to 18446744073709551615) we want to
    // be able to handle any input so we can report bad input strings back.
    if (inputText.isEmpty()) {
        // Empty string input - to reset to default the same as the no
        // argument case:
        pMudlet->mDiscord.setApplicationID(&host, QString());
        // This must always succeed
        lua_pushboolean(L, true);
        return 1;
    }
    bool isOk = false;
    quint64 const numericEquivalent = inputText.toULongLong(&isOk);
    if (numericEquivalent && isOk) {
        const QString appID = QString::number(numericEquivalent);
        if (pMudlet->mDiscord.setApplicationID(&host, appID)) {
            lua_pushboolean(L, true);
            return 1;
        }
        return warnArgumentValue(L, __func__, qsl("'%1' does not appear to be a valid Discord application ID").arg(inputText));
    }
    return warnArgumentValue(L, __func__, qsl("'%1' can not be converted to the expected numeric Discord application ID").arg(inputText));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordDetail
int TLuaInterpreter::setDiscordDetail(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetDetail)) {
        return warnArgumentValue(L, __func__, "access to Discord detail is disabled in settings for privacy");
    }

    auto discordText = getVerifiedString(L, __func__, 1, "text");
    if (discordText.size() == 1) {
        return warnArgumentValue(L, __func__, "text of length 1 not allowed by Discord");
    }

    pMudlet->mDiscord.setDetailText(&host, discordText);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordElapsedStartTime
int TLuaInterpreter::setDiscordElapsedStartTime(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetTimeInfo)) {
        return warnArgumentValue(L, __func__, "access to Discord time is disabled in settings for privacy");
    }

    int64_t const timeStamp = getVerifiedInt(L, __func__, 1, "epoch time");
    if (timeStamp < 0) {
        return warnArgumentValue(L, __func__, "the timestamp must be zero to clear the 'elapsed:' time or an epoch time value from the recent past");
    }
    pMudlet->mDiscord.setStartTimeStamp(&host, timeStamp);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordGame
int TLuaInterpreter::setDiscordGame(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetDetail)) {
        return warnArgumentValue(L, __func__, "access to Discord detail is disabled in settings for privacy");
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetLargeIcon)) {
        return warnArgumentValue(L, __func__, "access to Discord large icon is disabled in settings for privacy");
    }

    const QString gamename = getVerifiedString(L, __func__, 1, "game name");
    pMudlet->mDiscord.setDetailText(&host, tr("Playing %1").arg(gamename));
    pMudlet->mDiscord.setLargeImage(&host, gamename.toLower());
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordGameUrl
int TLuaInterpreter::setDiscordGameUrl(lua_State* L)
{
    // The invite URL changes what the Discord button opens, and the name is
    // what it displays on the button. It is not part of rich presence, so it
    // does not have the API enabled check that those Discord functions need
    // in order to respect privacy.
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);
    const bool isActiveHost = (pMudlet->mpCurrentActiveHost == &host);
    const int args = lua_gettop(L);

    if (!args) { // no args, blank the invite URL and game name
        host.setDiscordInviteURL(QString());
        host.setDiscordGameName(QString());
        if (isActiveHost) {
            pMudlet->updateDiscordNamedIcon();
        }
        lua_pushboolean(L, true);
        return 1;
    }
    QString inputText = getVerifiedString(L, __func__, 1, "url").trimmed();
    host.setDiscordInviteURL(inputText.isEmpty() ? QString() : inputText);
    if (args > 1) {
        inputText = getVerifiedString(L, __func__, 2, "game name").trimmed();
        host.setDiscordGameName(inputText);
    } else {
        host.setDiscordGameName(QString());
    }
    if (isActiveHost) {
        pMudlet->updateDiscordNamedIcon();
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordLargeIcon
int TLuaInterpreter::setDiscordLargeIcon(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L, true);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetLargeIcon)) {
        return warnArgumentValue(L, __func__, "access to Discord large icon is disabled in settings for privacy");
    }

    pMudlet->mDiscord.setLargeImage(&host, getVerifiedString(L, __func__, 1, "key").toLower());
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordLargeIconText
int TLuaInterpreter::setDiscordLargeIconText(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetLargeIconText)) {
        return warnArgumentValue(L, __func__, "access to Discord large icon text is disabled in settings for privacy");
    }

    auto discordText = getVerifiedString(L, __func__, 1, "text");
    if (discordText.size() == 1) {
        return warnArgumentValue(L, __func__, "text of length 1 not allowed by Discord");
    }

    pMudlet->mDiscord.setLargeImageText(&host, discordText);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordParty
int TLuaInterpreter::setDiscordParty(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetPartyInfo)) {
        return warnArgumentValue(L, __func__, "access to Discord party info is disabled in settings for privacy");
    }

    int64_t const partySize = getVerifiedInt(L, __func__, 1, "current party size");
    if (partySize < 0) {
        return warnArgumentValue(L, __func__, "the current party size must be zero or more");
    }

    int64_t partyMax = -1;
    if (lua_gettop(L) > 1) {
        partyMax = getVerifiedInt(L, __func__, 2, "party maximum size", true);
        if (partyMax < 0) {
            return warnArgumentValue(L, __func__, "the optional party maximum size must be zero (to remove the party details) or more (to set the maximum)");
        }

        pMudlet->mDiscord.setParty(&host, static_cast<int>(qMin(static_cast<int64_t>(INT_MAX), partySize)), static_cast<int>(qMin(static_cast<int64_t>(INT_MAX), partyMax)));
    } else {
        // Only got the partySize now
        pMudlet->mDiscord.setParty(&host, static_cast<int>(qMin(static_cast<int64_t>(INT_MAX), partySize)));
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordRemainingEndTime
int TLuaInterpreter::setDiscordRemainingEndTime(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetTimeInfo)) {
        return warnArgumentValue(L, __func__, "access to Discord time is disabled in settings for privacy");
    }

    int64_t const timeStamp = getVerifiedInt(L, __func__, 1, "epoch time");

    if (timeStamp < 0) {
        return warnArgumentValue(L, __func__, "the timestamp must be zero to clear the 'remaining:' time or an epoch time value in the recent future");
    }
    pMudlet->mDiscord.setEndTimeStamp(&host, timeStamp);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordSmallIcon
int TLuaInterpreter::setDiscordSmallIcon(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetSmallIcon)) {
        return warnArgumentValue(L, __func__, "access to Discord small icon is disabled in settings for privacy");
    }

    pMudlet->mDiscord.setSmallImage(&host, getVerifiedString(L, __func__, 1, "key").toLower());
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordSmallIconText
int TLuaInterpreter::setDiscordSmallIconText(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetSmallIconText)) {
        return warnArgumentValue(L, __func__, "access to Discord small icon text is disabled in settings for privacy");
    }

    auto discordText = getVerifiedString(L, __func__, 1, "text");
    if (discordText.size() == 1) {
        return warnArgumentValue(L, __func__, "text of length 1 not allowed by Discord");
    }

    pMudlet->mDiscord.setSmallImageText(&host, discordText);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordState
int TLuaInterpreter::setDiscordState(lua_State* L)
{
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetState)) {
        return warnArgumentValue(L, __func__, "access to Discord state is disabled in settings for privacy");
    }

    auto discordText = getVerifiedString(L, __func__, 1, "text");
    if (discordText.size() == 1) {
        return warnArgumentValue(L, __func__, "text of length 1 not allowed by Discord");
    }

    mudlet::self()->mDiscord.setStateText(&host, discordText);
    lua_pushboolean(L, true);
    return 1;
}

