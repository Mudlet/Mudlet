/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2023, 2025 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2016 by Eric Wallace - eewallace@gmail.com              *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2016-2018 by Ian Adkins - ieadkins@gmail.com            *
 *   Copyright (C) 2017 by Chris Reid - WackyWormer@hotmail.com            *
 *   Copyright (C) 2022-2025 by Lecker Kebap - Leris@mudlet.org            *
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
#include "TCommandLine.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TEvent.h"
#include "TFlipButton.h"
#include "TForkedProcess.h"
#include "TGameDetails.h"
#include "TLabel.h"
#include "TMapLabel.h"
#include "TRoomDB.h"
#include "TTextEdit.h"
#include "TTimer.h"
#include "dlgComposer.h"
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgModuleManager.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"
#if defined(INCLUDE_3DMAPPER)
#include "glwidget_integration.h"
#endif

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

using namespace std::chrono_literals;

extern "C" {
int luaopen_yajl(lua_State*);
}


// No documentation available in wiki - internal function
static bool isMain(const QString& name)
{
    if (name.isEmpty()) {
        return true;
    }
    if (!name.compare(qsl("main"))) {
        return true;
    }
    return false;
}

static const char *bad_cmdline_type = "%s: bad argument #%d type (command line name as string expected, got %s)!";
static const char *bad_cmdline_value = "command line \"%s\" not found";

const QString TLuaInterpreter::csmInvalidRoomID{qsl("number %1 is not a valid roomID")};
const QString TLuaInterpreter::csmInvalidStopWatchID{qsl("stopwatch with ID %1 not found")};
const QString TLuaInterpreter::csmInvalidRedValue{qsl("red value %1 needs to be between 0-255")};
const QString TLuaInterpreter::csmInvalidGreenValue{qsl("green value %1 needs to be between 0-255")};
const QString TLuaInterpreter::csmInvalidBlueValue{qsl("blue value %1 needs to be between 0-255")};
const QString TLuaInterpreter::csmInvalidAlphaValue{qsl("alpha value %1 needs to be between 0-255")};
const QString TLuaInterpreter::csmInvalidExitRoomID{qsl("number %1 is not a valid exit roomID")};
const QString TLuaInterpreter::csmInvalidItemID{qsl("item ID as %1 does not seem to be parseable as a positive integer")};
const QString TLuaInterpreter::csmInvalidAreaID{qsl("number %1 is not a valid area id")};
const QString TLuaInterpreter::csmInvalidAreaName{qsl("string '%1' is not a valid area name")};

#define CMDLINE_NAME(ARG_L, ARG_pos)                                                                 \
    ({                                                                                               \
        int pos_ = (ARG_pos);                                                                        \
        if (!lua_isstring(ARG_L, pos_)) {                                                            \
            lua_pushfstring(ARG_L, bad_cmdline_type, __FUNCTION__, pos_, luaL_typename(ARG_L, pos_));\
            return lua_error(ARG_L);                                                                 \
        }                                                                                            \
        lua_tostring(ARG_L, pos_);                                                                   \
    })

#define COMMANDLINE(ARG_L, ARG_name)                                                           \
    ({                                                                                         \
        const QString& name_ = (ARG_name);                                                     \
        auto console_ = getHostFromLua(ARG_L).mpConsole;                                       \
        auto cmdLine_ = isMain(name_) ? &*console_->mpCommandLine                              \
                                    : console_->mSubCommandLineMap.value(name_);               \
        if (!cmdLine_) {                                                                       \
            lua_pushnil(ARG_L);                                                                \
            lua_pushfstring(ARG_L, bad_cmdline_value, name_.toUtf8().constData());             \
            return 2;                                                                          \
        }                                                                                      \
        cmdLine_;                                                                              \
    })

// variable names within these macros have trailing underscores because in
// at least one case, masking an existing variable with the new one confused
// GCC, leading to a crash.


TLuaInterpreter::TLuaInterpreter(Host* pH, const QString& hostName, int id)
: mpHost(pH)
, hostName(hostName)
, mHostID(id)
, purgeTimer(this)
, mpFileDownloader(new QNetworkAccessManager(this))
, mpFileSystemWatcher(new QFileSystemWatcher(this))
{
    connect(&purgeTimer, &QTimer::timeout, this, &TLuaInterpreter::slot_purge);
    connect(mpFileDownloader, &QNetworkAccessManager::finished, this, &TLuaInterpreter::slot_httpRequestFinished);
    connect(mpFileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &TLuaInterpreter::slot_pathChanged);
    connect(mpFileSystemWatcher, &QFileSystemWatcher::directoryChanged, this, &TLuaInterpreter::slot_pathChanged);

    initLuaGlobals();

    purgeTimer.start(2s);
}

TLuaInterpreter::~TLuaInterpreter()
{
    lua_close(pGlobalLua);
}

// No documentation available in wiki - internal function
// Replaces a check like this:
//    if (!lua_isboolean(L, 14)) {
//        lua_pushfstring(L,
//            "createMapLabel: bad argument #14 type (showOnTop as boolean is optional, got %s!)",
//            luaL_typename(L, 14));
//        return lua_error(L);
//    }
//    bool showOnTop = lua_toboolean(L, 14);
//
// With reduced repetition like that:
//    bool showOnTop = getVerifiedBool(L, "createMapLabel", 14, "showOnTop", true);
//
// The "isOptional" parameter is optional but modifies the error message to say
// that an argument is optional and it will default to not-optional parameters!
// HOWEVER it does not actually handle the absence of an argument that is
// supposed to BE optional - that has to be done by the caller before it
// makes the call... 8-P
//
// See also: getVerifiedString, getVerifiedInt, getVerifiedFloat, errorArgumentType
bool TLuaInterpreter::getVerifiedBool(lua_State* L, const char* functionName, const int pos, const char* publicName, const bool isOptional)
{
    if (!lua_isboolean(L, pos)) {
        errorArgumentType(L, functionName, pos, publicName, "boolean", isOptional);
        lua_error(L);
        Q_UNREACHABLE();
    }
    return lua_toboolean(L, pos);
}

// No documentation available in wiki - internal function
// See also: getVerifiedBool
/*static*/ std::pair<bool, QString> TLuaInterpreter::getVerifiedStringOrInteger(lua_State* L, const char* functionName, const int pos, const char* publicName, const bool isOptional)
{
    if (lua_type(L, pos) == LUA_TNUMBER) {
        // use lua_tonumber(...) and round because lua_tointeger(...) can return
        // oversized values (long long int?) on Windows which do not always fit
        // into an int:
        return {true, QString::number(qRound(lua_tonumber(L, pos)))};
    }

    if (lua_type(L, pos) == LUA_TSTRING) {
        return {false, lua_tostring(L, pos)};
    }

    errorArgumentType(L, functionName, pos, publicName, "string or integer", isOptional);
    lua_error(L);
    Q_UNREACHABLE();
}

// No documentation available in wiki - internal function
// See also: getVerifiedBool
QString TLuaInterpreter::getVerifiedString(lua_State* L, const char* functionName, const int pos, const char* publicName, const bool isOptional)
{
    if (!lua_isstring(L, pos)) {
        errorArgumentType(L, functionName, pos, publicName, "string", isOptional);
        lua_error(L);
        Q_UNREACHABLE();
    }
    return lua_tostring(L, pos);
}

// No documentation available in wiki - internal function
// See also: getVerifiedBool
int TLuaInterpreter::getVerifiedInt(lua_State* L, const char* functionName, const int pos, const char* publicName, const bool isOptional)
{
    if (!lua_isnumber(L, pos)) {
        errorArgumentType(L, functionName, pos, publicName, "number", isOptional);
        lua_error(L);
        Q_UNREACHABLE();
    }
    return lua_tointeger(L, pos);
}

// No documentation available in wiki - internal function
// See also: getVerifiedBool
float TLuaInterpreter::getVerifiedFloat(lua_State* L, const char* functionName, const int pos, const char* publicName, const bool isOptional)
{
    if (!lua_isnumber(L, pos)) {
        errorArgumentType(L, functionName, pos, publicName, "number", isOptional);
        lua_error(L);
        Q_UNREACHABLE();
    }
    return static_cast <float> (lua_tonumber(L, pos));
}

// No documentation available in wiki - internal function
// See also: getVerifiedBool
double TLuaInterpreter::getVerifiedDouble(lua_State* L, const char* functionName, const int pos, const char* publicName, const bool isOptional)
{
    if (!lua_isnumber(L, pos)) {
        errorArgumentType(L, functionName, pos, publicName, "number", isOptional);
        lua_error(L);
        Q_UNREACHABLE();
    }
    return lua_tonumber(L, pos);
}

// No documentation available in wiki - internal function
// Raises a Lua error in case of an API usage mistake
// See also: getVerifiedBool, warnArgumentValue
void TLuaInterpreter::errorArgumentType(lua_State* L, const char* functionName, const int pos, const char* publicName, const char* publicType, const bool isOptional)
{
    if (isOptional) {
        lua_pushfstring(L, "%s: bad argument #%d type (%s as %s is optional, got %s!)",
            functionName, pos, publicName, publicType, luaL_typename(L, pos));
    } else {
        lua_pushfstring(L, "%s: bad argument #%d type (%s as %s expected, got %s!)",
            functionName, pos, publicName, publicType, luaL_typename(L, pos));
    }
}

// No documentation available in wiki - internal function
// returns nil+msg in case of a data mistake, for example a missing room. Should not raise a Lua error
// See also: announceWrongArgumentType
int TLuaInterpreter::warnArgumentValue(lua_State* L, const char* functionName, const QString& message, const bool useFalseInsteadofNil)
{
    if (Q_LIKELY(!useFalseInsteadofNil)) {
        lua_pushnil(L);
    } else {
        lua_pushboolean(L, false);
    }
    lua_pushstring(L, message.toUtf8().constData());
    if (mudlet::smDebugMode) {
        auto& host = getHostFromLua(L);
        TDebug(Qt::white, QColorConstants::Svg::orange) << "Lua: " << functionName << ": " << message << "\n" >> &host;
    }
    return 2;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::warnArgumentValue(lua_State* L, const char* functionName, const char* message, const bool useFalseInsteadofNil)
{
    if (Q_LIKELY(!useFalseInsteadofNil)) {
        lua_pushnil(L);
    } else {
        lua_pushboolean(L, false);
    }
    lua_pushstring(L, message);
    if (mudlet::smDebugMode) {
        auto& host = getHostFromLua(L);
        TDebug(Qt::white, QColorConstants::Svg::orange) << "Lua: " << functionName << ": " << message << "\n" >> &host;
    }
    return 2;
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
        qWarning() << qsl("TLuaInterpreter::slot_httpRequestFinished(...) ERROR: NULL Host pointer!");
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        TEvent event {};
        QString localFileName;

        switch (reply->operation()) {
        case QNetworkAccessManager::PostOperation:
            event.mArgumentList << qsl("sysPostHttpError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->errorString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->url().toString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            break;

        case QNetworkAccessManager::PutOperation:
            event.mArgumentList << qsl("sysPutHttpError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->errorString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->url().toString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            break;

        case QNetworkAccessManager::GetOperation:
            localFileName = downloadMap.value(reply);
            event.mArgumentList << (localFileName.isEmpty()
                                   ? qsl("sysGetHttpError")
                                   : qsl("sysDownloadError"));
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
            event.mArgumentList << qsl("sysDeleteHttpError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->errorString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->url().toString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            break;

        case QNetworkAccessManager::HeadOperation:
            break;
        case QNetworkAccessManager::CustomOperation:
            event.mArgumentList << qsl("sysCustomHttpError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->errorString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->url().toString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << reply->request().attribute(QNetworkRequest::CustomVerbAttribute).toString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
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

// No documentation available in wiki - internal function
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
        event.mArgumentList << qsl("sysDeleteHttpDone");
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << reply->url().toString();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << QString(reply->readAll());
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        break;

    case QNetworkAccessManager::CustomOperation:
        event.mArgumentList << QString("sysCustomHttpDone");
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << reply->url().toString();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << QString(reply->readAll());
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << reply->request().attribute(QNetworkRequest::CustomVerbAttribute).toString();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        break;
    case QNetworkAccessManager::UnknownOperation:
        break;

    case QNetworkAccessManager::PostOperation:
        event.mArgumentList << qsl("sysPostHttpDone");
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << reply->url().toString();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << QString(reply->readAll());
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        break;

    case QNetworkAccessManager::PutOperation:
        event.mArgumentList << qsl("sysPutHttpDone");
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << reply->url().toString();
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        event.mArgumentList << QString(reply->readAll());
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        break;

    case QNetworkAccessManager::GetOperation:
        const QString localFileName = downloadMap.value(reply);
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
            event.mArgumentList << QString(reply->readAll());
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            break;
        }

        QSaveFile localFile(localFileName);
        if (!localFile.open(QFile::WriteOnly)) {
            event.mArgumentList << QLatin1String("sysDownloadError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("Couldn't save to the destination file");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << localFileName;
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("Couldn't open the destination file for writing (permission errors?)");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            break;
        }

        qint64 const bytesWritten = localFile.write(reply->readAll());
        if (bytesWritten == -1) {
            event.mArgumentList << QLatin1String("sysDownloadError");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("Couldn't save to the destination file");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << localFileName;
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << QLatin1String("Couldn't write downloaded content into the destination file");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            break;
        }

        if (!localFile.commit()) {
            qDebug() << "TTLuaInterpreter::handleHttpOK: error saving downloaded file: " << localFile.errorString();
        }

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
            event.mArgumentList << QLatin1String("Couldn't save to the destination file");
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << localFileName;
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            event.mArgumentList << localFile.errorString();
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
        }
        break;

    }

    event.mArgumentList << QString::number(createHttpResponseTable(reply));
    event.mArgumentTypeList << ARGUMENT_TYPE_TABLE;


    reply->deleteLater();
    pHost->raiseEvent(event);
}

// No documentation available in wiki - internal function
void TLuaInterpreter::raiseDownloadProgressEvent(lua_State* L, QString fileUrl, qint64 bytesDownloaded, qint64 totalBytes)
{
    Host& host = getHostFromLua(L);

    TEvent event {};
    event.mArgumentList << qsl("sysDownloadFileProgress");
    event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
    event.mArgumentList << fileUrl;
    event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
    event.mArgumentList << QString::number(bytesDownloaded);
    event.mArgumentTypeList << ARGUMENT_TYPE_NUMBER;
    if(totalBytes >= 0) {
        event.mArgumentList << QString::number(totalBytes);
        event.mArgumentTypeList << ARGUMENT_TYPE_NUMBER;
    } else {
        event.mArgumentList << QString();
        event.mArgumentTypeList << ARGUMENT_TYPE_NIL;
    }

    host.raiseEvent(event);
}

// No documentation available in wiki - internal function
void TLuaInterpreter::slot_pathChanged(const QString& path)
{
    // According to QtDocs it is possible that some editors will delete old file and create new one on edits
    // Therefore it is required to add file to watches again
    if (!mpFileSystemWatcher->files().contains(path) && QFile::exists(path)) {
        mpFileSystemWatcher->addPath(path);
    }

    TEvent event {};
    event.mArgumentList << QLatin1String("sysPathChanged");
    event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
    event.mArgumentList << path;
    event.mArgumentTypeList << ARGUMENT_TYPE_STRING;

    mpHost->raiseEvent(event);
}

// No documentation available in wiki - internal function
void TLuaInterpreter::slot_deleteSender(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    objectsToDelete.append(sender());
}

// No documentation available in wiki - internal function
void TLuaInterpreter::slot_purge()
{
    while (!objectsToDelete.isEmpty()) {
        delete objectsToDelete.takeFirst();
    }
}

// No documentation available in wiki - internal function
int TLuaInterpreter::Wait(lua_State* L)
{
    const int n = lua_gettop(L);
    if (n != 1) {
        lua_pushstring(L, "Wait: wrong number of arguments");
        return lua_error(L);
    }

    const int luaSleepMsec = getVerifiedInt(L, __func__, 1, "sleep time in msec");
    msleep(luaSleepMsec); // FIXME thread::sleep()
    return 0;
}

// No documentation available in wiki - internal function
// dirToString will now catch and validate pretty much any string that could
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
        qint64 const dirNum = lua_tonumber(L, position);
        switch (dirNum) {
        // breaks not needed - all handled cases end in a return!
        case 1:
            return qsl("n");
        case 2:
            return qsl("ne");
        case 3:
            return qsl("nw");
        case 4:
            return qsl("e");
        case 5:
            return qsl("w");
        case 6:
            return qsl("s");
        case 7:
            return qsl("se");
        case 8:
            return qsl("sw");
        case 9:
            return qsl("up");
        case 10:
            return qsl("down");
        case 11:
            return qsl("in");
        case 12:
            return qsl("out");
        default:
            return QString();
        }

    } else if (lua_isstring(L, position)) {
        QString direction{lua_tostring(L, position)};
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
    if (lua_type(L, position) == LUA_TSTRING) {
        dir = lua_tostring(L, position);
        dir = dir.toLower();
        if (!dir.compare(QLatin1String("n")) || !dir.compare(QLatin1String("north"))) {
            return DIR_NORTH;
        }
        if (!dir.compare(QLatin1String("e")) || !dir.compare(QLatin1String("east"))) {
            return DIR_EAST;
        }
        if (!dir.compare(QLatin1String("s")) || !dir.compare(QLatin1String("south"))) {
            return DIR_SOUTH;
        }
        if (!dir.compare(QLatin1String("w")) || !dir.compare(QLatin1String("west"))) {
            return DIR_WEST;
        }
        if (!dir.compare(QLatin1String("u")) || !dir.compare(QLatin1String("up"))) {
            return DIR_UP;
        }
        if (!dir.compare(QLatin1String("d")) || !dir.compare(QLatin1String("down"))) {
            return DIR_DOWN;
        }
        if (!dir.compare(QLatin1String("ne")) || !dir.compare(QLatin1String("northeast"))) {
            return DIR_NORTHEAST;
        }
        if (!dir.compare(QLatin1String("nw")) || !dir.compare(QLatin1String("northwest"))) {
            return DIR_NORTHWEST;
        }
        if (!dir.compare(QLatin1String("se")) || !dir.compare(QLatin1String("southeast"))) {
            return DIR_SOUTHEAST;
        }
        if (!dir.compare(QLatin1String("sw")) || !dir.compare(QLatin1String("southwest"))) {
            return DIR_SOUTHWEST;
        }
        if (!dir.compare(QLatin1String("i")) || !dir.compare(QLatin1String("in"))) {
            return DIR_IN;
        }
        if (!dir.compare(QLatin1String("o")) || !dir.compare(QLatin1String("out"))) {
            return DIR_OUT;
        }
    }
    if (lua_type(L, position) == LUA_TNUMBER) {
        dirNum = lua_tonumber(L, position);
        return (dirNum >= DIR_NORTH && dirNum <= DIR_OUT ? dirNum : 0);
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetProfile
int TLuaInterpreter::resetProfile(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.resetProfile_phase1();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getWindowsCodepage
int TLuaInterpreter::getWindowsCodepage(lua_State* L)
{
#if defined (Q_OS_WINDOWS)
    QSettings registry(qsl(R"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Nls\CodePage)"),
                       QSettings::NativeFormat);
    auto value = registry.value(qsl("ACP"));
    lua_pushstring(L, value.toString().toUtf8().constData());
    return 1;
#else
    return warnArgumentValue(L, __func__, "this function is only needed on Windows, and does not work here");
#endif
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#spawn
int TLuaInterpreter::spawn(lua_State* L)
{
    Host& host = getHostFromLua(L);
    return TForkedProcess::startProcess(host.getLuaInterpreter(), L);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadReplay
int TLuaInterpreter::loadReplay(lua_State* L)
{
    const QString replayFileName = getVerifiedString(L, __func__, 1, "replay file name");
    if (replayFileName.isEmpty()) {
        return warnArgumentValue(L, __func__, "a blank string is not a valid replay file name");
    }

    Host& host = getHostFromLua(L);
    QString errMsg;
    if (mudlet::self()->loadReplay(&host, replayFileName, &errMsg)) {
        lua_pushboolean(L, true);
        return 1;
    } else {
        // Although we only use English text for Lua messages the errMsg could
        // contain a Windows pathFileName which may use non-ASCII characters:
        return warnArgumentValue(L, __func__, qsl("unable to start replay, reason: '%1'").arg(errMsg));
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#cut
int TLuaInterpreter::cut(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    host.mpConsole->cut();
    return 0;
}

// Internal helper for feedTelnet(...) and socketRaw(...) that enables the
// construction of data with bytes that cannot be prepared by normal means
// - including embedded nulls - for testing off-line and for writing protocol
// handlers for those that Mudlet does not provide itself respectively:
// Note: although "<<" and ">>" are extra codes that convert to '<' and '>'
// respectively this looks as though singular instances of either - or
// unrecognised "tags" with unknown other characters between them will still be
// passed through unchanged.
QByteArray TLuaInterpreter::parseTelnetCodes(const QByteArray& input)
{
    // Increment the version - and document below - any changes to the table:
    const int tableVersion = 1;
    // Changes:
    // 1 - Initial version.

    /*
     * Entry grouping
     * * Hex digits
     * * `O_` prefix - well known Telnet sub-option (less common/relevant ones ommitted)
     * * ASCII character abbreviation
     * * `T_` prefix - Telnet control code
     */
    const QHash<QByteArray, unsigned char> lookupTable = {
        {QByteArray("<00>"), '\0'},
        {QByteArray("<O_BINARY>"), '\0'},
        {QByteArray("<NUL>"), '\0'},

        {QByteArray("<01>"), '\x01'},
        {QByteArray("<O_ECHO>"), '\x01'},
        {QByteArray("<SOH>"), '\x01'},

        {QByteArray("<02>"), '\x02'}, // Reconnect
        {QByteArray("<STX>"), '\x02'},

        {QByteArray("<03>"), '\x03'},
        {QByteArray("<O_SGA>"), '\x03'},
        {QByteArray("<ETX>"), '\x03'},

        {QByteArray("<04>"), '\x04'}, // Approx Message Size Negotiation
        {QByteArray("<EOT>"), '\x04'},

        {QByteArray("<05>"), '\x05'},
        {QByteArray("<O_STATUS>"), '\x05'},
        {QByteArray("<ENQ>"), '\x05'},

        {QByteArray("<06>"), '\x06'}, // Timing Mark
        {QByteArray("<ACK>"), '\x06'},

        {QByteArray("<07>"), '\x07'}, // Remote Controlled Trans and Echo
        {QByteArray("<BELL>"), '\x07'},

        {QByteArray("<08>"), '\x08'}, // Output Line Width
        {QByteArray("<BS>"), '\x08'},

        {QByteArray("<09>"), '\x09'}, // Output Page Size
        {QByteArray("<HTAB>"), '\x09'},

        {QByteArray("<0A>"), '\x0a'}, // Output Carriage-Return Disposition
        {QByteArray("<LF>"), '\x0a'},

        {QByteArray("<0B>"), '\x0b'}, // Output Horizontal Tab Stops
        {QByteArray("<VTAB>"), '\x0b'},

        {QByteArray("<0C>"), '\x0c'}, // Output Horizontal Tab Disposition
        {QByteArray("<FF>"), '\x0c'},

        {QByteArray("<0D>"), '\x0d'}, // Output Formfeed Disposition
        {QByteArray("<CR>"), '\x0d'},

        {QByteArray("<0E>"), '\x0e'}, // Output Vertical Tab Stops
        {QByteArray("<SO>"), '\x0e'},

        {QByteArray("<0F>"), '\x0f'}, // Output Vertical Tab Disposition
        {QByteArray("<SI>"), '\x0f'},

        {QByteArray("<10>"), '\x10'}, // Output Linefeed Disposition
        {QByteArray("<DLE>"), '\x10'},

        {QByteArray("<11>"), '\x11'}, // Extended ASCII
        {QByteArray("<DC1>"), '\x11'},

        {QByteArray("<12>"), '\x12'}, // Logout
        {QByteArray("<DC2"), '\x12'},

        {QByteArray("<13>"), '\x13'}, // Byte Macro
        {QByteArray("<DC3>"), '\x13'},

        {QByteArray("<14>"), '\x14'}, // Data Entry Terminal
        {QByteArray("<DC4>"), '\x14'},

        {QByteArray("<15>"), '\x15'}, // SUPDUP
        {QByteArray("<NAK>"), '\x15'},

        {QByteArray("<16>"), '\x16'}, // SUPDUP Output
        {QByteArray("<SYN>"), '\x16'},

        {QByteArray("<17>"), '\x17'}, // Send location
        {QByteArray("<ETB>"), '\x17'},

        {QByteArray("<18>"), '\x18'},
        {QByteArray("<O_TERM>"), '\x18'},
        {QByteArray("<CAN>"), '\x18'},

        {QByteArray("<19>"), '\x19'},
        {QByteArray("<O_EOR>"), '\x19'},
        {QByteArray("<EM>"), '\x19'},

        {QByteArray("<1A>"), '\x1a'}, // TACACS User Identification
        {QByteArray("<SUB>"), '\x1a'},

        {QByteArray("<1B>"), '\x1b'}, // Output Marking
        {QByteArray("<ESC>"), '\x1b'},

        {QByteArray("<1C>"), '\x1c'}, // Terminal Location Number
        {QByteArray("<FS>"), '\x1c'},

        {QByteArray("<1D>"), '\x1d'}, // Telnet 3270 Regime
        {QByteArray("<GS>"), '\x1d'},

        {QByteArray("<1E>"), '\x1e'}, // X.3 PAD
        {QByteArray("<RS>"), '\x1e'},

        {QByteArray("<1F>"), '\x1f'},
        {QByteArray("<O_NAWS>"), '\x1f'},
        {QByteArray("<US>"), '\x1f'},

        {QByteArray("<SP>"), '\x20'}, // 32 dec, Space

        {QByteArray("<O_NENV>"), '\x27'}, // 39 dec, New Environment (also MNES)

        {QByteArray("<O_CHARS>"), '\x2a'}, // 42 dec, Character Set

        {QByteArray("<O_KERMIT>"), '\x2f'}, // 47 dec

        {QByteArray("<O_MSDP>"), '\x45'}, // 69 dec

        {QByteArray("<O_MSSP>"), '\x46'}, // 70 dec

        {QByteArray("<O_MCCP>"), '\x55'}, // 85 dec

        {QByteArray("<O_MCCP2>"), '\x56'}, // 86 dec

        {QByteArray("<O_MSP>"), '\x5a'}, // 90 dec

        {QByteArray("<O_MXP>"), '\x5b'}, // 91 dec

        {QByteArray("<O_ZENITH>"), '\x5d'}, // 93 dec

        {QByteArray("<O_AARDWULF>"), '\x66'}, // 102 dec

        {QByteArray("<DEL>"), '\x7f'}, // 127 dec

        {QByteArray("<O_ATCP>"), '\xc8'}, // 200 dec

        {QByteArray("<O_GMCP>"), '\xc9'}, // 201 dec

        {QByteArray("<T_EOR>"), '\xef'}, // 239 dec

        {QByteArray("<F0>"), '\xf0'},
        {QByteArray("<T_SE>"), '\xf0'},

        {QByteArray("<F1>"), '\xf1'},
        {QByteArray("<T_NOP>"), '\xf1'},

        {QByteArray("<F2>"), '\xf2'},
        {QByteArray("<T_DM>"), '\xf2'},

        {QByteArray("<F3>"), '\xf3'},
        {QByteArray("<T_BRK>"), '\xf3'},

        {QByteArray("<F4>"), '\xf4'},
        {QByteArray("<T_IP>"), '\xf4'},

        {QByteArray("<F5>"), '\xf5'},
        {QByteArray("<T_ABOP>"), '\xf5'},

        {QByteArray("<F6>"), '\xf6'},
        {QByteArray("<T_AYT>"), '\xf6'},

        {QByteArray("<F7>"), '\xf7'},
        {QByteArray("<T_EC>"), '\xf7'},

        {QByteArray("<F8>"), '\xf8'},
        {QByteArray("<T_EL>"), '\xf8'},

        {QByteArray("<F9>"), '\xf9'},
        {QByteArray("<T_GA>"), '\xf9'},

        {QByteArray("<FA>"), '\xfa'},
        {QByteArray("<T_SB>"), '\xfa'},

        {QByteArray("<FB>"), '\xfb'},
        {QByteArray("<T_WILL>"), '\xfb'},

        {QByteArray("<FC>"), '\xfc'},
        {QByteArray("<T_WONT>"), '\xfc'},

        {QByteArray("<FD>"), '\xfd'},
        {QByteArray("<T_DO>"), '\xfd'},

        {QByteArray("<FE>"), '\xfe'},
        {QByteArray("<T_DONT>"), '\xfe'},

        {QByteArray("<FF>"), '\xff'},
        {QByteArray("<T_IAC>"), '\xff'}
    };

    QByteArray bytes;
    if (input.isEmpty()) {
        bytes = QByteArray::number(tableVersion);
    } else {
        for (qsizetype index = 0, total = input.size(); index < total; ++index) {
            if (input.at(index) == '<') {
                // got an opening marker
                if (((index + 1) < total) && (input.at(index + 1) == '<')) {
                    // got an escaped less than sign - so store it
                    bytes.append('<');
                    // nudge the index up one character
                    ++index;
                    // and process the next one
                    continue;
                }
                // Else we haven't got an escaped one so find the closing greater then
                qsizetype tagEnd = input.indexOf('>', index);
                if (tagEnd > index) {
                    // Found it, so extract the whole tag including delimiters
                    QByteArray tag;
                    for (qsizetype i = index; i <= tagEnd; ++i) {
                        // store it
                        tag.append(input.at(i));
                    }
                    // look the tag up:
                    if (lookupTable.contains(tag)) {
                        bytes.append(lookupTable.value(tag));
                    } else {
                        // Not found so append the original tag instead
                        bytes.append(tag);
                    }
                    // nudge the index up to the closing greater then
                    index = tagEnd;
                    // and process the next character
                    continue;
                }
            } else {
                if (input.at(index) == '>' && ((index + 1) < total) && (input.at(index + 1) == '>')) {
                    // got an escaped greater than sign - so store it
                    bytes.append('>');
                    // nudge the index up one character
                    ++index;
                    // and process the next one
                    continue;
                }
            }
            // Since we have not done anything that would have otherwise advanced
            // index, then just copy the current character:
            bytes.append(input.at(index));
        }
    }
    return bytes;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#feedTelnet
int TLuaInterpreter::feedTelnet(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L,
                        "feedTelnet: bad argument #1 type (imitation game server data as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 1));
        lua_error(L);
        Q_UNREACHABLE();
    }
    if (host.mTelnet.getConnectionState() != QAbstractSocket::UnconnectedState) {
        lua_pushnil(L);
        lua_pushstring(L, "feedTelnet: refused, telnet connection socket is not in the unconnected state");
        return 2;
    }
    const QByteArray rawData{lua_tostring(L, 1)};
    // We need to convert any "<*>" codes to their raw byte forms:
    QByteArray cookedData{parseTelnetCodes(rawData)};
    if (rawData.isEmpty()) {
        // This is a special case to get the table version
        lua_pushboolean(L, true);
        lua_pushfstring(L, "feedTelnet: using table version %s", cookedData.constData());
        return 2;
    }

    host.mTelnet.loopbackTest(cookedData);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#feedTriggers
int TLuaInterpreter::feedTriggers(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L,
                        "feedTriggers: bad argument #1 type (imitation game server text as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }
    const QByteArray data{lua_tostring(L, 1)};
    bool dataIsUtf8Encoded = true;
    if (lua_gettop(L) > 1) {
        dataIsUtf8Encoded = getVerifiedBool(L, __func__, 2, "Utf8Encoded", true);
    }

    const QByteArray currentEncoding = host.mTelnet.getEncoding();
    if (dataIsUtf8Encoded) {
        // We can convert the data from a QByteArray to a QString:
        if (currentEncoding == "UTF-8") {
            // Simple case: the encoding is already what we are using:
            std::string dataStdString{data.toStdString()};
            host.mpConsole->printOnDisplay(dataStdString);
            lua_pushboolean(L, true);
            return 1;
        }
        const QString dataQString{data};
        // else
            // We need to transcode it from UTF-8 into the current Game Server
            // encoding - this can fail if it includes any characters (as UTF-8)
            // that the game encoding cannot convey:
        auto* pDataCodec = QTextCodec::codecForName(currentEncoding);
        auto* pDataEncoder = pDataCodec->makeEncoder(QTextCodec::IgnoreHeader);
        if (!(currentEncoding.isEmpty() || currentEncoding == "ASCII")) {
            if (!pDataCodec->canEncode(dataQString)) {
                return warnArgumentValue(L, __func__, qsl(
                    "cannot send '%1' as it contains one or more characters that cannot be conveyed in the current game server encoding of '%2'")
                    .arg(data.constData(), currentEncoding.constData()));
            }

            std::string encodedText{pDataEncoder->fromUnicode(dataQString).toStdString()};
            host.mpConsole->printOnDisplay(encodedText);
            lua_pushboolean(L, true);
            return 1;
        }

        // else plain, raw ASCII, we hope!
        for (const QChar c : dataQString) {
            if (c.row() || c.cell() > 127) {
                return warnArgumentValue(L, __func__, qsl(
                    "cannot send '%1' as it contains one or more characters that cannot be conveyed in the current game server encoding of 'ASCII'")
                    .arg(data.constData()));
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

// No documentation available in wiki - internal helper
// to get ID of stopwatch from either a (numeric) ID argument or a (string) name
// - used to refactor the same code out of four separate stop-watch functions:
std::tuple<bool, int> TLuaInterpreter::getWatchId(lua_State* L, Host& h)
{
    if (lua_type(L, 1) == LUA_TNUMBER) {
        return {true, static_cast<int>(lua_tointeger(L, 1))};
    }

    const QString name{lua_tostring(L, 1)};
    // Using an empty string will return the first unnamed stopwatch:
    const int watchId = h.findStopWatchId(name);
    if (!watchId) {
        lua_pushnil(L);
        if (name.isEmpty()) {
            lua_pushstring(L, "no unnamed stopwatches found");
        } else {
            lua_pushfstring(L, "stopwatch with name '%s' not found", name.toUtf8().constData());
        }
        return {false, 0};
    }

    return {true, watchId};
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#holdingModifiers
int TLuaInterpreter::holdingModifiers(lua_State* L)
{
    Qt::KeyboardModifiers keyModifiers;
    keyModifiers = static_cast<Qt::KeyboardModifiers>(
        getVerifiedInt(L, __func__, 1, "key modifier", true)
    );
    Qt::KeyboardModifiers modifiersHeld = QGuiApplication::queryKeyboardModifiers();
    lua_pushboolean(L, modifiersHeld == keyModifiers);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableCommandLine
// This (and the next) function originally only worked on TConsole instances
// to show/hide a command line at the bottom (and the first would create the
// TCommandLine if needed) but they have been extended to also work on extra
// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getModulePath
int TLuaInterpreter::getModulePath(lua_State* L)
{
    const QString moduleName = getVerifiedString(L, __func__, 1, "module name");
    const Host& host = getHostFromLua(L);
    QMap<QString, QStringList> modules = host.mInstalledModules;
    if (modules.contains(moduleName)) {
        const QString modPath = modules[moduleName][0];
        lua_pushstring(L, modPath.toUtf8().constData());
        return 1;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getModulePriority
int TLuaInterpreter::getModulePriority(lua_State* L)
{
    const QString moduleName = getVerifiedString(L, __func__, 1, "module name");
    Host& host = getHostFromLua(L);
    if (host.mModulePriorities.contains(moduleName)) {
        const int priority = host.mModulePriorities[moduleName];
        lua_pushnumber(L, priority);
        return 1;
    } else {
        lua_pushstring(L, "getModulePriority: module doesn't exist");
        return lua_error(L);
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setModulePriority
int TLuaInterpreter::setModulePriority(lua_State* L)
{
    const QString moduleName = getVerifiedString(L, __func__, 1, "module name");
    const int modulePriority = getVerifiedInt(L, __func__, 2, "module priority");

    Host& host = getHostFromLua(L);
    if (!host.mInstalledModules.contains(moduleName)) {
        return warnArgumentValue(L, __func__, "module doesn't exist");
    }
    host.mModulePriorities[moduleName] = modulePriority;
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#closeMudlet
int TLuaInterpreter::closeMudlet(lua_State* L)
{
    Q_UNUSED(L)
    mudlet::self()->armForceClose();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#saveProfile
int TLuaInterpreter::saveProfile(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString saveToDir;
    if (lua_isstring(L, 1)) {
        saveToDir = lua_tostring(L, 1);
    }
    QString saveAsFile;
    if (!lua_isnoneornil(L, 2)) {
        saveAsFile = getVerifiedString(L, __func__, 2, "file name", true);
        if (!saveAsFile.endsWith(".xml", Qt::CaseInsensitive)) {
            saveAsFile = saveAsFile + ".xml";
        }
    }

    auto [ok, filename, error] = (saveAsFile.isNull())
                               ? host.saveProfile(saveToDir)
                               : host.saveProfileAs(saveToDir + "/" + saveAsFile);

    if (ok) {
        lua_pushboolean(L, true);
        lua_pushstring(L, (filename.toUtf8().constData()));
        return 2;
    } else {
        auto message = QString("Couldn't save '%1' to '%2' because: %3").arg(host.getName(), filename, error);
        return warnArgumentValue(L, __func__, message);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMudletInfo
int TLuaInterpreter::getMudletInfo(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QStringList knownEncodings{"ASCII"};
    // cTelnet::getEncoding() returns a QByteArray NOT a QString:
    QString currentEncoding{host.mTelnet.getEncoding()};
    {
        auto adjustEncoding = [](auto encodingName) {
            auto originalEncoding = encodingName;
            if (encodingName.startsWith(qsl("M_"))) {
                encodingName.remove(0, 2);
            }

            return (originalEncoding == encodingName) ? originalEncoding : qsl("%1 (%2)").arg(encodingName).arg(originalEncoding);
        };
        // cTelnet::getEncodingsList() returns a QByteArrayList NOT a QStringList/QList<QString>:
        for (const auto& encoding : host.mTelnet.getEncodingsList()) {
            knownEncodings.append(adjustEncoding(QString(encoding)));
        }
        QCollator sorter;
        sorter.setNumericMode(true);
        sorter.setCaseSensitivity(Qt::CaseInsensitive);
        std::sort(knownEncodings.begin(), knownEncodings.end(), sorter);

        if (currentEncoding.isEmpty()) {
            currentEncoding = qsl("\"ASCII\"");
        } else {
            currentEncoding = adjustEncoding(currentEncoding);
        }
    }

    host.postMessage(qsl("[ INFO ]  - Current encoding: %1").arg(currentEncoding));

    host.postMessage(qsl("[ INFO ]  - Available encodings:"));
    host.postMessage(qsl("  %1").arg(knownEncodings.join(qsl(", "))));

    return 0;
}

// Internal Function createLabel in an UserWindow
int TLuaInterpreter::createLabelUserWindow(lua_State* L, const QString& windowName, const QString& labelName)
{
    const int n = lua_gettop(L);
    const int x = getVerifiedInt(L, "createLabel", 3, "label x-coordinate");
    const int y = getVerifiedInt(L, "createLabel", 4, "label y-coordinate");
    const int width = getVerifiedInt(L, "createLabel", 5, "label width");
    const int height = getVerifiedInt(L, "createLabel", 6, "label height");

    bool fillBackground = false;
    if ((!lua_isnumber(L, 7)) && (!lua_isboolean(L, 7))) {
        lua_pushfstring(L, "createLabel: bad argument #7 type (label fillBackground as boolean/number (0/1) expected, got %s!)", luaL_typename(L, 7));
        return lua_error(L);
    }
    if (lua_isboolean(L, 7)) {
        fillBackground = lua_toboolean(L, 7);
    } else {
        fillBackground = (lua_tointeger(L, 7) != 0);
    }

    bool clickthrough = false;
    if (n >= 8) {
        if ((!lua_isnumber(L, 8)) && (!lua_isboolean(L, 8))) {
            lua_pushfstring(L, "createLabel: bad argument #8 type (label clickthrough as boolean/number (0/1) expected, got %s!)", luaL_typename(L, 8));
            return lua_error(L);
        }
        if (lua_isboolean(L, 8)) {
            clickthrough = lua_toboolean(L, 8);
        } else {
            clickthrough = (lua_tointeger(L, 8) != 0);
        }
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.createLabel(windowName, labelName, x, y, width, height, fillBackground, clickthrough); !success) {
        // We should, perhaps be returning a nil here but the published API
        // says the function returns true or false and we cannot change that now
        return warnArgumentValue(L, "createLabel", message, true);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Internal Function create Label in MainWindow
int TLuaInterpreter::createLabelMainWindow(lua_State* L, const QString& labelName)
{
    const QString windowName = QLatin1String("main");
    const int n = lua_gettop(L);
    const int x = getVerifiedInt(L, "createLabel", 2, "label x-coordinate");
    const int y = getVerifiedInt(L, "createLabel", 3, "label y-coordinate");
    const int width = getVerifiedInt(L, "createLabel", 4, "label width");
    const int height = getVerifiedInt(L, "createLabel", 5, "label height");

    bool fillBackground = false;
    if ((!lua_isnumber(L, 6)) && (!lua_isboolean(L, 6))) {
        lua_pushfstring(L, "createLabel: bad argument #6 type (label fillBackground as boolean/number (0/1) expected, got %s!)", luaL_typename(L, 6));
        return lua_error(L);
    }
    if (lua_isboolean(L, 6)) {
        fillBackground = lua_toboolean(L, 6);
    } else {
        fillBackground = (lua_tointeger(L, 6) != 0);
    }

    bool clickthrough = false;
    if (n >= 7) {
        if ((!lua_isnumber(L, 7)) && (!lua_isboolean(L, 7))) {
            lua_pushfstring(L, "createLabel: bad argument #7 type (label clickthrough as boolean/number (0/1) expected, got %s!)", luaL_typename(L, 7));
            return lua_error(L);
        }
        if (lua_isboolean(L, 7)) {
            clickthrough = lua_toboolean(L, 7);
        } else {
            clickthrough = (lua_tointeger(L, 7) != 0);
        }
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.createLabel(windowName, labelName, x, y, width, height, fillBackground, clickthrough); !success) {
        // We should, perhaps be returning a nil here but the published API
        // says the function returns true or false and we cannot change that now
        return warnArgumentValue(L, "createLabel", message, true);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: ? - public function but should stay undocumented -- compare https://github.com/Mudlet/Mudlet/issues/1149
int TLuaInterpreter::closeUserWindow(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    host.closeWindow(text);
    return 0;
}


// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#startLogging
int TLuaInterpreter::startLogging(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    const bool logOn = getVerifiedBool(L, __func__, 1, "turn logging on/off");

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
            lua_pushfstring(L, "Main console output has started to be logged to file: %s", host.mpConsole->mLogFileName.toUtf8().constData());
            lua_pushstring(L, host.mpConsole->mLogFileName.toUtf8().constData());
            lua_pushnumber(L, 1);
        } else {
            host.mpConsole->logButton->setChecked(false);
            lua_pushfstring(L, "Main console output has stopped being logged to file: %s", savedLogFileName.toUtf8().constData());
            lua_pushstring(L, host.mpConsole->mLogFileName.toUtf8().constData());
            lua_pushnumber(L, 0);
        }

    } else {
        lua_pushnil(L);
        if (host.mpConsole->mLogToLogFile) {
            lua_pushfstring(L, "Main console output is already being logged to file: %s", host.mpConsole->mLogFileName.toUtf8().constData());
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

int TLuaInterpreter::appendLog(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "text to append to logfile", true);

    const Host& host = getHostFromLua(L);

    host.mpConsole->buffer.appendLog(text);

    return 0;
}


// No documentation available in wiki - internal function
int TLuaInterpreter::setLabelCallback(lua_State* L, const QString& funcName)
{
    Host& host = getHostFromLua(L);
    const QString labelName = getVerifiedString(L, funcName.toUtf8().constData(), 1, "label name");
    if (labelName.isEmpty()) {
        return warnArgumentValue(L, __func__, "label name cannot be an empty string");
    }
    lua_remove(L, 1);

    if (!lua_isfunction(L, 1)) {
        lua_pushfstring(L, "%s: bad argument #2 type (function expected, got %s!)", funcName.toUtf8().constData(), luaL_typename(L, 1));
        return lua_error(L);
    }
    const int func = luaL_ref(L, LUA_REGISTRYINDEX);

    bool lua_result = false;
    if (funcName == qsl("setLabelClickCallback")) {
        lua_result = host.setLabelClickCallback(labelName, func);
    } else if (funcName == qsl("setLabelDoubleClickCallback")) {
        lua_result = host.setLabelDoubleClickCallback(labelName, func);
    } else if (funcName == qsl("setLabelReleaseCallback")) {
        lua_result = host.setLabelReleaseCallback(labelName, func);
    } else if (funcName == qsl("setLabelMoveCallback")) {
        lua_result = host.setLabelMoveCallback(labelName, func);
    } else if (funcName == qsl("setLabelWheelCallback")) {
        lua_result = host.setLabelWheelCallback(labelName, func);
    } else if (funcName == qsl("setLabelOnEnter")) {
        lua_result = host.setLabelOnEnter(labelName, func);
    } else if (funcName == qsl("setLabelOnLeave")) {
        lua_result = host.setLabelOnLeave(labelName, func);
    } else {
        return warnArgumentValue(L, __func__, qsl("'%1' is not a known function name - bug in Mudlet, please report it").arg(funcName));
    }

    if (!lua_result) {
        return warnArgumentValue(L, __func__, qsl("label name '%1' not found").arg(labelName));
    }
    lua_pushboolean(L, true);
    return 1;
}

// No documentation available in wiki - internal function
// this was an internal only function used by the package system, but it was
// inactive and has been removed
int TLuaInterpreter::showUnzipProgress(lua_State* L)
{
    return warnArgumentValue(L, __func__, "removed command, this function is now inactive and does nothing");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMudletHomeDir
int TLuaInterpreter::getMudletHomeDir(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const QString nativeHomeDirectory = mudlet::getMudletPath(enums::profileHomePath, host.getName());
    lua_pushstring(L, nativeHomeDirectory.toUtf8().constData());
    return 1;
}

// No documentation available in wiki - internal function used by printError in DebugTools.lua
int TLuaInterpreter::errorc(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const int n = lua_gettop(L);
    if (!n) {
        // Nothing to show
        return 0;
    }
    QString luaErrorText;
    QString luaFunctionInfo;
    luaErrorText = qsl(" %1").arg(lua_tostring(L, 1));
    if (n == 2) {
        luaFunctionInfo = qsl("%1").arg(lua_tostring(L, 2));
    } else {
        luaFunctionInfo = qsl(" <no debug data available> ");
    }
    luaFunctionInfo.append(QChar::LineFeed);
    luaErrorText.append(QChar::LineFeed);
    if (host.mpEditorDialog) {
        host.mpEditorDialog->mpErrorConsole->print(QLatin1String("[ERROR:] "), QColor(Qt::blue), QColor(Qt::black));
        host.mpEditorDialog->mpErrorConsole->print(luaFunctionInfo, QColor(Qt::green), QColor(Qt::black));
        host.mpEditorDialog->mpErrorConsole->print(qsl("         %1").arg(luaErrorText), QColor(Qt::red), QColor(Qt::black));
    }

    if (host.mEchoLuaErrors) {
        if (!host.mpConsole->buffer.isEmpty() && !host.mpConsole->buffer.lineBuffer.at(host.mpConsole->buffer.lineBuffer.size() - 1).isEmpty()) {
            host.postMessage(qsl("\n"));
        }
        host.mpConsole->print(qsl("[  LUA  ] - "), QColor(80,160,255), QColor(Qt::black));
        host.mpConsole->print(qsl("ERROR: "), QColor(Qt::blue), QColor(Qt::black));
        host.mpConsole->print(qsl("%1").arg(luaFunctionInfo), QColor(Qt::green), QColor(Qt::black));
        host.mpConsole->print(qsl("           %1").arg(luaErrorText), QColor(200,50,42), QColor(Qt::black));
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#debugc -- not #debug - compare GlobalLua
int TLuaInterpreter::debug(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    const int n = lua_gettop(L);
    if (!n) {
        // Nothing to show
        return 0;
    }

    QString luaDebugText;
    if (n > 1) {
        for (int i = 0; i < n; ++i) {
            luaDebugText += qsl(" (%1) %2").arg(QString::number(i + 1), lua_tostring(L, i + 1));
        }
    } else {
        // n == 1
        luaDebugText = qsl(" %1").arg(lua_tostring(L, 1));
    }
    luaDebugText.append(QChar::LineFeed);

    if (host.mpEditorDialog) {
        host.mpEditorDialog->mpErrorConsole->print(QLatin1String("[DEBUG:]"), QColor(Qt::blue), QColor(Qt::black));
        host.mpEditorDialog->mpErrorConsole->print(luaDebugText, QColor(Qt::green), QColor(Qt::black));
    }

    return 0;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::showHandlerError(lua_State* L)
{
    Host& host = getHostFromLua(L);
    const QString event = getVerifiedString(L, __func__, 1, "event name");
    const QString error = getVerifiedString(L, __func__, 2, "error message");
    host.mLuaInterpreter.logEventError(event, error);
    return 0;
}

// Internal helper function for two following functions:
// returns 0 and a pointer to the TAction with the ID or
//   the first one found with the name as the index item on success.
// returns a non-zero number and a nullptr on failure (the first is the number
//   of items on the stack for the caller to return to ITS caller).
// does NOT return (normally) if the index item on the stack is not a number or
//   a string
std::pair<int, TAction*> TLuaInterpreter::getTActionFromIdOrName(lua_State* L, const int index, const char* func)
{
    auto& host = getHostFromLua(L);
    auto argType = lua_type(L, index);
    TAction* pItem = nullptr;
    if (argType == LUA_TNUMBER) {
        const int id = qRound(lua_tonumber(L, index));
        if (id < 0) {
            return {warnArgumentValue(L, func, qsl("item ID (%1) invalid, it must be equal or greater than zero").arg(id).toUtf8().constData()), pItem};
        }
        pItem = host.getActionUnit()->getAction(id);
        if (!pItem) {
            return {warnArgumentValue(L, func, qsl("no button item with ID %1 found").arg(id).toUtf8().constData()), pItem};
        }
        if (!pItem->isPushDownButton()) {
            pItem = nullptr;
            return {warnArgumentValue(L, func, qsl("item ID with %1 is not a push-down button").arg(id).toUtf8().constData()), pItem};
        }
    }

    if (argType == LUA_TSTRING) {
        const QString name = lua_tostring(L, index);
        if (name.isEmpty()) {
            return {warnArgumentValue(L, func, "item name must not be an empty string"), pItem};
        }
        pItem = host.getActionUnit()->findAction(name);
        if (!pItem) {
            return {warnArgumentValue(L, func, qsl("no button item with name '%1' found").arg(name).toUtf8().constData()), pItem};
        }
        if (!pItem->isPushDownButton()) {
            pItem = nullptr;
            return {warnArgumentValue(L, func, qsl("item with name '%1' is not a push-down button").arg(name).toUtf8().constData()), pItem};
        }
    }

    if (!pItem) {
        // we'll get here if the (index) argument is NOT usable:
        lua_pushfstring(L, "%s: bad argument #%d type (ID as number or name as string expected, got %s!)",
                        func, index, luaL_typename(L, index));
        lua_error(L); // Does not return!
        Q_UNREACHABLE();
    }

    return {0, pItem};
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempColorTrigger
// This is documented as using a simple color table - but the numbers do not
// Documentation: https://wiki.mudlet.org/w/Manual:Mudlet_Object_Functions#tempAnsiColorTrigger
// This is the replacement for tempColorTrigger() which uses the right numbers
// for ANSI colours in the range 0 to 255 or TTrigger::scmDefault for default
// colour or TTrigger::scmIgnored ignore; it is anticipated that additional
// special values less than zero may be added to detect other types of text (or
// for a 16M colour value where the components have to be given)
// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#findItems
int TLuaInterpreter::findItems(lua_State* L)
{
    const int n = lua_gettop(L);
    const auto name = getVerifiedString(L, __func__, 1, "item name");
    // Although we only use 6 ASCII strings the user may not enter a purely
    // ASCII value which we might have to report...
    const QString type = getVerifiedString(L, __func__, 2, "item type");
    bool exactMatch = true;
    bool caseSensitive = true;
    if (n > 2) {
        exactMatch = getVerifiedBool(L, __func__, 3, "exact match", true);
    }
    if (n > 3) {
        caseSensitive = getVerifiedBool(L, __func__, 4, "case sensitive", true);
    }
    Host& host = getHostFromLua(L);
    auto generateList = [](const auto vector, auto l) {
        lua_newtable(l);
        int index = 0;
        for (const auto& item : vector) {
            lua_pushnumber(l, ++index);
            lua_pushnumber(l, item);
            lua_settable(l, -3);
        }
    };
    if (!type.compare(QLatin1String("timer"), Qt::CaseInsensitive)) {
        const auto itemList = host.getTimerUnit()->findItems(name, exactMatch, caseSensitive);
        generateList(itemList, L);
        return 1;
    }
    if (!type.compare(QLatin1String("trigger"), Qt::CaseInsensitive)) {
        const auto itemList = host.getTriggerUnit()->findItems(name, exactMatch, caseSensitive);
        generateList(itemList, L);
        return 1;
    }
    if (!type.compare(QLatin1String("alias"), Qt::CaseInsensitive)) {
        const auto itemList = host.getAliasUnit()->findItems(name, exactMatch, caseSensitive);
        generateList(itemList, L);
        return 1;
    }
    if (!type.compare(QLatin1String("keybind"), Qt::CaseInsensitive)) {
        const auto itemList = host.getKeyUnit()->findItems(name, exactMatch, caseSensitive);
        generateList(itemList, L);
        return 1;
    }
    if (!type.compare(QLatin1String("button"), Qt::CaseInsensitive)) {
        const auto itemList = host.getActionUnit()->findItems(name, exactMatch, caseSensitive);
        generateList(itemList, L);
        return 1;
    }
    if (!type.compare(QLatin1String("script"), Qt::CaseInsensitive)) {
        const auto itemList = host.getScriptUnit()->findItems(name, exactMatch, caseSensitive);
        generateList(itemList, L);
        return 1;
    }
    return warnArgumentValue(L, __func__, qsl("invalid item type '%1' given, it should be one of: 'alias', 'button', 'script', 'keybind', 'timer' or 'trigger'").arg(type));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isAncestorsActive
int TLuaInterpreter::isAncestorsActive(lua_State* L)
{
    auto id = getVerifiedInt(L, __func__, 1, "item ID");
    // Although we only use ASCII strings for the type the user may not enter a
    // purely ASCII value which we might have to report...
    QString type = getVerifiedString(L, __func__, 2, "item type");
    if (id < 0) {
        // Must be zero or more but doesn't seem to be:
        return warnArgumentValue(L, __func__, qsl("item ID as %1 does not seem to be parseable as a positive integer").arg(lua_tostring(L, 1)));
    }

    Host& host = getHostFromLua(L);
    // Remember, QString::compare(...) returns zero for a match:
    QString typeCheck{QLatin1String("timer")};
    if (!type.compare(typeCheck, Qt::CaseInsensitive)) {
        auto pT = host.getTimerUnit()->getTimer(id);
        if (!pT) {
            return warnArgumentValue(L, __func__, qsl("%1 item ID %2 does not exist").arg(typeCheck, QString::number(id)));
        }

        // Offset timer have their active state recorded differently
        lua_pushboolean(L, pT->isOffsetTimer() ? pT->shouldAncestorsBeActive() : pT->ancestorsActive());
        return 1;
    }

    typeCheck = QLatin1String("trigger");
    if (!type.compare(typeCheck, Qt::CaseInsensitive)) {
        auto pT = host.getTriggerUnit()->getTrigger(id);
        if (!pT) {
            return warnArgumentValue(L, __func__, qsl("%1 item ID %2 does not exist").arg(typeCheck, QString::number(id)));
        }
        lua_pushboolean(L, pT->ancestorsActive());
        return 1;
    }

    typeCheck = QLatin1String("alias");
    if (!type.compare(typeCheck, Qt::CaseInsensitive)) {
        auto pT = host.getAliasUnit()->getAlias(id);
        if (!pT) {
            return warnArgumentValue(L, __func__, qsl("%1 item ID %2 does not exist").arg(typeCheck, QString::number(id)));
        }
        lua_pushboolean(L, pT->ancestorsActive());
        return 1;
    }

    typeCheck = QLatin1String("keybind");
    if (!type.compare(typeCheck, Qt::CaseInsensitive)) {
        auto pT = host.getKeyUnit()->getKey(id);
        if (!pT) {
            return warnArgumentValue(L, __func__, qsl("%1 item ID %2 does not exist").arg(typeCheck, QString::number(id)));
        }
        lua_pushboolean(L, pT->ancestorsActive());
        return 1;
    }

    typeCheck = QLatin1String("button");
    if (!type.compare(typeCheck, Qt::CaseInsensitive)) {
        auto pT = host.getActionUnit()->getAction(id);
        if (!pT) {
            return warnArgumentValue(L, __func__, qsl("%1 item ID %2 does not exist").arg(typeCheck, QString::number(id)));
        }
        lua_pushboolean(L, pT->ancestorsActive());
        return 1;
    }

    typeCheck = QLatin1String("script");
    if (!type.compare(typeCheck, Qt::CaseInsensitive)) {
        auto pT = host.getScriptUnit()->getScript(id);
        if (!pT) {
            return warnArgumentValue(L, __func__, qsl("%1 item ID %2 does not exist").arg(typeCheck, QString::number(id)));
        }
        lua_pushboolean(L, pT->ancestorsActive());
        return 1;
    }

    return warnArgumentValue(L, __func__, qsl("invalid item type '%1' given, it should be one (case insensitive) of: 'alias', 'button', 'script', 'keybind', 'timer' or 'trigger'").arg(type));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ancestors
int TLuaInterpreter::ancestors(lua_State* L)
{
    auto id = getVerifiedInt(L, __func__, 1, "item ID");
    // Although we only use ASCII strings for the type the user may not enter a
    // purely ASCII value which we might have to report...
    QString type = getVerifiedString(L, __func__, 2, "item type");
    if (id < 0) {
        // Must be zero or more but doesn't seem to be:
        return warnArgumentValue(L, __func__, qsl("item ID as %1 does not seem to be parseable as a positive integer").arg(lua_tostring(L, 1)));
    }

    Host& host = getHostFromLua(L);
    // Remember, QString::compare(...) returns zero for a match:
    QString typeCheck{QLatin1String("timer")};
    if (!type.compare(typeCheck, Qt::CaseInsensitive)) {
        auto pT = host.getTimerUnit()->getTimer(id);
        if (!pT) {
            return warnArgumentValue(L, __func__, qsl("%1 item ID %2 does not exist").arg(typeCheck, QString::number(id)));
        }
        const auto ancestorsList = pT->getAncestorList();
        lua_newtable(L);
        int index = 0;
        for (const auto pAncestor : ancestorsList) {
            if (!pAncestor) {
                // Uh oh! This is not expected, so clear that table off the
                // stack so we can push an error message there:
                lua_pop(L, 1);
                lua_pushfstring(L, "%s: internal error, got a nullptr whilst looking for an ancestor of the %s with ID: %i", __func__, typeCheck.toLatin1().constData(), id);
                lua_error(L);
                Q_UNREACHABLE();
            }
            lua_pushnumber(L, ++index);
            lua_newtable(L);
            {
                // We are confining ourselves to a small set of details here
                // enough to help to build a table of the items perhaps but
                // something to provide more details about each of the diffent
                // item types (once the user knows which IDs/names to use to
                // get them) would probably be a good idea as well:
                lua_pushstring(L, "id");
                lua_pushnumber(L, pAncestor->getID());
                lua_settable(L, -3);

                lua_pushstring(L, "name");
                lua_pushstring(L, pAncestor->getName().toUtf8().constData());
                lua_settable(L, -3);

                lua_pushstring(L, "node");
                if (pAncestor->isFolder()) {
                    if (!pAncestor->mPackageName.isEmpty() && pAncestor->mPackageName == pAncestor->getName()) {
                        lua_pushstring(L, "package");
                    } else {
                        lua_pushstring(L, "group");
                    }
                } else {
                    // offset timers have a parent node that is NOT a group!
                    lua_pushstring(L, "item");
                }
                lua_settable(L, -3);

                lua_pushstring(L, "isActive");
                // Offset timer have their active state recorded differently
                lua_pushboolean(L, pAncestor->isOffsetTimer() ? pAncestor->shouldBeActive() : pAncestor->isActive());
                lua_settable(L, -3);
            }
            lua_settable(L, -3);
        }

        return 1;
    }

    typeCheck = QLatin1String("trigger");
    if (!type.compare(typeCheck, Qt::CaseInsensitive)) {
        auto pT = host.getTriggerUnit()->getTrigger(id);
        if (!pT) {
            return warnArgumentValue(L, __func__, qsl("%1 item ID %2 does not exist").arg(typeCheck, QString::number(id)));
        }
        const auto ancestorsList = pT->getAncestorList();
        lua_newtable(L);
        int index = 0;
        for (const auto pAncestor : ancestorsList) {
            if (!pAncestor) {
                // Uh oh! This is not expected, so clear that table off the
                // stack so we can push an error message there:
                lua_pop(L, 1);
                lua_pushfstring(L, "%s: internal error, got a nullptr whilst looking for an ancestor of the %s with ID: %i", __func__, typeCheck.toLatin1().constData(), id);
                lua_error(L);
                Q_UNREACHABLE();
            }
            lua_pushnumber(L, ++index);
            lua_newtable(L);
            {
                lua_pushstring(L, "id");
                lua_pushnumber(L, pAncestor->getID());
                lua_settable(L, -3);

                lua_pushstring(L, "name");
                lua_pushstring(L, pAncestor->getName().toUtf8().constData());
                lua_settable(L, -3);

                lua_pushstring(L, "node");
                if (pAncestor->isFolder()) {
                    if (!pAncestor->mPackageName.isEmpty() && pAncestor->mPackageName == pAncestor->getName()) {
                        lua_pushstring(L, "package");
                    } else {
                        lua_pushstring(L, "group");
                    }
                } else {
                    lua_pushstring(L, "item");
                }
                lua_settable(L, -3);

                lua_pushstring(L, "isActive");
                lua_pushboolean(L, pAncestor->isActive());
                lua_settable(L, -3);
            }
            lua_settable(L, -3);
        }

        return 1;
    }

    typeCheck = QLatin1String("alias");
    if (!type.compare(typeCheck, Qt::CaseInsensitive)) {
        auto pT = host.getAliasUnit()->getAlias(id);
        if (!pT) {
            return warnArgumentValue(L, __func__, qsl("%1 item ID %2 does not exist").arg(typeCheck, QString::number(id)));
        }
        const auto ancestorsList = pT->getAncestorList();
        lua_newtable(L);
        int index = 0;
        for (const auto pAncestor : ancestorsList) {
            if (!pAncestor) {
                // Uh oh! This is not expected, so clear that table off the
                // stack so we can push an error message there:
                lua_pop(L, 1);
                lua_pushfstring(L, "%s: internal error, got a nullptr whilst looking for an ancestor of the %s with ID: %i", __func__, typeCheck.toLatin1().constData(), id);
                lua_error(L);
                Q_UNREACHABLE();
            }
            lua_pushnumber(L, ++index);
            lua_newtable(L);
            {
                lua_pushstring(L, "id");
                lua_pushnumber(L, pAncestor->getID());
                lua_settable(L, -3);

                lua_pushstring(L, "name");
                lua_pushstring(L, pAncestor->getName().toUtf8().constData());
                lua_settable(L, -3);

                lua_pushstring(L, "node");
                if (pAncestor->isFolder()) {
                    if (!pAncestor->mPackageName.isEmpty() && pAncestor->mPackageName == pAncestor->getName()) {
                        lua_pushstring(L, "package");
                    } else {
                        lua_pushstring(L, "group");
                    }
                } else {
                    lua_pushstring(L, "item");
                }
                lua_settable(L, -3);

                lua_pushstring(L, "isActive");
                lua_pushboolean(L, pAncestor->isActive());
                lua_settable(L, -3);
            }
            lua_settable(L, -3);
        }

        return 1;
    }

    typeCheck = QLatin1String("keybind");
    if (!type.compare(typeCheck, Qt::CaseInsensitive)) {
        auto pT = host.getKeyUnit()->getKey(id);
        if (!pT) {
            return warnArgumentValue(L, __func__, qsl("%1 item ID %2 does not exist").arg(typeCheck, QString::number(id)));
        }
        const auto ancestorsList = pT->getAncestorList();
        lua_newtable(L);
        int index = 0;
        for (const auto pAncestor : ancestorsList) {
            if (!pAncestor) {
                // Uh oh! This is not expected, so clear that table off the
                // stack so we can push an error message there:
                lua_pop(L, 1);
                lua_pushfstring(L, "%s: internal error, got a nullptr whilst looking for an ancestor of the %s with ID: %i", __func__, typeCheck.toLatin1().constData(), id);
                lua_error(L);
                Q_UNREACHABLE();
            }
            lua_pushnumber(L, ++index);
            lua_newtable(L);
            {
                lua_pushstring(L, "id");
                lua_pushnumber(L, pAncestor->getID());
                lua_settable(L, -3);

                lua_pushstring(L, "name");
                lua_pushstring(L, pAncestor->getName().toUtf8().constData());
                lua_settable(L, -3);

                lua_pushstring(L, "node");
                if (pAncestor->isFolder()) {
                    if (!pAncestor->mPackageName.isEmpty() && pAncestor->mPackageName == pAncestor->getName()) {
                        lua_pushstring(L, "package");
                    } else {
                        lua_pushstring(L, "group");
                    }
                } else {
                    lua_pushstring(L, "item");
                }
                lua_settable(L, -3);

                lua_pushstring(L, "isActive");
                lua_pushboolean(L, pAncestor->isActive());
                lua_settable(L, -3);
            }
            lua_settable(L, -3);
        }

        return 1;
    }

    typeCheck = QLatin1String("button");
    if (!type.compare(typeCheck, Qt::CaseInsensitive)) {
        auto pT = host.getActionUnit()->getAction(id);
        if (!pT) {
            return warnArgumentValue(L, __func__, qsl("%1 item ID %2 does not exist").arg(typeCheck, QString::number(id)));
        }
        const auto ancestorsList = pT->getAncestorList();
        lua_newtable(L);
        int index = 0;
        for (const auto pAncestor : ancestorsList) {
            if (!pAncestor) {
                // Uh oh! This is not expected, so clear that table off the
                // stack so we can push an error message there:
                lua_pop(L, 1);
                lua_pushfstring(L, "%s: internal error, got a nullptr whilst looking for an ancestor of the %s with ID: %i", __func__, typeCheck.toLatin1().constData(), id);
                lua_error(L);
                Q_UNREACHABLE();
            }
            lua_pushnumber(L, ++index);
            lua_newtable(L);
            {
                lua_pushstring(L, "id");
                lua_pushnumber(L, pAncestor->getID());
                lua_settable(L, -3);

                lua_pushstring(L, "name");
                lua_pushstring(L, pAncestor->getName().toUtf8().constData());
                lua_settable(L, -3);

                lua_pushstring(L, "node");
                if (pAncestor->isFolder()) {
                    if (!pAncestor->mPackageName.isEmpty() && pAncestor->mPackageName == pAncestor->getName()) {
                        lua_pushstring(L, "package");
                    } else {
                        lua_pushstring(L, "group");
                    }
                } else {
                    lua_pushstring(L, "item");
                }
                lua_settable(L, -3);

                lua_pushstring(L, "isActive");
                lua_pushboolean(L, pAncestor->isActive());
                lua_settable(L, -3);
            }
            lua_settable(L, -3);
        }

        return 1;
    }

    typeCheck = QLatin1String("script");
    if (!type.compare(typeCheck, Qt::CaseInsensitive)) {
        auto pT = host.getScriptUnit()->getScript(id);
        if (!pT) {
            return warnArgumentValue(L, __func__, qsl("%1 item ID %2 does not exist").arg(typeCheck, QString::number(id)));
        }
        const auto ancestorsList = pT->getAncestorList();
        lua_newtable(L);
        int index = 0;
        for (const auto pAncestor : ancestorsList) {
            if (!pAncestor) {
                // Uh oh! This is not expected, so clear that table off the
                // stack so we can push an error message there:
                lua_pop(L, 1);
                lua_pushfstring(L, "%s: internal error, got a nullptr whilst looking for an ancestor of the %s with ID: %i", __func__, typeCheck.toLatin1().constData(), id);
                lua_error(L);
                Q_UNREACHABLE();
            }
            lua_pushnumber(L, ++index);
            lua_newtable(L);
            {
                lua_pushstring(L, "id");
                lua_pushnumber(L, pAncestor->getID());
                lua_settable(L, -3);

                lua_pushstring(L, "name");
                lua_pushstring(L, pAncestor->getName().toUtf8().constData());
                lua_settable(L, -3);

                lua_pushstring(L, "node");
                if (pAncestor->isFolder()) {
                    if (!pAncestor->mPackageName.isEmpty() && pAncestor->mPackageName == pAncestor->getName()) {
                        lua_pushstring(L, "package");
                    } else {
                        lua_pushstring(L, "group");
                    }
                } else {
                    lua_pushstring(L, "item");
                }
                lua_settable(L, -3);

                lua_pushstring(L, "isActive");
                lua_pushboolean(L, pAncestor->isActive());
                lua_settable(L, -3);
            }
            lua_settable(L, -3);
        }

        return 1;
    }

    return warnArgumentValue(L, __func__, qsl("invalid item type '%1' given, it should be one (case insensitive) of: 'alias', 'button', 'script', 'keybind', 'timer' or 'trigger'").arg(type));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getTimestamp
int TLuaInterpreter::getTimestamp(lua_State* L)
{
    const int n = lua_gettop(L);
    int s = 1;
    QString name;
    if (n > 1) {
        name = getVerifiedString(L, __func__, s++, "mini console, user window or buffer name {may be omitted for the \"main\" console}");
        if (name == QLatin1String("main")) {
            // clear it so it is treated as the main console below
            name.clear();
        }
    }

    qint64 const luaLine = getVerifiedInt(L, __func__, s, "line number");
    if (luaLine < 1) {
        return warnArgumentValue(L, __func__, qsl(
            "line number %1 invalid, it should be greater than zero").arg(luaLine));
    }

    const Host& host = getHostFromLua(L);
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
        if (!pC) {
            return warnArgumentValue(L, __func__, qsl("mini console, user window or buffer '%1' not found").arg(name));
        }
        if (luaLine > 0 && luaLine < pC->buffer.timeBuffer.size()) {
            lua_pushstring(L, pC->buffer.timeBuffer.at(luaLine).toUtf8().constData());
        } else {
            lua_pushstring(L, "getTimestamp: invalid line number");
        }
        return 1;
    }
}


// No documentation available - internal function
void TLuaInterpreter::pushMapLabelPropertiesToLua(lua_State* L, const TMapLabel& label)
{
    lua_pushstring(L, "X");
    lua_pushnumber(L, label.pos.x());
    lua_settable(L, -3);
    lua_pushstring(L, "Y");
    lua_pushnumber(L, label.pos.y());
    lua_settable(L, -3);
    lua_pushstring(L, "Z");
    lua_pushnumber(L, qRound(label.pos.z()));
    lua_settable(L, -3);
    lua_pushstring(L, "Height");
    lua_pushnumber(L, label.size.height());
    lua_settable(L, -3);
    lua_pushstring(L, "Width");
    lua_pushnumber(L, label.size.width());
    lua_settable(L, -3);
    lua_pushstring(L, "Text");
    lua_pushstring(L, label.text.toUtf8().constData());
    lua_settable(L, -3);
    lua_pushstring(L, "Pixmap");
    lua_pushstring(L, label.base64EncodePixmap().constData());
    lua_settable(L, -3);
    lua_pushstring(L, "OnTop");
    lua_pushboolean(L, label.showOnTop);
    lua_settable(L, -3);
    lua_pushstring(L, "Scaling");
    lua_pushboolean(L, !label.noScaling);
    lua_settable(L, -3);
    lua_pushstring(L, "Temporary");
    lua_pushboolean(L, label.temporary);
    lua_settable(L, -3);

    lua_pushstring(L, "FgColor");
    {
        lua_newtable(L);
        lua_pushstring(L, "r");
        lua_pushinteger(L, label.fgColor.red());
        lua_settable(L, -3);
        lua_pushstring(L, "g");
        lua_pushinteger(L, label.fgColor.green());
        lua_settable(L, -3);
        lua_pushstring(L, "b");
        lua_pushinteger(L, label.fgColor.blue());
        lua_settable(L, -3);
    }
    lua_settable(L, -3);

    lua_pushstring(L, "BgColor");
    {
        lua_newtable(L);
        lua_pushstring(L, "r");
        lua_pushinteger(L, label.bgColor.red());
        lua_settable(L, -3);
        lua_pushstring(L, "g");
        lua_pushinteger(L, label.bgColor.green());
        lua_settable(L, -3);
        lua_pushstring(L, "b");
        lua_pushinteger(L, label.bgColor.blue());
        lua_settable(L, -3);
    }
    lua_settable(L, -3);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getSpecialExits
// This function was slightly borked - the version in place from 2011 to 2020
// did not handle the corner case of multiple special exits that go to the same
// room, it would only show one of them at random. Each special exit was listed
// in its own table (against the key of the exit roomID) and it is a key to a
// "1" or "0" depending on whether the exit is locked or not. This was not
// The next three functions are internal helpers for use by
// (echo|insert|set)|(Link|Popup) functions
void TLuaInterpreter::parseCommandOrFunction(lua_State* lState, const char* functionName, int& index, QString& command, int& luaFunctionNumber)
{
    if (!(lua_isstring(lState, index) || lua_isfunction(lState, index))) {
        lua_pushfstring(lState, "%s: bad argument #%d type (command as string or function expected, got %s!)", functionName, index, luaL_typename(lState, index));
        lua_error(lState);
        Q_UNREACHABLE();
    }

    if (lua_isfunction(lState, index)) {
        lua_pushvalue(lState, index);
        luaFunctionNumber = luaL_ref(lState, LUA_REGISTRYINDEX);
        return;
    }
    command = lua_tostring(lState, index);
}

// No documentation available in wiki - internal function
void TLuaInterpreter::parseHintsTable(lua_State* lState, const char* functionName, int& index, QStringList& hintList)
{
    if (!lua_istable(lState, index)) {
        lua_pushfstring(lState, "%s: bad argument #%d type (%s as table expected, got %s!)", functionName, "hints", luaL_typename(lState, index));
        lua_error(lState);
        Q_UNREACHABLE();
    }

    lua_pushnil(lState);
    // Keep track of the index of the item in the table
    int subIndex = 0;
    while (lua_next(lState, index)) {
        // key at index -2 and value at index -1
        ++subIndex;
        if (!lua_isstring(lState, -1)) {
            lua_pushfstring(lState, "%s: bad item #%d in table argument #%d in type (hint as string expected, got %s!)", functionName, subIndex, index, luaL_typename(lState, -1));
            lua_error(lState);
            Q_UNREACHABLE();
        }

        const QString hint = lua_tostring(lState, -1);
        hintList << hint;

        // removes value, but keeps key for next iteration
        lua_pop(lState, 1);
    }
}

// No documentation available in wiki - internal function
void TLuaInterpreter::parseCommandsOrFunctionsTable(lua_State* lState, const char* functionName, int& index, QStringList& commandsList, QVector<int>& luaFunctionNumbers)
{
    if (!lua_istable(lState, index)) {
        lua_pushfstring(lState, "%s: bad argument #%d type (%s as table expected, got %s!)", functionName, "commands/functions", luaL_typename(lState, index));
        lua_error(lState);
        Q_UNREACHABLE();
    }

    lua_pushnil(lState);
    // Keep track of the index of the item in the table
    int subIndex = 0;
    while (lua_next(lState, index)) {
        // key at index -2 and value at index -1
        ++subIndex;
        if (!(lua_isstring(lState, -1) || lua_isfunction(lState, -1))) {
            lua_pushfstring(lState, "%s: bad item #%d in table argument #%d in type (command as string or function expected, got %s!)", functionName, subIndex, index, luaL_typename(lState, -1));
            lua_error(lState);
            Q_UNREACHABLE();
        }

        if (lua_isfunction(lState, -1)) {
            lua_pushvalue(lState, -1);
            luaFunctionNumbers << luaL_ref(lState, LUA_REGISTRYINDEX);
            commandsList << QString();
        } else {
            const QString command = lua_tostring(lState, -1);
            luaFunctionNumbers << 0;
            commandsList << command;
        }

        // removes value, but keeps key for next iteration
        lua_pop(lState, 1);
    }
}

// No Documentation - public function but should stay undocumented -- compare https://github.com/Mudlet/Mudlet/issues/1149
int TLuaInterpreter::insertHTML(lua_State* L)
{
    const QString sendText = getVerifiedString(L, __func__, 1, "sendText");
    const Host& host = getHostFromLua(L);
    host.mpConsole->insertHTML(sendText);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addSupportedTelnetOption
int TLuaInterpreter::addSupportedTelnetOption(lua_State* L)
{
    const int option = getVerifiedInt(L, __func__, 1, "option");
    Host& host = getHostFromLua(L);
    host.mTelnet.supportedTelnetOptions[option] = true;
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echo
int TLuaInterpreter::echo(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString consoleName;
    const int n = lua_gettop(L);
    int s = 1;

    if (n > 1) {
        consoleName = getVerifiedString(L, __func__, s++, "console name", true);
    }

    const QString displayText = getVerifiedString(L, __func__, s, "text to display");

    if (isMain(consoleName)) {
        host.mpConsole->buffer.mEchoingText = true;
        host.mpConsole->echo(displayText);
        host.mpConsole->buffer.mEchoingText = false;
        // Writing to the main window must always succeed, but for consistent
        // results, we now return a true for that
        lua_pushboolean(L, true);
        return 1;
    }
    if (!host.echoWindow(consoleName, displayText)) {
        return warnArgumentValue(L, __func__, qsl("console/label '%1' does not exist").arg(consoleName));
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMergeTables
int TLuaInterpreter::setMergeTables(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QStringList modulesList;
    const int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        modulesList << getVerifiedString(L, __func__, i, "module");
    }

    host.mGMCP_merge_table_keys = host.mGMCP_merge_table_keys + modulesList;
    host.mGMCP_merge_table_keys.removeDuplicates();

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMudletVersion
int TLuaInterpreter::getMudletVersion(lua_State* L)
{
    QByteArray version = QByteArray(APP_VERSION).trimmed();
    const QByteArray build = mudlet::self()->mAppBuild.trimmed().toLocal8Bit();

    QList<QByteArray> const versionData = version.split('.');
    if (versionData.size() != 3) {
        qWarning() << "TLuaInterpreter::getMudletVersion(): ERROR: Version data not correctly set on compilation,\n"
                   << "   is the VERSION value in the project file present?";
        lua_pushstring(L, "getMudletVersion: sorry, version information not available.");
        return lua_error(L);
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
        return lua_error(L);
    }

    const int n = lua_gettop(L);

    if (n == 1) {
        const QString tidiedWhat = getVerifiedString(L, __func__, 1, "style", true).toLower().trimmed();
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
            return lua_error(L);
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
        lua_pushstring(L, mudlet::self()->mAppBuild.trimmed().toUtf8().constData());
        lua_settable(L, -3);
    } else {
        lua_pushstring(L,
                       "getMudletVersion: only takes one (optional) argument:\n"
                       "   \"major\", \"minor\", \"revision\", \"build\", \"string\" or \"table\".");
        return lua_error(L);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#openWebPage
int TLuaInterpreter::openWebPage(lua_State* L)
{
    const QString url = getVerifiedString(L, __func__, 1, "URL");
    lua_pushboolean(L, mudlet::self()->openWebPage(url));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getTime
int TLuaInterpreter::getTime(lua_State* L)
{
    const int n = lua_gettop(L);
    bool return_string = false;
    QString format = qsl("yyyy.MM.dd hh:mm:ss.zzz");
    QString tm;
    if (n > 0) {
        return_string = getVerifiedBool(L, __func__, 1, "return as string", true);
        if (n > 1) {
            format = getVerifiedString(L, __func__, 2, "custom time format");
        }
    }
    const QDateTime time = QDateTime::currentDateTime();
    if (return_string) {
        tm = time.toString(format);
        lua_pushstring(L, tm.toUtf8().constData());
    } else {
        const QDate dt = time.date();
        const QTime tm = time.time();
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


// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectCmdLineText
int TLuaInterpreter::addCmdLineBlacklist(lua_State* L)
{
    const int n = lua_gettop(L);
    QString name = "main";
    if (n > 1) {
        name = CMDLINE_NAME(L, 1);
    }
    const QString text = getVerifiedString(L, __func__, n, "suggestion text");
    auto pN = COMMANDLINE(L, name);
    pN->addBlacklist(text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeCmdLineBlacklist
int TLuaInterpreter::removeCmdLineBlacklist(lua_State* L)
{
    const int n = lua_gettop(L);
    QString name = "main";
    if (n > 1) {
        name = CMDLINE_NAME(L, 1);
    }
    const QString text = getVerifiedString(L, __func__, n, "suggestion text");
    auto pN = COMMANDLINE(L, name);
    pN->removeBlacklist(text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearCmdLineBlacklist
int TLuaInterpreter::clearCmdLineBlacklist(lua_State* L)
{
    const int n = lua_gettop(L);
    QString name = "main";
    if (n == 1) {
        name = CMDLINE_NAME(L, 1);
    }
    auto pN = COMMANDLINE(L, name);
    pN->clearBlacklist();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#installPackage
int TLuaInterpreter::installPackage(lua_State* L)
{
    const QString location = getVerifiedString(L, __func__, 1, "package location path and file name");
    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.installPackage(location, enums::PackageModuleType::Package); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#uninstallPackage
int TLuaInterpreter::uninstallPackage(lua_State* L)
{
    const QString packageName = getVerifiedString(L, __func__, 1, "package name");
    Host& host = getHostFromLua(L);
    const bool result = host.uninstallPackage(packageName, enums::PackageModuleType::Package);
    if (!result) {
        lua_pushnil(L);
    } else {
        lua_pushboolean(L, result);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#installModule
int TLuaInterpreter::installModule(lua_State* L)
{
    const QString modName = getVerifiedString(L, __func__, 1, "module location");
    Host& host = getHostFromLua(L);
    const QString module = QDir::fromNativeSeparators(modName);

    if (auto [success, message] = host.installPackage(module, enums::PackageModuleType::ModuleFromScript); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    auto moduleManager = host.mpModuleManager;
    if (moduleManager && moduleManager->moduleTable->isVisible()) {
        moduleManager->layoutModules();
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#uninstallModule
int TLuaInterpreter::uninstallModule(lua_State* L)
{
    const QString module = getVerifiedString(L, __func__, 1, "module name");
    Host& host = getHostFromLua(L);
    if (!host.uninstallPackage(module, enums::PackageModuleType::ModuleFromScript)) {
        lua_pushboolean(L, false);
        return 1;
    }
    auto moduleManager = host.mpModuleManager;
    if (moduleManager && moduleManager->moduleTable->isVisible()) {
        moduleManager->layoutModules();
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#reloadModule
int TLuaInterpreter::reloadModule(lua_State* L)
{
    const QString module = getVerifiedString(L, __func__, 1, "module name");
    Host& host = getHostFromLua(L);
    host.reloadModule(module);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableModuleSync
int TLuaInterpreter::enableModuleSync(lua_State* L)
{
    const QString module = getVerifiedString(L, __func__, 1, "module name");
    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.changeModuleSync(module, QLatin1String("1")); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    auto moduleManager = host.mpModuleManager;
    if (moduleManager && !moduleManager->moduleTable->findItems(module, Qt::MatchExactly).isEmpty()) {
        const int row = moduleManager->moduleTable->findItems(module, Qt::MatchExactly)[0]->row();
        auto checkItem = moduleManager->moduleTable->item(row, 2);
        checkItem->setCheckState(Qt::Checked);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableModuleSync
int TLuaInterpreter::disableModuleSync(lua_State* L)
{
    const QString module = getVerifiedString(L, __func__, 1, "module name");
    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.changeModuleSync(module, QLatin1String("0")); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    auto moduleManager = host.mpModuleManager;
    if (moduleManager && !moduleManager->moduleTable->findItems(module, Qt::MatchExactly).isEmpty()) {
        const int row = moduleManager->moduleTable->findItems(module, Qt::MatchExactly)[0]->row();
        auto checkItem = moduleManager->moduleTable->item(row, 2);
        checkItem->setCheckState(Qt::Unchecked);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getModuleSync
int TLuaInterpreter::getModuleSync(lua_State* L)
{
    const QString module = getVerifiedString(L, __func__, 1, "module name");
    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.getModuleSync(module); !success) {
        return warnArgumentValue(L, __func__, message);
    } else if (message == QLatin1String("1")) {
        lua_pushboolean(L, true);
        return 1;
    } else {
        lua_pushboolean(L, false);
        return 1;
    }
}


// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPackages
int TLuaInterpreter::getPackages(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    auto packages = host.mInstalledPackages;
    lua_newtable(L);
    for (int i = 0; i < packages.size(); i++) {
        lua_pushnumber(L, i + 1);
        lua_pushstring(L, packages.at(i).toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getModules
int TLuaInterpreter::getModules(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    auto modules = host.mInstalledModules;
    int counter = 0;
    QMap<QString, QStringList>::const_iterator iter = modules.constBegin();
    lua_newtable(L);
    while (iter != modules.constEnd()) {
        lua_pushnumber(L, ++counter);
        lua_pushstring(L, iter.key().toUtf8().constData());
        lua_settable(L, -3);
        ++iter;
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getModuleInfo
int TLuaInterpreter::getModuleInfo(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    auto infoMap = host.mModuleInfo;
    const int n = lua_gettop(L);
    const QString name = getVerifiedString(L, __func__, 1, "module name");
    QString info;
    if (n > 1) {
        info = getVerifiedString(L, __func__, 2, "info", true);
    }
    if (info.isEmpty()) {
        QMap<QString, QString>::const_iterator iter = infoMap.value(name).constBegin();
        lua_newtable(L);
        while (iter != infoMap.value(name).constEnd()) {
            lua_pushstring(L, iter.key().toUtf8().constData());
            lua_pushstring(L, iter.value().toUtf8().constData());
            lua_settable(L, -3);
            ++iter;
        }
    } else {
        lua_pushstring(L, infoMap.value(name).value(info).toUtf8().constData());
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPackageInfo
int TLuaInterpreter::getPackageInfo(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    auto infoMap = host.mPackageInfo;
    const int n = lua_gettop(L);
    const QString name = getVerifiedString(L, __func__, 1, "package name");
    QString info;
    if (n > 1) {
        info = getVerifiedString(L, __func__, 2, "info", true);
    }
    if (info.isEmpty()) {
        QMap<QString, QString>::const_iterator iter = infoMap.value(name).constBegin();
        lua_newtable(L);
        while (iter != infoMap.value(name).constEnd()) {
            lua_pushstring(L, iter.key().toUtf8().constData());
            lua_pushstring(L, iter.value().toUtf8().constData());
            lua_settable(L, -3);
            ++iter;
        }
    } else {
        lua_pushstring(L, infoMap.value(name).value(info).toUtf8().constData());
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setModuleInfo
int TLuaInterpreter::setModuleInfo(lua_State* L)
{
  Host& host = getHostFromLua(L);
  const QString moduleName = getVerifiedString(L, __func__, 1, "module name");
  const QString info = getVerifiedString(L, __func__, 2, "info");
  const QString value = getVerifiedString(L, __func__, 3, "value");
  host.mModuleInfo[moduleName][info] = value;
  lua_pushboolean(L, true);
  return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setPackageInfo
int TLuaInterpreter::setPackageInfo(lua_State* L)
{
  Host& host = getHostFromLua(L);
  const QString packageName = getVerifiedString(L, __func__, 1, "package name");
  const QString info = getVerifiedString(L, __func__, 2, "info");
  const QString value = getVerifiedString(L, __func__, 3, "value");
  host.mPackageInfo[packageName][info] = value;
  lua_pushboolean(L, true);
  return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDefaultAreaVisible
int TLuaInterpreter::setDefaultAreaVisible(lua_State* L)
{
    const Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    const bool isToShowDefaultArea = getVerifiedBool(L, __func__, 1, "isToShowDefaultArea");
    if (host.mpMap->mpMapper) {
        // If we are re-enabling the display of the default area
        // AND the mapper was showing the default area
        // the area widget will NOT be showing the correct area name afterwards
        bool isAreaWidgetInNeedOfResetting = false;
        if ((!host.mpMap->getDefaultAreaShown()) && (isToShowDefaultArea) && (host.mpMap->mpMapper->mp2dMap->mAreaID == -1)) {
            isAreaWidgetInNeedOfResetting = true;
        }

        host.mpMap->setDefaultAreaShown(isToShowDefaultArea);
        if (isAreaWidgetInNeedOfResetting) {
            // Corner case fixup:
            host.mpMap->mpMapper->comboBox_showArea->setCurrentText(host.mpMap->getDefaultAreaName());
        }
        host.mpMap->mpMapper->mp2dMap->repaint();
        host.mpMap->mpMapper->update();
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#registerAnonymousEventHandler
// The function below is mostly unused now as it is overwritten in lua.
// The overwriting function poses as a transparent proxy and internally uses
// this function to get called events.
int TLuaInterpreter::registerAnonymousEventHandler(lua_State* L)
{
    const QString event = getVerifiedString(L, __func__, 1, "event name");
    const QString func = getVerifiedString(L, __func__, 2, "function name");
    Host& host = getHostFromLua(L);
    host.registerAnonymousEventHandler(event, func);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#expandAlias
int TLuaInterpreter::expandAlias(lua_State* L)
{
    const QString payload = getVerifiedString(L, __func__, 1, "text to parse");
    bool wantPrint = true;
    if (lua_gettop(L) > 1) {
        // check if the 2nd argument is a 'false', but don't match if it is 'nil'
        // because expandAlias("command") should be the same as expandAlias("command", nil)
        if (lua_isnil(L, 2)) {
            wantPrint = false;
        } else {
            wantPrint = getVerifiedBool(L, __func__, 2, "echo", true);
        }
    }
    Host& host = getHostFromLua(L);
    // Host::send will encode the UTF encoded data here in the wanted Server
    // encoding:
    host.send(payload, wantPrint, false);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendCmdLine
int TLuaInterpreter::sendCmdLine(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "command");
    Host& host = getHostFromLua(L);
    host.sendCmdLine(text);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#send
// Note this is registered as send NOT sendRaw - see initLuaGlobals()
// It converts the bytes in the command (the first argument) from Utf-8 to be
// encoded in the required Mud Server encoding.
int TLuaInterpreter::sendRaw(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "command");
    bool wantPrint = true;
    if (lua_gettop(L) > 1) {
        wantPrint = getVerifiedBool(L, __func__, 2, "showOnScreen", true);
    }
    Host& host = getHostFromLua(L);
    // Host::send will encode the UTF encoded data here in the wanted Server encoding:
    host.send(text, wantPrint, true);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendSocket
// The data can, theoretically, contain embedded ASCII NUL characters, but they
// cannot be entered directly as they immediately terminate the string. Instead
// provide a true as a second argument and use the appropriate "code" value
// defined in the parseTelnetCodes() function:
int TLuaInterpreter::sendSocket(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "sendSocket: bad argument #1 type (data as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    bool parseCodes = false;
    if (lua_gettop(L) > 1) {
        parseCodes = getVerifiedBool(L, __func__, 2, "parse telnet codes {default = false}", true);
    }
    const QByteArray data{lua_tostring(L, 1)};
    std::string dataStdString{parseCodes ? parseTelnetCodes(data).toStdString() : data.toStdString()};

    Host& host = getHostFromLua(L);
    // msg is not in an encoded form here it is a literal set of bytes, which
    // is what this usage needs:
    if (!host.mTelnet.socketOutRaw(dataStdString)) {
        lua_pushnil(L);
        lua_pushstring(L, "sendSocket: unable to send any/all of the data, is the Server connected?");
        return 2;
    }

    lua_pushboolean(L, true);
    return 1;
}



// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setServerEncoding
int TLuaInterpreter::setServerEncoding(lua_State* L)
{
    Host& host = getHostFromLua(L);

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setServerEncoding: bad argument #1 type (newEncoding as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    const QByteArray newEncoding = lua_tostring(L, 1);
    QPair<bool, QString> const results = host.mTelnet.setEncoding(newEncoding);

    if (!results.first) {
        return warnArgumentValue(L, __func__, results.second);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getServerEncoding
int TLuaInterpreter::getServerEncoding(lua_State* L)
{
    const Host& host = getHostFromLua(L);

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
    const Host& host = getHostFromLua(L);

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
    auto pushProcessor = [&] () {
#if defined(Q_PROCESSOR_IA64)
        lua_pushstring(L, "ia64");
        return 1;
#elif defined(Q_PROCESSOR_X86_64)
        lua_pushstring(L, "x86 (64-bit)");
        return 1;
#elif defined(Q_PROCESSOR_X86_32)
        lua_pushstring(L, "x86 (32-bit)");
        return 1;
#elif defined(Q_PROCESSOR_POWER_64)
        lua_pushstring(L, "ppc (64-bit)");
        return 1;
#elif defined(Q_PROCESSOR_POWER_32)
        lua_pushstring(L, "ppc (32-bit)");
        return 1;
#elif defined(Q_PROCESSOR_ARM_V7)
        lua_pushstring(L, "arm7");
        return 1;
#elif defined(Q_PROCESSOR_ARM_V6)
        lua_pushstring(L, "arm6");
        return 1;
#elif defined(Q_PROCESSOR_ARM_V5)
        lua_pushstring(L, "arm5");
        return 1;
#else
        return 0;
#endif
    };

#if defined(Q_OS_CYGWIN)
    // Try for this one before Q_OS_WINDOWS as both are likely to be defined on
    // a Cygwin platform
    // CHECK: hopefully will NOT be triggered on mingw/msys
    lua_pushstring(L, "cygwin");
    lua_pushstring(L, QSysInfo::productVersion().toUtf8().constData());
    return 2 + pushProcessor();
#elif defined(Q_OS_WINDOWS)
    lua_pushstring(L, "windows");
    lua_pushstring(L, QSysInfo::productVersion().toUtf8().constData());
    return 2 + pushProcessor();
#elif defined(Q_OS_MACOS)
    lua_pushstring(L, "mac");
    lua_pushstring(L, QSysInfo::productVersion().toUtf8().constData());
    return 2 + pushProcessor();
#elif defined(Q_OS_LINUX)
    lua_pushstring(L, "linux");
    lua_pushstring(L, QSysInfo::productVersion().toUtf8().constData());
    lua_pushstring(L, QSysInfo::productType().toUtf8().constData());
    return 3 + pushProcessor();
#elif defined(Q_OS_HURD)
    lua_pushstring(L, "hurd");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2 + pushProcessor();
#elif defined(Q_OS_FREEBSD)
    // Only defined on FreeBSD but NOT Debian kFreeBSD so we should check for
    // this first
    lua_pushstring(L, "freebsd");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2 + pushProcessor();
#elif defined(Q_OS_FREEBSD_KERNEL)
    // Defined for BOTH Debian kFreeBSD hybrid with a GNU userland and
    // main FreeBSD so it must be after Q_OS_FREEBSD check; included for Debian
    // packager who may want to have this!
    lua_pushstring(L, "kfreebsd");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2 + pushProcessor();
#elif defined(Q_OS_OPENBSD)
    lua_pushstring(L, "openbsd");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2 + pushProcessor();
#elif defined(Q_OS_NETBSD)
    lua_pushstring(L, "netbsd");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2 + pushProcessor();
#elif defined(Q_OS_BSD4)
    // Generic *nix - must be before unix and after other more specific results
    lua_pushstring(L, "bsd4");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2 + pushProcessor();
#elif defined(Q_OS_UNIX)
    // Most generic *nix - must be after bsd4 and other more specific results
    lua_pushstring(L, "unix");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2 + pushProcessor();
#else
    lua_pushstring(L, "unknown");
    lua_pushstring(L, "unknown");
    return 2 + pushProcessor();
#endif
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getProcessID
int TLuaInterpreter::getProcessID(lua_State* L)
{
    int pid = QApplication::applicationPid();
    lua_pushinteger(L, pid);
    return 1;
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::compileAndExecuteScript(const QString& code)
{
    if (code.isEmpty()) {
        return false;
    }
    lua_State* L = pGlobalLua;

    const int error = luaL_dostring(L, code.toUtf8().constData());
    if (error) {
        std::string e = "no error message available from Lua";
        if (lua_isstring(L, 1)) {
            e = "Lua error:";
            e += lua_tostring(L, 1);
        }
        if (mudlet::smDebugMode) {
            qDebug() << "LUA ERROR: code did not compile: ERROR:" << e.c_str();
        }
        const QString _n = "error in Lua code";
        const QString _n2 = "no debug data available";
        logError(e, _n, _n2);
    }

    lua_pop(L, lua_gettop(L));

    return !error;
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

    if (!validLuaCode(code).first) {
        return code;
    }

    QString escapedCode = code;
    // escape backslashes so we can pass \n to the function
    escapedCode.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    // escape quotes since we'll be using quotes to pass data to the function
    escapedCode.replace(QLatin1String("\""), QLatin1String("\\\""));
    // escape newlines so they don't interpreted as newlines, but instead get passed onto the function
    escapedCode.replace(QLatin1String("\n"), QLatin1String("\\n"));

    const QString thing = QString(R"(return get_formatted_code(get_ast("%1"), {indent_chunk = '  ', right_margin = 100, max_text_width = 160, keep_comments = true}))").arg(escapedCode);
    const int error = luaL_dostring(L, thing.toUtf8().constData());
    if (error) {
        std::string e = "no error message available from Lua";
        if (lua_isstring(L, 1)) {
            e = "Lua error:";
            e += lua_tostring(L, 1);
        }
        if (mudlet::smDebugMode) {
            qDebug() << "LUA ERROR: code did not compile: ERROR:" << e.c_str();
        }
        const QString objectName = "error in Lua code";
        const QString functionName = "no debug data available";
        logError(e, objectName, functionName);
        lua_pop(L, lua_gettop(L));
        return code;
    }

    QString result = lua_tostring(L, 1);
    lua_pop(L, lua_gettop(L));
    return result;
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::compile(const QString& code, QString& errorMsg, const QString& name)
{
    lua_State* L = pGlobalLua;

    const int error = (luaL_loadbuffer(L, code.toUtf8().constData(),
                                 strlen(code.toUtf8().constData()),
                                 name.toUtf8().constData()) || lua_pcall(L, 0, 0, 0));

    if (error) {
        std::string e = "Lua syntax error:";
        if (lua_isstring(L, 1)) {
            e.append(lua_tostring(L, 1));
        }
        errorMsg = "<b><font color='blue'>";
        errorMsg.append(QString::fromStdString(e).toHtmlEscaped().toUtf8());
        errorMsg.append("</font></b>");
        if (mudlet::smDebugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::red) << "\n " << e.c_str() << "\n" >> &host;
        }
    } else {
        if (mudlet::smDebugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA: code compiled without errors. OK\n" >> &host;
        }
    }
    lua_pop(L, lua_gettop(L));

    return !error;
}

// No documentation available in wiki - internal function
// returns pair where first is bool stating true the given Lua code is valid, false otherwise
// second is empty if code is valid, error message if not valid
std::pair<bool, QString> TLuaInterpreter::validateLuaCodeParam(int index)
{
    lua_State* L = pGlobalLua;
    if (!lua_isstring(L, index)) {
        return {false, qsl("lua script as string expected, got %1!").arg(luaL_typename(L, index))};
    }
    const QString script{lua_tostring(L, index)};
    return validLuaCode(script);
}

// No documentation available in wiki - internal function
// returns pair where first is bool stating true the given Lua code is valid, false otherwise
// second is empty if code is valid, error message if not valid
std::pair<bool, QString> TLuaInterpreter::validLuaCode(const QString &code)
{
    lua_State* L = pGlobalLua;
    const int error = luaL_loadbuffer(L, code.toUtf8().constData(), strlen(code.toUtf8().constData()), code.toUtf8().data());
    const int topElementIndex = lua_gettop(L);
    QString e = "invalid Lua code: ";
    if (error) {
        if (lua_isstring(L, topElementIndex)) {
            e += lua_tostring(L, topElementIndex);
        } else {
            e += "No error message available from Lua";
        }
    }
    lua_pop(L, 1);
    return {!error, e};
}

// No documentation available in wiki - internal function
void TLuaInterpreter::setMultiCaptureGroups(const std::list<std::list<std::string>>& captureList, const std::list<std::list<int>>& posList, QVector<QVector<QPair<QString, QString>>>& nameGroups)
{
    mMultiCaptureGroupList = captureList;
    mMultiCaptureGroupPosList = posList;
    mMultiCaptureNameGroups = nameGroups;

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
void TLuaInterpreter::setCaptureNameGroups(const NameGroupMatches& nameGroups, const NamedMatchesRanges& namePositions)
{
    mCapturedNameGroups = nameGroups;
    mCapturedNameGroupsPosList = namePositions;
}

// No documentation available in wiki - internal function
void TLuaInterpreter::clearCaptureGroups()
{
    mCaptureGroupList.clear();
    mCaptureGroupPosList.clear();
    mMultiCaptureGroupList.clear();
    mMultiCaptureGroupPosList.clear();
    mCapturedNameGroups.clear();
    mCapturedNameGroupsPosList.clear();
    mMultiCaptureNameGroups.clear();

    lua_State* L = pGlobalLua;
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

    NamedMatchesRanges::iterator i;
    for (i = mCapturedNameGroupsPosList.begin(); i != mCapturedNameGroupsPosList.cend(); ++i) {
        i.value().first += a;
        i.value().second += a;
    }
}

// No documentation available in wiki - internal function
void TLuaInterpreter::setAtcpTable(const QString& var, const QString& arg)
{
    lua_State* L = pGlobalLua;
    lua_getglobal(L, "atcp"); //defined in LuaGlobal.lua
    lua_pushstring(L, var.toUtf8().constData());
    lua_pushstring(L, arg.toUtf8().constData());
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

// No documentation available in wiki - internal function
void TLuaInterpreter::signalMXPEvent(const QString &type, const QMap<QString, QString> &attrs, const QStringList &actions, const QString &caption)
{
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
    lua_pop(L, 1);

    lua_pushstring(L, caption.toUtf8().constData());
    lua_setfield(L, -2, "text");

    lua_pop(L, lua_gettop(L));


    TEvent event{};
    QString token("mxp");
    token.append(".");
    token.append(type.toUtf8().toLower().constData());

    event.mArgumentList.append(token);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

    Host &host = getHostFromLua(L);
    if (mudlet::smDebugMode) {
        const QString msg = qsl("\n%1 event <%2> display(%1) to see the full content\n").arg("mxp", token);
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
        qCritical() << "ERROR: could not grow Lua stack by" << tokenList.size() + 5 << "elements, parsing GMCP/MSDP failed. Current stack size is" << lua_gettop(L);
        return;
    }
    int i = 0;
    for (const int total = tokenList.size() - 1; i < total; ++i) {
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
    const int error = lua_pcall(L, 1, 1, 0);
    if (!error) {
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
            for (const int total = tokenList.size() - 1; i < total; ++i) {
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
            const QString _n = "JSON decoder error:";
            const QString _f = "json_to_value";
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
        if (mudlet::smDebugMode) {
            const QString msg = qsl("\n%1 event <%2> display(%1) to see the full content\n").arg(protocol, token);
            host.mpConsole->printSystemMessage(msg);
        }
        host.raiseEvent(event);
    }
    // auto-detect IRE composer
    if (tokenList.size() == 3 && tokenList.at(0).toLower() == "ire" && tokenList.at(1).toLower() == "composer" && tokenList.at(2).toLower() == "edit") {
        handleIreComposerEdit(string_data);
    }
    lua_pop(L, lua_gettop(L));
}

void TLuaInterpreter::handleIreComposerEdit(const QString& jsonData)
{
    Host& host = getHostFromLua(pGlobalLua);

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "IRE Composer: JSON parse error:" << parseError.errorString();
        return;
    }

    if (!jsonDoc.isObject()) {
        qDebug() << "IRE Composer: JSON is not an object";
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    if (!jsonObj.contains("title") || !jsonObj.contains("text")) {
        qDebug() << "IRE Composer: Missing required 'title' or 'text' fields";
        return;
    }

    const QString title = jsonObj["title"].toString();
    const QString initialText = jsonObj["text"].toString();

    if (host.mTelnet.mpComposer) {
        return;
    }

    host.mTelnet.mpComposer = new dlgComposer(&host);
    host.mTelnet.mpComposer->init(title, initialText);
    host.mTelnet.mpComposer->raise();
    host.mTelnet.mpComposer->show();
}

// No documentation available in wiki - internal function
void TLuaInterpreter::parseMSSP(const QString& string_data)
{
    lua_State* L = pGlobalLua;

    // string_data is in the format of MSSP_VAR "PLAYERS" MSSP_VAL "52" MSSP_VAR "UPTIME" MSSP_VAL "1234567890"
    // The quote characters mean that the encased word is a string, the quotes themselves are not sent.
    QStringList packageList = string_data.split(MSSP_VAR);

    if (!packageList.isEmpty()) {
        Host& host = getHostFromLua(L);

        for (int i = 1; i < packageList.size(); i++) {
            // clear the stack to avoid it getting to big
            lua_settop(L, 0);

            QStringList payloadList = packageList[i].split(MSSP_VAL);

            if (payloadList.size() != 2) {
                continue;
            }

            const QString msspVAR = payloadList[0];
            const QString msspVAL = payloadList[1];

            lua_getglobal(L, "mssp");
            lua_pushstring(L, msspVAR.toUtf8().constData());
            lua_pushlstring(L, msspVAL.toUtf8().constData(), msspVAL.toUtf8().length());

            lua_rawset(L, -3);

            // Raise an event
            const QString protocol = qsl("mssp");
            QString token = protocol;
            token.append(".");
            token.append(msspVAR);

            TEvent event{};
            event.mArgumentList.append(token);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            event.mArgumentList.append(token);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            if (mudlet::smDebugMode) {
                const QString msg = qsl("\n%1 event <%2> display(%1) to see the full content\n").arg(protocol, token);
                host.mpConsole->printSystemMessage(msg);
            }
            host.raiseEvent(event);

            if (msspVAR == "HOSTNAME") {
                host.mMSSPHostName = msspVAL;
            } else if (msspVAR == "TLS" || msspVAR == "SSL") {
                // In certain MSSP fields "-1" and "1" denote not supported/supported indicators,
                // however the port is the standard value here. Ignore those values here.
                host.mMSSPTlsPort = (msspVAL != "-1" && msspVAL != "1") ? msspVAL.toInt() : 0;
            }
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
    const QByteArray transcodedSrc = host.mTelnet.decodeBytes(src);
    QStringList varList;
    QByteArray lastVar;
    const int textLength = transcodedSrc.length();
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
                    setMSDPTable(token, script);
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
            varList.append(lastVar);
            last = MSDP_VAL;
            break;
        case '\\':
            script.append('\\');
            break;
        case '\"':
            script.append('\\');
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
        setMSDPTable(token, script);
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
            // if ((*it).length() < 1) continue; //have empty capture groups to be undefined keys i.e. matches[emptyCapGroupNumber] = nil otherwise it's = "" i.e. an empty string
            lua_pushnumber(L, i);
            lua_pushstring(L, (*it).c_str());
            lua_settable(L, -3);
        }
        for (const auto &[name, capture] : mCapturedNameGroups) {
            lua_pushstring(L, name.toUtf8().constData());
            lua_pushstring(L, capture.toUtf8().constData());
            lua_settable(L, -3);
        }
        lua_setglobal(L, "matches");
    }
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::call_luafunction(void* pT)
{
    QElapsedTimer executionTimer;

    if (mudlet::smDebugMode) {
        executionTimer.start();
    }

    lua_State* L = pGlobalLua;
    lua_pushlightuserdata(L, pT);
    lua_gettable(L, LUA_REGISTRYINDEX);
    if (lua_isfunction(L, -1)) {
        setMatches(L);
        const int error = lua_pcall(L, 0, LUA_MULTRET, 0);
        if (error) {
            const int nbpossible_errors = lua_gettop(L);
            for (int i = 1; i <= nbpossible_errors; i++) {
                std::string e = "";
                if (lua_isstring(L, i)) {
                    e = "Lua error:";
                    e += lua_tostring(L, i);
                    const QString _n = "error in anonymous Lua function";
                    const QString _n2 = "no debug data available";
                    logError(e, _n, _n2);
                    if (mudlet::smDebugMode) {
                        auto& host = getHostFromLua(L);
                        TDebug(Qt::white, Qt::red) << "LUA: ERROR running anonymous Lua function ERROR:" << e.c_str() >> &host;
                    }
                }
            }
        } else {
            if (mudlet::smDebugMode) {
                auto& host = getHostFromLua(L);
                TDebug(Qt::white, Qt::darkGreen) << "LUA OK anonymous Lua function ran without errors" << (executionTimer.isValid() ? " in " + QString::number(executionTimer.elapsed()) + "ms.\n" : ".\n") >> &host;
            }
        }
        lua_pop(L, lua_gettop(L));
        //lua_settop(L, 0);
        return !error;

    }

    const QString _n = "error in anonymous Lua function";
    const QString _n2 = "func reference not found by Lua, func cannot be called";
    std::string e = "Lua error:";
    logError(e, _n, _n2);
    return false;
}

// No documentation available in wiki - internal function
void TLuaInterpreter::delete_luafunction(void* pT)
{
    lua_State* L = pGlobalLua;
    lua_pushlightuserdata(L, pT);
    lua_pushnil(L);
    lua_rawset(L, LUA_REGISTRYINDEX);
}

// No documentation available in wiki - internal function
void TLuaInterpreter::delete_luafunction(const QString& name)
{
    lua_State* L = pGlobalLua;
    lua_getglobal(L, name.toUtf8().constData());
    if (lua_isfunction(L, -1)) {
        lua_pushnil(L);
        lua_setglobal(L, name.toUtf8().constData());
        lua_pop(L, lua_gettop(L));
    } else if (mudlet::smDebugMode) {
        qWarning() << "LUA: ERROR deleting " << name << ", it is not a function as expected";
    }
}

// No documentation available in wiki - internal function
// returns true if function ran without errors
// as well as the boolean return value from the function
std::pair<bool, bool> TLuaInterpreter::callLuaFunctionReturnBool(void* pT)
{
    QElapsedTimer executionTimer;

    if (mudlet::smDebugMode) {
        executionTimer.start();
    }

    lua_State* L = pGlobalLua;

    lua_pushlightuserdata(L, pT);
    lua_gettable(L, LUA_REGISTRYINDEX);
    bool returnValue = false;

    if (lua_isfunction(L, -1)) {
        setMatches(L);
        const int error = lua_pcall(L, 0, LUA_MULTRET, 0);
        if (error) {
            const int nbpossible_errors = lua_gettop(L);
            for (int i = 1; i <= nbpossible_errors; i++) {
                std::string e = "";
                if (lua_isstring(L, i)) {
                    e = "Lua error:";
                    e += lua_tostring(L, i);
                    const QString _n = "error in anonymous Lua function";
                    const QString _n2 = "no debug data available";
                    logError(e, _n, _n2);
                    if (mudlet::smDebugMode) {
                        auto& host = getHostFromLua(L);
                        TDebug(Qt::white, Qt::red) << "LUA: ERROR running anonymous Lua function ERROR:" << e.c_str() >> &host;
                    }
                }
            }
        } else {
            auto index = lua_gettop(L);
            if (lua_isboolean(L, index)) {
                returnValue = lua_toboolean(L, index);
            }

            if (mudlet::smDebugMode) {
                auto& host = getHostFromLua(L);
                TDebug(Qt::white, Qt::darkGreen) << "LUA OK anonymous Lua function ran without errors" << (executionTimer.isValid() ? " in " + QString::number(executionTimer.elapsed()) + "ms.\n" : ".\n") >> &host;
            }
        }
        lua_pop(L, lua_gettop(L));
        return {!error , returnValue};
    }

    const QString _n = "error in anonymous Lua function";
    const QString _n2 = "func reference not found by Lua, func cannot be called";
    std::string e = "Lua error:";
    logError(e, _n, _n2);

    return {false, false};
}

// No documentation available in wiki - internal function
// Third argument hides the "LUA OK" type message if it true which may be used
// to cut down on spammy output if things are okay.
bool TLuaInterpreter::call(const QString& function, const QString& mName, const bool muteDebugOutput)
{
    QElapsedTimer executionTimer;

    if (mudlet::smDebugMode) {
        executionTimer.start();
    }

    lua_State* L = pGlobalLua;
    setMatches(L);

    lua_getglobal(L, function.toUtf8().constData());
    const int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error) {
        const int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                logError(e, mName, function);
                if (mudlet::smDebugMode) {
                    auto& host = getHostFromLua(L);
                    TDebug(Qt::white, Qt::red) << "LUA ERROR: when running script " << mName << " (" << function << "),\nreason: " << e.c_str() << "\n" >> &host;
                }
            }
        }
    } else {
        if (mudlet::smDebugMode && !muteDebugOutput) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA OK: script " << mName << " (" << function << ") ran without errors" << (executionTimer.isValid() ? " in " + QString::number(executionTimer.elapsed()) + "ms.\n" : ".\n") >> &host;
        }
    }
    lua_pop(L, lua_gettop(L));

    return (error);
}

// No documentation available in wiki - internal function
std::pair<bool, bool> TLuaInterpreter::callReturnBool(const QString& function, const QString& mName)
{
    QElapsedTimer executionTimer;

    if (mudlet::smDebugMode) {
        executionTimer.start();
    }

    lua_State* L = pGlobalLua;
    bool returnValue = false;

    setMatches(L);

    lua_getglobal(L, function.toUtf8().constData());
    const int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error) {
        const int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                logError(e, mName, function);
                if (mudlet::smDebugMode) {
                    auto& host = getHostFromLua(L);
                    TDebug(Qt::white, Qt::red) << "LUA: ERROR running script " << mName << " (" << function << ") ERROR:" << e.c_str() << "\n" >> &host;
                }
            }
        }
    } else {
        auto index = lua_gettop(L);
        if (lua_isboolean(L, index)) {
            returnValue = lua_toboolean(L, index);
        }

        if (mudlet::smDebugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA OK script " << mName << " (" << function << ") ran without errors" << (executionTimer.isValid() ? " in " + QString::number(executionTimer.elapsed()) + "ms.\n" : ".\n") >> &host;
        }
    }
    lua_pop(L, lua_gettop(L));
    return {!error, returnValue};
}

// No documentation available in wiki - internal function
void TLuaInterpreter::logError(std::string& e, const QString& name, const QString& function)
{
    // Log error to Editor's Errors TConsole:
    if (mpHost->mpEditorDialog) {
        mpHost->mpEditorDialog->mpErrorConsole->print(qsl("[%1:]").arg(tr("ERROR")), QColor(Qt::blue), QColor(Qt::black));
        mpHost->mpEditorDialog->mpErrorConsole->print(qsl(" %1:<%2> %3:<%4>\n").arg(
            //: object is the Mudlet alias/trigger/script, used in this sample message: object:<Alias1> function:<cure_me>
            tr("object"),
            name,
            //: function is the Lua function, used in this sample message: object:<Alias1> function:<cure_me>
            tr("function"), function), QColor(Qt::green), QColor(Qt::black));
        mpHost->mpEditorDialog->mpErrorConsole->print(qsl("        <%1>\n").arg(e.c_str()), QColor(Qt::red), QColor(Qt::black));
    }

    // Log error to Profile's Main TConsole:
    if (mpHost->mEchoLuaErrors) {
        // ensure the Lua error is on a line of its own and is not prepended to
        // the previous line, however there is a nasty gotcha in that during
        // profile loading the (TMainConsole*) Host::mpConsole pointer is
        // null - but then the buffer must itself be empty:
        if (mpHost->mpConsole && !mpHost->mpConsole->buffer.isEmpty() && !mpHost->mpConsole->buffer.lineBuffer.at(mpHost->mpConsole->buffer.lineBuffer.size() - 1).isEmpty()) {
            mpHost->postMessage(qsl("\n"));
        }

        mpHost->postMessage(qsl("[  LUA  ] - %1: <%2> %3:<%4>\n<%5>").arg(
            //: object is the Mudlet alias/trigger/script, used in this sample message: object:<Alias1> function:<cure_me>
            tr("object"),
            name,
            //: function is the Lua function, used in this sample message: object:<Alias1> function:<cure_me>
            tr("function"), function, e.c_str()));
    }
}

// No documentation available in wiki - internal function
void TLuaInterpreter::logEventError(const QString& event, const QString& error)
{
    // Log error to Editor's Errors TConsole:
    if (mpHost->mpEditorDialog) {
        mpHost->mpEditorDialog->mpErrorConsole->print(qsl("[%1:]").arg(tr("ERROR")), QColor(Qt::blue), QColor(Qt::black));
        mpHost->mpEditorDialog->mpErrorConsole->print(qsl(" event handler for %1:\n").arg(event), QColor(Qt::green), QColor(Qt::black));
        mpHost->mpEditorDialog->mpErrorConsole->print(qsl("        <%1>\n").arg(error), QColor(Qt::red), QColor(Qt::black));
    }

    // Log error to Profile's Main TConsole:
    if (mpHost->mEchoLuaErrors) {
        // ensure the Lua error is on a line of its own and is not prepended to the previous line
        if (!mpHost->mpConsole->buffer.isEmpty() && !mpHost->mpConsole->buffer.lineBuffer.at(mpHost->mpConsole->buffer.lineBuffer.size() - 1).isEmpty()) {
            mpHost->postMessage(qsl("\n"));
        }

        mpHost->postMessage(qsl("[  LUA  ] - error in event handler for %1:\n<%2>").arg(event, error));
    }
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callConditionFunction(std::string& function, const QString& mName)
{
    QElapsedTimer executionTimer;

    if (mudlet::smDebugMode) {
        executionTimer.start();
    }

    lua_State* L = pGlobalLua;

    lua_getfield(L, LUA_GLOBALSINDEX, function.c_str());
    const int error = lua_pcall(L, 0, 1, 0);
    if (error) {
        const int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                const QString _f = function.c_str();
                logError(e, mName, _f);
                if (mudlet::smDebugMode) {
                    auto& host = getHostFromLua(L);
                    TDebug(Qt::white, Qt::red) << "LUA: ERROR running script " << mName << " (" << function.c_str() << ") ERROR:" << e.c_str() << "\n" >> &host;
                }
            }
        }
    } else {
        if (mudlet::smDebugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA OK script " << mName << " (" << function.c_str() << ") ran without errors" << (executionTimer.isValid() ? " in " + QString::number(executionTimer.elapsed()) + "ms\n" : ".\n") >> &host;
        }
    }

    bool ret = false;
    const int returnValues = lua_gettop(L);
    if (returnValues > 0) {
        // Lua docs: Like all tests in Lua, lua_toboolean returns 1 for any Lua value different from false and nil; otherwise it returns 0
        // This means trigger patterns don't have to strictly return true or false, as it is accepted in Lua
        ret = lua_toboolean(L, 1);
    }
    lua_pop(L, returnValues);
    return ((!error) && (ret > 0));
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callMulti(const QString& function, const QString& mName)
{
    QElapsedTimer executionTimer;

    if (mudlet::smDebugMode) {
        executionTimer.start();
    }

    lua_State* L = pGlobalLua;

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
            for (const auto &[name, capture] : mMultiCaptureNameGroups.value(k - 1)) {
                lua_pushstring(L, name.toUtf8().constData());
                lua_pushstring(L, capture.toUtf8().constData());
                lua_settable(L, -3);
            }
            lua_settable(L, -3); //matches in regex
        }
        lua_setglobal(L, "multimatches");
    }

    lua_getglobal(L, function.toUtf8().constData());
    const int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error) {
        const int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                logError(e, mName, function);
                if (mudlet::smDebugMode) {
                    auto& host = getHostFromLua(L);
                    TDebug(Qt::white, Qt::red) << "LUA: ERROR running script " << mName << " (" << function << ") ERROR:" << e.c_str() << "\n" >> &host;
                }
            }
        }
    } else {
        if (mudlet::smDebugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA OK script " << mName << " (" << function << ") ran without errors" << (executionTimer.isValid() ? " in " + QString::number(executionTimer.elapsed()) + "ms\n" : ".\n") >> &host;
        }
    }
    lua_pop(L, lua_gettop(L));
    return !error;
}

// No documentation available in wiki - internal function
std::pair<bool, bool> TLuaInterpreter::callMultiReturnBool(const QString& function, const QString& mName)
{
    QElapsedTimer executionTimer;

    if (mudlet::smDebugMode) {
        executionTimer.start();
    }

    lua_State* L = pGlobalLua;

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
    const int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error) {
        const int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                logError(e, mName, function);
                if (mudlet::smDebugMode) {
                    auto& host = getHostFromLua(L);
                    TDebug(Qt::white, Qt::red) << "LUA: ERROR running script " << mName << " (" << function << ") ERROR:" << e.c_str() << "\n" >> &host;
                }
            }
        }
    } else {
        auto index = lua_gettop(L);
        if (lua_isboolean(L, index)) {
            returnValue = lua_toboolean(L, index);
        }

        if (mudlet::smDebugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA OK script " << mName << " (" << function << ") ran without errors" << (executionTimer.isValid() ? " in " + QString::number(executionTimer.elapsed()) + "ms\n" : ".\n") >> &host;
        }
    }
    lua_pop(L, lua_gettop(L));
    return {!error, returnValue};
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callReference(lua_State* L, QString name, int parameters)
{
    int error = 0;
    error = lua_pcall(L, parameters, LUA_MULTRET, 0);
    if (error) {
        std::string err = "";
        if (lua_isstring(L, -1)) {
            err += lua_tostring(L, -1);
        }
        logError(err, name, qsl("anonymous Lua function"));
        if (mudlet::smDebugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::red) << "LUA: ERROR running anonymous Lua function (" << name << ")\nError: " << err.c_str() << "\n" >> &host;
        }
    }
    lua_pop(L, lua_gettop(L));
    return !error;
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callAnonymousFunction(const int func, QString name)
{
    lua_State* L = pGlobalLua;
    lua_rawgeti(L, LUA_REGISTRYINDEX, func);
    return callReference(L, name, 0);
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callCmdLineAction(const int func, QString text)
{
    lua_State* L = pGlobalLua;
    auto& host = getHostFromLua(L);

    // Suppress command line actions if remote echo is active (e.g., during password entry)
    if (host.isRemoteEchoingActive()) {
        return false; // Do not invoke actions during password entry
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, func);
    lua_pushstring(L, text.toUtf8().constData());
    return callReference(L, qsl("cmdLineAction"), 1);
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callLabelCallbackEvent(const int func, const QEvent* qE)
{
    lua_State* L = pGlobalLua;
    lua_rawgeti(L, LUA_REGISTRYINDEX, func);
    const QString name = qsl("label callback event");

    if (qE) {
        // Create Lua table with QEvent data if needed
        switch (qE->type()) {
        // This means the default argument value was used, so ignore
        case (QEvent::None):
            return callReference(L, name, 0);
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
            lua_pushstring(L, csmMouseButtons.value(qME->button()).toUtf8().constData());
            lua_setfield(L, -2, qsl("button").toUtf8().constData());

            // push buttons()
            lua_newtable(L);
            QMap<Qt::MouseButton, QString>::const_iterator iter = csmMouseButtons.constBegin();
            int counter = 1;
            while (iter != csmMouseButtons.constEnd()) {
                if (iter.key() & qME->buttons()) {
                    lua_pushnumber(L, counter);
                    lua_pushstring(L, iter.value().toUtf8().constData());
                    lua_settable(L, -3);
                    counter++;
                }
                ++iter;
            }
            lua_setfield(L, -2, qsl("buttons").toUtf8().constData());
            auto globalPosition = qME->globalPosition().toPoint();
            auto position = qME->position().toPoint();
            // Push globalX()
            lua_pushnumber(L, globalPosition.x());
            lua_setfield(L, -2, qsl("globalX").toUtf8().constData());

            // Push globalY()
            lua_pushnumber(L, globalPosition.y());
            lua_setfield(L, -2, qsl("globalY").toUtf8().constData());

            // Push x()
            lua_pushnumber(L, position.x());
            lua_setfield(L, -2, qsl("x").toUtf8().constData());

            // Push y()
            lua_pushnumber(L, position.y());
            lua_setfield(L, -2, qsl("y").toUtf8().constData());
            return callReference(L, name, 1);
        }
        // These are QEvents
        case (QEvent::Enter): {
            auto qME = static_cast<const QEnterEvent*>(qE);
            lua_newtable(L);
            auto globalPosition = qME->globalPosition().toPoint();
            auto position = qME->position().toPoint();
            // Push globalX()
            lua_pushnumber(L, globalPosition.x());
            lua_setfield(L, -2, qsl("globalX").toUtf8().constData());

            // Push globalY()
            lua_pushnumber(L, globalPosition.y());
            lua_setfield(L, -2, qsl("globalY").toUtf8().constData());

            // Push x()
            lua_pushnumber(L, position.x());
            lua_setfield(L, -2, qsl("x").toUtf8().constData());

            // Push y()
            lua_pushnumber(L, position.y());
            lua_setfield(L, -2, qsl("y").toUtf8().constData());
            return callReference(L, name, 1);
        }
        case (QEvent::Leave): {
            // Seems there isn't a QLeaveEvent, so no
            // extra information to be gotten
            return callReference(L, name, 0);
        }
        // This is a QWheelEvent
        case (QEvent::Wheel): {
            auto qME = static_cast<const QWheelEvent*>(qE);
            lua_newtable(L);

            // push buttons()
            lua_newtable(L);
            QMap<Qt::MouseButton, QString>::const_iterator iter = csmMouseButtons.constBegin();
            int counter = 1;
            while (iter != csmMouseButtons.constEnd()) {
                if (iter.key() & qME->buttons()) {
                    lua_pushnumber(L, counter);
                    lua_pushstring(L, iter.value().toUtf8().constData());
                    lua_settable(L, -3);
                    counter++;
                }
                ++iter;
            }
            lua_setfield(L, -2, qsl("buttons").toUtf8().constData());

            auto globalPosition = qME->globalPosition();
            // Push globalX()
            lua_pushnumber(L, globalPosition.x());
            lua_setfield(L, -2, qsl("globalX").toUtf8().constData());

            // Push globalY()
            lua_pushnumber(L, globalPosition.y());
            lua_setfield(L, -2, qsl("globalY").toUtf8().constData());

            auto position = qME->position();

            // Push x()
            lua_pushnumber(L, position.x());
            lua_setfield(L, -2, qsl("x").toUtf8().constData());

            // Push y()
            lua_pushnumber(L, position.y());
            lua_setfield(L, -2, qsl("y").toUtf8().constData());

            // Push angleDelta()
            lua_pushnumber(L, qME->angleDelta().x());
            lua_setfield(L, -2, qsl("angleDeltaX").toUtf8().constData());
            lua_pushnumber(L, qME->angleDelta().y());
            lua_setfield(L, -2, qsl("angleDeltaY").toUtf8().constData());
            return callReference(L, name, 1);
        }
        default: {
            // No-op - this silences warnings about unhandled QEvent types
        }
        }
    } else {
        return callReference(L, name, 0);
    }
    lua_pop(L, lua_gettop(L));
    return true;
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callEventHandler(const QString& function, const TEvent& pE)
{
    if (function.isEmpty()) {
        return false;
    }

    lua_State* L = pGlobalLua;

    int error = luaL_dostring(L, qsl("return %1").arg(function).toUtf8().constData());
    if (error) {
        std::string err;
        if (lua_isstring(L, 1)) {
            err = "Lua error: ";
            err += lua_tostring(L, 1);
        }
        const QString name = "event handler function";
        logError(err, name, function);
        return false;
    }

    // Lua is limited to ~50 arguments on a function
    auto maxArguments = std::min<qsizetype>(pE.mArgumentList.size(), LUA_FUNCTION_MAX_ARGS);
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

    if (mudlet::smDebugMode && pE.mArgumentList.size() > LUA_FUNCTION_MAX_ARGS) {
        auto& host = getHostFromLua(L);
        TDebug(Qt::white, Qt::red) << "LUA: ERROR running script " << function << " (" << function << ")\nError: more than " << LUA_FUNCTION_MAX_ARGS
                                   << " arguments passed to Lua function, exceeding Lua's limit. Trimmed arguments to " << LUA_FUNCTION_MAX_ARGS << " only.\n"
                >> &host;
    }

    if (error) {
        std::string err = "";
        if (lua_isstring(L, -1)) {
            err += lua_tostring(L, -1);
        }
        const QString name = "event handler function";
        logError(err, name, function);
        if (mudlet::smDebugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::red) << "LUA: ERROR running script " << function << " (" << function << ")\nError: " << err.c_str() << "\n" >> &host;
        }
    }

    lua_pop(L, lua_gettop(L));
    return !error;
}

// No documentation available in wiki - internal function
double TLuaInterpreter::condenseMapLoad()
{
    QElapsedTimer executionTimer;

    if (mudlet::smDebugMode) {
        executionTimer.start();
    }

    const QString luaFunction = qsl("condenseMapLoad");
    double loadTime = -1.0;

    lua_State* L = pGlobalLua;

    lua_getfield(L, LUA_GLOBALSINDEX, "condenseMapLoad");
    const int error = lua_pcall(L, 0, 1, 0);
    if (error) {
        const int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                const QString _f = luaFunction.toUtf8().constData();
                logError(e, luaFunction, _f);
                if (mudlet::smDebugMode) {
                    auto& host = getHostFromLua(L);
                    TDebug(Qt::white, Qt::red) << "LUA: ERROR running " << luaFunction << " ERROR:" << e.c_str() << "\n" >> &host;
                }
            }
        }
    } else {
        if (mudlet::smDebugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA OK " << luaFunction << " ran without errors in" << (executionTimer.isValid() ? " in " + QString::number(executionTimer.elapsed()) + "ms\n" : ".\n") >> &host;
        }
    }

    const int returnValues = lua_gettop(L);
    if (returnValues > 0 && !lua_isnoneornil(L, 1)) {
        loadTime = lua_tonumber(L, 1);
    }
    lua_pop(L, returnValues);
    return loadTime;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::performHttpRequest(lua_State *L, const char* functionName, const int pos, QNetworkAccessManager::Operation operation, const QString& verb)
{
    auto& host = getHostFromLua(L);

    QString dataToPost;
    if (!lua_isstring(L, pos + 1) && !lua_isstring(L, pos + 4)) {
        lua_pushfstring(L, "%s: bad argument #%d type (data to send as string expected, got %s!)", functionName, pos + 1, luaL_typename(L, pos + 1));
        return lua_error(L);
    }
    if (lua_isstring(L, pos + 1)) {
        dataToPost = lua_tostring(L, pos + 1);
    }

    const QString urlString = getVerifiedString(L, functionName, pos + 2, "remote url");
    const QUrl url = QUrl::fromUserInput(urlString);

    if (!url.isValid()) {
        return warnArgumentValue(L, __func__, qsl("url is invalid, reason: %1.").arg(url.errorString()));
    }

    QNetworkRequest request = QNetworkRequest(url);
    mudlet::self()->setNetworkRequestDefaults(url, request);

    if (!lua_istable(L, pos + 3) && !lua_isnoneornil(L, pos + 3)) {
        lua_pushfstring(L, "%s: bad argument #%d type (headers as a table expected, got %s!)", functionName, pos + 3, luaL_typename(L, 3));
        return lua_error(L);
    }
    if (lua_istable(L, pos + 3)) {
        lua_pushnil(L);
        while (lua_next(L, pos + 3) != 0) {
            // key at index -2 and value at index -1
            if (lua_type(L, -1) == LUA_TSTRING && lua_type(L, -2) == LUA_TSTRING) {
                request.setRawHeader(QByteArray(lua_tostring(L, -2)), QByteArray(lua_tostring(L, -1)));
            } else {
                lua_pushfstring(L,
                                "%s: bad argument #%d type (custom headers must be strings, got header: %s (should be string) and value: %s (should be string))",
                                functionName,
                                pos + 3,
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
    if (!lua_isstring(L, pos + 4) && !lua_isnoneornil(L, pos + 4)) {
        lua_pushfstring(L, "%s: bad argument #%d type (file to send as string location expected, got %s!)", functionName, pos + 4, luaL_typename(L, 4));
        return lua_error(L);
    }
    if (lua_isstring(L, pos + 4)) {
        fileLocation = lua_tostring(L, pos + 4);
    }

    if (!fileLocation.isEmpty()) {
        QFile file(fileLocation);
        if (!file.open(QFile::ReadOnly)) {
            return warnArgumentValue(L, functionName, qsl("couldn't open '%1', is the location correct and do you have permissions to it?").arg(fileLocation));
        }

        fileToUpload = file.readAll();
        file.close();
    }

    host.updateProxySettings(host.mLuaInterpreter.mpFileDownloader);

    QNetworkReply* reply;
    switch (operation) {
        case QNetworkAccessManager::PostOperation:
            reply = host.mLuaInterpreter.mpFileDownloader->post(request, fileToUpload.isEmpty() ?dataToPost.toUtf8() : fileToUpload);
            break;
        case QNetworkAccessManager::PutOperation:
            reply = host.mLuaInterpreter.mpFileDownloader->put(request, fileToUpload.isEmpty() ?dataToPost.toUtf8() : fileToUpload);
            break;
        default:
            reply = host.mLuaInterpreter.mpFileDownloader->sendCustomRequest(request, verb.toUtf8(), fileToUpload.isEmpty() ?dataToPost.toUtf8() : fileToUpload);
    };

    if (mudlet::smDebugMode) {
        TDebug(Qt::white, Qt::blue) << functionName << ": script is uploading data to " << reply->url().toString() << "\n" >> &host;
    }

    lua_pushboolean(L, true);
    lua_pushstring(L, reply->url().toString().toUtf8().constData()); // Returns the Url that was ACTUALLY used
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Networking_Functions#unzipAsync
int TLuaInterpreter::unzipAsync(lua_State *L)
{
    const QString zipLocation = getVerifiedString(L, __func__, 1, "zip location");
    QString extractLocation = getVerifiedString(L, __func__, 2, "extract location");

    const QTemporaryDir temporaryDir;
    if (!temporaryDir.isValid()) {
        return warnArgumentValue(L, __func__, "couldn't create temporary directory to extract the zip into");
    }

    extractLocation = QDir::fromNativeSeparators(extractLocation);
    if (!extractLocation.endsWith(QLatin1String("/"))) {
        extractLocation.append(QLatin1String("/"));
    }

    const QDir dir;
    if (!dir.mkpath(extractLocation)) {
        return warnArgumentValue(L, __func__, "couldn't create output directory to put the extracted files into");
    }

    auto future = QtConcurrent::run(mudlet::unzip, zipLocation, extractLocation, temporaryDir.path());
    auto watcher = new QFutureWatcher<bool>;
    connect(watcher, &QFutureWatcher<bool>::finished, watcher, [=]() {
        TEvent event {};
        Host& host = getHostFromLua(L);

        if (future.result()) {
            event.mArgumentList.append(qsl("sysUnzipDone"));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        } else {
            event.mArgumentList.append(qsl("sysUnzipError"));
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

    lua_pushstring(L, varValue.toUtf8().constData());
    lua_setglobal(L, varName.toUtf8().constData());
    lua_pop(pGlobalLua, lua_gettop(pGlobalLua));
}

// No documentation available in wiki - internal function
void TLuaInterpreter::set_lua_integer(const QString& varName, int varValue)
{
    lua_State* L = pGlobalLua;
    const int top = lua_gettop(L);

    lua_pushnumber(L, varValue);
    lua_setglobal(L, varName.toUtf8().constData());
    lua_settop(L, top);
}

// No documentation available in wiki - internal function
QString TLuaInterpreter::getLuaString(const QString& stringName)
{
    lua_State* L = pGlobalLua;

    const int error = luaL_dostring(L, qsl("return %1").arg(stringName).toUtf8().constData());
    if (!error) {
        return lua_tostring(L, 1);
    } else {
        return QString();
    }
}

// check for <whitespace><no_valid_representation> as output

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#handleWindowResizeEvent is using noop function publicly - compare initLuaGlobals()
int TLuaInterpreter::noop(lua_State* L)
{
    Q_UNUSED(L)
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

    const int r = lua_toboolean(L, -1);
    lua_pop(L, 2);
    return r;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::check_for_custom_speedwalk()
{
    lua_State* L = pGlobalLua;
    lua_getglobal(L, "mudlet");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }

    lua_getfield(L, -1, "custom_speedwalk");
    if (!lua_isboolean(L, -1)) {
        lua_pop(L, 2);
        return 0;
    }
    const int r = lua_toboolean(L, -1);
    lua_pop(L, 2);
    return r;
}

// No documentation available in wiki - internal function
static lua_State* newstate()
{
    return luaL_newstate();
}

// No documentation available in wiki - internal function
static void storeHostInLua(lua_State* L, Host* h);

// No documentation available in wiki - internal helper function
// On success will swap out any messages in the queue and replace them
// with its success message, on failure will just append...
bool TLuaInterpreter::loadLuaModule(QQueue<QString>& resultMsgsQueue, const QString& requirement, const QString& failureConsequence, const QString& description, const QString& luaModuleId)
{
    const int error = luaL_dostring(pGlobalLua, qsl("%1require \"%2\"")
                              .arg(luaModuleId.isEmpty() ? QString() : qsl("%1 =").arg(luaModuleId),
                                   requirement).toUtf8().constData());
    if (error) {
        QString luaErrorMsg = tr("No error message available from Lua");
        if (lua_isstring(pGlobalLua, -1)) {
            luaErrorMsg = tr("Lua error: %1").arg(lua_tostring(pGlobalLua, -1));
        }
        /*:
        %1 is the name of the module;
        %2 will be a line-feed inserted to put the next argument on a new line;
        %3 is the error message from the lua sub-system;
        %4 can be an additional message about the expected effect (but may be blank).
        */
        resultMsgsQueue.enqueue(tr("[ ERROR ] - Cannot find Lua module %1.%2%3%4")
                                .arg((description.isEmpty() ? requirement : description),
                                     QLatin1String("\n"),
                                     luaErrorMsg,
                                     (failureConsequence.isEmpty() ? QString() : qsl("\n%1").arg(failureConsequence))));
        return false;
    }

    // clear out the queue in case any previous attempts to load the module of the same name failed
    QQueue<QString> newMessageQueue;
    // uncomment to show Lua module messages for debugging
    // newMessageQueue.enqueue(tr("[  OK  ]  - Lua module %1 loaded.",
    //                         "%1 is the name (may specify which variant) of the module.")
    //                     .arg(description.isEmpty() ? requirement : description));
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
    lua_register(pGlobalLua, "echo", TLuaInterpreter::echo);
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
    lua_register(pGlobalLua, "sendCmdLine", TLuaInterpreter::sendCmdLine);
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
    lua_register(pGlobalLua, "errorc", TLuaInterpreter::errorc);
    lua_register(pGlobalLua, "showHandlerError", TLuaInterpreter::showHandlerError);
    lua_register(pGlobalLua, "setWindowWrap", TLuaInterpreter::setWindowWrap);
    lua_register(pGlobalLua, "getWindowWrap", TLuaInterpreter::getWindowWrap);
    lua_register(pGlobalLua, "setWindowWrapIndent", TLuaInterpreter::setWindowWrapIndent);
    lua_register(pGlobalLua, "setWindowWrapHangingIndent", TLuaInterpreter::setWindowWrapHangingIndent);
    lua_register(pGlobalLua, "resetFormat", TLuaInterpreter::resetFormat);
    lua_register(pGlobalLua, "moveCursorEnd", TLuaInterpreter::moveCursorEnd);
    lua_register(pGlobalLua, "getLastLineNumber", TLuaInterpreter::getLastLineNumber);
    lua_register(pGlobalLua, "getNetworkLatency", TLuaInterpreter::getNetworkLatency);
    lua_register(pGlobalLua, "createMiniConsole", TLuaInterpreter::createMiniConsole);
    lua_register(pGlobalLua, "createScrollBox", TLuaInterpreter::createScrollBox);
    lua_register(pGlobalLua, "createLabel", TLuaInterpreter::createLabel);
    lua_register(pGlobalLua, "deleteLabel", TLuaInterpreter::deleteLabel);
    lua_register(pGlobalLua, "deleteMiniConsole", TLuaInterpreter::deleteMiniConsole);
    lua_register(pGlobalLua, "deleteCommandLine", TLuaInterpreter::deleteCommandLine);
    lua_register(pGlobalLua, "deleteScrollBox", TLuaInterpreter::deleteScrollBox);
    lua_register(pGlobalLua, "setLabelToolTip", TLuaInterpreter::setLabelToolTip);
    lua_register(pGlobalLua, "setLabelCursor", TLuaInterpreter::setLabelCursor);
    lua_register(pGlobalLua, "setLabelCustomCursor", TLuaInterpreter::setLabelCustomCursor);
    lua_register(pGlobalLua, "raiseWindow", TLuaInterpreter::raiseWindow);
    lua_register(pGlobalLua, "lowerWindow", TLuaInterpreter::lowerWindow);
    lua_register(pGlobalLua, "hideWindow", TLuaInterpreter::hideWindow);
    lua_register(pGlobalLua, "showWindow", TLuaInterpreter::showWindow);
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
    lua_register(pGlobalLua, "resetBackgroundImage", TLuaInterpreter::resetBackgroundImage);
    lua_register(pGlobalLua, "setBackgroundColor", TLuaInterpreter::setBackgroundColor);
    lua_register(pGlobalLua, "setCommandBackgroundColor", TLuaInterpreter::setCommandBackgroundColor);
    lua_register(pGlobalLua, "setCommandForegroundColor", TLuaInterpreter::setCommandForegroundColor);
    lua_register(pGlobalLua, "setCmdLineAction", TLuaInterpreter::setCmdLineAction);
    lua_register(pGlobalLua, "resetCmdLineAction", TLuaInterpreter::resetCmdLineAction);
    lua_register(pGlobalLua, "setCmdLineStyleSheet", TLuaInterpreter::setCmdLineStyleSheet);
    lua_register(pGlobalLua, "setLabelClickCallback", TLuaInterpreter::setLabelClickCallback);
    lua_register(pGlobalLua, "setLabelDoubleClickCallback", TLuaInterpreter::setLabelDoubleClickCallback);
    lua_register(pGlobalLua, "setLabelReleaseCallback", TLuaInterpreter::setLabelReleaseCallback);
    lua_register(pGlobalLua, "setLabelMoveCallback", TLuaInterpreter::setLabelMoveCallback);
    lua_register(pGlobalLua, "setLabelWheelCallback", TLuaInterpreter::setLabelWheelCallback);
    lua_register(pGlobalLua, "setLabelOnEnter", TLuaInterpreter::setLabelOnEnter);
    lua_register(pGlobalLua, "setLabelOnLeave", TLuaInterpreter::setLabelOnLeave);
    lua_register(pGlobalLua, "setMovie", TLuaInterpreter::setMovie);
    lua_register(pGlobalLua, "startMovie", TLuaInterpreter::startMovie);
    lua_register(pGlobalLua, "setMovieSpeed", TLuaInterpreter::setMovieSpeed);
    lua_register(pGlobalLua, "scaleMovie", TLuaInterpreter::scaleMovie);
    lua_register(pGlobalLua, "setMovieFrame", TLuaInterpreter::setMovieFrame);
    lua_register(pGlobalLua, "pauseMovie", TLuaInterpreter::pauseMovie);
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
    lua_register(pGlobalLua, "setMiniConsoleFontSize", TLuaInterpreter::setFontSize);
    lua_register(pGlobalLua, "selectCurrentLine", TLuaInterpreter::selectCurrentLine);
    lua_register(pGlobalLua, "spawn", TLuaInterpreter::spawn);
    lua_register(pGlobalLua, "getButtonState", TLuaInterpreter::getButtonState);
    lua_register(pGlobalLua, "setButtonState", TLuaInterpreter::setButtonState);
    lua_register(pGlobalLua, "showToolBar", TLuaInterpreter::showToolBar);
    lua_register(pGlobalLua, "hideToolBar", TLuaInterpreter::hideToolBar);
    lua_register(pGlobalLua, "loadRawFile", TLuaInterpreter::loadReplay);
    lua_register(pGlobalLua, "loadReplay", TLuaInterpreter::loadReplay);
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
    lua_register(pGlobalLua, "receiveMSP", TLuaInterpreter::receiveMSP);
    lua_register(pGlobalLua, "loadSoundFile", TLuaInterpreter::loadSoundFile);
    lua_register(pGlobalLua, "loadMusicFile", TLuaInterpreter::loadMusicFile);
    lua_register(pGlobalLua, "loadVideoFile", TLuaInterpreter::loadVideoFile);
    lua_register(pGlobalLua, "playSoundFile", TLuaInterpreter::playSoundFile);
    lua_register(pGlobalLua, "playMusicFile", TLuaInterpreter::playMusicFile);
    lua_register(pGlobalLua, "playVideoFile", TLuaInterpreter::playVideoFile);
    lua_register(pGlobalLua, "getPlayingMusic", TLuaInterpreter::getPlayingMusic);
    lua_register(pGlobalLua, "getPlayingSounds", TLuaInterpreter::getPlayingSounds);
    lua_register(pGlobalLua, "getPlayingVideos", TLuaInterpreter::getPlayingVideos);
    lua_register(pGlobalLua, "getPausedSounds", TLuaInterpreter::getPausedSounds);
    lua_register(pGlobalLua, "getPausedMusic", TLuaInterpreter::getPausedMusic);
    lua_register(pGlobalLua, "getPausedVideos", TLuaInterpreter::getPausedVideos);
    lua_register(pGlobalLua, "stopMusic", TLuaInterpreter::stopMusic);
    lua_register(pGlobalLua, "stopSounds", TLuaInterpreter::stopSounds);
    lua_register(pGlobalLua, "stopVideos", TLuaInterpreter::stopVideos);
    lua_register(pGlobalLua, "pauseSounds", TLuaInterpreter::pauseSounds);
    lua_register(pGlobalLua, "pauseMusic", TLuaInterpreter::pauseMusic);
    lua_register(pGlobalLua, "pauseVideos", TLuaInterpreter::pauseVideos);
    lua_register(pGlobalLua, "purgeMediaCache", TLuaInterpreter::purgeMediaCache);
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
    lua_register(pGlobalLua, "getConsoleBufferSize", TLuaInterpreter::getConsoleBufferSize);
    lua_register(pGlobalLua, "setConsoleBufferSize", TLuaInterpreter::setConsoleBufferSize);
    lua_register(pGlobalLua, "enableScrollBar", TLuaInterpreter::enableScrollBar);
    lua_register(pGlobalLua, "disableScrollBar", TLuaInterpreter::disableScrollBar);
    lua_register(pGlobalLua, "enableHorizontalScrollBar", TLuaInterpreter::enableHorizontalScrollBar);
    lua_register(pGlobalLua, "disableHorizontalScrollBar", TLuaInterpreter::disableHorizontalScrollBar);
    lua_register(pGlobalLua, "enableCommandLine", TLuaInterpreter::enableCommandLine);
    lua_register(pGlobalLua, "disableCommandLine", TLuaInterpreter::disableCommandLine);
    lua_register(pGlobalLua, "startLogging", TLuaInterpreter::startLogging);
    lua_register(pGlobalLua, "appendLog", TLuaInterpreter::appendLog);
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
    lua_register(pGlobalLua, "isAncestorsActive", TLuaInterpreter::isAncestorsActive);
    lua_register(pGlobalLua, "ancestors", TLuaInterpreter::ancestors);
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
    lua_register(pGlobalLua, "feedTelnet", TLuaInterpreter::feedTelnet);
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
    lua_register(pGlobalLua, "getAreaRooms1", TLuaInterpreter::getAreaRooms1);
    lua_register(pGlobalLua, "getPath", TLuaInterpreter::getPath);
#if defined(INCLUDE_3DMAPPER)
    lua_register(pGlobalLua, "shiftMapPerspective", TLuaInterpreter::shiftMapPerspective);
    lua_register(pGlobalLua, "setMapPerspective", TLuaInterpreter::setMapPerspective);
#endif
    lua_register(pGlobalLua, "centerview", TLuaInterpreter::centerview);
    lua_register(pGlobalLua, "denyCurrentSend", TLuaInterpreter::denyCurrentSend);
    lua_register(pGlobalLua, "tempBeginOfLineTrigger", TLuaInterpreter::tempBeginOfLineTrigger);
    lua_register(pGlobalLua, "tempExactMatchTrigger", TLuaInterpreter::tempExactMatchTrigger);
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
    lua_register(pGlobalLua, "selectCmdLineText", TLuaInterpreter::selectCmdLineText);
    lua_register(pGlobalLua, "getCmdLine", TLuaInterpreter::getCmdLine);
    lua_register(pGlobalLua, "addCmdLineSuggestion", TLuaInterpreter::addCmdLineSuggestion);
    lua_register(pGlobalLua, "removeCmdLineSuggestion", TLuaInterpreter::removeCmdLineSuggestion);
    lua_register(pGlobalLua, "clearCmdLineSuggestions", TLuaInterpreter::clearCmdLineSuggestions);
    lua_register(pGlobalLua, "addCmdLineBlacklist", TLuaInterpreter::addCmdLineBlacklist);
    lua_register(pGlobalLua, "removeCmdLineBlacklist", TLuaInterpreter::removeCmdLineBlacklist);
    lua_register(pGlobalLua, "clearCmdLineBlacklist", TLuaInterpreter::clearCmdLineBlacklist);
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
    lua_register(pGlobalLua, "setRoomCharColor", TLuaInterpreter::setRoomCharColor);
    lua_register(pGlobalLua, "unsetRoomCharColor", TLuaInterpreter::unsetRoomCharColor);
    lua_register(pGlobalLua, "getRoomCharColor", TLuaInterpreter::getRoomCharColor);
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
    lua_register(pGlobalLua, "getExitStubsNames", TLuaInterpreter::getExitStubsNames);
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
    lua_register(pGlobalLua, "getModules", TLuaInterpreter::getModules);
    lua_register(pGlobalLua, "getPackages", TLuaInterpreter::getPackages);
    lua_register(pGlobalLua, "getModuleInfo", TLuaInterpreter::getModuleInfo);
    lua_register(pGlobalLua, "getPackageInfo", TLuaInterpreter::getPackageInfo);
    lua_register(pGlobalLua, "setModuleInfo", TLuaInterpreter::setModuleInfo);
    lua_register(pGlobalLua, "setPackageInfo", TLuaInterpreter::setPackageInfo);
    lua_register(pGlobalLua, "createMapImageLabel", TLuaInterpreter::createMapImageLabel);
    lua_register(pGlobalLua, "setMapZoom", TLuaInterpreter::setMapZoom);
    lua_register(pGlobalLua, "getMapZoom", TLuaInterpreter::getMapZoom);
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
    lua_register(pGlobalLua, "getCustomLines1", TLuaInterpreter::getCustomLines1);
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
    lua_register(pGlobalLua, "getProcessID", TLuaInterpreter::getProcessID);
    lua_register(pGlobalLua, "getClipboardText", TLuaInterpreter::getClipboardText);
    lua_register(pGlobalLua, "setClipboardText", TLuaInterpreter::setClipboardText);
    lua_register(pGlobalLua, "getAvailableFonts", TLuaInterpreter::getAvailableFonts);
    lua_register(pGlobalLua, "tempAnsiColorTrigger", TLuaInterpreter::tempAnsiColorTrigger);
    lua_register(pGlobalLua, "setDiscordApplicationID", TLuaInterpreter::setDiscordApplicationID);
    lua_register(pGlobalLua, "setDiscordGameUrl", TLuaInterpreter::setDiscordGameUrl);
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
    lua_register(pGlobalLua, "resetDiscordData", TLuaInterpreter::resetDiscordData);
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
    lua_register(pGlobalLua, "getCharacterName", TLuaInterpreter::getCharacterName);
    lua_register(pGlobalLua, "getProfileInformation", TLuaInterpreter::getProfileInformation);
    lua_register(pGlobalLua, "setProfileInformation", TLuaInterpreter::setProfileInformation);
    lua_register(pGlobalLua, "clearProfileInformation", TLuaInterpreter::clearProfileInformation);
    lua_register(pGlobalLua, "getWindowsCodepage", TLuaInterpreter::getWindowsCodepage);
    lua_register(pGlobalLua, "getHTTP", TLuaInterpreter::getHTTP);
    lua_register(pGlobalLua, "customHTTP", TLuaInterpreter::customHTTP);
    lua_register(pGlobalLua, "putHTTP", TLuaInterpreter::putHTTP);
    lua_register(pGlobalLua, "postHTTP", TLuaInterpreter::postHTTP);
    lua_register(pGlobalLua, "deleteHTTP", TLuaInterpreter::deleteHTTP);
    lua_register(pGlobalLua, "getConnectionInfo", TLuaInterpreter::getConnectionInfo);
    lua_register(pGlobalLua, "unzipAsync", TLuaInterpreter::unzipAsync);
    lua_register(pGlobalLua, "setMapWindowTitle", TLuaInterpreter::setMapWindowTitle);
    lua_register(pGlobalLua, "getMudletInfo", TLuaInterpreter::getMudletInfo);
    lua_register(pGlobalLua, "getMapBackgroundColor", TLuaInterpreter::getMapBackgroundColor);
    lua_register(pGlobalLua, "setMapBackgroundColor", TLuaInterpreter::setMapBackgroundColor);
    lua_register(pGlobalLua, "getMapRoomExitsColor", TLuaInterpreter::getMapRoomExitsColor);
    lua_register(pGlobalLua, "setMapRoomExitsColor", TLuaInterpreter::setMapRoomExitsColor);
    lua_register(pGlobalLua, "showNotification", TLuaInterpreter::showNotification);
    lua_register(pGlobalLua, "saveJsonMap", TLuaInterpreter::saveJsonMap);
    lua_register(pGlobalLua, "loadJsonMap", TLuaInterpreter::loadJsonMap);
    lua_register(pGlobalLua, "registerMapInfo", TLuaInterpreter::registerMapInfo);
    lua_register(pGlobalLua, "killMapInfo", TLuaInterpreter::killMapInfo);
    lua_register(pGlobalLua, "enableMapInfo", TLuaInterpreter::enableMapInfo);
    lua_register(pGlobalLua, "disableMapInfo", TLuaInterpreter::disableMapInfo);
    lua_register(pGlobalLua, "getProfileTabNumber", TLuaInterpreter::getProfileTabNumber);
    lua_register(pGlobalLua, "addFileWatch", TLuaInterpreter::addFileWatch);
    lua_register(pGlobalLua, "removeFileWatch", TLuaInterpreter::removeFileWatch);
    lua_register(pGlobalLua, "addMouseEvent", TLuaInterpreter::addMouseEvent);
    lua_register(pGlobalLua, "removeMouseEvent", TLuaInterpreter::removeMouseEvent);
    lua_register(pGlobalLua, "getMouseEvents", TLuaInterpreter::getMouseEvents);
    lua_register(pGlobalLua, "setConfig", TLuaInterpreter::setConfig);
    lua_register(pGlobalLua, "addCommandLineMenuEvent", TLuaInterpreter::addCommandLineMenuEvent);
    lua_register(pGlobalLua, "removeCommandLineMenuEvent", TLuaInterpreter::removeCommandLineMenuEvent);
    lua_register(pGlobalLua, "deleteMap", TLuaInterpreter::deleteMap);
    lua_register(pGlobalLua, "windowType", TLuaInterpreter::windowType);
    lua_register(pGlobalLua, "getProfileStats", TLuaInterpreter::getProfileStats);
    lua_register(pGlobalLua, "getBackgroundColor", TLuaInterpreter::getBackgroundColor);
    lua_register(pGlobalLua, "getLabelStyleSheet", TLuaInterpreter::getLabelStyleSheet);
    lua_register(pGlobalLua, "getLabelSizeHint", TLuaInterpreter::getLabelSizeHint);
    lua_register(pGlobalLua, "announce", TLuaInterpreter::announce);
    lua_register(pGlobalLua, "scrollTo", TLuaInterpreter::scrollTo);
    lua_register(pGlobalLua, "getScroll", TLuaInterpreter::getScroll);
    lua_register(pGlobalLua, "getConfig", TLuaInterpreter::getConfig);
    lua_register(pGlobalLua, "setSaveCommandHistory", TLuaInterpreter::setSaveCommandHistory);
    lua_register(pGlobalLua, "getSaveCommandHistory", TLuaInterpreter::getSaveCommandHistory);
    lua_register(pGlobalLua, "enableScrolling", TLuaInterpreter::enableScrolling);
    lua_register(pGlobalLua, "disableScrolling", TLuaInterpreter::disableScrolling);
    lua_register(pGlobalLua, "clearMapSelection", TLuaInterpreter::clearMapSelection);
    lua_register(pGlobalLua, "scrollingActive", TLuaInterpreter::scrollingActive);
    lua_register(pGlobalLua, "findItems", TLuaInterpreter::findItems);
    lua_register(pGlobalLua, "holdingModifiers", TLuaInterpreter::holdingModifiers);
    lua_register(pGlobalLua, "getProfiles", TLuaInterpreter::getProfiles);
    lua_register(pGlobalLua, "loadProfile", TLuaInterpreter::loadProfile);
    lua_register(pGlobalLua, "closeProfile", TLuaInterpreter::closeProfile);
    lua_register(pGlobalLua, "getCollisionLocationsInArea", TLuaInterpreter::getCollisionLocationsInArea);
    lua_register(pGlobalLua, "exportAreaImage", TLuaInterpreter::exportAreaImage);
    lua_register(pGlobalLua, "disableTimeStamps", TLuaInterpreter::disableTimeStamps);
    lua_register(pGlobalLua, "enableTimeStamps", TLuaInterpreter::enableTimeStamps);
    lua_register(pGlobalLua, "timeStampsEnabled", TLuaInterpreter::timeStampsEnabled);
    lua_register(pGlobalLua, "aiChat", TLuaInterpreter::aiChat);
    lua_register(pGlobalLua, "aiPrompt", TLuaInterpreter::aiPrompt);
    lua_register(pGlobalLua, "aiPromptStream", TLuaInterpreter::aiPromptStream);
    lua_register(pGlobalLua, "setActiveProfile", TLuaInterpreter::setActiveProfile);
    // PLACEMARKER: End of main Lua interpreter functions registration
    // check new functions against https://www.linguistic-antipatterns.com when creating them

    QStringList additionalLuaPaths;
    QStringList additionalCPaths;
    const auto appPath{QCoreApplication::applicationDirPath()};
    const auto profilePath{mudlet::getMudletPath(enums::profileHomePath, hostName)};

    // Allow for modules or libraries placed in the profile root directory:
    additionalLuaPaths << qsl("%1/?.lua").arg(profilePath);
    additionalLuaPaths << qsl("%1/?/init.lua").arg(profilePath);
#if defined(Q_OS_WINDOWS)
    additionalCPaths << qsl("%1/?.dll").arg(profilePath);
#else
    additionalCPaths << qsl("%1/?.so").arg(profilePath);
#endif

#if defined(Q_OS_LINUX)
    // AppInstaller on Linux would like the C search path to also be set to
    // a ./lib sub-directory of the current binary directory:
    additionalCPaths << qsl("%1/lib/?.so").arg(appPath);
#elif defined(Q_OS_MACOS)
    // macOS app bundle would like the search path to also be set to the current
    // binary directory for both modules and binary libraries:
    additionalCPaths << qsl("%1/?.so").arg(appPath);
    additionalLuaPaths << qsl("%1/?.lua").arg(appPath);

    // Luarocks installs rocks locally for developers, even with sudo
    additionalCPaths << qsl("%1/.luarocks/lib/lua/5.1/?.so").arg(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
    additionalLuaPaths << qsl("%1/.luarocks/share/lua/5.1/?.lua;%1/.luarocks/share/lua/5.1/?/init.lua").arg(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
#endif

    insertNativeSeparatorsFunction(pGlobalLua);

    luaL_dostring(pGlobalLua, qsl("package.cpath = toNativeSeparators([[%1;]]) .. package.cpath").arg(additionalCPaths.join(QLatin1Char(';'))).toUtf8().constData());
    luaL_dostring(pGlobalLua, qsl("package.path = toNativeSeparators([[%1;]]) .. package.path").arg(additionalLuaPaths.join(QLatin1Char(';'))).toUtf8().constData());

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
     *     alternative, when omitted or a null/empty QString the second
     *     argument is used instead)
     * 5 - string used in Lua as the identifier for the loaded module/library,
     *     passed as X in the lua 'X = require("name")' (optional, in which
     *     case nothing is put before the "require" in that usage and the module
     *     assumes whatever Lua name it offers).
     */
    QQueue<QString> modLoadMessageQueue;
    loadLuaModule(modLoadMessageQueue, QLatin1String("lfs"), tr("Probably will not be able to access Mudlet Lua code."), QLatin1String("lfs (Lua File System)"));
    while (!modLoadMessageQueue.isEmpty()) {
        mpHost->postMessage(modLoadMessageQueue.dequeue());
    }

    bool loaded = loadLuaModule(modLoadMessageQueue, QLatin1String("brimworks.zip"), QString(), qsl("lua-zip"), qsl("zip"));
    if (!loaded) {
        loadLuaModule(modLoadMessageQueue, QLatin1String("zip"), QString(), qsl("luazip"));
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
lua_State* TLuaInterpreter::getLuaGlobalState()
{
    return pGlobalLua;
}

// No documentation available in wiki - internal function
void TLuaInterpreter::setupLanguageData()
{
    // Creates a 'mudlet.translations' table with directions
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
void TLuaInterpreter::initIndenterGlobals()
{
    Q_ASSERT_X(!pIndenterState, "TLuaInterpreter::initIndenterGlobals()", "Indenter state is already initialized - re-initializing it is very expensive!");


    // Initialise a slimmed-down Lua state just to run the indenter in a separate sandbox.
    // The indenter by default pollutes the global environment with some utility functions
    // and we don't want to tie ourselves to it by exposing them for scripting.
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
    lua_register(pIndenterState.get(), "echo", TLuaInterpreter::echo);
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
    additionalCPaths << qsl("%1/?.so").arg(appPath);
#elif defined (Q_OS_LINUX)
    // AppInstaller on Linux would like the search path for the binary modules
    // to also be set to a lib sub-directory of the application directory:
    additionalCPaths << qsl("%1/lib/?.so").arg(appPath);
#endif

    insertNativeSeparatorsFunction(pIndenterState.get());

    // 1 installed *nix case - probably not applicable to Windows
    //     "LUA_DEFAULT_PATH/?.lua" (if defined and not empty)
    if (!qsl(LUA_DEFAULT_PATH).isEmpty()) {
        additionalLuaPaths << qsl(LUA_DEFAULT_PATH "/?.lua");
    }
    // 2 AppImage (directory of executable) - not needed for Wndows:
    //     "<applicationDirectory>/?.lua"
#if ! defined (Q_OS_WINDOWS)
    additionalLuaPaths << qsl("%1/?.lua").arg(appPath);
#endif
    // 3 QMake shadow builds without CONFIG containing "debug_and_release" but
    //    with "debug_and_release_target" (default on most OS but NOT Windows):
    //     "<applicationDirectory>/../3rdparty/?.lua"
    additionalLuaPaths << qsl("%1/../3rdparty/?.lua").arg(appPath);
    // 4 QMake shadow builds with CONFIG containing "debug_and_release" AND
    //   "debug_and_release_target" (usually Windows):
    //     "<applicationDirectory>/../../3rdparty/?.lua"
    additionalLuaPaths << qsl("%1/../../3rdparty/?.lua").arg(appPath);
    // 5 CMake shadow builds
    //    "<applicationDirectory>/../../mudlet/3rdparty/?.lua"
    additionalLuaPaths << qsl("%1/../../mudlet/3rdparty/?.lua").arg(appPath);

    int error = luaL_dostring(pIndenterState.get(), qsl("package.path = toNativeSeparators([[%1;]] .. package.path)")
                              .arg(additionalLuaPaths.join(QLatin1Char(';'))).toUtf8().constData());
    if (!error && !additionalCPaths.isEmpty()) {
        error = luaL_dostring(pIndenterState.get(), qsl("package.cpath = toNativeSeparators([[%1;]] .. package.cpath)")
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
            e = tr("Lua error: %1.").arg(lua_tostring(pIndenterState.get(), -1));
        }
        QString msg = qsl("%1\n").arg(tr("[ ERROR ] - Cannot load code formatter, indenting functionality won't be available."));
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
#if defined(Q_OS_WINDOWS)
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
        qsl("%1/../Resources/mudlet-lua/lua/LuaGlobal.lua").arg(executablePath),
#endif

        // For the installer we put the lua files under the executable's
        // location. This is the case for the Windows install:
        QDir::toNativeSeparators(qsl("%1/mudlet-lua/lua/LuaGlobal.lua").arg(executablePath)),

        // Although a no-op for an in source build an additional "../src/"
        // allows location of lua code when object code is in a directory
        // alongside the src directory as occurs using Qt Creator "Shadow
        // Builds":
        QDir::toNativeSeparators(qsl("%1/../src/mudlet-lua/lua/LuaGlobal.lua").arg(executablePath)),

        // Windows builds (or others where the qmake project file has CONFIG
        // containing debug_and_release AND debug_and_release_target options)
        // may be an additional sub-directory down:
        QDir::toNativeSeparators(qsl("%1/../../src/mudlet-lua/lua/LuaGlobal.lua").arg(executablePath)),

        // CMake builds done from Qt Creator tend to make their build directory
        // be in a "out-of-source" (the more common name for what Qt calls
        // "Shadow Builds") on the same level as the top level CMakeList.txt
        // project file - which is one level up compared to the QMake case.
        // and in a "src" subdirectory (to match the relative source file
        // location to that top-level project file) of the main project
        // "mudlet" directory:
        QDir::toNativeSeparators(qsl("%1/../../../src/mudlet-lua/lua/LuaGlobal.lua").arg(executablePath)),

        // CMake builds from Qt Creator on Windows 11 appear to use this directory:
        QDir::toNativeSeparators(qsl("%1/../../../mudlet-lua/lua/LuaGlobal.lua").arg(executablePath))
    };

    // Although it is relatively easy to detect whether something is #define d
    // it is not so easy to detect what it contains at the preprocessor stage,
    // so leave checking for its contents to run-time - this one is the one
    // for Linux/FreeBSD where the read-only shared Lua files go into the
    // /usr/share part of the file-system:
    if (!qsl(LUA_DEFAULT_PATH).isEmpty()) {
        mPossiblePaths <<  QDir::toNativeSeparators(qsl(LUA_DEFAULT_PATH "/LuaGlobal.lua"));
    };
    QStringList failedMessages{};

    // uncomment the following to enable some debugging texts in the LuaGlobal.lua script:
    // luaL_dostring(pGlobalLua, qsl("debugLoading = true").toUtf8().constData());

#if defined(Q_OS_WINDOWS)
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
    // Needed to enable permissions checks on NTFS file systems - normally
    // turned off for performance reasons:
    extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif
#endif

    int error;
    for (const auto& pathFileName : std::as_const(mPossiblePaths)) {
        if (!(QFileInfo::exists(pathFileName))) {
            failedMessages << tr("%1 (doesn't exist)", "This file doesn't exist").arg(pathFileName);
            continue;
        }

        if (!(QFileInfo(pathFileName).isFile())) {
            failedMessages << tr("%1 (isn't a file or symlink to a file)").arg(pathFileName);
            continue;
        }

#if defined(Q_OS_WINDOWS)
        // Turn on permission checking on NTFS file systems
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
        qt_ntfs_permission_lookup++;
#else
        qEnableNtfsPermissionChecks();
#endif
#endif
        if (!(QFileInfo(pathFileName).isReadable())) {
            failedMessages << tr("%1 (isn't a readable file or symlink to a readable file)").arg(pathFileName);
            continue;
        }
#if defined(Q_OS_WINDOWS)
        // Turn off permission checking on NTFS file systems
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
        qt_ntfs_permission_lookup--;
#else
        qDisableNtfsPermissionChecks();
#endif
#endif

        // Leave a global variable set to the path so we can use it to find the
        // other files around it - the script will convert the directory
        // separators as necessary:
        Q_ASSERT_X(!pathFileName.isEmpty(), "TLuaInterpreter::loadGlobal()", "trying to call QFileInfo(path).absolutePath() when path is empty");
        luaL_dostring(pGlobalLua, qsl("luaGlobalPath = \"%1\"").arg(QFileInfo(pathFileName).absolutePath()).toUtf8().constData());

        // load via Qt so UTF8 paths work on Windows - Lua can't handle it
        auto luaGlobal = readScriptFile(pathFileName);

        if (luaGlobal.isEmpty()) {
            failedMessages << tr("%1 (couldn't read file)", "This file could not be read for some reason (for example, no permission)").arg(pathFileName);
            continue;
        }

        error = luaL_dostring(pGlobalLua, luaGlobal.toUtf8().constData());
        if (!error) {
            return;
        }
        qWarning() << "TLuaInterpreter::loadGlobal() loading " << pathFileName << " failed: " << lua_tostring(pGlobalLua, -1);
        failedMessages << qsl("%1 (%2)").arg(pathFileName, lua_tostring(pGlobalLua, -1));
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

    /*
     * FIXME: Qt Documentation for this method reports:
     * "Reads the entire content of the stream, and returns it as a QString.
     * Avoid this function when working on large files, as it will consume a
     * significant amount of memory.
     *
     * Calling readLine() is better if you do not know how much data is
     * available."
     */
    QString text = in.readAll();
    file.close();

    return text;
}

#if defined(Q_OS_WINDOWS)
// No documentation available in wiki - internal function
// loads utf8_filenames from the resource system directly so it is not affected by
// non-ASCII characters that might be present in the users filesystem
void TLuaInterpreter::loadUtf8Filenames()
{
    auto path = qsl(":/mudlet-lua/lua/utf8_filenames.lua");
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
std::pair<int, QString> TLuaInterpreter::createPermScript(const QString& name, const QString& parent, const QString& luaCode)
{
    TScript* pS;
    if (parent.isEmpty()) {
        pS = new TScript(qsl("newPermScriptWithoutAnId"), mpHost);
    } else {
        // FIXME: There can be more than one script with the same name - we will
        // use only the FIRST one for now, but we really ought to enhance the
        // API to handle more than one potential parent with the same name:
        auto ids = mpHost->getScriptUnit()->findItems(parent);
        if (ids.empty()) {
            return {-1, qsl("parent '%1' not found").arg(parent)}; //parent not found
        }
        auto pParentScript = mpHost->getScriptUnit()->getScript(ids.at(0));
        pS = new TScript(pParentScript, mpHost);
    }
    pS->setIsFolder((luaCode.isEmpty()));
    pS->setName(name);
    // This will lead to the generation of the ID number:
    mpHost->getScriptUnit()->registerScript(pS);
    if (!pS->setScript(luaCode)) {
        const QString errMsg = pS->getError();
        delete pS;
        return {-1, qsl("unable to compile \"%1\", reason: %2").arg(luaCode, errMsg)};
    }

    const int id = pS->getID();
    pS->setIsActive(false);
    updateEditor();
    return {id, QString()};
}

// No documentation available in wiki - internal function
// pos is 0 for the first script with the matching name so needs
// to be incremented if it is to be referred to in an error message, but can
// be used to directly index into the QVector<int> that is "ids".
std::pair<int, QString> TLuaInterpreter::setScriptCode(const QString& name, const QString& luaCode, const int pos)
{
    if (name.isEmpty()) {
        return {-1, qsl("cannot have an empty string as name")};
    }

    const auto ids = mpHost->getScriptUnit()->findItems(name);
    int id = -1;
    TScript* pS = nullptr;
    if (pos >= 0 && pos < static_cast<ptrdiff_t>(ids.size())) {
        id = ids.at(pos);
        pS = mpHost->getScriptUnit()->getScript(id);
    }
    if (!pS) {
        return {-1, qsl("script \"%1\" at position %2 not found").arg(name, QString::number(pos + 1))}; //script not found
    }
    const auto oldCode = pS->getScript();
    if (!pS->setScript(luaCode)) {
        const QString errMsg = pS->getError();
        pS->setScript(oldCode);
        return {-1, qsl("unable to compile \"%1\" for the script \"%2\" at position %3, reason: %4").arg(luaCode, name, QString::number(pos + 1), errMsg)};
    }
    if (mpHost->mpEditorDialog) {
        mpHost->mpEditorDialog->writeScript(id);
    }
    return {id, QString()};
}

// No documentation available in wiki - internal function
std::pair<int, QString> TLuaInterpreter::startPermTimer(const QString& name, const QString& parent, double timeout, const QString& function)
{
    const QTime time = QTime(0, 0, 0, 0).addMSecs(qRound(timeout * 1000));
    TTimer* pT;
    if (parent.isEmpty()) {
        pT = new TTimer(qsl("newPermTimerWithoutAnId"), time, mpHost);
    } else {
        // FIXME: There can be more than one timer with the same name - we will
        // use only the FIRST one for now, but we really ought to enhance the
        // API to handle more than one potential parent with the same name:
        auto pParentTimer = mpHost->getTimerUnit()->findFirstTimer(parent);
        if (!pParentTimer) {
            return {-1, qsl("parent '%1' not found").arg(parent)};
        }
        pT = new TTimer(pParentTimer, mpHost);
    }
    pT->setTime(time);
    pT->setIsFolder((timeout == 0 && function.isEmpty()));
    pT->setTemporary(false);
    // The name should be set after isTempTimer, as that is faster.
    // Also for permanent timers it is easier to debug if it is set before
    // registration:
    pT->setName(name);
    // This will lead to the generation of the ID number:
    mpHost->getTimerUnit()->registerTimer(pT);
    if (!pT->setScript(function)) {
        const QString errMsg = pT->getError();
        // Apparently this will call the TTimer::unregisterTimer(...) method:
        delete pT;
        return {-1, qsl("unable to compile \"%1\", reason: %2").arg(function, errMsg)};
    }

    pT->setIsActive(false);
    updateEditor();
    return {pT->getID(), QString()};
}

// No documentation available in wiki - internal function
QPair<int, QString> TLuaInterpreter::startTempTimer(double timeout, const QString& function, const bool repeating)
{
    const QTime time = QTime(0,0,0,0).addMSecs(qRound(timeout * 1000));
    auto* pT = new TTimer(qsl("newTempTimerWithoutAnId"), time, mpHost, repeating);
    pT->setTime(time);
    pT->setIsFolder(false);
    pT->setTemporary(true);
    // This call leads to the allocation of an ID number - and its use as the
    // name for temporary timers:
    mpHost->getTimerUnit()->registerTimer(pT);
    if (!pT->setScript(function)) {
        const QString errMsg = pT->getError();
        // Apparently this will call the TTimer::unregisterTimer(...) method:
        delete pT;
        return qMakePair(-1, qsl("unable to compile \"%1\", reason: %2").arg(function, errMsg));
    }

    const int id = pT->getID();
    pT->setIsActive(true);
    pT->enableTimer(id);
    return qMakePair(id, QString());
}

// No documentation available in wiki - internal function
std::pair<int, QString> TLuaInterpreter::startPermAlias(const QString& name, const QString& parent, const QString& regex, const QString& function)
{
    TAlias* pT;

    if (parent.isEmpty()) {
        pT = new TAlias("a", mpHost);
    } else {
        TAlias* pP = mpHost->getAliasUnit()->findFirstAlias(parent);
        if (!pP) {
            return {-1, qsl("parent '%1' not found").arg(parent)};
        }
        pT = new TAlias(pP, mpHost);
    }
    pT->setRegexCode(regex);
    pT->setIsFolder((regex.isEmpty() && function.isEmpty()));
    pT->setIsActive(true);
    pT->setTemporary(false);
    pT->registerAlias();
    pT->setScript(function);
    pT->setName(name);
    updateEditor();
    return {pT->getID(), QString()};
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
    if (!function.isEmpty()) {
        pT->setScript(function);
    }
    const int id = pT->getID();
    pT->setName(QString::number(id));
    return id;
}

// No documentation available in wiki - internal function
std::pair<int, QString> TLuaInterpreter::startPermKey(QString& name, QString& parent, int& keycode, int& modifier, QString& function)
{
    TKey* pT;

    if (parent.isEmpty()) {
        pT = new TKey("a", mpHost); // The use of "a" seems a bit arbitrary...!
    } else {
        TKey* pP = mpHost->getKeyUnit()->findFirstKey(parent);
        if (!pP) {
            return {-1, qsl("parent '%1' not found").arg(parent)};
        }
        pT = new TKey(pP, mpHost);
    }
    pT->setKeyCode(keycode);
    pT->setKeyModifiers(modifier);
    pT->setIsFolder(keycode == -1);
    pT->setIsActive(keycode != -1); // Folders (keycode == -1) start as inactive
    pT->setTemporary(false);
    pT->registerKey();
    // CHECK: The lua code in function could fail to compile - but there is no feedback here to the caller.
    pT->setScript(function);
    pT->setName(name);
    updateEditor();
    return {pT->getID(), QString()};
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempKey(int& modifier, int& keycode, const QString& function)
{
    TKey* pT;
    pT = new TKey("a", mpHost);
    pT->setKeyCode(keycode);
    pT->setKeyModifiers(modifier);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerKey();
    if (!function.isEmpty()) {
        pT->setScript(function);
    }
    const int id = pT->getID();
    pT->setName(QString::number(id));
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempExactMatchTrigger(const QString& regex, const QString& function, int expiryCount)
{
    TTrigger* pT = nullptr;
    const QStringList sList {regex};
    QList<int> const propertyList {REGEX_EXACT_MATCH};
    pT = new TTrigger("a", sList, propertyList, false, mpHost);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setScript(function);
    const int id = pT->getID();
    pT->setName(QString::number(id));
    pT->setExpiryCount(expiryCount);
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempBeginOfLineTrigger(const QString& regex, const QString& function, int expiryCount)
{
    TTrigger* pT = nullptr;
    const QStringList sList {regex};
    QList<int> const propertyList {REGEX_BEGIN_OF_LINE_SUBSTRING};
    pT = new TTrigger("a", sList, propertyList, false, mpHost);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setScript(function);
    const int id = pT->getID();
    pT->setName(QString::number(id));
    pT->setExpiryCount(expiryCount);
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempTrigger(const QString& regex, const QString& function, int expiryCount)
{
    TTrigger* pT = nullptr;
    const QStringList sList {regex};
    QList<int> const propertyList {REGEX_SUBSTRING};
    pT = new TTrigger("a", sList, propertyList, false, mpHost);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setScript(function);
    const int id = pT->getID();
    pT->setName(QString::number(id));
    pT->setExpiryCount(expiryCount);
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempPromptTrigger(const QString& function, int expiryCount)
{
    TTrigger* pT;
    const QStringList sList = {QString()};
    QList<int> const propertyList = {REGEX_PROMPT};
    pT = new TTrigger("a", sList, propertyList, false, mpHost);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setScript(function);
    const int id = pT->getID();
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
    const int id = pT->getID();
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
    const int id = pT->getID();
    pT->setName(QString::number(id));
    pT->setExpiryCount(expiryCount);
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempRegexTrigger(const QString& regex, const QString& function, int expiryCount)
{
    TTrigger* pT = nullptr;
    const QStringList sList {regex};
    QList<int> const propertyList {REGEX_PERL};
    pT = new TTrigger("a", sList, propertyList, false, mpHost);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setScript(function);
    const int id = pT->getID();
    pT->setName(QString::number(id));
    pT->setExpiryCount(expiryCount);
    return id;
}

// No documentation available in wiki - internal function
std::pair<int, QString> TLuaInterpreter::startPermRegexTrigger(const QString& name, const QString& parent, QStringList& patterns, const QString& function)
{
    TTrigger* pT;
    QList<int> propertyList;
    for (int i = 0; i < patterns.size(); i++) {
        propertyList << REGEX_PERL;
    }
    if (parent.isEmpty()) {
        pT = new TTrigger("a", patterns, propertyList, (patterns.size() > 1), mpHost);
    } else {
        TTrigger* pP = mpHost->getTriggerUnit()->findTrigger(parent);
        if (!pP) {
            return std::pair(-1, qsl("parent '%1' not found").arg(parent));
        }
        pT = new TTrigger(pP, mpHost);
        pT->setRegexCodeList(patterns, propertyList);
    }
    pT->setIsFolder(patterns.empty());
    pT->setIsActive(true);
    pT->setTemporary(false);
    pT->registerTrigger();
    pT->setScript(function);
    pT->setName(name);
    updateEditor();
    return std::pair(pT->getID(), QString());
}

// No documentation available in wiki - internal function
std::pair<int, QString> TLuaInterpreter::startPermBeginOfLineStringTrigger(const QString& name, const QString& parent, QStringList& patterns, const QString& function)
{
    TTrigger* pT;
    QList<int> propertyList;
    for (int i = 0; i < patterns.size(); i++) {
        propertyList << REGEX_BEGIN_OF_LINE_SUBSTRING;
    }
    if (parent.isEmpty()) {
        pT = new TTrigger("a", patterns, propertyList, (patterns.size() > 1), mpHost);
    } else {
        TTrigger* pP = mpHost->getTriggerUnit()->findTrigger(parent);
        if (!pP) {
            return {-1, qsl("parent '%1' not found").arg(parent)};
        }
        pT = new TTrigger(pP, mpHost);
        pT->setRegexCodeList(patterns, propertyList);
    }
    pT->setIsFolder(patterns.empty());
    pT->setIsActive(true);
    pT->setTemporary(false);
    pT->registerTrigger();
    pT->setScript(function);
    pT->setName(name);
    updateEditor();
    return std::pair(pT->getID(), QString());
}

// No documentation available in wiki - internal function
std::pair<int, QString> TLuaInterpreter::startPermSubstringTrigger(const QString& name, const QString& parent, const QStringList& patterns, const QString& function)
{
    TTrigger* pT;
    QList<int> propertyList;
    for (int i = 0; i < patterns.size(); i++) {
        propertyList << REGEX_SUBSTRING;
    }
    if (parent.isEmpty()) {
        pT = new TTrigger("a", patterns, propertyList, (patterns.size() > 1), mpHost);
    } else {
        TTrigger* pP = mpHost->getTriggerUnit()->findTrigger(parent);
        if (!pP) {
            return {-1, qsl("parent '%1' not found").arg(parent)};
        }
        pT = new TTrigger(pP, mpHost);
        pT->setRegexCodeList(patterns, propertyList);
    }
    pT->setIsFolder(patterns.empty());
    pT->setIsActive(true);
    pT->setTemporary(false);
    pT->registerTrigger();
    pT->setScript(function);
    pT->setName(name);
    updateEditor();
    return {pT->getID(), QString()};
}

// No documentation available in wiki - internal function
std::pair<int, QString> TLuaInterpreter::startPermPromptTrigger(const QString& name, const QString& parent, const QString& function)
{
    TTrigger* pT;
    QList<int> const propertyList = {REGEX_PROMPT};
    const QStringList patterns = {QString()};

    if (parent.isEmpty()) {
        pT = new TTrigger("a", patterns, propertyList, false, mpHost);
    } else {
        TTrigger* pP = mpHost->getTriggerUnit()->findTrigger(parent);
        if (!pP) {
            return {-1, qsl("parent '%1' not found").arg(parent)};
        }
        pT = new TTrigger(pP, mpHost);
        pT->setRegexCodeList(patterns, propertyList);
    }
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(false);
    pT->registerTrigger();
    pT->setScript(function);
    pT->setName(name);
    updateEditor();
    return {pT->getID(), QString()};
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#alert
int TLuaInterpreter::alert(lua_State* L)
{
    double luaAlertDuration = 0.0;

    if (lua_gettop(L) > 0) {
        luaAlertDuration = getVerifiedDouble(L, __func__, 1, "alert duration in seconds");
        if (luaAlertDuration < 0.000) {
            lua_pushstring(L, "alert: duration, in seconds, is optional but if given must be zero or greater.");
            return lua_error(L);
        }
    }

    // QApplication::alert expects milliseconds, not seconds
    QApplication::alert(mudlet::self(), qRound(luaAlertDuration * 1000.0));

    return 0;
}

static int host_key = 0;

// No documentation available in wiki - internal function
static void storeHostInLua(lua_State* L, Host* h)
{
    lua_pushlightuserdata(L, &host_key); // 1 - push unique key
    lua_pushlightuserdata(L, h);         // 2 - push host ptr
    lua_rawset(L, LUA_REGISTRYINDEX);    // 0 - register[key] = host
}

// No documentation available in wiki - internal function
Host& getHostFromLua(lua_State* L)
{
    lua_pushlightuserdata(L, &host_key);    // 1 - push unique key
    lua_rawget(L, LUA_REGISTRYINDEX);       // 1 - pop key, push host ptr
    auto* h = static_cast<Host*>(lua_touserdata(L, -1)); // 1 - get host ptr
    lua_pop(L, 1);                          // 0 - pop host ptr
    assert(h);
    return *h;
}

// No documentation available in wiki - internal function
// Used to unref lua objects in the registry to avoid memory leaks
// i.e. Unrefing tables passed into TLabel's event parameters.
void TLuaInterpreter::freeLuaRegistryIndex(int index)
{
    luaL_unref(pGlobalLua, LUA_REGISTRYINDEX, index);
}

// No documentation available in wiki - internal function
// Looks for argument types in an 'event' that have stored
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addWordToDictionary
int TLuaInterpreter::addWordToDictionary(lua_State* L)
{
    Host& host = getHostFromLua(L);
    bool hasUserDictionary = false;
    bool hasSharedDictionary = false;
    host.getUserDictionaryOptions(hasUserDictionary, hasSharedDictionary);
    if (!hasUserDictionary) {
        return warnArgumentValue(L, __func__, "no user dictionary enabled in the preferences for this profile");
    }

    const QString text = getVerifiedString(L, __func__, 1, "word");
    QPair<bool, QString> const result = host.mpConsole->addWordToSet(text);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeWordFromDictionary
int TLuaInterpreter::removeWordFromDictionary(lua_State* L)
{
    Host& host = getHostFromLua(L);
    bool hasUserDictionary = false;
    bool hasSharedDictionary = false;
    host.getUserDictionaryOptions(hasUserDictionary, hasSharedDictionary);
    if (!hasUserDictionary) {
        return warnArgumentValue(L, __func__, "no user dictionary enabled in the preferences for this profile");
    }

    const QString text = getVerifiedString(L, __func__, 1, "word");
    QPair<bool, QString> const result = host.mpConsole->removeWordFromSet(text);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#spellCheckWord
int TLuaInterpreter::spellCheckWord(lua_State* L)
{
    Host& host = getHostFromLua(L);
    bool hasUserDictionary = false;
    bool hasSharedDictionary = false;
    host.getUserDictionaryOptions(hasUserDictionary, hasSharedDictionary);

    const QString text = getVerifiedString(L, __func__, 1, "word");

    bool useUserDictionary = false;
    if (lua_gettop(L) > 1) {
        useUserDictionary = getVerifiedBool(L, __func__, 2, "check profile dictionary {use 'false' or omit to check against system dictionary}", true);
        if (useUserDictionary && !hasUserDictionary) {
            return warnArgumentValue(L, __func__, "no user dictionary enabled in the preferences for this profile");
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
            return warnArgumentValue(L, __func__,
                "no main dictionaries found: Mudlet has not been able to find any dictionary files to use so is unable to check your word");
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

    const QString text = getVerifiedString(L, __func__, 1, "word");

    bool useUserDictionary = false;
    if (lua_gettop(L) > 1) {
        useUserDictionary = getVerifiedBool(L, __func__, 2, "check profile dictionary {use 'false' or omit to check against system dictionary}", true);
        if (useUserDictionary && !hasUserDictionary) {
            return warnArgumentValue(L, __func__, "no user dictionary enabled in the preferences for this profile");
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
            return warnArgumentValue(L, __func__,
                "no main dictionaries found: Mudlet has not been able to find any dictionary files to use so is unable to make suggestions for your word");
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
        return warnArgumentValue(L, __func__, "no user dictionary enabled in the preferences for this profile");
    }

    // This may stall if this is accessing the shared user dictionary and that
    // is being updated by another profile, but it should eventually return...
    // We must keep a local reference/copy of the value returned because the
    // returned item is a deep-copy in the case of a shared dictionary and two
    // calls to TConsole::getWordSet() can return two different instances which
    // is fatally dangerous if used in a range based initialiser:
    QSet<QString> wordSet{host.mpConsole->getWordSet()};
    QStringList wordList{wordSet.begin(), wordSet.end()};
    const int wordCount = wordList.size();
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

// Documentation: https://wiki.mudlet.org/w/Manual:Miscellaneous_Functions#getCharacterName
int TLuaInterpreter::getCharacterName(lua_State* L)
{
    Host& host = getHostFromLua(L);

    const QString name{host.getLogin()};
    if (name.isEmpty()) {
        lua_pushnil(L);
        lua_pushstring(L, "no character name set");
        return 2;
    }

    lua_pushstring(L, name.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Miscellaneous_Functions#getProfileInformation
int TLuaInterpreter::getProfileInformation(lua_State* L)
{
    QString info;
    Host& host = getHostFromLua(L);
    const int params = lua_gettop(L);

    switch (params) {
        case 0:
        {
            info = host.readProfileData(qsl("description"));
            break;
        }
        default:
        {
            QString profileName = getVerifiedString(L, __func__, 1, "profile name");
            if (profileName.isEmpty()) {
                lua_pushnil(L);
                lua_pushstring(L, "getProfileInformation: profile name cannot be empty");
                return 2;
            }
            if (!mudlet::self()->profileExists(profileName)) {
                lua_pushnil(L);
                lua_pushfstring(L, "getProfileInformation: profile '%s' does not exist", profileName.toUtf8().constData());
                return 2;
            } else {
                info = mudlet::self()->readProfileData(profileName, qsl("description"));
            }
            break;
        }
    }

    lua_pushstring(L, info.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Miscellaneous_Functions#setProfileInformation
int TLuaInterpreter::setProfileInformation(lua_State* L)
{
    QString profileName = getHostFromLua(L).getName();
    QString text;
    const int params = lua_gettop(L);

    switch (params) {
        case 1:
        {
            text = getVerifiedString(L, __func__, 1, "text");
            break;
        }
        default:
        {
            profileName = getVerifiedString(L, __func__, 1, "profile name");
            text = getVerifiedString(L, __func__, 2, "text");
            break;
        }
    }

    QPair<bool, QString> result = mudlet::self()->writeProfileData(profileName, qsl("description"), text);
    int returnCode = 1;
    lua_pushboolean(L, result.first);
    if (!result.second.isEmpty()) {
        lua_pushfstring(L, "setProfileInformation: %s does not exist", profileName.toUtf8().constData());
        returnCode = 2;
    }
    return returnCode;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Miscellaneous_Functions#clearProfileInformation
int TLuaInterpreter::clearProfileInformation(lua_State* L)
{
    QString profileName = getHostFromLua(L).getName();
    QString desc = "";
    const int params = lua_gettop(L);

    switch (params) {
    case 0:
        break;
    default:
        profileName = getVerifiedString(L, __func__, 1, "profile name");
        break;
    }

    // if this is a default game, return to the orginal text
    auto itDetails = TGameDetails::findGame(profileName);
    if (itDetails != TGameDetails::scmDefaultGames.constEnd()) {
        if (!(*itDetails).description.isEmpty()) {
            desc = (*itDetails).description;
        }
    }

    QPair<bool, QString> result = mudlet::self()->writeProfileData(profileName, qsl("description"), desc);
    int returnCode = 1;
    lua_pushboolean(L, result.first);
    if (!result.second.isEmpty()) {
        lua_pushstring(L, "Profile not found");
        returnCode = 2;
    }
    return returnCode;
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

    lua_pushstring(L, name.toUtf8().constData());
    lua_insert(L, -2);
    lua_settable(L, -3);
    lua_pop(L, 1);
}

// Internal function - copies current profile's 16 ANSI colors into the Lua "color_table"
void TLuaInterpreter::updateAnsi16ColorsInTable()
{
    lua_State* L = pGlobalLua;

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

    // Now we can add/update the items we need to, though it is a bit repetitive:
    QColor color = mpHost->mBlack;
    insertColorTableEntry(L, color, qsl("ansi_000"));
    insertColorTableEntry(L, color, qsl("ansi_black"));
    insertColorTableEntry(L, color, qsl("ansiBlack"));

    color = mpHost->mRed;
    insertColorTableEntry(L, color, qsl("ansi_001"));
    insertColorTableEntry(L, color, qsl("ansi_red"));
    insertColorTableEntry(L, color, qsl("ansiRed"));

    color = mpHost->mGreen;
    insertColorTableEntry(L, color, qsl("ansi_002"));
    insertColorTableEntry(L, color, qsl("ansi_green"));
    insertColorTableEntry(L, color, qsl("ansiGreen"));

    color = mpHost->mYellow;
    insertColorTableEntry(L, color, qsl("ansi_003"));
    insertColorTableEntry(L, color, qsl("ansi_yellow"));
    insertColorTableEntry(L, color, qsl("ansiYellow"));

    color = mpHost->mBlue;
    insertColorTableEntry(L, color, qsl("ansi_004"));
    insertColorTableEntry(L, color, qsl("ansi_blue"));
    insertColorTableEntry(L, color, qsl("ansiBlue"));

    color = mpHost->mMagenta;
    insertColorTableEntry(L, color, qsl("ansi_005"));
    insertColorTableEntry(L, color, qsl("ansi_magenta"));
    insertColorTableEntry(L, color, qsl("ansiMagenta"));

    color = mpHost->mCyan;
    insertColorTableEntry(L, color, qsl("ansi_006"));
    insertColorTableEntry(L, color, qsl("ansi_cyan"));
    insertColorTableEntry(L, color, qsl("ansiCyan"));

    color = mpHost->mWhite;
    insertColorTableEntry(L, color, qsl("ansi_007"));
    insertColorTableEntry(L, color, qsl("ansi_white"));
    insertColorTableEntry(L, color, qsl("ansiWhite"));

    color = mpHost->mLightBlack;
    insertColorTableEntry(L, color, qsl("ansi_008"));
    insertColorTableEntry(L, color, qsl("ansi_light_black"));
    insertColorTableEntry(L, color, qsl("ansiLightBlack"));

    color = mpHost->mLightRed;
    insertColorTableEntry(L, color, qsl("ansi_009"));
    insertColorTableEntry(L, color, qsl("ansi_light_red"));
    insertColorTableEntry(L, color, qsl("ansiLightRed"));

    color = mpHost->mLightGreen;
    insertColorTableEntry(L, color, qsl("ansi_010"));
    insertColorTableEntry(L, color, qsl("ansi_light_green"));
    insertColorTableEntry(L, color, qsl("ansiLightGreen"));

    color = mpHost->mLightYellow;
    insertColorTableEntry(L, color, qsl("ansi_011"));
    insertColorTableEntry(L, color, qsl("ansi_light_yellow"));
    insertColorTableEntry(L, color, qsl("ansiLightYellow"));

    color = mpHost->mLightBlue;
    insertColorTableEntry(L, color, qsl("ansi_012"));
    insertColorTableEntry(L, color, qsl("ansi_light_blue"));
    insertColorTableEntry(L, color, qsl("ansiLightBlue"));

    color = mpHost->mLightMagenta;
    insertColorTableEntry(L, color, qsl("ansi_013"));
    insertColorTableEntry(L, color, qsl("ansi_light_magenta"));
    insertColorTableEntry(L, color, qsl("ansiLightMagenta"));

    color = mpHost->mLightCyan;
    insertColorTableEntry(L, color, qsl("ansi_014"));
    insertColorTableEntry(L, color, qsl("ansi_light_cyan"));
    insertColorTableEntry(L, color, qsl("ansiLightCyan"));

    color = mpHost->mLightWhite;
    insertColorTableEntry(L, color, qsl("ansi_015"));
    insertColorTableEntry(L, color, qsl("ansi_light_white"));
    insertColorTableEntry(L, color, qsl("ansiLightWhite"));
}

// Internal function - copies current profile's extended ANSI colors into the
// Lua "color_table" - it might be feasible to do this entirely within an
// external lua file ("GUIUtils.lua2) as we do not provide a means to vary
// the ANSI colours 17 to 255 that this handles...
void TLuaInterpreter::updateExtendedAnsiColorsInTable()
{
    lua_State* L = pGlobalLua;

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
        const int r = i / 36;
        const int g = (i - (r * 36)) / 6;
        const int b = (i - (r * 36)) - (g * 6);

        lua_createtable(L, 3, 0);

        lua_pushnumber(L, r == 0 ? 0 : (r - 1) * 40 + 95);
        lua_rawseti(L, -2, 1);

        lua_pushnumber(L, g == 0 ? 0 : (g - 1) * 40 + 95);
        lua_rawseti(L, -2, 2);

        lua_pushnumber(L, b == 0 ? 0 : (b - 1) * 40 + 95);
        lua_rawseti(L, -2, 3);

        lua_getfield(L, LUA_GLOBALSINDEX, "color_table");
        lua_insert(L, -2);

        const QString name = qsl("ansi_%1").arg(i + 16, 3, 10, QLatin1Char('0'));
        lua_pushstring(L, name.toUtf8().constData());
        lua_insert(L, -2);
        lua_settable(L, -3);
        lua_pop(L, 1);
    }

    // And insert the 24 Greyscale colours
    for (int i = 232; i < 256; ++i) {
        lua_createtable(L, 3, 0);

        const int value = (i - 232) * 10 + 8;

        lua_pushnumber(L, value);
        lua_rawseti(L, -2, 1);

        lua_pushnumber(L, value);
        lua_rawseti(L, -2, 2);

        lua_pushnumber(L, value);
        lua_rawseti(L, -2, 3);

        lua_getfield(L, LUA_GLOBALSINDEX, "color_table");
        lua_insert(L, -2);

        const QString name = qsl("ansi_%1").arg(i, 3, 10, QLatin1Char('0'));
        lua_pushstring(L, name.toUtf8().constData());
        lua_insert(L, -2);
        lua_settable(L, -3);
        lua_pop(L, 1);
    }
}

// Internal function - Creates a table for useful information from the http
// response. It creates an empty table, calls upon other functions to
// add things to it, and then returns a key to where it is in the lua registry.
int TLuaInterpreter::createHttpResponseTable(QNetworkReply* reply)
{
    lua_State* L = pGlobalLua;

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
    for (const QByteArray& header : headerList) {
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
    const Host& host = getHostFromLua(L);
    QNetworkCookieJar* cookieJar = host.mLuaInterpreter.mpFileDownloader->cookieJar();
    const QList<QNetworkCookie> cookies = cookieJar->cookiesForUrl(reply->url());
    for (const QNetworkCookie& cookie : cookies) {
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapBackgroundColor
int TLuaInterpreter::getMapBackgroundColor(lua_State* L)
{
    auto& host = getHostFromLua(L);
    auto color = host.mBgColor_2;
    lua_pushnumber(L, color.red());
    lua_pushnumber(L, color.green());
    lua_pushnumber(L, color.blue());
    lua_pushnumber(L, color.alpha());
    return 4;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMapBackgroundColor
int TLuaInterpreter::setMapBackgroundColor(lua_State* L)
{
    int a = 255;
    const int r = getVerifiedInt(L, __func__, 1, "red component");
    if (r < 0 || r > 255) {
        return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(r));
    }

    const int g = getVerifiedInt(L, __func__, 2, "green component");
    if (g < 0 || g > 255) {
        return warnArgumentValue(L, __func__, csmInvalidGreenValue.arg(g));
    }

    const int b = getVerifiedInt(L, __func__, 3, "blue component");
    if (b < 0 || b > 255) {
        return warnArgumentValue(L, __func__, csmInvalidBlueValue.arg(b));
    }

    if (lua_gettop(L) > 3) {
        a = getVerifiedInt(L, __func__, 4, "alpha", true);
        if (a < 0 || a > 255) {
            return warnArgumentValue(L, __func__, csmInvalidAlphaValue.arg(a));
        }
    }

    auto& host = getHostFromLua(L);
    host.mBgColor_2 = QColor(r, g, b, a);
    updateMap(L);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapRoomExitsColor
int TLuaInterpreter::getMapRoomExitsColor(lua_State* L)
{
    auto& host = getHostFromLua(L);
    auto color = host.mFgColor_2;
    lua_pushnumber(L, color.red());
    lua_pushnumber(L, color.green());
    lua_pushnumber(L, color.blue());
    return 3;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMapRoomExitsColor
int TLuaInterpreter::setMapRoomExitsColor(lua_State* L)
{
    const int r = getVerifiedInt(L, __func__, 1, "red component");
    if (r < 0 || r > 255) {
        return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(r));
    }

    const int g = getVerifiedInt(L, __func__, 2, "green component");
    if (g < 0 || g > 255) {
        return warnArgumentValue(L, __func__, csmInvalidGreenValue.arg(g));
    }

    const int b = getVerifiedInt(L, __func__, 3, "blue component");
    if (b < 0 || b > 255) {
        return warnArgumentValue(L, __func__, csmInvalidBlueValue.arg(b));
    }

    auto& host = getHostFromLua(L);
    host.mFgColor_2 = QColor(r, g, b);
    updateMap(L);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#showNotification
int TLuaInterpreter::showNotification(lua_State* L)
{
    const int n = lua_gettop(L);
    const QString title = getVerifiedString(L, __func__, 1, "title");
    QString text = title;
    if (n >= 2) {
        text = getVerifiedString(L, __func__, 2, "message");
    }
    std::optional<int> notificationExpirationTime;
    if (n >= 3) {
        notificationExpirationTime = qMax(qRound(getVerifiedDouble(L, __func__, 3, "expiration time in seconds") * 1000), 1000);
    }

    mudlet::self()->mTrayIcon.show();
    if (notificationExpirationTime.has_value()) {
        mudlet::self()->mTrayIcon.showMessage(title, text, mudlet::self()->mTrayIcon.icon(), notificationExpirationTime.value());
    } else {
        mudlet::self()->mTrayIcon.showMessage(title, text, mudlet::self()->mTrayIcon.icon());
    }
    mudlet::self()->mTrayIcon.hide();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addFileWatch
int TLuaInterpreter::addFileWatch(lua_State * L)
{
    auto path = getVerifiedString(L, __func__, 1, "path");
    auto& host = getHostFromLua(L);

    const QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        return warnArgumentValue(L, __func__, qsl("path '%1' does not exist").arg(path));
    }

    lua_pushboolean(L, host.getLuaInterpreter()->mpFileSystemWatcher->addPath(path));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeFileWatch
int TLuaInterpreter::removeFileWatch(lua_State * L)
{
    auto path = getVerifiedString(L, __func__, 1, "path");
    auto& host = getHostFromLua(L);

    lua_pushboolean(L, host.getLuaInterpreter()->mpFileSystemWatcher->removePath(path));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setConfig
// Please use same options with same names in setConfig and getConfig and keep them in sync
// The table argument case for setting multiple properties at once is handled
// by setConfig in Other.lua.
int TLuaInterpreter::setConfig(lua_State * L)
{
    auto& host = getHostFromLua(L);
    const bool currentHost = (mudlet::self()->mpCurrentActiveHost == &host);
    QString key = getVerifiedString(L, __func__, 1, "key");
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "you must provide key");
    }

    auto success = [&]()
    {
        if (mudlet::smDebugMode) {
            TDebug(Qt::white, Qt::blue) << qsl("setConfig: a script has changed %1\n").arg(key) >> &host;
        }
        lua_pushboolean(L, true);
        return 1;
    };

    if (host.mpMap && host.mpMap->mpMapper) {
        if (key == qsl("mapRoomSize")) {
            host.mpMap->mpMapper->slot_roomSize(getVerifiedInt(L, __func__, 2, "value"));
            return success();
        }
        if (key == qsl("mapExitSize")) {
            host.mpMap->mpMapper->slot_exitSize(getVerifiedInt(L, __func__, 2, "value"));
            return success();
        }
        if (key == qsl("mapRoundRooms")) {
            host.mpMap->mpMapper->slot_toggleRoundRooms(getVerifiedBool(L, __func__, 2, "value"));
            return success();
        }
        if (key == qsl("showRoomIdsOnMap")) {
            host.mpMap->mpMapper->slot_setShowRoomIds(getVerifiedBool(L, __func__, 2, "value"));
            return success();
        }
        if (key == qsl("showMapInfo")) {
            host.mMapInfoContributors.insert(getVerifiedString(L, __func__, 2, "value"));
            host.mpMap->mpMapper->slot_updateInfoContributors();
            return success();
        }
        if (key == qsl("hideMapInfo")) {
            host.mMapInfoContributors.remove(getVerifiedString(L, __func__, 2, "value"));
            host.mpMap->mpMapper->slot_updateInfoContributors();
            return success();
        }
#if defined(INCLUDE_3DMAPPER)
        if (key == qsl("show3dMapView")) {
            host.mpMap->mpMapper->slot_toggle3DView(getVerifiedBool(L, __func__, 2, "value"));
            return success();
        }
#endif
        if (key == qsl("mapperPanelVisible")) {
            host.mpMap->mpMapper->slot_setMapperPanelVisible(getVerifiedBool(L, __func__, 2, "value"));
            return success();
        }
        if (key == qsl("mapShowRoomBorders")) {
            host.mMapperShowRoomBorders = getVerifiedBool(L, __func__, 2, "value");
            return success();
        }
        if (key == qsl("showUpperLowerLevels")) {
            mudlet::self()->mDrawUpperLowerLevels = getVerifiedBool(L, __func__, 2, "value");

            if (host.mpMap && host.mpMap->mpMapper && host.mpMap->mpMapper->mp2dMap) {
                host.mpMap->mpMapper->mp2dMap->update();
            }

            return success();
        }
        if (key == qsl("mapInfoColor")) {
            if (!lua_istable(L, 2)) {
                lua_pushfstring(L, "%s: bad argument #%d type (table expected for mapInfoColor, got %s!)",
                    __func__, 2, luaL_typename(L, 2));
                return warnArgumentValue(L, __func__, qsl("mapInfoColor requires a table {r, g, b} or {r, g, b, a}"));
            }

            // Get red component (index 1)
            lua_rawgeti(L, 2, 1);
            if (!lua_isnumber(L, -1)) {
                lua_pop(L, 1);
                return warnArgumentValue(L, __func__, qsl("mapInfoColor table must have red component at index 1"));
            }
            const int r = lua_tonumber(L, -1);
            lua_pop(L, 1);
            if (r < 0 || r > 255) {
                return warnArgumentValue(L, __func__, csmInvalidRedValue.arg(r));
            }

            // Get green component (index 2)
            lua_rawgeti(L, 2, 2);
            if (!lua_isnumber(L, -1)) {
                lua_pop(L, 1);
                return warnArgumentValue(L, __func__, qsl("mapInfoColor table must have green component at index 2"));
            }
            const int g = lua_tonumber(L, -1);
            lua_pop(L, 1);
            if (g < 0 || g > 255) {
                return warnArgumentValue(L, __func__, csmInvalidGreenValue.arg(g));
            }

            // Get blue component (index 3)
            lua_rawgeti(L, 2, 3);
            if (!lua_isnumber(L, -1)) {
                lua_pop(L, 1);
                return warnArgumentValue(L, __func__, qsl("mapInfoColor table must have blue component at index 3"));
            }
            const int b = lua_tonumber(L, -1);
            lua_pop(L, 1);
            if (b < 0 || b > 255) {
                return warnArgumentValue(L, __func__, csmInvalidBlueValue.arg(b));
            }

            // Get alpha component (index 4, optional, defaults to 255)
            int a = 255;
            lua_rawgeti(L, 2, 4);
            if (lua_isnumber(L, -1)) {
                a = lua_tonumber(L, -1);
                if (a < 0 || a > 255) {
                    lua_pop(L, 1);
                    return warnArgumentValue(L, __func__, csmInvalidAlphaValue.arg(a));
                }
            }
            lua_pop(L, 1);

            host.mMapInfoBg = QColor(r, g, b, a);
            updateMap(L);
            return success();
        }
    }

    if (key == qsl("enableGMCP")) {
        host.mEnableGMCP = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("enableMSSP")) {
        host.mEnableMSSP = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("enableMSDP")) {
        host.mEnableMSDP = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("enableMSP")) {
        host.mEnableMSP = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("enableMTTS")) {
        host.mEnableMTTS = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("enableMNES")) {
        host.mEnableMNES = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("specialForceMxpNegotiationOff")) {
        // specialForceMxpNegotiationOff should not be used anymore, but we support it for compatibility
        // it will be the inverse of enableMXP
        host.mEnableMXP = !getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("enableMXP")) {
        host.mEnableMXP = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("askTlsAvailable")) {
        host.mAskTlsAvailable = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("promptForMXPProcessorOn")) {
        host.mPromptedForMXPProcessorOn = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("promptForVersionInTTYPE")) {
        host.mPromptedForVersionInTTYPE = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("versionInTTYPE")) {
        host.mVersionInTTYPE = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("inputLineStrictUnixEndings")) {
        host.mUSE_UNIX_EOL = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("autoClearInputLine")) {
        host.mAutoClearCommandLineAfterSend = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("showSentText")) {
        // Handle both legacy boolean and new string values
        if (lua_isboolean(L, 2)) {
            bool value = getVerifiedBool(L, __func__, 2, "value");
            host.mCommandEchoMode = value ? Host::CommandEchoMode::ScriptControl : Host::CommandEchoMode::Never;
        } else if (lua_isstring(L, 2)) {
            QString value = getVerifiedString(L, __func__, 2, "value");
            if (value == "never") {
                host.mCommandEchoMode = Host::CommandEchoMode::Never;
            } else if (value == "always") {
                host.mCommandEchoMode = Host::CommandEchoMode::Always;
            } else if (value == "script") {
                host.mCommandEchoMode = Host::CommandEchoMode::ScriptControl;
            } else {
                lua_pushfstring(L, "setConfig: bad argument #2 value (expected 'never', 'always', or 'script', got '%s')", value.toUtf8().constData());
                return warnArgumentValue(L, __func__, value);
            }
        } else {
            lua_pushfstring(L, "setConfig: bad argument #2 type (expected boolean or string for 'showSentText', got %s)", luaL_typename(L, 2));
            return warnArgumentValue(L, __func__, qsl("showSentText"));
        }
        return success();
    }
    if (key == qsl("fixUnnecessaryLinebreaks")) {
        host.set_USE_IRE_DRIVER_BUGFIX(getVerifiedBool(L, __func__, 2, "value"));
        return success();
    }
    if (key == qsl("specialForceCompressionOff")) {
        host.mFORCE_NO_COMPRESSION = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("specialForceGAOff")) {
        host.mFORCE_GA_OFF = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("specialForceCharsetNegotiationOff")) {
        // specialForceCharsetNegotiationOff should not be used anymore, but we support it for compatibility
        // it will be the inverse of enableCHARSET
        host.mEnableCHARSET = !getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("enableCHARSET")) {
        host.mEnableCHARSET = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("forceNewEnvironNegotiationOff")) {
        // forceNewEnvironNegotiationOff should not be used anymore, but we support it for compatibility
        // it will be the inverse of enableNEWENVIRON
        host.mEnableNEWENVIRON = !getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("enableNEWENVIRON")) {
        host.mEnableNEWENVIRON = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("compactInputLine")) {
        const bool value = getVerifiedBool(L, __func__, 2, "value");
        host.setCompactInputLine(value);
        if (currentHost) {
            mudlet::self()->dactionInputLine->setChecked(value);
        }

        return success();
    }
    if (key == qsl("editorAutoComplete")) {
        host.mEditorAutoComplete = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("announceIncomingText")) {
        host.mAnnounceIncomingText = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("advertiseScreenReader")) {
        host.mAdvertiseScreenReader = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("enableClosedCaption")) {
        host.mEnableClosedCaption = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("blankLinesBehaviour")) {
        static const QStringList behaviours{"show", "hide", "replacewithspace"};
        const auto behaviour = getVerifiedString(L, __func__, 2, "value");

        if (!behaviours.contains(behaviour)) {
            lua_pushnil(L);
            lua_pushfstring(L, "invalid caretShortcut string \"%s\", it should be one of \"%s\"",
                            lua_tostring(L, 2), behaviours.join(qsl("\", \"")).toUtf8().constData());
            return 2;
        }

        if (behaviour == qsl("show")) {
            host.mBlankLineBehaviour = Host::BlankLineBehaviour::Show;
        } else if (behaviour == qsl("hide")) {
            host.mBlankLineBehaviour = Host::BlankLineBehaviour::Hide;
        } else if (behaviour == qsl("replacewithspace")) {
            host.mBlankLineBehaviour = Host::BlankLineBehaviour::ReplaceWithSpace;
        }
        return success();
    }
    if (key == qsl("caretShortcut")) {
        static const QStringList values{"none", "tab", "ctrltab", "f6"};
        const auto value = getVerifiedString(L, __func__, 2, "value");

        if (!values.contains(value)) {
            lua_pushnil(L);
            lua_pushfstring(L, "invalid caretShortcut string \"%s\", it should be one of \"%s\"",
                            lua_tostring(L, 2), values.join(qsl("\", \"")).toUtf8().constData());
            return 2;
        }

        if (value == qsl("tab")) {
            host.mCaretShortcut = Host::CaretShortcut::Tab;
        } else if (value == qsl("ctrltab")) {
            host.mCaretShortcut = Host::CaretShortcut::CtrlTab;
        } else if (value == qsl("f6")) {
            host.mCaretShortcut = Host::CaretShortcut::F6;
        } else {
            host.mCaretShortcut = Host::CaretShortcut::None;
        }
        return success();
    }
    if (key == qsl("commandLineHistorySaveSize")) {
        const auto value = getVerifiedInt(L, __func__, 2, "value");
        host.setCommandLineHistorySaveSize(value);
        return success();
    }
    if (key == qsl("controlCharacterHandling")) {
        static const QStringList values{"asis", "oem", "picture"};
        const auto value = getVerifiedString(L, __func__, 2, "value");

        if (!values.contains(value)) {
            lua_pushnil(L);
            lua_pushfstring(L, "invalid commandLineHistorySaveSize string \"%s\", it should be one of \"%s\"",
                            lua_tostring(L, 2), values.join(qsl("\", \"")).toUtf8().constData());
            return 2;
        }

        if (value == qsl("oem")) {
            host.setControlCharacterMode(ControlCharacterMode::OEM);
        } else if (value == qsl("picture")) {
            host.setControlCharacterMode(ControlCharacterMode::Picture);
        } else {
            host.setControlCharacterMode(ControlCharacterMode::AsIs);
        }
        return success();
    }
    if (key == qsl("logInHTML")) {
        host.mIsNextLogFileInHtmlFormat = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("f3SearchEnabled")) {
        const bool value = getVerifiedBool(L, __func__, 2, "value");
        host.setF3SearchEnabled(value);
        return success();
    }
    if (key == qsl("showTabConnectionIndicators")) {
        mudlet::self()->setShowTabConnectionIndicators(getVerifiedBool(L, __func__, 2, "value"));
        return success();
    }
    if (key == qsl("ambiguousEAsianWidthCharacters")) {
        static const QStringList values{"narrow", "wide", "auto"};
        const auto value = getVerifiedString(L, __func__, 2, "value");

        if (!values.contains(value)) {
            lua_pushnil(L);
            lua_pushfstring(L, "invalid ambiguousEAsianWidthCharacters string \"%s\", it should be one of \"%s\"",
                            lua_tostring(L, 2), values.join(qsl("\", \"")).toUtf8().constData());
            return 2;
        }

        if (value == qsl("narrow")) {
            host.setWideAmbiguousEAsianGlyphs(Qt::Unchecked);
        } else if (value == qsl("wide")) {
            host.setWideAmbiguousEAsianGlyphs(Qt::Checked);
        } else {
            host.setWideAmbiguousEAsianGlyphs(Qt::PartiallyChecked);
        }
        return success();
    }

    // Handle experiment keys
    if (key.startsWith(qsl("experiment."))) {
        auto [result, errorMessage] = host.setExperimentEnabled(key, getVerifiedBool(L, __func__, 2, "value"));
        if (!result) {
            return warnArgumentValue(L, __func__, errorMessage);
        }

        // Special handling for 3D mapper experiment
        if (key == qsl("experiment.3dmap.modernmapper")) {
#if defined(INCLUDE_3DMAPPER)
            if (host.mpMap && host.mpMap->mpMapper) {
                host.mpMap->mpMapper->recreate3DWidget();
            }
#endif
        }
        return success();
    }

    return warnArgumentValue(L, __func__, qsl("'%1' isn't a valid configuration option").arg(key));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#announce
int TLuaInterpreter::announce(lua_State *L)
{
    const QString text = getVerifiedString(L, __func__, 1, "text to announce");
    static const QStringList processingKinds{"importantall", "importantmostrecent", "all", "mostrecent", "currentthenmostrecent"};
    QString processing;

    const int n = lua_gettop(L);
    if (n > 1) {
        // while this only has effect on Windows, it should fail silently in order not to spam
        processing = getVerifiedString(L, __func__, 2, "processing style");

        if (!processingKinds.contains(processing)) {
            lua_pushfstring(L, "%s: bad argument #%d type (processing should be one of %s, got %s!)",
                __func__, 2, processingKinds.join(qsl(", ")).toUtf8().constData(), processing.toUtf8().constData());
            return lua_error(L);
        }
    }

    mudlet::self()->announce(text, processing, true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getConfig
// Please use same options with same names in setConfig and getConfig and keep them in sync
// The no args case that returns a table is handled by getConfig in Other.lua
// that runs a loop with a list of these properties, please update that list.
int TLuaInterpreter::getConfig(lua_State *L)
{
    auto &host = getHostFromLua(L);
    const QString key = getVerifiedString(L, __func__, 1, "key");
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "you must provide a key");
    }

    // Check if second parameter requests string format (for enhanced API)
    bool useStringFormat = false;
    if (lua_gettop(L) >= 2 && lua_isboolean(L, 2)) {
        useStringFormat = lua_toboolean(L, 2);
    }

    const std::unordered_map<QString, std::function<void()>> configMap = {
        { qsl("mapRoomSize"), [&](){ lua_pushnumber(L, host.mRoomSize); } },
        { qsl("mapExitSize"), [&](){ lua_pushnumber(L, host.mLineSize); } },
        { qsl("mapRoundRooms"), [&](){ lua_pushboolean(L, host.mBubbleMode); } },
        { qsl("showRoomIdsOnMap"), [&](){ lua_pushboolean(L, host.mShowRoomID); } },
        { qsl("show3dMapView"), [&](){
#if defined(INCLUDE_3DMAPPER)
            if (host.mpMap && host.mpMap->mpMapper) {
                auto widget = host.mpMap->mpMapper->glWidget;
                lua_pushboolean(L, (widget && widget->isVisible()));
                return;
            }
#endif
            lua_pushboolean(L, false);
        }},
        { qsl("mapperPanelVisible"), [&](){ lua_pushboolean(L, host.mShowPanel); } },
        { qsl("mapShowRoomBorders"), [&](){ lua_pushboolean(L, host.mMapperShowRoomBorders); } },
        { qsl("mapInfoColor"), [&](){
            lua_newtable(L);
            lua_pushnumber(L, host.mMapInfoBg.red());
            lua_rawseti(L, -2, 1);
            lua_pushnumber(L, host.mMapInfoBg.green());
            lua_rawseti(L, -2, 2);
            lua_pushnumber(L, host.mMapInfoBg.blue());
            lua_rawseti(L, -2, 3);
            lua_pushnumber(L, host.mMapInfoBg.alpha());
            lua_rawseti(L, -2, 4);
        } },
        { qsl("editorAutoComplete"), [&](){ lua_pushboolean(L, host.mEditorAutoComplete); } },
        { qsl("enableGMCP"), [&](){ lua_pushboolean(L, host.mEnableGMCP); } },
        { qsl("enableMSSP"), [&](){ lua_pushboolean(L, host.mEnableMSSP); } },
        { qsl("enableMSDP"), [&](){ lua_pushboolean(L, host.mEnableMSDP); } },
        { qsl("enableMSP"), [&](){ lua_pushboolean(L, host.mEnableMSP); } },
        { qsl("enableMTTS"), [&](){ lua_pushboolean(L, host.mEnableMTTS); } },
        { qsl("enableMNES"), [&](){ lua_pushboolean(L, host.mEnableMNES); } },
        { qsl("enableMXP"), [&](){ lua_pushboolean(L, host.mEnableMXP); } },
        { qsl("logDirectory"), [&](){
            const auto logDir = host.mLogDir;

            if (logDir == nullptr || logDir.isEmpty()) {
                lua_pushstring(L, mudlet::getMudletPath(enums::profileReplayAndLogFilesPath, getHostFromLua(L).getName()).toUtf8().constData());
            } else {
                lua_pushstring(L, host.mLogDir.toUtf8().constData());
            }
        } },
        { qsl("askTlsAvailable"), [&](){lua_pushboolean(L, host.mAskTlsAvailable); } },
        { qsl("promptForMXPProcessorOn"), [&](){lua_pushboolean(L, host.mPromptedForMXPProcessorOn); } },
        { qsl("specialForceMXPProcessorOn"), [&](){lua_pushboolean(L, host.getForceMXPProcessorOn()); } },
        { qsl("promptForVersionInTTYPE"), [&](){lua_pushboolean(L, host.mPromptedForVersionInTTYPE); } },
        { qsl("versionInTTYPE"), [&](){ lua_pushboolean(L, host.mVersionInTTYPE); } },
        { qsl("inputLineStrictUnixEndings"), [&](){ lua_pushboolean(L, host.mUSE_UNIX_EOL); } },
        { qsl("autoClearInputLine"), [&](){ lua_pushboolean(L, host.mAutoClearCommandLineAfterSend); } },
        { qsl("showSentText"), [&](){
            if (useStringFormat) {
                // Return string representation for new enhanced API
                switch (host.mCommandEchoMode) {
                case Host::CommandEchoMode::Never:
                    lua_pushstring(L, "never");
                    break;
                case Host::CommandEchoMode::Always:
                    lua_pushstring(L, "always");
                    break;
                case Host::CommandEchoMode::ScriptControl:
                    lua_pushstring(L, "script");
                    break;
                }
            } else {
                // For backward compatibility, return boolean for legacy scripts
                switch (host.mCommandEchoMode) {
                case Host::CommandEchoMode::Never:
                    lua_pushboolean(L, false);
                    break;
                case Host::CommandEchoMode::Always:
                case Host::CommandEchoMode::ScriptControl:
                    // Both modes map to true for backward compatibility
                    lua_pushboolean(L, true);
                    break;
                }
            }
        } },
        { qsl("fixUnnecessaryLinebreaks"), [&](){ lua_pushboolean(L, host.mUSE_IRE_DRIVER_BUGFIX); } },
        { qsl("specialForceCompressionOff"), [&](){ lua_pushboolean(L, host.mFORCE_NO_COMPRESSION); } },
        { qsl("specialForceGAOff"), [&](){ lua_pushboolean(L, host.mFORCE_GA_OFF); } },
        { qsl("specialForceMxpNegotiationOff"), [&](){
            // specialForceMxpNegotiationOff should not be used anymore, but we support it for compatibility
            // it will be the inverse of enableMXP
            lua_pushboolean(L, !host.mEnableMXP);
        } },
        { qsl("specialForceCharsetNegotiationOff"), [&](){
            // specialForceCharsetNegotiationOff should not be used anymore, but we support it for compatibility
            // it will be the inverse of enableCHARSET
            lua_pushboolean(L, !host.mEnableCHARSET);
        } },
        { qsl("enableCHARSET"), [&](){ lua_pushboolean(L, host.mEnableCHARSET); } },
        { qsl("forceNewEnvironNegotiationOff"), [&](){
            // forceNewEnvironNegotiationOff should not be used anymore, but we support it for compatibility
            // it will be the inverse of enableNEWENVIRON
            lua_pushboolean(L, !host.mEnableNEWENVIRON);
        } },
        { qsl("enableNEWENVIRON"), [&](){ lua_pushboolean(L, host.mEnableNEWENVIRON); } },
        { qsl("compactInputLine"), [&](){ lua_pushboolean(L, host.getCompactInputLine()); } },
        { qsl("announceIncomingText"), [&](){ lua_pushboolean(L, host.mAnnounceIncomingText); } },
        { qsl("blankLinesBehaviour"), [&](){
            const auto behaviour = host.mBlankLineBehaviour;
            if (behaviour == Host::BlankLineBehaviour::Show) {
                lua_pushstring(L, "show");
            } else if (behaviour == Host::BlankLineBehaviour::Hide) {
                lua_pushstring(L, "hide");
            } else if (behaviour == Host::BlankLineBehaviour::ReplaceWithSpace) {
                lua_pushstring(L, "replacewithspace");
            }
        } },
        { qsl("caretShortcut"), [&](){
            const auto caret = host.mCaretShortcut;
            if (caret == Host::CaretShortcut::None) {
                lua_pushstring(L, "none");
            } else if (caret == Host::CaretShortcut::Tab) {
                lua_pushstring(L, "tab");
            } else if (caret == Host::CaretShortcut::CtrlTab) {
                lua_pushstring(L, "ctrltab");
            } else if (caret == Host::CaretShortcut::F6) {
                lua_pushstring(L, "f6");
            }
        } },
        { qsl("commandLineHistorySaveSize"), [&](){ lua_pushnumber(L, host.getCommandLineHistorySaveSize()); } },
        { qsl("controlCharacterHandling"), [&](){
            const auto controlCharacterMode = host.getControlCharacterMode();
            switch (controlCharacterMode) {
            case ControlCharacterMode::Picture:
                lua_pushstring(L, "picture");
                break;
            case ControlCharacterMode::OEM:
                lua_pushstring(L, "oem");
                break;
            default:
                lua_pushstring(L, "asis");
            }
        } },
        { qsl("logInHTML"), [&](){ lua_pushboolean(L, host.mIsNextLogFileInHtmlFormat); } },
        { qsl("f3SearchEnabled"), [&](){ lua_pushboolean(L, host.getF3SearchEnabled()); } },
        { qsl("showTabConnectionIndicators"), [&](){ lua_pushboolean(L, mudlet::self()->mShowTabConnectionIndicators); } },
        { qsl("advertiseScreenReader"), [&](){ lua_pushboolean(L, host.mAdvertiseScreenReader); } },
        { qsl("ambiguousEAsianWidthCharacters"), [&]() {
            const auto ambiguousEAsianGlyphWidth = host.getWideAmbiguousEAsianGlyphsControlState();
            switch (ambiguousEAsianGlyphWidth) {
            case Qt::Unchecked:
                lua_pushstring(L, "narrow");
                break;
            case Qt::Checked:
                lua_pushstring(L, "wide");
            break;
            default:
                lua_pushstring(L, "auto");
            }
        } },
        { qsl("enableClosedCaption"), [&](){ lua_pushboolean(L, host.mEnableClosedCaption); } },
        { qsl("showUpperLowerLevels"), [&](){ lua_pushboolean(L, mudlet::self()->mDrawUpperLowerLevels); } }
    };

    auto it = configMap.find(key);
    if (it != configMap.end()) {
        it->second();
        return 1;
    }

    // Handle experiment keys
    if (key.startsWith(qsl("experiment."))) {
        if (key == qsl("experiment.list")) {
            // Special case: return list of all valid experiments
            QStringList validExperiments = host.getValidExperiments();
            lua_createtable(L, validExperiments.size(), 0);
            for (int i = 0; i < validExperiments.size(); ++i) {
                lua_pushstring(L, validExperiments.at(i).toUtf8().constData());
                lua_rawseti(L, -2, i + 1);
            }
        } else if (key.endsWith(qsl(".active"))) {
            // Handle special case: experiment.<group>.active returns active experiment name
            QString group = key.left(key.length() - 7); // Remove ".active"
            QString activeExperiment = host.getActiveExperimentInGroup(group);
            if (activeExperiment.isEmpty()) {
                lua_pushnil(L);
            } else {
                lua_pushstring(L, activeExperiment.toUtf8().constData());
            }
        } else {
            // Regular experiment key - but only if it's valid
            QStringList validExperiments = host.getValidExperiments();
            if (validExperiments.contains(key)) {
                lua_pushboolean(L, host.experimentEnabled(key));
            } else {
                lua_pushboolean(L, false);  // Invalid experiments are always false
            }
        }
        return 1;
    }

    return warnArgumentValue(L, __func__, qsl("'%1' isn't a valid configuration option").arg(key));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getSaveCommandHistory
int TLuaInterpreter::getSaveCommandHistory(lua_State* L)
{
    auto& host = getHostFromLua(L);
    auto numberOfLines = host.getCommandLineHistorySaveSize();
    if (!numberOfLines) {
        // We do not use warnArgumentValue(...) because it is valid to have
        // this disabled and we do not want a message to be painted on the
        // Central Debug Console:
        lua_pushboolean(L, false);
        lua_pushstring(L, "disabled by profile global preference");
        return 2;
    }
    QString name = QLatin1String("main");
    if (lua_gettop(L)) {
        name = CMDLINE_NAME(L, 1);
    }
    auto pCommandline = COMMANDLINE(L, name);
    lua_pushboolean(L, pCommandline->mSaveCommands);
    lua_pushstring(L, (pCommandline->mSaveCommands ? qsl("enabled (%1 lines will be saved)").arg(QString::number(numberOfLines)) : qsl("disabled")).toUtf8().constData());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setSaveCommandHistory
int TLuaInterpreter::setSaveCommandHistory(lua_State* L)
{
    auto n = lua_gettop(L);
    auto& host = getHostFromLua(L);
    auto numberOfLines = host.getCommandLineHistorySaveSize();
    if (!numberOfLines) {
        // Unlike for the getter we do want to alert on trying to set the
        // per-commandLine option when things are disabled globally for the
        // profile:
        return warnArgumentValue(L, __func__, "disabled by profile global preference");
    }
    QString name = QLatin1String("main");
    bool saveCommands = true;
    // if there is no arguments we will set the "save command history" on the
    // main  command line:
    if (n == 1) {
        saveCommands = getVerifiedBool(L, __func__, 1, "save command history", true);
    } else {
        if (lua_type(L, 1) == LUA_TSTRING) {
            // First argument is a string so is presumably a command line name
            name = CMDLINE_NAME(L, 1);
            if (n > 1) {
                saveCommands = !getVerifiedBool(L, __func__, 2, "save command history", true);
            }

        } else {
            if (lua_type(L, 1) != LUA_TBOOLEAN) {
                lua_pushfstring(L, "%s: bad argument #1 type (command line name as string or save history as boolean is optional, got %s!)",
                                __func__, luaL_typename(L, 1));
                return lua_error(L); // Dummy return!
            }

            saveCommands = !getVerifiedBool(L, __func__, 1, "save command history", true);
        }
    }

    auto pCommandline = COMMANDLINE(L, name);
    pCommandline->mSaveCommands = saveCommands;
    lua_pushboolean(L, true);
    return 1;
}

void TLuaInterpreter::updateEditor()
{
    if (mpHost->mpEditorDialog) {
        mpHost->mpEditorDialog->mNeedUpdateData = true;
    }
}
