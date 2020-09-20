/***************************************************************************
*   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
*   Copyright (C) 2013-2020 by Stephen Lyons - slysven@virginmedia.com    *
*   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
*   Copyright (C) 2016 by Eric Wallace - eewallace@gmail.com              *
*   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
*   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
*   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
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


#include "Host.h"
#include "TAlias.h"
#include "TArea.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TEvent.h"
#include "TForkedProcess.h"
#include "TRoomDB.h"
#include "TTextEdit.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "dlgComposer.h"
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"
#if defined(INCLUDE_3DMAPPER)
#include "glwidget.h"
#endif

#include "pre_guard.h"
#include <QtConcurrent>
#include <QCollator>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QTableWidget>
#include <QFileInfo>
#include <QVector>
#ifdef QT_TEXTTOSPEECH_LIB
#include <QTextToSpeech>
#endif // QT_TEXTTOSPEECH_LIB
#include "post_guard.h"

#include <limits>

const QMap<Qt::MouseButton, QString> TLuaInterpreter::mMouseButtons = {
        {Qt::NoButton, QStringLiteral("NoButton")},           {Qt::LeftButton, QStringLiteral("LeftButton")},       {Qt::RightButton, QStringLiteral("RightButton")},
        {Qt::MidButton, QStringLiteral("MidButton")},         {Qt::BackButton, QStringLiteral("BackButton")},       {Qt::ForwardButton, QStringLiteral("ForwardButton")},
        {Qt::TaskButton, QStringLiteral("TaskButton")},       {Qt::ExtraButton4, QStringLiteral("ExtraButton4")},   {Qt::ExtraButton5, QStringLiteral("ExtraButton5")},
        {Qt::ExtraButton6, QStringLiteral("ExtraButton6")},   {Qt::ExtraButton7, QStringLiteral("ExtraButton7")},   {Qt::ExtraButton8, QStringLiteral("ExtraButton8")},
        {Qt::ExtraButton9, QStringLiteral("ExtraButton9")},   {Qt::ExtraButton10, QStringLiteral("ExtraButton10")}, {Qt::ExtraButton11, QStringLiteral("ExtraButton11")},
        {Qt::ExtraButton12, QStringLiteral("ExtraButton12")}, {Qt::ExtraButton13, QStringLiteral("ExtraButton13")}, {Qt::ExtraButton14, QStringLiteral("ExtraButton14")},
        {Qt::ExtraButton15, QStringLiteral("ExtraButton15")}, {Qt::ExtraButton16, QStringLiteral("ExtraButton16")}, {Qt::ExtraButton17, QStringLiteral("ExtraButton17")},
        {Qt::ExtraButton18, QStringLiteral("ExtraButton18")}, {Qt::ExtraButton19, QStringLiteral("ExtraButton19")}, {Qt::ExtraButton20, QStringLiteral("ExtraButton20")},
        {Qt::ExtraButton21, QStringLiteral("ExtraButton21")}, {Qt::ExtraButton22, QStringLiteral("ExtraButton22")}, {Qt::ExtraButton23, QStringLiteral("ExtraButton23")},
        {Qt::ExtraButton24, QStringLiteral("ExtraButton24")},

};


extern "C" {
int luaopen_yajl(lua_State*);
}

#ifdef QT_TEXTTOSPEECH_LIB
QPointer<QTextToSpeech> speechUnit;
QVector<QString> speechQueue;
bool bSpeechBuilt;
bool bSpeechQueueing;
int speechState = QTextToSpeech::Ready;
QString speechCurrent;
#endif // QT_TEXTTOSPEECH_LIB

TLuaInterpreter::TLuaInterpreter(Host* pH, const QString& hostName, int id) : mpHost(pH), hostName(hostName), mHostID(id), purgeTimer(this)
{
    pGlobalLua = nullptr;

    connect(&purgeTimer, &QTimer::timeout, this, &TLuaInterpreter::slotPurge);

    mpFileDownloader = new QNetworkAccessManager(this);
    connect(mpFileDownloader, &QNetworkAccessManager::finished, this, &TLuaInterpreter::slot_httpRequestFinished);

    initLuaGlobals();

    purgeTimer.start(2000);
}

TLuaInterpreter::~TLuaInterpreter()
{
    lua_close(pGlobalLua);
}

// No documentation available in wiki - internal function
// Raises additional sysDownloadError Events on failure to process
// the local file, the second argument is "failureToWriteLocalFile" and besides
// the file to be written being the third argument (as multiple downloads are
// supported) a fourth argument gives the local file problem, one of:
// * "unableToOpenLocalFileForWriting"
// * "unableToWriteLocalFile"
// or a QFile::errorString() for the issue at hand
// Upon success we now give an additional (third value) which gives the number
// of bytes written into the downloaded file.
void TLuaInterpreter::slot_httpRequestFinished(QNetworkReply* reply)
{
    Host* pHost = mpHost;
    if (!pHost) {
        qWarning() << QStringLiteral("TLuaInterpreter::slot_httpRequestFinished(...) ERROR: NULL Host pointer!");
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        TEvent event {};
        QString localFileName;

        switch (reply->operation()) {
        case QNetworkAccessManager::PostOperation:
            event.mArgumentList << QStringLiteral("sysPostHttpError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->errorString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->url().toString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            break;

        case QNetworkAccessManager::PutOperation:
            event.mArgumentList << QStringLiteral("sysPutHttpError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->errorString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->url().toString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            break;

        case QNetworkAccessManager::GetOperation:
            localFileName = downloadMap.value(reply);
            event.mArgumentList << (localFileName.isEmpty()
                                   ? QStringLiteral("sysGetHttpError")
                                   : QStringLiteral("sysDownloadError"));
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->errorString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            if (!localFileName.isEmpty()) {
                event.mArgumentList << localFileName;
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            }
            event.mArgumentList << reply->url().toString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

            downloadMap.remove(reply);
            break;

        case QNetworkAccessManager::DeleteOperation:
            event.mArgumentList << QStringLiteral("sysDeleteHttpError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->errorString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->url().toString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            break;

        case QNetworkAccessManager::HeadOperation:
            break;
        case QNetworkAccessManager::CustomOperation:
            break;
        case QNetworkAccessManager::UnknownOperation:
            break;
        }

        event.mArgumentList << QString::number(createHttpResponseTable(reply));
        event.mArgumentTypeList << ARGUMENT_TYPE_TABLE;

        reply->deleteLater();
        downloadMap.remove(reply);
        pHost->raiseEvent(event);
        return;
    }

    handleHttpOK(reply);
}

void TLuaInterpreter::handleHttpOK(QNetworkReply* reply)
{
    TEvent event {};
    Host* pHost = mpHost;
    if (!pHost) {
        return;
    }

    switch (reply->operation()) {
    case QNetworkAccessManager::HeadOperation:
        break;
    case QNetworkAccessManager::DeleteOperation:
        event.mArgumentList << QStringLiteral("sysDeleteHttpDone");
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << reply->url().toString();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        if (auto replyText = QString(reply->readAll()); replyText.size() <= 10000) {

            // our linewrapping algorithm doesn't like 150k long lines, so don't show the response if it's too big
            event.mArgumentList << replyText;
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        }
        break;

    case QNetworkAccessManager::CustomOperation:
        break;
    case QNetworkAccessManager::UnknownOperation:
        break;

    case QNetworkAccessManager::PostOperation:
        event.mArgumentList << QStringLiteral("sysPostHttpDone");
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << reply->url().toString();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        if (auto replyText = QString(reply->readAll()); replyText.size() <= 10000) {
            // our linewrapping algorithm doesn't like 150k long lines, so don't show the response if it's too big
            event.mArgumentList << replyText;
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        }
        break;

    case QNetworkAccessManager::PutOperation:
        event.mArgumentList << QStringLiteral("sysPutHttpDone");
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << reply->url().toString();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        if (auto replyText = QString(reply->readAll()); replyText.size() <= 10000) {

            // our linewrapping algorithm doesn't like 150k long lines, so don't show the response if it's too big
            event.mArgumentList << replyText;
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        }
        break;

    case QNetworkAccessManager::GetOperation:
        QString localFileName = downloadMap.value(reply);
        downloadMap.remove(reply);

        // If the user did not give us a file path, we're not going to
        // consider this an error, we're just going to attach the reply
        // directly. Another way this could happen is the user made a POST
        // request, and it redirected to a GET. In the case of POST requests,
        // we don't ask the user for a file path.
        if (localFileName.isEmpty()) {
            event.mArgumentList << QLatin1String("sysGetHttpDone");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->url().toString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            if (auto replyText = QString(reply->readAll()); replyText.size() <= 10000) {

                // our linewrapping algorithm doesn't like 150k long lines, so don't show the response if it's too big
                event.mArgumentList << replyText;
                event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            }
            break;
        }

        QFile localFile(localFileName);
        if (!localFile.open(QFile::WriteOnly)) {
            event.mArgumentList << QLatin1String("sysDownloadError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("failureToWriteLocalFile");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << localFileName;
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("unableToOpenLocalFileForWriting");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            break;
        }

        qint64 bytesWritten = localFile.write(reply->readAll());
        if (bytesWritten == -1) {
            event.mArgumentList << QLatin1String("sysDownloadError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("failureToWriteLocalFile");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << localFileName;
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("unableToWriteLocalFile");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            break;
        }

        localFile.flush();

        if (localFile.error() == QFile::NoError) {
            event.mArgumentList << QLatin1String("sysDownloadDone");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << localFileName;
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QString::number(bytesWritten);
            event.mArgumentTypeList << ARGUMENT_TYPE_NUMBER;
        } else {
            event.mArgumentList << QLatin1String("sysDownloadError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("failureToWriteLocalFile");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << localFileName;
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << localFile.errorString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        }

        localFile.close();
        break;

    }

    event.mArgumentList << QString::number(createHttpResponseTable(reply));
    event.mArgumentTypeList << ARGUMENT_TYPE_TABLE;


    reply->deleteLater();
    pHost->raiseEvent(event);
}

// No documentation available in wiki - internal function
void TLuaInterpreter::slotDeleteSender(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);

    objectsToDelete.append(sender());
}

// No documentation available in wiki - internal function
void TLuaInterpreter::slotPurge()
{
    while (!objectsToDelete.isEmpty()) {
        delete objectsToDelete.takeFirst();
    }
}

// No documentation available in wiki - internal function
int TLuaInterpreter::Wait(lua_State* L)
{
    int n = lua_gettop(L);
    if (n != 1) {
        lua_pushstring(L, "Wait: wrong number of arguments");
        lua_error(L);
        return 1;
    }

    int luaSleepMsec;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "Wait: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSleepMsec = lua_tointeger(L, 1);
    }
    msleep(luaSleepMsec); // FIXME thread::sleep()
    return 0;
}

// No documentation available in wiki - internal function
// dirToString wiil now catch and validate pretty much any string that could
// be a normal direction in a case insensitive manner and convert it to a
// standard value (one of: "n", "ne", ..., "nw", "up", "down", "in" or
// "out") but leave anything else as entered; OR convert a direction code as
// a number from 1 to 12 to those same standard direction strings.
// This is intended as a temporary step until a uniform means of specifying
// both "normal" and "special" exits for all lua commands that take exit
// directions as arguments in an unambiguous manner can be formulated - though
// to maintain backwards compatibility the current functions will remain even
// if they get marked out as being deprecated.
QString TLuaInterpreter::dirToString(lua_State* L, int position)
{
    if (lua_isnumber(L, position)) {
        qint64 dirNum = lua_tonumber(L, position);
        switch (dirNum) {
        // breaks not needed - all handled cases end in a return!
        case 1:
            return QStringLiteral("n");
        case 2:
            return QStringLiteral("ne");
        case 3:
            return QStringLiteral("nw");
        case 4:
            return QStringLiteral("e");
        case 5:
            return QStringLiteral("w");
        case 6:
            return QStringLiteral("s");
        case 7:
            return QStringLiteral("se");
        case 8:
            return QStringLiteral("sw");
        case 9:
            return QStringLiteral("up");
        case 10:
            return QStringLiteral("down");
        case 11:
            return QStringLiteral("in");
        case 12:
            return QStringLiteral("out");
        default:
            return QString();
        }

    } else if (lua_isstring(L, position)) {
        QString direction(QString::fromUtf8(lua_tostring(L, position)));
        if (!direction.compare(QLatin1String("n"), Qt::CaseInsensitive) || !direction.compare(QLatin1String("north"), Qt::CaseInsensitive)) {
            return QLatin1String("n");
        } else if (!direction.compare(QLatin1String("e"), Qt::CaseInsensitive) || !direction.compare(QLatin1String("east"), Qt::CaseInsensitive)) {
            return QLatin1String("e");
        } else if (!direction.compare(QLatin1String("s"), Qt::CaseInsensitive) || !direction.compare(QLatin1String("south"), Qt::CaseInsensitive)) {
            return QLatin1String("s");
        } else if (!direction.compare(QLatin1String("w"), Qt::CaseInsensitive) || !direction.compare(QLatin1String("west"), Qt::CaseInsensitive)) {
            return QLatin1String("w");
        } else if (!direction.compare(QLatin1String("u"), Qt::CaseInsensitive) || !direction.compare(QLatin1String("up"), Qt::CaseInsensitive)) {
            return QLatin1String("up");
        } else if (!direction.compare(QLatin1String("d"), Qt::CaseInsensitive) || !direction.compare(QLatin1String("down"), Qt::CaseInsensitive)) {
            return QLatin1String("down");
        } else if (!direction.compare(QLatin1String("ne"), Qt::CaseInsensitive) || !direction.compare(QLatin1String("northeast"), Qt::CaseInsensitive)
                   || !direction.compare(QLatin1String("north-east"), Qt::CaseInsensitive)) {
            return QLatin1String("ne");
        } else if (!direction.compare(QLatin1String("se"), Qt::CaseInsensitive) || !direction.compare(QLatin1String("southeast"), Qt::CaseInsensitive)
                   || !direction.compare(QLatin1String("south-east"), Qt::CaseInsensitive)) {
            return QLatin1String("se");
        } else if (!direction.compare(QLatin1String("sw"), Qt::CaseInsensitive) || !direction.compare(QLatin1String("southwest"), Qt::CaseInsensitive)
                   || !direction.compare(QLatin1String("south-west"), Qt::CaseInsensitive)) {
            return QLatin1String("sw");
        } else if (!direction.compare(QLatin1String("nw"), Qt::CaseInsensitive) || !direction.compare(QLatin1String("northwest"), Qt::CaseInsensitive)
                   || !direction.compare(QLatin1String("north-west"), Qt::CaseInsensitive)) {
            return QLatin1String("nw");
        } else if (!direction.compare(QLatin1String("i"), Qt::CaseInsensitive) || !direction.compare(QLatin1String("in"), Qt::CaseInsensitive)) {
            return QLatin1String("in");
        } else if (!direction.compare(QLatin1String("o"), Qt::CaseInsensitive) || !direction.compare(QLatin1String("out"), Qt::CaseInsensitive)) {
            return QLatin1String("out");
        } else {
            return direction;
        }

    } else {
        return QString();
    }
}

// No documentation available in wiki - internal function
int TLuaInterpreter::dirToNumber(lua_State* L, int position)
{
    QString dir;
    int dirNum;
    if (lua_isstring(L, position)) {
        dir = lua_tostring(L, position);
        dir = dir.toLower();
        if (dir == QStringLiteral("n") || dir == QStringLiteral("north")) {
            return 1;
        }
        if (dir == QStringLiteral("ne") || dir == QStringLiteral("northeast")) {
            return 2;
        }
        if (dir == QStringLiteral("nw") || dir == QStringLiteral("northwest")) {
            return 3;
        }
        if (dir == QStringLiteral("e") || dir == QStringLiteral("east")) {
            return 4;
        }
        if (dir == QStringLiteral("w") || dir == QStringLiteral("west")) {
            return 5;
        }
        if (dir == QStringLiteral("s") || dir == QStringLiteral("south")) {
            return 6;
        }
        if (dir == QStringLiteral("se") || dir == QStringLiteral("southeast")) {
            return 7;
        }
        if (dir == QStringLiteral("sw") || dir == QStringLiteral("southwest")) {
            return 8;
        }
        if (dir == QStringLiteral("u") || dir == QStringLiteral("up")) {
            return 9;
        }
        if (dir == QStringLiteral("d") || dir == QStringLiteral("down")) {
            return 10;
        }
        if (dir == QStringLiteral("in")) {
            return 11;
        }
        if (dir == QStringLiteral("out")) {
            return 12;
        }
    }
    if (lua_isnumber(L, position)) {
        dirNum = lua_tonumber(L, position);
        return (dirNum >= 1 && dirNum <= 12 ? dirNum : 0);
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#denyCurrentSend
int TLuaInterpreter::denyCurrentSend(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.mAllowToSendCommand = false;
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#raiseEvent
int TLuaInterpreter::raiseEvent(lua_State* L)
{
    Host& host = getHostFromLua(L);

    TEvent event {};

    int n = lua_gettop(L);
    // We go from the top of the stack down, because luaL_ref will
    // only reference the object at the top of the stack
    for (int i = n; i >= 1; i--) {
        if (lua_isnumber(L, -1)) {
            event.mArgumentList.prepend(QString::number(lua_tonumber(L, -1)));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_NUMBER);
            lua_pop(L, 1);
        } else if (lua_isstring(L, -1)) {
            event.mArgumentList.prepend(QString::fromUtf8(lua_tostring(L, -1)));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_STRING);
            lua_pop(L, 1);
        } else if (lua_isboolean(L, -1)) {
            event.mArgumentList.prepend(QString::number(lua_toboolean(L, -1)));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_BOOLEAN);
            lua_pop(L, 1);
        } else if (lua_isnil(L, -1)) {
            event.mArgumentList.prepend(QString());
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_NIL);
            lua_pop(L, 1);
        } else if (lua_istable(L, -1)) {
            event.mArgumentList.prepend(QString::number(luaL_ref(L, LUA_REGISTRYINDEX)));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_TABLE);
            // luaL_ref pops the object, so we don't have to
        } else if (lua_isfunction(L, -1)) {
            event.mArgumentList.prepend(QString::number(luaL_ref(L, LUA_REGISTRYINDEX)));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_FUNCTION);
            // luaL_ref pops the object, so we don't have to
        } else {
            lua_pushfstring(L,
                            "raiseEvent: bad argument #%d type (string, number, boolean, table,\n"
                            "function, or nil expected, got a %s!)",
                            i,
                            luaL_typename(L, -1));
            lua_error(L);
            return 1;
        }
    }

    host.raiseEvent(event);

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getProfileName
int TLuaInterpreter::getProfileName(lua_State* L)
{
    Host& host = getHostFromLua(L);
    lua_pushstring(L, host.getName().toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCommandSeparator
int TLuaInterpreter::getCommandSeparator(lua_State* L)
{
    Host& host = getHostFromLua(L);
    lua_pushstring(L, host.getCommandSeparator().toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#raiseGlobalEvent
int TLuaInterpreter::raiseGlobalEvent(lua_State* L)
{
    Host& host = getHostFromLua(L);

    int n = lua_gettop(L);
    if (!n) {
        lua_pushstring(L, "raiseGlobalEvent: missing argument #1 (eventName as, probably, a string expected!)");
        lua_error(L);
        return 1;
    }

    TEvent event {};

    for (int i = 1; i <= n; ++i) {
        // The sending profile of the event does not receive the event if
        // sent via this command but if the same eventName is to be used for
        // an event within a profile and to other profiles it is safest to
        // insert a string like "local" or "self" or the profile name from
        // getProfileName() as an (last) additional argument after all the
        // other so the handler can tell it is handling a local event from
        // raiseEvent(...) and not one from another profile! - Slysven
        if (lua_isnumber(L, i)) {
            event.mArgumentList.append(QString::number(lua_tonumber(L, i)));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        } else if (lua_isstring(L, i)) {
            event.mArgumentList.append(QString::fromUtf8(lua_tostring(L, i)));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        } else if (lua_isboolean(L, i)) {
            event.mArgumentList.append(QString::number(lua_toboolean(L, i)));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_BOOLEAN);
        } else if (lua_isnil(L, i)) {
            event.mArgumentList.append(QString());
            event.mArgumentTypeList.append(ARGUMENT_TYPE_NIL);
        } else {
            lua_pushfstring(L,
                            "raiseGlobalEvent: bad argument type #%d (boolean, number, string or nil\n"
                            "expected, got a %s!)",
                            i,
                            luaL_typename(L, i));
            lua_error(L);
            return 1;
        }
    }

    event.mArgumentList.append(host.getName());
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

    mudlet::self()->getHostManager().postInterHostEvent(&host, event);

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetProfile
int TLuaInterpreter::resetProfile(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.resetProfile_phase1();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectString
int TLuaInterpreter::selectString(lua_State* L)
{
    Host& host = getHostFromLua(L);

    int s = 1;
    QString windowName; // only for 3 argument case, will be null if not assigned to which is different from being empty
    if (lua_gettop(L) > 2) {
        if (!lua_isstring(L, s)) {
            lua_pushfstring(L, R"(selectString: bad argument #%d type (window name as string, is optional {defaults to "main" if omitted}, got %s!))", s, luaL_typename(L, s));
            lua_error(L);
            return 1;
        } else {
            // We cannot yet properly handle non-ASCII windows names but we will eventually!
            windowName = QString::fromUtf8(lua_tostring(L, s));
            if (windowName == QLatin1String("main")) {
                // This matches the identifier for the main window - so make it
                // appear so by emptying it...
                windowName.clear();
            }
            s++;
        }
    }

    QString searchText;
    if (!lua_isstring(L, s)) {
        lua_pushfstring(L, "selectString: bad argument #%d type (text to select as string expected, got %s!)", s, luaL_typename(L, s));
        lua_error(L);
        return 1;
    } else {
        searchText = QString::fromUtf8(lua_tostring(L, s));
        // CHECK: Do we need to qualify this for a non-blank string?
        s++;
    }

    qint64 numOfMatch = 0;
    if (!lua_isnumber(L, s)) {
        lua_pushfstring(L, "selectString: bad argument #%d type (match count as number {1 for first} expected, got %s!)", s, luaL_typename(L, s));
        lua_error(L);
        return 1;
    } else {
        numOfMatch = lua_tointeger(L, s);
    }

    if (windowName.isEmpty()) {
        lua_pushnumber(L, host.mpConsole->select(searchText, numOfMatch));
    } else {
        lua_pushnumber(L, mudlet::self()->selectString(&host, windowName, searchText, numOfMatch));
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectCurrentLine
int TLuaInterpreter::selectCurrentLine(lua_State* L)
{
    std::string luaSendText = "";
    if (lua_gettop(L) == 0) {
        luaSendText = "main";
    } else {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "selectCurrentLine: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            luaSendText = lua_tostring(L, 1);
        }
    }
    Host& host = getHostFromLua(L);
    host.mpConsole->selectCurrentLine(luaSendText);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isAnsiFgColor
int TLuaInterpreter::isAnsiFgColor(lua_State* L)
{
    int ansiFg;

    std::string windowName = "main";

    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "isAnsiFgColor: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        ansiFg = lua_tointeger(L, 1);
    }

    std::list<int> result;
    Host& host = getHostFromLua(L);
    result = host.mpConsole->getFgColor(windowName);
    auto it = result.begin();
    if (result.size() < 3) {
        return 0;
    }
    if (ansiFg < 0) {
        return 0;
    }
    if (ansiFg > 16) {
        return 0;
    }


    QColor c;
    switch (ansiFg) {
    case 0:
        c = host.mFgColor;
        break;
    case 1:
        c = host.mLightBlack;
        break;
    case 2:
        c = host.mBlack;
        break;
    case 3:
        c = host.mLightRed;
        break;
    case 4:
        c = host.mRed;
        break;
    case 5:
        c = host.mLightGreen;
        break;
    case 6:
        c = host.mGreen;
        break;
    case 7:
        c = host.mLightYellow;
        break;
    case 8:
        c = host.mYellow;
        break;
    case 9:
        c = host.mLightBlue;
        break;
    case 10:
        c = host.mBlue;
        break;
    case 11:
        c = host.mLightMagenta;
        break;
    case 12:
        c = host.mMagenta;
        break;
    case 13:
        c = host.mLightCyan;
        break;
    case 14:
        c = host.mCyan;
        break;
    case 15:
        c = host.mLightWhite;
        break;
    case 16:
        c = host.mWhite;
        break;
    }

    int val = *it;
    if (val == c.red()) {
        it++;
        val = *it;
        if (val == c.green()) {
            it++;
            val = *it;
            if (val == c.blue()) {
                lua_pushboolean(L, true);
                return 1;
            }
        }
    }

    lua_pushboolean(L, false);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isAnsiBgColor
int TLuaInterpreter::isAnsiBgColor(lua_State* L)
{
    int ansiBg;

    std::string windowName = "main";

    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "isAnsiBgColor: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        ansiBg = lua_tointeger(L, 1);
    }

    std::list<int> result;
    Host& host = getHostFromLua(L);
    result = host.mpConsole->getBgColor(windowName);
    auto it = result.begin();
    if (result.size() < 3) {
        return 0;
    }
    if (ansiBg < 0) {
        return 0;
    }
    if (ansiBg > 16) {
        return 0;
    }


    QColor c;
    switch (ansiBg) {
    case 0:
        c = host.mBgColor;
        break;
    case 1:
        c = host.mLightBlack;
        break;
    case 2:
        c = host.mBlack;
        break;
    case 3:
        c = host.mLightRed;
        break;
    case 4:
        c = host.mRed;
        break;
    case 5:
        c = host.mLightGreen;
        break;
    case 6:
        c = host.mGreen;
        break;
    case 7:
        c = host.mLightYellow;
        break;
    case 8:
        c = host.mYellow;
        break;
    case 9:
        c = host.mLightBlue;
        break;
    case 10:
        c = host.mBlue;
        break;
    case 11:
        c = host.mLightMagenta;
        break;
    case 12:
        c = host.mMagenta;
        break;
    case 13:
        c = host.mLightCyan;
        break;
    case 14:
        c = host.mCyan;
        break;
    case 15:
        c = host.mLightWhite;
        break;
    case 16:
        c = host.mWhite;
        break;
    }

    int val = *it;
    if (val == c.red()) {
        it++;
        val = *it;
        if (val == c.green()) {
            it++;
            val = *it;
            if (val == c.blue()) {
                lua_pushboolean(L, true);
                return 1;
            }
        }
    }

    lua_pushboolean(L, false);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getFgColor
int TLuaInterpreter::getFgColor(lua_State* L)
{
    std::string windowName = "main";
    if (lua_gettop(L) > 0) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "getFgColor: bad argument #1 type (window name as string is optional, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            windowName = lua_tostring(L, 1);
        }
    }

    Host& host = getHostFromLua(L);
    std::list<int> result = host.mpConsole->getFgColor(windowName);
    for (int pos : result) {
        lua_pushnumber(L, pos);
    }
    return result.size();
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBgColor
int TLuaInterpreter::getBgColor(lua_State* L)
{
    std::string windowName = "main";
    if (lua_gettop(L) > 0) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "getBgColor: bad argument #1 type (window name as string is optional, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            windowName = lua_tostring(L, 1);
        }
    }

    Host& host = getHostFromLua(L);
    std::list<int> result = host.mpConsole->getBgColor(windowName);
    for (int pos : result) {
        lua_pushnumber(L, pos);
    }
    return result.size();
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getTextFormat
int TLuaInterpreter::getTextFormat(lua_State* L)
{
    QString windowName = QStringLiteral("main");
    if (lua_gettop(L)) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "getTextFormat: bad argument #1 type (window name as string is optional, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        }
        windowName = lua_tostring(L, 1);
    }

    Host& host = getHostFromLua(L);
    QPair<quint8, TChar> result = host.mpConsole->getTextAttributes(windowName);
    if (result.first == 1) {
        lua_pushnil(L);
        lua_pushfstring(L, "window \"%s\" not found", windowName.toUtf8().constData());
        return 2;
    }

    if (result.first == 2) {
        lua_pushnil(L);
        lua_pushfstring(L, "current selection invalid in window \"%s\"", windowName.toUtf8().constData());
        return 2;
    }

    lua_newtable(L);

    TChar::AttributeFlags format = result.second.allDisplayAttributes();
    lua_pushstring(L, "bold");
    lua_pushboolean(L, format & TChar::Bold);
    lua_settable(L, -3);

    lua_pushstring(L, "italic");
    lua_pushboolean(L, format & TChar::Italic);
    lua_settable(L, -3);

    lua_pushstring(L, "overline");
    lua_pushboolean(L, format & TChar::Overline);
    lua_settable(L, -3);

    lua_pushstring(L, "reverse");
    lua_pushboolean(L, format & TChar::Reverse);
    lua_settable(L, -3);

    lua_pushstring(L, "strikeout");
    lua_pushboolean(L, format & TChar::StrikeOut);
    lua_settable(L, -3);

    lua_pushstring(L, "underline");
    lua_pushboolean(L, format & TChar::Underline);
    lua_settable(L, -3);

    QColor foreground(result.second.foreground());
    lua_pushstring(L, "foreground");
    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, foreground.red());
    lua_settable(L, -3);

    lua_pushnumber(L, 2);
    lua_pushnumber(L, foreground.green());
    lua_settable(L, -3);

    lua_pushnumber(L, 3);
    lua_pushnumber(L, foreground.blue());
    lua_settable(L, -3);
    lua_settable(L, -3);

    QColor background(result.second.background());
    lua_pushstring(L, "background");
    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, background.red());
    lua_settable(L, -3);

    lua_pushnumber(L, 2);
    lua_pushnumber(L, background.green());
    lua_settable(L, -3);

    lua_pushnumber(L, 3);
    lua_pushnumber(L, background.blue());
    lua_settable(L, -3);
    lua_settable(L, -3);

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getWindowsCodepage
int TLuaInterpreter::getWindowsCodepage(lua_State* L)
{
#if defined (Q_OS_WIN32)
    QSettings registry(QStringLiteral(R"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Nls\CodePage)"),
                       QSettings::NativeFormat);
    auto value = registry.value(QStringLiteral("ACP"));
    lua_pushstring(L, value.toString().toUtf8().constData());
    return 1;
#else
    lua_pushnil(L);
    lua_pushstring(L, "this function is only needed on Windows, and does not work here");
    return 2;
#endif
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#wrapLine
int TLuaInterpreter::wrapLine(lua_State* L)
{
    int s = 1;
    std::string windowName = "main";
    if (lua_gettop(L)) {
        if (!lua_isstring(L, s)) {
            lua_pushfstring(L, "wrapLine: bad argument #%d type (window name as string expected, got %s!)", s, luaL_typename(L, 1));
            return lua_error(L);
        } else {
            windowName = lua_tostring(L, s);
            s++;
        }
    }

    int lineNumber;
    if (!lua_isnumber(L, s)) {
        lua_pushfstring(L, "wrapLine: bad argument #%d type (line as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        lineNumber = lua_tointeger(L, s);
    }

    Host& host = getHostFromLua(L);
    host.mpConsole->luaWrapLine(windowName, lineNumber);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#spawn
int TLuaInterpreter::spawn(lua_State* L)
{
    Host& host = getHostFromLua(L);
    return TForkedProcess::startProcess(host.getLuaInterpreter(), L);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectCaptureGroup
int TLuaInterpreter::selectCaptureGroup(lua_State* L)
{
    int captureGroup;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "selectCaptureGroup: bad argument #1 type (capture group as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        captureGroup = lua_tointeger(L, 1);
    }
    Host& host = getHostFromLua(L);
    if (captureGroup < 1) {
        lua_pushnumber(L, -1);
        return 1;
    }
    // We want capture groups to start with 1 instead of 0 so predecrement
    // luaNumOfMatch :
    if (--captureGroup < static_cast<int>(host.getLuaInterpreter()->mCaptureGroupList.size())) {
        TLuaInterpreter* pL = host.getLuaInterpreter();
        auto iti = pL->mCaptureGroupPosList.begin();
        auto its = pL->mCaptureGroupList.begin();
        int begin = *iti;
        std::string& s = *its;

        for (int i = 0; iti != pL->mCaptureGroupPosList.end(); ++iti, ++i) {
            begin = *iti;
            if (i >= captureGroup) {
                break;
            }
        }
        for (int i = 0; its != pL->mCaptureGroupList.end(); ++its, ++i) {
            s = *its;
            if (i >= captureGroup) {
                break;
            }
        }

        int length = QString::fromStdString(s).size();
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::red)) << "selectCaptureGroup(" << begin << ", " << length << ")\n" >> 0;
        }
        int pos = host.mpConsole->selectSection(begin, length);
        lua_pushnumber(L, pos);
    } else {
        lua_pushnumber(L, -1);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLines
int TLuaInterpreter::getLines(lua_State* L)
{
    int n = lua_gettop(L);
    int s = 0;
    QString windowName = QLatin1String("main");
    if (n > 2) {
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L, "getLines: bad argument #%d type (mini console, user window or buffer name as string expected {may be omitted for the \"main\" console}, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }

    int lineFrom;
    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "getLines: bad argument #%d type (start line as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        lineFrom = lua_tointeger(L, s);
    }

    int lineTo;
    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "getLines: bad argument #%d type (end line as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        lineTo = lua_tointeger(L, s);
    }
    Host& host = getHostFromLua(L);
    QPair<bool, QStringList> result = mudlet::self()->getLines(&host, windowName, lineFrom, lineTo);
    if (!result.first) {
        // Only one QString in .second - the error message
        lua_pushnil(L);
        lua_pushstring(L, result.second.at(0).toUtf8().constData());
        return 2;
    } else {
        lua_newtable(L);
        for (int i = 0, total = result.second.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushstring(L, result.second.at(i).toUtf8().constData());
            lua_settable(L, -3);
        }
        return 1;
    }
}

// Documentation: ? - public function missing documentation in wiki
// Should have been called loadReplay(...) but this name is already in the
// published Lua API
int TLuaInterpreter::loadRawFile(lua_State* L)
{
    QString replayFileName;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "loadRawFile: bad argument #1 type (replay file name, {may include a relative to \n"
                           "profile's \"logs\" sub-directory, or an absolute path}, as string expected, \n"
                           "got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    } else {
        replayFileName = QString::fromUtf8(lua_tostring(L, 1));
        if (replayFileName.isEmpty()) {
            lua_pushnil(L);
            lua_pushstring(L, "a blank string is not a valid replay file name");
            return 2;
        }
    }

    Host& host = getHostFromLua(L);
    QString errMsg;
    if (mudlet::self()->loadReplay(&host, replayFileName, &errMsg)) {
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushnil(L);
        // Although we only use English text for Lua messages the errMsg could
        // contain a Windows pathFileName which may use non-ASCII characters:
        lua_pushfstring(L, "unable to start replay, reason: '%s'", errMsg.toUtf8().constData());
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setProfileIcon
int TLuaInterpreter::setProfileIcon(lua_State* L)
{
    QString iconPath;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setProfileIcon: bad argument #1 type (icon file path expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    } else {
        iconPath = QString::fromUtf8(lua_tostring(L, 1));
        if (iconPath.isEmpty()) {
            lua_pushnil(L);
            lua_pushstring(L, "a blank string is not a valid icon file location");
            return 2;
        }
    }

    if (!QFileInfo::exists(iconPath)) {
        lua_pushnil(L);
        lua_pushfstring(L, "'%s' doesn't exist", iconPath.toUtf8().constData());
        return 2;
    }

    Host& host = getHostFromLua(L);

    auto[success, message] = mudlet::self()->setProfileIcon(host.getName(), iconPath);
    if (success) {
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetProfileIcon
int TLuaInterpreter::resetProfileIcon(lua_State* L)
{
    Host& host = getHostFromLua(L);

    auto [success, message] = mudlet::self()->resetProfileIcon(host.getName());
    if (success) {
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCurrentLine
int TLuaInterpreter::getCurrentLine(lua_State* L)
{
    std::string windowName = "";
    if (lua_gettop(L) == 0) {
        windowName = "main";
    } else {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "getCurrentLine: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            windowName = lua_tostring(L, 1);
        }
    }

    Host& host = getHostFromLua(L);
    QString line = host.mpConsole->getCurrentLine(windowName);
    lua_pushstring(L, line.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMiniConsoleFontSize
int TLuaInterpreter::setMiniConsoleFontSize(lua_State* L)
{
    QString windowName;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setMiniConsoleFontSize: bad argument #1 type (miniconsole name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        windowName = QString::fromUtf8(lua_tostring(L, 1));
    }
    int size;
    if (!lua_isnumber(L, 2)) {
        lua_pushfstring(L, "setMiniConsoleFontSize: bad argument #2 type (font size as number expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else {
        size = lua_tointeger(L, 2);
    }
    Host* host = &getHostFromLua(L);
    if (mudlet::self()->setWindowFontSize(host, windowName, size)) {
        lua_pushboolean(L, true);
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, R"(miniconsole "%s" not found)", windowName.toUtf8().constData());
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLineNumber
int TLuaInterpreter::getLineNumber(lua_State* L)
{
    Host& host = getHostFromLua(L);
    QString windowName;
    int s = 0;

    if (lua_gettop(L) > 0) { // Have more than one argument so first must be a console name
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L, "getLineNumber: bad argument #%d type (window name as string expected, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        lua_pushnumber(L, host.mpConsole->getLineNumber());
        return 1;
    } else {
        auto[success, lineNumber] = mudlet::self()->getLineNumber(&host, windowName);

        if (success) {
            lua_pushnumber(L, lineNumber);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, "window \"%s\" not found", windowName.toUtf8().constData());
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#updateMap
int TLuaInterpreter::updateMap(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (host.mpMap) {
#if defined(INCLUDE_3DMAPPER)
        if (host.mpMap->mpM) {
            host.mpMap->mpM->update();
        }
#endif
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                host.mpMap->mpMapper->mp2dMap->mNewMoveAction = true;
                host.mpMap->mpMapper->mp2dMap->update();
            }
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addMapMenu
int TLuaInterpreter::addMapMenu(lua_State* L)
{
    //    first arg = unique name, second arg= parent name, third arg = display name (=unique name if not provided)
    QString uniqueName;
    QStringList menuList;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "addMapMenu: wrong first argument type");
        lua_error(L);
        return 1;
    } else {
        uniqueName = lua_tostring(L, 1);
    }
    if (!lua_isstring(L, 2)) {
        menuList << "";
    } else {
        menuList << lua_tostring(L, 2);
    }
    if (!lua_isstring(L, 3)) {
        menuList << uniqueName;
    } else {
        menuList << lua_tostring(L, 3);
    }
    Host& host = getHostFromLua(L);
    if (host.mpMap) {
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                host.mpMap->mpMapper->mp2dMap->mUserMenus.insert(uniqueName, menuList);
            }
        }
    }
    return 0;
}

// Documentation: ? - public function missing documentation in wiki
int TLuaInterpreter::removeMapMenu(lua_State* L)
{
    QString uniqueName;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "removeMapMenu: wrong first argument type");
        lua_error(L);
        return 1;
    } else {
        uniqueName = lua_tostring(L, 1);
    }
    if (uniqueName == "") {
        return 0;
    }
    Host& host = getHostFromLua(L);
    if (host.mpMap) {
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                host.mpMap->mpMapper->mp2dMap->mUserMenus.remove(uniqueName);
                //remove all entries with this as parent
                QStringList removeList;
                removeList.append(uniqueName);
                bool newElement = true;
                while (newElement) {
                    newElement = false;
                    QMapIterator<QString, QStringList> it(host.mpMap->mpMapper->mp2dMap->mUserMenus);
                    while (it.hasNext()) {
                        it.next();
                        QStringList menuInfo = it.value();
                        QString parent = menuInfo[0];
                        if (removeList.contains(parent)) {
                            host.mpMap->mpMapper->mp2dMap->mUserMenus.remove(it.key());
                            if (it.key() != "" && !removeList.contains(it.key())) {
                                host.mpMap->mpMapper->mp2dMap->mUserMenus.remove(it.key());
                                removeList.append(it.key());
                                newElement = true;
                            }
                        }
                    }
                }
                qDebug() << removeList;
                QMapIterator<QString, QStringList> it2(host.mpMap->mpMapper->mp2dMap->mUserActions);
                while (it2.hasNext()) {
                    it2.next();
                    QString actParent = it2.value()[1];
                    if (removeList.contains(actParent)) {
                        host.mpMap->mpMapper->mp2dMap->mUserActions.remove(it2.key());
                    }
                }
            }
        }
    }
    return 0;
}

// Documentation: ? - public function missing documentation in wiki
int TLuaInterpreter::getMapMenus(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (host.mpMap) {
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                lua_newtable(L);
                QMapIterator<QString, QStringList> it(host.mpMap->mpMapper->mp2dMap->mUserMenus);
                while (it.hasNext()) {
                    it.next();
                    QString parent, display;
                    QStringList menuInfo = it.value();
                    parent = menuInfo[0];
                    display = menuInfo[1];
                    lua_pushstring(L, it.key().toLatin1().data());
                    lua_pushstring(L, parent.toLatin1().data());
                    lua_pushstring(L, display.toLatin1().data());
                    lua_settable(L, -3);
                }
            }
            return 1;
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addMapEvent
int TLuaInterpreter::addMapEvent(lua_State* L)
{
    QString uniqueName, eventName, parent, displayName;
    QStringList actionInfo;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "addMapEvent: bad argument #1 type (uniquename as string expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        uniqueName = QString::fromUtf8(lua_tostring(L, 1));
    }
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "addMapEvent: bad argument #2 type (event name as string expected, got %s!)", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        actionInfo << QString::fromUtf8(lua_tostring(L, 2));
    }
    if (!lua_isstring(L, 3)) {
        actionInfo << QString();
    } else {
        actionInfo << QString::fromUtf8(lua_tostring(L, 3));
    }
    if (!lua_isstring(L, 4)) {
        actionInfo << uniqueName;
    } else {
        actionInfo << QString::fromUtf8(lua_tostring(L, 4));
    }
    //variable number of arguments
    for (int i = 5; i <= lua_gettop(L); i++) {
        actionInfo << QString::fromUtf8(lua_tostring(L, i));
    }
    qDebug() << actionInfo;
    Host& host = getHostFromLua(L);
    if (host.mpMap) {
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                host.mpMap->mpMapper->mp2dMap->mUserActions.insert(uniqueName, actionInfo);
            }
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeMapEvent
int TLuaInterpreter::removeMapEvent(lua_State* L)
{
    QString displayName;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "removeMapEvent: wrong first argument type");
        lua_error(L);
        return 1;
    } else {
        displayName = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    if (host.mpMap) {
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                host.mpMap->mpMapper->mp2dMap->mUserActions.remove(displayName);
            }
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapEvents
int TLuaInterpreter::getMapEvents(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (host.mpMap) {
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                // create the result table
                lua_newtable(L);
                QMapIterator<QString, QStringList> it(host.mpMap->mpMapper->mp2dMap->mUserActions);
                while (it.hasNext()) {
                    it.next();
                    QStringList eventInfo = it.value();
                    lua_createtable(L, 0, 4);
                    lua_pushstring(L, eventInfo.at(0).toUtf8().constData());
                    lua_setfield(L, -2, "event name");
                    lua_pushstring(L, eventInfo.at(1).toUtf8().constData());
                    lua_setfield(L, -2, "parent");
                    lua_pushstring(L, eventInfo.at(2).toUtf8().constData());
                    lua_setfield(L, -2, "display name");
                    lua_createtable(L, eventInfo.length() - 3, 0);
                    for (int i = 3; i < eventInfo.length(); i++) {
                        lua_pushinteger(L, i - 2); //lua indexes are 1 based!
                        lua_pushstring(L, eventInfo.at(i).toUtf8().constData());
                        lua_settable(L, -3);
                    }
                    lua_setfield(L, -2, "arguments");

                    // Add the mapEvent object to the result table
                    lua_setfield(L, -2, it.key().toUtf8().constData());
                }
            }
            return 1;
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#centerview
int TLuaInterpreter::centerview(lua_State* L)
{
    Host& host = getHostFromLua(L);

    if (!host.mpMap || !host.mpMap->mpRoomDB || !host.mpMap->mpMapper) {
        lua_pushnil(L);
        lua_pushstring(L, "centerview: you haven't opened a map yet");
        return 2;
    }

    int roomId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "centerview: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tointeger(L, 1);
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (pR) {
        host.mpMap->mRoomIdHash[host.getName()] = roomId;
        host.mpMap->mNewMove = true;
#if defined(INCLUDE_3DMAPPER)
        if (host.mpMap->mpM) {
            host.mpMap->mpM->update();
        }
#endif

        if (host.mpMap->mpMapper->mp2dMap) {
            host.mpMap->mpMapper->mp2dMap->isCenterViewCall = true;
            host.mpMap->mpMapper->mp2dMap->update();
            host.mpMap->mpMapper->mp2dMap->isCenterViewCall = false;
            host.mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
        }
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "centerview: bad argument #1 value (%d is not a valid room id).", roomId);
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPlayerRoom
int TLuaInterpreter::getPlayerRoom(lua_State* L)
{
    Host& host = getHostFromLua(L);

    if (!host.mpMap || !host.mpMap->mpRoomDB || !host.mpMap->mpMapper) {
        lua_pushnil(L);
        lua_pushstring(L, "you haven't opened a map yet");
        return 2;
    }

    auto roomID = host.mpMap->mRoomIdHash.value(host.getName(), -1);
    if (roomID == -1) {
        lua_pushnil(L);
        lua_pushstring(L, "the player does not have a valid room id set");
        return 2;
    } else {
        lua_pushnumber(L, roomID);
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#copy
int TLuaInterpreter::copy(lua_State* L)
{
    std::string luaWindowName = "";
    if (lua_isstring(L, 1)) {
        luaWindowName = lua_tostring(L, 1);
    } else {
        luaWindowName = "main";
    }

    Host& host = getHostFromLua(L);
    QString windowName(luaWindowName.c_str());
    if (luaWindowName == "main") {
        host.mpConsole->copy();
    } else {
        mudlet::self()->copy(&host, windowName);
    }
    return 0;
}

// Documentation: ? - public function missing documentation in wiki
int TLuaInterpreter::cut(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.mpConsole->cut();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#paste
int TLuaInterpreter::paste(lua_State* L)
{
    std::string luaWindowName = "";
    if (lua_isstring(L, 1)) {
        luaWindowName = lua_tostring(L, 1);
    } else {
        luaWindowName = "main";
    }

    Host& host = getHostFromLua(L);
    QString windowName(luaWindowName.c_str());
    if (luaWindowName == "main") {
        host.mpConsole->paste();
    } else {
        mudlet::self()->pasteWindow(&host, windowName);
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#feedTriggers
int TLuaInterpreter::feedTriggers(lua_State* L)
{
    Host& host = getHostFromLua(L);

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L,
                        "feedTriggers: bad argument #1 type (imitation MUD server text as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }

    bool dataIsUtf8Encoded = true;
    if (lua_gettop(L) > 1) {
        if (!lua_isboolean(L, 2)) {
            lua_pushfstring(L,
                            "feedTriggers: bad argument #2 type (Utf8Encoded as boolean is optional, got %s!)",
                            luaL_typename(L, 2));
            return lua_error(L);
        }
        dataIsUtf8Encoded = lua_toboolean(L, 2);
    }
    QByteArray data{lua_tostring(L, 1)};

    QString currentEncoding = host.mTelnet.getEncoding();
    if (dataIsUtf8Encoded) {
        // We can convert the data from a QByteArray to a QString:
        if (currentEncoding == QStringLiteral("UTF-8")) {
            // Simple case: the encoding is already what we are using:
            std::string dataStdString{data.toStdString()};
            host.mpConsole->printOnDisplay(dataStdString);
            lua_pushboolean(L, true);
            return 1;
        }
        auto dataQString = QString::fromUtf8(data);
        // else
            // We need to transcode it from UTF-8 into the current Game Server
            // encoding - this can fail if it includes any characters (as UTF-8)
            // that the game encoding cannot convey:
        auto* pDataCodec = QTextCodec::codecForName(currentEncoding.toLatin1().constData());
        auto* pDataEncoder = pDataCodec->makeEncoder(QTextCodec::IgnoreHeader);
        if (!(currentEncoding.isEmpty() || currentEncoding == QStringLiteral("ASCII"))) {
            if (!pDataCodec->canEncode(dataQString)) {
                lua_pushnil(L);
                lua_pushfstring(L, "cannot send \"%s\" as it contains one or more characters that cannot be conveyed in the current game server encoding of \"%s\"", data.constData(), currentEncoding.toLatin1().constData());
                return 2;
            }

            std::string encodedText{pDataEncoder->fromUnicode(dataQString).toStdString()};
            host.mpConsole->printOnDisplay(encodedText);
            lua_pushboolean(L, true);
            return 1;
        }

        // else plain, raw ASCII, we hope!
        for (int i = 0, total = dataQString.size(); i < total; ++i) {
            if (dataQString.at(i).row() || dataQString.at(i).cell() > 127) {
                lua_pushnil(L);
                lua_pushfstring(L, "cannot send \"%s\" as it contains one or more characters that cannot be conveyed in the current game server encoding of \"ASCII\"", data.constData());
                return 2;
            }
        }

        // It is safe to use the data directly now as we have already proved it
        // to be plain ASCII
        std::string dataStdString{dataQString.toStdString()};
        host.mpConsole->printOnDisplay(dataStdString);
        lua_pushboolean(L, true);
        return 1;
    }

    // else the user is assumed to have coded it themselves into the Game
    // Server's current encoding - the backwards "compatible" form:
    std::string dataStdString{data.toStdString()};
    host.mpConsole->printOnDisplay(dataStdString);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isPrompt
int TLuaInterpreter::isPrompt(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int userCursorY = host.mpConsole->getLineNumber();
    if (userCursorY < host.mpConsole->buffer.promptBuffer.size() && userCursorY >= 0) {
        lua_pushboolean(L, host.mpConsole->buffer.promptBuffer.at(userCursorY));
        return 1;
    } else {
        if (host.mpConsole->mTriggerEngineMode && host.mpConsole->mIsPromptLine) {
            lua_pushboolean(L, true);
        } else {
            lua_pushboolean(L, false);
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setWindowWrap
int TLuaInterpreter::setWindowWrap(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "setWindowWrap: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    int luaFrom;
    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "setWindowWrap: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaFrom = lua_tointeger(L, 2);
    }

    Host& host = getHostFromLua(L);
    QString name = luaSendText.c_str();
    if (name == "main") {
        host.mpConsole->setWrapAt(luaFrom);
    } else {
        mudlet::self()->setWindowWrap(&host, name, luaFrom);
    }
    return 0;
}

// Documentation: ? - public function missing documentation in wiki
int TLuaInterpreter::setWindowWrapIndent(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "setWindowWrapIndent: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    int luaFrom;
    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "setWindowWrapIndent: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaFrom = lua_tointeger(L, 2);
    }

    Host& host = getHostFromLua(L);
    QString name = luaSendText.c_str();
    mudlet::self()->setWindowWrapIndent(&host, name, luaFrom);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLineCount
int TLuaInterpreter::getLineCount(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (lua_isstring(L, 1)) {
        std::string window = lua_tostring(L, 1);
        QString _window = window.c_str();
        lua_pushnumber(L, mudlet::self()->getLastLineNumber(&host, _window) + 1);
        return 1;
    } else {
        int lineNumber = host.mpConsole->getLineCount();
        lua_pushnumber(L, lineNumber);
        return 1;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getColumnNumber
int TLuaInterpreter::getColumnNumber(lua_State* L)
{
    QString windowName;
    Host& host = getHostFromLua(L);
    if (lua_gettop(L)) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "getColumnNumber: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, 1));
        }
        int result = 0;
        if (windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
            result = host.mpConsole->getColumnNumber();
        } else {
            result = mudlet::self()->getColumnNumber(&host, windowName);
        }
        if (result == -1) {
            lua_pushnil(L);
            lua_pushfstring(L, "window \"%s\" does not exist", windowName.toUtf8().constData());
            return 2;
        } else {
            lua_pushnumber(L, result);
            return 1;
        }
    } else {
        lua_pushnumber(L, host.mpConsole->getColumnNumber());
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getStopWatchTime
int TLuaInterpreter::getStopWatchTime(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "getStopWatchTime: bad argument #1 type (stopwatch id as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    int watchId = 0;
    QPair<bool, double> result;
    Host& host = getHostFromLua(L);
    if (lua_type(L, 1) == LUA_TNUMBER) {
        watchId = static_cast<int>(lua_tointeger(L, 1));
        result = host.getStopWatchTime(watchId);
        if (!result.first) {
            lua_pushnil(L);
            lua_pushfstring(L, "stopwatch with id %d not found", watchId);
            return 2;
        }

    } else {
        QString name = QString::fromUtf8(lua_tostring(L, 1));
        // Using an empty string will return the first unnamed stopwatch:
        watchId = host.findStopWatchId(name);
        if (!watchId) {
            lua_pushnil(L);
            if (name.isEmpty()) {
                lua_pushstring(L, "no unnamed stopwatches found");
            } else {
                lua_pushfstring(L, "stopwatch with name \"%s\" not found", name.toUtf8().constData());
            }
            return 2;
        }

        result = host.getStopWatchTime(watchId);
        // We have already validated the name to get the watchId - so for things
        // to fail now is, unlikely?
        if (Q_UNLIKELY(!result.first)) {
            lua_pushnil(L);
            lua_pushfstring(L, "stopwatch with name \"%s\" (id: %d) has disappeared - this should not happen, please report it to Mudlet developers", name.toUtf8().constData(), watchId);
            return 2;
        }
    }

    lua_pushnumber(L, result.second);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createStopWatch
int TLuaInterpreter::createStopWatch(lua_State* L)
{
    QString name;
    bool autoStart = true;
    int n = lua_gettop(L);
    int s = 0;
    if (n) {
        if (lua_type(L, ++s) == LUA_TBOOLEAN) {
            autoStart = lua_toboolean(L, s);
        } else if (lua_type(L, s) == LUA_TSTRING) {
            autoStart = false;
            name = QString::fromUtf8(lua_tostring(L, 1));
        } else if (lua_type(L, s) == LUA_TNIL) {
            ; // fallthrough for compatibility with old-style stopwatches in case createStopWatch(nil) is passed
            // note that 'nil' will still count towards the stack's gettop amount
        } else {
            lua_pushfstring(L, "createStopWatch: bad argument #%d type (name as string or autostart as boolean are optional, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        }

        if (n > 1) {
            if (lua_type(L, ++s) == LUA_TBOOLEAN) {
                autoStart = lua_toboolean(L, s);
            } else {
                lua_pushfstring(L, "createStopWatch: bad argument #%d type (autostart as boolean is optional, got %s!)", s,  luaL_typename(L, s));
                return lua_error(L);
            }
        }
    }


    Host& host = getHostFromLua(L);
    QPair<int, QString> result = host.createStopWatch(name);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushfstring(L, result.second.toUtf8().constData());
        return 2;
    }

    if (autoStart) {
        host.startStopWatch(result.first);
    }

    lua_pushnumber(L, result.first);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopStopWatch
int TLuaInterpreter::stopStopWatch(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "stopStopWatch: bad argument #1 type (stopwatch id as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    int watchId = 0;
    if (lua_type(L, 1) == LUA_TNUMBER) {
        watchId = static_cast<int>(lua_tointeger(L, 1));
        QPair<bool, QString> result = host.stopStopWatch(watchId);
        if (!result.first) {
            lua_pushnil(L);
            lua_pushstring(L, result.second.toUtf8().constData());
            return 2;
        }

    } else {
        QString name = QString::fromUtf8(lua_tostring(L, 1));
        QPair<bool, QString> result = host.stopStopWatch(name);
        if (!result.first) {
            lua_pushnil(L);
            lua_pushstring(L, result.second.toUtf8().constData());
            return 2;
        }

        watchId = host.findStopWatchId(name);
        // We have already validated the name to get the watchId - so for things
        // to fail now is, unlikely?
        if (Q_UNLIKELY(!watchId)) {
            lua_pushnil(L);
            lua_pushfstring(L, "stopwatch with name \"%s\" (id: %d) has disappeared - this should not happen, please report it to Mudlet developers", name.toUtf8().constData(), watchId);
            return 2;
        }
    }

    // We know that this watchId is valid so can use the return value directly
    // as we want to emulate the past behaviour where stopping the stopWatch
    // returned the elapsed time ONCE:
    lua_pushnumber(L, host.getStopWatchTime(watchId).second);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#startStopWatch
int TLuaInterpreter::startStopWatch(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "startStopWatch: bad argument #1 type (stopwatch id as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    if (lua_type(L, 1) == LUA_TNUMBER) {
        // Flag (if true) to replicate previous (reset and start again from zero
        // if call is repeated without any other actions being carried out on
        // stopwatch) behaviour if only a single NUMERIC argument (ID) supplied:
        bool autoResetAndRestart = true;
        if (lua_gettop(L) > 1) {
            if (!lua_isboolean(L, 2)) {
                lua_pushfstring(L, "startStopWatch: bad argument #2 type (automatic reset and restart as boolean is optional with a numeric stopwatch id, got %s!)", luaL_typename(L, 2));
                return lua_error(L);
            }
            autoResetAndRestart = lua_toboolean(L, 2);
        }

        QPair<bool, QString> result;
        if (autoResetAndRestart) {
            result = host.resetAndRestartStopWatch(static_cast<int>(lua_tointeger(L, 1)));
        } else {
            result = host.startStopWatch(static_cast<int>(lua_tointeger(L, 1)));
        }
        if (!result.first) {
            lua_pushnil(L);
            lua_pushstring(L, result.second.toUtf8().constData());
            return 2;
        }

        lua_pushboolean(L, true);
        return 1;
    }

    QPair<bool, QString> result = host.startStopWatch(QString::fromUtf8(lua_tostring(L, 1)));
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetStopWatch
int TLuaInterpreter::resetStopWatch(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "resetStopWatch: bad argument #1 type (stopwatch id as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    if (lua_type(L, 1) == LUA_TNUMBER) {
        QPair<bool, QString> result = host.resetStopWatch(static_cast<int>(lua_tointeger(L, 1)));
        if (!result.first) {
            lua_pushnil(L);
            lua_pushstring(L, result.second.toUtf8().constData());
            return 2;
        }

        lua_pushboolean(L, true);
        return 1;
    }

    QPair<bool, QString> result = host.resetStopWatch(QString::fromUtf8(lua_tostring(L, 1)));
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// No documentation available in wiki - internal helper
// to get ID of stopwatch from either a (numeric) ID argument or a (string) name
// - used to refactor the same code out of four separate stop-watch functions:
std::tuple<bool, int> TLuaInterpreter::getWatchId(lua_State* L, Host& h)
{
    if (lua_type(L, 1) == LUA_TNUMBER) {
        return std::make_tuple(true, static_cast<int>(lua_tointeger(L, 1)));
    }

    QString name = QString::fromUtf8(lua_tostring(L, 1));
    // Using an empty string will return the first unnamed stopwatch:
    int watchId = h.findStopWatchId(name);
    if (!watchId) {
        lua_pushnil(L);
        if (name.isEmpty()) {
            lua_pushstring(L, "no unnamed stopwatches found");
        } else {
            lua_pushfstring(L, "stopwatch with name \"%s\" not found", name.toUtf8().constData());
        }
        return std::make_tuple(false, 0);
    }

    return std::make_tuple(true, watchId);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#adjustStopWatch
int TLuaInterpreter::adjustStopWatch(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "adjustStopWatch: bad argument #1 type (stopwatch id as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    auto [success, watchId] = getWatchId(L, host);
    if (!success) {
        return 2;
    }

    if (!lua_isnumber(L, 2)) {
        lua_pushfstring(L, "adjustStopWatch: bad argument #2 type (modification in seconds as number expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    double adjustment = lua_tonumber(L, 2);
    bool result = host.adjustStopWatch(watchId, qRound(adjustment * 1000.0));
    // This is only likely to fail when a numeric first argument was given:
    if (!result) {
        lua_pushnil(L);
        lua_pushfstring(L, "stopwatch with id %d not found", watchId);
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteStopWatch
int TLuaInterpreter::deleteStopWatch(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "deleteStopWatch: bad argument #1 type (stopwatch id as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    auto [success, watchId] = getWatchId(L, host);
    if (!success) {
        return 2;
    }

    bool result = host.destroyStopWatch(watchId);
    // This is only likely to fail when a numeric first argument was given:
    if (!result) {
        lua_pushnil(L);
        lua_pushfstring(L, "stopwatch with id %d not found", watchId);
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setStopWatchPersistence
int TLuaInterpreter::setStopWatchPersistence(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "setStopWatchPersistence: bad argument #1 type (stopwatch id as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    auto [success, watchId] = getWatchId(L, host);
    if (!success) {
        return 2;
    }

    if (!lua_isboolean(L, 2)) {
        lua_pushfstring(L, "setStopWatchPersistence: bad argument #2 type (persistence as boolean expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    bool isPersistent = lua_toboolean(L, 2);
    // This is only likely to fail when a numeric first argument was given:
    if (!host.makeStopWatchPersistent(watchId, isPersistent)) {
        lua_pushnil(L);
        lua_pushfstring(L, "stopwatch with id %d not found", watchId);
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setStopWatchName
int TLuaInterpreter::setStopWatchName(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "setStopWatchName: bad argument #1 type (stopwatch id as number or current name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    int watchId = 0;
    Host& host = getHostFromLua(L);
    QString currentName;
    if (lua_type(L, 1) == LUA_TNUMBER) {
        watchId = static_cast<int>(lua_tointeger(L, 1));
    } else {
        // Using an empty string will return the first unnamed stopwatch:
        currentName = QString::fromUtf8(lua_tostring(L, 1));
    }

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "setStopWatchName: bad argument #2 type (stopwatch new name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString newName = QString::fromUtf8(lua_tostring(L, 2));

    QPair<bool, QString> result;
    if (currentName.isNull()) {
        // Will be null if no value was assigned to it - so use the id form:
        result = host.setStopWatchName(watchId, newName);
    } else {
        result = host.setStopWatchName(currentName, newName);
    }

    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: none - internal helper for getStopWatchBrokenDownTime()/getStopWatches()
void TLuaInterpreter::generateElapsedTimeTable(lua_State* L, const QStringList& elapsedTimeSplitString, const bool includeDecimalSeconds, const qint64 elapsedTimeMilliSeconds)
{
    lua_newtable(L);
    lua_pushstring(L, "negative");
    // Qt 5.7 seemed to not like comparing a QString with a QLatin1Char so
    // use a QLatin1String instead even though it is only a single character:
    lua_pushboolean(L, elapsedTimeSplitString.at(0) == QLatin1String("-"));
    lua_settable(L, -3);

    lua_pushstring(L, "days");
    lua_pushinteger(L, elapsedTimeSplitString.at(1).toInt());
    lua_settable(L, -3);

    lua_pushstring(L, "hours");
    lua_pushinteger(L, elapsedTimeSplitString.at(2).toInt());
    lua_settable(L, -3);

    lua_pushstring(L, "minutes");
    lua_pushinteger(L, elapsedTimeSplitString.at(3).toInt());
    lua_settable(L, -3);

    lua_pushstring(L, "seconds");
    lua_pushinteger(L, elapsedTimeSplitString.at(4).toInt());
    lua_settable(L, -3);

    lua_pushstring(L, "milliSeconds");
    lua_pushinteger(L, elapsedTimeSplitString.at(5).toInt());
    lua_settable(L, -3);

    if (includeDecimalSeconds) {
        lua_pushstring(L, "decimalSeconds");
        lua_pushnumber(L, elapsedTimeMilliSeconds / 1000.0);
        lua_settable(L, -3);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getStopWatchBrokenDownTime
int TLuaInterpreter::getStopWatchBrokenDownTime(lua_State* L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L, "getStopWatchBrokenDownTime: bad argument #1 type (stopwatch id as number or name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    auto [success, watchId] = getWatchId(L, host);
    if (!success) {
        return 2;
    }

    QPair<bool, QString> result = host.getBrokenDownStopWatchTime(watchId);
    // This is only likely to fail when a numeric first argument was given:
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    }

    const QStringList splitTimeString(result.second.split(QLatin1Char(':')));
    generateElapsedTimeTable(L, splitTimeString, false);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getStopWatches
int TLuaInterpreter::getStopWatches(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const QList<int> stopWatchIds = host.getStopWatchIds();
    lua_newtable(L);
    for (int index = 0, total = stopWatchIds.count(); index < total; ++index) {
        int watchId = stopWatchIds.at(index);
        lua_pushnumber(L, watchId);
        auto pStopWatch = host.getStopWatch(watchId);
        lua_newtable(L);
        {
            lua_pushstring(L, "name");
            lua_pushstring(L, pStopWatch->name().toUtf8().constData());
            lua_settable(L, -3);

            lua_pushstring(L, "isRunning");
            lua_pushboolean(L, pStopWatch->running());
            lua_settable(L, -3);

            lua_pushstring(L, "isPersistent");
            lua_pushboolean(L, pStopWatch->persistent());
            lua_settable(L, -3);

            lua_pushstring(L, "elapsedTime");
            const QStringList splitTimeString(pStopWatch->getElapsedDayTimeString().split(QLatin1Char(':')));
            generateElapsedTimeTable(L, splitTimeString, true, pStopWatch->getElapsedMilliSeconds());
            lua_settable(L, -3);
        }
        lua_settable(L, -3);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectSection
int TLuaInterpreter::selectSection(lua_State* L)
{
    int from;
    int to;
    int s = 1;
    int argumentsCount = lua_gettop(L);
    QString windowName;

    if (argumentsCount > 2) {
        if (!lua_isstring(L, s)) {
            lua_pushfstring(L, "selectSection: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
            s++;
        }
    }
    if (!lua_isnumber(L, s)) {
        lua_pushfstring(L, "selectSection: bad argument #%d type (from position as number expected, got %s!)", s, luaL_typename(L, 1));
        return lua_error(L);
    } else {
        from = lua_tointeger(L, s);
        s++;
    }

    if (!lua_isnumber(L, s)) {
        lua_pushfstring(L, "selectSection: bad argument #%d type (length as number expected, got %s!)", s, luaL_typename(L, 1));
        return lua_error(L);
    } else {
        to = lua_tointeger(L, s);
    }

    Host& host = getHostFromLua(L);

    int ret;
    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        ret = host.mpConsole->selectSection(from, to);
    } else {
        ret = mudlet::self()->selectSection(&host, windowName, from, to);
    }
    lua_pushboolean(L, ret == -1 ? false : true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getSelection
int TLuaInterpreter::getSelection(lua_State* L)
{
    bool valid;
    QString text;
    int start, length;

    auto& host = getHostFromLua(L);

    QString windowName;
    if (lua_gettop(L) > 0) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "getSelection: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, 1));
        }
    }
    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        std::tie(valid, text, start, length) = host.mpConsole->getSelection();
    } else {
        std::tie(valid, text, start, length) = mudlet::self()->getSelection(&host, windowName);
    }

    if (!valid) {
        lua_pushnil(L);
        lua_pushstring(L, text.toUtf8().constData());
        return 2;
    }

    lua_pushstring(L, text.toUtf8().constData());
    lua_pushnumber(L, start);
    lua_pushnumber(L, length);
    return 3;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#moveCursor
int TLuaInterpreter::moveCursor(lua_State* L)
{
    int s = 1;
    int n = lua_gettop(L);
    std::string a1;
    if (n > 2) {
        if (!lua_isstring(L, s)) {
            lua_pushstring(L, "moveCursor: wrong argument type");
            lua_error(L);
            return 1;
        } else {
            a1 = lua_tostring(L, s);
            s++;
        }
    }
    int luaFrom;
    if (!lua_isnumber(L, s)) {
        lua_pushstring(L, "moveCursor: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaFrom = lua_tointeger(L, s);
        s++;
    }

    int luaTo;
    if (!lua_isnumber(L, s)) {
        lua_pushstring(L, "moveCursor: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaTo = lua_tointeger(L, s);
    }

    Host& host = getHostFromLua(L);

    if (a1 == "main" || n < 3) {
        lua_pushboolean(L, host.mpConsole->moveCursor(luaFrom, luaTo));
    } else {
        QString windowName = a1.c_str();
        lua_pushboolean(L, mudlet::self()->moveCursor(&host, windowName, luaFrom, luaTo));
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setConsoleBufferSize
int TLuaInterpreter::setConsoleBufferSize(lua_State* L)
{
    int s = 1;
    int n = lua_gettop(L);
    std::string a1;
    if (n > 2) {
        if (!lua_isstring(L, s)) {
            lua_pushstring(L, "setConsoleBufferSize: wrong argument type");
            lua_error(L);
            return 1;
        } else {
            a1 = lua_tostring(L, s);
            s++;
        }
    }
    int luaFrom;
    if (!lua_isnumber(L, s)) {
        lua_pushstring(L, "setConsoleBufferSize: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaFrom = lua_tointeger(L, s);
        s++;
    }

    int luaTo;
    if (!lua_isnumber(L, s)) {
        lua_pushstring(L, "setConsoleBufferSize: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaTo = lua_tointeger(L, s);
    }

    Host& host = getHostFromLua(L);

    if (a1 == "main" || n < 3) {
        host.mpConsole->buffer.setBufferSize(luaFrom, luaTo);
    } else {
        QString windowName = a1.c_str();
        mudlet::self()->setConsoleBufferSize(&host, windowName, luaFrom, luaTo);
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableScrollBar
int TLuaInterpreter::enableScrollBar(lua_State* L)
{
    int n = lua_gettop(L);
    QString windowName;
    if (n == 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "enableScrollBar: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            windowName = lua_tostring(L, 1);
        }
    }

    Host& host = getHostFromLua(L);

    mudlet::self()->setScrollBarVisible(&host, windowName, true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableScrollBar
int TLuaInterpreter::disableScrollBar(lua_State* L)
{
    int n = lua_gettop(L);
    QString windowName;
    if (n == 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "disableScrollBar: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            windowName = lua_tostring(L, 1);
        }
    }

    Host& host = getHostFromLua(L);

    mudlet::self()->setScrollBarVisible(&host, windowName, false);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableScrollBar
int TLuaInterpreter::enableCommandLine(lua_State* L)
{
    int n = lua_gettop(L);
    QString windowName;
    if (n == 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "enableCommandLine: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            windowName = lua_tostring(L, 1);
        }
    }

    Host& host = getHostFromLua(L);

    mudlet::self()->setMiniConsoleCmdVisible(&host, windowName, true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableScrollBar
int TLuaInterpreter::disableCommandLine(lua_State* L)
{
    int n = lua_gettop(L);
    QString windowName;
    if (n == 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "disableCommandLine: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            windowName = lua_tostring(L, 1);
        }
    }

    Host& host = getHostFromLua(L);

    mudlet::self()->setMiniConsoleCmdVisible(&host, windowName, false);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#replace
int TLuaInterpreter::replace(lua_State* L)
{
    std::string a1 = "";
    std::string a2 = "";
    int n = lua_gettop(L);
    int s = 1;
    if (!lua_isstring(L, s)) {
        lua_pushstring(L, "replace: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        a1 = lua_tostring(L, s);
        s++;
    }

    QString _name(a1.c_str());
    std::string luaSendText = "";
    if (n > 1) {
        if (!lua_isstring(L, s)) {
            lua_pushstring(L, "replace: wrong argument type");
            lua_error(L);
            return 1;
        } else {
            a2 = lua_tostring(L, s);
        }
    }

    Host& host = getHostFromLua(L);
    if (n == 1) {
        host.mpConsole->replace(QString(a1.c_str()));
    } else {
        mudlet::self()->replace(&host, _name, QString(a2.c_str()));
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteLine
int TLuaInterpreter::deleteLine(lua_State* L)
{
    std::string name = "";
    if (lua_gettop(L) == 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "deleteLine: wrong argument type");
            lua_error(L);
            return 1;
        } else {
            name = lua_tostring(L, 1);
        }
    }

    QString _name(name.c_str());
    Host& host = getHostFromLua(L);

    if (name.empty()) {
        host.mpConsole->skipLine();
    } else {
        mudlet::self()->deleteLine(&host, _name);
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#saveMap
int TLuaInterpreter::saveMap(lua_State* L)
{
    QString location;
    int saveVersion = 0;

    if (lua_gettop(L) > 0) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L,
                            "saveMap: bad argument #1 type (save location path and file name "
                            "as string expected, got %s!)",
                            luaL_typename(L, 1));
            return lua_error(L);
        } else {
            location = QString::fromUtf8(lua_tostring(L, 1));
        }
        if (lua_gettop(L) > 1) {
            if (!lua_isnumber(L, 2)) {
                lua_pushfstring(L,
                                "saveMap: bad argument #2 type (map format version as "
                                "integer expected, got %s!)",
                                luaL_typename(L, 2));
                return lua_error(L);
            } else {
                saveVersion = lua_tointeger(L, 2);
            }
        }
    }

    Host& host = getHostFromLua(L);

    bool error = host.mpConsole->saveMap(location, saveVersion);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setExitStub
int TLuaInterpreter::setExitStub(lua_State* L)
{
    //args:room id, direction (as given by the #define direction table), status
    int roomId, dirType;
    bool status;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "setExitStub: Need a room number as first argument");
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tonumber(L, 1);
    }
    dirType = dirToNumber(L, 2);
    if (!dirType) {
        lua_pushstring(L, "setExitStub: Need a dir number as 2nd argument");
        lua_error(L);
        return 1;
    }
    if (!lua_isboolean(L, 3)) {
        lua_pushstring(L, "setExitStub: Need a true/false for third argument");
        lua_error(L);
        return 1;
    } else {
        status = lua_toboolean(L, 3);
    }

    Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return 0;
    }
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        lua_pushstring(L, "setExitStub: RoomId doesn't exist");
        lua_error(L);
        return 1;
    }
    if (dirType > 12 || dirType < 1) {
        lua_pushstring(L, "setExitStub: dirType must be between 1 and 12");
        lua_error(L);
        return 1;
    }
    pR->setExitStub(dirType, status);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#connectExitStub
int TLuaInterpreter::connectExitStub(lua_State* L)
{
    int roomId;
    int toRoom;
    int dirType;
    int roomsGiven = 0;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "connectExitStub: Need a room number as first argument");
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tonumber(L, 1);
    }
    dirType = dirToNumber(L, 2);
    if (!dirType) {
        lua_pushstring(L, "connectExitStub: Need a direction number (or room id) as 2nd argument");
        lua_error(L);
        return 1;
    }
    if (!lua_isnumber(L, 3) && !lua_isstring(L, 3)) {
        roomsGiven = 0;
    } else {
        roomsGiven = 1;
        toRoom = lua_tonumber(L, 2);
        dirType = dirToNumber(L, 3);
        if (!dirType) {
            lua_pushstring(L, "connectExitStub: Invalid direction entered.");
            lua_error(L);
            return 1;
        }
    }
    Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return 0;
    }
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        lua_pushstring(L, "connectExitStub: RoomId doesn't exist");
        lua_error(L);
        return 1;
    }
    if (!pR->exitStubs.contains(dirType)) {
        lua_pushstring(L, "connectExitStub: ExitStub doesn't exist");
        lua_error(L);
        return 1;
    }
    if (roomsGiven) {
        TRoom* pR_to = host.mpMap->mpRoomDB->getRoom(toRoom);
        if (!pR_to) {
            lua_pushstring(L, "connectExitStub: toRoom doesn't exist");
            lua_error(L);
            return 1;
        }
        Host& host = getHostFromLua(L);
        lua_pushboolean(L, host.mpMap->setExit(roomId, toRoom, dirType));
    } else {
        if (!pR->exitStubs.contains(dirType)) {
            lua_pushstring(L, "connectExitStub: ExitStub doesn't exist");
            lua_error(L);
            return 1;
        }
        host.mpMap->connectExitStub(roomId, dirType);
        // Nothing has yet been put onto stack for a LUA return value in this case,
        // and it should always be possible to add a stub exit, so provide a true value :
        lua_pushboolean(L, true);
    }
    host.mpMap->mMapGraphNeedsUpdate = true;
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getExitStubs
int TLuaInterpreter::getExitStubs(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getExitStubs: no map present or loaded!");
        return 2;
    }

    int roomId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getExitStubs: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tonumber(L, 1);
    }

    // Previously threw a Lua error on non-existent room!
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        lua_pushnil(L);
        lua_pushfstring(L, "getExitStubs: bad argument #1 value (number %d is not a valid room id).", roomId);
        return 2;
    } else {
        QList<int> stubs = pR->exitStubs;
        if (!stubs.empty()) {
            lua_newtable(L);
            for (int i = 0, total = stubs.size(); i < total; ++i) {
                lua_pushnumber(L, i);
                lua_pushnumber(L, stubs.at(i));
                lua_settable(L, -3);
            }
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, "getExitStubs: no stubs in this room with id %d.", roomId);
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getExitStubs1
int TLuaInterpreter::getExitStubs1(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getExitStubs1: no map present or loaded!");
        return 2;
    }

    int roomId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getExitStubs1: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tonumber(L, 1);
    }

    // Previously threw a Lua error on non-existent room!
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        lua_pushnil(L);
        lua_pushfstring(L, "getExitStubs1: bad argument #1 value (number %d is not a valid room id).", roomId);
        return 2;
    } else {
        QList<int> stubs = pR->exitStubs;
        if (!stubs.empty()) {
            lua_newtable(L);
            for (int i = 0, total = stubs.size(); i < total; ++i) {
                lua_pushnumber(L, i + 1);
                lua_pushnumber(L, stubs.at(i));
                lua_settable(L, -3);
            }
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, "getExitStubs1: no stubs in this room with id %d.", roomId);
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getModulePath
int TLuaInterpreter::getModulePath(lua_State* L)
{
    QString moduleName;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "getModulePath: Module be be a string");
        lua_error(L);
        return 1;
    } else {
        moduleName = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QMap<QString, QStringList> modules = host.mInstalledModules;
    if (modules.contains(moduleName)) {
        QString modPath = modules[moduleName][0];
        lua_pushstring(L, modPath.toLatin1().data());
        return 1;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getModulePriority
int TLuaInterpreter::getModulePriority(lua_State* L)
{
    QString moduleName;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "getModulePriority: Module be be a string");
        lua_error(L);
        return 1;
    } else {
        moduleName = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    if (host.mModulePriorities.contains(moduleName)) {
        int priority = host.mModulePriorities[moduleName];
        lua_pushnumber(L, priority);
        return 1;
    } else {
        lua_pushstring(L, "getModulePriority: Module doesn't exist");
        lua_error(L);
        return 1;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setModulePriority
int TLuaInterpreter::setModulePriority(lua_State* L)
{
    QString moduleName;
    int modulePriority;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setModulePriority: bad argument #1 type (module name as string expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        moduleName = lua_tostring(L, 1);
    }
    if (!lua_isnumber(L, 2)) {
        lua_pushfstring(L, "setModulePriority: bad argument #2 type (module priority as number expected, got %s!)", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        modulePriority = lua_tonumber(L, 2);
    }
    Host& host = getHostFromLua(L);
    if (host.mInstalledModules.contains(moduleName)) {
        host.mModulePriorities[moduleName] = modulePriority;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, "module doesn't exist");
        return 2;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadMap
int TLuaInterpreter::loadMap(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString location;
    if (lua_gettop(L)) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L,
                            "loadMap: bad argument #1 type (Map pathFile as string is optional {loads last\n"
                            "stored map if omitted}, got %s!)",
                            luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            location = QString::fromUtf8(lua_tostring(L, 1));
        }
    }

    bool isOk = false;
    if (!location.isEmpty() && location.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)) {
        QString errMsg;
        isOk = host.mpConsole->importMap(location, &errMsg);
        if (!isOk) {
            // A false was returned which indicates an error, convert it to a nil
            lua_pushnil(L);
            // And add the expected error message, is to be structured in a
            // compatible manner
            if (!errMsg.isEmpty()) {
                lua_pushstring(L, errMsg.toUtf8().constData());
                return 2;
            } else {
                return 1;
            }
        }
    } else {
        isOk = host.mpConsole->loadMap(location);
    }
    lua_pushboolean(L, isOk);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableTimer
int TLuaInterpreter::enableTimer(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "enableTimer: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    bool error = host.getTimerUnit()->enableTimer(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableTimer
int TLuaInterpreter::disableTimer(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "disableTimer: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    bool error = host.getTimerUnit()->disableTimer(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableKey
int TLuaInterpreter::enableKey(lua_State* L)
{
    QString keyName;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "enableKey: bad argument #1 type (key name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        keyName = QString::fromUtf8(lua_tostring(L, 1));
    }
    Host& host = getHostFromLua(L);
    bool error = host.getKeyUnit()->enableKey(keyName);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableKey
int TLuaInterpreter::disableKey(lua_State* L)
{
    QString keyName;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "disableKey: bad argument #1 type (key name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        keyName = QString::fromUtf8(lua_tostring(L, 1));
    }
    Host& host = getHostFromLua(L);
    bool error = host.getKeyUnit()->disableKey(keyName);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killKey
int TLuaInterpreter::killKey(lua_State* L)
{
    QString keyName;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "killKey: bad argument #1 type (key name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        keyName = QString::fromUtf8(lua_tostring(L, 1));
    }
    Host& host = getHostFromLua(L);
    bool error = host.getKeyUnit()->killKey(keyName);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableAlias
int TLuaInterpreter::enableAlias(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "enableAlias: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    bool error = host.getAliasUnit()->enableAlias(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableAlias
int TLuaInterpreter::disableAlias(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "disableAlias: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    bool error = host.getAliasUnit()->disableAlias(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killAlias
int TLuaInterpreter::killAlias(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "killAlias: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    lua_pushboolean(L, host.getAliasUnit()->killAlias(text));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableTrigger
int TLuaInterpreter::enableTrigger(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "enableTrigger: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    bool error = host.getTriggerUnit()->enableTrigger(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableTrigger
int TLuaInterpreter::disableTrigger(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "disableTrigger: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    bool error = host.getTriggerUnit()->disableTrigger(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableScript
int TLuaInterpreter::enableScript(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "enableScript: bad argument #1 type (script name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name = QString::fromUtf8(lua_tostring(L, 1));

    Host& host = getHostFromLua(L);
    int cnt = 0;
    QMap<int, TScript*> scripts = host.getScriptUnit()->getScriptList();
    for (auto script : scripts) {
        if (script->getName() == name) {
            cnt++;
            script->setIsActive(true);
        }
    }
    if (cnt == 0) {
        lua_pushnil(L);
        lua_pushstring(L, QStringLiteral("script \"%1\" not found").arg(name).toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableScript
int TLuaInterpreter::disableScript(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "disableScript: bad argument #1 type (script name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name = QString::fromUtf8(lua_tostring(L, 1));

    Host& host = getHostFromLua(L);
    int cnt = 0;
    QMap<int, TScript*> scripts = host.getScriptUnit()->getScriptList();
    for (auto script : scripts) {
        if (script->getName() == name) {
            cnt++;
            script->setIsActive(false);
        }
    }
    if (cnt == 0) {
        lua_pushnil(L);
        lua_pushstring(L, QStringLiteral("script \"%1\" not found").arg(name).toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killTimer
int TLuaInterpreter::killTimer(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "killTimer: killTimer requires a string ID");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    lua_pushboolean(L, host.killTimer(text));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killTrigger
int TLuaInterpreter::killTrigger(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "killTrigger: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    lua_pushboolean(L, host.killTrigger(text));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#remainingTime
int TLuaInterpreter::remainingTime(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "remainingTime: bad argument #1 (timer name as string or timer id as number expected, got %s!", luaL_typename(L, 1));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    int result = -2;
    QString timerName;
    qint64 timerId = 0;
    if (lua_type(L, 1) == LUA_TNUMBER) {
        // Is definitely a number and not a string that can be coerced into a number
        timerId = lua_tointeger(L, 1);
        result = host.getTimerUnit()->remainingTime(static_cast<int>(timerId));
    } else {
        timerName = QString::fromUtf8(lua_tostring(L, 1));
        result = host.getTimerUnit()->remainingTime(timerName);
    }

    if (result == -1) {
        lua_pushnil(L);
        lua_pushstring(L, "timer is inactive or expired");
        return 2;
    }

    if (result == -2) {
        lua_pushnil(L);
        if (timerName.isNull()) {
            // timerName was never set so we must have used the number
            lua_pushfstring(L, "timer id %d not found", timerId);
        } else {
            lua_pushfstring(L, "timer named \"%s\" not found", timerName.toUtf8().constData());
        }
        return 2;
    }

    lua_pushnumber(L, result / 1000.0);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#closeMudlet
int TLuaInterpreter::closeMudlet(lua_State* L)
{
    mudlet::self()->forceClose();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadWindowLayout
int TLuaInterpreter::loadWindowLayout(lua_State* L)
{
    lua_pushboolean(L, mudlet::self()->loadWindowLayout());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#saveWindowLayout
int TLuaInterpreter::saveWindowLayout(lua_State* L)
{
    mudlet::self()->mHasSavedLayout = false;
    lua_pushboolean(L, mudlet::self()->saveWindowLayout());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#saveProfile
int TLuaInterpreter::saveProfile(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString saveToDir;
    if (lua_isstring(L, 1)) {
        saveToDir = QString::fromUtf8(lua_tostring(L, 1));
    }

    std::tuple<bool, QString, QString> result = host.saveProfile(saveToDir);

    if (std::get<0>(result)) {
        lua_pushboolean(L, true);
        lua_pushstring(L, (std::get<1>(result).toUtf8().constData()));
        return 2;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, QString("Couldn't save %1 to %2 because: %3").arg(host.getName(), std::get<1>(result), std::get<2>(result)).toUtf8().constData());
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setFont
int TLuaInterpreter::setFont(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);

    QString windowName;
    int s = 0;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L, "setFont: bad argument #%d type for the optional window name - expected string, got %s!", s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }

    QString font;
    if (!lua_isstring(L, ++s)) {
        lua_pushfstring(L, "setFont: bad argument #%d type (name as string expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        font = QString::fromUtf8(lua_tostring(L, s));
    }

#if defined(Q_OS_LINUX)
    // On Linux ensure that emojis are displayed in colour even if this font
    // doesn't support it:
    QFont::insertSubstitution(font, QStringLiteral("Noto Color Emoji"));
#endif

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        if (mudlet::self()->mConsoleMap.contains(pHost)) {
            if (auto [setNewFont, errorMessage] = pHost->setDisplayFont(font); !setNewFont) {
                lua_pushnil(L);
                lua_pushstring(L, errorMessage.toUtf8().constData());
                return 2;
            }

            // apply changes to main console and its while-scrolling component too.
            mudlet::self()->mConsoleMap[pHost]->mUpperPane->setFont(pHost->getDisplayFont());
            mudlet::self()->mConsoleMap[pHost]->mUpperPane->updateScreenView();
            mudlet::self()->mConsoleMap[pHost]->mUpperPane->forceUpdate();
            mudlet::self()->mConsoleMap[pHost]->mLowerPane->setFont(pHost->getDisplayFont());
            mudlet::self()->mConsoleMap[pHost]->mLowerPane->updateScreenView();
            mudlet::self()->mConsoleMap[pHost]->mLowerPane->forceUpdate();
            mudlet::self()->mConsoleMap[pHost]->refresh();
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "could not find the main window");
            return 2;
        }
    } else {
        if (!mudlet::self()->setWindowFont(pHost, windowName, font)) {
            lua_pushnil(L);
            lua_pushfstring(L, R"(window "%s" not found)", windowName.toUtf8().constData());
            return 2;
        }
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getFont
int TLuaInterpreter::getFont(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);

    QString windowName = QStringLiteral("main");
    QString font;
    if (lua_gettop(L) == 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "getFont: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, 1));

            if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
                font = pHost->mpConsole->mUpperPane->fontInfo().family();
            } else {
                font = mudlet::self()->getWindowFont(pHost, windowName);
            }

            if (font.isEmpty()) {
                lua_pushnil(L);
                lua_pushfstring(L, R"(window "%s" not found)", windowName.toUtf8().constData());
                return 2;
            }
        }
    } else {
        font = pHost->mpConsole->mUpperPane->fontInfo().family();
    }

    lua_pushstring(L, font.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setFontSize
int TLuaInterpreter::setFontSize(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);

    QString windowName;
    int s = 0;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L,
                            "setFontSize: bad argument #%d type (more than one argument supplied and first,\n"
                            "window name, as string expected (omission selects \"main\" window), got %s!",
                            s,
                            luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }

    int size;
    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "setFontSize: bad argument #%d type (size as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        size = lua_tointeger(L, s);
        if (size <= 0) {
            // just throw an error, no default needed.
            lua_pushnil(L);
            lua_pushstring(L, "size cannot be 0 or negative");
            return 2;
        }
    }

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        if (mudlet::self()->mConsoleMap.contains(pHost)) {
            // get host profile display font and alter it, since that is how it's done in Settings.
            pHost->setDisplayFontSize(size);
            // apply changes to main console and its while-scrolling component too.
            mudlet::self()->mConsoleMap[pHost]->mUpperPane->updateScreenView();
            mudlet::self()->mConsoleMap[pHost]->mUpperPane->forceUpdate();
            mudlet::self()->mConsoleMap[pHost]->mLowerPane->updateScreenView();
            mudlet::self()->mConsoleMap[pHost]->mLowerPane->forceUpdate();
            mudlet::self()->mConsoleMap[pHost]->refresh();
            lua_pushboolean(L, true);
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "could not find the main window");
            return 2;
        }
    } else {
        if (mudlet::self()->setWindowFontSize(pHost, windowName, size)) {
            lua_pushboolean(L, true);
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, R"(window "%s" not found)", windowName.toUtf8().constData());
            return 2;
        }
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getFontSize
int TLuaInterpreter::getFontSize(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);

    QString windowName = QStringLiteral("main");
    int rval = -1;
    if (lua_gettop(L) == 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "getFontSize: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, 1));

            if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
                rval = pHost->getDisplayFont().pointSize();
            } else {
                rval = mudlet::self()->getFontSize(pHost, windowName);
            }
        }
    } else {
        rval = pHost->getDisplayFont().pointSize();
    }

    if (rval <= -1) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, rval);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#openUserWindow
int TLuaInterpreter::openUserWindow(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = QString();
    if (lua_type(L, 1) != LUA_TSTRING) {
        lua_pushfstring(L, "openUserWindow:  bad argument #1 type (name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        name = QString::fromUtf8(lua_tostring(L, 1));
    }

    bool loadLayout = true, autoDock = true;
    if (n > 1) {
        if (!lua_isboolean(L, 2)) {
            lua_pushfstring(L, "openUserWindow:  bad argument #2 type (loadLayout as boolean expected, got %s!)", luaL_typename(L, 2));
            return lua_error(L);
        } else {
            loadLayout = lua_toboolean(L, 2);
        }
    }
    if (n > 2) {
        if (!lua_isboolean(L, 3)) {
            lua_pushfstring(L, "openUserWindow:  bad argument #3 type (autoDock as boolean expected, got %s!)", luaL_typename(L, 3));
            return lua_error(L);
        } else {
            autoDock = lua_toboolean(L, 3);
        }
    }

    QString area = QString();
    if (n > 3) {
        if (lua_type(L, 4) != LUA_TSTRING) {
            lua_pushfstring(L, "openUserWindow: bad argument #4 type (area as string expected, got %s!)", luaL_typename(L, 4));
            return lua_error(L);
        } else {
            area = QString::fromUtf8(lua_tostring(L, 4));
        }
    }

    Host& host = getHostFromLua(L);
    //Dont create Userwindow if there is a Label with the same name already. It breaks the UserWindow

    if (auto [success, message] = mudlet::self()->openWindow(&host, name, loadLayout, autoDock, area.toLower()); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setUserWindowTitle
int TLuaInterpreter::setUserWindowTitle(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setUserWindowTitle: bad argument #1 type (name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name = QString::fromUtf8(lua_tostring(L, 1));

    QString title;
    if (lua_gettop(L) > 1) {
        if (!lua_isstring(L, 2)) {
            lua_pushfstring(L, "setUserWindowTitle: bad argument #2 type (title as string is optional, got %s!)", luaL_typename(L, 2));
            return lua_error(L);
        }
        title = QString::fromUtf8(lua_tostring(L, 2));
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.mpConsole->setUserWindowTitle(name, title); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMapWindowTitle
int TLuaInterpreter::setMapWindowTitle(lua_State* L)
{
    QString title;
    if (lua_gettop(L)) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "setMapWindowTitle: bad argument #1 type (title as string is optional, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        }
        title = QString::fromUtf8(lua_tostring(L, 1));
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.setMapperTitle(title); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::getMudletInfo(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QStringList knownEncodings{"ASCII"};
    QString currentEncoding{host.mTelnet.getEncoding()};
    {
        auto adjustEncoding = [](auto encodingName) {
            auto originalEncoding = encodingName;
            if (encodingName.startsWith("M_")) {
                encodingName.remove(0, 2);
            }

            return (originalEncoding == encodingName) ? originalEncoding : QStringLiteral("%1 (%2)").arg(encodingName, originalEncoding);
        };
        for (const auto& encoding : host.mTelnet.getEncodingsList()) {
            knownEncodings.append(adjustEncoding(QString(encoding)));
        }
        QCollator sorter;
        sorter.setNumericMode(true);
        sorter.setCaseSensitivity(Qt::CaseInsensitive);
        std::sort(knownEncodings.begin(), knownEncodings.end(), sorter);

        if (currentEncoding.isEmpty()) {
            currentEncoding = "\"ASCII\"";
        } else {
            currentEncoding = adjustEncoding(currentEncoding);
        }
    }



    host.postMessage(QStringLiteral("[ INFO ]  - Current encoding: %1").arg(currentEncoding));

    host.postMessage(QStringLiteral("[ INFO ]  - Available encodings:"));
    host.postMessage(QStringLiteral("  %1").arg(knownEncodings.join(QStringLiteral(", "))));

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createMiniConsole
int TLuaInterpreter::createMiniConsole(lua_State* L)
{
    std::string luaSendText = "";
    std::string luaSendWindow = "";
    int x, y, width, height, counter;
    counter = 3;
    //make the windowname optional by using counter. If windowname "main" add to main console
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "createMiniConsole: bad argument #1 type (miniconsole name as string expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        luaSendWindow = lua_tostring(L, 1);
        if (luaSendWindow == "main") {
            // QString::compare is zero for a match on the "default"
            // case so clear the variable - to flag this as the main
            // window case - as is the case for an empty string
            luaSendWindow.clear();
        }
    }
    if (!lua_isnumber(L, 2)) {
        if (!lua_isstring(L, 2)) {
            lua_pushfstring(L, "createMiniConsole: bad argument #2 type (miniconsole name as string expected, got %s!)", luaL_typename(L, 2));
            lua_error(L);
            return 1;
        }
        luaSendText = lua_tostring(L, 2);
    } else {
        luaSendText = luaSendWindow;
        luaSendWindow.clear();
        counter = 2;
    }

    if (!lua_isnumber(L, counter)) {
        lua_pushfstring(L, "createMiniConsole: bad argument #%d type (miniconsole x-coordinate as number expected, got %s!)", counter, luaL_typename(L, counter));
        lua_error(L);
        return 1;
    } else {
        x = lua_tonumber(L, counter);
        counter++;
    }
    if (!lua_isnumber(L, counter)) {
        lua_pushfstring(L, "createMiniConsole: bad argument #%d type (miniconsole y-coordinate as number expected, got %s!)", counter, luaL_typename(L, counter));
        lua_error(L);
        return 1;
    } else {
        y = lua_tonumber(L, counter);
        counter++;
    }
    if (!lua_isnumber(L, counter)) {
        lua_pushfstring(L, "createMiniConsole: bad argument #%d type (miniconsole width as number expected, got %s!)", counter, luaL_typename(L, counter));
        lua_error(L);
        return 1;
    } else {
        width = lua_tonumber(L, counter);
        counter++;
    }
    if (!lua_isnumber(L, counter)) {
        lua_pushfstring(L, "createMiniConsole: bad argument #%d type (miniconsole height as number expected, got %s!)", counter, luaL_typename(L, counter));
        lua_error(L);
        return 1;
    } else {
        height = lua_tonumber(L, 5);
    }
    Host& host = getHostFromLua(L);
    QString name(luaSendText.c_str());
    QString windowname(luaSendWindow.c_str());
    if (auto [success, message] = mudlet::self()->createMiniConsole(&host, windowname, name, x, y, width, height); !success) {
        lua_pushboolean(L, false);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createLabel
int TLuaInterpreter::createLabel(lua_State* L)
{
    QString labelName;
    QString windowName = QLatin1String("main");

    if (lua_type(L, 1) != LUA_TSTRING) {
        lua_pushfstring(L, "createLabel: bad argument #1 type (label or parent window name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else if ((lua_type(L, 1) == LUA_TSTRING) && (lua_type(L, 2) == LUA_TSTRING)) {
        windowName = QString::fromUtf8(lua_tostring(L, 1));
        labelName = QString::fromUtf8(lua_tostring(L, 2));
        createLabelUserWindow(L, windowName, labelName);
    } else if ((lua_type(L, 1) == LUA_TSTRING) && (lua_type(L, 2) == LUA_TNUMBER)) {
        labelName = QString::fromUtf8(lua_tostring(L, 1));
        createLabelMainWindow(L, labelName);
    } else {
        lua_pushfstring(L, "createLabel: bad argument #2 type (label name as string or label x-coordinate as number expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    return 1;
}

// Internal Function createLabel in an UserWindow
int TLuaInterpreter::createLabelUserWindow(lua_State* L, const QString& windowName, const QString& labelName)
{
    int n = lua_gettop(L);
    int x = 0;
    if (!lua_isnumber(L, 3)) {
        lua_pushfstring(L, "createLabel: bad argument #3 type (label x-coordinate as number expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    } else {
        x = lua_tonumber(L, 3);
    }
    int y = 0;
    if (!lua_isnumber(L, 4)) {
        lua_pushfstring(L, "createLabel: bad argument #4 type (label y-coordinate as number expected, got %s!)", luaL_typename(L, 4));
        return lua_error(L);
    } else {
        y = lua_tonumber(L, 4);
    }
    int width = 0;
    if (!lua_isnumber(L, 5)) {
        lua_pushfstring(L, "createLabel: bad argument #5 type (label width as number expected, got %s!)", luaL_typename(L, 5));
        return lua_error(L);
    } else {
        width = lua_tonumber(L, 5);
    }
    int height = 0;
    if (!lua_isnumber(L, 6)) {
        lua_pushfstring(L, "createLabel: bad argument #6 type (label height as number expected, got %s!)", luaL_typename(L, 6));
        return lua_error(L);
    } else {
        height = lua_tonumber(L, 6);
    }
    bool fillBackground = false;
    if ((!lua_isnumber(L, 7)) && (!lua_isboolean(L, 7))) {
        lua_pushfstring(L, "createLabel: bad argument #7 type (label fillBackground as boolean/number (0/1) expected, got %s!)", luaL_typename(L, 7));
        return lua_error(L);
    } else {
        if (lua_isboolean(L, 7)) {
            fillBackground = lua_toboolean(L, 7);
        } else {
            fillBackground = (lua_tointeger(L, 7) != 0);
        }
    }
    bool clickthrough = false;
    if (n >= 8) {
        if ((!lua_isnumber(L, 8)) && (!lua_isboolean(L, 8))) {
            lua_pushfstring(L, "createLabel: bad argument #8 type (label clickthrough as boolean/number (0/1) expected, got %s!)", luaL_typename(L, 8));
            return lua_error(L);
        } else {
            if (lua_isboolean(L, 8)) {
                clickthrough = lua_toboolean(L, 8);
            } else {
                clickthrough = (lua_tointeger(L, 8) != 0);
            }
        }
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = mudlet::self()->createLabel(&host, windowName, labelName, x, y, width, height, fillBackground, clickthrough); !success) {
        // We should, perhaps be returning a nil here but the published API
        // says the function returns true or false and we cannot change that now
        lua_pushboolean(L, false);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Internal Function create Label in MainWindow
int TLuaInterpreter::createLabelMainWindow(lua_State* L, const QString& labelName)
{
    QString windowName = QLatin1String("main");
    int n = lua_gettop(L);
    int x = 0;
    if (!lua_isnumber(L, 2)) {
        lua_pushfstring(L, "createLabel: bad argument #2 type (label x-coordinate as number expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else {
        x = lua_tonumber(L, 2);
    }
    int y = 0;
    if (!lua_isnumber(L, 3)) {
        lua_pushfstring(L, "createLabel: bad argument #3 type (label y-coordinate as number expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    } else {
        y = lua_tonumber(L, 3);
    }
    int width = 0;
    if (!lua_isnumber(L, 4)) {
        lua_pushfstring(L, "createLabel: bad argument #4 type (label width as number expected, got %s!)", luaL_typename(L, 4));
        return lua_error(L);
    } else {
        width = lua_tonumber(L, 4);
    }
    int height = 0;
    if (!lua_isnumber(L, 5)) {
        lua_pushfstring(L, "createLabel: bad argument #5 type (label height as number expected, got %s!)", luaL_typename(L, 5));
        return lua_error(L);
    } else {
        height = lua_tonumber(L, 5);
    }
    bool fillBackground = false;
    if ((!lua_isnumber(L, 6)) && (!lua_isboolean(L, 6))) {
        lua_pushfstring(L, "createLabel: bad argument #6 type (label fillBackground as boolean/number (0/1) expected, got %s!)", luaL_typename(L, 6));
        return lua_error(L);
    } else {
        if (lua_isboolean(L, 6)) {
            fillBackground = lua_toboolean(L, 6);
        } else {
            fillBackground = (lua_tointeger(L, 6) != 0);
        }
    }
    bool clickthrough = false;
    if (n >= 7) {
        if ((!lua_isnumber(L, 7)) && (!lua_isboolean(L, 7))) {
            lua_pushfstring(L, "createLabel: bad argument #7 type (label clickthrough as boolean/number (0/1) expected, got %s!)", luaL_typename(L, 7));
            return lua_error(L);
        } else {
            if (lua_isboolean(L, 7)) {
                clickthrough = lua_toboolean(L, 7);
            } else {
                clickthrough = (lua_tointeger(L, 7) != 0);
            }
        }
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = mudlet::self()->createLabel(&host, windowName, labelName, x, y, width, height, fillBackground, clickthrough); !success) {
        // We should, perhaps be returning a nil here but the published API
        // says the function returns true or false and we cannot change that now
        lua_pushboolean(L, false);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::deleteLabel(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "deleteLabel: bad argument #1 type (label name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString labelName{QString::fromUtf8(lua_tostring(L, 1))};
    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.mpConsole->deleteLabel(labelName); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::setLabelToolTip(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setLabelToolTip: bad argument #1 type (label name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "setLabelToolTip: bad argument #2 type (text as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    if ((lua_gettop(L) > 2) && !lua_isnumber(L, 3)) {
        lua_pushfstring(L, "setLabelToolTip: bad argument #3 type (duration as number expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    QString labelName{QString::fromUtf8(lua_tostring(L, 1))};
    QString labelToolTip{QString::fromUtf8(lua_tostring(L, 2))};
    double duration = lua_tonumber(L, 3);
    Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setLabelToolTip(labelName, labelToolTip, duration); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::setLabelCursor(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setLabelCursor: bad argument #1 type (label name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    if (!lua_isnumber(L, 2)) {
        lua_pushfstring(L, "setLabelCursor: bad argument #2 type (cursortype as number expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    QString labelName{QString::fromUtf8(lua_tostring(L, 1))};
    int labelCursor = lua_tonumber(L, 2);
    Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setLabelCursor(labelName, labelCursor); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::setLabelCustomCursor(lua_State* L)
{
    int n = lua_gettop(L);
    int hotX = -1, hotY = -1;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setLabelCustomCursor: bad argument #1 type (label name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "setLabelCustomCursor: bad argument #2 type (custom cursor location as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    if (n > 2) {
        if (!lua_isnumber(L, 3)) {
            lua_pushfstring(L, "setLabelCustomCursor: bad argument #3 type (hot spot x-coordinate as number expected, got %s!)", luaL_typename(L, 3));
            return lua_error(L);
        }
        if (!lua_isnumber(L, 4)) {
            lua_pushfstring(L, "setLabelCustomCursor: bad argument #4 type (hot spot y-coordinate as number expected, got %s!)", luaL_typename(L, 4));
            return lua_error(L);
        }
        hotX = lua_tonumber(L, 3);
        hotY = lua_tonumber(L, 4);
    }

    QString labelName{QString::fromUtf8(lua_tostring(L, 1))};
    QString pixmapLocation{QString::fromUtf8(lua_tostring(L, 2))};
    Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setLabelCustomCursor(labelName, pixmapLocation, hotX, hotY); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createMapper
int TLuaInterpreter::createMapper(lua_State* L)
{
    int n = lua_gettop(L);
    std::string luaSendWindow = "";
    int x, y, width, height, counter;
    counter = 1;

    if (n > 4 && lua_type(L, 1) != LUA_TSTRING) {
        lua_pushfstring(L, "createMapper: bad argument #1 type (parent window name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    if (n > 4 && lua_type(L, 1) == LUA_TSTRING) {
        luaSendWindow = lua_tostring(L, 1);
        counter++;
        if (luaSendWindow == "main") {
            // QString::compare is zero for a match on the "default"
            // case so clear the variable - to flag this as the main
            // window case - as is the case for an empty string
            luaSendWindow.clear();
        }
    }

    if (!lua_isnumber(L, counter)) {
        lua_pushfstring(L, "createMapper: bad argument #%d type (mapper x-coordinate as number expected, got %s!)", counter, luaL_typename(L, counter));
        return lua_error(L);
    } else {
        x = lua_tonumber(L, counter);
        counter++;
    }
    if (!lua_isnumber(L, counter)) {
        lua_pushfstring(L, "createMapper: bad argument #%d type (mapper y-coordinate as number expected, got %s!)", counter, luaL_typename(L, counter));
        return lua_error(L);
    } else {
        y = lua_tonumber(L, counter);
        counter++;
    }
    if (!lua_isnumber(L, counter)) {
        lua_pushfstring(L, "createMapper: bad argument #%d type (mapper width as number expected, got %s!)", counter, luaL_typename(L, counter));
        return lua_error(L);
    } else {
        width = lua_tonumber(L, counter);
        counter++;
    }
    if (!lua_isnumber(L, counter)) {
        lua_pushfstring(L, "createMapper: bad argument #%d type (mapper height as number expected, got %s!)", counter, luaL_typename(L, counter));
        return lua_error(L);
    } else {
        height = lua_tonumber(L, counter);
    }
    Host& host = getHostFromLua(L);
    QString windowname(luaSendWindow.c_str());
    if (auto [success, message] = host.mpConsole->createMapper(windowname, x, y, width, height); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createCommandLine
int TLuaInterpreter::createCommandLine(lua_State* L)
{
    QString commandLineName;
    QString windowName = QLatin1String("main");
    int n = lua_gettop(L);
    int x, y, width, height, counter;
    counter = 1;

    if (n > 5 && lua_type(L, 1) != LUA_TSTRING) {
        lua_pushfstring(L, "createCommandLine: bad argument #1 type (parent window name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    if (n > 5 && lua_type(L, 1) == LUA_TSTRING) {
        windowName = QString::fromUtf8(lua_tostring(L, 1));
        counter++;
        if (windowName == "main") {
            // QString::compare is zero for a match on the "default"
            // case so clear the variable - to flag this as the main
            // window case - as is the case for an empty string
            windowName.clear();
        }
    }

    if (lua_type(L, counter) != LUA_TSTRING) {
        lua_pushfstring(L, "createCommandLine: bad argument #%d type (commandLine name as string expected, got %s!)", counter, luaL_typename(L, counter));
        return lua_error(L);
    } else {
        commandLineName = QString::fromUtf8(lua_tostring(L, counter));
        counter++;
    }

    if (!lua_isnumber(L, counter)) {
        lua_pushfstring(L, "createCommandLine: bad argument #%d type (commandline x-coordinate as number expected, got %s!)", counter, luaL_typename(L, counter));
        return lua_error(L);
    } else {
        x = lua_tonumber(L, counter);
        counter++;
    }
    if (!lua_isnumber(L, counter)) {
        lua_pushfstring(L, "createCommandLine: bad argument #%d type (commandline y-coordinate as number expected, got %s!)", counter, luaL_typename(L, counter));
        return lua_error(L);
    } else {
        y = lua_tonumber(L, counter);
        counter++;
    }
    if (!lua_isnumber(L, counter)) {
        lua_pushfstring(L, "createCommandLine: bad argument #%d type (commandline width as number expected, got %s!)", counter, luaL_typename(L, counter));
        return lua_error(L);
    } else {
        width = lua_tonumber(L, counter);
        counter++;
    }
    if (!lua_isnumber(L, counter)) {
        lua_pushfstring(L, "createCommandLine: bad argument #%d type (commandline height as number expected, got %s!)", counter, luaL_typename(L, counter));
        return lua_error(L);
    } else {
        height = lua_tonumber(L, counter);
        counter++;
    }
    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.mpConsole->createCommandLine(windowName, commandLineName, x, y, width, height); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createBuffer
int TLuaInterpreter::createBuffer(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "createBuffer: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    mudlet::self()->createBuffer(&host, text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearUserWindow
int TLuaInterpreter::clearUserWindow(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        Host& host = getHostFromLua(L);
        host.mpConsole->buffer.clear();
        host.mpConsole->mUpperPane->forceUpdate();
        return 0;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    mudlet::self()->clearWindow(&host, text);

    return 0;
}

// Documentation: ? - public function but should stay undocumented -- compare https://github.com/Mudlet/Mudlet/issues/1149
int TLuaInterpreter::closeUserWindow(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "closeUserWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    mudlet::self()->closeWindow(&host, text);

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hideWindow -- not hideUserWindow - compare initLuaGlobals()
int TLuaInterpreter::hideUserWindow(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "hideWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    mudlet::self()->hideWindow(&host, text);

    return 0;
}

// No documentation available in wiki - internal function
void TLuaInterpreter::setBorderSize(lua_State* L, int size, int position, bool resizeMudlet)
{
    Host& host = getHostFromLua(L);
    switch (position) {
        case Qt::TopSection: host.mBorderTopHeight = size; break;
        case Qt::RightSection: host.mBorderRightWidth = size; break;
        case Qt::BottomSection: host.mBorderBottomHeight = size; break;
        case Qt::LeftSection: host.mBorderLeftWidth = size; break;
    }
    if (resizeMudlet) {
        int x, y;
        x = host.mpConsole->width();
        y = host.mpConsole->height();
        QSize s = QSize(x, y);
        QResizeEvent event(s, s);
        QApplication::sendEvent(host.mpConsole, &event);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderSizes
int TLuaInterpreter::setBorderSizes(lua_State* L)
{
    int numberOfArguments = lua_gettop(L);
    int sizeTop = 0;
    int sizeRight = 0;
    int sizeBottom = 0;
    int sizeLeft = 0;
    switch (numberOfArguments) {
        case 0: break;
        case 1: {
            if (!lua_isnumber(L, 1)) {
                lua_pushfstring(L, "setBorderSizes: bad argument #1 value (new size as number expected, got %s!)", luaL_typename(L, 1));
                return lua_error(L);
            }
            sizeTop = lua_tonumber(L, 1);
            sizeRight = lua_tonumber(L, 1);
            sizeBottom = lua_tonumber(L, 1);
            sizeLeft = lua_tonumber(L, 1);
            break;
        }
        case 2: {
            if (!lua_isnumber(L, 1)) {
                lua_pushfstring(L, "setBorderSizes: bad argument #1 value (new height as number expected, got %s!)", luaL_typename(L, 1));
                return lua_error(L);
            }
            if (!lua_isnumber(L, 2)) {
                lua_pushfstring(L, "setBorderSizes: bad argument #2 value (new width as number expected, got %s!)", luaL_typename(L, 2));
                return lua_error(L);
            }
            sizeTop = lua_tonumber(L, 1);
            sizeRight = lua_tonumber(L, 2);
            sizeBottom = lua_tonumber(L, 1);
            sizeLeft = lua_tonumber(L, 2);
            break;
        }
        case 3: {
            if (!lua_isnumber(L, 1)) {
                lua_pushfstring(L, "setBorderSizes: bad argument #1 value (new top size as number expected, got %s!)", luaL_typename(L, 1));
                return lua_error(L);
            }
            if (!lua_isnumber(L, 2)) {
                lua_pushfstring(L, "setBorderSizes: bad argument #2 value (new width as number expected, got %s!)", luaL_typename(L, 2));
                return lua_error(L);
            }
            if (!lua_isnumber(L, 3)) {
                lua_pushfstring(L, "setBorderSizes: bad argument #3 value (new bottom size as number expected, got %s!)", luaL_typename(L, 3));
                return lua_error(L);
            }
            sizeTop = lua_tonumber(L, 1);
            sizeRight = lua_tonumber(L, 2);
            sizeBottom = lua_tonumber(L, 3);
            sizeLeft = lua_tonumber(L, 2);
            break;
        }
        default: {
            if (!lua_isnumber(L, 1)) {
                lua_pushfstring(L, "setBorderSizes: bad argument #1 value (new top size as number expected, got %s!)", luaL_typename(L, 1));
                return lua_error(L);
            }
            if (!lua_isnumber(L, 2)) {
                lua_pushfstring(L, "setBorderSizes: bad argument #2 value (new right size as number expected, got %s!)", luaL_typename(L, 2));
                return lua_error(L);
            }
            if (!lua_isnumber(L, 3)) {
                lua_pushfstring(L, "setBorderSizes: bad argument #3 value (new bottom size as number expected, got %s!)", luaL_typename(L, 3));
                return lua_error(L);
            }
            if (!lua_isnumber(L, 4)) {
                lua_pushfstring(L, "setBorderSizes: bad argument #4 value (new left size as number expected, got %s!)", luaL_typename(L, 4));
                return lua_error(L);
            }
            sizeTop = lua_tonumber(L, 1);
            sizeRight = lua_tonumber(L, 2);
            sizeBottom = lua_tonumber(L, 3);
            sizeLeft = lua_tonumber(L, 4);
            break;
        }
    }
    setBorderSize(L, sizeTop, Qt::TopSection, false);
    setBorderSize(L, sizeRight, Qt::RightSection, false);
    setBorderSize(L, sizeBottom, Qt::BottomSection, false);
    setBorderSize(L, sizeLeft, Qt::LeftSection, true); // only now send update event to resize Mudlet window
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderTop
int TLuaInterpreter::setBorderTop(lua_State* L)
{
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setBorderTop: bad argument #1 value (new size as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    int size = lua_tonumber(L, 1);
    setBorderSize(L, size, Qt::TopSection);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderRight
int TLuaInterpreter::setBorderRight(lua_State* L)
{
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setBorderRight: bad argument #1 value (new size as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    int size = lua_tonumber(L, 1);
    setBorderSize(L, size, Qt::RightSection);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderBottom
int TLuaInterpreter::setBorderBottom(lua_State* L)
{
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setBorderBottom: bad argument #1 value (new size as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    int size = lua_tonumber(L, 1);
    setBorderSize(L, size, Qt::BottomSection);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderLeft
int TLuaInterpreter::setBorderLeft(lua_State* L)
{
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setBorderLeft: bad argument #1 value (new size as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    int size = lua_tonumber(L, 1);
    setBorderSize(L, size, Qt::LeftSection);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderTop
int TLuaInterpreter::getBorderTop(lua_State* L)
{
    Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.mBorderTopHeight);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderLeft
int TLuaInterpreter::getBorderLeft(lua_State* L)
{
    Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.mBorderLeftWidth);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderBottom
int TLuaInterpreter::getBorderBottom(lua_State* L)
{
    Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.mBorderBottomHeight);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderRight
int TLuaInterpreter::getBorderRight(lua_State* L)
{
    Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.mBorderRightWidth);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBorderSizes
int TLuaInterpreter::getBorderSizes(lua_State* L)
{
    Host& host = getHostFromLua(L);
    lua_createtable(L, 0, 4);
    lua_pushinteger(L, host.mBorderTopHeight);
    lua_setfield(L, -2, "top");
    lua_pushinteger(L, host.mBorderRightWidth);
    lua_setfield(L, -2, "right");
    lua_pushinteger(L, host.mBorderBottomHeight);
    lua_setfield(L, -2, "bottom");
    lua_pushinteger(L, host.mBorderLeftWidth);
    lua_setfield(L, -2, "left");
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resizeWindow -- not resizeUserWindow - compare initLuaGlobals()
int TLuaInterpreter::resizeWindow(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "resizeWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    double x1;
    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "resizeWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        x1 = lua_tonumber(L, 2);
    }
    double y1;
    if (!lua_isnumber(L, 3)) {
        lua_pushstring(L, "resizeWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        y1 = lua_tonumber(L, 3);
    }

    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    mudlet::self()->resizeWindow(&host, text, static_cast<int>(x1), static_cast<int>(y1));

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#moveWindow
int TLuaInterpreter::moveWindow(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "moveWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    double x1;
    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "moveWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        x1 = lua_tonumber(L, 2);
    }
    double y1;
    if (!lua_isnumber(L, 3)) {
        lua_pushstring(L, "moveWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        y1 = lua_tonumber(L, 3);
    }

    Host& host = getHostFromLua(L);

    QString text(luaSendText.c_str());
    mudlet::self()->moveWindow(&host, text, static_cast<int>(x1), static_cast<int>(y1));
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setWindow
int TLuaInterpreter::setWindow(lua_State* L)
{
    QString windowname;
    QString name;
    int n = lua_gettop(L);
    int x = 0, y = 0;
    bool show = true;

    if (lua_type(L, 1) != LUA_TSTRING) {
        lua_pushfstring(L, "setWindow: bad argument #1 type (parent windowname as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        windowname = QString::fromUtf8(lua_tostring(L, 1));
    }

    if (lua_type(L, 2) != LUA_TSTRING) {
        lua_pushfstring(L, "setWindow: bad argument #2 type (element name as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else {
        name = QString::fromUtf8(lua_tostring(L, 2));
    }
    if (n > 2) {
        if (!lua_isnumber(L, 3)) {
            lua_pushfstring(L, "setWindow: bad argument #3 type (x-coordinate as number expected, got %s!)", luaL_typename(L, 3));
            return lua_error(L);

        } else {
            x = lua_tonumber(L, 3);
        }
        if (!lua_isnumber(L, 4)) {
            lua_pushfstring(L, "setWindow: bad argument #4 type (y-coordinate as number expected, got %s!)", luaL_typename(L, 4));
            return lua_error(L);
        } else {
            y = lua_tonumber(L, 4);
        }
        if (!lua_isboolean(L, 5)) {
            lua_pushfstring(L, "setWindow: bad argument #5 type (show element as boolean expected, got %s!)", luaL_typename(L, 5));
            return lua_error(L);
        } else {
            show = lua_toboolean(L, 5);
        }
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = mudlet::self()->setWindow(&host, windowname, name, x, y, show); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }
    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::openMapWidget(lua_State* L)
{
    int n = lua_gettop(L);
    QString area = QString();
    int x = -1, y = -1, width = -1, height = -1;
    if (n == 1) {
        if (lua_type(L, 1) != LUA_TSTRING) {
            lua_pushfstring(L, "openMapWidget: bad argument #1 type (area as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            area = QString::fromUtf8(lua_tostring(L, 1));
        }
    }
    if (n > 1) {
        area = QStringLiteral("f");
        if (!lua_isnumber(L, 1)) {
            lua_pushfstring(L, "openMapWidget: bad argument #1 type (x-coordinate as number expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            x = lua_tonumber(L, 1);
        }
        if (!lua_isnumber(L, 2)) {
            lua_pushfstring(L, "openMapWidget: bad argument #2 type (y-coordinate as number expected, got %s!)", luaL_typename(L, 2));
            return lua_error(L);
        } else {
            y = lua_tonumber(L, 2);
        }
    }
    if (n > 2) {
        if (!lua_isnumber(L, 3)) {
            lua_pushfstring(L, "openMapWidget: bad argument #3 type (width as number expected, got %s!)", luaL_typename(L, 3));
            return lua_error(L);
        } else {
            width = lua_tonumber(L, 3);
        }
        if (!lua_isnumber(L, 4)) {
            lua_pushfstring(L, "openMapWidget: bad argument #4 type (height as number expected, got %s!)", luaL_typename(L, 4));
            return lua_error(L);
        } else {
            height = lua_tonumber(L, 4);
        }
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = mudlet::self()->openMapWidget(&host, area.toLower(), x, y, width, height); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }
    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::closeMapWidget(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (auto [success, message] = mudlet::self()->closeMapWidget(&host); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMainWindowSize
int TLuaInterpreter::setMainWindowSize(lua_State* L)
{
    int x1;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "setMainWindowSize: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        x1 = lua_tonumber(L, 1);
    }
    int y1;
    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "setMainWindowSize: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        y1 = lua_tonumber(L, 2);
    }

    mudlet::self()->resize(x1, y1);

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBackgroundColor
int TLuaInterpreter::setBackgroundColor(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    QString windowName;
    int r, g, b, alpha;

    auto validRange = [](int number) {
        return number >= 0 and number <= 255;
    };

    int s = 1;
    if (lua_isstring(L, s) && !lua_isnumber(L, s)) {
        windowName = QString::fromUtf8(lua_tostring(L, s));

        if (!lua_isnumber(L, ++s)) {
            lua_pushfstring(L, "setBackgroundColor: bad argument #%d type (red value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            r = static_cast<int>(lua_tonumber(L, s));

            if (!validRange(r)) {
                lua_pushnil(L);
                lua_pushfstring(L, "setBackgroundColor: bad argument #%d value (red value needs to be between 0-255, got %d!)", s, r);
                return 2;
            }
        }
    } else if (lua_isnumber(L, s)) {
        r = static_cast<int>(lua_tonumber(L, s));

        if (!validRange(r)) {
            lua_pushnil(L);
            lua_pushfstring(L, "setBackgroundColor: bad argument #%d value (red value needs to be between 0-255, got %d!)", s, r);
            return 2;
        }
    } else {
        lua_pushfstring(L, "setBackgroundColor: bad argument #%d type (window name as string, or red value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "setBackgroundColor: bad argument #%d type (green value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        g = static_cast<int>(lua_tonumber(L, s));

        if (!validRange(g)) {
            lua_pushnil(L);
            lua_pushfstring(L, "setBackgroundColor: bad argument #%d value (green value needs to be between 0-255, got %d!)", s, g);
            return 2;
        }
    }

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "setBackgroundColor: bad argument #%d type (blue value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        b = static_cast<int>(lua_tonumber(L, s));

        if (!validRange(b)) {
            lua_pushnil(L);
            lua_pushfstring(L, "setBackgroundColor: bad argument #%d value (blue value needs to be between 0-255, got %d!)", s, b);
            return 2;
        }
    }

    // if we get nothing for the alpha value, assume it is 255. If we get a non-number value, complain.
    if (lua_gettop(L) <= s) {
        alpha = 255;
    } else if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "setBackgroundColor: bad argument #%d type (optional alpha value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        alpha = static_cast<int>(lua_tonumber(L, s));

        if (!validRange(alpha)) {
            lua_pushnil(L);
            lua_pushfstring(L, "setBackgroundColor: bad argument #%d value (alpha value needs to be between 0-255, got %d!)", s, alpha);
            return 2;
        }
    }

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        if (mudlet::self()->mConsoleMap.contains(pHost)) {
            pHost->mBgColor.setRgb(r, g, b);
            pHost->mpConsole->setConsoleBgColor(r, g, b);
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "could not find the main window");
            return 2;
        }
    } else if (!mudlet::self()->setBackgroundColor(pHost, windowName, r, g, b, alpha)) {
        lua_pushnil(L);
        lua_pushfstring(L, R"(window "%s" not found)", windowName.toUtf8().constData());
        return 2;
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#calcFontSize
int TLuaInterpreter::calcFontSize(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);

    QString windowName = QStringLiteral("main");
    QSize size;

    // font name and size are passed in as arguments
    if (lua_gettop(L) == 2) {
        if (!lua_isnumber(L, 1)) {
            lua_pushfstring(L, R"(calcFontSize: bad argument #1 (font size as number expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        }

        if (!lua_isstring(L, 2)) {
            lua_pushfstring(L, R"(calcFontSize: bad argument #2 (font name as string expected, got %s!)", luaL_typename(L, 2));
            return lua_error(L);
        }

        auto font = QFont(QString::fromUtf8(lua_tostring(L, 2)), static_cast<int> (lua_tonumber(L, 1)), QFont::Normal);
        auto fontMetrics = QFontMetrics(font);
        size = QSize(fontMetrics.averageCharWidth(), fontMetrics.height());

        lua_pushnumber(L, size.width());
        lua_pushnumber(L, size.height());
        return 2;
    }

    // only either window name or font size is passed in
    if (lua_gettop(L) == 1 && lua_isnumber(L, 1)) {
        auto fontSize = lua_tonumber(L, 1);
        auto font = QFont(QStringLiteral("Bitstream Vera Sans Mono"), fontSize, QFont::Normal);

        auto fontMetrics = QFontMetrics(font);
        size = QSize(fontMetrics.averageCharWidth(), fontMetrics.height());
    } else if (lua_gettop(L) && !lua_isstring(L, 1)) {
        lua_pushfstring(L, "calcFontSize: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        windowName = QString::fromUtf8(lua_tostring(L, 1));
        size = mudlet::self()->calcFontSize(pHost, windowName);
    }

    if (size.width() <= -1) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, size.width());
    lua_pushnumber(L, size.height());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#startLogging
int TLuaInterpreter::startLogging(lua_State* L)
{
    Host& host = getHostFromLua(L);

    bool logOn = true;
    if (!lua_isboolean(L, 1)) {
        lua_pushfstring(L, "startLogging: bad argument #1 type (turn logging on/off, as boolean expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        logOn = lua_toboolean(L, 1);
    }

    QString savedLogFileName;
    if (host.mpConsole->mLogToLogFile) {
        savedLogFileName = host.mpConsole->mLogFileName;
        // Don't assume we will be able to find the file name once recording has
        // stopped.
    }

    if (host.mpConsole->mLogToLogFile != logOn) {
        host.mpConsole->toggleLogging(false);
        // Changes state of host.mpConsole->mLogToLogFile, but that can't be
        // really be called a side-effect!

        lua_pushboolean(L, true);
        if (host.mpConsole->mLogToLogFile) {
            host.mpConsole->logButton->setChecked(true);
            // Sets the button as checked but clicked() & pressed() signals are NOT generated
            lua_pushfstring(L, "Main console output has started to be logged to file: %s.", host.mpConsole->mLogFileName.toUtf8().constData());
            lua_pushstring(L, host.mpConsole->mLogFileName.toUtf8().constData());
            lua_pushnumber(L, 1);
        } else {
            host.mpConsole->logButton->setChecked(false);
            lua_pushfstring(L, "Main console output has stopped being logged to file: %s.", savedLogFileName.toUtf8().constData());
            lua_pushstring(L, host.mpConsole->mLogFileName.toUtf8().constData());
            lua_pushnumber(L, 0);
        }

    } else {
        lua_pushnil(L);
        if (host.mpConsole->mLogToLogFile) {
            lua_pushfstring(L, "Main console output is already being logged to file: %s.", host.mpConsole->mLogFileName.toUtf8().constData());
            lua_pushstring(L, host.mpConsole->mLogFileName.toUtf8().constData());
            lua_pushnumber(L, -1);
        } else {
            lua_pushstring(L, "Main console output was already not being logged to a file.");
            lua_pushnil(L);
            lua_pushnumber(L, -2);
        }
    }
    return 4;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBackgroundImage
int TLuaInterpreter::setBackgroundImage(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "setBackgroundImage: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    std::string luaName = "";
    if (!lua_isstring(L, 2)) {
        lua_pushstring(L, "setBackgroundImage: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaName = lua_tostring(L, 2);
    }

    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    QString name(luaName.c_str());
    mudlet::self()->setBackgroundImage(&host, text, name);

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getImageSize
int TLuaInterpreter::getImageSize(lua_State* L)
{
    QString imageLocation;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "getImageSize: bad argument #1 type (image location as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        imageLocation = QString::fromUtf8(lua_tostring(L, 1));
        if (imageLocation.isEmpty()) {
            lua_pushnil(L);
            lua_pushstring(L, "bad argument #1 value (image location cannot be an empty string)");
            return 2;
        }
    }

    if (auto size = mudlet::self()->getImageSize(imageLocation)) {
        lua_pushnumber(L, size->width());
        lua_pushnumber(L, size->height());
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "couldn't retrieve image size, is the location '%s' correct?", imageLocation.toUtf8().constData());
    }
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCmdLineAction
int TLuaInterpreter::setCmdLineAction(lua_State* L){
    Host& host = getHostFromLua(L);

    QString name;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setCmdLineAction: bad argument #1 type (command line name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        name = QString::fromUtf8(lua_tostring(L, 1));
        if (name.isEmpty()) {
            lua_pushnil(L);
            lua_pushfstring(L, "setCmdAction: bad argument #1 value (command line name cannot be an empty string.)");
            return 2;
        }
        lua_remove(L, 1);
    }

    int func;
    if (!lua_isfunction(L, 1)) {
        lua_pushfstring(L, "setCmdLineAction: bad argument #2 type (function expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    func = luaL_ref(L, LUA_REGISTRYINDEX);
    bool lua_result = false;
    lua_result = mudlet::self()->setCmdLineAction(&host, name, func);

    if (lua_result) {
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, R"("setCmdLineAction": bad argument #1 value (command line name "%s" not found.))", name.toUtf8().constData());
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetCmdLineAction
int TLuaInterpreter::resetCmdLineAction(lua_State* L){
    Host& host = getHostFromLua(L);

    QString name;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "resetCmdLineAction: bad argument #1 type (command line name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        name = QString::fromUtf8(lua_tostring(L, 1));
        if (name.isEmpty()) {
            lua_pushnil(L);
            lua_pushfstring(L, "resetCmdAction: bad argument #1 value (command line name cannot be an empty string.)");
            return 2;
        }
    }

    bool lua_result = false;
    lua_result = mudlet::self()->resetCmdLineAction(&host, name);
    if (lua_result) {
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, R"("resetCmdLineAction": bad argument #1 value (command line name "%s" not found.))", name.toUtf8().constData());
        return 2;
    }
}

// No documentation available in wiki - internal function
int TLuaInterpreter::setLabelCallback(lua_State* L, const QString& funcName)
{
    Host& host = getHostFromLua(L);

    QString labelName;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "%s: bad argument #1 type (label name as string expected, got %s!)", funcName.toUtf8().constData(), luaL_typename(L, 1));
        return lua_error(L);
    } else {
        labelName = QString::fromUtf8(lua_tostring(L, 1));
        if (labelName.isEmpty()) {
            lua_pushnil(L);
            lua_pushfstring(L, "%s: bad argument #1 value (label name cannot be an empty string.)", funcName.toUtf8().constData());
            return 2;
        }
        lua_remove(L, 1);
    }

    int func;
    if (!lua_isfunction(L, 1)) {
        lua_pushfstring(L, "%s: bad argument #2 type (function expected, got %s!)", funcName.toUtf8().constData(), luaL_typename(L, 1));
        return lua_error(L);
    }
    func = luaL_ref(L, LUA_REGISTRYINDEX);

    bool lua_result = false;
    if (funcName == QStringLiteral("setLabelClickCallback"))
        lua_result = mudlet::self()->setLabelClickCallback(&host, labelName, func);
    else if (funcName == QStringLiteral("setLabelDoubleClickCallback"))
        lua_result = mudlet::self()->setLabelDoubleClickCallback(&host, labelName, func);
    else if (funcName == QStringLiteral("setLabelReleaseCallback"))
        lua_result = mudlet::self()->setLabelReleaseCallback(&host, labelName, func);
    else if (funcName == QStringLiteral("setLabelMoveCallback"))
        lua_result = mudlet::self()->setLabelMoveCallback(&host, labelName, func);
    else if (funcName == QStringLiteral("setLabelWheelCallback"))
        lua_result = mudlet::self()->setLabelWheelCallback(&host, labelName, func);
    else if (funcName == QStringLiteral("setLabelOnEnter"))
        lua_result = mudlet::self()->setLabelOnEnter(&host, labelName, func);
    else if (funcName == QStringLiteral("setLabelOnLeave"))
        lua_result = mudlet::self()->setLabelOnLeave(&host, labelName, func);
    else {
        lua_pushnil(L);
        lua_pushfstring(L, R"("%s" is not a known function name - bug in Mudlet, please report it)", funcName.toUtf8().constData());
        return 2;
    }

    if (lua_result) {
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, R"("%s": bad argument #1 value (label name "%s" not found.))", funcName.toUtf8().constData(), labelName.toUtf8().constData());
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelClickCallback
int TLuaInterpreter::setLabelClickCallback(lua_State* L)
{
    return setLabelCallback(L, QStringLiteral("setLabelClickCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelDoubleClickCallback
int TLuaInterpreter::setLabelDoubleClickCallback(lua_State* L)
{
    return setLabelCallback(L, QStringLiteral("setLabelDoubleClickCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelReleaseCallback
int TLuaInterpreter::setLabelReleaseCallback(lua_State* L)
{
    return setLabelCallback(L, QStringLiteral("setLabelReleaseCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelMoveCallback
int TLuaInterpreter::setLabelMoveCallback(lua_State* L)
{
    return setLabelCallback(L, QStringLiteral("setLabelMoveCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelWheelCallback
int TLuaInterpreter::setLabelWheelCallback(lua_State* L)
{
    return setLabelCallback(L, QStringLiteral("setLabelWheelCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelOnEnter
int TLuaInterpreter::setLabelOnEnter(lua_State* L)
{
    return setLabelCallback(L, QStringLiteral("setLabelOnEnter"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelOnLeave
int TLuaInterpreter::setLabelOnLeave(lua_State* L)
{
    return setLabelCallback(L, QStringLiteral("setLabelOnLeave"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTextFormat
int TLuaInterpreter::setTextFormat(lua_State* L)
{
    Host& host = getHostFromLua(L);

    int n = lua_gettop(L);
    int s = 0;

    QString windowName;
    if (!lua_isstring(L, ++s)) {
        lua_pushfstring(L,
                        "setTextFormat: bad argument #%d type (window name as string is optional, got %s!)",
                        s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        windowName = QString::fromUtf8(lua_tostring(L, s));
    }

    QVector<int> colorComponents(6); // 0-2 RGB background, 3-5 RGB foreground
    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L,
                        "setTextFormat: bad argument #%d type (red background color component as number\n"
                        "expected, got %s!)",
                        s,
                        luaL_typename(L, s));
        return lua_error(L);
    } else {
        colorComponents[0] = qRound(qBound(0.0, lua_tonumber(L, s), 255.0));
    }

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L,
                        "setTextFormat: bad argument #%d type (green background color component as number\n"
                        "expected, got %s!)",
                        s,
                        luaL_typename(L, s));
        return lua_error(L);
    } else {
        colorComponents[1] = qRound(qBound(0.0, lua_tonumber(L, s), 255.0));
    }

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L,
                        "setTextFormat: bad argument #%d type (blue background color component as number\n"
                        "expected, got %s!)",
                        s,
                        luaL_typename(L, s));
        return lua_error(L);
    } else {
        colorComponents[2] = qRound(qBound(0.0, lua_tonumber(L, s), 255.0));
    }

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L,
                        "setTextFormat: bad argument #%d type (red foreground color component as number\n"
                        "expected, got %s!)",
                        s,
                        luaL_typename(L, s));
        return lua_error(L);
    } else {
        colorComponents[3] = qRound(qBound(0.0, lua_tonumber(L, s), 255.0));
    }

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L,
                        "setTextFormat: bad argument #%d type (green foreground color component as number\n"
                        "expected, got %s!)",
                        s,
                        luaL_typename(L, s));
        return lua_error(L);
    } else {
        colorComponents[4] = qRound(qBound(0.0, lua_tonumber(L, s), 255.0));
    }

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "setTextFormat: bad argument #%d type (blue foreground color component as number\n"
                           "expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        colorComponents[5] = qRound(qBound(0.0, lua_tonumber(L, s), 255.0));
    }

    bool bold;
    if (lua_isboolean(L, ++s)) {
        bold = lua_toboolean(L, s);
    } else if (lua_isnumber(L, s)) {
        bold = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
    } else {
        lua_pushfstring(L, "setTextFormat: bad argument #%d type (bold format as boolean or number {true/non-zero to enable} expected, got %s!)",
                        s, luaL_typename(L, s));
        return lua_error(L);
    }

    bool underline;
    if (lua_isboolean(L, ++s)) {
        underline = lua_toboolean(L, s);
    } else if (lua_isnumber(L, s)) {
        underline = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
    } else {
        lua_pushfstring(L, "setTextFormat: bad argument #%d type (underline format as boolean or number {true/non-zero to enable} expected, got %s!)",
                        s, luaL_typename(L, s));
        return lua_error(L);
    }

    bool italics;
    if (lua_isboolean(L, ++s)) {
        italics = lua_toboolean(L, s);
    } else if (lua_isnumber(L, s)) {
        italics = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
    } else {
        lua_pushfstring(L, "setTextFormat: bad argument #%d type (italic format as boolean or number {true/non-zero to enable} expected, got %s!)",
                        s, luaL_typename(L, s));
        return lua_error(L);
    }

    bool strikeout = false;
    if (s < n) {
        // s has not been incremented yet so this means we still have another argument!

        if (lua_isboolean(L, ++s)) {
            strikeout = lua_toboolean(L, s);
        } else if (lua_isnumber(L, s)) {
            strikeout = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
        } else {
            lua_pushfstring(L, "setTextFormat: bad argument #%d type (strikeout format as boolean or number {true/non-zero to enable} is optional, got %s!)",
                            s, luaL_typename(L, s));
            return lua_error(L);
        }
    }

    bool overline = false;
    if (s < n) {
        // s has not been incremented yet so this means we still have another argument!
        if (lua_isboolean(L, ++s)) {
            overline = lua_toboolean(L, s);
        } else if (lua_isnumber(L, s)) {
            overline = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
        } else {
            lua_pushfstring(L, "setTextFormat: bad argument #%d type (overline format as boolean or number {true/non-zero to enable} is optional, got %s!)",
                            s, luaL_typename(L, s));
            return lua_error(L);
        }
    }

    bool reverse = false;
    if (s < n) {
        // s has not been incremented yet so this means we still have another argument!
        if (lua_isboolean(L, ++s)) {
            reverse = lua_toboolean(L, s);
        } else if (lua_isnumber(L, s)) {
            reverse = !qFuzzyCompare(1.0, 1.0 + lua_tonumber(L, s));
        } else {
            lua_pushfstring(L, "setTextFormat: bad argument #%d type (reverse format as boolean or number {true/non-zero to enable} is optional, got %s!)",
                            s, luaL_typename(L, s));
            return lua_error(L);
        }
    }

    TChar::AttributeFlags flags = (bold ? TChar::Bold : TChar::None)
            | (italics ? TChar::Italic : TChar::None)
            | (overline ? TChar::Overline : TChar::None)
            | (reverse ? TChar::Reverse : TChar::None)
            | (strikeout ? TChar::StrikeOut : TChar::None)
            | (underline ? TChar::Underline : TChar::None);

    if (!host.mpConsole->setTextFormat(windowName,
                                      QColor(colorComponents.at(3), colorComponents.at(4), colorComponents.at(5)),
                                      QColor(colorComponents.at(0), colorComponents.at(1), colorComponents.at(2)),
                                      flags)) {
        lua_pushboolean(L, false);
        lua_pushfstring(L, "window \"%s\" does not exist", windowName.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#raiseWindow
int TLuaInterpreter::raiseWindow(lua_State* L)
{
    QString windowName;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "raiseWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        windowName = QString::fromUtf8(lua_tostring(L, 1));
    }
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpConsole->raiseWindow(windowName));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#lowerWindow
int TLuaInterpreter::lowerWindow(lua_State* L)
{
    QString windowName;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "lowerWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        windowName = QString::fromUtf8(lua_tostring(L, 1));
    }
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpConsole->lowerWindow(windowName));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#showWindow -- not showUserWindow - compare initLuaGlobals()
int TLuaInterpreter::showUserWindow(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "showWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    lua_pushboolean(L, mudlet::self()->showWindow(&host, text));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomEnv
int TLuaInterpreter::setRoomEnv(lua_State* L)
{
    int id, env;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setRoomEnv: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        id = lua_tonumber(L, 1);
    }
    if (!lua_isnumber(L, 2)) {
        lua_pushfstring(L, "setRoomEnv: bad argument #2 type (environment id as number expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else {
        env = lua_tonumber(L, 2);
    }
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->environment = env;
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "room %d doesn't exist", id);
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomName
int TLuaInterpreter::setRoomName(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "setRoomName: no map present or loaded!");
        return 2;
    }

    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setRoomName: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        id = lua_tonumber(L, 1);
    }

    QString name;
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "setRoomName: bad argument #2 type (room name as string expected, got %s!)", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        name = QString::fromUtf8(lua_tostring(L, 2));
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->name = name;
        updateMap(L);
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "setRoomName: bad argument #1 value (number %d is not a valid room id).", id);
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomName
int TLuaInterpreter::getRoomName(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getRoomName: no map present or loaded!");
        return 2;
    }

    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getRoomName: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        id = lua_tonumber(L, 1);
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        lua_pushstring(L, pR->name.toUtf8().constData());
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "room %d doesn't exist", id);
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomWeight
int TLuaInterpreter::setRoomWeight(lua_State* L)
{
    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "setRoomWeight: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tonumber(L, 1);
    }
    int w;
    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "setRoomWeight: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        w = lua_tonumber(L, 2);
    }
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->setWeight(w);
        host.mpMap->mMapGraphNeedsUpdate = true;
    }

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#connectToServer
int TLuaInterpreter::connectToServer(lua_State* L)
{
    // The lua_tointeger(...) call can return a 64-bit integer number, on
    // Windows Platform that is bigger than the int32_t type (a.k.a. "int" AND
    // "long" types on that platform)! 8-O
    lua_Integer port = 23;
    std::string url;
    bool isToSaveToProfile = false;

    Host& host = getHostFromLua(L);

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "connectToServer: bad argument #1 type (url as string expected, got %s!)", lua_typename(L, 1));
        return lua_error(L);
    } else {
        url = lua_tostring(L, 1);
    }

    if (!lua_isnoneornil(L, 2)) {
        if (!lua_isnumber(L, 2)) {
            lua_pushfstring(L, "connectToServer: bad argument #2 type (port number as number is optional {default = 23}, got %s!)", lua_typename(L, 2));
            return lua_error(L);
        } else {
            port = lua_tointeger(L, 2);
            if (port > 65535 || port < 1) {
                lua_pushnil(L);
                lua_pushfstring(L, "invalid port number %d given, if supplied it must be in range 1 to 65535, {defaults to 23 if not provided}", port);
                return 2;
            }
        }
    }

    // Optional argument to save this new connection to disk for this profile.
    if (!lua_isnoneornil(L, 3)) {
        if (!lua_isboolean(L, 3)) {
            lua_pushfstring(L, "connectToServer: bad argument #3 type (save host name and port number as boolean expected, got %1!)", lua_typename(L, 3));
            return lua_error(L);
        } else {
            isToSaveToProfile = lua_toboolean(L, 3);
        }
    }

    if (isToSaveToProfile) {
        QPair<bool, QString> result = host.writeProfileData(QLatin1String("url"), url.c_str());
        if (!result.first) {
            lua_pushnil(L);
            lua_pushfstring(L, "unable to save host name, reason: %s", result.second.toUtf8().constData());
            return 2;
        }

        result = host.writeProfileData(QLatin1String("url"), QString::number(port));
        if (!result.first) {
            lua_pushnil(L);
            lua_pushfstring(L, "unable to save port number, reason: %s", result.second.toUtf8().constData());
            return 2;
        }
    }

    host.mTelnet.connectIt(url.c_str(), port);

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomIDbyHash
int TLuaInterpreter::setRoomIDbyHash(lua_State* L)
{
    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setRoomIDbyHash: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    id = lua_tonumber(L, 1);
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "setRoomIDbyHash: bad argument #2 type (hash as string expected, got %s)", luaL_typename(L, 2));
        return lua_error(L);
    }
    QString hash = QString::fromUtf8(lua_tostring(L, 2));
    Host& host = getHostFromLua(L);
    if (host.mpMap->mpRoomDB->roomIDToHash.contains(id)) {
        host.mpMap->mpRoomDB->hashToRoomID.remove(host.mpMap->mpRoomDB->roomIDToHash[id]);
    }
    if (host.mpMap->mpRoomDB->hashToRoomID.contains(hash)) {
        host.mpMap->mpRoomDB->roomIDToHash.remove(host.mpMap->mpRoomDB->hashToRoomID[hash]);
    }
    host.mpMap->mpRoomDB->hashToRoomID[hash] = id;
    host.mpMap->mpRoomDB->roomIDToHash[id] = hash;
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomIDbyHash
int TLuaInterpreter::getRoomIDbyHash(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "getRoomIDbyHash: bad argument #1 type (hash as string expected, got %s)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString hash = QString::fromUtf8(lua_tostring(L, 1));
    Host& host = getHostFromLua(L);
    int retID = host.mpMap->mpRoomDB->hashToRoomID.value(hash, -1);
    lua_pushnumber(L, retID);

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomHashByID
int TLuaInterpreter::getRoomHashByID(lua_State* L)
{
    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getRoomHashByID: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    id = lua_tonumber(L, 1);

    Host& host = getHostFromLua(L);
    if (host.mpMap->mpRoomDB->roomIDToHash.contains(id)) {
        QString retHash = host.mpMap->mpRoomDB->roomIDToHash[id];
        lua_pushstring(L, retHash.toUtf8().constData());
        return 1;
    }
    lua_pushnil(L);
    lua_pushfstring(L, "no hash for room %d", id);
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#roomLocked
int TLuaInterpreter::roomLocked(lua_State* L)
{
    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "roomLocked: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tonumber(L, 1);
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        bool r = pR->isLocked;
        lua_pushboolean(L, r);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#lockRoom
int TLuaInterpreter::lockRoom(lua_State* L)
{
    bool b = true;
    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "lockRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tonumber(L, 1);
    }

    if (!lua_isboolean(L, 2)) {
        lua_pushstring(L, "lockRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        b = lua_toboolean(L, 2);
    }
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->isLocked = b;
        host.mpMap->mMapGraphNeedsUpdate = true;
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#lockExit
int TLuaInterpreter::lockExit(lua_State* L)
{
    bool b = true;
    int id;
    int dir;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "lockExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tonumber(L, 1);
    }

    dir = dirToNumber(L, 2);
    if (!dir) {
        lua_pushstring(L, "lockExit: wrong argument type");
        lua_error(L);
        return 1;
    }

    if (!lua_isboolean(L, 3)) {
        lua_pushstring(L, "lockExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        b = lua_toboolean(L, 3);
    }
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->setExitLock(dir, b);
        host.mpMap->mMapGraphNeedsUpdate = true;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#lockSpecialExit
int TLuaInterpreter::lockSpecialExit(lua_State* L)
{
    bool b = true;
    int id, to;
    std::string dir;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "lockSpecialExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tonumber(L, 1);
    }

    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "lockSpecialExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        to = lua_tonumber(L, 2);
    }

    if (!lua_isstring(L, 3)) {
        lua_pushstring(L, "lockSpecialExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        dir = lua_tostring(L, 3);
    }

    if (!lua_isboolean(L, 4)) {
        lua_pushstring(L, "lockSpecialExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        b = lua_toboolean(L, 4);
    }
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        QString _dir = dir.c_str();
        pR->setSpecialExitLock(to, _dir, b);
        host.mpMap->mMapGraphNeedsUpdate = true;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hasSpecialExitLock
int TLuaInterpreter::hasSpecialExitLock(lua_State* L)
{
    int id, to;
    std::string dir;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "hasSpecialExitLock: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tonumber(L, 1);
    }


    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "hasSpecialExitLock: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        to = lua_tonumber(L, 2);
    }
    if (!lua_isstring(L, 3)) {
        lua_pushstring(L, "hasSpecialExitLock: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        dir = lua_tostring(L, 3);
    }
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        QString _dir = dir.c_str();
        lua_pushboolean(L, pR->hasSpecialExitLock(to, _dir));
        return 1;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hasExitLock
int TLuaInterpreter::hasExitLock(lua_State* L)
{
    int id;
    int dir;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "hasExitLock: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tonumber(L, 1);
    }

    dir = dirToNumber(L, 2);
    if (!dir) {
        lua_pushstring(L, "hasExitLock: wrong argument type");
        lua_error(L);
        return 1;
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        lua_pushboolean(L, pR->hasExitLock(dir));
        return 1;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomExits
int TLuaInterpreter::getRoomExits(lua_State* L)
{
    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "getRoomExits: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tonumber(L, 1);
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        lua_newtable(L);
        if (pR->getNorth() != -1) {
            lua_pushstring(L, "north");
            lua_pushnumber(L, pR->getNorth());
            lua_settable(L, -3);
        }
        if (pR->getNorthwest() != -1) {
            lua_pushstring(L, "northwest");
            lua_pushnumber(L, pR->getNorthwest());
            lua_settable(L, -3);
        }
        if (pR->getNortheast() != -1) {
            lua_pushstring(L, "northeast");
            lua_pushnumber(L, pR->getNortheast());
            lua_settable(L, -3);
        }
        if (pR->getSouth() != -1) {
            lua_pushstring(L, "south");
            lua_pushnumber(L, pR->getSouth());
            lua_settable(L, -3);
        }
        if (pR->getSouthwest() != -1) {
            lua_pushstring(L, "southwest");
            lua_pushnumber(L, pR->getSouthwest());
            lua_settable(L, -3);
        }
        if (pR->getSoutheast() != -1) {
            lua_pushstring(L, "southeast");
            lua_pushnumber(L, pR->getSoutheast());
            lua_settable(L, -3);
        }
        if (pR->getWest() != -1) {
            lua_pushstring(L, "west");
            lua_pushnumber(L, pR->getWest());
            lua_settable(L, -3);
        }
        if (pR->getEast() != -1) {
            lua_pushstring(L, "east");
            lua_pushnumber(L, pR->getEast());
            lua_settable(L, -3);
        }
        if (pR->getUp() != -1) {
            lua_pushstring(L, "up");
            lua_pushnumber(L, pR->getUp());
            lua_settable(L, -3);
        }
        if (pR->getDown() != -1) {
            lua_pushstring(L, "down");
            lua_pushnumber(L, pR->getDown());
            lua_settable(L, -3);
        }
        if (pR->getIn() != -1) {
            lua_pushstring(L, "in");
            lua_pushnumber(L, pR->getIn());
            lua_settable(L, -3);
        }
        if (pR->getOut() != -1) {
            lua_pushstring(L, "out");
            lua_pushnumber(L, pR->getOut());
            lua_settable(L, -3);
        }
        return 1;
    } else {
        return 0;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAllRoomEntrances
int TLuaInterpreter::getAllRoomEntrances(lua_State* L)
{
    int roomId = 0;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getAllRoomEntrances: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        roomId = lua_tonumber(L, 1);
    }

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getAllRoomEntrances: no map present or loaded!");
        return 2;
    } else {
        TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
        if (!pR) {
            lua_pushnil(L);
            lua_pushfstring(L, "getAllRoomEntrances: bad argument #1 value (number %d is not a valid room id).", roomId);
            return 2;
        }
        lua_newtable(L);
        QList<int> entrances = host.mpMap->mpRoomDB->getEntranceHash().values(roomId);
        // Could use a .toSet().toList() to remove duplicates values
        if (entrances.count() > 1) {
            std::sort(entrances.begin(), entrances.end());
        }
        for (uint i = 0; i < entrances.size(); i++) {
            lua_pushnumber(L, i + 1);
            lua_pushnumber(L, entrances.at(i));
            lua_settable(L, -3);
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#searchRoom
int TLuaInterpreter::searchRoom(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "searchRoom: no map present or loaded!");
        return 2;
    }

    int room_id = 0;
    int n = lua_gettop(L);
    bool gotRoomID = false;
    bool caseSensitive = false;
    bool exactMatch = false;
    QString room;

    if (lua_isnumber(L, 1)) {
        room_id = lua_tointeger(L, 1);
        gotRoomID = true;
    } else if (lua_isstring(L, 1)) {
        if (n > 1) {
            if (lua_isboolean(L, 2)) {
                caseSensitive = lua_toboolean(L, 2);
                if (n > 2) {
                    if (lua_isboolean(L, 3)) {
                        exactMatch = lua_toboolean(L, 3);
                    } else {
                        lua_pushfstring(L, R"(searchRoom: bad argument #3 type ("exact match" as boolean is optional, got %s!))", luaL_typename(L, 3));
                        lua_error(L);
                        return 1;
                    }
                }
            } else {
                lua_pushfstring(L, R"(searchRoom: bad argument #2 type ("case sensitive" as boolean is optional, got %s!))", luaL_typename(L, 2));
                lua_error(L);
                return 1;
            }
        }
        room = QString::fromUtf8(lua_tostring(L, 1));
    } else {
        lua_pushfstring(L, R"(searchRoom: bad argument #1 ("room name" as string expected, got %s!))", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    }

    if (gotRoomID) {
        TRoom* pR = host.mpMap->mpRoomDB->getRoom(room_id);
        if (pR) {
            lua_pushstring(L, pR->name.toUtf8().constData());
            return 1;
        } else {
            lua_pushfstring(L, "searchRoom: bad argument #1 value (room id %d does not exist!)", room_id);
            // Should've been a nil with this as an second returned string!
            return 1;
        }
    } else {
        QList<TRoom*> roomList = host.mpMap->mpRoomDB->getRoomPtrList();
        lua_newtable(L);
        QList<int> roomIdsFound;
        for (auto pR : roomList) {
            if (!pR) {
                continue;
            }
            if (exactMatch) {
                if (pR->name.compare(room, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive) == 0) {
                    roomIdsFound.append(pR->getId());
                }
            } else {
                if (pR->name.contains(room, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
                    roomIdsFound.append(pR->getId());
                }
            }
        }
        if (!roomIdsFound.isEmpty()) {
            for (int i : roomIdsFound) {
                TRoom* pR = host.mpMap->mpRoomDB->getRoom(i);
                // This test is to keep Coverity happy as it thinks pR could be
                // a nullptr in some odd situation {CID 1415023}:
                if (pR) {
                    QString name = pR->name;
                    int roomID = pR->getId();
                    lua_pushnumber(L, roomID);
                    lua_pushstring(L, name.toUtf8().constData());
                    lua_settable(L, -3);
                }
            }
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#searchRoomUserData
int TLuaInterpreter::searchRoomUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "searchRoomUserData: no map present or loaded!");
        return 2;
    }

    QString key = QString();
    QString value = QString(); //both of these assigns a null value which is detectably different from the empty value

    if (lua_gettop(L)) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, R"(searchRoomUserData: bad argument #1 ("key" as string is optional, got %s!))", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            key = QString::fromUtf8(lua_tostring(L, 1));
        }

        if (lua_gettop(L) > 1) {
            if (!lua_isstring(L, 2)) {
                lua_pushfstring(L, R"(searchRoomUserData: bad argument #2 ("value" as string is optional, got %s!))", luaL_typename(L, 2));
                lua_error(L);
                return 1;
            } else {
                value = QString::fromUtf8(lua_tostring(L, 2));
            }
        }
    }

    lua_newtable(L);

    QHashIterator<int, TRoom*> itRoom(host.mpMap->mpRoomDB->getRoomMap());
    // For best performance do the three different types of action in three
    // different branches each with a loop - rather than choosing a branch
    // within a loop for each room

    lua_newtable(L);
    if (key.isNull()) { // Find all keys everywhere
        QSet<QString> keysSet;
        while (itRoom.hasNext()) {
            itRoom.next();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
            // In the brave new world of range based initializers one must use
            // a pair of iterators that point to the SAME thing that lasts
            // long enough - using the output of a Qt method that returns a
            // QList twice is not good enough and causes seg. faults...
            QList<QString> roomDataKeysList{itRoom.value()->userData.keys()};
            keysSet.unite(QSet<QString>{roomDataKeysList.begin(), roomDataKeysList.end()});
#else
            keysSet.unite(itRoom.value()->userData.keys().toSet());
#endif
        }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        QStringList keys{keysSet.begin(), keysSet.end()};
#else
        QStringList keys{keysSet.toList()};
#endif
        if (keys.size() > 1) {
            std::sort(keys.begin(), keys.end());
        }

        for (unsigned int i = 0, total = keys.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushstring(L, keys.at(i).toUtf8().constData());
            lua_settable(L, -3);
        }
    } else if (value.isNull()) { // Find all values for a particular key in every room
        QSet<QString> valuesSet; // Use a set as it automatically eliminates duplicates
        while (itRoom.hasNext()) {
            itRoom.next();
            QString roomValueForKey = itRoom.value()->userData.value(key, QString());
            // If the key is NOT present, will return second argument which is a
            // null QString which is NOT the same as an empty QString.
            if (!roomValueForKey.isNull()) {
                valuesSet.insert(roomValueForKey);
            }
        }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        QStringList values{valuesSet.begin(), valuesSet.end()};
#else
        QStringList values{valuesSet.toList()};
#endif
        if (values.size() > 1) {
            std::sort(values.begin(), values.end());
        }

        for (unsigned int i = 0, total = values.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushstring(L, values.at(i).toUtf8().constData());
            lua_settable(L, -3);
        }
    } else { // Find all rooms where key and value match
        QSet<int> roomIdsSet;
        while (itRoom.hasNext()) {
            itRoom.next();

            QString roomDataValue = itRoom.value()->userData.value(key, QString());
            if ((!roomDataValue.isNull()) && (!value.compare(roomDataValue, Qt::CaseSensitive))) {
                roomIdsSet.insert(itRoom.key());
            }
        }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        QList<int> roomIds{roomIdsSet.begin(), roomIdsSet.end()};
#else
        QList<int> roomIds{roomIdsSet.toList()};
#endif
        if (roomIds.size() > 1) {
            std::sort(roomIds.begin(), roomIds.end());
        }

        for (unsigned int i = 0, total = roomIds.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushnumber(L, roomIds.at(i));
            lua_settable(L, -3);
        }
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#searchAreaUserData
int TLuaInterpreter::searchAreaUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "searchAreaUserData: no map present or loaded!");
        return 2;
    }

    QString key = QString();
    QString value = QString(); //both of these assigns a null value which is detectably different from the empty value

    if (lua_gettop(L)) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, R"(searchAreaUserData: bad argument #1 ("key" as string is optional, got %s!))", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            key = QString::fromUtf8(lua_tostring(L, 1));
        }

        if (lua_gettop(L) > 1) {
            if (!lua_isstring(L, 2)) {
                lua_pushfstring(L, R"(searchAreaUserData: bad argument #2 ("value" as string is optional, got %s!))", luaL_typename(L, 2));
                lua_error(L);
                return 1;
            } else {
                value = QString::fromUtf8(lua_tostring(L, 2));
            }
        }
    }

    lua_newtable(L);

    QMapIterator<int, TArea*> itArea(host.mpMap->mpRoomDB->getAreaMap());
    // For best performance do the three different types of action in three
    // different branches each with a loop - rather than choosing a branch
    // within a loop for each room

    lua_newtable(L);
    if (key.isNull()) { // Find all keys everywhere
        QSet<QString> keysSet;
        while (itArea.hasNext()) {
            itArea.next();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
            QList<QString> areaDataKeysList{itArea.value()->mUserData.keys()};
            keysSet.unite(QSet<QString>{areaDataKeysList.begin(), areaDataKeysList.end()});
#else
            keysSet.unite(itArea.value()->mUserData.keys().toSet());
#endif
        }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        QStringList keys{keysSet.begin(), keysSet.end()};
#else
        QStringList keys{keysSet.toList()};
#endif
        if (keys.size() > 1) {
            std::sort(keys.begin(), keys.end());
        }

        for (unsigned int i = 0, total = keys.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushstring(L, keys.at(i).toUtf8().constData());
            lua_settable(L, -3);
        }
    } else if (value.isNull()) { // Find all values for a particular key in every room
        QSet<QString> valuesSet; // Use a set as it automatically eliminates duplicates
        while (itArea.hasNext()) {
            itArea.next();
            QString areaValueForKey = itArea.value()->mUserData.value(key, QString());
            // If the key is NOT present, will return second argument which is a
            // null QString which is NOT the same as an empty QString.
            if (!areaValueForKey.isNull()) {
                valuesSet.insert(areaValueForKey);
            }
        }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        QStringList values{valuesSet.begin(), valuesSet.end()};
#else
        QStringList values{valuesSet.toList()};
#endif
        if (values.size() > 1) {
            std::sort(values.begin(), values.end());
        }

        for (unsigned int i = 0, total = values.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushstring(L, values.at(i).toUtf8().constData());
            lua_settable(L, -3);
        }
    } else {
        QSet<int> areaIdsSet;
        while (itArea.hasNext()) { // Find all areas with a particular key AND value
            itArea.next();

            QString areaDataValue = itArea.value()->mUserData.value(key, QString());
            if ((!areaDataValue.isNull()) && (!value.compare(areaDataValue, Qt::CaseSensitive))) {
                areaIdsSet.insert(itArea.key());
            }
        }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        QList<int> areaIds{areaIdsSet.begin(), areaIdsSet.begin()};
#else
        QList<int> areaIds{areaIdsSet.toList()};
#endif
        if (areaIds.size() > 1) {
            std::sort(areaIds.begin(), areaIds.end());
        }

        for (unsigned int i = 0, total = areaIds.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushnumber(L, areaIds.at(i));
            lua_settable(L, -3);
        }
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAreaTable
int TLuaInterpreter::getAreaTable(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getAreaTable: no map present or loaded!");
        return 2;
    }

    QMapIterator<int, QString> it(host.mpMap->mpRoomDB->getAreaNamesMap());
    lua_newtable(L);
    while (it.hasNext()) {
        it.next();
        int areaId = it.key();
        QString name = it.value();
        lua_pushstring(L, name.toUtf8().constData());
        lua_pushnumber(L, areaId);
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAreaTableSwap
int TLuaInterpreter::getAreaTableSwap(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getAreaTableSwap: no map present or loaded!");
        return 2;
    }

    QMapIterator<int, QString> it(host.mpMap->mpRoomDB->getAreaNamesMap());
    lua_newtable(L);
    while (it.hasNext()) {
        it.next();
        int areaId = it.key();
        QString name = it.value();
        lua_pushnumber(L, areaId);
        lua_pushstring(L, name.toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAreaRooms
int TLuaInterpreter::getAreaRooms(lua_State* L)
{
    int area;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "getAreaRooms: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        area = lua_tonumber(L, 1);
    }
    Host& host = getHostFromLua(L);
    TArea* pA = host.mpMap->mpRoomDB->getArea(area);
    if (!pA) {
        lua_pushnil(L);
        return 1;
    }
    lua_newtable(L);
    QSetIterator<int> itAreaRoom(pA->getAreaRooms());
    int i = -1;
    while (itAreaRoom.hasNext()) {
        lua_pushnumber(L, ++i);
        // We should have started at 1 but past code had incorrectly started
        // with a zero index and we must maintain compatibilty with code written
        // for that
        lua_pushnumber(L, itAreaRoom.next());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRooms
int TLuaInterpreter::getRooms(lua_State* L)
{
    Host& host = getHostFromLua(L);
    lua_newtable(L);
    QHashIterator<int, TRoom*> it(host.mpMap->mpRoomDB->getRoomMap());
    while (it.hasNext()) {
        it.next();
        lua_pushnumber(L, it.key());
        lua_pushstring(L, it.value()->name.toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAreaExits
int TLuaInterpreter::getAreaExits(lua_State* L)
{
    int area = 0;
    int n = lua_gettop(L);
    bool isFullDataRequired = false;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getAreaExits: bad argument #1 type (area id as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        area = lua_tonumber(L, 1);
    }

    if (n > 1) {
        if (!lua_isboolean(L, 2)) {
            lua_pushfstring(L, "getAreaExits: bad argument #2 type (full data wanted as boolean is optional, got %s!)", luaL_typename(L, 2));
            lua_error(L);
            return 1;
        } else {
            isFullDataRequired = lua_toboolean(L, 2);
        }
    }

    Host& host = getHostFromLua(L);
    TArea* pA = host.mpMap->mpRoomDB->getArea(area);
    if (!pA) {
        lua_pushnil(L);
        lua_pushfstring(L, "getAreaExits: bad argument #1 value (number %d is not a valid area id).", area);
        return 2;
    }

    lua_newtable(L);
    if (n < 2 || (n > 1 && !isFullDataRequired)) {
        // Replicate original implimentation
        QList<int> areaExits = pA->getAreaExitRoomIds();
        if (areaExits.size() > 1) {
            std::sort(areaExits.begin(), areaExits.end());
        }
        for (int i = 0; i < areaExits.size(); i++) {
            lua_pushnumber(L, i + 1); // Lua lists/arrays begin at 1 not 0!
            lua_pushnumber(L, areaExits.at(i));
            lua_settable(L, -3);
        }
    } else {
        QMultiMap<int, QPair<QString, int>> areaExits = pA->getAreaExitRoomData();
        QList<int> fromRooms = areaExits.uniqueKeys();
        for (int fromRoom : fromRooms) {
            lua_pushnumber(L, fromRoom);
            lua_newtable(L);
            QList<QPair<QString, int>> toRoomsData = areaExits.values(fromRoom);
            for (const auto& j : toRoomsData) {
                lua_pushstring(L, j.first.toUtf8().constData());
                lua_pushnumber(L, j.second);
                lua_settable(L, -3);
            }
            lua_settable(L, -3);
        }
    }
    return 1;
}

// Documentation: ? - public function missing documentation in wiki
// Now audits the whole map
int TLuaInterpreter::auditAreas(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.mpMap->audit();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#gotoRoom
int TLuaInterpreter::getRoomWeight(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int roomId;
    if (lua_gettop(L) > 0) {
        if (!lua_isnumber(L, 1)) {
            lua_pushstring(L, "getRoomWeight: wrong argument type");
            lua_error(L);
            return 1;
        } else {
            roomId = lua_tonumber(L, 1);
        }
    } else {
        roomId = host.mpMap->mRoomIdHash.value(host.getName());
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (pR) {
        lua_pushnumber(L, pR->getWeight());
        return 1;
    } else {
        return 0;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#gotoRoom
int TLuaInterpreter::gotoRoom(lua_State* L)
{
    int targetRoomId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "gotoRoom: bad argument #1 type (target room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        targetRoomId = lua_tonumber(L, 1);
    }

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "gotoRoom: no map present or loaded!");
        return 2;
    } else if (!host.mpMap->mpRoomDB->getRoom(targetRoomId)) {
        lua_pushnil(L);
        lua_pushfstring(L, "gotoRoom: bad argument #1 value (number %d is not a valid target room id).", targetRoomId);
        return 2;
    }

    if (host.mpMap->gotoRoom(targetRoomId)) {
        host.startSpeedWalk();
        lua_pushboolean(L, true);
        return 1;
    } else {
        int totalWeight = host.assemblePath(); // Needed if unsucessful to clear lua speedwalk tables
        Q_UNUSED(totalWeight);
        lua_pushboolean(L, false);
        lua_pushfstring(L, "gotoRoom: no path found from current room to room with id %d!", targetRoomId);
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPath
int TLuaInterpreter::getPath(lua_State* L)
{
    int originRoomId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getPath: bad argument #1 type (starting room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        originRoomId = lua_tonumber(L, 1);
    }

    int targetRoomId;
    if (!lua_isnumber(L, 2)) {
        lua_pushfstring(L, "getPath: bad argument #2 type (target room id as number expected, got %s!)", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        targetRoomId = lua_tonumber(L, 2);
    }

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getPath: no map present or loaded!");
        return 2;
    } else if (!host.mpMap->mpRoomDB->getRoom(originRoomId)) {
        lua_pushnil(L);
        lua_pushfstring(L, "getPath: bad argument #1 value (number %d is not a valid source room id).", originRoomId);
        return 2;
    } else if (!host.mpMap->mpRoomDB->getRoom(targetRoomId)) {
        lua_pushnil(L);
        lua_pushfstring(L, "getPath: bad argument #2 value (number %d is not a valid target room id).", targetRoomId);
        return 2;
    }

    bool ret = host.mpMap->gotoRoom(originRoomId, targetRoomId);
    int totalWeight = host.assemblePath(); // Needed even if unsucessful, to clear lua tables then
    if (ret) {
        lua_pushboolean(L, true);
        lua_pushnumber(L, totalWeight);
        return 2;
    } else {
        lua_pushboolean(L, false);
        lua_pushnumber(L, -1);
        lua_pushfstring(L, "getPath: no path found from room, with Id %d to room %d!", originRoomId, targetRoomId);
        return 3;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deselect
int TLuaInterpreter::deselect(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName; // only for case with an argument, will be null if not assigned to which is different from being empty
    if (lua_gettop(L) > 0) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, R"(deselect: bad argument #1 type (window name as string, is optional {defaults to "main" if omitted}, got %s!))", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            // We cannot yet properly handle non-ASCII windows names but we will eventually!
            windowName = QString::fromUtf8(lua_tostring(L, 1));
            if (windowName == QLatin1String("main")) {
                // This matches the identifier for the main window - so make it
                // appear so by emptying it...
                windowName.clear();
            }
        }
    }

    if (windowName.isEmpty()) {
        host.mpConsole->deselect();
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, mudlet::self()->deselect(&host, windowName));
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetFormat
int TLuaInterpreter::resetFormat(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName; // only for case with an argument, will be null if not assigned to which is different from being empty
    if (lua_gettop(L) > 0) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, R"(resetFormat: bad argument #1 type (window name as string, is optional {defaults to "main" if omitted}, got %s!))", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            // We cannot yet properly handle non-ASCII windows names but we will eventually!
            windowName = QString::fromUtf8(lua_tostring(L, 1));
            if (windowName == QLatin1String("main")) {
                // This matches the identifier for the main window - so make it
                // appear so by emptying it...
                windowName.clear();
            }
        }
    }

    if (windowName.isEmpty()) {
        host.mpConsole->reset();
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, mudlet::self()->resetFormat(&host, windowName));
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hasFocus
int TLuaInterpreter::hasFocus(lua_State* L)
{
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpConsole->hasFocus()); //FIXME
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echoUserWindow
int TLuaInterpreter::echoUserWindow(lua_State* L)
{
    std::string luaWindowName = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "echoUserWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaWindowName = lua_tostring(L, 1);
    }

    std::string luaSendText = "";
    if (!lua_isstring(L, 2)) {
        lua_pushstring(L, "echoUserWindow: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 2);
    }
    Host& host = getHostFromLua(L);
    QString text(luaSendText.c_str());
    QString windowName(luaWindowName.c_str());
    mudlet::self()->echoWindow(&host, windowName, text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setAppStyleSheet
int TLuaInterpreter::setAppStyleSheet(lua_State* L)
{
    QString styleSheet;
    QString tag;
    int s = 0;
    int n = lua_gettop(L);
    if (n) {
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L, "setAppStyleSheet: bad argument #%d type (style sheet as string expected, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        }
        styleSheet = QString::fromUtf8(lua_tostring(L, s));
    }

    if (n > 1) {
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L, "setAppStyleSheet: bad argument #%d type (tag as string is optional, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        }
        tag = QString::fromUtf8(lua_tostring(L, s));
    }

    Host& host = getHostFromLua(L);
    TEvent event {};
    event.mArgumentList.append(QLatin1String("sysAppStyleSheetChange"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(tag);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(host.getName());
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    qApp->setStyleSheet(styleSheet);
    mudlet::self()->getHostManager().postInterHostEvent(nullptr, event, true);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setAppStyleSheet
int TLuaInterpreter::setProfileStyleSheet(lua_State* L)
{
    QString styleSheet;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setProfileStyleSheet: bad argument #1 type (style sheet as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    styleSheet = QString::fromUtf8(lua_tostring(L, 1));

    Host& host = getHostFromLua(L);
    lua_pushboolean(L, mudlet::self()->setProfileStyleSheet(&host, styleSheet));
    return 1;
}

// No documentation available in wiki - internal function
// this was an internal only function used by the package system, but it was
// inactive and has been removed
int TLuaInterpreter::showUnzipProgress(lua_State* L)
{
    lua_pushnil(L);
    lua_pushstring(L, "showUnzipProgress: removed command, this function is now inactive and does nothing!");
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#playSoundFile
int TLuaInterpreter::playSoundFile(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "playSoundFile: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }

    QString sound = luaSendText.c_str();
    if (QDir::homePath().contains('\\')) {
        sound.replace('/', R"(\)");
    } else {
        sound.replace('\\', "/");
    }
    /* if no volume provided, substitute 100 (maximum) */
    mudlet::self()->playSound(sound, lua_isnumber(L, 2) ? lua_tointeger(L, 2) : 100);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopSounds
int TLuaInterpreter::stopSounds(lua_State* L)
{
    //doesn't take an argument
    mudlet::self()->stopSounds();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#moveCursorEnd
int TLuaInterpreter::moveCursorEnd(lua_State* L)
{
    std::string luaWindowName = "";
    if (lua_isstring(L, 1)) {
        luaWindowName = lua_tostring(L, 1);
    } else {
        luaWindowName = "main";
    }

    Host& host = getHostFromLua(L);
    QString windowName(luaWindowName.c_str());
    if (luaWindowName == "main") {
        host.mpConsole->moveCursorEnd();
    } else {
        mudlet::self()->moveCursorEnd(&host, windowName);
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLastLineNumber
int TLuaInterpreter::getLastLineNumber(lua_State* L)
{
    std::string luaWindowName;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "getLastLineNumber: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaWindowName = lua_tostring(L, 1);
    }

    Host& host = getHostFromLua(L);
    QString windowName(luaWindowName.c_str());
    int number;
    if (luaWindowName == "main") {
        number = host.mpConsole->getLastLineNumber();
    } else {
        number = mudlet::self()->getLastLineNumber(&host, windowName);
    }
    lua_pushnumber(L, number);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMudletHomeDir
int TLuaInterpreter::getMudletHomeDir(lua_State* L)
{
    Host& host = getHostFromLua(L);
    QString nativeHomeDirectory = mudlet::getMudletPath(mudlet::profileHomePath, host.getName());
    lua_pushstring(L, nativeHomeDirectory.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disconnect
int TLuaInterpreter::disconnect(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.mTelnet.disconnectIt();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#reconnect
int TLuaInterpreter::reconnect(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.mTelnet.reconnect();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTriggerStayOpen
int TLuaInterpreter::setTriggerStayOpen(lua_State* L)
{
    std::string luaWindowName;
    double b;
    int s = 1;
    if (lua_gettop(L) > 1) {
        if (!lua_isstring(L, s)) {
            lua_pushstring(L, "setTriggerStayOpen: wrong argument type");
            lua_error(L);
            return 1;
        } else {
            luaWindowName = lua_tostring(L, s);
            s++;
        }
    }
    if (!lua_isnumber(L, s)) {
        lua_pushstring(L, "setTriggerStayOpen: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        b = lua_tonumber(L, s);
    }

    Host& host = getHostFromLua(L);
    QString windowName(luaWindowName.c_str());
    host.getTriggerUnit()->setTriggerStayOpen(QString(luaWindowName.c_str()), static_cast<int>(b));
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLink
int TLuaInterpreter::setLink(lua_State* L)
{
    std::string luaWindowName;
    std::string linkFunction;
    std::string linkHint;
    int s = 1;
    if (lua_gettop(L) > 2) {
        if (!lua_isstring(L, s)) {
            lua_pushstring(L, "setLink: wrong argument type");
            lua_error(L);
            return 1;
        } else {
            luaWindowName = lua_tostring(L, s);
            s++;
        }
    }

    if (!lua_isstring(L, s)) {
        lua_pushstring(L, "setLink: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        linkFunction = lua_tostring(L, s);
        s++;
    }
    if (!lua_isstring(L, s)) {
        lua_pushstring(L, "setLink: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        linkHint = lua_tostring(L, s);
        s++;
    }


    Host& host = getHostFromLua(L);
    QString windowName(luaWindowName.c_str());
    QStringList _linkFunction;
    _linkFunction << QString(linkFunction.c_str());
    QStringList _linkHint;
    _linkHint << QString(linkHint.c_str());
    if (windowName.size() > 0) {
        mudlet::self()->setLink(&host, windowName, _linkFunction, _linkHint);
    } else {
        host.mpConsole->setLink(_linkFunction, _linkHint);
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setPopup
int TLuaInterpreter::setPopup(lua_State* L)
{
    std::string a1 = "";
    std::string a2;
    QStringList _hintList;
    QStringList _commandList;
    int s = 1;
    int n = lua_gettop(L);
    // console name is an optional first argument
    if (n > 4) {
        if (!lua_isstring(L, s)) {
            lua_pushstring(L, "setPopup: wrong argument type");
            lua_error(L);
            return 1;
        } else {
            a1 = lua_tostring(L, s);
            s++;
        }
    }
    if (!lua_isstring(L, s)) {
        lua_pushstring(L, "setPopup: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        a2 = lua_tostring(L, s);
        s++;
    }

    if (!lua_istable(L, s)) {
        lua_pushstring(L, "setPopup: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        lua_pushnil(L);
        while (lua_next(L, s) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING) {
                QString cmd = lua_tostring(L, -1);
                _commandList << cmd;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
        s++;
    }
    if (!lua_istable(L, s)) {
        lua_pushstring(L, "setPopup: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        lua_pushnil(L);
        while (lua_next(L, s) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING) {
                QString hint = lua_tostring(L, -1);
                _hintList << hint;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
        s++;
    }
    Host& host = getHostFromLua(L);
    QString txt = a2.c_str();
    QString name = a1.c_str();
    if (_commandList.size() != _hintList.size()) {
        lua_pushstring(L, "Error: command list size and hint list size do not match cannot create popup");
        lua_error(L);
        return 1;
    }

    if (a1.empty()) {
        host.mpConsole->setLink(_commandList, _hintList);
    } else {
        mudlet::self()->setLink(&host, name, _commandList, _hintList);
    }

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBold
int TLuaInterpreter::setBold(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName;
    int s = 0;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L,
                            "setBold: bad argument #%d type (window name, as string expected {omission selects \"main\" console window}, got %s!",
                            s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }

    bool isAttributeEnabled;
    if (!lua_isboolean(L, ++s)) {
        lua_pushfstring(L, "setBold: bad argument #%d type (enable bold attribute as boolean expected, got %s!)", s, luaL_typename(L, s));
        lua_error(L);
        return 1;
    } else {
        isAttributeEnabled = lua_toboolean(L, s);
    }

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        host.mpConsole->setDisplayAttributes(TChar::Bold, isAttributeEnabled);
        // Always succeeds on main console:
        lua_pushboolean(L, true);
        return 1;
    } else {
        if (mudlet::self()->setDisplayAttributes(&host, windowName, TChar::Bold, isAttributeEnabled)) {
            lua_pushboolean(L, true);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, "window \"%s\" does not exist", windowName.toUtf8().constData());
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setItalics
int TLuaInterpreter::setItalics(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName;
    int s = 0;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L,
                            "setItalics: bad argument #%d type (window name, as string expected {omission selects \"main\" console window}, got %s!",
                            s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }

    bool isAttributeEnabled;
    if (!lua_isboolean(L, ++s)) {
        lua_pushfstring(L, "setItalics: bad argument #%d type (enable italic attribute as boolean expected, got %s!)", s, luaL_typename(L, s));
        lua_error(L);
        return 1;
    } else {
        isAttributeEnabled = lua_toboolean(L, s);
    }

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        host.mpConsole->setDisplayAttributes(TChar::Italic, isAttributeEnabled);
        // Always succeeds on main console:
        lua_pushboolean(L, true);
        return 1;
    } else {
        if (mudlet::self()->setDisplayAttributes(&host, windowName, TChar::Italic, isAttributeEnabled)) {
            lua_pushboolean(L, true);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, "window \"%s\" does not exist", windowName.toUtf8().constData());
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setOverline
int TLuaInterpreter::setOverline(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName;
    int s = 0;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L,
                            "setOverline: bad argument #%d type (window name, as string expected {omission selects \"main\" console window}, got %s!",
                            s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }

    bool isAttributeEnabled;
    if (!lua_isboolean(L, ++s)) {
        lua_pushfstring(L, "setOverline: bad argument #%d type (enable underline attribute as boolean expected, got %s!)", s, luaL_typename(L, s));
        lua_error(L);
        return 1;
    } else {
        isAttributeEnabled = lua_toboolean(L, s);
    }

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        host.mpConsole->setDisplayAttributes(TChar::Overline, isAttributeEnabled);
        // Always succeeds on main console:
        lua_pushboolean(L, true);
        return 1;
    } else {
        if (mudlet::self()->setDisplayAttributes(&host, windowName, TChar::Overline, isAttributeEnabled)) {
            lua_pushboolean(L, true);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, "window \"%s\" does not exist", windowName.toUtf8().constData());
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setReverse
int TLuaInterpreter::setReverse(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName;
    int s = 0;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L,
                            "setReverse: bad argument #%d type (window name, as string expected {omission selects \"main\" console window}, got %s!",
                            s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }

    bool isAttributeEnabled;
    if (!lua_isboolean(L, ++s)) {
        lua_pushfstring(L, "setReverse: bad argument #%d type (enable underline attribute as boolean expected, got %s!)", s, luaL_typename(L, s));
        lua_error(L);
        return 1;
    } else {
        isAttributeEnabled = lua_toboolean(L, s);
    }

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        host.mpConsole->setDisplayAttributes(TChar::Reverse, isAttributeEnabled);
        // Always succeeds on main console:
        lua_pushboolean(L, true);
        return 1;
    } else {
        if (mudlet::self()->setDisplayAttributes(&host, windowName, TChar::Reverse, isAttributeEnabled)) {
            lua_pushboolean(L, true);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, "window \"%s\" does not exist", windowName.toUtf8().constData());
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setStrikeOut
int TLuaInterpreter::setStrikeOut(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName;
    int s = 0;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L,
                            "setStrikeOut: bad argument #%d type (window name, as string expected {omission selects \"main\" console window}, got %s!",
                            s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }

    bool isAttributeEnabled;
    if (!lua_isboolean(L, ++s)) {
        lua_pushfstring(L, "setStrikeOut: bad argument #%d type (enable strikeout attribute as boolean expected, got %s!)", s, luaL_typename(L, s));
        lua_error(L);
        return 1;
    } else {
        isAttributeEnabled = lua_toboolean(L, s);
    }

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        host.mpConsole->setDisplayAttributes(TChar::StrikeOut, isAttributeEnabled);
        // Always succeeds on main console:
        lua_pushboolean(L, true);
        return 1;
    } else {
        if (mudlet::self()->setDisplayAttributes(&host, windowName, TChar::StrikeOut, isAttributeEnabled)) {
            lua_pushboolean(L, true);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, "window \"%s\" does not exist", windowName.toUtf8().constData());
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setUnderline
int TLuaInterpreter::setUnderline(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName;
    int s = 0;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L,
                            "setUnderline: bad argument #%d type (window name, as string expected {omission selects \"main\" console window}, got %s!",
                            s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }

    bool isAttributeEnabled;
    if (!lua_isboolean(L, ++s)) {
        lua_pushfstring(L, "setUnderline: bad argument #%d type (enable underline attribute as boolean expected, got %s!)", s, luaL_typename(L, s));
        lua_error(L);
        return 1;
    } else {
        isAttributeEnabled = lua_toboolean(L, s);
    }

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        host.mpConsole->setDisplayAttributes(TChar::Underline, isAttributeEnabled);
        // Always succeeds on main console:
        lua_pushboolean(L, true);
        return 1;
    } else {
        if (mudlet::self()->setDisplayAttributes(&host, windowName, TChar::Underline, isAttributeEnabled)) {
            lua_pushboolean(L, true);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, "window \"%s\" does not exist", windowName.toUtf8().constData());
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#debugc -- not #debug - compare GlobalLua
int TLuaInterpreter::debug(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int n = lua_gettop(L);
    if (!n) {
        // Nothing to show
        return 0;
    }

    QString luaDebugText;
    if (n > 1) {
        for (int i = 0; i < n; ++i) {
            luaDebugText += QStringLiteral(" (%1) %2").arg(QString::number(i + 1), QString::fromUtf8(lua_tostring(L, i + 1)));
        }
    } else {
        // n == 1
        luaDebugText = QStringLiteral(" %1").arg(QString::fromUtf8(lua_tostring(L, 1)));
    }
    luaDebugText.append(QChar::LineFeed);

    if (host.mpEditorDialog) {
        host.mpEditorDialog->mpErrorConsole->print(QLatin1String("[DEBUG:]"), QColor(Qt::blue), QColor(Qt::black));
        host.mpEditorDialog->mpErrorConsole->print(luaDebugText, QColor(Qt::green), QColor(Qt::black));
    }

    return 0;
}

int TLuaInterpreter::showHandlerError(lua_State* L)
{
    Host& host = getHostFromLua(L);
    QString event, error;

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "showHandlerError: bad argument #1 type (event name as string expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        event = QString::fromUtf8(lua_tostring(L, 1));
    }
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "showHandlerError: bad argument #2 type (error message as string expected, got %s!)", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        error = QString::fromUtf8(lua_tostring(L, 2));
    }

    host.mLuaInterpreter.logEventError(event, error);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hideToolBar
int TLuaInterpreter::hideToolBar(lua_State* L)
{
    std::string luaWindowName;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "hideToolBar: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaWindowName = lua_tostring(L, 1);
    }

    Host& host = getHostFromLua(L);
    QString windowName(luaWindowName.c_str());
    host.getActionUnit()->hideToolBar(windowName);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#showToolBar
int TLuaInterpreter::showToolBar(lua_State* L)
{
    std::string luaWindowName;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "showToolBar: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaWindowName = lua_tostring(L, 1);
    }

    Host& host = getHostFromLua(L);
    QString windowName(luaWindowName.c_str());
    host.getActionUnit()->showToolBar(windowName);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendATCP
int TLuaInterpreter::sendATCP(lua_State* L)
{
    Host& host = getHostFromLua(L);
    std::string msg;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "sendATCP: bad argument #1 type (message as string expected, got %1!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        msg = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 1));
    }

    std::string what;
    if (lua_gettop(L) > 1) {
        if (!lua_isstring(L, 2)) {
            lua_pushfstring(L, "sendATCP: bad argument #2 type (what as string is optional, got %1!)", luaL_typename(L, 2));
            return lua_error(L);
        } else {
            what = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 2));
        }
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
        lua_pushnil(L);
        lua_pushstring(L, "ATCP is not currently enabled");
        return 2;
    }

    // output is in Mud Server Encoding form here:
    if (!host.mTelnet.socketOutRaw(output)) {
        lua_pushnil(L);
        lua_pushstring(L, "unable to send all of the ATCP message");
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#receiveMSP
int TLuaInterpreter::receiveMSP(lua_State* L)
{
    Host& host = getHostFromLua(L);
    std::string msg;

    if (!host.mTelnet.isMSPEnabled()) {
        lua_pushnil(L);
        lua_pushstring(L, "MSP is not currently enabled");
        return 2;
    }

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "receiveMSP: bad argument #1 type (message as string expected, got %1!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    msg = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 1));
    host.mTelnet.setMSPVariables(QByteArray(msg.c_str(), msg.length()));

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#purgeMediaCache
int TLuaInterpreter::purgeMediaCache(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.mTelnet.purgeMediaCache();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendGMCP
int TLuaInterpreter::sendGMCP(lua_State* L)
{
    Host& host = getHostFromLua(L);
    std::string msg;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "sendGMCP: bad argument #1 type (message as string expected, got %1!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        msg = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 1));
    }

    std::string what;
    if (lua_gettop(L) > 1) {
        if (!lua_isstring(L, 2)) {
            lua_pushfstring(L, "sendGMCP: bad argument #2 type (what as string is optional, got %1!)", luaL_typename(L, 2));
            return lua_error(L);
        } else {
            what = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 2));
        }
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
        lua_pushnil(L);
        lua_pushstring(L, "GMCP is not currently enabled");
        return 2;
    }

    // output is in Mud Server Encoding form here:
    if (!host.mTelnet.socketOutRaw(output)) {
        lua_pushnil(L);
        lua_pushstring(L, "unable to send all of the GMCP message");
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendMSDP
int TLuaInterpreter::sendMSDP(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int n = lua_gettop(L);

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

    std::string variable = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 1));

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
    std::string msg;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "sendTelnetChannel102: bad argument #1 type (message bytes {2 characters} as string expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    } else {
        msg = lua_tostring(L, 1);
        if (msg.length() != 2) {
            lua_pushnil(L);
            lua_pushfstring(L, "invalid message of length %d supplied, it should be two bytes (may use lua \\### for each byte where ### is a number between 1 and 254)",
                            msg.length());
            return 2;
        }
    }

    std::string output;
    output += TN_IAC;
    output += TN_SB;
    output += OPT_102;
    output += msg;
    output += TN_IAC;
    output += TN_SE;

    Host& host = getHostFromLua(L);
    if (host.mTelnet.isChannel102Enabled()) {
        // We have already validated output to contain a 2 byte payload so we
        // should not need to worry about the "encoding" in this use of
        // socketOutRaw(...) - with the exception of handling any occurance of
        // 0xFF as either of the bytes to send - however Aardwolf does not use
        // *THAT* value so, though it is probably okay to not worry about the
        // need to "escape" it to get it through the telnet protocol unscathed
        // it is trival to fix:
        output = mudlet::replaceString(output, "\xff", "\xff\xff");
        host.mTelnet.socketOutRaw(output);
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "unable to send message as the 102 subchannel support has not been enabled by the MUD Server");
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getButtonState
int TLuaInterpreter::getButtonState(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int state;
    state = host.mpConsole->getButtonState();
    lua_pushnumber(L, state);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getNetworkLatency
int TLuaInterpreter::getNetworkLatency(lua_State* L)
{
    Host& host = getHostFromLua(L);
    double number;
    number = host.mTelnet.networkLatency;
    lua_pushnumber(L, number);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMainConsoleWidth
int TLuaInterpreter::getMainConsoleWidth(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int fw = QFontMetrics(host.getDisplayFont()).averageCharWidth();
    fw *= host.mWrapAt + 1;
    lua_pushnumber(L, fw);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMainWindowSize
int TLuaInterpreter::getMainWindowSize(lua_State* L)
{
    Host& host = getHostFromLua(L);
    QSize mainWindowSize = host.mpConsole->getMainWindowSize();

    lua_pushnumber(L, mainWindowSize.width());
    lua_pushnumber(L, mainWindowSize.height());

    return 2;
}
//add getUserWindowSize
int TLuaInterpreter::getUserWindowSize(lua_State* L)
{
    std::string luaSendWindow = "";
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "getUserWindowSize: bad argument #1 type (name as string expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        luaSendWindow = lua_tostring(L, 1);
    }

    Host& host = getHostFromLua(L);
    QString windowname(luaSendWindow.c_str());
    QSize userWindowSize = host.mpConsole->getUserWindowSize(windowname);
    lua_pushnumber(L, userWindowSize.width());
    lua_pushnumber(L, userWindowSize.height());

    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMousePosition
int TLuaInterpreter::getMousePosition(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QPoint pos = host.mpConsole->mapFromGlobal(QCursor::pos());

    lua_pushnumber(L, pos.x());
    lua_pushnumber(L, pos.y());

    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempTimer
int TLuaInterpreter::tempTimer(lua_State* L)
{
    double time;
    bool repeating{};
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "tempTimer: bad argument #1 type (time in seconds as {maybe decimal} number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    time = lua_tonumber(L, 1);

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (lua_isfunction(L, 2)) {
        if (lua_isboolean(L, 3)) {
            repeating = lua_toboolean(L, 3);
        }

        QPair<int, QString> result = pLuaInterpreter->startTempTimer(time, QString(), repeating);
        if (result.first == -1) {
            lua_pushnumber(L, -1);
            lua_pushstring(L, result.second.toUtf8().constData());
            return 2;
        }

        TTimer* timer = host.getTimerUnit()->getTimer(result.first);
        Q_ASSERT_X(timer,
                   "TLuaInterpreter::tempTimer(...)",
                   "Got a positive result from LuaInterpreter::startTempTimer(...) but that failed to produce pointer to it from Host::mTimerUnit::getTimer(...)");
        timer->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, timer);
        lua_pushvalue(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
        lua_pushnumber(L, result.first);
        return 1;
    }

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "tempTimer: bad argument #2 type (script or function name as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    QString luaCode = QString::fromUtf8(lua_tostring(L, 2));
    if (lua_isboolean(L, 3)) {
        repeating = lua_toboolean(L, 3);
    }

    QPair<int, QString> result = pLuaInterpreter->startTempTimer(time, luaCode, repeating);
    lua_pushnumber(L, result.first);
    if (result.first == -1) {
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempExactMatchTrigger
int TLuaInterpreter::tempExactMatchTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    int triggerID;
    int expirationCount = -1;

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "tempExactMatchTrigger: bad argument #1 type (exact match pattern as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString exactMatchPattern = QString::fromUtf8(lua_tostring(L, 1));

    if (lua_isnumber(L, 3)) {
        expirationCount = lua_tonumber(L, 3);

        if (expirationCount < 1) {
            lua_pushnil(L);
            lua_pushfstring(L, "tempExactMatchTrigger: bad argument #3 value (trigger expiration count must be greater than zero, got %d)", expirationCount);
            return 2;
        }
    }

    if (lua_isstring(L, 2)) {
        triggerID = pLuaInterpreter->startTempExactMatchTrigger(exactMatchPattern, QString::fromUtf8(lua_tostring(L, 2)), expirationCount);
    } else if (lua_isfunction(L, 2)) {
        triggerID = pLuaInterpreter->startTempExactMatchTrigger(exactMatchPattern, QString(), expirationCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempExactMatchTrigger: bad argument #2 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempBeginOfLineTrigger
int TLuaInterpreter::tempBeginOfLineTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    int triggerID;
    int expiryCount = -1;

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "tempBeginOfLineTrigger: bad argument #1 type (pattern as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString pattern = QString::fromUtf8(lua_tostring(L, 1));

        if (lua_isnumber(L, 3)) {
        expiryCount = lua_tonumber(L, 3);

        if (expiryCount < 1) {
            lua_pushnil(L);
            lua_pushfstring(L, "tempBeginOfLineTrigger: bad argument #3 value (trigger expiration count must be greater than zero, got %d)", expiryCount);
            return 2;
        }
    }

    if (lua_isstring(L, 2)) {
        triggerID = pLuaInterpreter->startTempBeginOfLineTrigger(pattern, QString::fromUtf8(lua_tostring(L, 2)), expiryCount);
    } else if (lua_isfunction(L, 2)) {
        triggerID = pLuaInterpreter->startTempBeginOfLineTrigger(pattern, QString(), expiryCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempBeginOfLineTrigger: bad argument #2 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempTrigger
int TLuaInterpreter::tempTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    QString substringPattern;
    int triggerID;
    int expiryCount = -1;

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "tempTrigger: bad argument #1 type (substring pattern as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    substringPattern = QString::fromUtf8(lua_tostring(L, 1));

    if (lua_isnumber(L, 3)) {
        expiryCount = lua_tonumber(L, 3);
        if (expiryCount < 1) {
            lua_pushnil(L);
            lua_pushfstring(L, "tempTrigger: bad argument #3 value (trigger expiration count must be greater than zero, got %d)", expiryCount);
            return 2;
        }
    }

    if (lua_isstring(L, 2)) {
        triggerID = pLuaInterpreter->startTempTrigger(substringPattern, QString::fromUtf8(lua_tostring(L, 2)), expiryCount);
    } else if (lua_isfunction(L, 2)) {
        triggerID = pLuaInterpreter->startTempTrigger(substringPattern, QString(), expiryCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempTrigger: bad argument #2 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempPromptTrigger
int TLuaInterpreter::tempPromptTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    int triggerID;
    int expiryCount = -1;

    if (lua_isnumber(L, 2)) {
        expiryCount = lua_tonumber(L, 2);

        if (expiryCount < 1) {
            lua_pushnil(L);
            lua_pushfstring(L, "tempPromptTrigger: bad argument #2 value (trigger expiration count must be greater than zero, got %d)", expiryCount);
            return 2;
        }
    }

    if (lua_isstring(L, 1)) {
        triggerID = pLuaInterpreter->startTempPromptTrigger(QString::fromUtf8(lua_tostring(L, 1)), expiryCount);
    } else if (lua_isfunction(L, 1)) {
        triggerID = pLuaInterpreter->startTempPromptTrigger(QString(), expiryCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 1);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempPromptTrigger: bad argument #1 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// This is documented as using a simple color table - but the numbers do not
// match ANSI numbering and fixing that would break existing scripts.
// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempColorTrigger
int TLuaInterpreter::tempColorTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "tempColorTrigger: bad argument #1 type (foreground color as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    // match ANSI numbering and fixing that would break existing scripts so it has
    // to be tweaked here (and in the Mudlet XML save file format!)
    int foregroundColor = TTrigger::scmIgnored;
    int value = lua_tointeger(L, 1);
    // clang-format off
    switch (value) {
    case 0:     foregroundColor = TTrigger::scmDefault;  break; // Default foreground colour
    case 1:     foregroundColor =      8;   break; // light black (dark gray)
    case 2:     foregroundColor =      0;   break; // black
    case 3:     foregroundColor =      9;   break; // light red
    case 4:     foregroundColor =      1;   break; // red
    case 5:     foregroundColor =     10;   break; // light green
    case 6:     foregroundColor =      2;   break; // green
    case 7:     foregroundColor =     11;   break; // light yellow
    case 8:     foregroundColor =      3;   break; // yellow
    case 9:     foregroundColor =     12;   break; // light blue
    case 10:    foregroundColor =      4;   break; // blue
    case 11:    foregroundColor =     13;   break; // light magenta
    case 12:    foregroundColor =      5;   break; // magenta
    case 13:    foregroundColor =     14;   break; // light cyan
    case 14:    foregroundColor =      6;   break; // cyan
    case 15:    foregroundColor =     15;   break; // light white
    case 16:    foregroundColor =      7;   break; // white (light gray)
    // The default includes case -1:    foregroundColor = TTrigger::scmIgnored
    // which means only consider the background color now (and that cannot be
    // set to this value) - NOTE: TTrigger::scmIgnored has been set to BE -1
    // when it was added after Mudlet 3.7.1 but if that is subsequently changed
    // it will break the API for this lua function
    // other colours in ANSI 256 colours handled but not mentioned in Wiki
    default:    foregroundColor =  value;   break;
    // clang-format on
    }

    if (!lua_isnumber(L, 2)) {
        lua_pushfstring(L, "tempColorTrigger: bad argument #2 type (background color as number is expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    value = lua_tointeger(L, 2);
    int backgroundColor = TTrigger::scmIgnored;
    // clang-format off
    switch (value) {
    case 0:     backgroundColor = TTrigger::scmDefault;  break; // Default background colour
    case 1:     backgroundColor =      8;   break; // light black (dark gray)
    case 2:     backgroundColor =      0;   break; // black
    case 3:     backgroundColor =      9;   break; // light red
    case 4:     backgroundColor =      1;   break; // red
    case 5:     backgroundColor =     10;   break; // light green
    case 6:     backgroundColor =      2;   break; // green
    case 7:     backgroundColor =     11;   break; // light yellow
    case 8:     backgroundColor =      3;   break; // yellow
    case 9:     backgroundColor =     12;   break; // light blue
    case 10:    backgroundColor =      4;   break; // blue
    case 11:    backgroundColor =     13;   break; // light magenta
    case 12:    backgroundColor =      5;   break; // magenta
    case 13:    backgroundColor =     14;   break; // light cyan
    case 14:    backgroundColor =      6;   break; // cyan
    case 15:    backgroundColor =     15;   break; // light white
    case 16:    backgroundColor =      7;   break; // white (light gray)
    // The default includes case -1:    backgroundColor = TTrigger::scmIgnored
    // but this cannot be used for the foreground case at the same time:
    default:    backgroundColor =  value;   break;
    // clang-format on
    }

    if (foregroundColor == TTrigger::scmIgnored && backgroundColor == TTrigger::scmIgnored) {
        lua_pushnil(L);
        lua_pushstring(L, "only one of foreground and background colors can be -1 (ignored)");
    }

    int triggerID;
    int expiryCount = -1;

    if (lua_isnumber(L, 4)) {
        expiryCount = lua_tonumber(L, 4);

        if (expiryCount < 1) {
            lua_pushnil(L);
            lua_pushfstring(L, "tempColorTrigger: bad argument #4 value (trigger expiration count must be greater than zero, got %d)", expiryCount);
            return 2;
        }
    }

    if (lua_isstring(L, 3)) {
        triggerID = pLuaInterpreter->startTempColorTrigger(foregroundColor, backgroundColor, QString::fromUtf8(lua_tostring(L, 3)), expiryCount);
    } else if (lua_isfunction(L, 3)) {
        triggerID = pLuaInterpreter->startTempColorTrigger(foregroundColor, backgroundColor, QString(), expiryCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 3);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempColorTrigger: bad argument #3 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// This is the replacement for tempColorTrigger() which uses the right numbers
// for ANSI colours in the range 0 to 255 or TTrigger::scmDefault for default
// colour or TTrigger::scmIgnored ignore; it is anticipated that additional
// special values less than zero may be added to detect other types of text (or
// for a 16M colour value where the components have to be given)
// Documentation: https://wiki.mudlet.org/w/Manual:Mudlet_Object_Functions#tempAnsiColorTrigger
int TLuaInterpreter::tempAnsiColorTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    QString code;
    int ansiFgColor = TTrigger::scmIgnored;
    int ansiBgColor = TTrigger::scmIgnored;
    if (lua_isstring(L, 1)) {
        code = QString::fromUtf8(lua_tostring(L, 1));
    } else if (lua_isfunction(L, 1)) {
        // leave code as a null QString()
    } else {
        lua_pushfstring(L, "tempAnsiColorTrigger: bad argument #1 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }

    if (!lua_isnumber(L, 2)) {
        lua_pushfstring(L, "tempAnsiColorTrigger: bad argument #2 type (foreground color as ANSI Color number {%d = ignore foreground color, %d = default colour, 0 to 255 ANSI colour} expected, got %s!)",
                        TTrigger::scmIgnored, TTrigger::scmDefault, luaL_typename(L, 2));
        return lua_error(L);
    } else {
        int value = lua_tointeger(L, 2);
        // At present we limit the range to (Trigger::scmIgnored),
        // (Trigger::scmDefault) and 0-255 ANSI colors - in the future we could
        // extend it to other "coded" values for locally generated textual
        // content
        if (!(value == TTrigger::scmIgnored || value == TTrigger::scmDefault || (value >= 0 && value <= 255))) {
            lua_pushnil(L);
            lua_pushfstring(L, "invalid ANSI color number %d, currently only %d (ignore foreground color), %d (default foregroud colour) or 0 to 255 recognised",
                            value, TTrigger::scmIgnored, TTrigger::scmDefault);
        } else if (value == TTrigger::scmIgnored && lua_gettop(L) < 2) {
            lua_pushnil(L);
            lua_pushfstring(L, "invalid ANSI color number %d, it cannot be used (to ignore the foreground color) if the background color is ommitted",
                            value);
        } else {
            ansiFgColor = value;
        }
    }

    if (lua_gettop(L) > 2 && !lua_isnumber(L, 2)) {
        lua_pushfstring(L, "tempAnsiColorTrigger: bad argument #3 type (background color as ANSI Color number {%d = ignore foreground color, %d = default colour, 0 to 255 ANSI colour} expected, got %s!)",
                        TTrigger::scmIgnored, TTrigger::scmDefault, luaL_typename(L, 3));
                 return lua_error(L);
    } else {
        int value = lua_tointeger(L, 2);
        if (!(value == TTrigger::scmIgnored || value == TTrigger::scmDefault || (value >= 0 && value <= 255))) {
            lua_pushnil(L);
            lua_pushfstring(L, "invalid ANSI color number %d, currently only %d (ignore background color), %d (default background color) or 0 to 255 recognised",
                            value, TTrigger::scmIgnored, TTrigger::scmDefault);
        } else if (value == TTrigger::scmIgnored && ansiFgColor == TTrigger::scmIgnored) {
            lua_pushnil(L);
            lua_pushfstring(L, "invalid ANSI color number %d, it cannot be used for the background color (to ignore that) if the foreground is also set to that value to also be ignored",
                            value);
        } else {
            ansiBgColor = value;
        }
    }

    int triggerID = pLuaInterpreter->startTempColorTrigger(ansiFgColor, ansiBgColor, code);
    if (code.isNull()) {
        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 3);
        lua_settable(L, LUA_REGISTRYINDEX);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempLineTrigger
int TLuaInterpreter::tempLineTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "tempLineTrigger: bad argument #1 type (line to start matching from as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else if (!lua_isnumber(L, 2)) {
        lua_pushfstring(L, "tempLineTrigger: bad argument #2 type (how many lines to match for as number expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    int from = lua_tointeger(L, 1);
    int howMany = lua_tointeger(L, 2);
    int triggerID;
    int expiryCount = -1;

    if (lua_isnumber(L, 4)) {
        expiryCount = lua_tonumber(L, 4);

        if (expiryCount < 1) {
            lua_pushnil(L);
            lua_pushfstring(L, "tempLineTrigger: bad argument #4 value (trigger expiration count must be greater than zero, got %d)", expiryCount);
            return 2;
        }
    }

    if (lua_isstring(L, 3)) {
        triggerID = pLuaInterpreter->startTempLineTrigger(from, howMany, QString::fromUtf8(lua_tostring(L, 3)), expiryCount);
    } else if (lua_isfunction(L, 3)) {
        triggerID = pLuaInterpreter->startTempLineTrigger(from, howMany, QString(), expiryCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 3);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempLineTrigger: bad argument #3 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempComplexRegexTrigger
int TLuaInterpreter::tempComplexRegexTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #1 type (trigger name create or add to as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #2 type (regex pattern to match as string, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else if (!lua_isstring(L, 3) && !lua_isfunction(L, 3)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #3 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    } else if (!lua_isnumber(L, 4)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #4 type (multiline flag as number expected, got %s!)", luaL_typename(L, 4));
        return lua_error(L);
    } else if (!lua_isnumber(L, 7)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #7 type (filter flag as number expected, got %s!)", luaL_typename(L, 7));
        return lua_error(L);
    } else if (!lua_isnumber(L, 8)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #8 type (match all flag as number expected, got %s!)", luaL_typename(L, 8));
        return lua_error(L);
    } else if (!lua_isnumber(L, 12)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #12 type (fire length as number expected, got %s!)", luaL_typename(L, 12));
        return lua_error(L);
    } else if (!lua_isnumber(L, 13)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #13 type (line delta as number expected, got %s!)", luaL_typename(L, 13));
        return lua_error(L);
    }

    QString triggerName = QString::fromUtf8(lua_tostring(L, 1));
    bool multiLine = lua_tonumber(L, 4);

    bool colorTrigger;
    QString fgColor;
    if (lua_isnumber(L, 5)) {
        colorTrigger = false;
    } else {
        colorTrigger = true;
        fgColor = lua_tostring(L, 5);
    }

    QString bgColor;
    if (lua_isnumber(L, 6)) {
        colorTrigger = false;
    } else {
        bgColor = lua_tostring(L, 6);
    }

    bool filter = lua_tonumber(L, 7);
    bool matchAll = lua_tonumber(L, 8);

    bool highlight;
    QColor hlFgColor;
    if (lua_isnumber(L, 9)) {
        highlight = false;
    } else {
        highlight = true;
        hlFgColor.setNamedColor(lua_tostring(L, 9));
    }
    QColor hlBgColor;
    if (lua_isnumber(L, 10)) {
        highlight = false;
    } else {
        highlight = true;
        hlBgColor.setNamedColor(lua_tostring(L, 10));
    }

    QString soundFile;
    bool playSound;
    if (lua_isstring(L, 11)) {
        playSound = true;
        soundFile = QString::fromUtf8(lua_tostring(L, 11));
    } else {
        playSound = false;
    }

    int fireLength = lua_tonumber(L, 12);
    int lineDelta = lua_tonumber(L, 13);

    int expiryCount = -1;

    if (lua_isnumber(L, 14)) {
        expiryCount = lua_tonumber(L, 14);

        if (expiryCount < 1) {
            lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #14 value (trigger expiration count must be greater than zero, got %d)", expiryCount);
            return lua_error(L);
        }
    }

    QString pattern = QString::fromUtf8(lua_tostring(L, 2));
    QStringList regexList;
    QList<int> propertyList;
    TTrigger* pP = host.getTriggerUnit()->findTrigger(triggerName);
    if (!pP) {
        regexList << pattern;
        if (colorTrigger) {
            propertyList << REGEX_COLOR_PATTERN;
        } else {
            propertyList << REGEX_PERL;
        }
    } else {
        regexList = pP->getRegexCodeList();
        propertyList = pP->getRegexCodePropertyList();
    }

    auto pT = new TTrigger("a", regexList, propertyList, multiLine, &host);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setName(pattern);
    pT->mPerlSlashGOption = matchAll; //match all
    pT->mFilterTrigger = filter;
    pT->setConditionLineDelta(lineDelta); //line delta
    pT->mStayOpen = fireLength;           //fire length
    pT->mSoundTrigger = playSound;        //sound trigger, need to set sound file if true
    if (playSound) {
        pT->setSound(soundFile);
    }
    pT->setIsColorizerTrigger(highlight); //highlight
    pT->setExpiryCount(expiryCount);
    if (highlight) {
        pT->setColorizerFgColor(hlFgColor);
        pT->setColorizerBgColor(hlBgColor);
    }

    if (lua_isstring(L, 3)) {
        pT->setScript(QString::fromUtf8(lua_tostring(L, 3)));
    } else if (lua_isfunction(L, 3)) {
        pT->setScript(QString());

        pT->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, pT);
        lua_pushvalue(L, 3);
        lua_settable(L, LUA_REGISTRYINDEX);
    }

    lua_pushstring(L, pattern.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempButton
int TLuaInterpreter::tempButton(lua_State* L)
{
    //args: parent, name, orientation
    QString toolbar, name;
    QString cmdButtonUp = "";
    QString cmdButtonDown = "";
    QString script = "";
    QStringList nameL;
    nameL << toolbar;
    int orientation;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "tempButton: wrong first arg");
        lua_error(L);
        return 1;
    } else {
        toolbar = lua_tostring(L, 1);
    }
    if (!lua_isstring(L, 2)) {
        lua_pushstring(L, "tempButton: wrong second arg");
        lua_error(L);
        return 1;
    } else {
        name = lua_tostring(L, 2);
    }
    if (!lua_isnumber(L, 3)) {
        lua_pushstring(L, "tempButton: wrong third arg");
        lua_error(L);
        return 1;
    } else {
        orientation = lua_tonumber(L, 3);
    }

    Host& host = getHostFromLua(L);
    TAction* pP = host.getActionUnit()->findAction(toolbar);
    if (!pP) {
        return 0;
    }
    TAction* pT = host.getActionUnit()->findAction(name);
    if (pT) {
        return 0;
    }
    pT = new TAction(pP, &host);
    pT->setName(name);
    pT->setCommandButtonUp(cmdButtonUp);
    pT->setCommandButtonDown(cmdButtonDown);
    pT->setIsPushDownButton(false);
    pT->mLocation = pP->mLocation;
    pT->mOrientation = orientation;
    pT->setScript(script);
    pT->setIsFolder(false);
    pT->setIsActive(true);


    //    pT->setIsPushDownButton( isChecked );
    //    pT->mLocation = location;
    //    pT->mOrientation = orientation;
    //    pT->setIsActive( pT->shouldBeActive() );
    //    pT->setButtonColor( color );
    //    pT->setButtonRotation( rotation );
    //    pT->setButtonColumns( columns );
    ////      pT->setButtonFlat( flatButton );
    //    pT->mUseCustomLayout = useCustomLayout;
    //    pT->mPosX = posX;
    //    pT->mPosY = posY;
    //    pT->mSizeX = sizeX;
    //    pT->mSizeY = sizeY;
    //    pT->css = mpActionsMainArea->css->toPlainText();


    pT->registerAction();
    // N/U:     int childID = pT->getID();
    host.getActionUnit()->updateToolbar();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setButtonStyleSheet
int TLuaInterpreter::setButtonStyleSheet(lua_State* L)
{
    //args: name, css text
    QString name, css;

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setButtonStyleSheet: bad argument #1 type (name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        name = QString::fromUtf8(lua_tostring(L, 1));
    }
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "setButtonStyleSheet: bad argument #2 type (css as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else {
        css = QString::fromUtf8(lua_tostring(L, 2));
    }

    Host& host = getHostFromLua(L);
    auto actionsList = host.getActionUnit()->findActionsByName(name);
    if (actionsList.empty()) {
        lua_pushnil(L);
        lua_pushfstring(L, "No button named \"%s\" found.", name.toUtf8().constData());
        return 2;
    }
    for (auto action : actionsList) {
        action->css = css;
    }
    host.getActionUnit()->updateToolbar();
    lua_pushboolean(L, 1);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempButtonToolbar
int TLuaInterpreter::tempButtonToolbar(lua_State* L)
{
    QString name;
    QString cmdButtonUp = "";
    QString cmdButtonDown = "";
    QString script = "";
    QStringList nameL;
    nameL << name;
    int location, orientation;

    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "tempButtonToolbar: wrong first arg");
        lua_error(L);
        return 1;
    } else {
        name = lua_tostring(L, 1);
    }
    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "tempButtonToolbar: wrong first arg");
        lua_error(L);
        return 1;
    } else {
        location = lua_tonumber(L, 2);
    }
    if (!lua_isnumber(L, 3)) {
        lua_pushstring(L, "tempButtonToolbar: wrong first arg");
        lua_error(L);
        return 1;
    } else {
        orientation = lua_tonumber(L, 3);
    }

    if (location > 0) {
        location++;
    }
    Host& host = getHostFromLua(L);
    TAction* pT = host.getActionUnit()->findAction(name);
    if (pT) {
        return 0;
    }

    //insert a new root item
    //ROOT_ACTION:

    pT = new TAction(name, &host);
    pT->setCommandButtonUp(cmdButtonUp);
    QStringList nl;
    nl << name;

    pT->setName(name);
    pT->setCommandButtonUp(cmdButtonUp);
    pT->setCommandButtonDown(cmdButtonDown);
    pT->setIsPushDownButton(false);
    pT->mLocation = location;
    pT->mOrientation = orientation;
    pT->setScript(script);
    pT->setIsFolder(true);
    pT->setIsActive(true);
    pT->registerAction();
    // N/U:     int childID = pT->getID();
    host.getActionUnit()->updateToolbar();


    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempRegexTrigger
int TLuaInterpreter::tempRegexTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    int triggerID;
    int expiryCount = -1;

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "tempRegexTrigger: bad argument #1 type (regex pattern as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString regexPattern = QString::fromUtf8(lua_tostring(L, 1));

    if (lua_isnumber(L, 3)) {
        expiryCount = lua_tonumber(L, 3);

        if (expiryCount < 1) {
            lua_pushnil(L);
            lua_pushfstring(L, "tempRegexTrigger: bad argument #3 value (trigger expiration count must be greater than zero, got %d)", expiryCount);
            return 2;
        }
    }

    if (lua_isstring(L, 2)) {
        triggerID = pLuaInterpreter->startTempRegexTrigger(regexPattern, QString::fromUtf8(lua_tostring(L, 2)), expiryCount);
    } else if (lua_isfunction(L, 2)) {
        triggerID = pLuaInterpreter->startTempRegexTrigger(regexPattern, QString(), expiryCount);

        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
    } else {
        lua_pushfstring(L, "tempRegexTrigger: bad argument #2 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempAlias
int TLuaInterpreter::tempAlias(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "tempAlias: bad argument #1 type (regex-type pattern as string expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }
    QString regex = QString::fromUtf8(lua_tostring(L, 1));

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "tempAlias: bad argument #2 type (lua script as string expected, got %s!)",
                        luaL_typename(L, 2));
        return lua_error(L);
    }
    QString script = QString::fromUtf8(lua_tostring(L, 2));

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    lua_pushnumber(L, pLuaInterpreter->startTempAlias(regex, script));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#exists
int TLuaInterpreter::exists(lua_State* L)
{
    std::string _name;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "exists: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        _name = lua_tostring(L, 1);
    }
    std::string _type;
    if (!lua_isstring(L, 2)) {
        lua_pushstring(L, "exists: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        _type = lua_tostring(L, 2);
    }
    Host& host = getHostFromLua(L);
    int cnt = 0;
    QString type = _type.c_str();
    type = type.toLower();
    QString name = _name.c_str();
    if (type == "timer") {
        cnt += host.getTimerUnit()->mLookupTable.count(name);
    } else if (type == "trigger") {
        cnt += host.getTriggerUnit()->mLookupTable.count(name);
    } else if (type == "alias") {
        cnt += host.getAliasUnit()->mLookupTable.count(name);
    } else if (type == "keybind") {
        cnt += host.getKeyUnit()->mLookupTable.count(name);
    } else if (type == "script") {
        cnt += host.getScriptUnit()->findScriptId(name).size();
    }
    lua_pushnumber(L, cnt);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isActive
int TLuaInterpreter::isActive(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "isActive: bad argument #1 type (item name as string expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name = QString::fromUtf8(lua_tostring(L, 1));

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "isActive: bad argument #1 type (item type as string expected, got %s!)",
                        luaL_typename(L, 2));
        return lua_error(L);
    }
    // Although we only use 4 ASCII strings the user may not enter a purely
    // ASCII value which we might have to report...
    QString type = QString::fromUtf8(lua_tostring(L, 2));

    Host& host = getHostFromLua(L);
    int cnt = 0;
    if (type.compare(QLatin1String("timer"), Qt::CaseInsensitive) == 0) {
        QMap<QString, TTimer*>::const_iterator it1 = host.getTimerUnit()->mLookupTable.constFind(name);
        while (it1 != host.getTimerUnit()->mLookupTable.cend() && it1.key() == name) {
            if (it1.value()->isActive()) {
                cnt++;
            }
            it1++;
        }
    } else if (type.compare(QLatin1String("trigger"), Qt::CaseInsensitive) == 0) {
        QMap<QString, TTrigger*>::const_iterator it1 = host.getTriggerUnit()->mLookupTable.constFind(name);
        while (it1 != host.getTriggerUnit()->mLookupTable.cend() && it1.key() == name) {
            if (it1.value()->isActive()) {
                cnt++;
            }
            it1++;
        }
    } else if (type.compare(QLatin1String("alias"), Qt::CaseInsensitive) == 0) {
        QMap<QString, TAlias*>::const_iterator it1 = host.getAliasUnit()->mLookupTable.constFind(name);
        while (it1 != host.getAliasUnit()->mLookupTable.cend() && it1.key() == name) {
            if (it1.value()->isActive()) {
                cnt++;
            }
            it1++;
        }
    } else if (type.compare(QLatin1String("keybind"), Qt::CaseInsensitive) == 0) {
        QMap<QString, TKey*>::const_iterator it1 = host.getKeyUnit()->mLookupTable.constFind(name);
        while (it1 != host.getKeyUnit()->mLookupTable.cend() && it1.key() == name) {
            if (it1.value()->isActive()) {
                cnt++;
            }
            it1++;
        }
    } else if (type.compare(QLatin1String("script"), Qt::CaseInsensitive) == 0) {
        QMap<int, TScript*> scripts = host.getScriptUnit()->getScriptList();
        for (auto script : scripts) {
            if (script->getName() == name && script->isActive()) {
                ++cnt;
            }
        }
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "invalid type '%s' given, it should be one (case insensitive) of: 'alias', 'keybind', 'timer' or 'trigger'",
                        type.toUtf8().constData());
    }
    lua_pushnumber(L, cnt);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permAlias
int TLuaInterpreter::permAlias(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "permAlias: bad argument #1 type (alias name as string expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name = QString::fromUtf8(lua_tostring(L, 1));

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "permAlias: bad argument #2 type (alias group/parent as string expected, got %s!)",
                        luaL_typename(L, 2));
        return lua_error(L);
    }
    QString parent = QString::fromUtf8(lua_tostring(L, 2));

    if (!lua_isstring(L, 3)) {
        lua_pushfstring(L, "permAlias: bad argument #3 type (regexp pattern as string expected, got %s!)",
                        luaL_typename(L, 3));
        return lua_error(L);
    }
    QString regex = QString::fromUtf8(lua_tostring(L, 3));

    if (!lua_isstring(L, 4)) {
        lua_pushfstring(L, "permAlias: bad argument #4 type (lua script as string expected, got %s!)",
                        luaL_typename(L, 4));
        return lua_error(L);
    }
    QString script = QString::fromUtf8(lua_tostring(L, 4));

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    lua_pushnumber(L, pLuaInterpreter->startPermAlias(name, parent, regex, script));
    return 1;
}

int TLuaInterpreter::getScript(lua_State* L)
{
    int n = lua_gettop(L);
    int pos = 1;

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "getScript: bad argument #1 type (script name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name = QString::fromUtf8(lua_tostring(L, 1));

    if (n > 1) {
        if (!lua_isnumber(L, 2)) {
            lua_pushfstring(L, "getScript: bad argument #2 type (script position as number expected, got %s!)", luaL_typename(L, 2));
            return lua_error(L);
        }
        pos = lua_tonumber(L, 2);
    }
    Host& host = getHostFromLua(L);

    auto ids = host.getScriptUnit()->findScriptId(name);
    auto pS = host.getScriptUnit()->getScript(ids.value(--pos, -1));
    if (!pS) {
        lua_pushnumber(L, -1);
        lua_pushstring(L, QStringLiteral("script \"%1\" at position \"%2\" not found").arg(name).arg(++pos).toUtf8().constData());
        return 2;
    }

    int id = pS->getID();
    lua_pushstring(L, pS->getScript().toUtf8().constData());
    lua_pushnumber(L, id);
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setScript
int TLuaInterpreter::setScript(lua_State* L)
{
    int n = lua_gettop(L);
    int pos = 1;

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setScript: bad argument #1 type (script name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name = QString::fromUtf8(lua_tostring(L, 1));

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "setScript: bad argument #2 type (script lua code as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    QString luaCode = QString::fromUtf8(lua_tostring(L, 2));

    if (n > 2) {
        if (!lua_isnumber(L, 3)) {
            lua_pushfstring(L, "setScript: bad argument #3 type (script position as number expected, got %s!)", luaL_typename(L, 3));
            return lua_error(L);
        }
        pos = lua_tonumber(L, 3);
    }

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    auto [id, message] = pLuaInterpreter->setScriptCode(name, luaCode, --pos);
    lua_pushnumber(L, id);
    if (id == -1) {
        lua_pushstring(L, message.toUtf8().constData());
        return 2;
    }

    return 1;
}


// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permScript
int TLuaInterpreter::permScript(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "permScript: bad argument #1 type (script name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name = QString::fromUtf8(lua_tostring(L, 1));

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "permScript: bad argument #2 type (script parent name as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    QString parent = QString::fromUtf8(lua_tostring(L, 2));

    if (!lua_isstring(L, 3)) {
        lua_pushfstring(L, "permScript: bad argument #3 type (script as string expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }
    QString luaCode = QString::fromUtf8(lua_tostring(L, 3));

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    auto [id, message] = pLuaInterpreter->createPermScript(name, parent, luaCode);
    lua_pushnumber(L, id);
    if (id == -1) {
        lua_pushstring(L, message.toUtf8().constData());
        return 2;
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permTimer
int TLuaInterpreter::permTimer(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "permTimer: bad argument #1 type (timer name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name = QString::fromUtf8(lua_tostring(L, 1));

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "permTimer: bad argument #2 type (timer parent name as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    QString parent = QString::fromUtf8(lua_tostring(L, 2));

    if (!lua_isnumber(L, 3)) {
        lua_pushfstring(L, "permTimer: bad argument #3 type (time in seconds as {maybe decimal} number expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }
    double time = lua_tonumber(L, 3);

    if (!lua_isstring(L, 4)) {
        lua_pushfstring(L, "permTimer: bad argument #4 type (script as string expected, got %s!)", luaL_typename(L, 4));
        return lua_error(L);
    }
    QString luaCode = QString::fromUtf8(lua_tostring(L, 4));

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    auto [id, message] = pLuaInterpreter->startPermTimer(name, parent, time, luaCode);
    lua_pushnumber(L, id);
    if (id == -1) {
        lua_pushstring(L, message.toUtf8().constData());
        return 2;
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permSubstringTrigger
int TLuaInterpreter::permSubstringTrigger(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "permSubstringTrigger: bad argument #1 type (trigger name as string expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name = QString::fromUtf8(lua_tostring(L, 1));

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "permSubstringTrigger: bad argument #2 type (trigger parent as string expected, got %s!)",
                        luaL_typename(L, 2));
        return lua_error(L);
    }
    QString parent = QString::fromUtf8(lua_tostring(L, 2));

    QStringList regList;
    if (!lua_istable(L, 3)) {
        lua_pushfstring(L, "permSubstringTrigger: bad argument #3 type (sub-strings list as table expected, got %s!)",
                        luaL_typename(L, 3));
        return lua_error(L);
    }
    lua_pushnil(L);
    while (lua_next(L, 3) != 0) {
        // key at index -2 and value at index -1
        if (lua_type(L, -1) == LUA_TSTRING) {
            regList << QString::fromUtf8(lua_tostring(L, -1));
        }
        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    if (!lua_isstring(L, 4)) {
        lua_pushfstring(L, "permSubstringTrigger: bad argument #4 type (lua script as string expected, got %s!)",
                        luaL_typename(L, 4));
        return lua_error(L);
    }
    QString script = QString::fromUtf8(lua_tostring(L, 4));

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    lua_pushnumber(L, pLuaInterpreter->startPermSubstringTrigger(name, parent, regList, script));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permPromptTrigger
int TLuaInterpreter::permPromptTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    int triggerID;
    QString triggerName, parentName, luaFunction;

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "permPromptTrigger: bad argument #1 type (trigger name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    triggerName = QString::fromUtf8(lua_tostring(L, 1));

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "permPromptTrigger: bad argument #2 type (parent trigger name as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    parentName = QString::fromUtf8(lua_tostring(L, 2));

    if (!lua_isstring(L, 3)) {
        lua_pushfstring(L, "permPromptTrigger: bad argument #3 type (code to run as string expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }
    luaFunction = QString::fromUtf8(lua_tostring(L, 3));

    triggerID = pLuaInterpreter->startPermPromptTrigger(triggerName, parentName, luaFunction);
    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permKey
int TLuaInterpreter::permKey(lua_State* L)
{
    uint_fast8_t argIndex = 0;
    QString keyName;
    if (!lua_isstring(L, ++argIndex)) {
        lua_pushfstring(L, "permKey: bad argument #1 type (key name as string expected, got %s!)", luaL_typename(L, argIndex));
        return lua_error(L);
    } else {
        keyName = QString::fromUtf8(lua_tostring(L, argIndex));
    }

    QString parentGroup;
    if (!lua_isstring(L, ++argIndex)) {
        lua_pushfstring(L, "permKey: bad argument #2 type (key parent group as string expected, got %s!)", luaL_typename(L, argIndex));
        return lua_error(L);
    } else {
        parentGroup = QString::fromUtf8(lua_tostring(L, argIndex));
    }

    int keyModifier = Qt::NoModifier;
    if (lua_gettop(L) > 4) {
        if (!lua_isnumber(L, ++argIndex) && !lua_isnil(L, argIndex)) {
            lua_pushfstring(L, "permKey: bad argument #%d type (key modifier as number is optional, got %s!)", argIndex, luaL_typename(L, argIndex));
            return lua_error(L);
        } else {
            keyModifier = lua_tointeger(L, argIndex);
        }
    }

    int keyCode = 0;
    if (!lua_isnumber(L, ++argIndex)) {
        lua_pushfstring(L, "permKey: bad argument #%d type (key code as number expected, got %s!)", argIndex, luaL_typename(L, argIndex));
        return lua_error(L);
    } else {
        keyCode = lua_tointeger(L, argIndex);
    }

    QString luaFunction;
    if (!lua_isstring(L, ++argIndex)) {
        lua_pushfstring(L, "permKey: bad argument #%d type (lua script as string expected, got %s!)", argIndex, luaL_typename(L, argIndex));
        return lua_error(L);
    } else {
        luaFunction = QString::fromUtf8(lua_tostring(L, argIndex));
    }

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    // FIXME: The script in the luaFunction could fail to compile - although this will still create a key (which will error each time it is encountered)
    int keyID = pLuaInterpreter->startPermKey(keyName, parentGroup, keyCode, keyModifier, luaFunction);
    lua_pushnumber(L, keyID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempKey
int TLuaInterpreter::tempKey(lua_State* L)
{
    uint_fast8_t argIndex = 0;
    int keyModifier = Qt::NoModifier;
    if (lua_gettop(L) > 2) {
        if (!lua_isnumber(L, ++argIndex) && !lua_isnil(L, argIndex)) {
            lua_pushfstring(L, "tempKey: bad argument #%d type (key modifier as number is optional, got %s!)", argIndex, luaL_typename(L, argIndex));
            return lua_error(L);
        } else {
            keyModifier = lua_tointeger(L, argIndex);
        }
    }

    int keyCode = 0;
    if (!lua_isnumber(L, ++argIndex)) {
        lua_pushfstring(L, "tempKey: bad argument #%d type (key code as number expected, got %s!)", argIndex, luaL_typename(L, argIndex));
        return lua_error(L);
    } else {
        keyCode = lua_tointeger(L, argIndex);
    }

    QString luaFunction;
    if (!lua_isstring(L, ++argIndex)) {
        lua_pushfstring(L, "tempKey: bad argument #%d type (lua script as string expected, got %s!)", argIndex, luaL_typename(L, argIndex));
        return lua_error(L);
    } else {
        luaFunction = QString::fromUtf8(lua_tostring(L, argIndex));
    }

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    int timerID = pLuaInterpreter->startTempKey(keyModifier, keyCode, luaFunction);
    lua_pushnumber(L, timerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permBeginOfLineStringTrigger
int TLuaInterpreter::permBeginOfLineStringTrigger(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "permBeginOfLineStringTrigger: bad argument #1 type (trigger name as string expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name = QString::fromUtf8(lua_tostring(L, 1));

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "permBeginOfLineStringTrigger: bad argument #2 type (trigger parent as string expected, got %s!)",
                        luaL_typename(L, 2));
        return lua_error(L);
    }
    QString parent = QString::fromUtf8(lua_tostring(L, 2));

    QStringList regList;
    if (!lua_istable(L, 3)) {
        lua_pushfstring(L, "permBeginOfLineStringTrigger: bad argument #3 type (sub-strings list as table expected, got %s!)",
                        luaL_typename(L, 3));
        return lua_error(L);
    }
    lua_pushnil(L);
    while (lua_next(L, 3) != 0) {
        // key at index -2 and value at index -1
        if (lua_type(L, -1) == LUA_TSTRING) {
            regList << QString::fromUtf8(lua_tostring(L, -1));
        }
        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    if (!lua_isstring(L, 4)) {
        lua_pushfstring(L, "permBeginOfLineStringTrigger: bad argument #4 type (lua script as string expected, got %s!)",
                        luaL_typename(L, 4));
        return lua_error(L);
    }
    QString script = QString::fromUtf8(lua_tostring(L, 4));

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    lua_pushnumber(L, pLuaInterpreter->startPermBeginOfLineStringTrigger(name, parent, regList, script));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permRegexTrigger
int TLuaInterpreter::permRegexTrigger(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "permRegexTrigger: bad argument #1 type (trigger name as string expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name = QString::fromUtf8(lua_tostring(L, 1));

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "permRegexTrigger: bad argument #2 type (trigger parent as string expected, got %s!)",
                        luaL_typename(L, 2));
        return lua_error(L);
    }
    QString parent = QString::fromUtf8(lua_tostring(L, 2));

    QStringList regList;
    if (!lua_istable(L, 3)) {
        lua_pushfstring(L, "permRegexTrigger: bad argument #3 type (sub-strings list as table expected, got %s!)",
                        luaL_typename(L, 3));
        return lua_error(L);
    }
    lua_pushnil(L);
    while (lua_next(L, 3) != 0) {
        // key at index -2 and value at index -1
        if (lua_type(L, -1) == LUA_TSTRING) {
            regList << QString::fromUtf8(lua_tostring(L, -1));
        }
        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    if (!lua_isstring(L, 4)) {
        lua_pushfstring(L, "permRegexTrigger: bad argument #4 type (lua script as string expected, got %s!)",
                        luaL_typename(L, 4));
        return lua_error(L);
    }
    QString script = QString::fromUtf8(lua_tostring(L, 4));

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    lua_pushnumber(L, pLuaInterpreter->startPermRegexTrigger(name, parent, regList, script));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#invokeFileDialog
int TLuaInterpreter::invokeFileDialog(lua_State* L)
{
    bool luaDir = false; //default is to choose a directory
    if (!lua_isboolean(L, 1)) {
        lua_pushstring(L, "invokeFileDialog: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaDir = lua_toboolean(L, 1);
    }
    std::string luaTitle;
    if (!lua_isstring(L, 2)) {
        lua_pushstring(L, "invokeFileDialog: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaTitle = lua_tostring(L, 2);
    }
    if (!luaDir) {
        QString fileName = QFileDialog::getExistingDirectory(nullptr, QString(luaTitle.c_str()), QDir::currentPath());
        lua_pushstring(L, fileName.toLatin1().data());
        return 1;
    } else {
        QString fileName = QFileDialog::getOpenFileName(nullptr, QString(luaTitle.c_str()), QDir::currentPath());
        lua_pushstring(L, fileName.toLatin1().data());
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getTimestamp
int TLuaInterpreter::getTimestamp(lua_State* L)
{
    int n = lua_gettop(L);
    int s = 0;
    QString name;
    if (n > 1) {
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L, "getTimestamp: bad argument #%d type (mini console, user window or buffer name as string expected {may be omitted for the \"main\" console}, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            name = QString::fromUtf8(lua_tostring(L, s));
            if (name == QLatin1String("main")) {
                // clear it so it is treated as the main console below
                name.clear();
            }
        }
    }

    qint64 luaLine;
    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "getTimestamp: bad argument #%d type (line number as number expected, got %s!", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        luaLine = lua_tointeger(L, n);
        if (luaLine < 1) {
            lua_pushnil(L);
            lua_pushfstring(L, "line number %d invalid, it should be greater than zero", luaLine);
            return 2;
        }
    }

    Host& host = getHostFromLua(L);
    if (name.isEmpty()) {
        if (luaLine > 0 && luaLine < host.mpConsole->buffer.timeBuffer.size()) {
            // CHECK: Lua starts counting at 1 but we are indexing into a C/C++
            // structure but the previous code did not accept a zero line number
            lua_pushstring(L, host.mpConsole->buffer.timeBuffer.at(luaLine).toUtf8().constData());
        } else {
            lua_pushstring(L, "getTimestamp: invalid line number");
        }
        return 1;
    } else {
        auto pC = host.mpConsole->mSubConsoleMap.value(name);
        if (pC) {
            if (luaLine > 0 && luaLine < pC->buffer.timeBuffer.size()) {
                lua_pushstring(L, pC->buffer.timeBuffer.at(luaLine).toUtf8().constData());
            } else {
                lua_pushstring(L, "getTimestamp: invalid line number");
            }
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, "mini console, user window or buffer \"%s\" not found", name.toUtf8().constData());
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderColor
int TLuaInterpreter::setBorderColor(lua_State* L)
{
    int luaRed;
    int luaGreen;
    int luaBlue;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "setBorderColor: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaRed = lua_tointeger(L, 1);
    }

    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "setBorderColor: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaGreen = lua_tointeger(L, 2);
    }

    if (!lua_isnumber(L, 3)) {
        lua_pushstring(L, "setBorderColor: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaBlue = lua_tointeger(L, 3);
    }

    Host& host = getHostFromLua(L);
    QPalette framePalette;
    framePalette.setColor(QPalette::Text, QColor(Qt::black));
    framePalette.setColor(QPalette::Highlight, QColor(55, 55, 255));
    framePalette.setColor(QPalette::Window, QColor(luaRed, luaGreen, luaBlue, 255));
    host.mpConsole->mpMainFrame->setPalette(framePalette);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomCoordinates
int TLuaInterpreter::setRoomCoordinates(lua_State* L)
{
    int id;
    int x;
    int y;
    int z;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "setRoomCoordinates: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tointeger(L, 1);
    }

    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "setRoomCoordinates: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        x = lua_tointeger(L, 2);
    }

    if (!lua_isnumber(L, 3)) {
        lua_pushstring(L, "setRoomCoordinates: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        y = lua_tointeger(L, 3);
    }

    if (!lua_isnumber(L, 4)) {
        lua_pushstring(L, "setRoomCoordinates: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        z = lua_tointeger(L, 4);
    }

    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpMap->setRoomCoordinates(id, x, y, z));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCustomEnvColor
int TLuaInterpreter::setCustomEnvColor(lua_State* L)
{
    int id;
    int r;
    int g;
    int b;
    int alpha;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "setCustomEnvColor: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tointeger(L, 1);
    }

    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "setCustomEnvColor: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        r = lua_tointeger(L, 2);
    }

    if (!lua_isnumber(L, 3)) {
        lua_pushstring(L, "setCustomEnvColor: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        g = lua_tointeger(L, 3);
    }

    if (!lua_isnumber(L, 4)) {
        lua_pushstring(L, "setCustomEnvColor: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        b = lua_tointeger(L, 4);
    }

    if (!lua_isnumber(L, 5)) {
        lua_pushstring(L, "setCustomEnvColor: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        alpha = lua_tointeger(L, 5);
    }

    Host& host = getHostFromLua(L);
    host.mpMap->customEnvColors[id] = QColor(r, g, b, alpha);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setAreaName
int TLuaInterpreter::setAreaName(lua_State* L)
{
    int id = -1;
    QString existingName;
    QString newName;
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "setAreaName: no map present or loaded!");
        return 2;
    }

    if (lua_isnumber(L, 1)) {
        id = lua_tonumber(L, 1);
        if (id < 1) {
            lua_pushnil(L);
            lua_pushfstring(L,
                            "setAreaName: bad argument #1 value (number %d is not a valid area id as it is\n"
                            "less than 1).",
                            id);
            return 2;
        }
        // Strangely, previous code allowed this command to create a NEW area's name
        // with this ID, but without a TArea instance to accompany it (the latter was/is
        // instantiated as needed when a room is moved to the relevent area...) and we
        // need to continue to allow this - Slysven
        //        else if (!host.mpMap->mpRoomDB->getAreaIDList().contains(id)) {
        //            lua_pushnil(L);
        //            lua_pushstring(L, "setAreaName: bad argument #1 value (number %d is not a valid area id)."
        //                           id);
        //            return 2;
        //        }
    } else if (lua_isstring(L, 1)) {
        existingName = QString::fromUtf8(lua_tostring(L, 1));
        id = host.mpMap->mpRoomDB->getAreaNamesMap().key(existingName, 0);
        if (existingName.isEmpty()) {
            lua_pushnil(L);
            lua_pushfstring(L, "setAreaName: bad argument #1 value (area name cannot be empty).");
            return 2;
        } else if (!host.mpMap->mpRoomDB->getAreaNamesMap().values().contains(existingName)) {
            lua_pushnil(L);
            lua_pushfstring(L, R"(setAreaName: bad argument #1 value (area name "%s" does not exist).)", existingName.toUtf8().constData());
            return 2;
        } else if (host.mpMap->mpRoomDB->getAreaNamesMap().value(-1).contains(existingName)) {
            lua_pushnil(L);
            lua_pushfstring(L,
                            "setAreaName: bad argument #1 value (area name \"%s\" is reserved and\n"
                            "protected - it cannot be changed).",
                            existingName.toUtf8().constData());
            return 2;
        }
    } else {
        lua_pushfstring(L,
                        "setAreaName: bad argument #1 type (area id as number or area name as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 1));
        lua_error(L);
        return 1;
    }

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "setAreaName: bad argument #2 type (area name as string expected, got %s!)", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        newName = QString::fromUtf8(lua_tostring(L, 2)).trimmed();
        // Now allow non-Ascii names but eliminate any leading or trailing spaces
    }

    if (newName.isEmpty()) {
        // Empty name not allowed (any more)
        lua_pushnil(L);
        lua_pushfstring(L,
                        "setAreaName: bad argument #2 value (area names may not be empty strings\n"
                        "{and spaces are trimmed from the ends})!");
        return 2;
    } else if (host.mpMap->mpRoomDB->getAreaNamesMap().values().count(newName) > 0) {
        // That name is already IN the areaNamesMap, and since we now enforce
        // uniqueness there can be only one of it - so we can check if this is a
        // problem or just pointless quite easily...!
        if (host.mpMap->mpRoomDB->getAreaNamesMap().value(id) != newName) {
            lua_pushnil(L);
            // And it isn't the trivial case, where the given areaID already IS that name
            lua_pushfstring(L,
                            "setAreaName: bad argument #2 value (area names may not be duplicated and area\n"
                            "id %d already has the name \"%s\").",
                            host.mpMap->mpRoomDB->getAreaNamesMap().key(newName),
                            newName.toUtf8().constData());
            return 2;
        } else {
            // Renaming an area to the same name is pointlessly successful!
            lua_pushboolean(L, true);
            return 1;
        }
    }

    bool isCurrentAreaRenamed = false;
    if (host.mpMap->mpMapper) {
        if (id > 0 && host.mpMap->mpRoomDB->getAreaNamesMap().value(id) == host.mpMap->mpMapper->showArea->currentText()) {
            isCurrentAreaRenamed = true;
        }
    }

    bool result = host.mpMap->mpRoomDB->setAreaName(id, newName);
    if (result) {
        if (host.mpMap->mpMapper) {
            host.mpMap->mpMapper->updateAreaComboBox();
            if (isCurrentAreaRenamed) {
                host.mpMap->mpMapper->showArea->setCurrentText(newName);
            }
            updateMap(L);
        }
    }
    lua_pushboolean(L, result);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomAreaName
int TLuaInterpreter::getRoomAreaName(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getRoomAreaName: no map present or loaded!");
        return 2;
    }

    int id;
    QString name;
    if (!lua_isnumber(L, 1)) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L,
                            "getRoomAreaName: bad argument #1 type (area id as number or area name as string\n"
                            "expected, got %s!)",
                            luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            name = QString::fromUtf8(lua_tostring(L, 1));
        }
    } else {
        id = lua_tonumber(L, 1);
    }

    if (!name.isNull()) {
        int result = host.mpMap->mpRoomDB->getAreaNamesMap().key(name, -1);
        lua_pushnumber(L, result);
        if (result != -1) {
            return 1;
        } else {
            lua_pushfstring(L,
                            "getRoomAreaName: bad argument #1 value (string \"%s\" is\n"
                            "not a valid area name).",
                            name.toUtf8().constData());
            return 2;
        }
    } else {
        if (host.mpMap->mpRoomDB->getAreaNamesMap().contains(id)) {
            lua_pushstring(L, host.mpMap->mpRoomDB->getAreaNamesMap().value(id).toUtf8().constData());
            return 1;
        } else {
            lua_pushnumber(L, -1);
            lua_pushfstring(L, "getRoomAreaName: bad argument #1 value (number %d is not a valid area id).", id);
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addAreaName
int TLuaInterpreter::addAreaName(lua_State* L)
{
    QString name;

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "addAreaName: bad argument #1 type (area name as string expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        name = QString::fromUtf8(lua_tostring(L, 1)).trimmed();
    }

    Host& host = getHostFromLua(L);
    if ((!host.mpMap) || (!host.mpMap->mpRoomDB)) {
        lua_pushnil(L);
        lua_pushstring(L, "addAreaName: error, no map seems to be loaded!");
        return 2;
    } else if (name.isEmpty()) {
        // Empty names now not allowed
        lua_pushnil(L);
        lua_pushfstring(L,
                        "addAreaName: bad argument #1 value (area names may not be empty strings {and\n"
                        "spaces are trimmed from the ends})!");
        return 2;
    } else if (host.mpMap->mpRoomDB->getAreaNamesMap().values().count(name) > 0) {
        // That name is already IN the areaNamesMap
        lua_pushnil(L);
        lua_pushfstring(L,
                        "addAreaName: bad argument #2 value (area names may not be duplicated and area\n"
                        "id %d already has the name \"%s\").",
                        host.mpMap->mpRoomDB->getAreaNamesMap().key(name),
                        name.toUtf8().constData());
        return 2;
    }

    // Note that adding an area name implicitly creates an underlying TArea instance
    lua_pushnumber(L, host.mpMap->mpRoomDB->addArea(name));

    // Update mapper Area names widget, using method designed for it...!
    if (host.mpMap->mpMapper) {
        host.mpMap->mpMapper->updateAreaComboBox();
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteArea
int TLuaInterpreter::deleteArea(lua_State* L)
{
    int id = 0;
    QString name;

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "deleteArea: no map present or loaded!");
        return 2;
    }

    if (lua_isnumber(L, 1)) {
        id = lua_tonumber(L, 1);
        if (id < 1) {
            lua_pushnil(L);
            lua_pushfstring(L,
                            "deleteArea: bad argument #1 value (number %d is not a valid area id greater\n"
                            "than zero).",
                            id);
            return 2;
        } else if (!host.mpMap->mpRoomDB->getAreaIDList().contains(id) && !host.mpMap->mpRoomDB->getAreaNamesMap().contains(id)) {
            lua_pushnil(L);
            lua_pushfstring(L, "deleteArea: bad argument #1 value (number %d is not a valid area id).", id);
            return 2;
        }
    } else if (lua_isstring(L, 1)) {
        name = QString::fromUtf8(lua_tostring(L, 1));
        if (name.isEmpty()) {
            lua_pushnil(L);
            lua_pushstring(L, "deleteArea: bad argument #1 value (an empty string is not a valid area name).");
            return 2;
        } else if (!host.mpMap->mpRoomDB->getAreaNamesMap().values().contains(name)) {
            lua_pushnil(L);
            lua_pushfstring(L,
                            "deleteArea: bad argument #1 value (string \"%s\" is not a valid\n"
                            "area name).",
                            name.toUtf8().constData());
            return 2;
        } else if (name == host.mpMap->mpRoomDB->getDefaultAreaName()) {
            lua_pushnil(L);
            lua_pushfstring(L,
                            "deleteArea: bad argument #1 value (you can't delete the default area).");
            return 2;
        }
    } else {
        lua_pushfstring(L,
                        "deleteArea: bad argument #1 type (area Id as number or area name as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 1));
        lua_error(L);
        return 1;
    }

    bool result = false;
    if (!id) {
        result = host.mpMap->mpRoomDB->removeArea(name);
    } else {
        result = host.mpMap->mpRoomDB->removeArea(id);
    }

    if (result) {
        // Update mapper Area names widget, using method designed for it...!
        if (host.mpMap->mpMapper) {
            host.mpMap->mpMapper->updateAreaComboBox();
        }
        host.mpMap->mMapGraphNeedsUpdate = true;
    }
    lua_pushboolean(L, result);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteRoom
int TLuaInterpreter::deleteRoom(lua_State* L)
{
    int id;

    if (lua_isnumber(L, 1)) {
        id = lua_tonumber(L, 1);
        if (id <= 0) {
            return 0;
        }
    } else {
        lua_pushstring(L, "deleteRoom: wrong argument type");
        lua_error(L);
        return 1;
    }

    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpMap->mpRoomDB->removeRoom(id));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setExit
int TLuaInterpreter::setExit(lua_State* L)
{
    int from, to;
    int dir;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "setExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        from = lua_tointeger(L, 1);
    }

    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "setExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        to = lua_tointeger(L, 2);
    }
    dir = dirToNumber(L, 3);
    if (!dir) {
        lua_pushstring(L, "setExit: wrong argument type");
        lua_error(L);
        return 1;
    }

    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpMap->setExit(from, to, dir));
    host.mpMap->mMapGraphNeedsUpdate = true;
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomCoordinates
int TLuaInterpreter::getRoomCoordinates(lua_State* L)
{
    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "getRoomCoordinates: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tointeger(L, 1);
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    } else {
        lua_pushnumber(L, pR->x);
        lua_pushnumber(L, pR->y);
        lua_pushnumber(L, pR->z);
        return 3;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomArea
int TLuaInterpreter::getRoomArea(lua_State* L)
{
    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "getRoomArea: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tointeger(L, 1);
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, pR->getArea());
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#roomExists
int TLuaInterpreter::roomExists(lua_State* L)
{
    int id;
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 1)) {
        lua_pushstring(L, "roomExists: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tointeger(L, 1);
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addRoom
int TLuaInterpreter::addRoom(lua_State* L)
{
    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "addRoom: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        id = lua_tointeger(L, 1);
    }

    Host& host = getHostFromLua(L);
    bool added = host.mpMap->addRoom(id);
    lua_pushboolean(L, added);
    if (added) {
        host.mpMap->setRoomArea(id, -1, false);
        host.mpMap->mMapGraphNeedsUpdate = true;
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createRoomID
int TLuaInterpreter::createRoomID(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "createRoomID: no map present or loaded!");
        return 2;
    }

    if (lua_gettop(L) > 0) {
        if (!lua_isnumber(L, 1)) {
            lua_pushfstring(L,
                            "createRoomID: bad argument #1 type (minimum room Id as number is optional,\n"
                            "got %s!)",
                            luaL_typename(L, 1));
            lua_error(L);
        } else {
            int minId = lua_tointeger(L, 1);
            if (minId < 1) {
                lua_pushnil(L);
                lua_pushfstring(L,
                                "createRoomID: bad argument #1 value (minimum room id %d is an optional value\n"
                                "but if provided it must be greater than zero.)",
                                minId);
                return 2;
            }
        }
        lua_pushnumber(L, host.mpMap->createNewRoomID(lua_tointeger(L, 1)));
    } else {
        lua_pushnumber(L, host.mpMap->createNewRoomID());
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#unHighlightRoom
int TLuaInterpreter::unHighlightRoom(lua_State* L)
{
    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "unHighlightRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tointeger(L, 1);
    }
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->highlight = false;
        if (host.mpMap) {
            if (host.mpMap->mpMapper) {
                host.mpMap->mpMapper->mp2dMap->update();
            }
        }
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#highlightRoom
int TLuaInterpreter::highlightRoom(lua_State* L)
{
    int id, fgr, fgg, fgb, bgr, bgg, bgb;
    float radius;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "highlightRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tointeger(L, 1);
    }

    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "highlightRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        fgr = lua_tointeger(L, 2);
    }

    if (!lua_isnumber(L, 3)) {
        lua_pushstring(L, "highlightRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        fgg = lua_tointeger(L, 3);
    }

    if (!lua_isnumber(L, 4)) {
        lua_pushstring(L, "highlightRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        fgb = lua_tointeger(L, 4);
    }
    if (!lua_isnumber(L, 5)) {
        lua_pushstring(L, "highlightRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        bgr = lua_tointeger(L, 5);
    }

    if (!lua_isnumber(L, 6)) {
        lua_pushstring(L, "highlightRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        bgg = lua_tointeger(L, 6);
    }

    if (!lua_isnumber(L, 7)) {
        lua_pushstring(L, "highlightRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        bgb = lua_tointeger(L, 7);
    }
    if (!lua_isnumber(L, 8)) {
        lua_pushstring(L, "highlightRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        radius = lua_tonumber(L, 8);
    }
    int alpha1, alpha2;
    if (!lua_isnumber(L, 9)) {
        lua_pushstring(L, "highlightRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        alpha1 = lua_tointeger(L, 9);
    }
    if (!lua_isnumber(L, 10)) {
        lua_pushstring(L, "highlightRoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        alpha2 = lua_tointeger(L, 10);
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        auto fg = QColor(fgr, fgg, fgb, alpha1);
        auto bg = QColor(bgr, bgg, bgb, alpha2);
        pR->highlight = true;
        pR->highlightColor = fg;
        pR->highlightColor2 = bg;
        pR->highlightRadius = radius;

        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                host.mpMap->mpMapper->mp2dMap->update();
            }
        }
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createMapLabel
int TLuaInterpreter::createMapLabel(lua_State* L)
{
    int area, fgr, fgg, fgb, bgr, bgg, bgb;
    float posx, posy, posz;
    int fontSize = 50;
    float zoom = 30.0;
    std::string text;
    bool showOnTop = true;
    bool noScaling = true;

    int args = lua_gettop(L);
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "createMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        area = lua_tointeger(L, 1);
    }

    if (!lua_isstring(L, 2)) {
        lua_pushstring(L, "createMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        text = lua_tostring(L, 2);
    }

    if (!lua_isnumber(L, 3)) {
        lua_pushstring(L, "createMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        posx = lua_tonumber(L, 3);
    }

    if (!lua_isnumber(L, 4)) {
        lua_pushstring(L, "createMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        posy = lua_tonumber(L, 4);
    }

    if (!lua_isnumber(L, 5)) {
        lua_pushstring(L, "createMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        posz = lua_tonumber(L, 5);
    }

    if (!lua_isnumber(L, 6)) {
        lua_pushstring(L, "createMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        fgr = lua_tointeger(L, 6);
    }

    if (!lua_isnumber(L, 7)) {
        lua_pushstring(L, "createMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        fgg = lua_tointeger(L, 7);
    }

    if (!lua_isnumber(L, 8)) {
        lua_pushstring(L, "createMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        fgb = lua_tointeger(L, 8);
    }

    if (!lua_isnumber(L, 9)) {
        lua_pushstring(L, "createMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        bgr = lua_tointeger(L, 9);
    }

    if (!lua_isnumber(L, 10)) {
        lua_pushstring(L, "createMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        bgg = lua_tointeger(L, 10);
    }

    if (!lua_isnumber(L, 11)) {
        lua_pushstring(L, "createMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        bgb = lua_tointeger(L, 11);
    }

    if (args > 11) {
        if (!lua_isnumber(L, 12)) {
            lua_pushstring(L, "createMapLabel: wrong argument type");
            lua_error(L);
            return 1;
        } else {
            zoom = lua_tonumber(L, 12);
        }
        if (!lua_isnumber(L, 13)) {
            lua_pushstring(L, "createMapLabel: wrong argument type");
            lua_error(L);
            return 1;
        } else {
            fontSize = lua_tointeger(L, 13);
        }
        if (args > 13) {
            if (!lua_isboolean(L, 14)) {
                lua_pushstring(L, "createMapLabel: wrong argument type");
                lua_error(L);
                return 1;
            } else {
                showOnTop = lua_toboolean(L, 14);
            }
        }
        if (args > 14) {
            if (!lua_isboolean(L, 15)) {
                lua_pushstring(L, "createMapLabel: wrong argument type");
                lua_error(L);
                return 1;
            } else {
                noScaling = lua_toboolean(L, 15);
            }
        }
    }

    QString _text = text.c_str();
    Host& host = getHostFromLua(L);
    auto fg = QColor(fgr, fgg, fgb);
    auto bg = QColor(bgr, bgg, bgb);
    lua_pushinteger(L, host.mpMap->createMapLabel(area, _text, posx, posy, posz, fg, bg, showOnTop, noScaling, zoom, fontSize));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMapZoom
int TLuaInterpreter::setMapZoom(lua_State* L)
{
    qreal zoom = 3.0;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "setMapZoom: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        zoom = lua_tonumber(L, 1);
    }
    Host& host = getHostFromLua(L);
    if (host.mpMap) {
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                host.mpMap->mpMapper->mp2dMap->setMapZoom(zoom);
                updateMap(L);
            }
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createMapImageLabel
int TLuaInterpreter::createMapImageLabel(lua_State* L)
{
    int area;
    float posx, posy, posz, width, height, zoom;
    std::string text;
    bool showOnTop = true;

    // N/U:     int args = lua_gettop(L);
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "createMapImageLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        area = lua_tointeger(L, 1);
    }

    if (!lua_isstring(L, 2)) {
        lua_pushstring(L, "createMapImageLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        text = lua_tostring(L, 2);
    }

    if (!lua_isnumber(L, 3)) {
        lua_pushstring(L, "createMapImageLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        posx = lua_tonumber(L, 3);
    }

    if (!lua_isnumber(L, 4)) {
        lua_pushstring(L, "createMapImageLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        posy = lua_tonumber(L, 4);
    }

    if (!lua_isnumber(L, 5)) {
        lua_pushstring(L, "createMapImageLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        posz = lua_tonumber(L, 5);
    }

    if (!lua_isnumber(L, 6)) {
        lua_pushstring(L, "createMapImageLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        width = lua_tonumber(L, 6);
    }

    if (!lua_isnumber(L, 7)) {
        lua_pushstring(L, "createMapImageLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        height = lua_tonumber(L, 7);
    }

    if (!lua_isnumber(L, 8)) {
        lua_pushstring(L, "createMapImageLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        zoom = lua_tonumber(L, 8);
    }

    if (!lua_isboolean(L, 9)) {
        lua_pushstring(L, "createMapImageLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        showOnTop = lua_toboolean(L, 9);
    }

    QString _text = text.c_str();
    Host& host = getHostFromLua(L);
    lua_pushinteger(L, host.mpMap->createMapImageLabel(area, _text, posx, posy, posz, width, height, zoom, showOnTop, false));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDoor
int TLuaInterpreter::setDoor(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "setDoor: no map present or loaded!");
        return 2;
    }

    int roomId;
    TRoom* pR;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setDoor: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tointeger(L, 1);
        pR = host.mpMap->mpRoomDB->getRoom(roomId);
        if (!pR) {
            lua_pushnil(L);
            lua_pushfstring(L, "setDoor: bad argument #1 value (number %d is not a valid room id.)", roomId);
            return 2;
        }
    }

    QString exitCmd;
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "setDoor: bad argument #2 type (door command as string expected, got %s!)", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        exitCmd = QString::fromUtf8(lua_tostring(L, 2));
        if (exitCmd.compare(QStringLiteral("n")) && exitCmd.compare(QStringLiteral("e")) && exitCmd.compare(QStringLiteral("s")) && exitCmd.compare(QStringLiteral("w"))
            && exitCmd.compare(QStringLiteral("ne"))
            && exitCmd.compare(QStringLiteral("se"))
            && exitCmd.compare(QStringLiteral("sw"))
            && exitCmd.compare(QStringLiteral("nw"))
            && exitCmd.compare(QStringLiteral("up"))
            && exitCmd.compare(QStringLiteral("down"))
            && exitCmd.compare(QStringLiteral("in"))
            && exitCmd.compare(QStringLiteral("out"))) {
            // One of the above WILL BE ZERO if the exitCmd is ONE of the above QStringLiterals
            // So the above will be TRUE if NONE of above strings match - which
            // means we must treat the exitCmd as a SPECIAL exit
            if (!(pR->getOtherMap().values().contains(exitCmd) || pR->getOtherMap().values().contains(QStringLiteral("0%1").arg(exitCmd))
                  || pR->getOtherMap().values().contains(QStringLiteral("1%1").arg(exitCmd)))) {
                // And NOT a special one either
                lua_pushnil(L);
                lua_pushfstring(L,
                                "setDoor: bad argument #2 value (room with id %d does not have a special\n"
                                "exit in direction \"%s\".)",
                                roomId,
                                exitCmd.toUtf8().constData());
                return 2;
            }
            // else IS a valid special exit - so fall out of if and continue
        } else {
            // Is a normal exit so see if it is valid
            if (!(((!exitCmd.compare(QStringLiteral("n"))) && (pR->getExit(DIR_NORTH) > 0 || pR->exitStubs.contains(DIR_NORTH)))
                  || ((!exitCmd.compare(QStringLiteral("e"))) && (pR->getExit(DIR_EAST) > 0 || pR->exitStubs.contains(DIR_EAST)))
                  || ((!exitCmd.compare(QStringLiteral("s"))) && (pR->getExit(DIR_SOUTH) > 0 || pR->exitStubs.contains(DIR_SOUTH)))
                  || ((!exitCmd.compare(QStringLiteral("w"))) && (pR->getExit(DIR_WEST) > 0 || pR->exitStubs.contains(DIR_WEST)))
                  || ((!exitCmd.compare(QStringLiteral("ne"))) && (pR->getExit(DIR_NORTHEAST) > 0 || pR->exitStubs.contains(DIR_NORTHEAST)))
                  || ((!exitCmd.compare(QStringLiteral("se"))) && (pR->getExit(DIR_SOUTHEAST) > 0 || pR->exitStubs.contains(DIR_SOUTHEAST)))
                  || ((!exitCmd.compare(QStringLiteral("sw"))) && (pR->getExit(DIR_SOUTHWEST) > 0 || pR->exitStubs.contains(DIR_SOUTHWEST)))
                  || ((!exitCmd.compare(QStringLiteral("nw"))) && (pR->getExit(DIR_NORTHWEST) > 0 || pR->exitStubs.contains(DIR_NORTHWEST)))
                  || ((!exitCmd.compare(QStringLiteral("up"))) && (pR->getExit(DIR_UP) > 0 || pR->exitStubs.contains(DIR_UP)))
                  || ((!exitCmd.compare(QStringLiteral("down"))) && (pR->getExit(DIR_DOWN) > 0 || pR->exitStubs.contains(DIR_DOWN)))
                  || ((!exitCmd.compare(QStringLiteral("in"))) && (pR->getExit(DIR_IN) > 0 || pR->exitStubs.contains(DIR_IN)))
                  || ((!exitCmd.compare(QStringLiteral("out"))) && (pR->getExit(DIR_OUT) > 0 || pR->exitStubs.contains(DIR_OUT))))) {
                // No there IS NOT a stub or real exit in the exitCmd direction
                lua_pushnil(L);
                lua_pushfstring(L,
                                "setDoor: bad argument #2 value (room with id %d does not have a normal exit\n"
                                "or a stub exit in direction \"%s\".)",
                                roomId,
                                exitCmd.toUtf8().constData());
                return 2;
            }
            // else IS a valid stub or real normal exit -fall through to continue
        }
    }

    int doorStatus;
    if (!lua_isnumber(L, 3)) {
        lua_pushfstring(L,
                        "setDoor: bad argument #3 type (door type as number expected {0=\"none\",\n"
                        "1=\"open\", 2=\"closed\", 3=\"locked\"}, got %s!)",
                        luaL_typename(L, 3));
        lua_error(L);
        return 1;
    } else {
        doorStatus = lua_tointeger(L, 3);
        if (doorStatus < 0 || doorStatus > 3) {
            lua_pushnil(L);
            lua_pushfstring(L,
                            "setDoor: bad argument #3 value (door type %d is not one of 0=\"none\", 1=\"open\",\n"
                            "2=\"closed\" or 3=\"locked\".)",
                            doorStatus);
            return 2;
        }
    }

    bool result = pR->setDoor(exitCmd, doorStatus);
    if (result) {
        if (host.mpMap->mpMapper && host.mpMap->mpMapper->mp2dMap) {
            host.mpMap->mpMapper->mp2dMap->update();
        }
    }
    lua_pushboolean(L, result);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDoors
int TLuaInterpreter::getDoors(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getDoors: no map present or loaded!");
        return 2;
    }

    int roomId;
    TRoom* pR;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getDoors: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tointeger(L, 1);
        pR = host.mpMap->mpRoomDB->getRoom(roomId);
        if (!pR) {
            lua_pushnil(L);
            lua_pushfstring(L, "getDoors: bad argument #1 value (number %d is not a valid room id).", roomId);
            return 2;
        }
    }

    lua_newtable(L);
    QStringList keys = pR->doors.keys();
    for (unsigned int i = 0, total = keys.size(); i < total; ++i) {
        lua_pushstring(L, keys.at(i).toUtf8().constData());
        lua_pushnumber(L, pR->doors.value(keys.at(i)));
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setExitWeight
int TLuaInterpreter::setExitWeight(lua_State* L)
{
    Host& host = getHostFromLua(L);

    int roomID;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setExitWeight: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        roomID = lua_tointeger(L, 1);
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    if (!pR) {
        lua_pushnil(L);
        lua_pushfstring(L, "room id %d does not exist", roomID);
        return 2;
    }

    QString direction(dirToString(L, 2));
    if (direction.isEmpty()) {
        lua_pushfstring(L, "setExitWeight: bad argument #2 type (direction as string or number {between 1 and 12 inclusive} expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else {
        if (!pR->hasExitOrSpecialExit(direction)) {
            lua_pushnil(L);
            lua_pushfstring(L, "room id %d does not have an exit that can be identified from \"%s\"", roomID, lua_tostring(L, 2));
            return 2;
        }
    }

    qint64 weight;
    if (!lua_isnumber(L, 3)) {
        lua_pushfstring(L, "setExitWeight: bad argument #3 type (exit weight as number expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    } else {
        weight = lua_tonumber(L, 3);
        if (weight < 0 || weight > std::numeric_limits<int>::max()) {
            lua_pushnil(L);
            lua_pushfstring(L, "weight %d is outside of the usable range of 0 (which resets the weight back to that of the destination room) to %d", weight, std::numeric_limits<int>::max());
            return 2;
        }
    }

    pR->setExitWeight(direction, weight);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addCustomLine
int TLuaInterpreter::addCustomLine(lua_State* L)
{
    Host& host = getHostFromLua(L);

    //args: from id, id_to, direction, style, line color, arrow (bool)
    int id_from = 0;
    int id_to = 0;
    int r = 255;
    int g = 0;
    int b = 0;
    Qt::PenStyle line_style(Qt::SolidLine);
    QString direction;
    QList<qreal> x;
    QList<qreal> y;
    QList<int> z;
    bool arrow = false;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "addCustomLine: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        id_from = lua_tointeger(L, 1);
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id_from);
    if (!pR) {
        lua_pushnil(L);
        lua_pushfstring(L, "room id %d does not exist", id_from);
        return 2;
    }

    if (!lua_isnumber(L, 2) && !lua_istable(L, 2)) {
        lua_pushfstring(L, "addCustomLine: bad argument #2 type (target room id as number or coordinate list as table expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else if (lua_isnumber(L, 2)) {
        id_to = lua_tointeger(L, 2);
        TRoom* pR_to = host.mpMap->mpRoomDB->getRoom(id_to);
        if (!pR_to) {
            lua_pushnil(L);
            lua_pushfstring(L, "target room id %d does not exist", id_to);
            return 2;
        } else {
            int area = pR->getArea();
            int area_to = pR_to->getArea();
            if (area != area_to) {
                lua_pushnil(L);
                lua_pushfstring(L,
                                "target room is in area \"%s\" (id: %d) which is not the one \"%s\" (id: %d) in which this custom line is to be drawn",
                                (host.mpMap->mpRoomDB->getAreaNamesMap()).value(area_to).toUtf8().constData(),
                                area_to,
                                (host.mpMap->mpRoomDB->getAreaNamesMap()).value(area).toUtf8().constData(),
                                area);
                return 2;
            }

            x.append(static_cast<qreal>(pR_to->x));
            y.append(static_cast<qreal>(pR_to->y));
            z.append(pR->z);
        }
    } else if (lua_istable(L, 2)) {
        lua_pushnil(L);
        int i = 0; // Indexes groups of coordinates in the table
        while (lua_next(L, 2) != 0) {
            ++i;
            if (lua_type(L, -1) != LUA_TTABLE) {
                lua_pushfstring(L,
                                "addCustomLine: bad argument #2 table item index #%d type (coordinate list must be a table containing tables of three coordinates, got %s as indicated item!)",
                                i,
                                luaL_typename(L, -1));
                return lua_error(L);
            }
            lua_pushnil(L);
            int j = 0; // Indexes items (individual coordinates) in current inner table:
            while (lua_next(L, -2) != 0) {
                ++j;
                if (j <= 3) {
                    if (lua_type(L, -1) != LUA_TNUMBER) {
                        char coordinate = '\0';
                        switch (j) {
                        case 1:
                            coordinate = 'x';
                            break;
                        case 2:
                            coordinate = 'y';
                            break;
                        case 3:
                            coordinate = 'z';
                            break;
                        default:
                            Q_UNREACHABLE();
                        }
                        lua_pushfstring(L,
                                        "addCustomLine: bad argument #2 table item index #%d inner table item #%d type (coordinates list as table containing tables of three numbers (x, y and z "
                                        "coordinates} expected, but got a %s as the %c-coordinate at that index!)",
                                        i,
                                        j,
                                        luaL_typename(L, -1),
                                        coordinate);
                        return lua_error(L);
                    } else {
                        switch (j) {
                        case 1:
                            x.append(lua_tonumber(L, -1));
                            break;
                        case 2:
                            y.append(lua_tonumber(L, -1));
                            break;
                        case 3:
                            z.append(lua_tonumber(L, -1));
                            break;
                        default:; // No-op
                        }
                    }
                }
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
    }

    direction = dirToString(L, 3);
    if (direction.isEmpty()) {
        lua_pushfstring(L, "addCustomLine: bad argument #3 type (direction as string or number (between 1 and 12 inclusive) expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    } else {
        if (!pR->hasExitOrSpecialExit(direction)) {
            lua_pushnil(L);
            lua_pushfstring(L, "room id %d does not have an exit that can be identified from \"%s\"", id_from, lua_tostring(L, 3));
            return 2;
        }
    }

    if (!lua_isstring(L, 4)) {
        lua_pushfstring(L, "addCustomLine: bad argument #4 type (line style as string expected, got %s!)", luaL_typename(L, 4));
        return lua_error(L);
    } else {
        QString lineStyleString = QString::fromUtf8(lua_tostring(L, 4));
        if (!lineStyleString.compare(QLatin1String("solid line"))) {
            line_style = Qt::SolidLine;
        } else if (!lineStyleString.compare(QLatin1String("dot line"))) {
            line_style = Qt::DotLine;
        } else if (!lineStyleString.compare(QLatin1String("dash line"))) {
            line_style = Qt::DashLine;
        } else if (!lineStyleString.compare(QLatin1String("dash dot line"))) {
            line_style = Qt::DashDotLine;
        } else if (!lineStyleString.compare(QLatin1String("dash dot dot line"))) {
            line_style = Qt::DashDotDotLine;
        } else {
            lua_pushnil(L);
            lua_pushfstring(
                    L, "invalid line style \"%s\", only use one of: \"solid line\", \"dot line\", \"dash line\", \"dash dot line\" or \"dash dot dot line\"", lineStyleString.toUtf8().constData());
            return 2;
        }
    }

    if (!lua_istable(L, 5)) {
        lua_pushfstring(L, "addCustomLine: bad argument #5 type (RGB color components as a table expected, got %s!)", luaL_typename(L, 4));
    } else {
        lua_pushnil(L);
        int tind = 0;
        while (lua_next(L, 5) != 0) {
            if (++tind <= 3) {
                if (lua_type(L, -1) != LUA_TNUMBER) {
                    lua_pushfstring(L,
                                    "addCustomLine: bad argument #4 table item #%d type (%s color component as a number between 0 and 255 expected, got %s!)",
                                    tind,
                                    (tind == 1 ? "red" : (tind == 2 ? "green" : "blue")),
                                    luaL_typename(L, -1));
                    lua_error(L);
                    return 1;
                }

                qint64 component = lua_tonumber(L, -1);
                if (component < 0 || component > 255) {
                    lua_pushnil(L);
                    lua_pushfstring(L,
                                    "%s color component in the table of the fourth argument is %d which is out of the valid range (0 to 255)",
                                    (tind == 1 ? "red" : (tind == 2 ? "green" : "blue")),
                                    component);
                    return 2;
                } else {
                    switch (tind) {
                    case 1:
                        r = component;
                        break;
                    case 2:
                        g = component;
                        break;
                    case 3:
                        b = component;
                        break;
                    default:
                        Q_UNREACHABLE();
                    }
                }
            }
            lua_pop(L, 1);
        }
    }

    if (!lua_isboolean(L, 6)) {
        lua_pushfstring(L, "addCustomLine: bad argument #6 type (end with arrow as boolean expected, got %s!)", luaL_typename(L, 6));
        return lua_error(L);
    } else {
        arrow = lua_toboolean(L, 6);
    }

    int lz = z.at(0);
    QList<QPointF> points;
    // TODO: make provision for 3D custom lines (and store the z coordinates and allow them to vary)
    points.append(QPointF(x.at(0), y.at(0)));
    for (int i = 1, total = z.size(); i < total; ++i) {
        if (lz != z.at(i)) {
            lua_pushnil(L);
            lua_pushfstring(L, "the z values are not all on the same level (first wrong value is %d at index %d)", z.at(i), i + 1);
            return 2;
        }
        points.append(QPointF(x.at(i), y.at(i)));
    }

    //Heiko: direction/line relationship must be unique
    pR->customLines[direction] = points;
    pR->customLinesArrow[direction] = arrow;
    pR->customLinesStyle[direction] = line_style;
    pR->customLinesColor[direction] = QColor(r, g, b);

    // Need to update the TRoom {min|max}_{x|y} settings as they are used during
    // the painting process - and not doing that here causes the new line to not
    // show up properly:
    pR->calcRoomDimensions();

    // Better refresh the 2D map to show the new line:
    if (host.mpMap->mpMapper && host.mpMap->mpMapper->mp2dMap) {
        host.mpMap->mpMapper->mp2dMap->mNewMoveAction = true;
        host.mpMap->mpMapper->mp2dMap->update();
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeCustomLine
int TLuaInterpreter::removeCustomLine(lua_State* L)
{
    Host& host = getHostFromLua(L);

    //args: room_id, direction
    int roomId = 0;
    QString direction;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "removeCustomLine: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        roomId = lua_tointeger(L, 1);
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        lua_pushnil(L);
        lua_pushfstring(L, "room id %d does not exist", roomId);
        return 2;
    }

    direction = dirToString(L, 2);
    if (direction.isEmpty()) {
        lua_pushfstring(L, "removeCustomLine: bad argument #2 type (direction as string or number (between 1 and 12 inclusive) expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else {
        if (!pR->hasExitOrSpecialExit(direction)) {
            lua_pushnil(L);
            lua_pushfstring(L, "room id %d does not have an exit that can be identified from \"%s\"", roomId, lua_tostring(L, 2));
            return 2;
        }
    }

    if ((pR->customLines.remove(direction) + pR->customLinesArrow.remove(direction) + pR->customLinesStyle.remove(direction) + pR->customLinesColor.remove(direction)) > 0) {
        // Need to update the TRoom {min|max}_{x|y} settings as they are used during
        // the painting process:
        pR->calcRoomDimensions();
        // Better refresh the 2D map to show the new line:
        if (host.mpMap->mpMapper && host.mpMap->mpMapper->mp2dMap) {
            host.mpMap->mpMapper->mp2dMap->mNewMoveAction = true;
            host.mpMap->mpMapper->mp2dMap->update();
        }
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "room id %d does not appear to have a custom exit line for the exit indentifed from  \"%s\"", roomId, lua_tostring(L, 2));
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCustomLines
int TLuaInterpreter::getCustomLines(lua_State* L)
{
    int roomID;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getCustomLines: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        roomID = lua_tointeger(L, 1);
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    if (pR) {
        lua_newtable(L); //return table customLines[]
        QStringList exits = pR->customLines.keys();
        for (int i = 0, iTotal = exits.size(); i < iTotal; ++i) {
            lua_pushstring(L, exits.at(i).toUtf8().constData());
            lua_newtable(L); //customLines[direction]
            lua_pushstring(L, "attributes");
            lua_newtable(L); //customLines[attributes]
            lua_pushstring(L, "style");
            switch (pR->customLinesStyle.value(exits.at(i))) {
            case Qt::DotLine:
                lua_pushstring(L, "dot line");
                break;
            case Qt::DashLine:
                lua_pushstring(L, "dash line");
                break;
            case Qt::DashDotLine:
                lua_pushstring(L, "dash dot line");
                break;
            case Qt::DashDotDotLine:
                lua_pushstring(L, "dash dot dot line");
                break;
            case Qt::SolidLine:
                [[fallthrough]];
            default:
                lua_pushstring(L, "solid line");
            }
            lua_settable(L, -3);
            lua_pushstring(L, "arrow");
            lua_pushboolean(L, pR->customLinesArrow.value(exits.at(i)));
            lua_settable(L, -3);
            lua_pushstring(L, "color");
            lua_newtable(L);
            lua_pushstring(L, "r");
            lua_pushinteger(L, pR->customLinesColor.value(exits.at(i)).red());
            lua_settable(L, -3);
            lua_pushstring(L, "g");
            lua_pushinteger(L, pR->customLinesColor.value(exits.at(i)).green());
            lua_settable(L, -3);
            lua_pushstring(L, "b");
            lua_pushinteger(L, pR->customLinesColor.value(exits.at(i)).blue());
            lua_settable(L, -3);
            lua_settable(L, -3); //color
            lua_settable(L, -3); //attributes
            lua_pushstring(L, "points");
            lua_newtable(L); //customLines[points]
            QList<QPointF> pointL = pR->customLines.value(exits.at(i));
            for (int k = 0, kTotal = pointL.size(); k < kTotal; ++k) {
                lua_pushnumber(L, k);
                lua_newtable(L);
                lua_pushstring(L, "x");
                lua_pushnumber(L, pointL.at(k).x());
                lua_settable(L, -3);
                lua_pushstring(L, "y");
                lua_pushnumber(L, pointL.at(k).y());
                lua_settable(L, -3);
                lua_settable(L, -3);
            }
            lua_settable(L, -3); //customLines[direction][points]
            lua_settable(L, -3); //customLines[direction]
        }
        return 1;
    } else {
        lua_pushnil(L); //if the room doesnt exist return nil
        lua_pushfstring(L, "room %d doesn't exist", roomID);
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getExitWeights
int TLuaInterpreter::getExitWeights(lua_State* L)
{
    int roomID;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "getExitWeights: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        roomID = lua_tointeger(L, 1);
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    lua_newtable(L);
    if (pR) {
        QStringList keys = pR->getExitWeights().keys();
        for (int i = 0; i < keys.size(); i++) {
            lua_pushstring(L, keys[i].toLatin1().data());
            lua_pushnumber(L, pR->getExitWeight(keys[i]));
            lua_settable(L, -3);
        }
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteMapLabel
int TLuaInterpreter::deleteMapLabel(lua_State* L)
{
    int area, labelID;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "deleteMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        area = lua_tointeger(L, 1);
    }
    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "deleteMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        labelID = lua_tointeger(L, 2);
    }
    Host& host = getHostFromLua(L);
    host.mpMap->deleteMapLabel(area, labelID);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapLabels
int TLuaInterpreter::getMapLabels(lua_State* L)
{
    int area;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "getMapLabels: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        area = lua_tointeger(L, 1);
    }
    Host& host = getHostFromLua(L);
    if (host.mpMap->mapLabels.contains(area)) {
        lua_newtable(L);
        QMapIterator<int, TMapLabel> it(host.mpMap->mapLabels[area]);
        while (it.hasNext()) {
            it.next();
            lua_pushnumber(L, it.key());
            lua_pushstring(L, it.value().text.toLatin1().data());
            lua_settable(L, -3);
        }
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapLabel
int TLuaInterpreter::getMapLabel(lua_State* L)
{
    int area, labelId = -1;
    QString labelText;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "getMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        area = lua_tointeger(L, 1);
    }
    if (!lua_isstring(L, 2) && !lua_isnumber(L, 2)) {
        lua_pushstring(L, "getMapLabel: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        if (lua_isnumber(L, 2)) {
            labelId = lua_tointeger(L, 2);
        } else {
            labelText = lua_tostring(L, 2);
        }
    }
    Host& host = getHostFromLua(L);
    if (host.mpMap->mapLabels.contains(area)) {
        lua_newtable(L);
        if (labelId != -1) {
            if (host.mpMap->mapLabels[area].contains(labelId)) {
                TMapLabel label = host.mpMap->mapLabels[area][labelId];
                int x = label.pos.x();
                int y = label.pos.y();
                int z = label.pos.z();
                float height = label.size.height();
                float width = label.size.width();
                QString text = label.text;
                lua_pushstring(L, "X");
                lua_pushnumber(L, x);
                lua_settable(L, -3);
                lua_pushstring(L, "Y");
                lua_pushnumber(L, y);
                lua_settable(L, -3);
                lua_pushstring(L, "Z");
                lua_pushnumber(L, z);
                lua_settable(L, -3);
                lua_pushstring(L, "Height");
                lua_pushnumber(L, height);
                lua_settable(L, -3);
                lua_pushstring(L, "Width");
                lua_pushnumber(L, width);
                lua_settable(L, -3);
                lua_pushstring(L, "Text");
                lua_pushstring(L, text.toLatin1().data());
                lua_settable(L, -3);
            } else {
                lua_pushstring(L, "getMapLabel: labelId doesn't exist");
                lua_error(L);
                return 1;
            }
        } else {
            QMapIterator<int, TMapLabel> it(host.mpMap->mapLabels[area]);
            while (it.hasNext()) {
                it.next();
                if (it.value().text == labelText) {
                    TMapLabel label = it.value();
                    lua_newtable(L);
                    int id = it.key();
                    int x = label.pos.x();
                    int y = label.pos.y();
                    int z = label.pos.z();
                    float height = label.size.height();
                    float width = label.size.width();
                    QString text = label.text;
                    lua_pushstring(L, "X");
                    lua_pushnumber(L, x);
                    lua_settable(L, -3);
                    lua_pushstring(L, "Y");
                    lua_pushnumber(L, y);
                    lua_settable(L, -3);
                    lua_pushstring(L, "Z");
                    lua_pushnumber(L, z);
                    lua_settable(L, -3);
                    lua_pushstring(L, "Height");
                    lua_pushnumber(L, height);
                    lua_settable(L, -3);
                    lua_pushstring(L, "Width");
                    lua_pushnumber(L, width);
                    lua_settable(L, -3);
                    lua_pushstring(L, "Text");
                    lua_pushstring(L, text.toLatin1().data());
                    lua_settable(L, -3);
                    lua_pushnumber(L, id);
                    lua_insert(L, -2);
                    lua_settable(L, -3);
                }
            }
        }
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addSpecialExit
int TLuaInterpreter::addSpecialExit(lua_State* L)
{
    int id_from, id_to;
    std::string dir;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "addSpecialExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id_from = lua_tointeger(L, 1);
    }
    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "addSpecialExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id_to = lua_tointeger(L, 2);
    }
    if (!lua_isstring(L, 3)) {
        lua_pushstring(L, "addSpecialExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        dir = lua_tostring(L, 3);
    }
    QString _dir = dir.c_str();
    Host& host = getHostFromLua(L);
    TRoom* pR_from = host.mpMap->mpRoomDB->getRoom(id_from);
    TRoom* pR_to = host.mpMap->mpRoomDB->getRoom(id_to);
    if (pR_from && pR_to) {
        pR_from->setSpecialExit(id_to, _dir);
        pR_from->setSpecialExitLock(id_to, _dir, false);
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeSpecialExit
int TLuaInterpreter::removeSpecialExit(lua_State* L)
{
    int id;
    std::string dir;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "removeSpecialExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id = lua_tointeger(L, 1);
    }
    if (!lua_isstring(L, 2)) {
        lua_pushstring(L, "removeSpecialExit: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        dir = lua_tostring(L, 2);
    }
    QString _dir = dir.c_str();
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->setSpecialExit(-1, _dir);
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearRoomUserData
int TLuaInterpreter::clearRoomUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "clearRoomUserData: no map present or loaded!");
        return 2;
    }

    int roomId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "clearRoomUserData: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tointeger(L, 1);
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        lua_pushnil(L);
        lua_pushfstring(L, "clearRoomUserData: bad argument #1 value (number %d is not a valid room id).", roomId);
        return 2;
    } else {
        if (!pR->userData.isEmpty()) {
            pR->userData.clear();
            lua_pushboolean(L, true);
        } else {
            lua_pushboolean(L, false);
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearRoomUserDataItem
int TLuaInterpreter::clearRoomUserDataItem(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "clearRoomUserDataItem: no map present or loaded!");
        return 2;
    }

    int roomId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L,
                        "clearRoomUserDataItem: bad argument #1 type (room id as number expected,\n"
                        "got %s!)",
                        luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tointeger(L, 1);
    }

    QString key = QString(); // This assigns the null value which is different from an empty one
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, R"(clearRoomUserDataItem: bad argument #2 type ("key" as string expected, got %s!))", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        key = QString::fromUtf8(lua_tostring(L, 2));
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        lua_pushnil(L);
        lua_pushfstring(L, "clearRoomUserDataItem: bad argument #1 value (number %d is not a valid room id).", roomId);
        return 2;
    } else {
        // Turns out that an empty key IS possible, but if this changes this should be uncommented
        //        if (key.isEmpty()) {
        //            // If the user accidently supplied an white-space only or empty key
        //            // string we don't do anything, but we, sucessfully, fail to do it... 8-)
        //            lua_pushboolean( L, false );
        //        }
        /*      else */ if (pR->userData.contains(key)) {
            pR->userData.remove(key);
            lua_pushboolean(L, true);
        } else {
            lua_pushboolean(L, false);
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearAreaUserData
int TLuaInterpreter::clearAreaUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "clearAreaUserData: no map present or loaded!");
        return 2;
    }

    int areaId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "clearAreaUserData: bad argument #1 type (area id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        areaId = lua_tointeger(L, 1);
    }

    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        lua_pushnil(L);
        lua_pushfstring(L, "clearAreaUserData: bad argument #1 value (number %d is not a valid area id).", areaId);
        return 2;
    } else {
        if (!pA->mUserData.isEmpty()) {
            pA->mUserData.clear();
            lua_pushboolean(L, true);
        } else {
            lua_pushboolean(L, false);
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearAreaUserDataItem
int TLuaInterpreter::clearAreaUserDataItem(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "clearAreaUserDataItem: no map present or loaded!");
        return 2;
    }

    int areaId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "clearAreaUserDataItem: bad argument #1 type (area id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        areaId = lua_tointeger(L, 1);
    }

    QString key = QString(); // This assigns the null value which is different from an empty one
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, R"(clearAreaUserDataItem: bad argument #2 type ("key" as string expected, got %s!))", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        key = QString::fromUtf8(lua_tostring(L, 2));
    }

    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        lua_pushnil(L);
        lua_pushfstring(L, "clearAreaUserDataItem: bad argument #1 value (number %d is not a valid area id).", areaId);
        return 2;
    } else {
        if (key.isEmpty()) {
            lua_pushnil(L);
            lua_pushfstring(L, R"(clearAreaUserDataItem: bad argument #2 value ("key" can not be an empty string).)");
            return 2;
        } else {
            lua_pushboolean(L, (pA->mUserData.remove(key) > 0));
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearMapUserData
int TLuaInterpreter::clearMapUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        lua_pushnil(L);
        lua_pushstring(L, "clearMapUserData: no map present or loaded!");
        return 2;
    }

    if (!host.mpMap->mUserData.isEmpty()) {
        host.mpMap->mUserData.clear();
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearMapUserDataItem
int TLuaInterpreter::clearMapUserDataItem(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        lua_pushnil(L);
        lua_pushstring(L, "clearMapUserDataItem: no map present or loaded!");
        return 2;
    }

    QString key = QString(); // This assigns the null value which is different from an empty one
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, R"(clearMapUserDataItem: bad argument #1 type ("key" as string expected, got %s!))", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        key = QString::fromUtf8(lua_tostring(L, 1));
        if (key.isEmpty()) {
            lua_pushnil(L);
            lua_pushfstring(L, R"(clearMapUserDataItem: bad argument #1 value ("key" can not be an empty string).)");
            return 2;
        } else {
            lua_pushboolean(L, (host.mpMap->mUserData.remove(key) > 0));
            return 1;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearSpecialExits
int TLuaInterpreter::clearSpecialExits(lua_State* L)
{
    int id_from;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "clearSpecialExits: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id_from = lua_tointeger(L, 1);
    }
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id_from);
    if (pR) {
        pR->clearSpecialExits();
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getSpecialExits
int TLuaInterpreter::getSpecialExits(lua_State* L)
{
    int id_from;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "getSpecialExits: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id_from = lua_tointeger(L, 1);
    }
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id_from);
    if (pR) {
        QMapIterator<int, QString> it(pR->getOtherMap());
        lua_newtable(L);
        while (it.hasNext()) {
            it.next();
            lua_newtable(L);
            int id_to = it.key();
            QString dir = it.value();
            QString exitStatus;
            if (dir.size() > 0 && (dir.startsWith('0') || dir.startsWith('1'))) {
                exitStatus = dir.left(1);
            } else {
                exitStatus = "0";
            }
            QString exit;
            if (dir.size() > 0 && (dir.startsWith('0') || dir.startsWith('1'))) {
                exit = dir.remove(0, 1);
            } else {
                exit = dir;
            }
            lua_pushstring(L, exit.toLatin1().data());       //done to remove the prepended special exit status
            lua_pushstring(L, exitStatus.toLatin1().data()); //done to remove the prepended special exit status
            lua_settable(L, -3);
            lua_pushnumber(L, id_to);
            lua_insert(L, -2);
            lua_settable(L, -3);
        }
        return 1;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getSpecialExitsSwap
int TLuaInterpreter::getSpecialExitsSwap(lua_State* L)
{
    int id_from;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "getSpecialExitsSwap: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        id_from = lua_tointeger(L, 1);
    }
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id_from);
    if (pR) {
        QMapIterator<int, QString> it(pR->getOtherMap());
        lua_newtable(L);
        while (it.hasNext()) {
            it.next();
            int id_to = it.key();
            QString dir = it.value();
            // lua_pushstring(L, dir.toLatin1().data());
            QString exitStatus;
            QString exit;
            if (dir.size() > 0 && (dir.startsWith('0') || dir.startsWith('1'))) {
                exitStatus = dir.left(1);
            } else {
                exitStatus = "0";
            }

            if (dir.size() > 0 && (dir.startsWith('0') || dir.startsWith('1'))) {
                exit = dir.remove(0, 1);
            } else {
                exit = dir;
            }
            lua_pushstring(L, exit.toLatin1().data());
            lua_pushnumber(L, id_to);
            lua_settable(L, -3);
        }
        return 1;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomEnv
int TLuaInterpreter::getRoomEnv(lua_State* L)
{
    int roomID;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "getRoomEnv: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        roomID = lua_tointeger(L, 1);
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    if (pR) {
        lua_pushnumber(L, pR->environment);
        return 1;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomUserData
int TLuaInterpreter::getRoomUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getRoomUserData: no map present or loaded!");
        return 2;
    }

    int roomId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getRoomUserData: bad argument #1 (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tointeger(L, 1);
    }

    QString key;
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "getRoomUserData: bad argument #2 (key as string expected, got %s!)", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        key = QString::fromUtf8(lua_tostring(L, 2));
    }

    bool isBackwardCompatibilityRequired = true;
    if (lua_gettop(L) > 2) {
        if (!lua_isboolean(L, 3)) {
            lua_pushfstring(L,
                            "getRoomUserData: bad argument #3 (enableFullErrorReporting as boolean {default\n"
                            "= false} is optional, got %s!)",
                            luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            isBackwardCompatibilityRequired = !lua_toboolean(L, 3);
        }
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        if (isBackwardCompatibilityRequired) {
            lua_pushstring(L, QString().toUtf8().constData());
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, "getRoomUserData: bad argument #1 value (number %d is not a valid room id).", roomId);
            return 2;
        }
    } else {
        if (pR->userData.contains(key)) {
            lua_pushstring(L, pR->userData.value(key).toUtf8().constData());
            return 1;
        } else {
            if (isBackwardCompatibilityRequired) {
                lua_pushstring(L, QString().toUtf8().constData());
                return 1;
            } else {
                lua_pushnil(L);
                lua_pushfstring(L, R"(getRoomUserData: bad argument #2 value (no user data with key:"%s" in room with id: %d).)", key.toUtf8().constData(), roomId);
                return 2;
            }
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAreaUserData
int TLuaInterpreter::getAreaUserData(lua_State* L)
{
    int areaId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getAreaUserData: bad argument #1 (area id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        areaId = lua_tointeger(L, 1);
    }

    QString key;
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "getAreaUserData: bad argument #2 (key as string expected, got %s!)", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        key = QString::fromUtf8(lua_tostring(L, 2));
        if (key.isEmpty()) {
            lua_pushnil(L);
            lua_pushstring(L,
                           "getAreaUserData: bad argument #2 value (\"key\" is not allowed to be an\n"
                           "empty string).");
            return 2;
        }
    }

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getAreaUserData: no map present or loaded!");
        return 2;
    }
    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        lua_pushnil(L);
        lua_pushfstring(L, "getAreaUserData: bad argument #1 value (number %d is not a valid area id).", areaId);
        return 2;
    } else {
        if (pA->mUserData.contains(key)) {
            lua_pushstring(L, pA->mUserData.value(key).toUtf8().constData());
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L,
                            "getAreaUserData: bad argument #2 value (no user data with key:\"%s\"\n"
                            "in area with id:%d).",
                            key.toUtf8().constData(),
                            areaId);
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapUserData
int TLuaInterpreter::getMapUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        lua_pushnil(L);
        lua_pushstring(L, "getMapUserData: no map present or loaded!");
        return 2;
    }

    QString key;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "getMapUserData: bad argument #1 (key as string expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        key = QString::fromUtf8(lua_tostring(L, 1));
    }

    if (host.mpMap->mUserData.contains(key)) {
        lua_pushstring(L, host.mpMap->mUserData.value(key).toUtf8().constData());
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, R"(getMapUserData: bad argument #1 value (no user data with key:"%s" in map).)", key.toUtf8().constData());
        return 2;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomUserData
int TLuaInterpreter::setRoomUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "setRoomUserData: no map present or loaded!");
        return 2;
    }

    int roomId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setRoomUserData: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tointeger(L, 1);
    }

    QString key;
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, R"(setRoomUserData: bad argument #2 type ("key" as string expected, got %s!))", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        // Ideally should reject empty keys but this could break existing scripts so we can't
        key = QString::fromUtf8(lua_tostring(L, 2));
    }

    QString value;
    if (!lua_isstring(L, 3)) {
        lua_pushfstring(L, R"(setRoomUserData: bad argument #3 type ("value" as string expected, got %s!))", luaL_typename(L, 3));
        lua_error(L);
        return 1;
    } else {
        value = QString::fromUtf8(lua_tostring(L, 3));
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        lua_pushnil(L);
        lua_pushfstring(L, "setRoomUserData: bad argument #1 value (number %d is not a valid room id).", roomId);
        return 2;
    } else {
        pR->userData[key] = value;
        lua_pushboolean(L, true);
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setAreaUserData
int TLuaInterpreter::setAreaUserData(lua_State* L)
{
    int areaId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setAreaUserData: bad argument #1 type (area id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        areaId = lua_tointeger(L, 1);
    }

    QString key;
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, R"(setAreaUserData: bad argument #2 type ("key" as string expected, got %s!))", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        key = QString::fromUtf8(lua_tostring(L, 2));
        if (key.isEmpty()) {
            lua_pushnil(L);
            lua_pushstring(L,
                           "setAreaUserData: bad argument #2 value (\"key\" is not allowed to be an\n"
                           "empty string).");
            return 2;
        }
    }

    QString value;
    if (!lua_isstring(L, 3)) {
        lua_pushfstring(L, R"(setAreaUserData: bad argument #3 type ("value" as string expected, got %s!))", luaL_typename(L, 3));
        lua_error(L);
        return 1;
    } else {
        value = QString::fromUtf8(lua_tostring(L, 3));
    }

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "setAreaUserData: no map present or loaded!");
        return 2;
    }

    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        lua_pushnil(L);
        lua_pushfstring(L, "setAreaUserData: bad argument #1 value (number %d is not a valid area id).", areaId);
        return 2;
    } else {
        pA->mUserData[key] = value;
        lua_pushboolean(L, true);
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMapUserData
int TLuaInterpreter::setMapUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        lua_pushnil(L);
        lua_pushstring(L, "setMapUserData: no map present or loaded!");
        return 2;
    }

    QString key;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, R"(setMapUserData: bad argument #1 type ("key" as string expected, got %s!))", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        key = QString::fromUtf8(lua_tostring(L, 1));
        if (key.isEmpty()) {
            lua_pushnil(L);
            lua_pushfstring(L, R"(setMapUserData: bad argument #1 value ("key" is not allowed to be an empty string).)");
            return 2;
        }
    }

    QString value;
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, R"(setMapUserData: bad argument #2 type ("value" as string expected, got %s!))", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        value = QString::fromUtf8(lua_tostring(L, 2));
    }

    host.mpMap->mUserData[key] = value;
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomUserDataKeys
int TLuaInterpreter::getRoomUserDataKeys(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getRoomUserDataKeys: no map present or loaded!");
        return 2;
    }

    int roomId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getRoomUserDataKeys: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tointeger(L, 1);
    }

    QStringList keys;
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        lua_pushnil(L);
        lua_pushfstring(L, "getRoomUserDataKeys: bad argument #1 value (number %d is not a valid room id).", roomId);
        return 2;
    } else {
        keys = pR->userData.keys();
        lua_newtable(L);
        for (int i = 0; i < keys.size(); i++) {
            lua_pushnumber(L, i + 1);
            lua_pushstring(L, keys.at(i).toUtf8().constData());
            lua_settable(L, -3);
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAllRoomUserData
int TLuaInterpreter::getAllRoomUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getAllRoomUserData: no map present or loaded!");
        return 2;
    }

    int roomId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getAllRoomUserData: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        roomId = lua_tointeger(L, 1);
    }

    QStringList keys;
    QStringList values;
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        lua_pushnil(L);
        lua_pushfstring(L, "getAllRoomUserData: bad argument #1 value (number %d is not a valid room id).", roomId);
        return 2;
    } else {
        keys = pR->userData.keys();
        values = pR->userData.values();
        lua_newtable(L);
        for (int i = 0; i < keys.size(); i++) {
            lua_pushstring(L, keys.at(i).toUtf8().constData());
            lua_pushstring(L, values.at(i).toUtf8().constData());
            lua_settable(L, -3);
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAllAreaUserData
int TLuaInterpreter::getAllAreaUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "getAllAreaUserData: no map present or loaded!");
        return 2;
    }

    int areaId;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getAllAreaUserData: bad argument #1 type (area id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        areaId = lua_tointeger(L, 1);
    }

    QStringList keys;
    QStringList values;
    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        lua_pushnil(L);
        lua_pushfstring(L, "getAllAreaUserData: bad argument #1 value (number %d is not a valid area id).", areaId);
        return 2;
    } else {
        keys = pA->mUserData.keys();
        values = pA->mUserData.values();
        lua_newtable(L);
        for (int i = 0; i < keys.size(); i++) {
            lua_pushstring(L, keys.at(i).toUtf8().constData());
            lua_pushstring(L, values.at(i).toUtf8().constData());
            lua_settable(L, -3);
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAllMapUserData
int TLuaInterpreter::getAllMapUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        lua_pushnil(L);
        lua_pushstring(L, "getAllMapUserData: no map present or loaded!");
        return 2;
    }

    QStringList keys;
    QStringList values;
    keys = host.mpMap->mUserData.keys();
    values = host.mpMap->mUserData.values();
    lua_newtable(L);
    for (int i = 0; i < keys.size(); i++) {
        lua_pushstring(L, keys.at(i).toUtf8().constData());
        lua_pushstring(L, values.at(i).toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#downloadFile
int TLuaInterpreter::downloadFile(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString localFile;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "downloadFile: bad argument #1 type (local filename as string expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        localFile = QString::fromUtf8(lua_tostring(L, 1));
    }

    QString urlString;
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "downloadFile: bad argument #2 type (remote url as string expected, got %s!)", luaL_typename(L, 2));
        lua_error(L);
        return 1;
    } else {
        urlString = QString::fromUtf8(lua_tostring(L, 2));
    }

    QUrl url = QUrl::fromUserInput(urlString);

    if (!url.isValid()) {
        lua_pushnil(L);
        lua_pushfstring(L,
                        "downloadFile: bad argument #2 value (url is not deemed valid), validation\n"
                        "produced the following error message:\n%s.",
                        url.errorString().toUtf8().constData());
        return 2;
    }

    QNetworkRequest request = QNetworkRequest(url);
    mudlet::self()->setNetworkRequestDefaults(url, request);

    host.updateProxySettings(host.mLuaInterpreter.mpFileDownloader);
    QNetworkReply* reply = host.mLuaInterpreter.mpFileDownloader->get(request);
    host.mLuaInterpreter.downloadMap.insert(reply, localFile);

    if (mudlet::debugMode) {
        TDebug(QColor(Qt::white), QColor(Qt::blue)) << "downloadFile: script is downloading from " << reply->url().toString() << "\n" >> 0;
    }

    lua_pushboolean(L, true);
    lua_pushstring(L, reply->url().toString().toUtf8().constData()); // Returns the Url that was ACTUALLY used
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomArea
int TLuaInterpreter::setRoomArea(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "setRoomArea: no map present or loaded!");
        return 2;
    }

    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setRoomArea: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        id = lua_tointeger(L, 1);
        if (!host.mpMap->mpRoomDB->getRoomIDList().contains(id)) {
            lua_pushnil(L);
            lua_pushfstring(L, "setRoomArea: bad argument #1 value (number %d is not a valid room id).", id);
            return 2;
        }
    }

    int areaId;
    QString areaName;
    if (lua_isnumber(L, 2)) {
        areaId = lua_tonumber(L, 2);
        if (areaId < 1) {
            lua_pushnil(L);
            lua_pushfstring(L,
                            "setRoomArea: bad argument #2 value (number %d is not a valid area id greater\n"
                            "than zero.  To remove a room's area, use resetRoomArea( roomId ) ).",
                            areaId);
            return 2;
        } else if (!host.mpMap->mpRoomDB->getAreaNamesMap().contains(areaId)) {
            lua_pushnil(L);
            lua_pushfstring(L, "setRoomArea: bad argument #2 value (number %d is not a valid area id as it does not exist).", areaId);
            return 2;
        }
    } else if (lua_isstring(L, 2)) {
        areaName = QString::fromUtf8(lua_tostring(L, 2));
        // areaId will be zero if not found!
        if (areaName.isEmpty()) {
            lua_pushnil(L);
            lua_pushstring(L, "setRoomArea: bad argument #2 value (area name cannot be empty).");
            return 2;
        }
        areaId = host.mpMap->mpRoomDB->getAreaNamesMap().key(areaName, 0);
        if (!areaId) {
            lua_pushnil(L);
            lua_pushfstring(L, R"(setRoomArea: bad argument #2 value (area name "%s" does not exist).)", areaName.toUtf8().constData());
            return 2;
        }
    } else {
        lua_pushfstring(L,
                        "setRoomArea: bad argument #2 type (area Id as number or area name as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 2));
        lua_error(L);
        return 1;
    }

    // Can set the room to an area which does not have a TArea instance but does
    // appear in the TRoomDB::areaNamesMap...
    bool result = host.mpMap->setRoomArea(id, areaId, false);
    if (result) {
        // As a sucessfull result WILL change the area a room is in then the map
        // should be updated.  The GUI code that modifies room(s) areas already
        // includes such a call to update the mapper.
        if (host.mpMap->mpMapper) {
            host.mpMap->mpMapper->mp2dMap->update();
        }
#if defined(INCLUDE_3DMAPPER)
        if (host.mpMap->mpM) {
            host.mpMap->mpM->update();
        }
#endif
    }
    lua_pushboolean(L, result);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetRoomArea
int TLuaInterpreter::resetRoomArea(lua_State* L)
{
    //will reset the room area to our void area
    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "resetRoomArea: bad argument #1 type (room id as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        id = lua_tointeger(L, 1);
    }

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "resetRoomArea: no map present or loaded!");
        return 2;
    } else if (!host.mpMap->mpRoomDB->getRoomIDList().contains(id)) {
        lua_pushnil(L);
        lua_pushfstring(L, "resetRoomArea: bad argument #1 value (number %d is not a valid room id).", id);
        return 2;
    } else {
        bool result = host.mpMap->setRoomArea(id, -1, false);
        if (result) {
            // As a sucessfull result WILL change the area a room is in then the map
            // should be updated.  The GUI code that modifies room(s) areas already
            // includes such a call to update the mapper.
            if (host.mpMap->mpMapper) {
                host.mpMap->mpMapper->mp2dMap->update();
            }
#if defined(INCLUDE_3DMAPPER)
            if (host.mpMap->mpM) {
                host.mpMap->mpM->update();
            }
#endif
        }
        lua_pushboolean(L, result);
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomChar
int TLuaInterpreter::setRoomChar(lua_State* L)
{
    int id;
    QString symbol;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "setRoomChar: bad argument #1 type (room id as number expected, got %s!)",
                       luaL_typename(L, 1));
        return lua_error(L);
    } else {
        id = lua_tointeger(L, 1);
    }

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "setRoomChar: bad argument #2 type (room symbol as string expected, got %s!)",
                       luaL_typename(L, 2));
        return lua_error(L);
    } else {
        symbol = QString::fromUtf8(lua_tostring(L, 2));
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        lua_pushnil(L);
        lua_pushfstring(L, "room with id %d does not exist", id);
        return 2;
    } else {
        if (symbol.isEmpty()) {
            // Allow an empty string to be used to clear the symbol:
            pR->mSymbol.clear();
        } else {
            // 8.0 is the maximum supported by the Qt versions (5.6 to 5.10) we
            // handle/use/allow:
            pR->mSymbol = symbol.normalized(QString::NormalizationForm_C, QChar::Unicode_8_0);
        }
        lua_pushboolean(L, true);
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomChar
int TLuaInterpreter::getRoomChar(lua_State* L)
{
    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getRoomChar: bad argument #1 type (room id as number expected, got %s!)",
                       luaL_typename(L, 1));
        return lua_error(L);
    } else {
        id = lua_tointeger(L, 1);
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        lua_pushnil(L);
        lua_pushfstring(L, "room with id %d does not exist", id);
        return 2;
    } else {
        lua_pushstring(L, pR->mSymbol.toUtf8().constData());
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomsByPosition
int TLuaInterpreter::getRoomsByPosition(lua_State* L)
{
    int area, x, y, z;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "getRoomsByPosition: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        area = lua_tointeger(L, 1);
    }
    if (!lua_isnumber(L, 2)) {
        lua_pushstring(L, "getRoomsByPosition: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        x = lua_tointeger(L, 2);
    }
    if (!lua_isnumber(L, 3)) {
        lua_pushstring(L, "getRoomsByPosition: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        y = lua_tointeger(L, 3);
    }
    if (!lua_isnumber(L, 4)) {
        lua_pushstring(L, "getRoomsByPosition: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        z = lua_tointeger(L, 4);
    }


    Host& host = getHostFromLua(L);
    TArea* pA = host.mpMap->mpRoomDB->getArea(area);
    if (!pA) {
        lua_pushnil(L);
        return 1;
    }

    QList<int> rL = pA->getRoomsByPosition(x, y, z);
    lua_newtable(L);
    for (int i = 0; i < rL.size(); i++) {
        lua_pushnumber(L, i);
        lua_pushnumber(L, rL[i]);
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getGridMode
int TLuaInterpreter::getGridMode(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "no map present or loaded!");
        return 2;
    }

    int id;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getGridMode: bad argument #1 type (area id as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        id = lua_tonumber(L, 1);
    }

    TArea* area = host.mpMap->mpRoomDB->getArea(id);
    if (!area) {
        lua_pushnil(L);
        lua_pushfstring(L, "area with id %d does not exist", id);
        return 2;
    } else {
        lua_pushboolean(L, area->gridMode);
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setGridMode
int TLuaInterpreter::setGridMode(lua_State* L)
{
    int area;
    bool gridMode = false;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "setGridMode: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        area = lua_tointeger(L, 1);
    }
    if (!lua_isboolean(L, 2)) {
        lua_pushstring(L, "setGridMode: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        gridMode = lua_toboolean(L, 2);
    }

    Host& host = getHostFromLua(L);
    TArea* pA = host.mpMap->mpRoomDB->getArea(area);
    if (!pA) {
        lua_pushboolean(L, false);
        return 1;
    } else {
        pA->gridMode = gridMode;
        pA->calcSpan();
        if (host.mpMap->mpMapper) {
            if (host.mpMap->mpMapper->mp2dMap) {
                // Not needed IMHO - Slysven
                //                host.mpMap->mpMapper->mp2dMap->init();
                //                cout << "NEW GRID MAP: init" << endl;
                // But this is:
                host.mpMap->mpMapper->update();
            }
        }
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setFgColor
int TLuaInterpreter::setFgColor(lua_State* L)
{
    int s = 0;
    int n = lua_gettop(L);
    QString windowName = QStringLiteral("main");
    int luaRed;
    int luaGreen;
    int luaBlue;
    if (n > 3) {
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L, "setFgColor: bad argument #%d type (window name as string expected, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }
    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "setFgColor: bad argument #%d type (red component value as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        luaRed = lua_tointeger(L, s);
        if (luaRed < 0 || luaRed >  255) {
            lua_pushnil(L);
            lua_pushfstring(L, "the color's red component value %d is outside of the valid range (0 to 255)", luaRed);
            return 2;
        }
    }

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "setFgColor: bad argument #%d type (green component value as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        luaGreen = lua_tointeger(L, s);
        if (luaGreen< 0 || luaGreen >  255) {
            lua_pushnil(L);
            lua_pushfstring(L, "the color's green component value %d is outside of the valid range (0 to 255)", luaGreen);
            return 2;
        }
    }

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "setFgColor: bad argument #%d type (blue component value as number expected, got %s!)", s, luaL_typename(L, s));

        return lua_error(L);
    } else {
        luaBlue = lua_tointeger(L, s);
        if (luaBlue < 0 || luaBlue >  255) {
            lua_pushnil(L);
            lua_pushfstring(L, "the color's blue component value %d is outside of the valid range (0 to 255)", luaBlue);
            return 2;
        }
    }

    Host& host = getHostFromLua(L);

    if (n < 4 || windowName.isEmpty() || windowName.compare(QLatin1String("main")) == 0) {
        host.mpConsole->setFgColor(luaRed, luaGreen, luaBlue);
    } else {
        mudlet::self()->setFgColor(&host, windowName, luaRed, luaGreen, luaBlue);
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBgColor
int TLuaInterpreter::setBgColor(lua_State* L)
{
    int s = 0;
    int n = lua_gettop(L);
    QString windowName = QStringLiteral("main");
    int luaRed;
    int luaGreen;
    int luaBlue;
    if (n > 3) {
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L, "setBgColor: bad argument #%d type (window name as string expected, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }
    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "setBgColor: bad argument #%d type (red component value as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        luaRed = lua_tointeger(L, s);
        if (luaRed < 0 || luaRed >  255) {
            lua_pushnil(L);
            lua_pushfstring(L, "the color's red component value %d is outside of the valid range (0 to 255)", luaRed);
            return 2;
        }
    }

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "setBgColor: bad argument #%d type (green component value as number expected, got %s!)", s, luaL_typename(L, s));
        lua_error(L);
        return lua_error(L);
    } else {
        luaGreen = lua_tointeger(L, s);
        if (luaGreen < 0 || luaGreen >  255) {
            lua_pushnil(L);
            lua_pushfstring(L, "the color's green component value %d is outside of the valid range (0 to 255)", luaGreen);
            return 2;
        }
    }

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "setBgColor: bad argument #%d type (blue component value as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        luaBlue = lua_tointeger(L, s);
        if (luaBlue < 0 || luaBlue >  255) {
            lua_pushnil(L);
            lua_pushfstring(L, "the color's blue component value %d is outside of the valid range (0 to 255)", luaBlue);
            return 2;
        }
    }

    Host& host = getHostFromLua(L);
    if (n < 4 || windowName.isEmpty() || windowName.compare(QLatin1String("main")) == 0) {
        host.mpConsole->setBgColor(luaRed, luaGreen, luaBlue);
    } else {
        mudlet::self()->setBgColor(&host, windowName, luaRed, luaGreen, luaBlue);
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#insertLink
int TLuaInterpreter::insertLink(lua_State* L)
{
    QStringList sL;
    int n = lua_gettop(L);
    int s = 1;
    bool b = false;
    // N/U:     bool gotBool = false;
    for (; s <= n; s++) {
        if (lua_isstring(L, s)) {
            std::string _str = lua_tostring(L, s);
            QString qs = _str.c_str();
            sL << qs;
        } else if (lua_isboolean(L, s)) {
            // N/U:             gotBool = true;
            b = lua_toboolean(L, s);
        }
    }

    if (sL.size() < 4) {
        sL.prepend("main");
    }
    if (sL.size() < 4) {
        lua_pushstring(L, "insertLink: wrong number of params or wrong type of params");
        lua_error(L);
        return 1;
    }

    QString _name(sL[0]);
    QString printScreen = sL[1];
    QStringList command;
    QStringList hint;
    command << sL[2];
    hint << sL[3];

    Host& host = getHostFromLua(L);
    if (_name == "main") {
        host.mpConsole->insertLink(printScreen, command, hint, b);
    } else {
        mudlet::self()->insertLink(&host, _name, printScreen, command, hint, b);
    }

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#insertPopup
int TLuaInterpreter::insertPopup(lua_State* L)
{
    std::string a1 = "";
    std::string a2;
    QStringList _hintList;
    QStringList _commandList;
    bool customFormat = false;
    int s = 1;
    int n = lua_gettop(L);
    // console name is an optional first argument
    if (n >= 4) {
        if (!lua_isstring(L, s)) {
            lua_pushstring(L, "insertPopup: wrong argument type");
            lua_error(L);
            return 1;
        } else {
            a1 = lua_tostring(L, s);
            s++;
        }
    }
    if (!lua_isstring(L, s)) {
        lua_pushstring(L, "insertPopup: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        a2 = lua_tostring(L, s);
        s++;
    }

    if (!lua_istable(L, s)) {
        lua_pushstring(L, "insertPopup: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        lua_pushnil(L);
        while (lua_next(L, s) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING) {
                QString cmd = lua_tostring(L, -1);
                _commandList << cmd;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
        s++;
    }
    if (!lua_istable(L, s)) {
        lua_pushstring(L, "insertPopup: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        lua_pushnil(L);
        while (lua_next(L, s) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING) {
                QString hint = lua_tostring(L, -1);
                _hintList << hint;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
        s++;
    }
    if (n >= s) {
        customFormat = lua_toboolean(L, s);
    }

    Host& host = getHostFromLua(L);
    QString txt = a2.c_str();
    QString name = a1.c_str();
    if (_commandList.size() != _hintList.size()) {
        lua_pushstring(L, "Error: command list size and hint list size do not match cannot create popup");
        lua_error(L);
        return 1;
    }

    if (a1.empty() || a1 == "main") {
        host.mpConsole->insertLink(txt, _commandList, _hintList, customFormat);
    } else {
        mudlet::self()->insertLink(&host, name, txt, _commandList, _hintList, customFormat);
    }

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#insertText
int TLuaInterpreter::insertText(lua_State* L)
{
    auto& host = getHostFromLua(L);
    QString windowName;
    int s = 0;

    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        if (!lua_isstring(L, ++s)) {
            lua_pushfstring(L, "insertText: bad argument #%d type (name as string expected, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
        }
    }

    QString text;
    if (!lua_isstring(L, ++s)) {
        lua_pushfstring(L, "insertText: bad argument #%d type (text as string expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        text = QString::fromUtf8(lua_tostring(L, s));
    }

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        host.mpConsole->insertText(text);
        lua_pushboolean(L, true);
        return 1;
    } else {
        if (mudlet::self()->insertText(&host, windowName, text)) {
            lua_pushboolean(L, true);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(L, "window \"%s\" not found", windowName.toUtf8().constData());
            return 2;
        }
    }
}

// Documentation: ? - public function missing documentation in wiki
int TLuaInterpreter::insertHTML(lua_State* L)
{
    std::string luaSendText;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "insertHTML: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    host.mpConsole->insertHTML(QString(luaSendText.c_str()));
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addSupportedTelnetOption
int TLuaInterpreter::addSupportedTelnetOption(lua_State* L)
{
    int option;
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "addSupportedTelnetOption: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        option = lua_tointeger(L, 1);
    }
    Host& host = getHostFromLua(L);
    host.mTelnet.supportedTelnetOptions[option] = true;
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echo -- not Echo - compare initLuaGlobals()
int TLuaInterpreter::Echo(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString consoleName;
    QString displayText;
    int n = lua_gettop(L);

    if (n > 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "echo: bad argument #1 type (console name as string, is optional, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            consoleName = QString::fromUtf8(lua_tostring(L, 1));
            if (!consoleName.isEmpty()) {
                if (consoleName == QLatin1String("main")) {
                    // QString::compare is zero for a match on the "default"
                    // case so clear the variable - to flag this as the main
                    // window case - as is the case for an empty string
                    consoleName.clear();
                }
            }
        }
    } else if (!n) {
        // Handle case with NO arguments
        lua_pushstring(L, "echo: bad argument #1 type (text to display as string expected, got nil!)");
        return lua_error(L);
    }

    if (!lua_isstring(L, n)) {
        lua_pushfstring(L, "echo: bad argument #%d type (text to display as string expected, got %s!)", n, luaL_typename(L, n));
        return lua_error(L);
    } else {
        displayText = QString::fromUtf8(lua_tostring(L, n));
    }

    if (consoleName.isEmpty()) {
        host.mpConsole->buffer.mEchoingText = true;
        host.mpConsole->echo(displayText);
        host.mpConsole->buffer.mEchoingText = false;
        // Writing to the main window must always succeed, but for consistent
        // results, we now return a true for that
        lua_pushboolean(L, true);
        return 1;
    } else {
        if (mudlet::self()->echoWindow(&host, consoleName, displayText)) {
            lua_pushboolean(L, true);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushfstring(
                    L, R"(echo: bad argument #1 value (console name "%s" does not exist, omit this{or use the default "main"} to send text to main console!))", consoleName.toUtf8().constData());
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echoPopup
int TLuaInterpreter::echoPopup(lua_State* L)
{
    QString windowName;
    QString text;
    QStringList hintList;
    QStringList commandList;
    bool customFormat = false;
    int s = 1;
    int n = lua_gettop(L);
    // console name is an optional first argument
    if (n >= 4) {
        if (!lua_isstring(L, s)) {
            lua_pushfstring(L, "echoPopup: bad argument #%d type (window name as string expected, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, s));
            s++;
        }
    }
    if (!lua_isstring(L, s)) {
        lua_pushfstring(L, "echoPopup: bad argument #%d type (text as string expected, got %s!)", s, luaL_typename(L, s));
    } else {
        text = QString::fromUtf8(lua_tostring(L, s));
        s++;
    }

    if (!lua_istable(L, s)) {
        lua_pushfstring(L, "echoPopup: bad argument #%d type (command list as table expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        lua_pushnil(L);
        while (lua_next(L, s) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING) {
                QString cmd = lua_tostring(L, -1);
                commandList << cmd;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
        s++;
    }
    if (!lua_istable(L, s)) {
        lua_pushfstring(L, "echoPopup: bad argument #%d type (hint list as table expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    } else {
        lua_pushnil(L);
        while (lua_next(L, s) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING) {
                QString hint = lua_tostring(L, -1);
                hintList << hint;
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
        s++;
    }
    if (n >= s) {
        customFormat = lua_toboolean(L, s);
    }

    Host& host = getHostFromLua(L);
    if (commandList.size() != hintList.size()) {
        lua_pushfstring(L, "echoPopup: commands and hints list aren't the same size");
        return lua_error(L);
    }

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        host.mpConsole->echoLink(text, commandList, hintList, customFormat);
    } else {
        mudlet::self()->echoLink(&host, windowName, text, commandList, hintList, customFormat);
    }

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echoLink
int TLuaInterpreter::echoLink(lua_State* L)
{
    QString a1;
    QString a2;
    QString a3;
    QString a4;
    bool useCurrentFormat = false;
    bool gotBool = false;

    int s = 0;
    int n = lua_gettop(L);
    if (n > 3){
        if (lua_isboolean(L, 4)) gotBool = true;
    }
    if (!lua_isstring(L, ++s)) {
        if (n == 3 || (n > 3 && n < 5 && gotBool)) {
            lua_pushfstring(L, "echoLink: bad argument #%d type (text as string expected, got %s!)", s, luaL_typename(L, s));
        } else {
            lua_pushfstring(L, "echoLink: bad argument #%d type (optional window name as string expected, got %s!)", s, luaL_typename(L, s));
        }
        return lua_error(L);
    } else {
        a1 = QString::fromUtf8(lua_tostring(L, s));
    }
    if (n > 1) {
        if (!lua_isstring(L, ++s)) {
            if (n == 3 || (n > 3 && n < 5 && gotBool)) {
                lua_pushfstring(L, "echoLink: bad argument #%d type (command as string expected, got %s!)", s, luaL_typename(L, s));
            } else {
                lua_pushfstring(L, "echoLink: bad argument #%d type (text as string expected, got %s!)", s, luaL_typename(L, s));
            }
            return lua_error(L);
        } else {
            a2 = QString::fromUtf8(lua_tostring(L, s));
        }
    }
    if (n > 2) {
        if (!lua_isstring(L, ++s)) {
            if (n == 3 || (n > 3 && n < 5 && gotBool)) {
                lua_pushfstring(L, "echoLink: bad argument #%d type (hint as string expected, got %s!)", s, luaL_typename(L, s));
            } else {
                lua_pushfstring(L, "echoLink: bad argument #%d type (command as string expected, got %s!)", s, luaL_typename(L, s));
            }
            return lua_error(L);
        } else {
            a3 = QString::fromUtf8(lua_tostring(L, s));
        }
    }
    if (n > 3) {
        if (lua_isstring(L, ++s)) {
            a4 = QString::fromUtf8(lua_tostring(L, s));
        } else if (lua_isboolean(L, s)) {
            gotBool = true;
            useCurrentFormat = lua_toboolean(L, s);
        } else {
            if (gotBool) {
                lua_pushfstring(L, "echoLink: bad argument #%d type (useCurrentFormat as boolean expected, got %s!)", s, luaL_typename(L, s));
            } else {
                lua_pushfstring(L, "echoLink: bad argument #%d type (hint as string expected, got %s!)", s, luaL_typename(L, s));
            }
            return lua_error(L);
        }
    }
    if (n > 4) {
        if (!lua_isboolean(L, ++s)) {
            lua_pushfstring(L, "echoLink: bad argument #%d type (useCurrentFormat as boolean expected, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        } else {
            useCurrentFormat = lua_toboolean(L, s);
            gotBool = true;
        }
    }

    Host& host = getHostFromLua(L);
    QString text;
    QString windowName;
    QStringList function;
    QStringList hint;

    if (n == 3 || (n == 4 && gotBool)) {
        text = a1;
        function << a2;
        hint << a3;
        host.mpConsole->echoLink(text, function, hint, useCurrentFormat);
    } else {
        windowName = a1;
        text = a2;
        function << a3;
        hint << a4;

        if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
            host.mpConsole->echoLink(text, function, hint, useCurrentFormat);
        } else {
            mudlet::self()->echoLink(&host, windowName, text, function, hint, useCurrentFormat);
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMergeTables
int TLuaInterpreter::setMergeTables(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QStringList modulesList;
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        if (!lua_isstring(L, i)) {
            lua_pushfstring(L, "setMergeTables: bad argument #%d (string expected, got %s)", i, luaL_typename(L, 1));
            lua_error(L);
            return 1;
        }
        modulesList << QString(lua_tostring(L, i));
    }

    host.mGMCP_merge_table_keys = host.mGMCP_merge_table_keys + modulesList;
    host.mGMCP_merge_table_keys.removeDuplicates();

    return 0;
}

// Documentation: ? - public function missing documentation in wiki
int TLuaInterpreter::pasteWindow(lua_State* L)
{
    QString window;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "pasteWindow: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        window = QString::fromUtf8(lua_tostring(L, 1));
    }
    Host& host = getHostFromLua(L);
    mudlet::self()->pasteWindow(&host, window);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#exportAreaImage
int TLuaInterpreter::exportAreaImage(lua_State* L)
{
    int areaID;
    if (lua_isnumber(L, 1)) {
        areaID = lua_tointeger(L, 1);
        Host& host = getHostFromLua(L);
        if (host.mpMap->mpMapper) {
            host.mpMap->mpMapper->mp2dMap->exportAreaImage(areaID);
        }
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#openUrl
int TLuaInterpreter::openUrl(lua_State* L)
{
    std::string luaName;
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "openUrl: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaName = lua_tostring(L, 1);
    }
    QString url(luaName.c_str());
    QDesktopServices::openUrl(url);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelStyleSheet
int TLuaInterpreter::setLabelStyleSheet(lua_State* L)
{
    std::string luaSendText = "";
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "setLabelStyleSheet: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        luaSendText = lua_tostring(L, 1);
    }
    std::string a2;
    if (!lua_isstring(L, 2)) {
        lua_pushstring(L, "setLabelStyleSheet: wrong argument type");
        lua_error(L);
        return 1;
    } else {
        a2 = lua_tostring(L, 2);
    }
    Host& host = getHostFromLua(L);
    //qDebug()<<"CSS: name:"<<luaSendText.c_str()<<"<"<<a2.c_str()<<">";
    host.mpConsole->setLabelStyleSheet(luaSendText, a2);
    return 0;
}

int TLuaInterpreter::setUserWindowStyleSheet(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setUserWindowStyleSheet: bad argument #1 type (userwindow name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "setUserWindowStyleSheet: bad argument #2 type (StyleSheet as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    QString userWindowName{QString::fromUtf8(lua_tostring(L, 1))};
    QString userWindowStyleSheet{QString::fromUtf8(lua_tostring(L, 2))};

    Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setUserWindowStyleSheet(userWindowName, userWindowStyleSheet); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCustomEnvColorTable
int TLuaInterpreter::getCustomEnvColorTable(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap->customEnvColors.empty()) {
        lua_newtable(L);
        QList<int> colorList = host.mpMap->customEnvColors.keys();
        for (int& idx : colorList) {
            lua_pushnumber(L, idx);
            lua_newtable(L);
            // red component
            {
                lua_pushnumber(L, 1);
                lua_pushnumber(L, host.mpMap->customEnvColors[idx].red());
                lua_settable(L, -3); //match in matches
            }
            // green component
            {
                lua_pushnumber(L, 2);
                lua_pushnumber(L, host.mpMap->customEnvColors[idx].green());
                lua_settable(L, -3); //match in matches
            }
            // blue component
            {
                lua_pushnumber(L, 3);
                lua_pushnumber(L, host.mpMap->customEnvColors[idx].blue());
                lua_settable(L, -3); //match in matches
            }
            lua_settable(L, -3); //matches in regex
        }
    } else {
        lua_newtable(L);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMudletVersion
int TLuaInterpreter::getMudletVersion(lua_State* L)
{
    QByteArray version = QByteArray(APP_VERSION).trimmed();
    QByteArray build = QByteArray(APP_BUILD).trimmed();

    QList<QByteArray> versionData = version.split('.');
    if (versionData.size() != 3) {
        qWarning() << "TLuaInterpreter::getMudletVersion(): ERROR: Version data not correctly set on compilation,\n"
                   << "   is the VERSION value in the project file present?";
        lua_pushstring(L, "getMudletVersion: sorry, version information not available.");
        lua_error(L);
        return 1;
    }

    bool ok = true;
    int major = 0;
    int minor = 0;
    int revision = 0;
    {
        major = versionData.at(0).toInt(&ok);
        if (ok) {
            minor = versionData.at(1).toInt(&ok);
        }
        if (ok) {
            revision = versionData.at(2).toInt(&ok);
        }
    }
    if (!ok) {
        qWarning("TLuaInterpreter::getMudletVersion(): ERROR: Version data not correctly parsed,\n"
                 "   was the VERSION value in the project file correct at compilation time?");
        lua_pushstring(L, "getMudletVersion: sorry, version information corrupted.");
        lua_error(L);
        return 1;
    }

    int n = lua_gettop(L);

    if (n == 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushstring(L, "getMudletVersion: wrong argument type.");
            lua_error(L);
        } else {
            std::string what = lua_tostring(L, 1);
            QString tidiedWhat = QString(what.c_str()).toLower().trimmed();
            if (tidiedWhat.contains("major")) {
                lua_pushinteger(L, major);
            } else if (tidiedWhat.contains("minor")) {
                lua_pushinteger(L, minor);
            } else if (tidiedWhat.contains("revision")) {
                lua_pushinteger(L, revision);
            } else if (tidiedWhat.contains("build")) {
                if (build.isEmpty()) {
                    lua_pushnil(L);
                } else {
                    lua_pushstring(L, build);
                }
            } else if (tidiedWhat.contains("string")) {
                if (build.isEmpty()) {
                    lua_pushstring(L, version.constData());
                } else {
                    lua_pushstring(L, version.append(build).constData());
                }
            } else if (tidiedWhat.contains("table")) {
                lua_pushinteger(L, major);
                lua_pushinteger(L, minor);
                lua_pushinteger(L, revision);
                if (build.isEmpty()) {
                    lua_pushnil(L);
                } else {
                    lua_pushstring(L, build);
                }
                return 4;
            } else {
                lua_pushstring(L,
                               "getMudletVersion: takes one (optional) argument:\n"
                               "   \"major\", \"minor\", \"revision\", \"build\", \"string\" or \"table\".");
                lua_error(L);
            }
        }
    } else if (n == 0) {
        lua_newtable(L);
        lua_pushstring(L, "major");
        lua_pushinteger(L, major);
        lua_settable(L, -3);
        lua_pushstring(L, "minor");
        lua_pushinteger(L, minor);
        lua_settable(L, -3);
        lua_pushstring(L, "revision");
        lua_pushinteger(L, revision);
        lua_settable(L, -3);
        lua_pushstring(L, "build");
        lua_pushstring(L, QByteArray(APP_BUILD).trimmed().data());
        lua_settable(L, -3);
    } else {
        lua_pushstring(L,
                       "getMudletVersion: only takes one (optional) argument:\n"
                       "   \"major\", \"minor\", \"revision\", \"build\", \"string\" or \"table\".");
        lua_error(L);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#openWebPage
int TLuaInterpreter::openWebPage(lua_State* L)
{
    if (lua_isstring(L, 1)) {
        QString url = lua_tostring(L, 1);
        lua_pushboolean(L, mudlet::self()->openWebPage(url));
        return 1;
    } else {
        lua_pushfstring(L, "openWebPage: bad argument #%d (string expected, got %s)", 1, luaL_typename(L, 1));
        return lua_error(L);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordApplicationID
int TLuaInterpreter::setDiscordApplicationID(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L, true);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    }

    if (lua_gettop(L)) {
        if (lua_isstring(L, 1)) {
            // Treat it as a UTF-8 string because although it is likely to be an
            // unsigned long long integer (0 to 18446744073709551615) we want to
            // be able to handle any input so we can report bad input strings back:
            QString inputText = QString::fromUtf8(lua_tostring(L, 1)).trimmed();
            if (!inputText.isEmpty()) {
                bool isOk = false;
                quint64 numericEquivalent = inputText.toULongLong(&isOk);
                if (numericEquivalent && isOk) {
                    QString appID = QString::number(numericEquivalent);
                    if (pMudlet->mDiscord.setApplicationID(&host, appID)) {
                        lua_pushboolean(L, true);
                        return 1;
                    } else {
                        lua_pushnil(L);
                        lua_pushfstring(L, "%s does not appear to be a valid Discord application id", inputText.toUtf8().constData());
                        return 2;
                    }
                } else {
                    lua_pushnil(L);
                    lua_pushfstring(L, "%s can not be converted to the expected numeric Discord application id", inputText.toUtf8().constData());
                    return 2;
                }
            } else {
                // Empty string input - to reset to default the same as the no
                // argument case:
                pMudlet->mDiscord.setApplicationID(&host, QString());
                // This must always succeed
                lua_pushboolean(L, true);
                return 1;
            }
        } else {
            lua_pushfstring(L, "setDiscordApplicationID: bad argument #1 type (id as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        }
    } else {
        pMudlet->mDiscord.setApplicationID(&host, QString());
        lua_pushboolean(L, true);
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#usingMudletsDiscordID
int TLuaInterpreter::usingMudletsDiscordID(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, pMudlet->mDiscord.usingMudletsDiscordID(&host));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordLargeIcon
int TLuaInterpreter::setDiscordLargeIcon(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L, true);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetLargeIcon)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord large icon is disabled in settings for privacy");
        return 2;
    }

    if (lua_isstring(L, 1)) {
        pMudlet->mDiscord.setLargeImage(&host, QString::fromUtf8(lua_tostring(L, 1)).toLower());
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushfstring(L, "setDiscordLargeIcon: bad argument #1 type (key as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordLargeIcon
int TLuaInterpreter::getDiscordLargeIcon(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetLargeIcon)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord large icon is disabled in settings for privacy");
        return 2;
    }

    lua_pushfstring(L, pMudlet->mDiscord.getLargeImage(&host).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordLargeIconText
int TLuaInterpreter::setDiscordLargeIconText(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetLargeIconText)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord large icon text is disabled in settings for privacy");
        return 2;
    }

    if (lua_isstring(L, 1)) {
        pMudlet->mDiscord.setLargeImageText(&host, QString::fromUtf8(lua_tostring(L, 1)));
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushfstring(L, "setDiscordLargeIconText: bad argument #%d type (text as string expected, got %s!)", 1, luaL_typename(L, 1));
        return lua_error(L);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordLargeIconText
int TLuaInterpreter::getDiscordLargeIconText(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L, true);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetLargeIconText)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord large icon text is disabled in settings for privacy");
        return 2;
    }

    lua_pushfstring(L, pMudlet->mDiscord.getLargeImageText(&host).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordSmallIcon
int TLuaInterpreter::setDiscordSmallIcon(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetSmallIcon)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord small icon is disabled in settings for privacy");
        return 2;
    }

    if (lua_isstring(L, 1)) {
        pMudlet->mDiscord.setSmallImage(&host, QString::fromUtf8(lua_tostring(L, 1)).toLower());
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushfstring(L, "setDiscordSmallIcon: bad argument #%d type (key as string expected, got %s!)", 1, luaL_typename(L, 1));
        return lua_error(L);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordSmallIcon
int TLuaInterpreter::getDiscordSmallIcon(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L, true);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetSmallIcon)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord small icon is disabled in settings for privacy");
        return 2;
    }

    lua_pushfstring(L, pMudlet->mDiscord.getSmallImage(&host).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordSmallIconText
int TLuaInterpreter::setDiscordSmallIconText(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetSmallIconText)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord small icon text is disabled in settings for privacy");
        return 2;
    }

    if (lua_isstring(L, 1)) {
        pMudlet->mDiscord.setSmallImageText(&host, QString::fromUtf8(lua_tostring(L, 1)));
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushfstring(L, "setDiscordSmallIconText: bad argument #%d type (text as string expected, got %s!)", 1, luaL_typename(L, 1));
        return lua_error(L);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordSmallIconText
int TLuaInterpreter::getDiscordSmallIconText(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L, true);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetSmallIconText)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord small icon text is disabled in settings for privacy");
        return 2;
    }

    lua_pushfstring(L, pMudlet->mDiscord.getSmallImageText(&host).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordDetail
int TLuaInterpreter::setDiscordDetail(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetDetail)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord detail is disabled in settings for privacy");
        return 2;
    }

    if (lua_isstring(L, 1)) {
        pMudlet->mDiscord.setDetailText(&host, QString::fromUtf8(lua_tostring(L, 1)));
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushfstring(L, "setDiscordDetail: bad argument #%d type (text as string expected, got %s!)", 1, luaL_typename(L, 1));
        return lua_error(L);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordDetail
int TLuaInterpreter::getDiscordDetail(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetDetail)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord detail is disabled in settings for privacy");
        return 2;
    }

    lua_pushfstring(L, pMudlet->mDiscord.getDetailText(&host).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordGame
int TLuaInterpreter::setDiscordGame(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetDetail)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord detail is disabled in settings for privacy");
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetLargeIcon)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord large icon is disabled in settings for privacy");
        return 2;
    }

    if (lua_isstring(L, 1)) {
        auto gamename = QString::fromUtf8(lua_tostring(L, 1));
        pMudlet->mDiscord.setDetailText(&host, tr("Playing %1").arg(gamename));
        pMudlet->mDiscord.setLargeImage(&host, gamename.toLower());
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushfstring(L, "setDiscordGame: bad argument #%d type (game name as string expected, got %s!)", 1, luaL_typename(L, 1));
        return lua_error(L);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordState
int TLuaInterpreter::setDiscordState(lua_State* L)
{
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetState)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord state is disabled in settings for privacy");
        return 2;
    }

    if (lua_isstring(L, 1)) {
        mudlet::self()->mDiscord.setStateText(&host, QString::fromUtf8(lua_tostring(L, 1)));
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushfstring(L, "setDiscordState: bad argument #%d type (text as string expected, got %s!)", 1, luaL_typename(L, 1));
        return lua_error(L);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordState
int TLuaInterpreter::getDiscordState(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetState)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord state is disabled in settings for privacy");
        return 2;
    }

    lua_pushfstring(L, pMudlet->mDiscord.getStateText(&host).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordElapsedStartTime
int TLuaInterpreter::setDiscordElapsedStartTime(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetTimeInfo)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord time is disabled in settings for privacy");
        return 2;
    }

    if (lua_isnumber(L, 1)) {
        int64_t timeStamp = lua_tointeger(L, 1);
        if (timeStamp >= 0) {
            pMudlet->mDiscord.setStartTimeStamp(&host, timeStamp);
            lua_pushboolean(L, true);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "the timestamp must be zero to clear the \"elapsed:\" time or an epoch time value from the recent past");
            return 2;
        }
    } else {
        lua_pushfstring(L, "setDiscordElapsedStartTime: bad argument #%d type (epoch time as number, got %s!)", 1, luaL_typename(L, 1));
        return lua_error(L);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordRemainingEndTime
int TLuaInterpreter::setDiscordRemainingEndTime(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetTimeInfo)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord time is disabled in settings for privacy");
        return 2;
    }

    if (lua_isnumber(L, 1)) {
        int64_t timeStamp = lua_tointeger(L, 1);
        if (timeStamp >= 0) {
            pMudlet->mDiscord.setEndTimeStamp(&host, timeStamp);
            lua_pushboolean(L, true);
            return 1;
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "the timestamp must be zero to clear the \"remaining:\" time or an epoch time value in the recent future");
            return 2;
        }
    } else {
        lua_pushfstring(L, "setDiscordRemainingEndTime: bad argument #%d type (epoch time as number, got %s!)", 1, luaL_typename(L, 1));
        return lua_error(L);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordTimeStamps
int TLuaInterpreter::getDiscordTimeStamps(lua_State* L)
{
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetTimeInfo)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord time is disabled in settings for privacy");
        return 2;
    }

    QPair<int64_t, int64_t> timeStamps = mudlet::self()->mDiscord.getTimeStamps(&host);
    lua_pushnumber(L, timeStamps.first);
    lua_pushnumber(L, timeStamps.second);
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDiscordParty
int TLuaInterpreter::setDiscordParty(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetPartyInfo)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord party info is disabled in settings for privacy");
        return 2;
    }

    int64_t partySize = -1;
    int64_t partyMax = -1;
    if (lua_isnumber(L, 1)) {
        partySize = lua_tointeger(L, 1);
        if (partySize < 0) {
            lua_pushnil(L);
            lua_pushstring(L, "the current party size must be zero or more");
            return 2;
        }
    } else {
        lua_pushfstring(L, "setDiscordParty: bad argument #%d type (current party size as number expected, got %s!)", 1, luaL_typename(L, 1));
        return lua_error(L);
    }

    if (lua_gettop(L) > 1) {
        partyMax = lua_tointeger(L, 2);
        if (partyMax < 0) {
            lua_pushnil(L);
            lua_pushstring(L, "the optional party maximum size must be zero (to remove the party details) or more (to set the maximum)");
            return 2;
        }

        pMudlet->mDiscord.setParty(&host, static_cast<int>(qMin(static_cast<int64_t>(INT_MAX), partySize)), static_cast<int>(qMin(static_cast<int64_t>(INT_MAX), partyMax)));
    } else {
        // Only got the partySize now
        pMudlet->mDiscord.setParty(&host, static_cast<int>(qMin(static_cast<int64_t>(INT_MAX), partySize)));
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDiscordParty
int TLuaInterpreter::getDiscordParty(lua_State* L)
{
    mudlet* pMudlet = mudlet::self();
    auto& host = getHostFromLua(L);

    auto result = discordApiEnabled(L, true);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetPartyInfo)) {
        lua_pushnil(L);
        lua_pushstring(L, "Access to Discord party info is disabled in settings for privacy");
        return 2;
    }

    QPair<int, int> partyValues = pMudlet->mDiscord.getParty(&host);
    lua_pushnumber(L, partyValues.first);
    lua_pushnumber(L, partyValues.second);
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getTime
int TLuaInterpreter::getTime(lua_State* L)
{
    int n = lua_gettop(L);
    bool return_string = false;
    QString format = QStringLiteral("yyyy.MM.dd hh:mm:ss.zzz");
    QString tm;
    if (n > 0) {
        return_string = lua_toboolean(L, 1);
        if (n > 1) {
            if (!lua_isstring(L, 2)) {
                lua_pushfstring(L, "getTime: bad argument #2 (custom time format as string expected, got %s)", luaL_typename(L, 2));
                return lua_error(L);
            } else {
                format = lua_tostring(L, 2);
            }
        }
    }
    QDateTime time = QDateTime::currentDateTime();
    if (return_string) {
        tm = time.toString(format);
        lua_pushstring(L, tm.toLatin1().data());
    } else {
        QDate dt = time.date();
        QTime tm = time.time();
        lua_createtable(L, 0, 4);
        lua_pushstring(L, "hour");
        lua_pushinteger(L, tm.hour());
        lua_rawset(L, n + 1);
        lua_pushstring(L, "min");
        lua_pushinteger(L, tm.minute());
        lua_rawset(L, n + 1);
        lua_pushstring(L, "sec");
        lua_pushinteger(L, tm.second());
        lua_rawset(L, n + 1);
        lua_pushstring(L, "msec");
        lua_pushinteger(L, tm.msec());
        lua_rawset(L, n + 1);
        lua_pushstring(L, "year");
        lua_pushinteger(L, dt.year());
        lua_rawset(L, n + 1);
        lua_pushstring(L, "month");
        lua_pushinteger(L, dt.month());
        lua_rawset(L, n + 1);
        lua_pushstring(L, "day");
        lua_pushinteger(L, dt.day());
        lua_rawset(L, n + 1);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getEpoch
int TLuaInterpreter::getEpoch(lua_State *L)
{
    lua_pushnumber(L, static_cast<double>(QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#appendBuffer
int TLuaInterpreter::appendBuffer(lua_State* L)
{
    std::string a1;
    std::string a2;
    int s = 1;
    int n = lua_gettop(L);
    if (n > 0) {
        if (!lua_isstring(L, s)) {
            lua_pushstring(L, "appendBuffer: wrong argument type");
            lua_error(L);
            return 1;
        } else {
            a1 = lua_tostring(L, s);
            s++;
        }
    }
    Host& host = getHostFromLua(L);

    if (s == 1) {
        host.mpConsole->appendBuffer();
    } else {
        QString name = a1.c_str();
        mudlet::self()->appendBuffer(&host, name);
    }

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#appendCmdLine
int TLuaInterpreter::appendCmdLine(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = "main";
    QString text;
    if (n > 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "appendCmdLine: bad argument #1 (command line name as string expected, got %s)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            name = QString::fromUtf8(lua_tostring(L, 1));
        }
    }
    if (!lua_isstring(L, n)) {
        lua_pushfstring(L, "appendCmdLine: bad argument #%d (text to set on command line as string expected, got %s)", n, luaL_typename(L, n));
        return lua_error(L);
    } else {
        text = QString::fromUtf8(lua_tostring(L, n));
    }

    Host& host = getHostFromLua(L);
    auto pN = host.mpConsole->mSubCommandLineMap.value(name);
    if (!pN || name == "main") {
        pN = host.mpConsole->mpCommandLine;
    }
    if (!pN) {
        lua_pushnil(L);
        return 1;
    }

    QString curText = pN->toPlainText();
    pN->setPlainText(curText + text);
    QTextCursor cur = pN->textCursor();
    cur.clearSelection();
    cur.movePosition(QTextCursor::EndOfLine);
    pN->setTextCursor(cur);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCmdLine
int TLuaInterpreter::getCmdLine(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = "main";
    if (n > 0) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "getCmdLine: bad argument #1 (command line name as string expected, got %s)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            name = QString::fromUtf8(lua_tostring(L, 1));
        }
    }
    Host& host = getHostFromLua(L);
    auto pN = host.mpConsole->mSubCommandLineMap.value(name);
    if (!pN || name == "main") {
        pN = host.mpConsole->mpCommandLine;
    }
    if (!pN) {
        lua_pushnil(L);
        return 1;
    }
    QString curText = pN->toPlainText();
    lua_pushstring(L, curText.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#installPackage
int TLuaInterpreter::installPackage(lua_State* L)
{
    QString location;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "installPackage: bad argument #1 (package location path and file name as string expected, got %s)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        location = QString::fromUtf8(lua_tostring(L, 1));
    }
    Host& host = getHostFromLua(L);
    host.installPackage(location, 0);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#uninstallPackage
int TLuaInterpreter::uninstallPackage(lua_State* L)
{
    QString packageName;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "uninstallPackage: bad argument #1 (package name as string expected, got %s)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        packageName =  QString::fromUtf8(lua_tostring(L, 1));
    }
    Host& host = getHostFromLua(L);
    host.uninstallPackage(packageName, 0);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#installModule
int TLuaInterpreter::installModule(lua_State* L)
{
    std::string modName;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "installModule: bad argument #1 (module location as string expected, got %s)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        modName = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString module = QDir::fromNativeSeparators(modName.c_str());
    if (host.installPackage(module, 3) && mudlet::self()->moduleTableVisible()) {
        mudlet::self()->layoutModules();
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#uninstallModule
int TLuaInterpreter::uninstallModule(lua_State* L)
{
    std::string modName;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "uninstallModule: bad argument #1 (module name as string expected, got %s)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        modName = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString module = modName.c_str();
    if (host.uninstallPackage(module, 3) && mudlet::self()->moduleTableVisible()) {
        mudlet::self()->layoutModules();
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#reloadModule
int TLuaInterpreter::reloadModule(lua_State* L)
{
    std::string modName;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "reloadModule: bad argument #1 (module name as string expected, got %s)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        modName = lua_tostring(L, 1);
    }
    Host& host = getHostFromLua(L);
    QString module = modName.c_str();
    host.reloadModule(module);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableModuleSync
int TLuaInterpreter::enableModuleSync(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "enableModuleSync: bad argument #1 (module name as string expected, got %s)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString module{QString::fromUtf8(lua_tostring(L, 1))};

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.changeModuleSync(module, QLatin1String("1")); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    auto moduleTable = mudlet::self()->moduleTable;
    if (moduleTable && !moduleTable->findItems(module, Qt::MatchExactly).isEmpty()) {
        int row = moduleTable->findItems(module, Qt::MatchExactly)[0]->row();
        auto checkItem = moduleTable->item(row, 2);
        checkItem->setCheckState(Qt::Checked);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableModuleSync
int TLuaInterpreter::disableModuleSync(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "disableModuleSync: bad argument #1 (module name as string expected, got %s)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString module{QString::fromUtf8(lua_tostring(L, 1))};

    Host& host = getHostFromLua(L);

    if (auto [success, message] = host.changeModuleSync(module, QLatin1String("0")); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    }

    auto moduleTable = mudlet::self()->moduleTable;
    if (moduleTable && !moduleTable->findItems(module, Qt::MatchExactly).isEmpty()) {
        int row = moduleTable->findItems(module, Qt::MatchExactly)[0]->row();
        auto checkItem = moduleTable->item(row, 2);
        checkItem->setCheckState(Qt::Unchecked);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getModuleSync
int TLuaInterpreter::getModuleSync(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "getModuleSync: bad argument #1 (module name as string expected, got %s)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString module{QString::fromUtf8(lua_tostring(L, 1))};

    Host& host = getHostFromLua(L);

    if (auto [success, message] = host.getModuleSync(module); !success) {
        lua_pushnil(L);
        lua_pushfstring(L, message.toUtf8().constData());
        return 2;
    } else if (message == QLatin1String("1")) {
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushboolean(L, false);
        return 1;
    }
}

// Documentation: ? - public function missing documentation in wiki
// Once a mapper has been created it will, by default, include the "Default
// Area" associated with the reserved area Id -1 in the list of Areas shown in
// the area selection widget.  This function will immediately hide that entry
// if given a true argument and restore it if set to false.  The setting is NOT
// saved and this function was created to address a specific need for that area
// to not be immediately shown to users for one package writer who needed to
// hide rooms until they have been "explored".  This setting is ALSO present on
// the last "Special Options" tab of the "Profile Preferences" - although it is
// hidden until there IS a mapper to apply the setting to.
// Returns true on successfully setting the desired value or false if there is
// (not yet) a map display to apply it to.  Also throws an Error or returned a
// nil value - both with an accompied error string - if there are problems.
int TLuaInterpreter::setDefaultAreaVisible(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        lua_pushnil(L);
        lua_pushstring(L, "setDefaultAreaVisible: no map present or loaded!");
        return 2;
    }

    if (!lua_isboolean(L, 1)) {
        lua_pushfstring(L,
                        "setDefaultAreaVisible: bad argument #1 type (isToShowDefaultArea as boolean\n"
                        "expected, got %s!)",
                        luaL_typename(L, 1));
        lua_error(L);
    } else {
        bool isToShowDefaultArea = lua_toboolean(L, 1);
        if (host.mpMap->mpMapper) {
            // If we are reenabled the display of the default area
            // AND the mapper was showing the default area
            // the area widget will NOT be showing the correct area name afterwards
            bool isAreaWidgetInNeedOfResetting = false;
            if ((!host.mpMap->mpMapper->getDefaultAreaShown()) && (isToShowDefaultArea) && (host.mpMap->mpMapper->mp2dMap->mAreaID == -1)) {
                isAreaWidgetInNeedOfResetting = true;
            }

            host.mpMap->mpMapper->setDefaultAreaShown(isToShowDefaultArea);
            if (isAreaWidgetInNeedOfResetting) {
                // Corner case fixup:
                host.mpMap->mpMapper->showArea->setCurrentText(host.mpMap->mpRoomDB->getDefaultAreaName());
            }
            host.mpMap->mpMapper->mp2dMap->repaint();
            host.mpMap->mpMapper->update();
            lua_pushboolean(L, true);
        } else {
            lua_pushboolean(L, false);
        }
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#registerAnonymousEventHandler
// The function below is mostly unused now as it is overwritten in lua.
// The overwriting function poses as a transperant proxy and internally uses
// this function to get called events.
int TLuaInterpreter::registerAnonymousEventHandler(lua_State* L)
{
    std::string event;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "registerAnonymousEventHandler: bad argument #1 (event name as string expected, got %s)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        event = lua_tostring(L, 1);
    }
    std::string func;
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "registerAnonymousEventHandler: bad argument #2 (function name as string expected, got %s)", luaL_typename(L, 2));
        return lua_error(L);
    } else {
        func = lua_tostring(L, 2);
    }
    Host& host = getHostFromLua(L);
    QString e = event.c_str();
    QString f = func.c_str();
    host.registerAnonymousEventHandler(e, f);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#expandAlias
int TLuaInterpreter::expandAlias(lua_State* L)
{
    QString payload;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "expandAlias: bad argument #1 type (text to parse as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        payload = QString::fromUtf8(lua_tostring(L, 1));
    }

    bool wantPrint = true;
    if (lua_gettop(L) > 1) {
        // check if the 2nd argument is a 'false', but don't match if it is 'nil'
        // because expandAlias("command") should be the same as expandAlias("command", nil)
        if (lua_isnil(L, 2)) {
            wantPrint = false;
        } else if (!lua_isboolean(L, 2)) {
            lua_pushfstring(L, "expandAlias: bad argument #2 type (echo as boolean is optional, got %s!)", luaL_typename(L, 2));
            return lua_error(L);
        } else {
            wantPrint = lua_toboolean(L, 2);
        }
    }
    Host& host = getHostFromLua(L);
    // Host::send will encode the UTF encoded data here in the wanted Server
    // encoding:
    host.send(payload, wantPrint, false);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#printCmdLine
int TLuaInterpreter::printCmdLine(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = "main";
    QString text;
    if (n > 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "printCmdLine: bad argument #1 (command line name as string expected, got %s)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            name = QString::fromUtf8(lua_tostring(L, 1));
        }
    }
    if (!lua_isstring(L, n)) {
        lua_pushfstring(L, "printCmdLine: bad argument #%d (text to set on command line as string expected, got %s)", n, luaL_typename(L, n));
        return lua_error(L);
    } else {
        text = QString::fromUtf8(lua_tostring(L, n));
    }
    Host& host = getHostFromLua(L);
    auto pN = host.mpConsole->mSubCommandLineMap.value(name);
    if (!pN || name == "main") {
        pN = host.mpConsole->mpCommandLine;
    }
    if (!pN) {
        lua_pushnil(L);
        return 1;
    }
    pN->setPlainText(text);
    QTextCursor cur = pN->textCursor();
    cur.clearSelection();
    cur.movePosition(QTextCursor::EndOfLine);
    pN->setTextCursor(cur);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearCmdLine
int TLuaInterpreter::clearCmdLine(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = "main";
    if (n > 0) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "clearCmdLine: bad argument #1 (command line name as string expected, got %s)", luaL_typename(L, 1));
            return lua_error(L);
        } else {
            name = QString::fromUtf8(lua_tostring(L, 1));
        }
    }
    Host& host = getHostFromLua(L);
    auto pN = host.mpConsole->mSubCommandLineMap.value(name);
    if (!pN || name == "main") {
        pN = host.mpConsole->mpCommandLine;
    }
    if (!pN) {
        lua_pushnil(L);
        return 1;
    }
    pN->clear();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#send
// Note this is registered as send NOT sendRaw - see initLuaGlobals()
// It converts the bytes in the command (the first argument) from Utf-8 to be
// encoded in the required Mud Server encoding.
int TLuaInterpreter::sendRaw(lua_State* L)
{
    QString text;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "send: bad argument #1 type (command as string expected, got %s)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        text = QString::fromUtf8(lua_tostring(L, 1));
    }

    bool wantPrint = true;
    if (lua_gettop(L) > 1) {
        if (!lua_isboolean(L, 2)) {
            lua_pushfstring(L, "send: bad argument #2 type (showOnScreen as boolean expected, got %s)", luaL_typename(L, 2));
            return lua_error(L);
        } else {
            wantPrint = lua_toboolean(L, 2);
        }
    }
    Host& host = getHostFromLua(L);
    // Host::send will encode the UTF encoded data here in the wanted Server
    // encoding:
    host.send(text, wantPrint, true);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendSocket
// The data can, theoretically, contain embedded ASCII NUL characters:
int TLuaInterpreter::sendSocket(lua_State* L)
{
    std::string data;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "sendSocket: bad argument #1 type (data as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        data = lua_tostring(L, 1);
    }

    Host& host = getHostFromLua(L);
    // msg is not in an encoded form here it is a literal set of bytes, which
    // is what this usage needs:
    host.mTelnet.socketOutRaw(data);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendIrc
int TLuaInterpreter::sendIrc(lua_State* L)
{
    std::string who, text;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "sendIrc: bad argument #1 type (target as string expected, got %s!)", lua_typename(L, lua_type(L, 1)));
        return lua_error(L);
    } else {
        who = lua_tostring(L, 1);
    }
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "sendIrc: bad argument #2 type (message as string expected, got %s!)", lua_typename(L, lua_type(L, 2)));
        return lua_error(L);
    } else {
        text = lua_tostring(L, 2);
    }
    QString target = who.c_str();
    QString msg = text.c_str();
    Host* pHost = &getHostFromLua(L);
    if (!mudlet::self()->mpIrcClientMap.contains(pHost)) {
        // create a new irc client if one isn't ready.
        mudlet::self()->mpIrcClientMap[pHost] = new dlgIRC(pHost);
        mudlet::self()->mpIrcClientMap.value(pHost)->raise();
        mudlet::self()->mpIrcClientMap.value(pHost)->show();
    }

    // wait for our client to be ready before sending messages.
    if (!mudlet::self()->mpIrcClientMap.value(pHost)->mReadyForSending) {
        lua_pushnil(L);
        lua_pushstring(L, "not ready to send");
        return 2;
    }

    QPair<bool, QString> rval = mudlet::self()->mpIrcClientMap.value(pHost)->sendMsg(target, msg);

    if (rval.first) {
        lua_pushboolean(L, true);
    } else {
        lua_pushnil(L);
    }
    lua_pushstring(L, rval.second.toUtf8().constData());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getIrcNick
int TLuaInterpreter::getIrcNick(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    QString nick;
    if (mudlet::self()->mpIrcClientMap.contains(pHost)) {
        nick = mudlet::self()->mpIrcClientMap.value(pHost)->getNickName();
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
    int hport;
    if (mudlet::self()->mpIrcClientMap.contains(pHost)) {
        hname = mudlet::self()->mpIrcClientMap.value(pHost)->getHostName();
        hport = mudlet::self()->mpIrcClientMap.value(pHost)->getHostPort();
    } else {
        hname = dlgIRC::readIrcHostName(pHost);
        hport = dlgIRC::readIrcHostPort(pHost);
    }

    lua_pushstring(L, hname.toUtf8().constData());
    lua_pushinteger(L, hport);
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getIrcChannels
int TLuaInterpreter::getIrcChannels(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    QStringList channels;
    if (mudlet::self()->mpIrcClientMap.contains(pHost)) {
        channels = mudlet::self()->mpIrcClientMap.value(pHost)->getChannels();
    } else {
        channels = dlgIRC::readIrcChannels(pHost);
    }

    lua_newtable(L);
    int total = channels.count();
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
    QString error = QStringLiteral("no client active");
    if (mudlet::self()->mpIrcClientMap.contains(pHost)) {
        cHostName = mudlet::self()->mpIrcClientMap.value(pHost)->getConnectedHost();

        if (cHostName.isEmpty()) {
            error = QStringLiteral("not yet connected");
        }
    }

    if (cHostName.isEmpty()) {
        lua_pushboolean(L, false);
        lua_pushstring(L, error.toUtf8().constData());
    } else {
        lua_pushboolean(L, true);
        lua_pushstring(L, cHostName.toUtf8().constData());
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setIrcNick
int TLuaInterpreter::setIrcNick(lua_State* L)
{
    std::string nick;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setIrcNick: bad argument #1 type (nick as string expected, got %s!)", lua_typename(L, lua_type(L, 1)));
        return lua_error(L);
    } else {
        nick = lua_tostring(L, 1);
        if (nick.empty()) {
            lua_pushnil(L);
            lua_pushfstring(L, "nick must not be empty");
            return 2;
        }
    }

    Host* pHost = &getHostFromLua(L);
    QPair<bool, QString> result = dlgIRC::writeIrcNickName(pHost, QString::fromStdString(nick));
    if (!result.first) {
        lua_pushnil(L);
        lua_pushfstring(L, "unable to save nick name, reason: %s", result.second.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setIrcServer
int TLuaInterpreter::setIrcServer(lua_State* L)
{
    std::string addr;
    int port = 6667;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setIrcServer: bad argument #1 type (hostname as string expected, got %s!)", lua_typename(L, lua_type(L, 1)));
        return lua_error(L);
    } else {
        addr = lua_tostring(L, 1);
        if (addr.empty()) {
            lua_pushnil(L);
            lua_pushfstring(L, "hostname must not be empty");
            return 2;
        }
    }
    if (!lua_isnoneornil(L, 2)) {
        if (!lua_isnumber(L, 2)) {
            lua_pushfstring(L, "setIrcServer: bad argument #2 type (port number as number is optional {default = 6667}, got %s!)", lua_typename(L, lua_type(L, 2)));
            return lua_error(L);
        } else {
            port = lua_tointeger(L, 2);
            if (port > 65535 || port < 1) {
                lua_pushnil(L);
                lua_pushfstring(L, "invalid port number %d given, if supplied it must be in range 1 to 65535, {defaults to 6667 if not provided}", port);
                return 2;
            }
        }
    }

    Host* pHost = &getHostFromLua(L);
    QPair<bool, QString> result = dlgIRC::writeIrcHostName(pHost, QString::fromStdString(addr));
    if (!result.first) {
        lua_pushnil(L);
        lua_pushfstring(L, "unable to save hostname, reason: %s", result.second.toUtf8().constData());
        return 2;
    }

    result = dlgIRC::writeIrcHostPort(pHost, port);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushfstring(L, "unable to save port, reason: %s", result.second.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    lua_pushnil(L);
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setIrcChannels
int TLuaInterpreter::setIrcChannels(lua_State* L)
{
    QStringList newchannels;
    if (!lua_istable(L, 1)) {
        lua_pushfstring(L, "setIrcChannels: bad argument #1 type (channels as table expected, got %s!)", lua_typename(L, lua_type(L, 1)));
        return lua_error(L);
    } else {
        lua_pushnil(L);
        while (lua_next(L, 1) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING) {
                QString c = lua_tostring(L, -1);
                if (!c.isEmpty() && (c.startsWith(QLatin1String("#")) || c.startsWith(QLatin1String("&")) || c.startsWith(QLatin1String("+")))) {
                    newchannels << c;
                }
            }
            lua_pop(L, 1);
        }
    }

    if (newchannels.count() == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "channels must contain at least 1 valid channel name");
        return 2;
    }

    Host* pHost = &getHostFromLua(L);
    QPair<bool, QString> result = dlgIRC::writeIrcChannels(pHost, newchannels);
    if (!result.first) {
        lua_pushnil(L);
        lua_pushfstring(L, "unable to save channels, reason: %s", result.second.toUtf8().constData());
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#restartIrc
int TLuaInterpreter::restartIrc(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    bool rv = false;
    if (mudlet::self()->mpIrcClientMap.contains(pHost)) {
        mudlet::self()->mpIrcClientMap.value(pHost)->ircRestart();
        rv = true;
    }

    lua_pushboolean(L, rv);
    return 1;
}

#ifdef QT_TEXTTOSPEECH_LIB

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSpeak
int TLuaInterpreter::ttsSpeak(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "ttsSpeak: bad argument #%1 type (text to say as string expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    }

    QString textToSay;
    textToSay = QString::fromUtf8(lua_tostring(L, 1));

    speechUnit->say(textToSay);
    speechCurrent = textToSay;

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsBuild
void TLuaInterpreter::ttsBuild()
{
    if (bSpeechBuilt) {
        return;
    }

    speechUnit = new QTextToSpeech();

    bSpeechBuilt = true;
    bSpeechQueueing = false;

    connect(speechUnit, &QTextToSpeech::stateChanged, &TLuaInterpreter::ttsStateChanged);


    speechUnit->setVolume(1.0);
    speechUnit->setRate(0.0);
    speechUnit->setPitch(0.0);

    return;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSkip
int TLuaInterpreter::ttsSkip(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    speechUnit->stop();

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSetRate
int TLuaInterpreter::ttsSetRate(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    double rate;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "ttsSetRate: bad argument #1 type (rate as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        rate = lua_tonumber(L, 1);
    }

    if (rate > 1.0) {
        rate = 1.0;
    }

    if (rate < -1.0) {
        rate = -1.0;
    }

    speechUnit->setRate(rate);

    TEvent event {};
    event.mArgumentList.append(QLatin1String("ttsRateChanged"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(rate));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudlet::self()->getHostManager().postInterHostEvent(NULL, event, true);

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSetPitch
int TLuaInterpreter::ttsSetPitch(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    double pitch;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "ttsSetPitch: bad argument #1 type (pitch as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        pitch = lua_tonumber(L, 1);
    }

    if (pitch > 1.0) {
        pitch = 1.0;
    }

    if (pitch < -1.0) {
        pitch = -1.0;
    }

    speechUnit->setPitch(pitch);

    TEvent event {};
    event.mArgumentList.append(QLatin1String("ttsPitchChanged"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(pitch));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudlet::self()->getHostManager().postInterHostEvent(NULL, event, true);

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSetVolume
int TLuaInterpreter::ttsSetVolume(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    double volume;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "ttsSetVolume: bad argument #1 type (volume as number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        volume = lua_tonumber(L, 1);
    }

    if (volume > 1.0) {
        volume = 1.0;
    }

    if (volume < 0.0) {
        volume = 0.0;
    }

    speechUnit->setVolume(volume);

    TEvent event {};
    event.mArgumentList.append(QLatin1String("ttsVolumeChanged"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(volume));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    mudlet::self()->getHostManager().postInterHostEvent(NULL, event, true);

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetVolume
int TLuaInterpreter::ttsGetVolume(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    lua_pushnumber(L, speechUnit->volume());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetRate
int TLuaInterpreter::ttsGetRate(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    lua_pushnumber(L, speechUnit->rate());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetPitch
int TLuaInterpreter::ttsGetPitch(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    lua_pushnumber(L, speechUnit->pitch());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetVoices
int TLuaInterpreter::ttsGetVoices(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    QVector<QVoice> speechVoices = speechUnit->availableVoices();
    int i = 0;
    lua_newtable(L);
    for (const QVoice& voice : speechVoices) {
        lua_pushnumber(L, ++i);
        lua_pushstring(L, voice.name().toUtf8().constData());
        lua_settable(L, -3);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetCurrentVoice
int TLuaInterpreter::ttsGetCurrentVoice(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    QString currentVoice = speechUnit->voice().name();
    lua_pushstring(L, currentVoice.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSetVoiceByName
int TLuaInterpreter::ttsSetVoiceByName(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    QString nextVoice;

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "ttsSetVoiceByName: bad argument #1 type (voice as string expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        nextVoice = QString(lua_tostring(L, 1));
    }

    QVector<QVoice> speechVoices = speechUnit->availableVoices();
    for (auto voice : speechVoices) {
        if (voice.name() == nextVoice) {
            speechUnit->setVoice(voice);
            lua_pushboolean(L, true);

            TEvent event {};
            event.mArgumentList.append(QLatin1String("ttsVoiceChanged"));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            event.mArgumentList.append(voice.name());
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            mudlet::self()->getHostManager().postInterHostEvent(NULL, event, true);

            return 1;
        }
    }

    lua_pushboolean(L, false);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSetVoiceByIndex
int TLuaInterpreter::ttsSetVoiceByIndex(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    int index;
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "ttsSetVoiceByIndex: bad argument #1 type (voice as index number expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        index = lua_tonumber(L, 1);
    }

    index--;

    QVector<QVoice> speechVoices = speechUnit->availableVoices();
    if (index < 0 || index >= speechVoices.size()) {
        lua_pushboolean(L, false);
        return 1;
    }

    speechUnit->setVoice(speechVoices.at(index));

    TEvent event {};
    event.mArgumentList.append(QLatin1String("ttsVoiceChanged"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(speechVoices[index].name());
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    mudlet::self()->getHostManager().postInterHostEvent(NULL, event, true);

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsStateChanged
void TLuaInterpreter::ttsStateChanged(QTextToSpeech::State state)
{
    if (state != speechState) {
        speechState = state;
        TEvent event {};
        switch (state) {
        case QTextToSpeech::Paused:
            event.mArgumentList.append(QLatin1String("ttsSpeechPaused"));
            break;
        case QTextToSpeech::Speaking:
            event.mArgumentList.append(QLatin1String("ttsSpeechStarted"));
            break;
        case QTextToSpeech::BackendError:
            event.mArgumentList.append(QLatin1String("ttsSpeechError"));
            break;
        case QTextToSpeech::Ready:
            event.mArgumentList.append(QLatin1String("ttsSpeechReady"));
            break;
        }
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

        if (state == QTextToSpeech::Speaking) {
            event.mArgumentList.append(speechCurrent);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        }

        mudlet::self()->getHostManager().postInterHostEvent(NULL, event, true);
    }

    if (state != QTextToSpeech::Ready || speechQueue.empty()) {
        bSpeechQueueing = false;
        return;
    }

    QString textToSay;
    textToSay = speechQueue.takeFirst();

    speechUnit->say(textToSay);
    speechCurrent = textToSay;

    return;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsQueue
int TLuaInterpreter::ttsQueue(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "ttsQueueText: bad argument #1 type (input as string expected, got %s!)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    }

    QString inputText = lua_tostring(L, 1);
    int index;

    if (lua_gettop(L) > 1) {
        if (!lua_isnumber(L, 2)) {
            lua_pushfstring(L, "ttsQueueText: bad argument #2 type (index as number expected, got %s!)", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        }

        index = lua_tonumber(L, 2);
        index--;

        if (index < 0) {
            index = 0;
        }

        if (index > speechQueue.size()) {
            index = speechQueue.size();
        }
    } else {
        index = speechQueue.size();
    }

    speechQueue.insert(index, inputText);

    TEvent event {};
    Host& host = getHostFromLua(L);
    event.mArgumentList.append(QLatin1String("ttsSpeechQueued"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(inputText);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(index));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    host.raiseEvent(event);

    if (speechQueue.size() == 1 && speechUnit->state() == QTextToSpeech::Ready && !bSpeechQueueing) {
        bSpeechQueueing = true;
        TLuaInterpreter::ttsStateChanged(speechUnit->state());
    }

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetQueue
int TLuaInterpreter::ttsGetQueue(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    if (lua_gettop(L) > 0) {
        if (!lua_isnumber(L, 1)) {
            lua_pushfstring(L, "ttsGetQueue: bad argument #1 type (index as number expected, got %s!)", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        }

        int index = lua_tonumber(L, 1);
        index--;

        if (index < 0 || index > speechQueue.size()) {
            lua_pushboolean(L, false);
            return 1;
        }

        lua_pushstring(L, speechQueue.at(index).toLatin1().constData());
        return 1;
    }

    lua_newtable(L);

    for (int i = 0; i < speechQueue.size(); i++) {
        lua_pushnumber(L, i + 1);
        lua_pushstring(L, speechQueue.at(i).toUtf8().constData());
        lua_settable(L, -3);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsPause
int TLuaInterpreter::ttsPause(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    speechUnit->pause();

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsResume
int TLuaInterpreter::ttsResume(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    speechUnit->resume();

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsClearQueue
int TLuaInterpreter::ttsClearQueue(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    if (lua_gettop(L) > 0) {
        if (!lua_isnumber(L, 1)) {
            lua_pushfstring(L, "ttsClearQueue: bad argument #1 type (index as number expected, got %s!)", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        }

        int index = lua_tonumber(L, 1);
        index--;

        if (index < 0 || index >= speechQueue.size()) {
            lua_pushnil(L);
            lua_pushfstring(L, "index (%d) out of bounds for queue size %d", index + 1, speechQueue.size());
            return 2;
        }

        speechQueue.remove(index);
        return 0;
    }

    speechQueue.clear();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetCurrentLine
int TLuaInterpreter::ttsGetCurrentLine(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    if (speechUnit->state() == QTextToSpeech::Ready) {
        lua_pushnil(L);
        lua_pushfstring(L, "not speaking any text");
        return 2;
    } else if (speechUnit->state() == QTextToSpeech::BackendError) {
        lua_pushnil(L);
        lua_pushfstring(L, "error with the backend");
        return 2;
    }

    lua_pushstring(L, speechCurrent.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsGetState
int TLuaInterpreter::ttsGetState(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    switch (speechUnit->state()) {
    case QTextToSpeech::Ready:
        lua_pushstring(L, "ttsSpeechReady");
        break;
    case QTextToSpeech::Paused:
        lua_pushstring(L, "ttsSpeechPaused");
        break;
    case QTextToSpeech::Speaking:
        lua_pushstring(L, "ttsSpeechStarted");
        break;
    case QTextToSpeech::BackendError:
        lua_pushstring(L, "ttsSpeechError");
        break;
    default:
        lua_pushstring(L, "ttsUnknownState");
    }

    return 1;
}

#endif // QT_TEXTTOSPEECH_LIB

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setServerEncoding
int TLuaInterpreter::setServerEncoding(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QByteArray newEncoding;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setServerEncoding: bad argument #1 type (newEncoding as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    newEncoding = lua_tostring(L, 1);
    QPair<bool, QString> results = host.mTelnet.setEncoding(newEncoding);

    if (results.first) {
        lua_pushboolean(L, true);
        return 1;
    }

    lua_pushnil(L);
    lua_pushfstring(L, results.second.toLatin1().constData());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getServerEncoding
int TLuaInterpreter::getServerEncoding(lua_State* L)
{
    Host& host = getHostFromLua(L);

    // don't leak if we're using a Mudlet or a Qt-supplied codec to Lua
    auto sanitizeEncoding = [] (auto encodingName) {
        if (encodingName.startsWith("M_")) {
            encodingName.remove(0, 2);
        }
        return encodingName;
    };

    QByteArray encoding = host.mTelnet.getEncoding();
    if (encoding.isEmpty()) {
        encoding = "ASCII";
    }
    lua_pushstring(L, sanitizeEncoding(encoding).constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getServerEncodingsList
int TLuaInterpreter::getServerEncodingsList(lua_State* L)
{
    Host& host = getHostFromLua(L);

    // don't leak if we're using a Mudlet or a Qt-supplied codec to Lua
    auto sanitizeEncoding = [] (auto encodingName) {
        if (encodingName.startsWith("M_")) {
            encodingName.remove(0, 2);
        }
        return encodingName;
    };

    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_pushstring(L, "ASCII");
    lua_settable(L, -3);
    for (int i = 0, total = host.mTelnet.getEncodingsList().count(); i < total; ++i) {
        lua_pushnumber(L, i + 2); // Lua indexes start with 1 but we already have one entry
        lua_pushstring(L, sanitizeEncoding(host.mTelnet.getEncodingsList().at(i)).constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getOS
int TLuaInterpreter::getOS(lua_State* L)
{
#if defined(Q_OS_CYGWIN)
    // Try for this one before Q_OS_WIN32 as both are likely to be defined on
    // a Cygwin platform
    // CHECK: hopefully will NOT be triggered on mingw/msys
    lua_pushstring(L, "cygwin");
#elif defined(Q_OS_WIN32)
    lua_pushstring(L, "windows");
#elif defined(Q_OS_MACOS)
    lua_pushstring(L, "mac");
#elif defined(Q_OS_LINUX)
    lua_pushstring(L, "linux");
#elif defined(Q_OS_HURD)
    // One can hope/dream!
    lua_pushstring(L, "hurd");
#elif defined(Q_OS_FREEBSD)
    // Only defined on FreeBSD but NOT Debian kFreeBSD so we should check for
    // this first
    lua_pushstring(L, "freebsd");
#elif defined(Q_OS_FREEBSD_KERNEL)
    // Defined for BOTH Debian kFreeBSD hybrid with a GNU userland and
    // main FreeBSD so it must be after Q_OS_FREEBSD check; included for Debian
    // packager who may want to have this!
    lua_pushstring(L, "kfreebsd");
#elif defined(Q_OS_OPENBSD)
    lua_pushstring(L, "openbsd");
#elif defined(Q_OS_NETBSD)
    lua_pushstring(L, "netbsd");
#elif defined(Q_OS_BSD4)
    // Generic *nix - must be before unix and after other more specific results
    lua_pushstring(L, "bsd4");
#elif defined(Q_OS_UNIX)
    // Most generic *nix - must be after bsd4 and other more specific results
    lua_pushstring(L, "unix");
#else
    lua_pushstring(L, "unknown");
#endif
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getClipboardText
int TLuaInterpreter::getClipboardText(lua_State* L)
{
    QClipboard* clipboard = QApplication::clipboard();
    lua_pushstring(L, clipboard->text().toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setClipboardText
int TLuaInterpreter::setClipboardText(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setClipboardText: bad argument #%d type (text as string expected, got %s!)", 1, luaL_typename(L, 1));
        return lua_error(L);
    }
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(QString::fromUtf8(lua_tostring(L, 1)));
    lua_pushboolean(L, true);
    return 1;
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::compileAndExecuteScript(const QString& code)
{
    if (code.size() < 1) {
        return false;
    }
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    int error = luaL_dostring(L, code.toUtf8().constData());
    if (error != 0) {
        std::string e = "no error message available from Lua";
        if (lua_isstring(L, 1)) {
            e = "Lua error:";
            e += lua_tostring(L, 1);
        }
        if (mudlet::debugMode) {
            qDebug() << "LUA ERROR: code did not compile: ERROR:" << e.c_str();
        }
        QString _n = "error in Lua code";
        QString _n2 = "no debug data available";
        logError(e, _n, _n2);
    }

    lua_pop(L, lua_gettop(L));

    if (error == 0) {
        return true;
    } else {
        return false;
    }
}

// No documentation available in wiki - internal function
// reformats given Lua code. In case of any issues, returns the original code as-is
// issues could be invalid Lua code or the formatter code bugging out
QString TLuaInterpreter::formatLuaCode(const QString &code)
{
    if (code.isEmpty()) {
        return code;
    }

    if (!pIndenterState) {
        initIndenterGlobals();
    }

    lua_State* L = pIndenterState.get();
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return code;
    }

    if (!validLuaCode(code)) {
        return code;
    }

    QString escapedCode = code;
    // escape backslashes so we can pass \n to the function
    escapedCode.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    // escape quotes since we'll be using quotes to pass data to the function
    escapedCode.replace(QLatin1String("\""), QLatin1String("\\\""));
    // escape newlines so they don't interpreted as newlines, but instead get passed onto the function
    escapedCode.replace(QLatin1String("\n"), QLatin1String("\\n"));

    QString thing = QString(R"(return get_formatted_code(get_ast("%1"), {indent_chunk = '  ', right_margin = 100, max_text_width = 160, keep_comments = true}))").arg(escapedCode);
    int error = luaL_dostring(L, thing.toUtf8().constData());
    QString n;
    if (error != 0) {
        std::string e = "no error message available from Lua";
        if (lua_isstring(L, 1)) {
            e = "Lua error:";
            e += lua_tostring(L, 1);
        }
        if (mudlet::debugMode) {
            qDebug() << "LUA ERROR: code did not compile: ERROR:" << e.c_str();
        }
        QString objectName = "error in Lua code";
        QString functionName = "no debug data available";
        logError(e, objectName, functionName);
        lua_pop(L, lua_gettop(L));
        return code;
    }

    QString result = lua_tostring(L, 1);
    lua_pop(L, lua_gettop(L));
    return result;
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::compileScript(const QString& code)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    int error = luaL_dostring(L, code.toUtf8().constData());
    if (error != 0) {
        std::string e = "no error message available from Lua";
        if (lua_isstring(L, 1)) {
            e = "Lua error:";
            e += lua_tostring(L, 1);
        }
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: code did not compile: ERROR:" << e.c_str() << "\n" >> 0;
        }
    } else {
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::darkGreen)) << "LUA: code compiled without errors. OK\n" >> 0;
        }
    }
    lua_pop(L, lua_gettop(L));

    if (error == 0) {
        return true;
    } else {
        return false;
    }
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::compile(const QString& code, QString& errorMsg, const QString& name)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    int error = (luaL_loadbuffer(L, code.toUtf8().constData(),
                                 strlen(code.toUtf8().constData()),
                                 name.toUtf8().constData()) || lua_pcall(L, 0, 0, 0));

    QString n;
    if (error != 0) {
        std::string e = "Lua syntax error:";
        if (lua_isstring(L, 1)) {
            e.append(lua_tostring(L, 1));
        }
        errorMsg = "<b><font color='blue'>";
        errorMsg.append(e.c_str());
        errorMsg.append("</font></b>");
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::red)) << "\n " << e.c_str() << "\n" >> 0;
        }
    } else {
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::darkGreen)) << "\nLUA: code compiled without errors. OK\n" >> 0;
        }
    }
    lua_pop(L, lua_gettop(L));

    if (error == 0) {
        return true;
    } else {
        return false;
    }
}

// No documentation available in wiki - internal function
// returns true if the given Lua code is valid, false otherwise
bool TLuaInterpreter::validLuaCode(const QString &code)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qWarning() << "LUA CRITICAL ERROR: no pGlobalLua Lua execution unit found.";
        return false;
    }

    int error = luaL_loadbuffer(L, code.toUtf8().constData(), strlen(code.toUtf8().constData()), "Lua code validation");
    lua_pop(L, lua_gettop(L));

    return error == 0;
}

// No documentation available in wiki - internal function
std::pair<bool, QString> TLuaInterpreter::discordApiEnabled(lua_State* L, bool writeAccess)
{
    mudlet* pMudlet = mudlet::self();

    if (!pMudlet->mDiscord.libraryLoaded()) {
        return std::make_pair(false, QStringLiteral("Discord API is not available"));
    }

    auto& host = getHostFromLua(L);
    if (!(host.mDiscordAccessFlags & Host::DiscordLuaAccessEnabled)) {
        return std::make_pair(false, QStringLiteral("Discord API is disabled in settings for privacy"));
    }

    if (writeAccess && !pMudlet->mDiscord.discordUserIdMatch(&host)) {
        return std::make_pair(false, QStringLiteral("Discord API is read-only as you're logged in with a different account in Discord compared to the one you entered for this profile"));
    }

    return std::make_pair(true, QString());
}

// No documentation available in wiki - internal function
void TLuaInterpreter::setMultiCaptureGroups(const std::list<std::list<std::string>>& captureList, const std::list<std::list<int>>& posList)
{
    mMultiCaptureGroupList = captureList;
    mMultiCaptureGroupPosList = posList;

    /*
     * std::list< std::list<string> >::const_iterator mit = mMultiCaptureGroupList.begin();
     *
     * int k=1;
     * for ( ; mit!=mMultiCaptureGroupList.end(); mit++, k++) {
     *     cout << "regex#"<<k<<" got:"<<endl;
     *     std::list<string>::const_iterator it = (*mit).begin();
     *     for ( int i=1; it!=(*mit).end(); it++, i++ ) {
     *         cout << i<<"#"<<"<"<<*it<<">"<<endl;
     *     }
     *     cout << "-----------------------------"<<endl;
     * }
     */
}

// No documentation available in wiki - internal function
void TLuaInterpreter::setCaptureGroups(const std::list<std::string>& captureList, const std::list<int>& posList)
{
    mCaptureGroupList = captureList;
    mCaptureGroupPosList = posList;

    /*
     * std::list<string>::iterator it2 = mCaptureGroupList.begin();
     * std::list<int>::iterator it1 = mCaptureGroupPosList.begin();
     * int i=0;
     * for ( ; it1!=mCaptureGroupPosList.end(); it1++, it2++, i++) {
     *     cout << "group#"<<i<<" begin="<<*it1<<" len="<<(*it2).size()<<"word="<<*it2<<endl;
     * }
     */
}

// No documentation available in wiki - internal function
void TLuaInterpreter::clearCaptureGroups()
{
    mCaptureGroupList.clear();
    mCaptureGroupPosList.clear();
    mMultiCaptureGroupList.clear();
    mMultiCaptureGroupPosList.clear();

    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
    }

    lua_newtable(L);
    lua_setglobal(L, "matches");
    lua_newtable(L);
    lua_setglobal(L, "multimatches");

    lua_pop(L, lua_gettop(L));
}

// No documentation available in wiki - internal function
void TLuaInterpreter::adjustCaptureGroups(int x, int a)
{
    // adjust all capture group positions in line if data has been inserted by the user
    for (int& it : mCaptureGroupPosList) {
        if (it >= x) {
            it += a;
        }
    }
}

// No documentation available in wiki - internal function
void TLuaInterpreter::setAtcpTable(const QString& var, const QString& arg)
{
    lua_State* L = pGlobalLua;
    lua_getglobal(L, "atcp"); //defined in LuaGlobal.lua
    lua_pushstring(L, var.toLatin1().data());
    lua_pushstring(L, arg.toLatin1().data());
    lua_rawset(L, -3);
    lua_pop(L, 1);

    TEvent event {};
    event.mArgumentList.append(var);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(arg);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    Host& host = getHostFromLua(L);
    host.raiseEvent(event);
}

void
TLuaInterpreter::signalMXPEvent(const QString &type, const QMap<QString, QString> &attrs, const QStringList &actions) {
    lua_State *L = pGlobalLua;
    lua_getglobal(L, "mxp");
    if (!lua_istable(L, -1)) {
        lua_newtable(L);
        lua_setglobal(L, "mxp");
        lua_getglobal(L, "mxp");
        if (!lua_istable(L, -1)) {
            qDebug() << "ERROR: mxp table not defined";
            return;
        }
    }

    lua_newtable(L);
    lua_setfield(L, -2, type.toUtf8().toLower().constData());
    lua_getfield(L, -1, type.toUtf8().toLower().constData());
    if (!lua_istable(L, -1)) {
        qDebug() << "ERROR: 'mxp." << type << "' table could not be defined";
        return;
    }

    QMapIterator<QString, QString> itr(attrs);
    while (itr.hasNext()) {
        itr.next();
        lua_pushstring(L, itr.value().toUtf8().constData());
        lua_setfield(L, -2, itr.key().toUtf8().toLower().constData());
    }

    lua_newtable(L);
    lua_setfield(L, -2, "actions");
    lua_getfield(L, -1, "actions");
    for (int i = 0; i < actions.size(); i++) {
        lua_pushstring(L, actions[i].toUtf8().constData());
        lua_rawseti(L, -2, i + 1);
    }

    lua_pop(L, lua_gettop(L));


    TEvent event{};
    QString token("mxp");
    token.append(".");
    token.append(type.toUtf8().toLower().constData());

    event.mArgumentList.append(token);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

    Host &host = getHostFromLua(L);
    if (mudlet::debugMode) {
        QString msg = QStringLiteral("\n%1 event <%2> display(%1) to see the full content\n").arg("mxp", token);
        host.mpConsole->printSystemMessage(msg);
    }
    host.raiseEvent(event);
}



// No documentation available in wiki - internal function
void TLuaInterpreter::setGMCPTable(QString& key, const QString& string_data)
{
    lua_State* L = pGlobalLua;
    lua_getglobal(L, "gmcp"); //defined in Lua init
    if (!lua_istable(L, -1)) {
        lua_newtable(L);
        lua_setglobal(L, "gmcp");
        lua_getglobal(L, "gmcp");
        if (!lua_istable(L, -1)) {
            qDebug() << "ERROR: gmcp table not defined";
            return;
        }
    }
    parseJSON(key, string_data, QLatin1String("gmcp"));
}

// No documentation available in wiki - internal function
void TLuaInterpreter::setMSSPTable(const QString& string_data)
{
    lua_State* L = pGlobalLua;
    lua_getglobal(L, "mssp"); //defined in Lua init
    if (!lua_istable(L, -1)) {
        lua_newtable(L);
        lua_setglobal(L, "mssp");
        lua_getglobal(L, "mssp");
        if (!lua_istable(L, -1)) {
            qDebug() << "ERROR: mssp table not defined";
            return;
        }
    }
    parseMSSP(string_data);
}

// No documentation available in wiki - internal function
void TLuaInterpreter::setMSDPTable(QString& key, const QString& string_data)
{
    lua_State* L = pGlobalLua;
    lua_getglobal(L, "msdp");
    if (!lua_istable(L, -1)) {
        lua_newtable(L);
        lua_setglobal(L, "msdp");
        lua_getglobal(L, "msdp");
        if (!lua_istable(L, -1)) {
            qDebug() << "ERROR: msdp table not defined";
            return;
        }
    }

    parseJSON(key, string_data, QLatin1String("msdp"));
}

// No documentation available in wiki - internal function
void TLuaInterpreter::parseJSON(QString& key, const QString& string_data, const QString& protocol)
{
    // key is in format of Blah.Blah or Blah.Blah.Bleh - we want to push & pre-create the tables as appropriate
    lua_State* L = pGlobalLua;
    QStringList tokenList = key.split(QLatin1Char('.'));
    if (!lua_checkstack(L, tokenList.size() + 5)) {
        return;
    }
    int i = 0;
    for (int total = tokenList.size() - 1; i < total; ++i) {
        lua_getfield(L, -1, tokenList.at(i).toUtf8().constData());
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            lua_pushstring(L, tokenList.at(i).toUtf8().constData());
            lua_newtable(L);
            lua_rawset(L, -3);
            lua_getfield(L, -1, tokenList.at(i).toUtf8().constData());
        }
        lua_remove(L, -2);
    }
    bool __needMerge = false;
    lua_getfield(L, -1, tokenList.at(i).toUtf8().constData());
    Host& host = getHostFromLua(L);
    if (lua_istable(L, -1)) {
        // only merge tables (instead of replacing them) if the key has been registered as a need to merge key by the user default is Char.Status only
        if (host.mGMCP_merge_table_keys.contains(key)) {
            __needMerge = true;
        }
    }
    lua_pop(L, 1);
    if (!__needMerge) {
        lua_pushstring(L, tokenList.at(i).toUtf8().constData());
    } else {
        lua_pushstring(L, "__needMerge");
    }

    lua_getglobal(L, "json_to_value");

    if (!lua_isfunction(L, -1)) {
        lua_settop(L, 0);
        qDebug() << "CRITICAL ERROR: json_to_value not defined";
        return;
    }
    auto dataInUtf8 = string_data.toUtf8();
    lua_pushlstring(L, dataInUtf8.constData(), dataInUtf8.length());
    int error = lua_pcall(L, 1, 1, 0);
    if (error == 0) {
        // Top of stack should now contain the lua representation of json.
        lua_rawset(L, -3);
        if (__needMerge) {
            lua_settop(L, 0);
            lua_getglobal(L, "__gmcp_merge_gmcp_sub_tables");
            if (!lua_isfunction(L, -1)) {
                lua_settop(L, 0);
                qDebug() << "CRITICAL ERROR: __gmcp_merge_gmcp_sub_tables is not defined in lua_LuaGlobal.lua";
                return;
            }
            lua_getglobal(L, "gmcp");
            i = 0;
            for (int total = tokenList.size() - 1; i < total; ++i) {
                lua_getfield(L, -1, tokenList.at(i).toUtf8().constData());
                lua_remove(L, -2);
            }
            lua_pushstring(L, tokenList.at(i).toUtf8().constData());
            lua_pcall(L, 2, 0, 0);
        }
    } else {
        {
            std::string e;
            if (lua_isstring(L, -1)) {
                e = "Lua error:";
                e += lua_tostring(L, -1);
            }
            QString _n = "JSON decoder error:";
            QString _f = "json_to_value";
            logError(e, _n, _f);
        }
    }
    lua_settop(L, 0);

    // events: for key "foo.bar.top" we raise: gmcp.foo, gmcp.foo.bar and gmcp.foo.bar.top
    // with the actual key given as parameter e.g. event=gmcp.foo, param="gmcp.foo.bar"

    QString token = protocol;
    if (protocol == QLatin1String("msdp")) {
        key.prepend(QLatin1String("msdp."));
    } else {
        key.prepend(QLatin1String("gmcp."));
    }

    for (int k = 0, total = tokenList.size(); k < total; ++k) {
        TEvent event {};
        token.append(".");
        token.append(tokenList[k]);
        event.mArgumentList.append(token);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        event.mArgumentList.append(key);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        if (mudlet::debugMode) {
            QString msg = QStringLiteral("\n%1 event <%2> display(%1) to see the full content\n").arg(protocol, token);
            host.mpConsole->printSystemMessage(msg);
        }
        host.raiseEvent(event);
    }
    // auto-detect IRE composer
    if (tokenList.size() == 3 && tokenList.at(0).toLower() == "ire" && tokenList.at(1).toLower() == "composer" && tokenList.at(2).toLower() == "edit") {
        QRegularExpression rx(QStringLiteral(R"lit(\{ ?"title": ?"(.*)", ?"text": ?"(.*)" ?\})lit"));
        QRegularExpressionMatch match = rx.match(string_data);

        if (match.capturedStart() != -1) {
            QString title = match.captured(1);
            QString initialText = match.captured(2);
            QRegularExpression codeRegex(QStringLiteral(R"lit(\\n|\\t|\\"|\\\\|\\u[0-9a-cA-C][0-9a-fA-F]{3}|\\u[dD][0-7][0-9a-fA-F]{2}|\\u[efEF][0-9a-fA-F]{3}|\\u[dD][89abAB][0-9a-fA-F]{2}\\u[dD][c-fC-F][0-9a-fA-F]{2})lit"));
            // We are about to search for 8 escape code strings within the initial text that the game gave us, patterns are:
            // \n  \t  \"  \\ - new line, tab, quote, backslash
            // Then there are three patterns for \uXXXX where XXXX is a 4-digit hexadecimal value
            //   Characters in ranges U+0000-U+D7FF and U+E000-U+FFFD are stored as a single unit.
            //   0000-CFFF
            //   D000-D7FF
            //   D800-DFFF - are reserved for surrogate pairs; will not match a pattern
            //   E000-FFFF - note that FFFE and FFFF match the pattern but are not valid, will skip those later
            // Then one pattern for \uXXXX\uXXXX where each XXXX is a 4-digit hexadecimal value
            //   These are 'surrogate pairs', (U+D800-U+DBFF) followed by (U+DC00-U+DFFF).
            //   D800-DF00  DC00-DFFF
            int j = 0;
            while ((j = initialText.indexOf(codeRegex, j)) != -1) {
                uint u;
                switch (initialText.at(j+1).unicode()){
                    case 'n' : initialText.replace(j, 2, '\n'); break;
                    case 't' : initialText.replace(j, 2, '\t'); break;
                    case '\"' : initialText.replace(j, 2, '\"'); break;
                    case '\\' : initialText.replace(j, 2, '\\'); break;
                    case 'u': // handle lone code or pair of codes together
                        u = initialText.midRef(j+2, 4).toUShort(0, 16);
                        if(u > 0xFFFD){
                            j += 5; // FFFE and FFFF are guaranteed to not be Unicode characters.  Skip it.
                        }
                        else if((u < 0xD800) || (0xDFFF < u)){
                            // Characters in ranges U+0000-U+D7FF and U+E000-U+FFFD are stored as a single unit.
                            initialText.replace(j, 6, QChar(u));
                        }
                        else if((0xD7FF < u) && (u < 0xDC00)){
                            // Non-BMP characters (range U+10000-U+10FFFF) are stored as "surrogate pairs".
                            // A 'high' surrogate (U+D800-U+DBFF) followed by 'low' surrogate (U+DC00-U+DFFF).
                            // Surrogates are always written in pairs, a lone one is invalid.
                            // The regex above should ensure second code is DCxx-DFxx
                            QChar code[2];
                            code[0] = QChar(u);
                            code[1] = QChar(initialText.midRef(j+8, 4).toUShort(0, 16));
                            initialText.replace(j, 12, code, 2);
                            j++; // in this case we are adding 2 code points for the character
                        }
                        // DC00-DFFF should be filtered out by the regex.
                        break;
                }
                j++;
            }
            Host& host = getHostFromLua(L);
            if (host.mTelnet.mpComposer) {
                return;
            }

            host.mTelnet.mpComposer = new dlgComposer(&host);
            host.mTelnet.mpComposer->init(title, initialText);
            host.mTelnet.mpComposer->raise();
            host.mTelnet.mpComposer->show();
        }
    }
    lua_pop(L, lua_gettop(L));
}

// No documentation available in wiki - internal function
void TLuaInterpreter::parseMSSP(const QString& string_data)
{
    lua_State* L = pGlobalLua;

    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return;
    }

    // string_data is in the format of MSSP_VAR "PLAYERS" MSSP_VAL "52" MSSP_VAR "UPTIME" MSSP_VAL "1234567890"
    // The quote characters mean that the encased word is a string, the quotes themselves are not sent.
    QStringList packageList = string_data.split(MSSP_VAR);

    if (packageList.size() > 0) {
        Host& host = getHostFromLua(L);
        lua_getglobal(L, "mssp");

        for (int i = 1; i < packageList.size(); i++) {
            QStringList payloadList = packageList[i].split(MSSP_VAL);

            if (payloadList.size() != 2) {
                return;
            }

            QString msspVAR;
            QString msspVAL;

            for (int j = 0; j < payloadList.size(); j++) {
                if (j < 1) {
                    msspVAR = payloadList[j];
                    lua_pushstring(L, msspVAR.toUtf8().constData());
                } else {
                    msspVAL = payloadList[j];
                    lua_pushlstring(L, msspVAL.toUtf8().constData(), msspVAL.toUtf8().length());
                }
            }

            lua_rawset(L, -3);

            // Raise an event
            QString protocol = QStringLiteral("mssp");
            QString token = protocol;
            token.append(".");
            token.append(msspVAR);

            TEvent event {};
            event.mArgumentList.append(token);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            event.mArgumentList.append(token);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            if (mudlet::debugMode) {
                QString msg = QStringLiteral("\n%1 event <%2> display(%1) to see the full content\n").arg(protocol, token);
                host.mpConsole->printSystemMessage(msg);
            }
            host.raiseEvent(event);
        }

        lua_pop(L, lua_gettop(L));
    }
}

// No documentation available in wiki - internal function
// src is in Mud Server encoding and may need transcoding
// Includes MSDP code originally from recv_sb_msdp(...) in TinTin++'s telopt.c,
// https://tintin.sourceforge.io:
void TLuaInterpreter::msdp2Lua(const char* src)
{
    Host& host = getHostFromLua(pGlobalLua);
    QByteArray transcodedSrc = host.mTelnet.decodeBytes(src);
    QStringList varList;
    QByteArray lastVar;
    int textLength = transcodedSrc.length();
    int nest = 0;
    quint8 last = 0;

    QByteArray script;
    bool no_array_marker_bug = false;
    for (int i = 0; i < textLength; ++i) {
        switch (transcodedSrc.at(i)) {
        case MSDP_TABLE_OPEN:
            script.append('{');
            ++nest;
            last = MSDP_TABLE_OPEN;
            break;
        case MSDP_TABLE_CLOSE:
            if (last == MSDP_VAL || last == MSDP_VAR) {
                script.append('\"');
            }
            if (nest) {
                --nest;
            }
            script.append('}');
            last = MSDP_TABLE_CLOSE;
            break;
        case MSDP_ARRAY_OPEN:
            script.append('[');
            ++nest;
            last = MSDP_ARRAY_OPEN;
            break;
        case MSDP_ARRAY_CLOSE:
            if (last == MSDP_VAL || last == MSDP_VAR) {
                script.append('\"');
            }
            if (nest) {
                --nest;
            }
            script.append(']');
            last = MSDP_ARRAY_CLOSE;
            break;
        case MSDP_VAR:
            if (nest) {
                if (last == MSDP_VAL || last == MSDP_VAR) {
                    script.append('\"');
                }
                if (last == MSDP_VAL || last == MSDP_VAR || last == MSDP_TABLE_CLOSE || last == MSDP_ARRAY_CLOSE) {
                    script.append(',');
                }
                script.append('\"');
            } else {
                script.append('\"');

                if (!varList.empty()) {
                    QString token = varList.front();
                    token = token.remove(QLatin1Char('\"'));
                    script = script.replace(0, varList.front().toUtf8().size() + 3, QByteArray());
                    mpHost->processDiscordMSDP(token, script);
                    setMSDPTable(token, QString::fromUtf8(script));
                    varList.clear();
                    script.clear();
                }
            }
            last = MSDP_VAR;
            lastVar.clear();
            break;

        case MSDP_VAL:
            if (last == MSDP_VAR) {
                script.append("\":");
            }
            if (last == MSDP_VAL) {
                no_array_marker_bug = true;
                script.append('\"');
            }
            if (last == MSDP_VAL || last == MSDP_TABLE_CLOSE || last == MSDP_ARRAY_CLOSE) {
                script.append(',');
            }
            if (((textLength > i + 1) && transcodedSrc.at(i + 1) && transcodedSrc.at(i + 1) != MSDP_TABLE_OPEN && transcodedSrc.at(i + 1) != MSDP_ARRAY_OPEN) ||
                (textLength <= i + 1)) {
                script.append('\"');
            }
            varList.append(QString::fromUtf8(lastVar));
            last = MSDP_VAL;
            break;
        case '\\':
            script.append('\\');
            break;
        case '\"':
            script.append('\"');
            break;
        default:
            script.append(transcodedSrc.at(i));
            lastVar.append(transcodedSrc.at(i));
            break;
        }
    }
    if (last != MSDP_ARRAY_CLOSE && last != MSDP_TABLE_CLOSE) {
        script.append('\"');
        if (!script.startsWith('\"')) {
            script.prepend('\"');
        }
    }
    if (!varList.empty()) {
        QString token = varList.front();
        token = token.remove(QLatin1Char('\"'));
        script = script.replace(0, token.toUtf8().size() + 3, QByteArray());
        if (no_array_marker_bug) {
            if (!script.startsWith('[')) {
                script.prepend('[');
                script.append(']');
            }
        }
        setMSDPTable(token, QString::fromUtf8(script));
    }
}

// No documentation available in wiki - internal function
void TLuaInterpreter::setChannel102Table(int& var, int& arg)
{
    lua_State* L = pGlobalLua;
    lua_getglobal(L, "channel102"); //defined in LuaGlobal.lua
    lua_pushnumber(L, var);
    lua_pushnumber(L, arg);
    lua_rawset(L, -3);
    lua_pop(L, 1);

    TEvent event {};
    event.mArgumentList.append(QLatin1String("channel102Message"));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(QString::number(var));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    event.mArgumentList.append(QString::number(arg));
    event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    Host& host = getHostFromLua(L);
    host.raiseEvent(event);
}

// No documentation available in wiki - internal function
void TLuaInterpreter::setMatches(lua_State* L)
{
    if (!mCaptureGroupList.empty()) {
        lua_newtable(L);

        // set values
        int i = 1; // Lua indexes start with 1 as a general convention
        for (auto it = mCaptureGroupList.begin(); it != mCaptureGroupList.end(); it++, i++) {
            // if ((*it).length() < 1) continue; //have empty capture groups to be undefined keys i.e. machts[emptyCapGroupNumber] = nil otherwise it's = "" i.e. an empty string
            lua_pushnumber(L, i);
            lua_pushstring(L, (*it).c_str());
            lua_settable(L, -3);
        }
        lua_setglobal(L, "matches");
    }
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::call_luafunction(void* pT)
{
    lua_State* L = pGlobalLua;
    lua_pushlightuserdata(L, pT);
    lua_gettable(L, LUA_REGISTRYINDEX);
    if (lua_isfunction(L, -1)) {
        setMatches(L);
        int error = lua_pcall(L, 0, LUA_MULTRET, 0);
        if (error != 0) {
            int nbpossible_errors = lua_gettop(L);
            for (int i = 1; i <= nbpossible_errors; i++) {
                std::string e = "";
                if (lua_isstring(L, i)) {
                    e = "Lua error:";
                    e += lua_tostring(L, i);
                    QString _n = "error in anonymous Lua function";
                    QString _n2 = "no debug data available";
                    logError(e, _n, _n2);
                    if (mudlet::debugMode) {
                        TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: ERROR running anonymous Lua function ERROR:" << e.c_str() >> 0;
                    }
                }
            }
        } else {
            if (mudlet::debugMode) {
                TDebug(QColor(Qt::white), QColor(Qt::darkGreen)) << "LUA OK anonymous Lua function ran without errors\n" >> 0;
            }
        }
        lua_pop(L, lua_gettop(L));
        //lua_settop(L, 0);
        if (error == 0) {
            return true;
        } else {
            return false;
        }
    } else {
        QString _n = "error in anonymous Lua function";
        QString _n2 = "func reference not found by Lua, func cannot be called";
        std::string e = "Lua error:";
        logError(e, _n, _n2);
    }

    return false;
}

// No documentation available in wiki - internal function
// returns true if function ran without errors
// as well as the boolean return value from the function
std::pair<bool, bool> TLuaInterpreter::callLuaFunctionReturnBool(void* pT)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return std::make_pair(false, false);
    }

    lua_pushlightuserdata(L, pT);
    lua_gettable(L, LUA_REGISTRYINDEX);
    bool returnValue = false;

    if (lua_isfunction(L, -1)) {
        setMatches(L);
        int error = lua_pcall(L, 0, LUA_MULTRET, 0);
        if (error != 0) {
            int nbpossible_errors = lua_gettop(L);
            for (int i = 1; i <= nbpossible_errors; i++) {
                std::string e = "";
                if (lua_isstring(L, i)) {
                    e = "Lua error:";
                    e += lua_tostring(L, i);
                    QString _n = "error in anonymous Lua function";
                    QString _n2 = "no debug data available";
                    logError(e, _n, _n2);
                    if (mudlet::debugMode) {
                        TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: ERROR running anonymous Lua function ERROR:" << e.c_str() >> 0;
                    }
                }
            }
        } else {
            auto index = lua_gettop(L);
            if (lua_isboolean(L, index)) {
                returnValue = lua_toboolean(L, index);
            }

            if (mudlet::debugMode) {
                TDebug(QColor(Qt::white), QColor(Qt::darkGreen)) << "LUA OK anonymous Lua function ran without errors\n" >> 0;
            }
        }
        lua_pop(L, lua_gettop(L));
        //lua_settop(L, 0);
        if (error == 0) {
            return std::make_pair(true, returnValue);
        } else {
            return std::make_pair(false, returnValue);
        }
    } else {
        QString _n = "error in anonymous Lua function";
        QString _n2 = "func reference not found by Lua, func cannot be called";
        std::string e = "Lua error:";
        logError(e, _n, _n2);
    }

    return std::make_pair(false, false);
}

// No documentation available in wiki - internal function
// Third argument hides the "LUA OK" type message if it true which may be used
// to cut down on spammy output if things are okay.
bool TLuaInterpreter::call(const QString& function, const QString& mName, const bool muteDebugOutput)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    setMatches(L);

    lua_getglobal(L, function.toUtf8().constData());
    int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error != 0) {
        int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                logError(e, mName, function);
                if (mudlet::debugMode) {
                    TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA ERROR: when running script " << mName << " (" << function << "),\nreason: " << e.c_str() << "\n" >> 0;
                }
            }
        }
    } else {
        if (mudlet::debugMode && !muteDebugOutput) {
            TDebug(QColor(Qt::white), QColor(Qt::darkGreen)) << "LUA OK: script " << mName << " (" << function << ") ran without errors\n" >> 0;
        }
    }
    lua_pop(L, lua_gettop(L));

    return (error);
}

// No documentation available in wiki - internal function
std::pair<bool, bool> TLuaInterpreter::callReturnBool(const QString& function, const QString& mName)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return std::make_pair(false, false);
    }

    bool returnValue = false;

    setMatches(L);

    lua_getglobal(L, function.toUtf8().constData());
    int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error != 0) {
        int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                logError(e, mName, function);
                if (mudlet::debugMode) {
                    TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: ERROR running script " << mName << " (" << function << ") ERROR:" << e.c_str() << "\n" >> 0;
                }
            }
        }
    } else {
        auto index = lua_gettop(L);
        if (lua_isboolean(L, index)) {
            returnValue = lua_toboolean(L, index);
        }

        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::darkGreen)) << "LUA OK script " << mName << " (" << function << ") ran without errors\n" >> 0;
        }
    }
    lua_pop(L, lua_gettop(L));
    if (error == 0) {
        return std::make_pair(true, returnValue);
    } else {
        return std::make_pair(false, returnValue);
    }
}


// No documentation available in wiki - internal function
void TLuaInterpreter::logError(std::string& e, const QString& name, const QString& function)
{
    // Log error to Editor's Errors TConsole:
    if (mpHost->mpEditorDialog) {
        mpHost->mpEditorDialog->mpErrorConsole->print(QStringLiteral("[%1:]").arg(tr("ERROR")), QColor(Qt::blue), QColor(Qt::black));
        mpHost->mpEditorDialog->mpErrorConsole->print(QStringLiteral(" %1:<%2> %3:<%4>\n").arg(tr("object", "object is the Mudlet alias/trigger/script, used in this sample message: object:<Alias1> function:<cure_me>"), name, tr("function", "function is the Lua function, used in this sample message: object:<Alias1> function:<cure_me>"), function), QColor(Qt::green), QColor(Qt::black));
        mpHost->mpEditorDialog->mpErrorConsole->print(QStringLiteral("        <%1>\n").arg(e.c_str()), QColor(Qt::red), QColor(Qt::black));
    }

    // Log error to Profile's Main TConsole:
    if (mpHost->mEchoLuaErrors) {
        // ensure the Lua error is on a line of its own and is not prepended to the previous line
        if (mpHost->mpConsole->buffer.size() > 0 && !mpHost->mpConsole->buffer.lineBuffer.at(mpHost->mpConsole->buffer.lineBuffer.size() - 1).isEmpty()) {
            mpHost->postMessage(QStringLiteral("\n"));
        }

        mpHost->postMessage(QStringLiteral("[  LUA  ] - %1: <%2> %3:<%4>\n<%5>").arg(tr("object", "object is the Mudlet alias/trigger/script, used in this sample message: object:<Alias1> function:<cure_me>"), name, tr("function", "function is the Lua function, used in this sample message: object:<Alias1> function:<cure_me>"), function, e.c_str()));
    }
}

// No documentation available in wiki - internal function
void TLuaInterpreter::logEventError(const QString& event, const QString& error)
{
    // Log error to Editor's Errors TConsole:
    if (mpHost->mpEditorDialog) {
        mpHost->mpEditorDialog->mpErrorConsole->print(QStringLiteral("[%1:]").arg(tr("ERROR")), QColor(Qt::blue), QColor(Qt::black));
        mpHost->mpEditorDialog->mpErrorConsole->print(QStringLiteral(" event handler for %1:\n").arg(event), QColor(Qt::green), QColor(Qt::black));
        mpHost->mpEditorDialog->mpErrorConsole->print(QStringLiteral("        <%1>\n").arg(error), QColor(Qt::red), QColor(Qt::black));
    }

    // Log error to Profile's Main TConsole:
    if (mpHost->mEchoLuaErrors) {
        // ensure the Lua error is on a line of its own and is not prepended to the previous line
        if (mpHost->mpConsole->buffer.size() > 0 && !mpHost->mpConsole->buffer.lineBuffer.at(mpHost->mpConsole->buffer.lineBuffer.size() - 1).isEmpty()) {
            mpHost->postMessage(QStringLiteral("\n"));
        }

        mpHost->postMessage(QStringLiteral("[  LUA  ] - error in event handler for %1:\n<%2>").arg(event, error));
    }
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callConditionFunction(std::string& function, const QString& mName)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    lua_getfield(L, LUA_GLOBALSINDEX, function.c_str());
    int error = lua_pcall(L, 0, 1, 0);
    if (error != 0) {
        int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                QString _f = function.c_str();
                logError(e, mName, _f);
                if (mudlet::debugMode) {
                    TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: ERROR running script " << mName << " (" << function.c_str() << ") ERROR:" << e.c_str() << "\n" >> 0;
                }
            }
        }
    } else {
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::darkGreen)) << "LUA OK script " << mName << " (" << function.c_str() << ") ran without errors\n" >> 0;
        }
    }

    int ret = 0;
    int returnValues = lua_gettop(L);
    if (returnValues > 0) {
        // Lua docs: Like all tests in Lua, lua_toboolean returns 1 for any Lua value different from false and nil; otherwise it returns 0
        // This means trigger patterns don't have to strictly return true or false, as it is accepted in Lua */
        ret = lua_toboolean(L, 1);
    }
    lua_pop(L, returnValues);
    if ((error == 0) && (ret > 0)) {
        return true;
    } else {
        return false;
    }
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callMulti(const QString& function, const QString& mName)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return false;
    }

    if (!mMultiCaptureGroupList.empty()) {
        int k = 1;       // Lua indexes start with 1 as a general convention
        lua_newtable(L); //multimatches
        for (auto mit = mMultiCaptureGroupList.begin(); mit != mMultiCaptureGroupList.end(); mit++, k++) {
            // multimatches{ trigger_idx{ table_matches{ ... } } }
            lua_pushnumber(L, k);
            lua_newtable(L); //regex-value => table matches
            int i = 1;       // Lua indexes start with 1 as a general convention
            for (auto it = (*mit).begin(); it != (*mit).end(); it++, i++) {
                lua_pushnumber(L, i);
                lua_pushstring(L, (*it).c_str());
                lua_settable(L, -3); //match in matches
            }
            lua_settable(L, -3); //matches in regex
        }
        lua_setglobal(L, "multimatches");
    }

    lua_getglobal(L, function.toUtf8().constData());
    int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error != 0) {
        int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                logError(e, mName, function);
                if (mudlet::debugMode) {
                    TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: ERROR running script " << mName << " (" << function << ") ERROR:" << e.c_str() << "\n" >> 0;
                }
            }
        }
    } else {
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::darkGreen)) << "LUA OK script " << mName << " (" << function << ") ran without errors\n" >> 0;
        }
    }
    lua_pop(L, lua_gettop(L));
    if (error == 0) {
        return true;
    } else {
        return false;
    }
}

// No documentation available in wiki - internal function
std::pair<bool, bool> TLuaInterpreter::callMultiReturnBool(const QString& function, const QString& mName)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return std::make_pair(false, false);
    }

    bool returnValue = false;

    if (!mMultiCaptureGroupList.empty()) {
        int k = 1;       // Lua indexes start with 1 as a general convention
        lua_newtable(L); //multimatches
        for (auto mit = mMultiCaptureGroupList.begin(); mit != mMultiCaptureGroupList.end(); mit++, k++) {
            // multimatches{ trigger_idx{ table_matches{ ... } } }
            lua_pushnumber(L, k);
            lua_newtable(L); //regex-value => table matches
            int i = 1;       // Lua indexes start with 1 as a general convention
            for (auto it = (*mit).begin(); it != (*mit).end(); it++, i++) {
                lua_pushnumber(L, i);
                lua_pushstring(L, (*it).c_str());
                lua_settable(L, -3); //match in matches
            }
            lua_settable(L, -3); //matches in regex
        }
        lua_setglobal(L, "multimatches");
    }

    lua_getglobal(L, function.toUtf8().constData());
    int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error != 0) {
        int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                logError(e, mName, function);
                if (mudlet::debugMode) {
                    TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: ERROR running script " << mName << " (" << function << ") ERROR:" << e.c_str() << "\n" >> 0;
                }
            }
        }
    } else {
        auto index = lua_gettop(L);
        if (lua_isboolean(L, index)) {
            returnValue = lua_toboolean(L, index);
        }

        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::darkGreen)) << "LUA OK script " << mName << " (" << function << ") ran without errors\n" >> 0;
        }
    }
    lua_pop(L, lua_gettop(L));
    if (error == 0) {
        return std::make_pair(true, returnValue);
    } else {
        return std::make_pair(false, returnValue);
    }
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callCmdLineAction(const int func, QString text)
{
    lua_State* L = pGlobalLua;
    lua_rawgeti(L, LUA_REGISTRYINDEX, func);
    int error = 0;
    lua_pushstring(L, text.toUtf8().constData());
    error = lua_pcall(L, 1, LUA_MULTRET, 0);
    if (error) {
        std::string err = "";
        if (lua_isstring(L, -1)) {
            err += lua_tostring(L, -1);
        }

        QString function = "setCmdLineAction";
        QString name = "cmd line Action";
        logError(err, name, function);
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: ERROR running script " << function << " (" << function << ")\nError: " << QString::fromUtf8(err.c_str()) << "\n" >> 0;
        }
    }
    lua_pop(L, lua_gettop(L));
    return !error;
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callLabelCallbackEvent(const int func, const QEvent* qE)
{
    lua_State* L = pGlobalLua;
    lua_rawgeti(L, LUA_REGISTRYINDEX, func);
    int error = 0;
    if (qE) {
        // Create Lua table with QEvent data if needed
        switch (qE->type()) {
        // This means the default argument value was used, so ignore
        case (QEvent::None):
            error = lua_pcall(L, 0, LUA_MULTRET, 0);
            break;
        // These are all QMouseEvents
        case (QEvent::MouseButtonPress):
            [[fallthrough]];
        case (QEvent::MouseButtonDblClick):
            [[fallthrough]];
        case (QEvent::MouseButtonRelease):
            [[fallthrough]];
        case (QEvent::MouseMove): {
            auto qME = static_cast<const QMouseEvent*>(qE);
            lua_newtable(L);

            // push button()
            lua_pushstring(L, mMouseButtons.value(qME->button()).toUtf8().constData());
            lua_setfield(L, -2, QStringLiteral("button").toUtf8().constData());

            // push buttons()
            lua_newtable(L);
            QMap<Qt::MouseButton, QString>::const_iterator iter = mMouseButtons.constBegin();
            int counter = 1;
            while (iter != mMouseButtons.constEnd()) {
                if (iter.key() & qME->buttons()) {
                    lua_pushnumber(L, counter);
                    lua_pushstring(L, iter.value().toUtf8().constData());
                    lua_settable(L, -3);
                    counter++;
                }
                ++iter;
            }
            lua_setfield(L, -2, QStringLiteral("buttons").toUtf8().constData());

            // Push globalX()
            lua_pushnumber(L, qME->globalX());
            lua_setfield(L, -2, QStringLiteral("globalX").toUtf8().constData());

            // Push globalY()
            lua_pushnumber(L, qME->globalY());
            lua_setfield(L, -2, QStringLiteral("globalY").toUtf8().constData());

            // Push x()
            lua_pushnumber(L, qME->x());
            lua_setfield(L, -2, QStringLiteral("x").toUtf8().constData());

            // Push y()
            lua_pushnumber(L, qME->y());
            lua_setfield(L, -2, QStringLiteral("y").toUtf8().constData());

            error = lua_pcall(L, 1, LUA_MULTRET, 0);
            break;
        }
        // These are QEvents
        case (QEvent::Enter): {
            auto qME = static_cast<const QEnterEvent*>(qE);
            lua_newtable(L);

            // Push globalX()
            lua_pushnumber(L, qME->globalX());
            lua_setfield(L, -2, QStringLiteral("globalX").toUtf8().constData());

            // Push globalY()
            lua_pushnumber(L, qME->globalY());
            lua_setfield(L, -2, QStringLiteral("globalY").toUtf8().constData());

            // Push x()
            lua_pushnumber(L, qME->x());
            lua_setfield(L, -2, QStringLiteral("x").toUtf8().constData());

            // Push y()
            lua_pushnumber(L, qME->y());
            lua_setfield(L, -2, QStringLiteral("y").toUtf8().constData());

            error = lua_pcall(L, 1, LUA_MULTRET, 0);
            break;
        }
        case (QEvent::Leave): {
            // Seems there isn't a QLeaveEvent, so no
            // extra information to be gotten
            error = lua_pcall(L, 0, LUA_MULTRET, 0);
            break;
        }
        // This is a QWheelEvent
        case (QEvent::Wheel): {
            auto qME = static_cast<const QWheelEvent*>(qE);
            lua_newtable(L);

            // push buttons()
            lua_newtable(L);
            QMap<Qt::MouseButton, QString>::const_iterator iter = mMouseButtons.constBegin();
            int counter = 1;
            while (iter != mMouseButtons.constEnd()) {
                if (iter.key() & qME->buttons()) {
                    lua_pushnumber(L, counter);
                    lua_pushstring(L, iter.value().toUtf8().constData());
                    lua_settable(L, -3);
                    counter++;
                }
                ++iter;
            }
            lua_setfield(L, -2, QStringLiteral("buttons").toUtf8().constData());

            // Push globalX()
            lua_pushnumber(L, qME->globalX());
            lua_setfield(L, -2, QStringLiteral("globalX").toUtf8().constData());

            // Push globalY()
            lua_pushnumber(L, qME->globalY());
            lua_setfield(L, -2, QStringLiteral("globalY").toUtf8().constData());

            // Push x()
            lua_pushnumber(L, qME->x());
            lua_setfield(L, -2, QStringLiteral("x").toUtf8().constData());

            // Push y()
            lua_pushnumber(L, qME->y());
            lua_setfield(L, -2, QStringLiteral("y").toUtf8().constData());

            // Push angleDelta()
            lua_pushnumber(L, qME->angleDelta().x());
            lua_setfield(L, -2, QStringLiteral("angleDeltaX").toUtf8().constData());
            lua_pushnumber(L, qME->angleDelta().y());
            lua_setfield(L, -2, QStringLiteral("angleDeltaY").toUtf8().constData());

            error = lua_pcall(L, 1, LUA_MULTRET, 0);
            break;
        }
        }
    } else {
        error = lua_pcall(L, 0, LUA_MULTRET, 0);
    }

    if (error) {
        std::string err = "";
        if (lua_isstring(L, -1)) {
            err += lua_tostring(L, -1);
        }

        QString function = "setLabelCallback";
        QString name = "label callback event";
        logError(err, name, function);
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: ERROR running script " << function << " (" << function << ")\nError: " << QString::fromUtf8(err.c_str()) << "\n" >> 0;
        }
    }

    lua_pop(L, lua_gettop(L));
    return !error;
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callEventHandler(const QString& function, const TEvent& pE)
{
    if (function.isEmpty()) {
        return false;
    }

    lua_State* L = pGlobalLua;

    int error = luaL_dostring(L, QStringLiteral("return %1").arg(function).toUtf8().constData());
    if (error) {
        std::string err;
        if (lua_isstring(L, 1)) {
            err = "Lua error: ";
            err += lua_tostring(L, 1);
        }
        QString name = "event handler function";
        logError(err, name, function);
        return false;
    }

    // Lua is limited to ~50 arguments on a function
    auto maxArguments = std::min(pE.mArgumentList.size(), LUA_FUNCTION_MAX_ARGS);
    for (int i = 0; i < maxArguments; i++) {
        switch (pE.mArgumentTypeList.at(i)) {
        case ARGUMENT_TYPE_NUMBER:
            lua_pushnumber(L, pE.mArgumentList.at(i).toDouble());
            break;
        case ARGUMENT_TYPE_STRING:
            lua_pushstring(L, pE.mArgumentList.at(i).toUtf8().constData());
            break;
        case ARGUMENT_TYPE_BOOLEAN:
            lua_pushboolean(L, pE.mArgumentList.at(i).toInt());
            break;
        case ARGUMENT_TYPE_NIL:
            lua_pushnil(L);
            break;
        case ARGUMENT_TYPE_TABLE:
            lua_rawgeti(L, LUA_REGISTRYINDEX, pE.mArgumentList.at(i).toInt());
            break;
        case ARGUMENT_TYPE_FUNCTION:
            lua_rawgeti(L, LUA_REGISTRYINDEX, pE.mArgumentList.at(i).toInt());
            break;
        default:
            qWarning(R"(TLuaInterpreter::callEventHandler("%s", TEvent) ERROR: Unhandled ARGUMENT_TYPE: %i encountered in argument %i.)", function.toUtf8().constData(), pE.mArgumentTypeList.at(i), i);
            lua_pushnil(L);
        }
    }

    error = lua_pcall(L, maxArguments, LUA_MULTRET, 0);

    if (mudlet::debugMode && pE.mArgumentList.size() > LUA_FUNCTION_MAX_ARGS) {
        TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: ERROR running script " << function << " (" << function << ")\nError: more than " << LUA_FUNCTION_MAX_ARGS
                                                   << " arguments passed to Lua function, exceeding Lua's limit. Trimmed arguments to " << LUA_FUNCTION_MAX_ARGS << "\n"
                >> 0;
    }

    if (error) {
        std::string err = "";
        if (lua_isstring(L, -1)) {
            err += lua_tostring(L, -1);
        }
        QString name = "event handler function";
        logError(err, name, function);
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: ERROR running script " << function << " (" << function << ")\nError: " << QString::fromUtf8(err.c_str()) << "\n" >> 0;
        }
    }

    lua_pop(L, lua_gettop(L));
    return !error;
}

// No documentation available in wiki - internal function
double TLuaInterpreter::condenseMapLoad()
{
    QString luaFunction = QStringLiteral("condenseMapLoad");
    double loadTime = -1.0;

    lua_State* L = pGlobalLua;
    if (!L) {
        qWarning() << "condenseMapLoad: no suitable Lua execution unit found.";
        return false;
    }

    lua_getfield(L, LUA_GLOBALSINDEX, "condenseMapLoad");
    int error = lua_pcall(L, 0, 1, 0);
    if (error != 0) {
        int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                QString _f = luaFunction.toUtf8().constData();
                logError(e, luaFunction, _f);
                if (mudlet::debugMode) {
                    TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA: ERROR running " << luaFunction << " ERROR:" << e.c_str() << "\n" >> 0;
                }
            }
        }
    } else {
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::darkGreen)) << "LUA OK " << luaFunction << " ran without errors\n" >> 0;
        }
    }

    int returnValues = lua_gettop(L);
    if (returnValues > 0 && !lua_isnoneornil(L, 1)) {
        loadTime = lua_tonumber(L, 1);
    }
    lua_pop(L, returnValues);
    return loadTime;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAvailableFonts
int TLuaInterpreter::getAvailableFonts(lua_State* L)
{
    auto fontList = mudlet::self()->getAvailableFonts();

    lua_newtable(L);
    for (auto& font : fontList) {
        lua_pushstring(L, font.toUtf8().constData());
        lua_pushboolean(L, true);
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#putHTTP
int TLuaInterpreter::putHTTP(lua_State* L)
{
    auto& host = getHostFromLua(L);

    QString dataToPost;
    if (!lua_isstring(L, 1) && !lua_isstring(L, 4)) {
        lua_pushfstring(L, "putHTTP: bad argument #1 type (data to send as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else if (lua_isstring(L, 1)) {
        dataToPost = QString::fromUtf8(lua_tostring(L, 1));
    }

    QString urlString;
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "putHTTP: bad argument #2 type (remote url as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else {
        urlString = QString::fromUtf8(lua_tostring(L, 2));
    }

    QUrl url = QUrl::fromUserInput(urlString);

    if (!url.isValid()) {
        lua_pushnil(L);
        lua_pushfstring(L,
                        "putHTTP: bad argument #2 value (url is not deemed valid), validation\n"
                        "produced the following error message:\n%s.",
                        url.errorString().toUtf8().constData());
        return 2;
    }

    QNetworkRequest request = QNetworkRequest(url);
    mudlet::self()->setNetworkRequestDefaults(url, request);

    if (!lua_istable(L, 3) && !lua_isnoneornil(L, 3)) {
        lua_pushfstring(L, "putHTTP: bad argument #3 type (headers as a table expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    } else if (lua_istable(L, 3)) {
        lua_pushnil(L);
        while (lua_next(L, 3) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING && lua_type(L, -2) == LUA_TSTRING) {
                QString cmd = lua_tostring(L, -1);
                request.setRawHeader(QByteArray(lua_tostring(L, -2)), QByteArray(lua_tostring(L, -1)));
            } else {
                lua_pushfstring(L,
                                "putHTTP: bad argument #3 type (custom headers must be strings, got header: %s (should be string) and value: %s (should be string))",
                                luaL_typename(L, -2),
                                luaL_typename(L, -1));
                return lua_error(L);
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
    }

    QByteArray fileToUpload;
    QString fileLocation;
    if (!lua_isstring(L, 4) && !lua_isnoneornil(L, 4)) {
        lua_pushfstring(L, "putHTTP: bad argument #4 type (file to send as string location expected, got %s!)", luaL_typename(L, 4));
        return lua_error(L);
    } else if (lua_isstring(L, 4)) {
        fileLocation = QString::fromUtf8(lua_tostring(L, 4));
    }

    if (!fileLocation.isEmpty()) {
        QFile file(fileLocation);
        if (!file.open(QFile::ReadOnly)) {
            lua_pushnil(L);
            lua_pushfstring(L, "couldn't open \"%s\", is the location correct and do you have permissions to it?", fileLocation.toUtf8().constData());
            return 2;
        }

        fileToUpload = file.readAll();
        file.close();
    }

    host.updateProxySettings(host.mLuaInterpreter.mpFileDownloader);
    QNetworkReply* reply = host.mLuaInterpreter.mpFileDownloader->put(request, fileToUpload.isEmpty() ?dataToPost.toUtf8() : fileToUpload);

    if (mudlet::debugMode) {
        TDebug(QColor(Qt::white), QColor(Qt::blue)) << "putHTTP: script is uploading data to " << reply->url().toString() << "\n" >> 0;
    }

    lua_pushboolean(L, true);
    lua_pushstring(L, reply->url().toString().toUtf8().constData()); // Returns the Url that was ACTUALLY used
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getHTTP
int TLuaInterpreter::getHTTP(lua_State* L)
{
    auto& host = getHostFromLua(L);

    QString urlString;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "getHTTP: bad argument #1 type (remote url as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        urlString = QString::fromUtf8(lua_tostring(L, 1));
    }

    QUrl url = QUrl::fromUserInput(urlString);

    if (!url.isValid()) {
        lua_pushnil(L);
        lua_pushfstring(L,
                        "getHTTP: bad argument #1 value (url is not deemed valid), validation\n"
                        "produced the following error message:\n%s.",
                        url.errorString().toUtf8().constData());
        return 2;
    }

    QNetworkRequest request = QNetworkRequest(url);
    mudlet::self()->setNetworkRequestDefaults(url, request);

    if (!lua_istable(L, 2) && !lua_isnoneornil(L, 2)) {
        lua_pushfstring(L, "getHTTP: bad argument #2 type (headers as a table expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else if (lua_istable(L, 2)) {
        lua_pushnil(L);
        while (lua_next(L, 2) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING && lua_type(L, -2) == LUA_TSTRING) {
                QString cmd = lua_tostring(L, -1);
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

    if (mudlet::debugMode) {
        TDebug(QColor(Qt::white), QColor(Qt::blue)) << QStringLiteral("getHTTP: script is getting data from %1\n").arg(reply->url().toString()) >> 0;
    }

    lua_pushboolean(L, true);
    lua_pushstring(L, reply->url().toString().toUtf8().constData()); // Returns the Url that was ACTUALLY used
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#postHTTP
int TLuaInterpreter::postHTTP(lua_State* L)
{
    auto& host = getHostFromLua(L);

    QString dataToPost;
    if (!lua_isstring(L, 1) && !lua_isstring(L, 4)) {
        lua_pushfstring(L, "postHTTP: bad argument #1 type (data to send as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else if (lua_isstring(L, 1)) {
        dataToPost = QString::fromUtf8(lua_tostring(L, 1));
    }

    QString urlString;
    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "postHTTP: bad argument #2 type (remote url as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else {
        urlString = QString::fromUtf8(lua_tostring(L, 2));
    }

    QUrl url = QUrl::fromUserInput(urlString);

    if (!url.isValid()) {
        lua_pushnil(L);
        lua_pushfstring(L,
                        "postHTTP: bad argument #2 value (url is not deemed valid), validation\n"
                        "produced the following error message:\n%s.",
                        url.errorString().toUtf8().constData());
        return 2;
    }

    QNetworkRequest request = QNetworkRequest(url);
    mudlet::self()->setNetworkRequestDefaults(url, request);

    if (!lua_istable(L, 3) && !lua_isnoneornil(L, 3)) {
        lua_pushfstring(L, "postHTTP: bad argument #3 type (headers as a table expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    } else if (lua_istable(L, 3)) {
        lua_pushnil(L);
        while (lua_next(L, 3) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING && lua_type(L, -2) == LUA_TSTRING) {
                QString cmd = lua_tostring(L, -1);
                request.setRawHeader(QByteArray(lua_tostring(L, -2)), QByteArray(lua_tostring(L, -1)));
            } else {
                lua_pushfstring(L,
                                "postHTTP: bad argument #3 type (custom headers must be strings, got header: %s (should be string) and value: %s (should be string))",
                                luaL_typename(L, -2),
                                luaL_typename(L, -1));
                return lua_error(L);
            }
            // removes value, but keeps key for next iteration
            lua_pop(L, 1);
        }
    }

    QByteArray fileToUpload;
    QString fileLocation;
    if (!lua_isstring(L, 4) && !lua_isnoneornil(L, 4)) {
        lua_pushfstring(L, "postHTTP: bad argument #4 type (file to send as string location expected, got %s!)", luaL_typename(L, 4));
        return lua_error(L);
    } else if (lua_isstring(L, 4)) {
            fileLocation = QString::fromUtf8(lua_tostring(L, 4));
        }

    if (!fileLocation.isEmpty()) {
        QFile file(fileLocation);
        if (!file.open(QFile::ReadOnly)) {
            lua_pushnil(L);
            lua_pushfstring(L, "couldn't open \"%s\", is the location correct and do you have permissions to it?", fileLocation.toUtf8().constData());
            return 2;
        }

        fileToUpload = file.readAll();
        file.close();
    }

    host.updateProxySettings(host.mLuaInterpreter.mpFileDownloader);
    QNetworkReply* reply = host.mLuaInterpreter.mpFileDownloader->post(request, fileToUpload.isEmpty() ? dataToPost.toUtf8() : fileToUpload);

    if (mudlet::debugMode) {
        TDebug(QColor(Qt::white), QColor(Qt::blue)) << QStringLiteral("postHTTP: script is uploading data to %1\n").arg(reply->url().toString()) >> 0;
    }

    lua_pushboolean(L, true);
    lua_pushstring(L, reply->url().toString().toUtf8().constData()); // Returns the Url that was ACTUALLY used
    return 2;
}

int TLuaInterpreter::deleteHTTP(lua_State *L)
{
    auto& host = getHostFromLua(L);

    QString urlString;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "deleteHTTP: bad argument #1 type (remote url as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        urlString = QString::fromUtf8(lua_tostring(L, 1));
    }

    QUrl url = QUrl::fromUserInput(urlString);

    if (!url.isValid()) {
        lua_pushnil(L);
        lua_pushfstring(L,
                        "deleteHTTP: bad argument #1 value (url is not deemed valid), validation\n"
                        "produced the following error message:\n%s.",
                        url.errorString().toUtf8().constData());
        return 2;
    }

    QNetworkRequest request = QNetworkRequest(url);
    mudlet::self()->setNetworkRequestDefaults(url, request);

    if (!lua_istable(L, 2) && !lua_isnoneornil(L, 2)) {
        lua_pushfstring(L, "deleteHTTP: bad argument #2 type (headers as a table expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    } else if (lua_istable(L, 2)) {
        lua_pushnil(L);
        while (lua_next(L, 2) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING && lua_type(L, -2) == LUA_TSTRING) {

                QString cmd = lua_tostring(L, -1);
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

    if (mudlet::debugMode) {
        TDebug(QColor(Qt::white), QColor(Qt::blue)) << QStringLiteral("deleteHTTP: script is sending delete request for %1\n").arg(reply->url().toString()) >> 0;
    }

    lua_pushboolean(L, true);
    lua_pushstring(L, reply->url().toString().toUtf8().constData()); // Returns the Url that was ACTUALLY used
    return 2;
}


// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getConnectionInfo
int TLuaInterpreter::getConnectionInfo(lua_State *L)
{
    Host& host = getHostFromLua(L);

    auto [hostName, hostPort] = host.mTelnet.getConnectionInfo();
    lua_pushstring(L, hostName.toUtf8().constData());
    lua_pushnumber(L, hostPort);
    return 2;
}

int TLuaInterpreter::unzipAsync(lua_State *L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "unzipAsync: bad argument #1 type (zip location as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString zipLocation {QString::fromUtf8(lua_tostring(L, 1))};

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "unzipAsync: bad argument #2 type (extract location as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    QString extractLocation {QString::fromUtf8(lua_tostring(L, 2))};

    QTemporaryDir temporaryDir;
    if (!temporaryDir.isValid()) {
        lua_pushnil(L);
        lua_pushstring(L,
                       "couldn't create temporary directory to extract the zip into");
        return 2;
    }

    extractLocation = QDir::fromNativeSeparators(extractLocation);
    if (!extractLocation.endsWith(QLatin1String("/"))) {
        extractLocation.append(QLatin1String("/"));
    }

    QDir dir;
    if (!dir.mkpath(extractLocation)) {
        lua_pushnil(L);
        lua_pushstring(L,
                       "couldn't create output directory to put the extracted files into");
        return 2;
    }

    auto future = QtConcurrent::run(mudlet::unzip, zipLocation, extractLocation, temporaryDir.path());
    auto watcher = new QFutureWatcher<bool>;
    QObject::connect(watcher, &QFutureWatcher<bool>::finished, [=]() {
        TEvent event {};
        Host& host = getHostFromLua(L);

        if (future.result()) {
            event.mArgumentList.append(QStringLiteral("sysUnzipDone"));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        } else {
            event.mArgumentList.append(QStringLiteral("sysUnzipError"));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        }

        event.mArgumentList.append(zipLocation);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        event.mArgumentList.append(extractLocation);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        host.raiseEvent(event);
    });
    watcher->setFuture(future);

    lua_pushboolean(L, true);
    return 1;
}

// No documentation available in wiki - internal function
void TLuaInterpreter::set_lua_table(const QString& tableName, QStringList& variableList)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return;
    }
    lua_newtable(L);
    for (int i = 0; i < variableList.size(); i++) {
        lua_pushnumber(L, i + 1); // Lua indexes start with 1
        lua_pushstring(L, variableList[i].toUtf8().constData());
        lua_settable(L, -3);
    }
    lua_setglobal(L, tableName.toUtf8().constData());
    lua_pop(pGlobalLua, lua_gettop(pGlobalLua));
}

// No documentation available in wiki - internal function
void TLuaInterpreter::set_lua_string(const QString& varName, const QString& varValue)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return;
    }

    lua_pushstring(L, varValue.toUtf8().constData());
    lua_setglobal(L, varName.toUtf8().constData());
    lua_pop(pGlobalLua, lua_gettop(pGlobalLua));
}

// No documentation available in wiki - internal function
QString TLuaInterpreter::getLuaString(const QString& stringName)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return QStringLiteral("LUA CRITICAL ERROR");
    }

    int error = luaL_dostring(L, QStringLiteral("return %1").arg(stringName).toUtf8().constData());
    if (!error) {
        return QString::fromUtf8(lua_tostring(L, 1));
    } else {
        return QString();
    }
}

// check for <whitespace><no_valid_representation> as output

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#handleWindowResizeEvent is using noop function publicly - compare initLuaGlobals()
int TLuaInterpreter::noop(lua_State* L)
{
    return 0;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::check_for_mappingscript()
{
    lua_State* L = pGlobalLua;
    lua_getglobal(L, "mudlet");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }

    lua_getfield(L, -1, "mapper_script");
    if (!lua_isboolean(L, -1)) {
        lua_pop(L, 2);
        return 0;
    }

    int r = lua_toboolean(L, -1);
    lua_pop(L, 2);
    return r;
}

#if defined(_MSC_VER) && defined(_DEBUG)
// Enable leak detection for MSVC debug builds.

#define LUA_CLIENT_TYPE (_CLIENT_BLOCK | ((('L' << 8) | 'U') << 16))

// No documentation available in wiki - internal function
static void* l_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
    (void)ud;
    (void)osize;
    if (nsize == 0) {
        ::_free_dbg(ptr, LUA_CLIENT_TYPE);
        return NULL;
    } else {
        return ::_realloc_dbg(ptr, nsize, LUA_CLIENT_TYPE, __FILE__, __LINE__);
    }
}

// No documentation available in wiki - internal function
static int panic(lua_State* L)
{
    fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n", lua_tostring(L, -1));
    return 0;
}

// No documentation available in wiki - internal function
static lua_State* newstate()
{
    lua_State* L = lua_newstate(l_alloc, NULL);
    if (L) {
        lua_atpanic(L, &panic);
    }
    return L;
}

#else

// No documentation available in wiki - internal function
static lua_State* newstate()
{
    return luaL_newstate();
}

#endif // _MSC_VER && _DEBUG

// No documentation available in wiki - internal function
static void storeHostInLua(lua_State* L, Host* h);

// No documentation available in wiki - internal helper funtion
// On success will swap out any messages in the queue and replace them
// with it's success message, on failure will just append...
bool TLuaInterpreter::loadLuaModule(QQueue<QString>& resultMsgsQueue, const QString& requirement, const QString& failureConsequence, const QString& description, const QString& luaModuleId)
{
    int error = luaL_dostring(pGlobalLua, QStringLiteral("%1require \"%2\"")
                              .arg(luaModuleId.isEmpty() ? QString() : QStringLiteral("%1 =").arg(luaModuleId),
                                   requirement).toUtf8().constData());
    if (error) {
        QString luaErrorMsg = tr("No error message available from Lua");
        if (lua_isstring(pGlobalLua, -1)) {
            luaErrorMsg = tr("Lua error: %1").arg(QString::fromUtf8(lua_tostring(pGlobalLua, -1)));
        }
        resultMsgsQueue.enqueue(tr("[ ERROR ] - Cannot find Lua module %1.%2%3%4",
                                   // Intentional comment to separate arguments
                                   "%1 is the name of the module;"
                                   "%2 will be a line-feed inserted to put the next argument on a new line;"
                                   "%3 is the error message from the lua sub-system;"
                                   "%4 can be an additional message about the expected effect (but may be blank).")
                                .arg((description.isEmpty() ? requirement : description),
                                     QLatin1String("\n"),
                                     luaErrorMsg,
                                     (failureConsequence.isEmpty() ? QString() : QStringLiteral("\n%1").arg(failureConsequence))));
        return false;
    }

    QQueue<QString> newMessageQueue;
    newMessageQueue.enqueue(tr("[  OK  ]  - Lua module %1 loaded.",
                               "%1 is the name (may specify which variant) of the module.")
                            .arg(description.isEmpty() ? requirement : description));
    resultMsgsQueue.swap(newMessageQueue);
    return true;
}

// No documentation available in wiki - internal function
void TLuaInterpreter::insertNativeSeparatorsFunction(lua_State* L)
{
    // This is likely only needed for Windows but should not be harmful for
    // other OSes and it keeps things simpler if they all use it:
    // clang-format off
    luaL_dostring(L, R"LUA(function toNativeSeparators(rawPath)
  if package.config:sub(1,1) == '\\' then
    return string.gsub(rawPath, '/', '\\')
  end

  assert((package.config:sub(1,1) == '/'), "package path directory separator is neither '\\' nor '/' and cannot be handled")

  return string.gsub(rawPath, '\\', '/')
end)LUA");
    // clang-format on
}

// No documentation available in wiki - internal function
// This function initializes the main Lua Session interpreter.
// on initialization of a new session *or* in case of an interpreter reset by the user.
void TLuaInterpreter::initLuaGlobals()
{
    pGlobalLua = newstate();
    storeHostInLua(pGlobalLua, mpHost);

    luaL_openlibs(pGlobalLua);

    lua_pushstring(pGlobalLua, "SESSION");
    lua_pushnumber(pGlobalLua, mHostID);
    lua_settable(pGlobalLua, LUA_GLOBALSINDEX);

    lua_pushstring(pGlobalLua, "SCRIPT_NAME");
    lua_pushstring(pGlobalLua, "Global Lua Session Interpreter");
    lua_settable(pGlobalLua, LUA_GLOBALSINDEX);

    lua_pushstring(pGlobalLua, "SCRIPT_ID");
    lua_pushnumber(pGlobalLua, -1); // ID 1 is used to indicate that this is the global Lua interpreter
    lua_settable(pGlobalLua, LUA_GLOBALSINDEX);
    lua_register(pGlobalLua, "showUnzipProgress", TLuaInterpreter::showUnzipProgress); //internal function used by the package system NOT FOR USERS
    lua_register(pGlobalLua, "wait", TLuaInterpreter::Wait);
    lua_register(pGlobalLua, "expandAlias", TLuaInterpreter::expandAlias);
    lua_register(pGlobalLua, "echo", TLuaInterpreter::Echo);
    lua_register(pGlobalLua, "selectString", TLuaInterpreter::selectString);
    lua_register(pGlobalLua, "selectSection", TLuaInterpreter::selectSection);
    lua_register(pGlobalLua, "replace", TLuaInterpreter::replace);
    lua_register(pGlobalLua, "setBgColor", TLuaInterpreter::setBgColor);
    lua_register(pGlobalLua, "setFgColor", TLuaInterpreter::setFgColor);
    lua_register(pGlobalLua, "tempTimer", TLuaInterpreter::tempTimer);
    lua_register(pGlobalLua, "tempTrigger", TLuaInterpreter::tempTrigger);
    lua_register(pGlobalLua, "tempRegexTrigger", TLuaInterpreter::tempRegexTrigger);
    lua_register(pGlobalLua, "closeMudlet", TLuaInterpreter::closeMudlet);
    lua_register(pGlobalLua, "loadWindowLayout", TLuaInterpreter::loadWindowLayout);
    lua_register(pGlobalLua, "saveWindowLayout", TLuaInterpreter::saveWindowLayout);
    lua_register(pGlobalLua, "setFont", TLuaInterpreter::setFont);
    lua_register(pGlobalLua, "getFont", TLuaInterpreter::getFont);
    lua_register(pGlobalLua, "setFontSize", TLuaInterpreter::setFontSize);
    lua_register(pGlobalLua, "getFontSize", TLuaInterpreter::getFontSize);
    lua_register(pGlobalLua, "openUserWindow", TLuaInterpreter::openUserWindow);
    lua_register(pGlobalLua, "setUserWindowTitle", TLuaInterpreter::setUserWindowTitle);
    lua_register(pGlobalLua, "echoUserWindow", TLuaInterpreter::echoUserWindow);
    lua_register(pGlobalLua, "enableTimer", TLuaInterpreter::enableTimer);
    lua_register(pGlobalLua, "disableTimer", TLuaInterpreter::disableTimer);
    lua_register(pGlobalLua, "enableKey", TLuaInterpreter::enableKey);
    lua_register(pGlobalLua, "disableKey", TLuaInterpreter::disableKey);
    lua_register(pGlobalLua, "killKey", TLuaInterpreter::killKey);
    lua_register(pGlobalLua, "clearUserWindow", TLuaInterpreter::clearUserWindow);
    lua_register(pGlobalLua, "clearWindow", TLuaInterpreter::clearUserWindow);
    lua_register(pGlobalLua, "killTimer", TLuaInterpreter::killTimer);
    lua_register(pGlobalLua, "remainingTime", TLuaInterpreter::remainingTime);
    lua_register(pGlobalLua, "moveCursor", TLuaInterpreter::moveCursor);
    lua_register(pGlobalLua, "getLines", TLuaInterpreter::getLines);
    lua_register(pGlobalLua, "getLineNumber", TLuaInterpreter::getLineNumber);
    lua_register(pGlobalLua, "insertHTML", TLuaInterpreter::insertHTML);
    lua_register(pGlobalLua, "insertText", TLuaInterpreter::insertText);
    lua_register(pGlobalLua, "enableTrigger", TLuaInterpreter::enableTrigger);
    lua_register(pGlobalLua, "disableTrigger", TLuaInterpreter::disableTrigger);
    lua_register(pGlobalLua, "enableScript", TLuaInterpreter::enableScript);
    lua_register(pGlobalLua, "disableScript", TLuaInterpreter::disableScript);
    lua_register(pGlobalLua, "killTrigger", TLuaInterpreter::killTrigger);
    lua_register(pGlobalLua, "getLineCount", TLuaInterpreter::getLineCount);
    lua_register(pGlobalLua, "getColumnNumber", TLuaInterpreter::getColumnNumber);
    lua_register(pGlobalLua, "send", TLuaInterpreter::sendRaw);
    lua_register(pGlobalLua, "selectCaptureGroup", TLuaInterpreter::selectCaptureGroup);
    lua_register(pGlobalLua, "tempLineTrigger", TLuaInterpreter::tempLineTrigger);
    lua_register(pGlobalLua, "raiseEvent", TLuaInterpreter::raiseEvent);
    lua_register(pGlobalLua, "deleteLine", TLuaInterpreter::deleteLine);
    lua_register(pGlobalLua, "copy", TLuaInterpreter::copy);
    lua_register(pGlobalLua, "cut", TLuaInterpreter::cut);
    lua_register(pGlobalLua, "paste", TLuaInterpreter::paste);
    lua_register(pGlobalLua, "pasteWindow", TLuaInterpreter::pasteWindow);
    lua_register(pGlobalLua, "debugc", TLuaInterpreter::debug);
    lua_register(pGlobalLua, "showHandlerError", TLuaInterpreter::showHandlerError);
    lua_register(pGlobalLua, "setWindowWrap", TLuaInterpreter::setWindowWrap);
    lua_register(pGlobalLua, "setWindowWrapIndent", TLuaInterpreter::setWindowWrapIndent);
    lua_register(pGlobalLua, "resetFormat", TLuaInterpreter::resetFormat);
    lua_register(pGlobalLua, "moveCursorEnd", TLuaInterpreter::moveCursorEnd);
    lua_register(pGlobalLua, "getLastLineNumber", TLuaInterpreter::getLastLineNumber);
    lua_register(pGlobalLua, "getNetworkLatency", TLuaInterpreter::getNetworkLatency);
    lua_register(pGlobalLua, "createMiniConsole", TLuaInterpreter::createMiniConsole);
    lua_register(pGlobalLua, "createLabel", TLuaInterpreter::createLabel);
    lua_register(pGlobalLua, "deleteLabel", TLuaInterpreter::deleteLabel);
    lua_register(pGlobalLua, "setLabelToolTip", TLuaInterpreter::setLabelToolTip);
    lua_register(pGlobalLua, "setLabelCursor", TLuaInterpreter::setLabelCursor);
    lua_register(pGlobalLua, "setLabelCustomCursor", TLuaInterpreter::setLabelCustomCursor);
    lua_register(pGlobalLua, "raiseWindow", TLuaInterpreter::raiseWindow);
    lua_register(pGlobalLua, "lowerWindow", TLuaInterpreter::lowerWindow);
    lua_register(pGlobalLua, "hideWindow", TLuaInterpreter::hideUserWindow);
    lua_register(pGlobalLua, "showWindow", TLuaInterpreter::showUserWindow);
    lua_register(pGlobalLua, "createBuffer", TLuaInterpreter::createBuffer);
    lua_register(pGlobalLua, "createStopWatch", TLuaInterpreter::createStopWatch);
    lua_register(pGlobalLua, "getStopWatchTime", TLuaInterpreter::getStopWatchTime);
    lua_register(pGlobalLua, "stopStopWatch", TLuaInterpreter::stopStopWatch);
    lua_register(pGlobalLua, "startStopWatch", TLuaInterpreter::startStopWatch);
    lua_register(pGlobalLua, "resetStopWatch", TLuaInterpreter::resetStopWatch);
    lua_register(pGlobalLua, "adjustStopWatch", TLuaInterpreter::adjustStopWatch);
    lua_register(pGlobalLua, "deleteStopWatch", TLuaInterpreter::deleteStopWatch);
    lua_register(pGlobalLua, "setStopWatchPersistence", TLuaInterpreter::setStopWatchPersistence);
    lua_register(pGlobalLua, "getStopWatches", TLuaInterpreter::getStopWatches);
    lua_register(pGlobalLua, "setStopWatchName", TLuaInterpreter::setStopWatchName);
    lua_register(pGlobalLua, "getStopWatchBrokenDownTime", TLuaInterpreter::getStopWatchBrokenDownTime);
    lua_register(pGlobalLua, "closeUserWindow", TLuaInterpreter::closeUserWindow);
    lua_register(pGlobalLua, "resizeWindow", TLuaInterpreter::resizeWindow);
    lua_register(pGlobalLua, "appendBuffer", TLuaInterpreter::appendBuffer);
    lua_register(pGlobalLua, "setBackgroundImage", TLuaInterpreter::setBackgroundImage);
    lua_register(pGlobalLua, "setBackgroundColor", TLuaInterpreter::setBackgroundColor);
    lua_register(pGlobalLua, "setCmdLineAction", TLuaInterpreter::setCmdLineAction);
    lua_register(pGlobalLua, "resetCmdLineAction", TLuaInterpreter::resetCmdLineAction);
    lua_register(pGlobalLua, "setLabelClickCallback", TLuaInterpreter::setLabelClickCallback);
    lua_register(pGlobalLua, "setLabelDoubleClickCallback", TLuaInterpreter::setLabelDoubleClickCallback);
    lua_register(pGlobalLua, "setLabelReleaseCallback", TLuaInterpreter::setLabelReleaseCallback);
    lua_register(pGlobalLua, "setLabelMoveCallback", TLuaInterpreter::setLabelMoveCallback);
    lua_register(pGlobalLua, "setLabelWheelCallback", TLuaInterpreter::setLabelWheelCallback);
    lua_register(pGlobalLua, "setLabelOnEnter", TLuaInterpreter::setLabelOnEnter);
    lua_register(pGlobalLua, "setLabelOnLeave", TLuaInterpreter::setLabelOnLeave);
    lua_register(pGlobalLua, "getImageSize", TLuaInterpreter::getImageSize);
    lua_register(pGlobalLua, "moveWindow", TLuaInterpreter::moveWindow);
    lua_register(pGlobalLua, "setWindow", TLuaInterpreter::setWindow);
    lua_register(pGlobalLua, "openMapWidget", TLuaInterpreter::openMapWidget);
    lua_register(pGlobalLua, "closeMapWidget", TLuaInterpreter::closeMapWidget);
    lua_register(pGlobalLua, "setTextFormat", TLuaInterpreter::setTextFormat);
    lua_register(pGlobalLua, "getMainWindowSize", TLuaInterpreter::getMainWindowSize);
    lua_register(pGlobalLua, "getUserWindowSize", TLuaInterpreter::getUserWindowSize);
    lua_register(pGlobalLua, "getMousePosition", TLuaInterpreter::getMousePosition);
    lua_register(pGlobalLua, "setProfileIcon", TLuaInterpreter::setProfileIcon);
    lua_register(pGlobalLua, "resetProfileIcon", TLuaInterpreter::resetProfileIcon);
    lua_register(pGlobalLua, "getCurrentLine", TLuaInterpreter::getCurrentLine);
    lua_register(pGlobalLua, "setMiniConsoleFontSize", TLuaInterpreter::setMiniConsoleFontSize);
    lua_register(pGlobalLua, "selectCurrentLine", TLuaInterpreter::selectCurrentLine);
    lua_register(pGlobalLua, "spawn", TLuaInterpreter::spawn);
    lua_register(pGlobalLua, "getButtonState", TLuaInterpreter::getButtonState);
    lua_register(pGlobalLua, "showToolBar", TLuaInterpreter::showToolBar);
    lua_register(pGlobalLua, "hideToolBar", TLuaInterpreter::hideToolBar);
    lua_register(pGlobalLua, "loadRawFile", TLuaInterpreter::loadRawFile);
    lua_register(pGlobalLua, "setBold", TLuaInterpreter::setBold);
    lua_register(pGlobalLua, "setItalics", TLuaInterpreter::setItalics);
    lua_register(pGlobalLua, "setOverline", TLuaInterpreter::setOverline);
    lua_register(pGlobalLua, "setReverse", TLuaInterpreter::setReverse);
    lua_register(pGlobalLua, "setStrikeOut", TLuaInterpreter::setStrikeOut);
    lua_register(pGlobalLua, "setUnderline", TLuaInterpreter::setUnderline);
    lua_register(pGlobalLua, "disconnect", TLuaInterpreter::disconnect);
    lua_register(pGlobalLua, "tempButtonToolbar", TLuaInterpreter::tempButtonToolbar);
    lua_register(pGlobalLua, "tempButton", TLuaInterpreter::tempButton);
    lua_register(pGlobalLua, "setButtonStyleSheet", TLuaInterpreter::setButtonStyleSheet);
    lua_register(pGlobalLua, "reconnect", TLuaInterpreter::reconnect);
    lua_register(pGlobalLua, "getMudletHomeDir", TLuaInterpreter::getMudletHomeDir);
    lua_register(pGlobalLua, "setTriggerStayOpen", TLuaInterpreter::setTriggerStayOpen);
    lua_register(pGlobalLua, "wrapLine", TLuaInterpreter::wrapLine);
    lua_register(pGlobalLua, "getFgColor", TLuaInterpreter::getFgColor);
    lua_register(pGlobalLua, "getBgColor", TLuaInterpreter::getBgColor);
    lua_register(pGlobalLua, "tempColorTrigger", TLuaInterpreter::tempColorTrigger);
    lua_register(pGlobalLua, "isAnsiFgColor", TLuaInterpreter::isAnsiFgColor);
    lua_register(pGlobalLua, "isAnsiBgColor", TLuaInterpreter::isAnsiBgColor);
    lua_register(pGlobalLua, "stopSounds", TLuaInterpreter::stopSounds);
    lua_register(pGlobalLua, "playSoundFile", TLuaInterpreter::playSoundFile);
    lua_register(pGlobalLua, "setBorderSizes", TLuaInterpreter::setBorderSizes);
    lua_register(pGlobalLua, "setBorderTop", TLuaInterpreter::setBorderTop);
    lua_register(pGlobalLua, "setBorderRight", TLuaInterpreter::setBorderRight);
    lua_register(pGlobalLua, "setBorderBottom", TLuaInterpreter::setBorderBottom);
    lua_register(pGlobalLua, "setBorderLeft", TLuaInterpreter::setBorderLeft);
    lua_register(pGlobalLua, "setBorderColor", TLuaInterpreter::setBorderColor);
    lua_register(pGlobalLua, "getBorderTop", TLuaInterpreter::getBorderTop);
    lua_register(pGlobalLua, "getBorderRight", TLuaInterpreter::getBorderRight);
    lua_register(pGlobalLua, "getBorderBottom", TLuaInterpreter::getBorderBottom);
    lua_register(pGlobalLua, "getBorderLeft", TLuaInterpreter::getBorderLeft);
    lua_register(pGlobalLua, "getBorderSizes", TLuaInterpreter::getBorderSizes);
    lua_register(pGlobalLua, "setConsoleBufferSize", TLuaInterpreter::setConsoleBufferSize);
    lua_register(pGlobalLua, "enableScrollBar", TLuaInterpreter::enableScrollBar);
    lua_register(pGlobalLua, "disableScrollBar", TLuaInterpreter::disableScrollBar);
    lua_register(pGlobalLua, "enableCommandLine", TLuaInterpreter::enableCommandLine);
    lua_register(pGlobalLua, "disableCommandLine", TLuaInterpreter::disableCommandLine);
    lua_register(pGlobalLua, "startLogging", TLuaInterpreter::startLogging);
    lua_register(pGlobalLua, "calcFontSize", TLuaInterpreter::calcFontSize);
    lua_register(pGlobalLua, "permRegexTrigger", TLuaInterpreter::permRegexTrigger);
    lua_register(pGlobalLua, "permSubstringTrigger", TLuaInterpreter::permSubstringTrigger);
    lua_register(pGlobalLua, "permBeginOfLineStringTrigger", TLuaInterpreter::permBeginOfLineStringTrigger);
    lua_register(pGlobalLua, "tempComplexRegexTrigger", TLuaInterpreter::tempComplexRegexTrigger);
    lua_register(pGlobalLua, "permTimer", TLuaInterpreter::permTimer);
    lua_register(pGlobalLua, "permScript", TLuaInterpreter::permScript);
    lua_register(pGlobalLua, "getScript", TLuaInterpreter::getScript);
    lua_register(pGlobalLua, "setScript", TLuaInterpreter::setScript);
    lua_register(pGlobalLua, "permAlias", TLuaInterpreter::permAlias);
    lua_register(pGlobalLua, "permKey", TLuaInterpreter::permKey);
    lua_register(pGlobalLua, "tempKey", TLuaInterpreter::tempKey);
    lua_register(pGlobalLua, "exists", TLuaInterpreter::exists);
    lua_register(pGlobalLua, "isActive", TLuaInterpreter::isActive);
    lua_register(pGlobalLua, "enableAlias", TLuaInterpreter::enableAlias);
    lua_register(pGlobalLua, "tempAlias", TLuaInterpreter::tempAlias);
    lua_register(pGlobalLua, "disableAlias", TLuaInterpreter::disableAlias);
    lua_register(pGlobalLua, "killAlias", TLuaInterpreter::killAlias);
    lua_register(pGlobalLua, "setLabelStyleSheet", TLuaInterpreter::setLabelStyleSheet);
    lua_register(pGlobalLua, "setUserWindowStyleSheet", TLuaInterpreter::setUserWindowStyleSheet);
    lua_register(pGlobalLua, "getTime", TLuaInterpreter::getTime);
    lua_register(pGlobalLua, "getEpoch", TLuaInterpreter::getEpoch);
    lua_register(pGlobalLua, "invokeFileDialog", TLuaInterpreter::invokeFileDialog);
    lua_register(pGlobalLua, "getTimestamp", TLuaInterpreter::getTimestamp);
    lua_register(pGlobalLua, "setLink", TLuaInterpreter::setLink);
    lua_register(pGlobalLua, "deselect", TLuaInterpreter::deselect);
    lua_register(pGlobalLua, "insertLink", TLuaInterpreter::insertLink);
    lua_register(pGlobalLua, "echoLink", TLuaInterpreter::echoLink);
    lua_register(pGlobalLua, "echoPopup", TLuaInterpreter::echoPopup);
    lua_register(pGlobalLua, "insertPopup", TLuaInterpreter::insertPopup);
    lua_register(pGlobalLua, "setPopup", TLuaInterpreter::setPopup);
    lua_register(pGlobalLua, "sendATCP", TLuaInterpreter::sendATCP);
    lua_register(pGlobalLua, "hasFocus", TLuaInterpreter::hasFocus);
    lua_register(pGlobalLua, "isPrompt", TLuaInterpreter::isPrompt);
    lua_register(pGlobalLua, "feedTriggers", TLuaInterpreter::feedTriggers);
    lua_register(pGlobalLua, "sendTelnetChannel102", TLuaInterpreter::sendTelnetChannel102);
    lua_register(pGlobalLua, "setRoomWeight", TLuaInterpreter::setRoomWeight);
    lua_register(pGlobalLua, "getRoomWeight", TLuaInterpreter::getRoomWeight);
    lua_register(pGlobalLua, "gotoRoom", TLuaInterpreter::gotoRoom);
    lua_register(pGlobalLua, "getRoomExits", TLuaInterpreter::getRoomExits);
    lua_register(pGlobalLua, "lockRoom", TLuaInterpreter::lockRoom);
    lua_register(pGlobalLua, "createMapper", TLuaInterpreter::createMapper);
    lua_register(pGlobalLua, "createCommandLine", TLuaInterpreter::createCommandLine);
    lua_register(pGlobalLua, "getMainConsoleWidth", TLuaInterpreter::getMainConsoleWidth);
    lua_register(pGlobalLua, "resetProfile", TLuaInterpreter::resetProfile);
    lua_register(pGlobalLua, "printCmdLine", TLuaInterpreter::printCmdLine);
    lua_register(pGlobalLua, "searchRoom", TLuaInterpreter::searchRoom);
    lua_register(pGlobalLua, "clearCmdLine", TLuaInterpreter::clearCmdLine);
    lua_register(pGlobalLua, "getAreaTable", TLuaInterpreter::getAreaTable);
    lua_register(pGlobalLua, "getAreaTableSwap", TLuaInterpreter::getAreaTableSwap);
    lua_register(pGlobalLua, "getAreaRooms", TLuaInterpreter::getAreaRooms);
    lua_register(pGlobalLua, "getPath", TLuaInterpreter::getPath);
    lua_register(pGlobalLua, "centerview", TLuaInterpreter::centerview);
    lua_register(pGlobalLua, "denyCurrentSend", TLuaInterpreter::denyCurrentSend);
    lua_register(pGlobalLua, "tempBeginOfLineTrigger", TLuaInterpreter::tempBeginOfLineTrigger);
    lua_register(pGlobalLua, "tempExactMatchTrigger", TLuaInterpreter::tempExactMatchTrigger);
    lua_register(pGlobalLua, "receiveMSP", TLuaInterpreter::receiveMSP);
    lua_register(pGlobalLua, "purgeMediaCache", TLuaInterpreter::purgeMediaCache);
    lua_register(pGlobalLua, "sendGMCP", TLuaInterpreter::sendGMCP);
    lua_register(pGlobalLua, "roomExists", TLuaInterpreter::roomExists);
    lua_register(pGlobalLua, "addRoom", TLuaInterpreter::addRoom);
    lua_register(pGlobalLua, "setExit", TLuaInterpreter::setExit);
    lua_register(pGlobalLua, "setRoomCoordinates", TLuaInterpreter::setRoomCoordinates);
    lua_register(pGlobalLua, "getRoomCoordinates", TLuaInterpreter::getRoomCoordinates);
    lua_register(pGlobalLua, "createRoomID", TLuaInterpreter::createRoomID);
    lua_register(pGlobalLua, "getRoomArea", TLuaInterpreter::getRoomArea);
    lua_register(pGlobalLua, "setRoomArea", TLuaInterpreter::setRoomArea);
    lua_register(pGlobalLua, "resetRoomArea", TLuaInterpreter::resetRoomArea);
    lua_register(pGlobalLua, "setAreaName", TLuaInterpreter::setAreaName);
    lua_register(pGlobalLua, "roomLocked", TLuaInterpreter::roomLocked);
    lua_register(pGlobalLua, "setCustomEnvColor", TLuaInterpreter::setCustomEnvColor);
    lua_register(pGlobalLua, "getCustomEnvColorTable", TLuaInterpreter::getCustomEnvColorTable);
    lua_register(pGlobalLua, "setRoomEnv", TLuaInterpreter::setRoomEnv);
    lua_register(pGlobalLua, "setRoomName", TLuaInterpreter::setRoomName);
    lua_register(pGlobalLua, "getRoomName", TLuaInterpreter::getRoomName);
    lua_register(pGlobalLua, "setGridMode", TLuaInterpreter::setGridMode);
    lua_register(pGlobalLua, "getGridMode", TLuaInterpreter::getGridMode);
    lua_register(pGlobalLua, "addSpecialExit", TLuaInterpreter::addSpecialExit);
    lua_register(pGlobalLua, "removeSpecialExit", TLuaInterpreter::removeSpecialExit);
    lua_register(pGlobalLua, "getSpecialExits", TLuaInterpreter::getSpecialExits);
    lua_register(pGlobalLua, "getSpecialExitsSwap", TLuaInterpreter::getSpecialExitsSwap);
    lua_register(pGlobalLua, "clearSpecialExits", TLuaInterpreter::clearSpecialExits);
    lua_register(pGlobalLua, "getRoomEnv", TLuaInterpreter::getRoomEnv);
    lua_register(pGlobalLua, "getRoomUserData", TLuaInterpreter::getRoomUserData);
    lua_register(pGlobalLua, "setRoomUserData", TLuaInterpreter::setRoomUserData);
    lua_register(pGlobalLua, "searchRoomUserData", TLuaInterpreter::searchRoomUserData);
    lua_register(pGlobalLua, "getRoomsByPosition", TLuaInterpreter::getRoomsByPosition);
    lua_register(pGlobalLua, "clearRoomUserData", TLuaInterpreter::clearRoomUserData);
    lua_register(pGlobalLua, "clearRoomUserDataItem", TLuaInterpreter::clearRoomUserDataItem);
    lua_register(pGlobalLua, "downloadFile", TLuaInterpreter::downloadFile);
    lua_register(pGlobalLua, "appendCmdLine", TLuaInterpreter::appendCmdLine);
    lua_register(pGlobalLua, "getCmdLine", TLuaInterpreter::getCmdLine);
    lua_register(pGlobalLua, "openUrl", TLuaInterpreter::openUrl);
    lua_register(pGlobalLua, "sendSocket", TLuaInterpreter::sendSocket);
    lua_register(pGlobalLua, "setRoomIDbyHash", TLuaInterpreter::setRoomIDbyHash);
    lua_register(pGlobalLua, "getRoomIDbyHash", TLuaInterpreter::getRoomIDbyHash);
    lua_register(pGlobalLua, "getRoomHashByID", TLuaInterpreter::getRoomHashByID);
    lua_register(pGlobalLua, "addAreaName", TLuaInterpreter::addAreaName);
    lua_register(pGlobalLua, "getRoomAreaName", TLuaInterpreter::getRoomAreaName);
    lua_register(pGlobalLua, "deleteArea", TLuaInterpreter::deleteArea);
    lua_register(pGlobalLua, "deleteRoom", TLuaInterpreter::deleteRoom);
    lua_register(pGlobalLua, "setRoomChar", TLuaInterpreter::setRoomChar);
    lua_register(pGlobalLua, "getRoomChar", TLuaInterpreter::getRoomChar);
    lua_register(pGlobalLua, "registerAnonymousEventHandler", TLuaInterpreter::registerAnonymousEventHandler);
    lua_register(pGlobalLua, "saveMap", TLuaInterpreter::saveMap);
    lua_register(pGlobalLua, "loadMap", TLuaInterpreter::loadMap);
    lua_register(pGlobalLua, "setMainWindowSize", TLuaInterpreter::setMainWindowSize);
    lua_register(pGlobalLua, "setAppStyleSheet", TLuaInterpreter::setAppStyleSheet);
    lua_register(pGlobalLua, "setProfileStyleSheet", TLuaInterpreter::setProfileStyleSheet);
    lua_register(pGlobalLua, "sendIrc", TLuaInterpreter::sendIrc);
    lua_register(pGlobalLua, "getIrcNick", TLuaInterpreter::getIrcNick);
    lua_register(pGlobalLua, "getIrcServer", TLuaInterpreter::getIrcServer);
    lua_register(pGlobalLua, "getIrcChannels", TLuaInterpreter::getIrcChannels);
    lua_register(pGlobalLua, "getIrcConnectedHost", TLuaInterpreter::getIrcConnectedHost);
    lua_register(pGlobalLua, "setIrcNick", TLuaInterpreter::setIrcNick);
    lua_register(pGlobalLua, "setIrcServer", TLuaInterpreter::setIrcServer);
    lua_register(pGlobalLua, "setIrcChannels", TLuaInterpreter::setIrcChannels);
    lua_register(pGlobalLua, "restartIrc", TLuaInterpreter::restartIrc);
    lua_register(pGlobalLua, "connectToServer", TLuaInterpreter::connectToServer);
    lua_register(pGlobalLua, "getRooms", TLuaInterpreter::getRooms);
    lua_register(pGlobalLua, "createMapLabel", TLuaInterpreter::createMapLabel);
    lua_register(pGlobalLua, "deleteMapLabel", TLuaInterpreter::deleteMapLabel);
    lua_register(pGlobalLua, "highlightRoom", TLuaInterpreter::highlightRoom);
    lua_register(pGlobalLua, "unHighlightRoom", TLuaInterpreter::unHighlightRoom);
    lua_register(pGlobalLua, "getMapLabels", TLuaInterpreter::getMapLabels);
    lua_register(pGlobalLua, "getMapLabel", TLuaInterpreter::getMapLabel);
    lua_register(pGlobalLua, "lockExit", TLuaInterpreter::lockExit);
    lua_register(pGlobalLua, "hasExitLock", TLuaInterpreter::hasExitLock);
    lua_register(pGlobalLua, "lockSpecialExit", TLuaInterpreter::lockSpecialExit);
    lua_register(pGlobalLua, "hasSpecialExitLock", TLuaInterpreter::hasSpecialExitLock);
    lua_register(pGlobalLua, "setExitStub", TLuaInterpreter::setExitStub);
    lua_register(pGlobalLua, "connectExitStub", TLuaInterpreter::connectExitStub);
    lua_register(pGlobalLua, "getExitStubs", TLuaInterpreter::getExitStubs);
    lua_register(pGlobalLua, "getExitStubs1", TLuaInterpreter::getExitStubs1);
    lua_register(pGlobalLua, "setModulePriority", TLuaInterpreter::setModulePriority);
    lua_register(pGlobalLua, "getModulePriority", TLuaInterpreter::getModulePriority);
    lua_register(pGlobalLua, "updateMap", TLuaInterpreter::updateMap);
    lua_register(pGlobalLua, "addMapEvent", TLuaInterpreter::addMapEvent);
    lua_register(pGlobalLua, "removeMapEvent", TLuaInterpreter::removeMapEvent);
    lua_register(pGlobalLua, "getMapEvents", TLuaInterpreter::getMapEvents);
    lua_register(pGlobalLua, "addMapMenu", TLuaInterpreter::addMapMenu);
    lua_register(pGlobalLua, "removeMapMenu", TLuaInterpreter::removeMapMenu);
    lua_register(pGlobalLua, "getMapMenus", TLuaInterpreter::getMapMenus);
    lua_register(pGlobalLua, "installPackage", TLuaInterpreter::installPackage);
    lua_register(pGlobalLua, "installModule", TLuaInterpreter::installModule);
    lua_register(pGlobalLua, "uninstallModule", TLuaInterpreter::uninstallModule);
    lua_register(pGlobalLua, "reloadModule", TLuaInterpreter::reloadModule);
    lua_register(pGlobalLua, "enableModuleSync", TLuaInterpreter::enableModuleSync);
    lua_register(pGlobalLua, "disableModuleSync", TLuaInterpreter::disableModuleSync);
    lua_register(pGlobalLua, "getModuleSync", TLuaInterpreter::getModuleSync);
    lua_register(pGlobalLua, "exportAreaImage", TLuaInterpreter::exportAreaImage);
    lua_register(pGlobalLua, "createMapImageLabel", TLuaInterpreter::createMapImageLabel);
    lua_register(pGlobalLua, "setMapZoom", TLuaInterpreter::setMapZoom);
    lua_register(pGlobalLua, "uninstallPackage", TLuaInterpreter::uninstallPackage);
    lua_register(pGlobalLua, "setExitWeight", TLuaInterpreter::setExitWeight);
    lua_register(pGlobalLua, "setDoor", TLuaInterpreter::setDoor);
    lua_register(pGlobalLua, "getDoors", TLuaInterpreter::getDoors);
    lua_register(pGlobalLua, "getExitWeights", TLuaInterpreter::getExitWeights);
    lua_register(pGlobalLua, "addSupportedTelnetOption", TLuaInterpreter::addSupportedTelnetOption);
    lua_register(pGlobalLua, "setMergeTables", TLuaInterpreter::setMergeTables);
    lua_register(pGlobalLua, "getModulePath", TLuaInterpreter::getModulePath);
    lua_register(pGlobalLua, "getAreaExits", TLuaInterpreter::getAreaExits);
    lua_register(pGlobalLua, "auditAreas", TLuaInterpreter::auditAreas);
    lua_register(pGlobalLua, "sendMSDP", TLuaInterpreter::sendMSDP);
    lua_register(pGlobalLua, "handleWindowResizeEvent", TLuaInterpreter::noop);
    lua_register(pGlobalLua, "addCustomLine", TLuaInterpreter::addCustomLine);
    lua_register(pGlobalLua, "removeCustomLine", TLuaInterpreter::removeCustomLine);
    lua_register(pGlobalLua, "getCustomLines", TLuaInterpreter::getCustomLines);
    lua_register(pGlobalLua, "getMudletVersion", TLuaInterpreter::getMudletVersion);
    lua_register(pGlobalLua, "openWebPage", TLuaInterpreter::openWebPage);
    lua_register(pGlobalLua, "getAllRoomEntrances", TLuaInterpreter::getAllRoomEntrances);
    lua_register(pGlobalLua, "getRoomUserDataKeys", TLuaInterpreter::getRoomUserDataKeys);
    lua_register(pGlobalLua, "getAllRoomUserData", TLuaInterpreter::getAllRoomUserData);
    lua_register(pGlobalLua, "searchAreaUserData", TLuaInterpreter::searchAreaUserData);
    lua_register(pGlobalLua, "getMapUserData", TLuaInterpreter::getMapUserData);
    lua_register(pGlobalLua, "getAreaUserData", TLuaInterpreter::getAreaUserData);
    lua_register(pGlobalLua, "setMapUserData", TLuaInterpreter::setMapUserData);
    lua_register(pGlobalLua, "setAreaUserData", TLuaInterpreter::setAreaUserData);
    lua_register(pGlobalLua, "getAllAreaUserData", TLuaInterpreter::getAllAreaUserData);
    lua_register(pGlobalLua, "getAllMapUserData", TLuaInterpreter::getAllMapUserData);
    lua_register(pGlobalLua, "clearAreaUserData", TLuaInterpreter::clearAreaUserData);
    lua_register(pGlobalLua, "clearAreaUserDataItem", TLuaInterpreter::clearAreaUserDataItem);
    lua_register(pGlobalLua, "clearMapUserData", TLuaInterpreter::clearMapUserData);
    lua_register(pGlobalLua, "clearMapUserDataItem", TLuaInterpreter::clearMapUserDataItem);
    lua_register(pGlobalLua, "setDefaultAreaVisible", TLuaInterpreter::setDefaultAreaVisible);
    lua_register(pGlobalLua, "getProfileName", TLuaInterpreter::getProfileName);
    lua_register(pGlobalLua, "getCommandSeparator", TLuaInterpreter::getCommandSeparator);
    lua_register(pGlobalLua, "raiseGlobalEvent", TLuaInterpreter::raiseGlobalEvent);
    lua_register(pGlobalLua, "saveProfile", TLuaInterpreter::saveProfile);
#ifdef QT_TEXTTOSPEECH_LIB
    lua_register(pGlobalLua, "ttsSpeak", TLuaInterpreter::ttsSpeak);
    lua_register(pGlobalLua, "ttsSkip", TLuaInterpreter::ttsSkip);
    lua_register(pGlobalLua, "ttsSetRate", TLuaInterpreter::ttsSetRate);
    lua_register(pGlobalLua, "ttsSetPitch", TLuaInterpreter::ttsSetPitch);
    lua_register(pGlobalLua, "ttsSetVolume", TLuaInterpreter::ttsSetVolume);
    lua_register(pGlobalLua, "ttsGetRate", TLuaInterpreter::ttsGetRate);
    lua_register(pGlobalLua, "ttsGetPitch", TLuaInterpreter::ttsGetPitch);
    lua_register(pGlobalLua, "ttsGetVolume", TLuaInterpreter::ttsGetVolume);
    lua_register(pGlobalLua, "ttsSetVoiceByName", TLuaInterpreter::ttsSetVoiceByName);
    lua_register(pGlobalLua, "ttsSetVoiceByIndex", TLuaInterpreter::ttsSetVoiceByIndex);
    lua_register(pGlobalLua, "ttsGetCurrentVoice", TLuaInterpreter::ttsGetCurrentVoice);
    lua_register(pGlobalLua, "ttsGetVoices", TLuaInterpreter::ttsGetVoices);
    lua_register(pGlobalLua, "ttsQueue", TLuaInterpreter::ttsQueue);
    lua_register(pGlobalLua, "ttsGetQueue", TLuaInterpreter::ttsGetQueue);
    lua_register(pGlobalLua, "ttsPause", TLuaInterpreter::ttsPause);
    lua_register(pGlobalLua, "ttsResume", TLuaInterpreter::ttsResume);
    lua_register(pGlobalLua, "ttsClearQueue", TLuaInterpreter::ttsClearQueue);
    lua_register(pGlobalLua, "ttsGetCurrentLine", TLuaInterpreter::ttsGetCurrentLine);
    lua_register(pGlobalLua, "ttsGetState", TLuaInterpreter::ttsGetState);
#endif // QT_TEXTTOSPEECH_LIB
    lua_register(pGlobalLua, "setServerEncoding", TLuaInterpreter::setServerEncoding);
    lua_register(pGlobalLua, "getServerEncoding", TLuaInterpreter::getServerEncoding);
    lua_register(pGlobalLua, "getServerEncodingsList", TLuaInterpreter::getServerEncodingsList);
    lua_register(pGlobalLua, "alert", TLuaInterpreter::alert);
    lua_register(pGlobalLua, "tempPromptTrigger", TLuaInterpreter::tempPromptTrigger);
    lua_register(pGlobalLua, "permPromptTrigger", TLuaInterpreter::permPromptTrigger);
    lua_register(pGlobalLua, "getColumnCount", TLuaInterpreter::getColumnCount);
    lua_register(pGlobalLua, "getRowCount", TLuaInterpreter::getRowCount);
    lua_register(pGlobalLua, "getOS", TLuaInterpreter::getOS);
    lua_register(pGlobalLua, "getClipboardText", TLuaInterpreter::getClipboardText);
    lua_register(pGlobalLua, "setClipboardText", TLuaInterpreter::setClipboardText);
    lua_register(pGlobalLua, "getAvailableFonts", TLuaInterpreter::getAvailableFonts);
    lua_register(pGlobalLua, "tempAnsiColorTrigger", TLuaInterpreter::tempAnsiColorTrigger);
    lua_register(pGlobalLua, "setDiscordApplicationID", TLuaInterpreter::setDiscordApplicationID);
    lua_register(pGlobalLua, "usingMudletsDiscordID", TLuaInterpreter::usingMudletsDiscordID);
    lua_register(pGlobalLua, "setDiscordState", TLuaInterpreter::setDiscordState);
    lua_register(pGlobalLua, "setDiscordGame", TLuaInterpreter::setDiscordGame);
    lua_register(pGlobalLua, "setDiscordDetail", TLuaInterpreter::setDiscordDetail);
    lua_register(pGlobalLua, "setDiscordLargeIcon", TLuaInterpreter::setDiscordLargeIcon);
    lua_register(pGlobalLua, "setDiscordLargeIconText", TLuaInterpreter::setDiscordLargeIconText);
    lua_register(pGlobalLua, "setDiscordSmallIcon", TLuaInterpreter::setDiscordSmallIcon);
    lua_register(pGlobalLua, "setDiscordSmallIconText", TLuaInterpreter::setDiscordSmallIconText);
    lua_register(pGlobalLua, "getDiscordState", TLuaInterpreter::getDiscordState);
    lua_register(pGlobalLua, "getDiscordDetail", TLuaInterpreter::getDiscordDetail);
    lua_register(pGlobalLua, "getDiscordLargeIcon", TLuaInterpreter::getDiscordLargeIcon);
    lua_register(pGlobalLua, "getDiscordLargeIconText", TLuaInterpreter::getDiscordLargeIconText);
    lua_register(pGlobalLua, "getDiscordSmallIcon", TLuaInterpreter::getDiscordSmallIcon);
    lua_register(pGlobalLua, "getDiscordSmallIconText", TLuaInterpreter::getDiscordSmallIconText);
    lua_register(pGlobalLua, "setDiscordRemainingEndTime", TLuaInterpreter::setDiscordRemainingEndTime);
    lua_register(pGlobalLua, "setDiscordElapsedStartTime", TLuaInterpreter::setDiscordElapsedStartTime);
    lua_register(pGlobalLua, "getDiscordTimeStamps", TLuaInterpreter::getDiscordTimeStamps);
    lua_register(pGlobalLua, "setDiscordParty", TLuaInterpreter::setDiscordParty);
    lua_register(pGlobalLua, "getDiscordParty", TLuaInterpreter::getDiscordParty);
    lua_register(pGlobalLua, "getPlayerRoom", TLuaInterpreter::getPlayerRoom);
    lua_register(pGlobalLua, "getSelection", TLuaInterpreter::getSelection);
    lua_register(pGlobalLua, "getMapSelection", TLuaInterpreter::getMapSelection);
    lua_register(pGlobalLua, "enableClickthrough", TLuaInterpreter::enableClickthrough);
    lua_register(pGlobalLua, "disableClickthrough", TLuaInterpreter::disableClickthrough);
    lua_register(pGlobalLua, "addWordToDictionary", TLuaInterpreter::addWordToDictionary);
    lua_register(pGlobalLua, "removeWordFromDictionary", TLuaInterpreter::removeWordFromDictionary);
    lua_register(pGlobalLua, "spellCheckWord", TLuaInterpreter::spellCheckWord);
    lua_register(pGlobalLua, "spellSuggestWord", TLuaInterpreter::spellSuggestWord);
    lua_register(pGlobalLua, "getDictionaryWordList", TLuaInterpreter::getDictionaryWordList);
    lua_register(pGlobalLua, "getTextFormat", TLuaInterpreter::getTextFormat);
    lua_register(pGlobalLua, "getWindowsCodepage", TLuaInterpreter::getWindowsCodepage);
    lua_register(pGlobalLua, "getHTTP", TLuaInterpreter::getHTTP);
    lua_register(pGlobalLua, "putHTTP", TLuaInterpreter::putHTTP);
    lua_register(pGlobalLua, "postHTTP", TLuaInterpreter::postHTTP);
    lua_register(pGlobalLua, "deleteHTTP", TLuaInterpreter::deleteHTTP);
    lua_register(pGlobalLua, "getConnectionInfo", TLuaInterpreter::getConnectionInfo);
    lua_register(pGlobalLua, "unzipAsync", TLuaInterpreter::unzipAsync);
    lua_register(pGlobalLua, "setMapWindowTitle", TLuaInterpreter::setMapWindowTitle);
    lua_register(pGlobalLua, "getMudletInfo", TLuaInterpreter::getMudletInfo);
    // PLACEMARKER: End of main Lua interpreter functions registration

    QStringList additionalLuaPaths;
    QStringList additionalCPaths;
    const auto appPath{QCoreApplication::applicationDirPath()};
    const auto profilePath{mudlet::getMudletPath(mudlet::profileHomePath, hostName)};

    // Allow for modules or libraries placed in the profile root directory:
    additionalLuaPaths << QStringLiteral("%1/?.lua").arg(profilePath);
    additionalLuaPaths << QStringLiteral("%1/?/init.lua").arg(profilePath);
#if defined(Q_OS_WIN32)
    additionalCPaths << QStringLiteral("%1/?.dll").arg(profilePath);
#else
    additionalCPaths << QStringLiteral("%1/?.so").arg(profilePath);
#endif

#if defined(Q_OS_LINUX)
    // AppInstaller on Linux would like the C search path to also be set to
    // a ./lib sub-directory of the current binary directory:
    additionalCPaths << QStringLiteral("%1/lib/?.so").arg(appPath);
#elif defined(Q_OS_MAC)
    // macOS app bundle would like the search path to also be set to the current
    // binary directory for both modules and binary libraries:
    additionalCPaths << QStringLiteral("%1/?.so").arg(appPath);
    additionalLuaPaths << QStringLiteral("%1/?.lua").arg(appPath);
#elif defined(Q_OS_WIN32) && defined(INCLUDE_MAIN_BUILD_SYSTEM)
    // For CI builds or users/developers using the setup-windows-sdk.ps1 method:
    additionalCPaths << QStringLiteral("C:\\Qt\\Tools\\mingw730_32\\lib\\lua\\5.1\\?.dll");
#endif

    insertNativeSeparatorsFunction(pGlobalLua);

    luaL_dostring(pGlobalLua, QStringLiteral("package.cpath = toNativeSeparators([[%1;]]) .. package.cpath").arg(additionalCPaths.join(QLatin1Char(';'))).toUtf8().constData());
    luaL_dostring(pGlobalLua, QStringLiteral("package.path = toNativeSeparators([[%1;]]) .. package.path").arg(additionalLuaPaths.join(QLatin1Char(';'))).toUtf8().constData());

    /*
     * For uses like this where we try more than one alternative, only include
     * the consequence message for the LAST one (it doesn't make sense to show
     * it in multiple messages about the same functional library with alternative
     * names):
     * Reminder for parameters to loadLuaModule:
     * 1 - Store for the result messaged (required!)
     * 2 - string to pass to the Lua requires(...) function (required, will be
     *     different for variants of the same module/library)
     * 3 - string explaining what gets broken if (one of the alternatives of)
     *     the module/library is not loaded (optional, may be a null/empty
     *     QString if needed for a placeholder for following arguments)
     * 4 - string describing the module (optional, for alternatives may be
     *     different and reflect the luarock name for the particular
     *     alternative, when ommitted or a null/empty QString the second
     *     argument is used instead)
     * 5 - string used in Lua as the identifier for the loaded module/libary,
     *     passed as X in the lua 'X = require("name")' (optional, in which
     *     case nothing is put before the "require" in that usage and the module
     *     assumes whatever Lua name it offers).
     */
    QQueue<QString> modLoadMessageQueue;
    loadLuaModule(modLoadMessageQueue, QLatin1String("lfs"), tr("Probably will not be able to access Mudlet Lua code."), QLatin1String("lfs (Lua File System)"));
    while (!modLoadMessageQueue.isEmpty()) {
        mpHost->postMessage(modLoadMessageQueue.dequeue());
    }

    bool loaded = loadLuaModule(modLoadMessageQueue, QLatin1String("brimworks.zip"), QString(), QStringLiteral("lua-zip"), QStringLiteral("zip"));
    if (!loaded) {
        loadLuaModule(modLoadMessageQueue, QLatin1String("zip"), QString(), QStringLiteral("luazip"));
    }
    while (!modLoadMessageQueue.isEmpty()) {
        mpHost->postMessage(modLoadMessageQueue.dequeue());
    }

    loadLuaModule(modLoadMessageQueue, QLatin1String("rex_pcre"), tr("Some functions may not be available."));
    while (!modLoadMessageQueue.isEmpty()) {
        mpHost->postMessage(modLoadMessageQueue.dequeue());
    }

    loadLuaModule(modLoadMessageQueue, QLatin1String("luasql.sqlite3"), tr("Database support will not be available."), QLatin1String("sqlite3"), QLatin1String("luasql"));
    while (!modLoadMessageQueue.isEmpty()) {
        mpHost->postMessage(modLoadMessageQueue.dequeue());
    }

    loaded = loadLuaModule(modLoadMessageQueue, QLatin1String("lua-utf8"), QString(), QLatin1String("lua-utf8"), QLatin1String("utf8"));
    if (!loaded) {
        loadLuaModule(modLoadMessageQueue, QLatin1String("utf8"), tr("utf8.* Lua functions won't be available."));
    }
    while (!modLoadMessageQueue.isEmpty()) {
        mpHost->postMessage(modLoadMessageQueue.dequeue());
    }

    loadLuaModule(modLoadMessageQueue, QLatin1String("yajl"), tr("yajl.* Lua functions won't be available."), QString(), QLatin1String("yajl"));
    while (!modLoadMessageQueue.isEmpty()) {
        mpHost->postMessage(modLoadMessageQueue.dequeue());
    }

    QString tn = "atcp";
    QStringList args;
    set_lua_table(tn, args);

    tn = "channel102";
    set_lua_table(tn, args);

    lua_pop(pGlobalLua, lua_gettop(pGlobalLua));

    //FIXME make function call in destructor lua_close(L);
}

// No documentation available in wiki - internal function
// Creates a 'mudlet.translations' table with directions
void TLuaInterpreter::setupLanguageData()
{
    lua_State* L = pGlobalLua;

    // 'mudlet' global table
    lua_createtable(L, 0, 1);
    // language-specific table
    lua_createtable(L, 0, 1);
    // language-specific directions table
    lua_createtable(L, 0, 24);

    // this cannot be generated programmatically since lupdate needs the explicit string at extraction time
    lua_pushstring(L, QCoreApplication::translate("directions", "north", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "north");
    lua_pushstring(L, QCoreApplication::translate("directions", "n", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "n");
    lua_pushstring(L, QCoreApplication::translate("directions", "east", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "east");
    lua_pushstring(L, QCoreApplication::translate("directions", "e", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "s");
    lua_pushstring(L, QCoreApplication::translate("directions", "south", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "south");
    lua_pushstring(L, QCoreApplication::translate("directions", "s", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "s");
    lua_pushstring(L, QCoreApplication::translate("directions", "west", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "west");
    lua_pushstring(L, QCoreApplication::translate("directions", "w", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "w");
    lua_pushstring(L, QCoreApplication::translate("directions", "northeast", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "northeast");
    lua_pushstring(L, QCoreApplication::translate("directions", "ne", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "ne");
    lua_pushstring(L, QCoreApplication::translate("directions", "southeast", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "southeast");
    lua_pushstring(L, QCoreApplication::translate("directions", "se", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "se");
    lua_pushstring(L, QCoreApplication::translate("directions", "southwest", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "southwest");
    lua_pushstring(L, QCoreApplication::translate("directions", "sw", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "sw");
    lua_pushstring(L, QCoreApplication::translate("directions", "northwest", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "northwest");
    lua_pushstring(L, QCoreApplication::translate("directions", "nw", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "nw");
    lua_pushstring(L, QCoreApplication::translate("directions", "in", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "in");
    lua_pushstring(L, QCoreApplication::translate("directions", "i", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "i");
    lua_pushstring(L, QCoreApplication::translate("directions", "out", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "out");
    lua_pushstring(L, QCoreApplication::translate("directions", "o", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "o");
    lua_pushstring(L, QCoreApplication::translate("directions", "up", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "up");
    lua_pushstring(L, QCoreApplication::translate("directions", "u", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "u");
    lua_pushstring(L, QCoreApplication::translate("directions", "down", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "down");
    lua_pushstring(L, QCoreApplication::translate("directions", "d", "Entering this direction will move the player in the game").toUtf8().constData());
    lua_setfield(L, -2, "d");

    // finalize language-specific directions table
    lua_setfield(L, -2, mudlet::self()->getInterfaceLanguage().toUtf8().constData());

    lua_pushstring(L, mudlet::self()->getInterfaceLanguage().toUtf8().constData());
    lua_setfield(L, -2, "interfacelanguage");

    lua_setfield(L, -2, "translations");
    lua_setglobal(L, "mudlet");
    lua_pop(L, lua_gettop(L));
}

// No documentation available in wiki - internal function
// Initialised a slimmed-down Lua state just to run the indenter in a separate sandbox.
// The indenter by default pollutes the global environment with some utility functions
// and we don't want to tie ourselves to it by exposing them for scripting.
void TLuaInterpreter::initIndenterGlobals()
{
    Q_ASSERT_X(!pIndenterState, "TLuaInterpreter::initIndenterGlobals()", "Indenter state is already initialized - re-initializing it is very expensive!");

    pIndenterState.reset(newstate());
    storeHostInLua(pIndenterState.get(), mpHost);

    luaL_openlibs(pIndenterState.get());

    lua_pushstring(pIndenterState.get(), "SESSION");
    lua_pushnumber(pIndenterState.get(), mHostID);
    lua_settable(pIndenterState.get(), LUA_GLOBALSINDEX);

    lua_pushstring(pIndenterState.get(), "SCRIPT_NAME");
    lua_pushstring(pIndenterState.get(), "Lua Indenter Interpreter");
    lua_settable(pIndenterState.get(), LUA_GLOBALSINDEX);

    lua_pushstring(pIndenterState.get(), "SCRIPT_ID");
    lua_pushnumber(pIndenterState.get(), -2); // ID 2 is used to indicate that this is the indenter Lua interpreter
    lua_settable(pIndenterState.get(), LUA_GLOBALSINDEX);
    lua_register(pIndenterState.get(), "echo", TLuaInterpreter::Echo);
    lua_register(pIndenterState.get(), "tempTimer", TLuaInterpreter::tempTimer);
    lua_register(pIndenterState.get(), "send", TLuaInterpreter::sendRaw);
    lua_register(pIndenterState.get(), "debugc", TLuaInterpreter::debug);
    // PLACEMARKER: End of indenter Lua interpreter functions registration

    /*
     * Additional paths for the lua/C package search paths - '?' (and any '.'s,
     * in the file name, apart from the one before the filename extension) are
     * handled specially! See: https://stackoverflow.com/q/31904392/4805858 :
     */
    QStringList additionalLuaPaths;
    QStringList additionalCPaths;
    const QString appPath{QCoreApplication::applicationDirPath()};

#if defined(Q_OS_MACOS)
    // macOS app bundle would like the search path for the binary modules to
    // also be set to the current binary directory:
    additionalCPaths << QStringLiteral("%1/?.so").arg(appPath);
#elif defined (Q_OS_LINUX)
    // AppInstaller on Linux would like the search path for the binary modules
    // to also be set to a lib sub-directory of the application directory:
    additionalCPaths << QStringLiteral("%1/lib/?.so").arg(appPath);
#endif

    insertNativeSeparatorsFunction(pIndenterState.get());

    // 1 installed *nix case - probably not applicable to Windows
    //     "LUA_DEFAULT_PATH/?.lua" (if defined and not empty)
    if (!QStringLiteral(LUA_DEFAULT_PATH).isEmpty()) {
        additionalLuaPaths << QStringLiteral(LUA_DEFAULT_PATH "/?.lua");
    }
    // 2 AppImage (directory of executable) - not needed for Wndows:
    //     "<applicationDirectory>/?.lua"
#if ! defined (Q_OS_WIN32)
    additionalLuaPaths << QStringLiteral("%1/?.lua").arg(appPath);
#endif
    // 3 QMake shadow builds without CONFIG containing "debug_and_release" but
    //    with "debug_and_release_target" (default on most OS but NOT Windows):
    //     "<applicationDirectory>/../3rdparty/?.lua"
    additionalLuaPaths << QStringLiteral("%1/../3rdparty/?.lua").arg(appPath);
    // 4 QMake shadow builds with CONFIG containing "debug_and_release" AND
    //   "debug_and_release_target" (usually Windows):
    //     "<applicationDirectory>/../../3rdparty/?.lua"
    additionalLuaPaths << QStringLiteral("%1/../../3rdparty/?.lua").arg(appPath);
    // 5 CMake shadow builds
    //    "<applicationDirectory>/../../mudlet/3rdparty/?.lua"
    additionalLuaPaths << QStringLiteral("%1/../../mudlet/3rdparty/?.lua").arg(appPath);

    int error = luaL_dostring(pIndenterState.get(), QStringLiteral("package.path = toNativeSeparators([[%1;]] .. package.path)")
                              .arg(additionalLuaPaths.join(QLatin1Char(';'))).toUtf8().constData());
    if (!error && !additionalCPaths.isEmpty()) {
        error = luaL_dostring(pIndenterState.get(), QStringLiteral("package.cpath = toNativeSeparators([[%1;]] .. package.cpath)")
                              .arg(additionalCPaths.join(QLatin1Char(';'))).toUtf8().constData());
    }

    // clang-format off
    if (!error) {
        error = luaL_dostring(pIndenterState.get(), R"LUA(
  require('lcf.workshop.base')
  get_ast = request('!.lua.code.get_ast')
  get_formatted_code = request('!.lua.code.ast_as_code')
)LUA");
// clang-format on
    }
    if (error) {
        QString e = tr("No error message available from Lua.");
        if (lua_isstring(pIndenterState.get(), -1)) {
            e = tr("Lua error: %1.").arg(QString::fromUtf8(lua_tostring(pIndenterState.get(), -1)));
        }
        QString msg = tr("[ ERROR ] - Cannot load code formatter, indenting functionality won't be available.\n");
        msg.append(e);
        mpHost->postMessage(msg);
    }

    lua_pop(pIndenterState.get(), lua_gettop(pIndenterState.get()));
}

// No documentation available in wiki - internal function called AFTER
// initLuaGlobals() {it depends on that to load up some Lua libraries, including
// the LFS "Lua File System" one first}:
void TLuaInterpreter::loadGlobal()
{
#if defined(Q_OS_WIN32)
    loadUtf8Filenames();
#endif

    setupLanguageData();

    const QString executablePath{QCoreApplication::applicationDirPath()};
    // Initialise the list of path and file names so that
    // getMudletLuaDefaultPaths() can report them later if asked:
    mPossiblePaths = QStringList{
#if defined(Q_OS_MACOS)
        // Load relatively to MacOS inside Resources when we're in a .app
        // bundle, as mudlet-lua always gets copied in by the build script into
        // the bundle for the Mac installer build:
        QStringLiteral("%1/../Resources/mudlet-lua/lua/LuaGlobal.lua").arg(executablePath),
#endif

        // For the installer we put the lua files under the executable's
        // location. This is the case for the Windows install:
        QDir::toNativeSeparators(QStringLiteral("%1/mudlet-lua/lua/LuaGlobal.lua").arg(executablePath)),

        // Although a no-op for an in source build an additional "../src/"
        // allows location of lua code when object code is in a directory
        // alongside the src directory as occurs using Qt Creator "Shadow
        // Builds":
        QDir::toNativeSeparators(QStringLiteral("%1/../src/mudlet-lua/lua/LuaGlobal.lua").arg(executablePath)),

        // Windows builds (or others where the qmake project file has CONFIG
        // containing debug_and_release AND debug_and_release_target options)
        // may be an additional sub-directory down:
        QDir::toNativeSeparators(QStringLiteral("%1/../../src/mudlet-lua/lua/LuaGlobal.lua").arg(executablePath)),

        // CMake builds done from Qt Creator tend to make their build directory
        // be in a "out-of-source" (the more common name for what Qt calls
        // "Shadow Builds") on the same level as the top level CMakeList.txt
        // project file - which is one level up compared to the QMake case.
        // and in a "src" subdirectory (to match the relative source file
        // location to that top-level project file) of the main project
        // "mudlet" directory:
        QDir::toNativeSeparators(QStringLiteral("%1/../../mudlet/src/mudlet-lua/lua/LuaGlobal.lua").arg(executablePath))
    };

    // Although it is relatively easy to detect whether something is #define d
    // it is not so easy to detect what it contains at the preprocessor stage,
    // so leave checking for it's contents to run-time - this one is the one
    // for Linux/FreeBSD where the read-only shared Lua files go into the
    // /usr/share part of the file-system:
    if (!QStringLiteral(LUA_DEFAULT_PATH).isEmpty()) {
        mPossiblePaths <<  QDir::toNativeSeparators(QStringLiteral(LUA_DEFAULT_PATH "/LuaGlobal.lua"));
    };
    QStringList failedMessages{};

    // uncomment the following to enable some debugging texts in the LuaGlobal.lua script:
    // luaL_dostring(pGlobalLua, QStringLiteral("debugLoading = true").toUtf8().constData());

#if defined(Q_OS_WIN32)
    // Needed to enable permissions checks on NTFS file systems - normally
    // turned off for performance reasons:
    extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

    int error;
    for (const auto& pathFileName : qAsConst(mPossiblePaths)) {
        if (!(QFileInfo::exists(pathFileName))) {
            failedMessages << tr("%1 (doesn't exist)", "This file doesn't exist").arg(pathFileName);
            continue;
        }

        if (!(QFileInfo(pathFileName).isFile())) {
            failedMessages << tr("%1 (isn't a file or symlink to a file)").arg(pathFileName);
            continue;
        }

#if defined(Q_OS_WIN32)
        // Turn on permission checking on NTFS file systems
        qt_ntfs_permission_lookup++;
#endif
        if (!(QFileInfo(pathFileName).isReadable())) {
            failedMessages << tr("%1 (isn't a readable file or symlink to a readable file)").arg(pathFileName);
            continue;
        }
#if defined(Q_OS_WIN32)
        // Turn off permission checking
        qt_ntfs_permission_lookup--;
#endif

        // Leave a global variable set to the path so we can use it to find the
        // other files around it - the script will convert the directory
        // separators as necessary:
        Q_ASSERT_X(!pathFileName.isEmpty(), "TLuaInterpreter::loadGlobal()", "trying to call QFileInfo(path).absolutePath() when path is empty");
        luaL_dostring(pGlobalLua, QStringLiteral("luaGlobalPath = \"%1\"").arg(QFileInfo(pathFileName).absolutePath()).toUtf8().constData());

        // load via Qt so UTF8 paths work on Windows - Lua can't handle it
        auto luaGlobal = readScriptFile(pathFileName);

        if (luaGlobal.isEmpty()) {
            failedMessages << tr("%1 (couldn't read file)", "This file could not be read for some reason (for example, no permission)").arg(pathFileName);
            continue;
        }

        error = luaL_dostring(pGlobalLua, luaGlobal.toUtf8().constData());
        if (error == 0) {
            mpHost->postMessage(tr("[  OK  ]  - Mudlet-lua API & Geyser Layout manager loaded."));
            return;
        } else {
            qWarning() << "TLuaInterpreter::loadGlobal() loading " << pathFileName << " failed: " << lua_tostring(pGlobalLua, -1);
            failedMessages << QStringLiteral("%1 (%2)").arg(pathFileName, lua_tostring(pGlobalLua, -1));
        }
    }

    mpHost->postMessage(tr("[ ERROR ] - Couldn't find, load and successfully run LuaGlobal.lua - your Mudlet is broken!\nTried these locations:\n%1").arg(failedMessages.join(QChar::LineFeed)));
}

// No documentation available in wiki - internal function
// Returns contents of the file or empty string if it couldn't be read
QString TLuaInterpreter::readScriptFile(const QString& path) const
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return QString();
    }

    QTextStream in(&file);
    QString text = in.readAll();
    file.close();

    return text;
}

#if defined(Q_OS_WIN32)
// No documentation available in wiki - internal function
// loads utf8_filenames from the resource system directly so it is not affected by
// non-ASCII characters that might be present in the users filesystem
void TLuaInterpreter::loadUtf8Filenames()
{
    auto path = QStringLiteral(":/mudlet-lua/lua/utf8_filenames.lua");
    auto text = readScriptFile(path);
    if (text.isEmpty()) {
        qWarning() << "TLuaInterpreter::loadUtf8Filenames() ERROR: couldn't read file: " << path;
        return;
    }

    if (mpHost->getLuaInterpreter()->compileAndExecuteScript(text)) {
        qDebug() << "TLuaInterpreter::loadUtf8Filenames() - patched Lua IO functions to work on Windows with UTF8";
    } else {
        qWarning() << "TLuaInterpreter::loadUtf8Filenames() ERROR: there was an error running the script";
    }
}
#endif

// No documentation available in wiki - internal function
QPair<int, QString> TLuaInterpreter::createPermScript(const QString& name, const QString& parent, const QString& luaCode)
{
    TScript* pS;
    if (parent.isEmpty()) {
        pS = new TScript(QStringLiteral("newPermScriptWithoutAnId"), mpHost);
    } else {
        // FIXME: There can be more than one script with the same name - we will
        // use only the FIRST one for now, but we really ought to enhance the
        // API to handle more than one potential parent with the same name:
        auto ids = mpHost->getScriptUnit()->findScriptId(parent);
        auto pParentScript = mpHost->getScriptUnit()->getScript(ids.value(0, -1));
        if (!pParentScript) {
            return qMakePair(-1, QStringLiteral("parent \"%1\" not found").arg(parent)); //parent not found
        }
        pS = new TScript(pParentScript, mpHost);
    }
    pS->setIsFolder((luaCode.isEmpty()));
    pS->setName(name);
    // This will lead to the generation of the id number:
    mpHost->getScriptUnit()->registerScript(pS);
    if (!pS->setScript(luaCode)) {
        QString errMsg = pS->getError();
        delete pS;
        return qMakePair(-1, QStringLiteral("unable to compile \"%1\", reason: %2").arg(luaCode, errMsg));
    }

    int id = pS->getID();
    pS->setIsActive(false);
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return qMakePair(id, QString());
}

// No documentation available in wiki - internal function
QPair<int, QString> TLuaInterpreter::setScriptCode(QString& name, const QString& luaCode, int pos)
{
    if (name.isEmpty()) {
        return qMakePair(-1, QStringLiteral("cannot have an empty string as name"));
    }

    auto ids = mpHost->getScriptUnit()->findScriptId(name);
    TScript* pS = mpHost->getScriptUnit()->getScript(ids.value(pos, -1));
    if (!pS) {
        return qMakePair(-1, QStringLiteral("script \"%1\" at position \"%2\" not found").arg(name).arg(++pos)); //script not found
    }
    auto oldCode = pS->getScript();
    if (!pS->setScript(luaCode)) {
        QString errMsg = pS->getError();
        pS->setScript(oldCode);
        return qMakePair(-1, QStringLiteral("unable to compile \"%1\" at position \"%2\", reason: %3").arg(luaCode).arg(++pos).arg(errMsg));
    }
    pS->setScript(luaCode);
    int id = pS->getID();
    mpHost->mpEditorDialog->writeScript(id);
    return qMakePair(id, QString());
}

// No documentation available in wiki - internal function
QPair<int, QString> TLuaInterpreter::startPermTimer(const QString& name, const QString& parent, double timeout, const QString& function)
{
    QTime time = QTime(0, 0, 0, 0).addMSecs(qRound(timeout * 1000));
    TTimer* pT;
    if (parent.isEmpty()) {
        pT = new TTimer(QStringLiteral("newPermTimerWithoutAnId"), time, mpHost);
    } else {
        // FIXME: There can be more than one timer with the same name - we will
        // use only the FIRST one for now, but we really ought to enhance the
        // API to handle more than one potential parent with the same name:
        auto pParentTimer = mpHost->getTimerUnit()->findFirstTimer(parent);
        if (!pParentTimer) {
            return qMakePair(-1, QStringLiteral("parent \"%1\" not found").arg(parent)); //parent not found
        }
        pT = new TTimer(pParentTimer, mpHost);
    }
    pT->setTime(time);
    pT->setIsFolder((timeout == 0 && function.isEmpty()));
    pT->setTemporary(false);
    // The name should be set after isTempTimer, as that is faster.
    // Also for perminent timers it is easier to debug if it is set before
    // registration:
    pT->setName(name);
    // This will lead to the generation of the id number:
    mpHost->getTimerUnit()->registerTimer(pT);
    if (!pT->setScript(function)) {
        QString errMsg = pT->getError();
        // Apparently this will call the TTimer::unregisterTimer(...) method:
        delete pT;
        return qMakePair(-1, QStringLiteral("unable to compile \"%1\", reason: %2").arg(function, errMsg));
    }

    int id = pT->getID();
    pT->setIsActive(false);
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return qMakePair(id, QString());
}

// No documentation available in wiki - internal function
QPair<int, QString> TLuaInterpreter::startTempTimer(double timeout, const QString& function, const bool repeating)
{
    QTime time = QTime(0,0,0,0).addMSecs(qRound(timeout * 1000));
    auto* pT = new TTimer(QStringLiteral("newTempTimerWithoutAnId"), time, mpHost, repeating);
    pT->setTime(time);
    pT->setIsFolder(false);
    pT->setTemporary(true);
    // This call leads to the allocation of an ID number - and its use as the
    // name for temporary timers:
    mpHost->getTimerUnit()->registerTimer(pT);
    if (!pT->setScript(function)) {
        QString errMsg = pT->getError();
        // Apparently this will call the TTimer::unregisterTimer(...) method:
        delete pT;
        return qMakePair(-1, QStringLiteral("unable to compile \"%1\", reason: %2").arg(function, errMsg));
    }

    int id = pT->getID();
    pT->setIsActive(true);
    pT->enableTimer(id);
    return qMakePair(id, QString());
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startPermAlias(const QString& name, const QString& parent, const QString& regex, const QString& function)
{
    TAlias* pT;

    if (parent.isEmpty()) {
        pT = new TAlias("a", mpHost);
    } else {
        TAlias* pP = mpHost->getAliasUnit()->findFirstAlias(parent);
        if (!pP) {
            return -1; //parent not found
        }
        pT = new TAlias(pP, mpHost);
    }
    pT->setRegexCode(regex);
    pT->setIsFolder((regex.isEmpty() && function.isEmpty()));
    pT->setIsActive(true);
    pT->setTemporary(false);
    pT->registerAlias();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(name);
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempAlias(const QString& regex, const QString& function)
{
    TAlias* pT;
    pT = new TAlias("a", mpHost);
    pT->setRegexCode(regex);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerAlias();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(QString::number(id));
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startPermKey(QString& name, QString& parent, int& keycode, int& modifier, QString& function)
{
    TKey* pT;

    if (parent.isEmpty()) {
        pT = new TKey("a", mpHost); // The use of "a" seems a bit arbitary...!
    } else {
        TKey* pP = mpHost->getKeyUnit()->findFirstKey(parent);
        if (!pP) {
            return -1; //parent not found
        }
        pT = new TKey(pP, mpHost);
    }
    pT->setKeyCode(keycode);
    pT->setKeyModifiers(modifier);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(false);
    pT->registerKey();
    // CHECK: The lua code in function could fail to compile - but there is no feedback here to the caller.
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(name);
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempKey(int& modifier, int& keycode, QString& function)
{
    TKey* pT;
    pT = new TKey("a", mpHost);
    pT->setKeyCode(keycode);
    pT->setKeyModifiers(modifier);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerKey();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(QString::number(id));
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempExactMatchTrigger(const QString& regex, const QString& function, int expiryCount)
{
    TTrigger* pT;
    QStringList sList;
    sList << regex;
    QList<int> propertyList;
    propertyList << REGEX_EXACT_MATCH;
    pT = new TTrigger("a", sList, propertyList, false, mpHost);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(QString::number(id));
    pT->setExpiryCount(expiryCount);
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempBeginOfLineTrigger(const QString& regex, const QString& function, int expiryCount)
{
    TTrigger* pT;
    QStringList sList;
    sList << regex;
    QList<int> propertyList;
    propertyList << REGEX_BEGIN_OF_LINE_SUBSTRING;
    pT = new TTrigger("a", sList, propertyList, false, mpHost);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(QString::number(id));
    pT->setExpiryCount(expiryCount);
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempTrigger(const QString& regex, const QString& function, int expiryCount)
{
    TTrigger* pT;
    QStringList sList;
    sList << regex;
    QList<int> propertyList;
    propertyList << REGEX_SUBSTRING; // substring trigger is default
    pT = new TTrigger("a", sList, propertyList, false, mpHost);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(QString::number(id));
    pT->setExpiryCount(expiryCount);
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempPromptTrigger(const QString& function, int expiryCount)
{
    TTrigger* pT;
    QStringList sList = {QString()};
    QList<int> propertyList = {REGEX_PROMPT};
    pT = new TTrigger("a", sList, propertyList, false, mpHost);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(QString::number(id));
    pT->setExpiryCount(expiryCount);
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempLineTrigger(int from, int howmany, const QString& function, int expiryCount)
{
    TTrigger* pT;
    //    QStringList sList;
    //    QList<int> propertyList;
    //    propertyList << REGEX_SUBSTRING;// substring trigger is default
    //    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT = new TTrigger(nullptr, mpHost);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->setIsLineTrigger(true);
    pT->setStartOfLineDelta(from);
    pT->setLineDelta(howmany);
    pT->registerTrigger();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(QString::number(id));
    pT->setExpiryCount(expiryCount);
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempColorTrigger(int fg, int bg, const QString& function, int expiryCount)
{
    TTrigger* pT;
    //    QStringList sList;
    //    QList<int> propertyList;
    //    propertyList << REGEX_SUBSTRING;// substring trigger is default
    //    pT = new TTrigger("a", sList, propertyList, false, mpHost );
    pT = new TTrigger(nullptr, mpHost);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->setupTmpColorTrigger(fg, bg);

    pT->registerTrigger();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(QString::number(id));
    pT->setExpiryCount(expiryCount);
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempRegexTrigger(const QString& regex, const QString& function, int expiryCount)
{
    TTrigger* pT;
    QStringList sList;
    sList << regex;

    QList<int> propertyList;
    propertyList << REGEX_PERL; // substring trigger is default
    pT = new TTrigger("a", sList, propertyList, false, mpHost);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(QString::number(id));
    pT->setExpiryCount(expiryCount);
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startPermRegexTrigger(const QString& name, const QString& parent, QStringList& regexList, const QString& function)
{
    TTrigger* pT;
    QList<int> propertyList;
    for (int i = 0; i < regexList.size(); i++) {
        propertyList << REGEX_PERL;
    }
    if (parent.isEmpty()) {
        pT = new TTrigger("a", regexList, propertyList, (regexList.size() > 1), mpHost);
    } else {
        TTrigger* pP = mpHost->getTriggerUnit()->findTrigger(parent);
        if (!pP) {
            return -1; //parent not found
        }
        pT = new TTrigger(pP, mpHost);
        pT->setRegexCodeList(regexList, propertyList);
    }
    pT->setIsFolder(regexList.empty());
    pT->setIsActive(true);
    pT->setTemporary(false);
    pT->registerTrigger();
    pT->setScript(function);
    //pT->setName( name );
    int id = pT->getID();
    pT->setName(name);
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    //return 1;
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startPermBeginOfLineStringTrigger(const QString& name, const QString& parent, QStringList& regexList, const QString& function)
{
    TTrigger* pT;
    QList<int> propertyList;
    for (int i = 0; i < regexList.size(); i++) {
        propertyList << REGEX_BEGIN_OF_LINE_SUBSTRING;
    }
    if (parent.isEmpty()) {
        pT = new TTrigger("a", regexList, propertyList, (regexList.size() > 1), mpHost);
    } else {
        TTrigger* pP = mpHost->getTriggerUnit()->findTrigger(parent);
        if (!pP) {
            return -1; //parent not found
        }
        pT = new TTrigger(pP, mpHost);
        pT->setRegexCodeList(regexList, propertyList);
    }
    pT->setIsFolder(regexList.empty());
    pT->setIsActive(true);
    pT->setTemporary(false);
    pT->registerTrigger();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(name);
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    //return 1;
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startPermSubstringTrigger(const QString& name, const QString& parent, const QStringList& regexList, const QString& function)
{
    TTrigger* pT;
    QList<int> propertyList;
    for (int i = 0; i < regexList.size(); i++) {
        propertyList << REGEX_SUBSTRING;
    }
    if (parent.isEmpty()) {
        pT = new TTrigger("a", regexList, propertyList, (regexList.size() > 1), mpHost);
    } else {
        TTrigger* pP = mpHost->getTriggerUnit()->findTrigger(parent);
        if (!pP) {
            return -1; //parent not found
        }
        pT = new TTrigger(pP, mpHost);
        pT->setRegexCodeList(regexList, propertyList);
    }
    pT->setIsFolder(regexList.empty());
    pT->setIsActive(true);
    pT->setTemporary(false);
    pT->registerTrigger();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(name);
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    //return 1;
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startPermPromptTrigger(const QString& name, const QString& parent, const QString& function)
{
    TTrigger* pT;
    QList<int> propertyList = {REGEX_PROMPT};
    QStringList regexList = {QString()};

    if (parent.isEmpty()) {
        pT = new TTrigger("a", regexList, propertyList, false, mpHost);
    } else {
        TTrigger* pP = mpHost->getTriggerUnit()->findTrigger(parent);
        if (!pP) {
            return -1; //parent not found
        }
        pT = new TTrigger(pP, mpHost);
        pT->setRegexCodeList(regexList, propertyList);
    }
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(false);
    pT->registerTrigger();
    pT->setScript(function);
    int id = pT->getID();
    pT->setName(name);
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return id;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#alert
int TLuaInterpreter::alert(lua_State* L)
{
    double luaAlertDuration = 0.0;

    if (lua_gettop(L) > 0) {
        if (!lua_isnumber(L, 1)) {
            lua_pushfstring(L, "alert: bad argument #1 type (alert duration in seconds as number expected, got %s!)", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            luaAlertDuration = lua_tonumber(L, 1);

            if (luaAlertDuration < 0.000) {
                lua_pushstring(L, "alert: duration, in seconds, is optional but if given must be zero or greater.");
                return lua_error(L);
            }
        }
    }

    // QApplication::alert expects milliseconds, not seconds
    QApplication::alert(mudlet::self(), qRound(luaAlertDuration * 1000.0));

    return 0;
}

static int host_key = 0;

static void storeHostInLua(lua_State* L, Host* h)
{
    lua_pushlightuserdata(L, &host_key); // 1 - push unique key
    lua_pushlightuserdata(L, h);         // 2 - push host ptr
    lua_rawset(L, LUA_REGISTRYINDEX);    // 0 - register[key] = host
}

Host& getHostFromLua(lua_State* L)
{
    lua_pushlightuserdata(L, &host_key);    // 1 - push unique key
    lua_rawget(L, LUA_REGISTRYINDEX);       // 1 - pop key, push host ptr
    auto* h = static_cast<Host*>(lua_touserdata(L, -1)); // 1 - get host ptr
    lua_pop(L, 1);                          // 0 - pop host ptr
    assert(h);
    return *h;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getColumnCount
int TLuaInterpreter::getColumnCount(lua_State* L)
{
    QString windowName;

    if (!lua_gettop(L)) {
        windowName = QStringLiteral("main");
    } else if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "getColumnCount: bad argument #1 type (window name as string expected, got %s)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        windowName = QString::fromUtf8(lua_tostring(L, 1));
    }

    int columns;
    Host* pHost = &getHostFromLua(L);

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        columns = pHost->mpConsole->mUpperPane->getColumnCount();
    } else {
        columns = mudlet::self()->getColumnCount(pHost, windowName);
    }

    if (columns < 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "window \"%s\" not found", windowName.toUtf8().constData());
        return 2;
    }

    lua_pushnumber(L, columns);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRowCount
int TLuaInterpreter::getRowCount(lua_State* L)
{
    QString windowName;

    if (!lua_gettop(L)) {
        windowName = QStringLiteral("main");
    } else if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "getRowCount: bad argument #1 type (window name as string expected, got %s)", luaL_typename(L, 1));
        lua_error(L);
        return 1;
    } else {
        windowName = QString::fromUtf8(lua_tostring(L, 1));
    }

    int rows;
    Host* pHost = &getHostFromLua(L);

    if (windowName.isEmpty() || windowName.compare(QStringLiteral("main"), Qt::CaseSensitive) == 0) {
        rows = pHost->mpConsole->mUpperPane->getRowCount();
    } else {
        rows = mudlet::self()->getRowCount(pHost, windowName);
    }

    if (rows < 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "window \"%s\" not found", windowName.toUtf8().constData());
        return 2;
    }

    lua_pushnumber(L, rows);
    return 1;
}

// No documentation available in wiki - internal function
// Used to unref lua objects in the registry to avoid memory leaks
// i.e. Unrefing tables passed into TLabel's event parameters.
void TLuaInterpreter::freeLuaRegistryIndex(int index) {
    luaL_unref(pGlobalLua, LUA_REGISTRYINDEX, index);
}

// Internal function - Looks for argument types in an 'event' that have stored
// data in the lua registry, and frees this data.
void TLuaInterpreter::freeAllInLuaRegistry(TEvent event)
{
    for (int i = 0; i < event.mArgumentList.size(); i++) {
        if (event.mArgumentTypeList.at(i) == ARGUMENT_TYPE_TABLE || event.mArgumentTypeList.at(i) == ARGUMENT_TYPE_FUNCTION)
        {
             freeLuaRegistryIndex(event.mArgumentList.at(i).toInt());
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapSelection
int TLuaInterpreter::getMapSelection(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    if (!pHost || !pHost->mpMap || !pHost->mpMap->mpMapper || !pHost->mpMap->mpMapper->mp2dMap) {
        lua_pushnil(L);
        lua_pushstring(L, "getMapSelection: no map present or loaded!");
        return 2;
    }

    lua_newtable(L);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QList<int> selectionRoomsList{pHost->mpMap->mpMapper->mp2dMap->mMultiSelectionSet.begin(),
                                  pHost->mpMap->mpMapper->mp2dMap->mMultiSelectionSet.end()};
#else
    QList<int> selectionRoomsList{pHost->mpMap->mpMapper->mp2dMap->mMultiSelectionSet.toList()};
#endif
    if (!selectionRoomsList.isEmpty()) {
        if (selectionRoomsList.count() > 1) {
            std::sort(selectionRoomsList.begin(), selectionRoomsList.end());
        }

        lua_pushstring(L, "center");
        lua_pushnumber(L, pHost->mpMap->mpMapper->mp2dMap->getCenterSelectedRoomId());
        lua_settable(L, -3);

        lua_pushstring(L, "rooms");
        lua_newtable(L);
        for (int i = 0, total = selectionRoomsList.size(); i < total; ++i) {
            lua_pushnumber(L, i + 1);
            lua_pushnumber(L, selectionRoomsList.at(i));
            lua_settable(L, -3);
        }

        lua_settable(L, -3);

    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableClickthrough
int TLuaInterpreter::enableClickthrough(lua_State* L)
{
    int n = lua_gettop(L);
    QString windowName;
    if (n == 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "enableClickthrough: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, 1));
        }
    }

    Host& host = getHostFromLua(L);

    mudlet::self()->setClickthrough(&host, windowName, true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableClickthrough
int TLuaInterpreter::disableClickthrough(lua_State* L)
{
    int n = lua_gettop(L);
    QString windowName;
    if (n == 1) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L, "disableClickthrough: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
            lua_error(L);
            return 1;
        } else {
            windowName = QString::fromUtf8(lua_tostring(L, 1));
        }
    }

    Host& host = getHostFromLua(L);

    mudlet::self()->setClickthrough(&host, windowName, false);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addWordToDictionary
int TLuaInterpreter::addWordToDictionary(lua_State* L)
{
    Host& host = getHostFromLua(L);
    bool hasUserDictionary = false;
    bool hasSharedDictionary = false;
    host.getUserDictionaryOptions(hasUserDictionary, hasSharedDictionary);
    if (!hasUserDictionary) {
        lua_pushnil(L);
        lua_pushstring(L, "no user dictionary enabled in the preferences for this profile");
        return 2;
    }

    QString text;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "addWordToDictionary: bad argument #1 type (word as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        text = QString::fromUtf8(lua_tostring(L, 1));
    }

    QPair<bool, QString> result = host.mpConsole->addWordToSet(text);
    if (!result.first){
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else {
        lua_pushboolean(L, true);
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeWordFromDictionary
int TLuaInterpreter::removeWordFromDictionary(lua_State* L)
{
    Host& host = getHostFromLua(L);
    bool hasUserDictionary = false;
    bool hasSharedDictionary = false;
    host.getUserDictionaryOptions(hasUserDictionary, hasSharedDictionary);
    if (!hasUserDictionary) {
        lua_pushnil(L);
        lua_pushstring(L, "no user dictionary enabled in the preferences for this profile");
        return 2;
    }

    QString text;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "removeWordFromDictionary: bad argument #1 type (word as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        text = QString::fromUtf8(lua_tostring(L, 1));
    }

    QPair<bool, QString> result = host.mpConsole->removeWordFromSet(text);
    if (!result.first){
        lua_pushnil(L);
        lua_pushstring(L, result.second.toUtf8().constData());
        return 2;
    } else {
        lua_pushboolean(L, true);
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#spellCheckWord
int TLuaInterpreter::spellCheckWord(lua_State* L)
{
    Host& host = getHostFromLua(L);
    bool hasUserDictionary = false;
    bool hasSharedDictionary = false;
    host.getUserDictionaryOptions(hasUserDictionary, hasSharedDictionary);
    QString text;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "spellCheckWord: bad argument #1 type (word as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        text = QString::fromUtf8(lua_tostring(L, 1));
    }

    bool useUserDictionary = false;
    if (lua_gettop(L) > 1) {
        if (!lua_isboolean(L, 2)) {
            lua_pushfstring(L, "spellSuggestWord: bad argument #2 type (check profile dictionary as boolean is optional {use 'false' or omit to check against system dictionary}, got %s!)", luaL_typename(L, 2));
            return lua_error(L);
        } else {
            useUserDictionary = lua_toboolean(L, 2);
            if (useUserDictionary && !hasUserDictionary) {
                lua_pushnil(L);
                lua_pushstring(L, "no user dictionary enabled in the preferences for this profile");
                return 2;
            }
        }
    }

    Hunhandle* handle = nullptr;
    QByteArray encodedText;
    if (useUserDictionary) {
        handle = host.mpConsole->getHunspellHandle_user();
        encodedText = text.toUtf8();
    } else {
        handle = host.mpConsole->getHunspellHandle_system();
        if (!handle) {
            lua_pushnil(L);
            lua_pushstring(L, "no main dictionaries found: Mudlet has not been able to find any dictionary files to use so is unable to check your word");
            return 2;
        }

        encodedText = host.mpConsole->getHunspellCodec_system()->fromUnicode(text);
    }
    // CHECKME: Is there any danger of contention here - do we need to get mudlet::mDictionaryReadWriteLock locked for reading if we are accessing the shared user dictionary?
    lua_pushboolean(L, Hunspell_spell(handle, text.toUtf8().constData()));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#spellSuggestWord
int TLuaInterpreter::spellSuggestWord(lua_State* L)
{
    Host& host = getHostFromLua(L);
    bool hasUserDictionary = false;
    bool hasSharedDictionary = false;
    host.getUserDictionaryOptions(hasUserDictionary, hasSharedDictionary);
    QString text;
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "spellSuggestWord: bad argument #1 type (word as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    } else {
        text = QString::fromUtf8(lua_tostring(L, 1));
    }

    bool useUserDictionary = false;
    if (lua_gettop(L) > 1) {
        if (!lua_isboolean(L, 2)) {
            lua_pushfstring(L, "spellSuggestWord: bad argument #2 type (check profile dictionary as boolean is optional {use 'false' or omit to check against system dictionary}, got %s!)", luaL_typename(L, 2));
            return lua_error(L);
        } else {
            useUserDictionary = lua_toboolean(L, 2);
            if (useUserDictionary && !hasUserDictionary) {
                lua_pushnil(L);
                lua_pushstring(L, "no user dictionary enabled in the preferences for this profile");
                return 2;
            }
        }
    }

    char **wordList;
    size_t wordCount = 0;
    Hunhandle* handle = nullptr;
    QByteArray encodedText;
    if (useUserDictionary) {
        handle = host.mpConsole->getHunspellHandle_user();
        encodedText = text.toUtf8();
    } else {
        handle = host.mpConsole->getHunspellHandle_system();
        if (!handle) {
            lua_pushnil(L);
            lua_pushstring(L, "no main dictionaries found: Mudlet has not been able to find any dictionary files to use so is unable to make suggestions for your word");
            return 2;
        }

        encodedText = host.mpConsole->getHunspellCodec_system()->fromUnicode(text);
    }
    // CHECKME: Is there any danger of contention here - do we need to get mudlet::mDictionaryReadWriteLock locked for reading if we are accessing the shared user dictionary?
    wordCount = Hunspell_suggest(handle, &wordList, encodedText.constData());
    lua_newtable(L);
    for (size_t i = 0; i < wordCount; ++i) {
        lua_pushnumber(L, i+1);
        lua_pushstring(L, wordList[i]);
        lua_settable(L, -3);
    }
    Hunspell_free_list(handle, &wordList, wordCount);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getDictionaryWordList
int TLuaInterpreter::getDictionaryWordList(lua_State* L)
{
    Host& host = getHostFromLua(L);
    bool hasUserDictionary = false;
    bool hasSharedDictionary = false;
    host.getUserDictionaryOptions(hasUserDictionary, hasSharedDictionary);
    if (!hasUserDictionary) {
        lua_pushnil(L);
        lua_pushstring(L, "no user dictionary enabled in the preferences for this profile");
        return 2;
    }

    // This may stall if this is accessing the shared user dictionary and that
    // is being updated by another profile, but it should eventually return...
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    // We must keep a local reference/copy of the value returned because the
    // returned item is a deep-copy in the case of a shared dictionary and two
    // calls to TConsole::getWordSet() can return two different instances which
    // is fatally dangerous if used in a range based initialiser:
    QSet<QString> wordSet{host.mpConsole->getWordSet()};
    QStringList wordList{wordSet.begin(), wordSet.end()};
#else
    QStringList wordList{host.mpConsole->getWordSet().toList()};
#endif
    int wordCount = wordList.size();
    if (wordCount > 1) {
        QCollator sorter;
        sorter.setCaseSensitivity(Qt::CaseInsensitive);
        std::sort(wordList.begin(), wordList.end(), sorter);
    }

    lua_newtable(L);
    for (int i = 0; i < wordCount; ++i) {
        lua_pushinteger(L, i+1);
        lua_pushstring(L, wordList.at(i).toUtf8().constData());
        lua_settable(L, -3);
    }

    return 1;
}

// Internal function - helper for updateColorTable().
void TLuaInterpreter::insertColorTableEntry(lua_State* L, const QColor& color, const QString& name)
{
    // Equivalent (when called from updateColorTable()) to Lua (where the
    // '<8-bit unsigned int, i.e. 0 to 255>'s are provided from the QColor):
    // color_table["name"] = { <color.red()>, <color.green()>, <color.blue()> }

    // Creates a new empty table on the stack with space preallocated for 3
    // array elements and 0 non-array elements:
    lua_createtable(L, 3, 0);

    lua_pushnumber(L, color.red());
    lua_rawseti(L, -2, 1);

    lua_pushnumber(L, color.green());
    lua_rawseti(L, -2, 2);

    lua_pushnumber(L, color.blue());
    lua_rawseti(L, -2, 3);

    lua_getfield(L, LUA_GLOBALSINDEX, "color_table");
    lua_insert(L, -2);

    lua_pushstring(L, name.toLatin1().constData());
    lua_insert(L, -2);
    lua_settable(L, -3);
    lua_pop(L, 1);
}

// Internal function - copies current profile's 16 ANSI colors into the Lua "color_table"
void TLuaInterpreter::updateAnsi16ColorsInTable()
{
    lua_State* L = pGlobalLua;
    if (!L) {
        return;
    }

    // Does the color_table already exist:
    // Equivalent to Lua:
    // color_table = color_table or {}
    lua_getfield(L, LUA_GLOBALSINDEX, "color_table");
    if (!(lua_toboolean(L,-1))) {
        // no it doesn't
        lua_pop(L, 1);
        // So make it
        lua_newtable(L);
    }

    // Okay so now we point ourselves at the wanted table:
    lua_setfield(L, LUA_GLOBALSINDEX, "color_table");

    // Now we can add/update the items we need to, though it is a bit repetative:
    QColor color = mpHost->mBlack;
    insertColorTableEntry(L, color, QStringLiteral("ansi_000"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_black"));
    insertColorTableEntry(L, color, QStringLiteral("ansiBlack"));

    color = mpHost->mRed;
    insertColorTableEntry(L, color, QStringLiteral("ansi_001"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_red"));
    insertColorTableEntry(L, color, QStringLiteral("ansiRed"));

    color = mpHost->mGreen;
    insertColorTableEntry(L, color, QStringLiteral("ansi_002"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_green"));
    insertColorTableEntry(L, color, QStringLiteral("ansiGreen"));

    color = mpHost->mYellow;
    insertColorTableEntry(L, color, QStringLiteral("ansi_003"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_yellow"));
    insertColorTableEntry(L, color, QStringLiteral("ansiYellow"));

    color = mpHost->mBlue;
    insertColorTableEntry(L, color, QStringLiteral("ansi_004"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_blue"));
    insertColorTableEntry(L, color, QStringLiteral("ansiBlue"));

    color = mpHost->mMagenta;
    insertColorTableEntry(L, color, QStringLiteral("ansi_005"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_magenta"));
    insertColorTableEntry(L, color, QStringLiteral("ansiMagenta"));

    color = mpHost->mCyan;
    insertColorTableEntry(L, color, QStringLiteral("ansi_006"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_cyan"));
    insertColorTableEntry(L, color, QStringLiteral("ansiCyan"));

    color = mpHost->mWhite;
    insertColorTableEntry(L, color, QStringLiteral("ansi_007"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_white"));
    insertColorTableEntry(L, color, QStringLiteral("ansiWhite"));

    color = mpHost->mLightBlack;
    insertColorTableEntry(L, color, QStringLiteral("ansi_008"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_light_black"));
    insertColorTableEntry(L, color, QStringLiteral("ansiLightBlack"));

    color = mpHost->mLightRed;
    insertColorTableEntry(L, color, QStringLiteral("ansi_009"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_light_red"));
    insertColorTableEntry(L, color, QStringLiteral("ansiLightRed"));

    color = mpHost->mLightGreen;
    insertColorTableEntry(L, color, QStringLiteral("ansi_010"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_light_green"));
    insertColorTableEntry(L, color, QStringLiteral("ansiLightGreen"));

    color = mpHost->mLightYellow;
    insertColorTableEntry(L, color, QStringLiteral("ansi_011"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_light_yellow"));
    insertColorTableEntry(L, color, QStringLiteral("ansiLightYellow"));

    color = mpHost->mLightBlue;
    insertColorTableEntry(L, color, QStringLiteral("ansi_012"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_light_blue"));
    insertColorTableEntry(L, color, QStringLiteral("ansiLightBlue"));

    color = mpHost->mLightMagenta;
    insertColorTableEntry(L, color, QStringLiteral("ansi_013"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_light_magenta"));
    insertColorTableEntry(L, color, QStringLiteral("ansiLightMagenta"));

    color = mpHost->mLightCyan;
    insertColorTableEntry(L, color, QStringLiteral("ansi_014"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_light_cyan"));
    insertColorTableEntry(L, color, QStringLiteral("ansiLightCyan"));

    color = mpHost->mLightWhite;
    insertColorTableEntry(L, color, QStringLiteral("ansi_015"));
    insertColorTableEntry(L, color, QStringLiteral("ansi_light_white"));
    insertColorTableEntry(L, color, QStringLiteral("ansiLightWhite"));
}

// Internal function - copies current profile's extended ANSI colors into the
// Lua "color_table" - it might be feasible to do this entirely within an
// external lua file ("GUIUtils.lua2) as we do not provide a means to vary
// the ANSI colours 17 to 255 that this handles...
void TLuaInterpreter::updateExtendedAnsiColorsInTable()
{
    lua_State* L = pGlobalLua;
    if (!L) {
        return;
    }

    // Does the color_table already exist:
    // Equivalent to Lua:
    // color_table = color_table or {}
    lua_getfield(L, LUA_GLOBALSINDEX, "color_table");
    if (!(lua_toboolean(L,-1))) {
        // no it doesn't
        lua_pop(L, 1);
        // So make it
        lua_newtable(L);
    }

    // Okay so now we point ourselves at the wanted table:
    lua_setfield(L, LUA_GLOBALSINDEX, "color_table");

    // And insert the 6x6x6 RGB colours
    for (int i = 0; i < 216; ++i) {
        int r = i / 36;
        int g = (i - (r * 36)) / 6;
        int b = (i - (r * 36)) - (g * 6);

        lua_createtable(L, 3, 0);

        lua_pushnumber(L, 51 * r);
        lua_rawseti(L, -2, 1);

        lua_pushnumber(L, 51 * g);
        lua_rawseti(L, -2, 2);

        lua_pushnumber(L, 51 * b);
        lua_rawseti(L, -2, 3);

        lua_getfield(L, LUA_GLOBALSINDEX, "color_table");
        lua_insert(L, -2);

        QString name = QStringLiteral("ansi_%1").arg(i + 16, 3, 10, QLatin1Char('0'));
        lua_pushstring(L, name.toLatin1().constData());
        lua_insert(L, -2);
        lua_settable(L, -3);
        lua_pop(L, 1);
    }

    // And insert the 24 Greyscale colours
    for (int i = 232; i < 256; ++i) {
        lua_createtable(L, 3, 0);

        int value = 128;
        // Divide the range 0 to 255 into 23 + 1 values to give a 24 value
        // greyscale - this is lifted from TBuffer::decodeSGR{3|4}8() - and
        // we really ought to refactor things so we only have this in one common
        // place:
        switch (i) {
            case 232:   value =   0; break; //   0.000
            case 233:   value =  11; break; //  11.087
            case 234:   value =  22; break; //  22.174
            case 235:   value =  33; break; //  33.261
            case 236:   value =  44; break; //  44.348
            case 237:   value =  55; break; //  55.435
            case 238:   value =  67; break; //  66.522
            case 239:   value =  78; break; //  77.609
            case 240:   value =  89; break; //  88.696
            case 241:   value = 100; break; //  99.783
            case 242:   value = 111; break; // 110.870
            case 243:   value = 122; break; // 121.957
            case 244:   value = 133; break; // 133.043
            case 245:   value = 144; break; // 144.130
            case 246:   value = 155; break; // 155.217
            case 247:   value = 166; break; // 166.304
            case 248:   value = 177; break; // 177.391
            case 249:   value = 188; break; // 188.478
            case 250:   value = 200; break; // 199.565
            case 251:   value = 211; break; // 210.652
            case 252:   value = 222; break; // 221.739
            case 253:   value = 233; break; // 232.826
            case 254:   value = 244; break; // 243.913
            case 255:   value = 255; break; // 255.000
            default:
            Q_UNREACHABLE(); // We should not have a case outside of the range 232 to 255
        }

        lua_pushnumber(L, value);
        lua_rawseti(L, -2, 1);

        lua_pushnumber(L, value);
        lua_rawseti(L, -2, 2);

        lua_pushnumber(L, value);
        lua_rawseti(L, -2, 3);

        lua_getfield(L, LUA_GLOBALSINDEX, "color_table");
        lua_insert(L, -2);

        QString name = QStringLiteral("ansi_%1").arg(i, 3, 10, QLatin1Char('0'));
        lua_pushstring(L, name.toLatin1().constData());
        lua_insert(L, -2);
        lua_settable(L, -3);
        lua_pop(L, 1);
    }
}

// Internal function - Creates a table for useful information from the http
// response. It creates an empty table, calls upon other functions to
// add things to it, and then returns a key to where it is in the lua registery.
int TLuaInterpreter::createHttpResponseTable(QNetworkReply* reply)
{
    lua_State* L = pGlobalLua;
    if (!L) {
        qDebug() << "LUA CRITICAL ERROR: no suitable Lua execution unit found.";
        return {};
    }

    // Push empty table onto stack
    lua_newtable(L);
    // Add "headers" table to table
    createHttpHeadersTable(L, reply);
    // Add "cookies" table to table
    createCookiesTable(L, reply);
    // Pop table from stack, store it in registry, return key to it
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

// Internal function - Adds an empty table to a "headers" key to the table for
// tracking useful information from the http response. If there are any headers
// in the http response, it adds them to this new empty table.
void TLuaInterpreter::createHttpHeadersTable(lua_State* L, QNetworkReply* reply)
{
    // Assert table from createHttpResponseTable is where we expect it to be
    if (!lua_istable(L, -1)) {
        qDebug() << "Unable to find table at top of lua stack, aborting!";
        return;
    }

    // Push "headers" key and empty table value onto stack
    lua_pushstring(L, "headers");
    lua_newtable(L);

    // Parse headers, add them as key-value pairs to the empty table
    const QList<QByteArray> headerList = reply->rawHeaderList();
    for (QByteArray header : headerList) {
        // Push header key onto stack
        lua_pushstring(L, header.constData());
        // Push header value onto stack
        lua_pushstring(L,  reply->rawHeader(header).constData());
        // Put key-value pair into table (now 3 deep in stack), pop stack twice
        lua_settable(L, -3);
    }

    // Put "headers" table into table (now 3 deep in stack), pop stack twice
    lua_settable(L, -3);
}

// Internal function - Adds an empty table to a "cookies" key to the table for
// tracking useful information from the http response. If there are any cookies
// in the http response, it adds them to this new empty table.
void TLuaInterpreter::createCookiesTable(lua_State* L, QNetworkReply* reply)
{
    // Assert table from createHttpResponseTable is where we expect it to be
    if (!lua_istable(L, -1)) {
        qDebug() << "Unable to find table at top of lua stack, aborting!";
        return;
    }

    // Push "cookies" key and empty table value onto stack
    lua_pushstring(L, "cookies");
    lua_newtable(L);

    // Parse cookies, add them as key-value pairs to the empty table
    Host& host = getHostFromLua(L);
    QNetworkCookieJar* cookieJar = host.mLuaInterpreter.mpFileDownloader->cookieJar();
    QList<QNetworkCookie> cookies = cookieJar->cookiesForUrl(reply->url());
    for (QNetworkCookie cookie : cookies) {
        // Push cookie name onto stack
        lua_pushstring(L, cookie.name().constData());
        // Push cookie value onto stack
        lua_pushstring(L,  cookie.value().constData());
        // Put key-value pair into table (now 3 deep in stack), pop stack twice
        lua_settable(L, -3);
    }

    // Put "cookies" table into table (now 3 deep in stack), pop stack twice
    lua_settable(L, -3);
}
