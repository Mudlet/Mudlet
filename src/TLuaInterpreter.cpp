/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2022 by Stephen Lyons - slysven@virginmedia.com    *
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
#include "TTrigger.h"
#include "dlgComposer.h"
#include "dlgIRC.h"
#include "dlgMapper.h"
#include "dlgModuleManager.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"
#if defined(INCLUDE_3DMAPPER)
#include "glwidget.h"
#endif
#include "mapInfoContributorManager.h"

#include "math.h"
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

#include <limits>
using namespace std::chrono_literals;

const QMap<Qt::MouseButton, QString> TLuaInterpreter::mMouseButtons = {
        {Qt::NoButton, qsl("NoButton")},           {Qt::LeftButton, qsl("LeftButton")},       {Qt::RightButton, qsl("RightButton")},
        {Qt::MiddleButton, qsl("MidButton")},      {Qt::BackButton, qsl("BackButton")},       {Qt::ForwardButton, qsl("ForwardButton")},
        {Qt::TaskButton, qsl("TaskButton")},       {Qt::ExtraButton4, qsl("ExtraButton4")},   {Qt::ExtraButton5, qsl("ExtraButton5")},
        {Qt::ExtraButton6, qsl("ExtraButton6")},   {Qt::ExtraButton7, qsl("ExtraButton7")},   {Qt::ExtraButton8, qsl("ExtraButton8")},
        {Qt::ExtraButton9, qsl("ExtraButton9")},   {Qt::ExtraButton10, qsl("ExtraButton10")}, {Qt::ExtraButton11, qsl("ExtraButton11")},
        {Qt::ExtraButton12, qsl("ExtraButton12")}, {Qt::ExtraButton13, qsl("ExtraButton13")}, {Qt::ExtraButton14, qsl("ExtraButton14")},
        {Qt::ExtraButton15, qsl("ExtraButton15")}, {Qt::ExtraButton16, qsl("ExtraButton16")}, {Qt::ExtraButton17, qsl("ExtraButton17")},
        {Qt::ExtraButton18, qsl("ExtraButton18")}, {Qt::ExtraButton19, qsl("ExtraButton19")}, {Qt::ExtraButton20, qsl("ExtraButton20")},
        {Qt::ExtraButton21, qsl("ExtraButton21")}, {Qt::ExtraButton22, qsl("ExtraButton22")}, {Qt::ExtraButton23, qsl("ExtraButton23")},
        {Qt::ExtraButton24, qsl("ExtraButton24")},

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

static const char *bad_window_type = "%s: bad argument #%d type (window name as string expected, got %s)!";
static const char *bad_cmdline_type = "%s: bad argument #%d type (command line name as string expected, got %s)!";
static const char *bad_window_value = "window \"%s\" not found";
static const char *bad_cmdline_value = "command line \"%s\" not found";
static const char *bad_label_value = "label \"%s\" not found";

#define WINDOW_NAME(_L, _pos)                                                                  \
    ({                                                                                         \
        int pos_ = (_pos);                                                                     \
        const char *res_;                                                                      \
        if ((lua_gettop(_L) < pos_) || lua_isnil(_L, pos_)) {                                  \
            res_ = "";                                                                         \
        } else if (!lua_isstring(_L, pos_)) {                                                  \
            lua_pushfstring(_L, bad_window_type, __FUNCTION__, pos_, luaL_typename(_L, pos_)); \
            return lua_error(_L);                                                              \
        } else {                                                                               \
            res_ = lua_tostring(_L, pos_);                                                     \
        }                                                                                      \
        res_;                                                                                  \
    })

#define CMDLINE_NAME(_L, _pos)                                                                 \
    ({                                                                                         \
        int pos_ = (_pos);                                                                     \
        if (!lua_isstring(_L, pos_)) {                                                         \
            lua_pushfstring(_L, bad_cmdline_type, __FUNCTION__, pos_, luaL_typename(_L, pos_));\
            return lua_error(_L);                                                              \
        }                                                                                      \
        lua_tostring(_L, pos_);                                                                \
    })

#define CONSOLE_NIL(_L, _name)                                                                 \
    ({                                                                                         \
        auto name_ = (_name);                                                                  \
        auto console_ = getHostFromLua(_L).findConsole(name_);                                 \
        console_;                                                                              \
    })

#define CONSOLE(_L, _name)                                                                     \
    ({                                                                                         \
        auto name_ = (_name);                                                                  \
        auto console_ = getHostFromLua(_L).findConsole(name_);                                 \
        if (!console_) {                                                                       \
            lua_pushnil(L);                                                                    \
            lua_pushfstring(L, bad_window_value, name_.toUtf8().constData());                  \
            return 2;                                                                          \
        }                                                                                      \
        console_;                                                                              \
    })

#define COMMANDLINE(_L, _name)                                                                 \
    ({                                                                                         \
        const QString& name_ = (_name);                                                        \
        auto console_ = getHostFromLua(_L).mpConsole;                                          \
        auto cmdLine_ = isMain(name_) ? &*console_->mpCommandLine                              \
                                    : console_->mSubCommandLineMap.value(name_);               \
        if (!cmdLine_) {                                                                       \
            lua_pushnil(L);                                                                    \
            lua_pushfstring(L, bad_cmdline_value, name_.toUtf8().constData());                 \
            return 2;                                                                          \
        }                                                                                      \
        cmdLine_;                                                                              \
    })

#define LABEL(_L, _name)                                                                       \
    ({                                                                                         \
        const QString& name_ = (_name);                                                        \
        auto console_ = getHostFromLua(_L).mpConsole;                                          \
        auto label_ = console_->mLabelMap.value(name_);                                        \
        if (!label_) {                                                                         \
            lua_pushnil(L);                                                                    \
            lua_pushfstring(L, bad_label_value, name_.toUtf8().constData());                   \
            return 2;                                                                          \
        }                                                                                      \
        label_;                                                                                \
    })

// variable names within these macros have trailing underscores because in
// at least one case, masking an existing variable with the new one confused
// GCC, leading to a crash.


TLuaInterpreter::TLuaInterpreter(Host* pH, const QString& hostName, int id) : mpHost(pH), hostName(hostName), mHostID(id), purgeTimer(this)
{
    pGlobalLua = nullptr;

    connect(&purgeTimer, &QTimer::timeout, this, &TLuaInterpreter::slotPurge);

    mpFileDownloader = new QNetworkAccessManager(this);
    connect(mpFileDownloader, &QNetworkAccessManager::finished, this, &TLuaInterpreter::slot_httpRequestFinished);

    mpFileSystemWatcher = new QFileSystemWatcher(this);
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
// The "isOptional" parameter is optional, and will default to not-optional parameters! :)
//
// See also: getVerifiedString, getVerifiedInt, getVerifiedFloat, errorArgumentType
bool TLuaInterpreter::getVerifiedBool(lua_State* L, const char* functionName, const int pos, const char* publicName, const bool isOptional)
{
    if (!lua_isboolean(L, pos)) {
        errorArgumentType(L, functionName, pos, publicName, "boolean", isOptional);
        lua_error(L);
        Q_UNREACHABLE();
        return false;
    }
    return lua_toboolean(L, pos);
}

// No documentation available in wiki - internal function
// See also: getVerifiedBool
QString TLuaInterpreter::getVerifiedString(lua_State* L, const char* functionName, const int pos, const char* publicName, const bool isOptional)
{
    if (!lua_isstring(L, pos)) {
        errorArgumentType(L, functionName, pos, publicName, "string", isOptional);
        lua_error(L);
        Q_UNREACHABLE();
        return QString();
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
        return -1;
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
        return 0;
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
        return 0;
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
    if (mudlet::debugMode) {
        auto& host = getHostFromLua(L);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        TDebug(Qt::white, QColorConstants::Svg::orange) << "Lua: " << functionName << ": " << message << "\n" >> &host;
#else
        TDebug(Qt::white, QColor("orange")) << "Lua: " << functionName << ": " << message << "\n" >> &host;
#endif
    }
    return 2;
}

int TLuaInterpreter::warnArgumentValue(lua_State* L, const char* functionName, const char* message, const bool useFalseInsteadofNil)
{
    if (Q_LIKELY(!useFalseInsteadofNil)) {
        lua_pushnil(L);
    } else {
        lua_pushboolean(L, false);
    }
    lua_pushstring(L, message);
    if (mudlet::debugMode) {
        auto& host = getHostFromLua(L);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        TDebug(Qt::white, QColorConstants::Svg::orange) << "Lua: " << functionName << ": " << message << "\n" >> &host;
#else
        TDebug(Qt::white, QColor("orange")) << "Lua: " << functionName << ": " << message << "\n" >> &host;
#endif
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
            event.mArgumentList << QString(reply->readAll());
            event.mArgumentTypeList << ARGUMENT_TYPE_STRING;
            break;
        }

        QFile localFile(localFileName);
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

        qint64 bytesWritten = localFile.write(reply->readAll());
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
            event.mArgumentList << QLatin1String("Couldn't save to the destination file");
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
        return lua_error(L);
    }

    int luaSleepMsec = getVerifiedInt(L, __func__, 1, "sleep time in msec");
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
        qint64 dirNum = lua_tonumber(L, position);
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#raiseEvent
int TLuaInterpreter::raiseEvent(lua_State* L)
{
    Host& host = getHostFromLua(L);

    TEvent event {};

    int n = lua_gettop(L);
    // We go from the top of the stack down, because luaL_ref will
    // only reference the object at the top of the stack
    for (int i = n; i >= 1; i--) {
        switch (lua_type(L, -1)) {
        case LUA_TNUMBER:
            // https://en.wikipedia.org/wiki/Double-precision_floating-point_format#IEEE_754_double-precision_binary_floating-point_format:_binary64
            // suggests that 17 decimal digits is the most we can rely on:
            event.mArgumentList.prepend(QString::number(lua_tonumber(L, -1), 'g', 17));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_NUMBER);
            lua_pop(L, 1);
            break;
        case LUA_TSTRING:
            event.mArgumentList.prepend(lua_tostring(L, -1));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_STRING);
            lua_pop(L, 1);
            break;
        case LUA_TBOOLEAN:
            event.mArgumentList.prepend(QString::number(lua_toboolean(L, -1)));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_BOOLEAN);
            lua_pop(L, 1);
            break;
        case LUA_TNIL:
            event.mArgumentList.prepend(QString());
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_NIL);
            lua_pop(L, 1);
            break;
        case LUA_TTABLE:
            event.mArgumentList.prepend(QString::number(luaL_ref(L, LUA_REGISTRYINDEX)));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_TABLE);
            // luaL_ref pops the object, so we don't have to
            break;
        case LUA_TFUNCTION:
            event.mArgumentList.prepend(QString::number(luaL_ref(L, LUA_REGISTRYINDEX)));
            event.mArgumentTypeList.prepend(ARGUMENT_TYPE_FUNCTION);
            // luaL_ref pops the object, so we don't have to
            break;
        default:
            lua_pushfstring(L,
                            "raiseEvent: bad argument #%d type (string, number, boolean, table,\n"
                            "function, or nil expected, got a %s!)",
                            i,
                            luaL_typename(L, -1));
            return lua_error(L);
        }
    }

    host.raiseEvent(event);

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getProfileName
int TLuaInterpreter::getProfileName(lua_State* L)
{
    Host& host = getHostFromLua(L);
    lua_pushstring(L, host.getName().toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getProfileTabNumber
int TLuaInterpreter::getProfileTabNumber(lua_State* L)
{
    Host& host = getHostFromLua(L);
    auto profileIndex = mudlet::self()->mpTabBar->tabIndex(host.getName());
    if (profileIndex != -1) {
        lua_pushnumber(L, profileIndex + 1);
        return 1;
    }

    return warnArgumentValue(L, __func__, "could not retrieve the tab number");
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
        return lua_error(L);
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
        switch (lua_type(L, i)) {
        case LUA_TNUMBER:
            event.mArgumentList.append(QString::number(lua_tonumber(L, i), 'g', 17));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
            break;
        case LUA_TSTRING:
            event.mArgumentList.append(lua_tostring(L, i));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            break;
        case LUA_TBOOLEAN:
            event.mArgumentList.append(QString::number(lua_toboolean(L, i)));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_BOOLEAN);
            break;
        case LUA_TNIL:
            event.mArgumentList.append(QString());
            event.mArgumentTypeList.append(ARGUMENT_TYPE_NIL);
            break;
        default:
            lua_pushfstring(L,
                            "raiseGlobalEvent: bad argument type #%d (boolean, number, string or nil\n"
                            "expected, got a %s!)",
                            i,
                            luaL_typename(L, i));
            return lua_error(L);
        }
    }

    event.mArgumentList.append(host.getName());
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);

    mudlet::self()->getHostManager().postInterHostEvent(&host, event);

    lua_pushboolean(L, true);
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectString
int TLuaInterpreter::selectString(lua_State* L)
{
    int s = 1;
    QString windowName;
    if (lua_gettop(L) > 2) {
        windowName = WINDOW_NAME(L, s++);
    }

    QString searchText = getVerifiedString(L, __func__, s++, "text to select");
    // CHECK: Do we need to qualify this for a non-blank string?

    qint64 numOfMatch = static_cast <qint64> (getVerifiedInt(L, __func__, s, "match count {1 for first}"));

    auto console = CONSOLE(L, windowName);
    lua_pushnumber(L, console->select(searchText, numOfMatch));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectCurrentLine
int TLuaInterpreter::selectCurrentLine(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L) > 0) {
        windowName = WINDOW_NAME(L, 1);
    }

    auto console = CONSOLE(L, windowName);
    console->selectCurrentLine();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isAnsiFgColor
int TLuaInterpreter::isAnsiFgColor(lua_State* L)
{
    std::string windowName = "main";
    int ansiFg = getVerifiedInt(L, __func__, 1, "ANSI color");

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
    std::string windowName = "main";
    int ansiBg = getVerifiedInt(L, __func__, 1, "ANSI color");

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
        windowName = getVerifiedString(L, __func__, 1, "window name", true).toStdString();
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
        windowName = getVerifiedString(L, __func__, 1, "window name", true).toStdString();
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
    QString windowName;
    if (lua_gettop(L)) {
        windowName = getVerifiedString(L, __func__, 1, "window name", true);
    }

    Host& host = getHostFromLua(L);
    QPair<quint8, TChar> result = host.mpConsole->getTextAttributes(windowName);
    if (result.first == 1) {
        return warnArgumentValue(L, __func__, qsl("window '%1' not found").arg(windowName));
    }

    if (result.first == 2) {
        return warnArgumentValue(L, __func__, qsl("current selection invalid in window '%1'").arg(windowName));
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
    QSettings registry(qsl(R"(HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Nls\CodePage)"),
                       QSettings::NativeFormat);
    auto value = registry.value(qsl("ACP"));
    lua_pushstring(L, value.toString().toUtf8().constData());
    return 1;
#else
    return warnArgumentValue(L, __func__, "this function is only needed on Windows, and does not work here");
#endif
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#wrapLine
int TLuaInterpreter::wrapLine(lua_State* L)
{
    int s = 1;
    std::string windowName;
    if (lua_gettop(L)) {
        windowName = getVerifiedString(L, __func__, s++, "window name").toStdString();
    }
    int lineNumber = getVerifiedInt(L, __func__, s, "line");

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
int TLuaInterpreter::selectCaptureGroup(lua_State *L)
{
    if (!(lua_isnumber(L, 1) || lua_isstring(L, 1))) {
        lua_pushfstring(L,
                        "selectCaptureGroup: bad argument #1 type (capture group as number or capture group name as string expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }

    Host &host = getHostFromLua(L);
    TLuaInterpreter *pL = host.getLuaInterpreter();
    int begin = 0;
    int length = 0;

    if (lua_isnumber(L, 1)) {
        auto captureGroup = lua_tonumber(L, 1);
        if (captureGroup < 1) {
            lua_pushnumber(L, -1);
            return 1;
        }
        // We want capture groups to start with 1 instead of 0 so predecrement
        // luaNumOfMatch :
        if (--captureGroup < static_cast<int>(host.getLuaInterpreter()->mCaptureGroupList.size())) {
            auto iti = pL->mCaptureGroupPosList.begin();
            auto its = pL->mCaptureGroupList.begin();
            begin = *iti;
            std::string &s = *its;

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

            length = QString::fromStdString(s).size();
            if (mudlet::debugMode) {
                TDebug(Qt::white, Qt::red) << "selectCaptureGroup(" << begin << ", " << length << ")\n" >> &host;
            }
        }
    } else if (lua_isstring(L, 1)) {
        auto name = lua_tostring(L, 1);
        if (pL->mCapturedNameGroupsPosList.contains(name)) {
            begin = pL->mCapturedNameGroupsPosList.value(name).first;
            length = pL->mCapturedNameGroupsPosList.value(name).second;
        }
    }
    if (length > 0) {
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
    int s = 1;
    QString windowName;
    if (n > 2) {
        windowName = getVerifiedString(L, __func__, s++, "mini console, user window or buffer name {may be omitted for the \"main\" console}", true);
    }
    int lineFrom = getVerifiedInt(L, __func__, s++, "start line");
    int lineTo = getVerifiedInt(L, __func__, s, "end line");

    Host& host = getHostFromLua(L);
    QPair<bool, QStringList> result = host.getLines(windowName, lineFrom, lineTo);
    if (!result.first) {
        // Only one QString in .second - the error message
        return warnArgumentValue(L, __func__, result.second.at(0));
    }
    lua_newtable(L);
    for (int i = 0, total = result.second.size(); i < total; ++i) {
        lua_pushnumber(L, i + 1);
        lua_pushstring(L, result.second.at(i).toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadReplay
int TLuaInterpreter::loadReplay(lua_State* L)
{
    QString replayFileName = getVerifiedString(L, __func__, 1, "replay file name");
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setProfileIcon
int TLuaInterpreter::setProfileIcon(lua_State* L)
{
    QString iconPath = getVerifiedString(L, __func__, 1, "icon file path");
    if (iconPath.isEmpty()) {
        return warnArgumentValue(L, __func__, "a blank string is not a valid icon file path");
    }
    if (!QFileInfo::exists(iconPath)) {
        return warnArgumentValue(L, __func__, qsl("path '%1' doesn't exist").arg(iconPath));
    }

    Host& host = getHostFromLua(L);

    auto[success, message] = mudlet::self()->setProfileIcon(host.getName(), iconPath);
    if (!success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetProfileIcon
int TLuaInterpreter::resetProfileIcon(lua_State* L)
{
    Host& host = getHostFromLua(L);

    auto [success, message] = mudlet::self()->resetProfileIcon(host.getName());
    if (!success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCurrentLine
int TLuaInterpreter::getCurrentLine(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = getHostFromLua(L).findConsole(windowName);
    if (!console) {
        // the next line should be "pushnil"; compatibility with old bugs and all that
        lua_pushstring(L, "ERROR: mini console does not exist");
        lua_pushfstring(L, bad_window_value, windowName.toUtf8().constData());
        return 2;
    }
    QString line = console->getCurrentLine();
    lua_pushstring(L, line.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMiniConsoleFontSize
int TLuaInterpreter::setMiniConsoleFontSize(lua_State* L)
{
    QString windowName = getVerifiedString(L, __func__, 1, "miniconsole name");
    int size = getVerifiedInt(L, __func__, 2, "font size");
    auto console = CONSOLE(L, windowName);
    if (console->setFontSize(size)) {
        lua_pushboolean(L, true);
    } else {
        return warnArgumentValue(L, __func__, qsl("setting font size of '%1' failed").arg(windowName));
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLineNumber
int TLuaInterpreter::getLineNumber(lua_State* L)
{
    QString windowName;
    int s = 0;

    if (lua_gettop(L) > 0) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, ++s);
    }

    auto console = CONSOLE(L, windowName);
    lua_pushnumber(L, console->getLineNumber());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#updateMap
int TLuaInterpreter::updateMap(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (host.mpMap) {
        host.mpMap->update();
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addMapMenu
int TLuaInterpreter::addMapMenu(lua_State* L)
{
    //    first arg = unique name, second arg= parent name, third arg = display name (=unique name if not provided)
    QStringList menuList;
    QString uniqueName = getVerifiedString(L, __func__, 1, "uniquename");

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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeMapMenu
int TLuaInterpreter::removeMapMenu(lua_State* L)
{
    QString uniqueName = getVerifiedString(L, __func__, 1, "Menu name");
    if (uniqueName.isEmpty()) {
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapMenus
int TLuaInterpreter::getMapMenus(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!(host.mpMap && host.mpMap->mpMapper && host.mpMap->mpMapper->mp2dMap)) {
        return warnArgumentValue(L, __func__, "you haven't opened a map yet");
    }

    lua_newtable(L);
    QMapIterator<QString, QStringList> it(host.mpMap->mpMapper->mp2dMap->mUserMenus);
    while (it.hasNext()) {
        it.next();
        QString parent, display;
        QStringList menuInfo = it.value();
        parent = menuInfo[0];
        display = menuInfo[1];
        qDebug() << it.key() << parent << display;
        lua_pushstring(L, display.toUtf8().constData());
        lua_pushstring(L, parent.isEmpty() ? "top-level" :parent.toUtf8().constData());
        lua_settable(L, -3);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addMapEvent
int TLuaInterpreter::addMapEvent(lua_State* L)
{
    QStringList actionInfo;
    QString uniqueName = getVerifiedString(L, __func__, 1, "uniquename");
    actionInfo << getVerifiedString(L, __func__, 2, "event name");

    if (!lua_isstring(L, 3)) {
        actionInfo << QString();
    } else {
        actionInfo << lua_tostring(L, 3);
    }
    if (!lua_isstring(L, 4)) {
        actionInfo << uniqueName;
    } else {
        actionInfo << lua_tostring(L, 4);
    }
    //variable number of arguments
    for (int i = 5; i <= lua_gettop(L); i++) {
        actionInfo << lua_tostring(L, i);
    }
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
    QString displayName = getVerifiedString(L, __func__, 1, "event name");
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
        return warnArgumentValue(L, __func__, "you haven't opened a map yet");
    }

    int roomId = getVerifiedInt(L, __func__, 1, "room id");

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
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id.").arg(roomId));
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPlayerRoom
int TLuaInterpreter::getPlayerRoom(lua_State* L)
{
    Host& host = getHostFromLua(L);

    if (!host.mpMap || !host.mpMap->mpRoomDB || !host.mpMap->mpMapper) {
        return warnArgumentValue(L, __func__, "you haven't opened a map yet");
    }

    auto roomID = host.mpMap->mRoomIdHash.value(host.getName(), -1);
    if (roomID == -1) {
        return warnArgumentValue(L, __func__, "the player does not have a valid room id set");
    }
    lua_pushnumber(L, roomID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#copy
int TLuaInterpreter::copy(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L) > 0) {
        windowName = WINDOW_NAME(L, 1);
    }

    auto console = CONSOLE(L, windowName);
    console->copy();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#cut
int TLuaInterpreter::cut(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.mpConsole->cut();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#paste
int TLuaInterpreter::paste(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L) > 0) {
        windowName = WINDOW_NAME(L, 1);
    }

    auto console = CONSOLE(L, windowName);
    console->paste();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#feedTriggers
int TLuaInterpreter::feedTriggers(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L,
                        "feedTriggers: bad argument #1 type (imitation game server text as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }
    QByteArray data{lua_tostring(L, 1)};
    bool dataIsUtf8Encoded = true;
    if (lua_gettop(L) > 1) {
        dataIsUtf8Encoded = getVerifiedBool(L, __func__, 2, "Utf8Encoded", true);
    }

    QByteArray currentEncoding = host.mTelnet.getEncoding();
    if (dataIsUtf8Encoded) {
        // We can convert the data from a QByteArray to a QString:
        if (currentEncoding == "UTF-8") {
            // Simple case: the encoding is already what we are using:
            std::string dataStdString{data.toStdString()};
            host.mpConsole->printOnDisplay(dataStdString);
            lua_pushboolean(L, true);
            return 1;
        }
        QString dataQString{data};
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
        for (int i = 0, total = dataQString.size(); i < total; ++i) {
            if (dataQString.at(i).row() || dataQString.at(i).cell() > 127) {
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
    int s = 1;
    QString windowName;
    if (lua_gettop(L) > 1) {
        windowName = WINDOW_NAME(L, s++);
    }
    int luaFrom = getVerifiedInt(L, __func__, s, "wrapAt");
    auto console = CONSOLE(L, windowName);
    console->setWrapAt(luaFrom);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getWindowWrap
int TLuaInterpreter::getWindowWrap(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};

    auto console = CONSOLE(L, windowName);
    lua_pushnumber(L, console->getWrapAt());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setWindowWrapIndent
int TLuaInterpreter::setWindowWrapIndent(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    int luaFrom = getVerifiedInt(L, __func__, 2, "wrapTo");
    auto console = CONSOLE(L, windowName);
    console->setIndentCount(luaFrom);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLineCount
int TLuaInterpreter::getLineCount(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L) > 0) {
        windowName = WINDOW_NAME(L, 1);
    }

    auto console = CONSOLE(L, windowName);
    lua_pushnumber(L, console->getLineCount());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getColumnNumber
int TLuaInterpreter::getColumnNumber(lua_State* L)
{
    QString windowName;
    if (lua_gettop(L) > 0) {
        windowName = WINDOW_NAME(L, 1);
    }

    auto console = CONSOLE(L, windowName);
    lua_pushnumber(L, console->getColumnNumber());
    return 1;
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
            return warnArgumentValue(L, __func__, qsl("stopwatch with id %1 not found").arg(watchId));
        }

    } else {
        QString name{lua_tostring(L, 1)};
        // Using an empty string will return the first unnamed stopwatch:
        watchId = host.findStopWatchId(name);
        if (!watchId) {
            if (name.isEmpty()) {
                return warnArgumentValue(L, __func__, "no unnamed stopwatches found");
            }
            return warnArgumentValue(L, __func__, qsl("stopwatch with name '%1' not found").arg(name));
        }

        result = host.getStopWatchTime(watchId);
        // We have already validated the name to get the watchId - so for things
        // to fail now is, unlikely?
        if (Q_UNLIKELY(!result.first)) {
            return warnArgumentValue(L, __func__, qsl(
                "stopwatch with name '%1' (id: %2) has disappeared - this should not happen, please report it to Mudlet developers")
                .arg(name, QString::number(watchId)));
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
    int s = 1;
    if (n) {
        if (lua_type(L, s) == LUA_TBOOLEAN) {
            autoStart = lua_toboolean(L, s);
        } else if (lua_type(L, s) == LUA_TSTRING) {
            autoStart = false;
            name = lua_tostring(L, 1);
        } else if (lua_type(L, s) == LUA_TNIL) {
            ; // fallthrough for compatibility with old-style stopwatches in case createStopWatch(nil) is passed
            // note that 'nil' will still count towards the stack's gettop amount
        } else {
            lua_pushfstring(L, "createStopWatch: bad argument #%d type (name as string or autostart as boolean are optional, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        }

        if (n > 1) {
            autoStart = getVerifiedBool(L, __func__, ++s, "autostart", true);
        }
    }


    Host& host = getHostFromLua(L);
    QPair<int, QString> result = host.createStopWatch(name);
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
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
            return warnArgumentValue(L, __func__, result.second);
        }

    } else {
        QString name{lua_tostring(L, 1)};
        QPair<bool, QString> result = host.stopStopWatch(name);
        if (!result.first) {
            return warnArgumentValue(L, __func__, result.second);
        }

        watchId = host.findStopWatchId(name);
        // We have already validated the name to get the watchId - so for things
        // to fail now is, unlikely?
        if (Q_UNLIKELY(!watchId)) {
            return warnArgumentValue(L, __func__, qsl(
                "stopwatch with name '%1' (id: %2) has disappeared - this should not happen, please report it to Mudlet developers")
                .arg(name, QString::number(watchId)));
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
            autoResetAndRestart = getVerifiedBool(L, __func__, 2, "automatic reset and restart with a numeric stopwatch id", true);
        }

        QPair<bool, QString> result;
        if (autoResetAndRestart) {
            result = host.resetAndRestartStopWatch(static_cast<int>(lua_tointeger(L, 1)));
        } else {
            result = host.startStopWatch(static_cast<int>(lua_tointeger(L, 1)));
        }
        if (!result.first) {
            return warnArgumentValue(L, __func__, result.second);
        }

        lua_pushboolean(L, true);
        return 1;
    }

    QPair<bool, QString> result = host.startStopWatch(lua_tostring(L, 1));
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
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
            return warnArgumentValue(L, __func__, result.second);
        }

        lua_pushboolean(L, true);
        return 1;
    }

    QPair<bool, QString> result = host.resetStopWatch(lua_tostring(L, 1));
    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
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

    QString name{lua_tostring(L, 1)};
    // Using an empty string will return the first unnamed stopwatch:
    int watchId = h.findStopWatchId(name);
    if (!watchId) {
        lua_pushnil(L);
        if (name.isEmpty()) {
            lua_pushstring(L, "no unnamed stopwatches found");
        } else {
            lua_pushfstring(L, "stopwatch with name '%s' not found", name.toUtf8().constData());
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

    double adjustment = getVerifiedDouble(L, __func__, 2, "modification in seconds");
    bool result = host.adjustStopWatch(watchId, qRound(adjustment * 1000.0));
    // This is only likely to fail when a numeric first argument was given:
    if (!result) {
        return warnArgumentValue(L, __func__, qsl("stopwatch with id %1 not found").arg(watchId));
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
        return warnArgumentValue(L, __func__, qsl("stopwatch with id %1 not found").arg(watchId));
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

    bool isPersistent = getVerifiedBool(L, __func__, 2, "persistence");

    // This is only likely to fail when a numeric first argument was given:
    if (!host.makeStopWatchPersistent(watchId, isPersistent)) {
        return warnArgumentValue(L, __func__, qsl("stopwatch with id %1 not found").arg(watchId));
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
        currentName = lua_tostring(L, 1);
    }

    QString newName = getVerifiedString(L, __func__, 2, "stopwatch new name");

    QPair<bool, QString> result;
    if (currentName.isNull()) {
        // Will be null if no value was assigned to it - so use the id form:
        result = host.setStopWatchName(watchId, newName);
    } else {
        result = host.setStopWatchName(currentName, newName);
    }

    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second);
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
        return warnArgumentValue(L, __func__, result.second);
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
    int s = 1;
    QString windowName;

    if (lua_gettop(L) > 2) {
        windowName = WINDOW_NAME(L, s++);
    }
    int from = getVerifiedInt(L, __func__, s++, "from position");
    int to = getVerifiedInt(L, __func__, s, "length");

    auto console = CONSOLE(L, windowName);
    int ret = console->selectSection(from, to);
    lua_pushboolean(L, ret != -1);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getSelection
int TLuaInterpreter::getSelection(lua_State* L)
{
    bool valid;
    QString text;
    int start, length;

    QString windowName;
    if (lua_gettop(L) > 0) {
        windowName = WINDOW_NAME(L, 1);
    }
    auto console = CONSOLE(L, windowName);
    std::tie(valid, text, start, length) = console->getSelection();

    if (!valid) {
        return warnArgumentValue(L, __func__, text);
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
    QString windowName;
    if (n > 2) {
        windowName = WINDOW_NAME(L, s++);
    }

    int luaFrom = getVerifiedInt(L, __func__, s++, "x");
    int luaTo = getVerifiedInt(L, __func__, s, "y");

    auto console = CONSOLE(L, windowName);
    lua_pushboolean(L, console->moveCursor(luaFrom, luaTo));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setConsoleBufferSize
int TLuaInterpreter::setConsoleBufferSize(lua_State* L)
{
    int s = 1;
    int n = lua_gettop(L);
    QString windowName;
    if (n > 2) {
        windowName = WINDOW_NAME(L, s++);
    }

    int luaFrom = getVerifiedInt(L, __func__, s++, "linesLimit");
    int luaTo = getVerifiedInt(L, __func__, s, "sizeOfBatchDeletion");

    auto console = CONSOLE(L, windowName);
    console->buffer.setBufferSize(luaFrom, luaTo);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableScrollBar
int TLuaInterpreter::enableScrollBar(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setScrollBarVisible(true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableScrollBar
int TLuaInterpreter::disableScrollBar(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setScrollBarVisible(false);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableHorizontalScrollBar
int TLuaInterpreter::enableHorizontalScrollBar(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setHorizontalScrollBar(true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableHorizontalScrollBar
int TLuaInterpreter::disableHorizontalScrollBar(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setHorizontalScrollBar(false);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableCommandLine
int TLuaInterpreter::enableCommandLine(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setCmdVisible(true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableCommandLine
int TLuaInterpreter::disableCommandLine(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->setCmdVisible(false);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#replace
int TLuaInterpreter::replace(lua_State* L)
{
    int n = lua_gettop(L);
    int s = 1;
    QString windowName;

    if (n > 1) {
        windowName = WINDOW_NAME(L, s++);
    }
    QString text = getVerifiedString(L, __func__, s, "with");

    auto console = CONSOLE(L, windowName);
    console->replace(text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteLine
int TLuaInterpreter::deleteLine(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->skipLine();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#saveMap
int TLuaInterpreter::saveMap(lua_State* L)
{
    QString location;
    int saveVersion = 0;

    if (lua_gettop(L) > 0) {
        location = getVerifiedString(L, __func__, 1, "save location path and file name", true);
        if (lua_gettop(L) > 1) {
            saveVersion = getVerifiedInt(L, __func__, 2, "map format version", true);
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
    int roomId = getVerifiedInt(L, __func__, 1, "roomID");

    int dir = dirToNumber(L, 2);
    if (!dir) {
        lua_pushfstring(L, "setExitStub: bad argument #2 type (direction as number or string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    bool status = getVerifiedBool(L, __func__, 3, "set/unset");

    Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return 0;
    }
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        lua_pushstring(L, "setExitStub: RoomId doesn't exist");
        return lua_error(L);
    }
    if (dir > 12 || dir < 1) {
        lua_pushstring(L, "setExitStub: direction must be between 1 and 12");
        return lua_error(L);
    }
    pR->setExitStub(dir, status);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#connectExitStub
int TLuaInterpreter::connectExitStub(lua_State* L)
{
    int toRoom = 0;
    bool hasDirection = false;
    bool hasToRoomId = false;
    int fromRoom = getVerifiedInt(L, __func__, 1, "fromID");

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }
    if (lua_gettop(L) < 2) {
        lua_pushfstring(L, "connectExitStub: missing argument #2 (toID as number or direction as number or string expected)");
        return lua_error(L); // lua_error() doesn't return to here!
    }

    if (lua_gettop(L) > 2) {
        // Both toRoomID AND direction given
        hasDirection = true;
        hasToRoomId = true;
    } else {
        // Only have one of toRoomID or direction given - we need to examine the
        // argument more closely
        if (lua_type(L, 2) == LUA_TSTRING) {
            // It is a string so it is (we will assume) a direction
            hasDirection = true;
        } else if (lua_type(L, 2) == LUA_TNUMBER) {
            int value = qRound(lua_tonumber(L, 2));
            if (value >= DIR_OUT || value <= DIR_NORTH) {
                // Ambiguous - look in more detail and check whether there is a
                // a room with the given number and/or an exit stub:
                bool hasRoomWithNumberAsId = static_cast<bool>(host.mpMap->mpRoomDB->getRoom(value));
                auto pR = host.mpMap->mpRoomDB->getRoom(fromRoom);
                bool hasExitStubWithNumberAsDirection = (pR && pR->exitStubs.contains(value));
                if (hasRoomWithNumberAsId) {
                    if (hasExitStubWithNumberAsDirection) {
                        return warnArgumentValue(
                                L, __func__, qsl("%1 is too ambiguous a number to parse into a toID or a direction code as both are valid in this case. If this is a direction, try providing it as a string").arg(lua_tonumber(L, 2)));
                    }
                    // else - usable as only one of the two flags is set:
                    hasToRoomId = true;
                } else {
                    if (!hasExitStubWithNumberAsDirection) {
                        // not usable, as neither flag is set:
                        return warnArgumentValue(L, __func__, qsl("%1 is not valid as a toID nor a direction code").arg(lua_tonumber(L, 2)));
                    }
                    // else - usable as only one of the two flags is set:
                    hasDirection = true;
                }
            } else {
                // it is a number greater than 12 so it is (we will assume) a
                // toRoomID - or it is zero or a negative number and will never
                // work as a roomID but treat it as such so that it will trigger
                // an invalid roomID run-time error message:
                hasToRoomId = true;
            }

        } else {
            errorArgumentType(L, __func__, 2, "toID or direction", "number or string");
            return lua_error(L); // lua_error() doesn't return to here!
        }
    }

    // dirType will be 1 to 12 if it was parsed as one of that range as a NUMBER
    // or an (English) STRING of one of the directions
    int dirType = 0;
    if (hasDirection) {
        int argNumber = hasToRoomId ? 3 : 2;
        dirType = dirToNumber(L, argNumber);
        if (!dirType) {
            return warnArgumentValue(L, __func__, qsl("argument %1 as '%2' cannot be parsed as a valid direction").arg(QString::number(argNumber), QString::fromUtf8(lua_tostring(L, argNumber))));
        }
    }

    if (hasToRoomId) {
        toRoom = getVerifiedInt(L, __func__, 2, "toID");
    }

    QString errMsg;
    if (hasDirection) {
        if (hasToRoomId) {
            errMsg = host.mpMap->connectExitStubByDirectionAndToId(fromRoom, dirType, toRoom);
        } else {
            errMsg = host.mpMap->connectExitStubByDirection(fromRoom, dirType);
        }

    } else /* effectively: if (!hasDirection && hasToRoomId) */ {
        errMsg = host.mpMap->connectExitStubByToId(fromRoom, toRoom);
    }

    if (!errMsg.isEmpty()) {
        lua_pushnil(L);
        lua_pushstring(L, errMsg.toUtf8().constData());
        return 2;
    }

    host.mpMap->mMapGraphNeedsUpdate = true;
    // equivalent to a call to updateMap(L):
    host.mpMap->update();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getExitStubs
int TLuaInterpreter::getExitStubs(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int roomId = getVerifiedInt(L, __func__, 1, "room id");

    // Previously threw a Lua error on non-existent room!
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(roomId));
    }
    QList<int> stubs = pR->exitStubs;
    lua_newtable(L);
    for (int i = 0, total = stubs.size(); i < total; ++i) {
        lua_pushnumber(L, i);
        lua_pushnumber(L, stubs.at(i));
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getExitStubs1
int TLuaInterpreter::getExitStubs1(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int roomId = getVerifiedInt(L, __func__, 1, "room id");

    // Previously threw a Lua error on non-existent room!
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(roomId));
    }
    QList<int> stubs = pR->exitStubs;
    lua_newtable(L);
    for (int i = 0, total = stubs.size(); i < total; ++i) {
        lua_pushnumber(L, i + 1);
        lua_pushnumber(L, stubs.at(i));
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getModulePath
int TLuaInterpreter::getModulePath(lua_State* L)
{
    QString moduleName = getVerifiedString(L, __func__, 1, "module name");
    Host& host = getHostFromLua(L);
    QMap<QString, QStringList> modules = host.mInstalledModules;
    if (modules.contains(moduleName)) {
        QString modPath = modules[moduleName][0];
        lua_pushstring(L, modPath.toUtf8().constData());
        return 1;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getModulePriority
int TLuaInterpreter::getModulePriority(lua_State* L)
{
    QString moduleName = getVerifiedString(L, __func__, 1, "module name");
    Host& host = getHostFromLua(L);
    if (host.mModulePriorities.contains(moduleName)) {
        int priority = host.mModulePriorities[moduleName];
        lua_pushnumber(L, priority);
        return 1;
    } else {
        lua_pushstring(L, "getModulePriority: Module doesn't exist");
        return lua_error(L);
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setModulePriority
int TLuaInterpreter::setModulePriority(lua_State* L)
{
    QString moduleName = getVerifiedString(L, __func__, 1, "module name");
    int modulePriority = getVerifiedInt(L, __func__, 2, "module priority");

    Host& host = getHostFromLua(L);
    if (!host.mInstalledModules.contains(moduleName)) {
        return warnArgumentValue(L, __func__, "module doesn't exist");
    }
    host.mModulePriorities[moduleName] = modulePriority;
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadMap
int TLuaInterpreter::loadMap(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString location;
    if (lua_gettop(L)) {
        location = getVerifiedString(L, __func__, 1, "Map pathFile {loads last stored map if omitted}", true);
    }

    bool isOk = false;
    if (!location.isEmpty() && location.endsWith(qsl(".xml"), Qt::CaseInsensitive)) {
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
    QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    bool error = host.getTimerUnit()->enableTimer(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableTimer
int TLuaInterpreter::disableTimer(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    bool error = host.getTimerUnit()->disableTimer(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableKey
int TLuaInterpreter::enableKey(lua_State* L)
{
    QString keyName = getVerifiedString(L, __func__, 1, "key name");
    Host& host = getHostFromLua(L);
    bool error = host.getKeyUnit()->enableKey(keyName);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableKey
int TLuaInterpreter::disableKey(lua_State* L)
{
    QString keyName = getVerifiedString(L, __func__, 1, "key name");
    Host& host = getHostFromLua(L);
    bool error = host.getKeyUnit()->disableKey(keyName);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killKey
int TLuaInterpreter::killKey(lua_State* L)
{
    QString keyName = getVerifiedString(L, __func__, 1, "key name");
    Host& host = getHostFromLua(L);
    bool error = host.getKeyUnit()->killKey(keyName);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableAlias
int TLuaInterpreter::enableAlias(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    bool error = host.getAliasUnit()->enableAlias(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableAlias
int TLuaInterpreter::disableAlias(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    bool error = host.getAliasUnit()->disableAlias(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killAlias
int TLuaInterpreter::killAlias(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.getAliasUnit()->killAlias(text));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableTrigger
int TLuaInterpreter::enableTrigger(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    bool error = host.getTriggerUnit()->enableTrigger(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableTrigger
int TLuaInterpreter::disableTrigger(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    bool error = host.getTriggerUnit()->disableTrigger(text);
    lua_pushboolean(L, error);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableScript
int TLuaInterpreter::enableScript(lua_State* L)
{
    QString name = getVerifiedString(L, __func__, 1, "script name");

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
        return warnArgumentValue(L, __func__, qsl("script '%1' not found").arg(name));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableScript
int TLuaInterpreter::disableScript(lua_State* L)
{
    QString name = getVerifiedString(L, __func__, 1, "script name");

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
        return warnArgumentValue(L, __func__, qsl("script '%1' not found").arg(name));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killTimer
int TLuaInterpreter::killTimer(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "ID");
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.killTimer(text));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killTrigger
int TLuaInterpreter::killTrigger(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "ID");
    Host& host = getHostFromLua(L);
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
        timerName = lua_tostring(L, 1);
        result = host.getTimerUnit()->remainingTime(timerName);
    }

    if (result == -1) {
        return warnArgumentValue(L, __func__, "timer is inactive or expired");
    }

    if (result == -2) {
        if (timerName.isNull()) {
            // timerName was never set so we must have used the number
            return warnArgumentValue(L, __func__, qsl("timer id %1 not found").arg(timerId));
        }
        return warnArgumentValue(L, __func__, qsl("timer named '%1' not found").arg(timerName));
    }

    lua_pushnumber(L, result / 1000.0);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#closeMudlet
int TLuaInterpreter::closeMudlet(lua_State* L)
{
    Q_UNUSED(L)
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
        saveToDir = lua_tostring(L, 1);
    }

    auto [ok, filename, error] = host.saveProfile(saveToDir);

    if (ok) {
        lua_pushboolean(L, true);
        lua_pushstring(L, (filename.toUtf8().constData()));
        return 2;
    } else {
        auto message = QString("Couldn't save '%1' to '%2' because: %3").arg(host.getName(), filename, error);
        return warnArgumentValue(L, __func__, message);
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setFont
int TLuaInterpreter::setFont(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }

    QString font = getVerifiedString(L, __func__, s, "name");

    if (!mudlet::self()->getAvailableFonts().contains(font, Qt::CaseInsensitive)) {
        return warnArgumentValue(L, __func__, qsl("font '%1' is not available").arg(font));
    }

#if defined(Q_OS_LINUX)
    // On Linux ensure that emojis are displayed in colour even if this font
    // doesn't support it:
    QFont::insertSubstitution(font, qsl("Noto Color Emoji"));
    // TODO issue #4159: a nonexisting font breaks the console
#endif

    auto console = CONSOLE(L, windowName);
    if (console == host.mpConsole) {
        // apply changes to main console and its while-scrolling component too.
        auto result = host.setDisplayFont(QFont(font, host.getDisplayFont().pointSize()));
        if (!result.first) {
            return warnArgumentValue(L, __func__, result.second);
        }
        console->refreshView();
    } else {
        console->setFont(font);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getFont
int TLuaInterpreter::getFont(lua_State* L)
{
    QString windowName = qsl("main");
    QString font;
    windowName = WINDOW_NAME(L, 1);
    auto console = CONSOLE(L, windowName);
    font = console->mUpperPane->fontInfo().family();
    lua_pushstring(L, font.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setFontSize
int TLuaInterpreter::setFontSize(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }

    int size = getVerifiedInt(L, __func__, s, "size");
    if (size <= 0) {
        // just throw an error, no default needed.
        return warnArgumentValue(L, __func__, "size cannot be 0 or negative");
    }

    auto console = CONSOLE(L, windowName);
    if (console == host.mpConsole) {
        // get host profile display font and alter it, since that is how it's done in Settings.
        host.setDisplayFontSize(size);
    } else {
        console->setFontSize(size);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getFontSize
int TLuaInterpreter::getFontSize(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int rval = -1;
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    if (console == host.mpConsole) {
        rval = host.getDisplayFont().pointSize();
    } else {
        rval = console->mUpperPane->mDisplayFont.pointSize();
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
    if (lua_type(L, 1) != LUA_TSTRING) {
        lua_pushfstring(L, "openUserWindow:  bad argument #1 type (name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString name{lua_tostring(L, 1)};

    bool loadLayout = true;
    if (n > 1) {
        loadLayout = getVerifiedBool(L, __func__, 2, "loadLayout", true);
    }
    bool autoDock = true;
    if (n > 2) {
        autoDock = getVerifiedBool(L, __func__, 3, "autoDock", true);
    }
    QString area = QString();
    if (n > 3) {
        if (lua_type(L, 4) != LUA_TSTRING) {
            lua_pushfstring(L, "openUserWindow: bad argument #4 type (area as string expected, got %s!)", luaL_typename(L, 4));
            return lua_error(L);
        }
        area = lua_tostring(L, 4);
    }

    Host& host = getHostFromLua(L);
    //Don't create Userwindow if there is a Label with the same name already. It breaks the UserWindow

    if (auto [success, message] = host.openWindow(name, loadLayout, autoDock, area.toLower()); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setUserWindowTitle
int TLuaInterpreter::setUserWindowTitle(lua_State* L)
{
    QString name = getVerifiedString(L, __func__, 1, "name");
    QString title;
    if (lua_gettop(L) > 1) {
        title = getVerifiedString(L, __func__, 2, "title", true);
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.mpConsole->setUserWindowTitle(name, title); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMapWindowTitle
int TLuaInterpreter::setMapWindowTitle(lua_State* L)
{
    QString title;
    if (lua_gettop(L)) {
        title = getVerifiedString(L, __func__, 1, "title", true);
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.setMapperTitle(title); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createMiniConsole
int TLuaInterpreter::createMiniConsole(lua_State* L)
{
    QString name = "";
    int counter = 3;
    //make the windowname optional by using counter. If windowname "main" add to main console

    QString windowName = getVerifiedString(L, __func__, 1, "miniconsole name");
    if (isMain(windowName)) {
        // createMiniConsole only accepts the empty name as the main window
        windowName.clear();
    }

    if (!lua_isnumber(L, 2) && lua_gettop(L) >= 2) {
        name = getVerifiedString(L, __func__, 2, "miniconsole name");
    } else {
        name = windowName;
        windowName.clear();
        counter = 2;
    }

    int x = getVerifiedInt(L, __func__, counter, "miniconsole x-coordinate");
    counter++;
    int y = getVerifiedInt(L, __func__, counter, "miniconsole y-coordinate");
    counter++;
    int width = getVerifiedInt(L, __func__, counter, "miniconsole width");
    counter++;
    int height = getVerifiedInt(L, __func__, counter, "miniconsole height");

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.createMiniConsole(windowName, name, x, y, width, height); !success) {
        return warnArgumentValue(L, __func__, message, true);
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
    }
    if ((lua_type(L, 1) == LUA_TSTRING) && (lua_type(L, 2) == LUA_TSTRING)) {
        windowName = lua_tostring(L, 1);
        labelName = lua_tostring(L, 2);
        createLabelUserWindow(L, windowName, labelName);
    } else if ((lua_type(L, 1) == LUA_TSTRING) && (lua_type(L, 2) == LUA_TNUMBER)) {
        labelName = lua_tostring(L, 1);
        createLabelMainWindow(L, labelName);
    } else {
        lua_pushfstring(L, "createLabel: bad argument #2 type (label name as string or label x-coordinate as number expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createScrollBox
int TLuaInterpreter::createScrollBox(lua_State* L)
{
    QString name = "";
    int counter = 3;
    // make the windowname optional by using counter. If windowname "main" - add to main console

    QString windowName = getVerifiedString(L, __func__, 1, "scrollBox name");
    if (isMain(windowName)) {
        // createScrollBox only accepts the empty name as the main window
        windowName.clear();
    }

    if (!lua_isnumber(L, 2) && lua_gettop(L) >= 2) {
        name = getVerifiedString(L, __func__, 2, "scrollBox name");
    } else {
        name = windowName;
        windowName.clear();
        counter = 2;
    }

    int x = getVerifiedInt(L, __func__, counter, "scrollBox x-coordinate");
    counter++;
    int y = getVerifiedInt(L, __func__, counter, "scrollBox y-coordinate");
    counter++;
    int width = getVerifiedInt(L, __func__, counter, "scrollBox width");
    counter++;
    int height = getVerifiedInt(L, __func__, counter, "scrollBox height");

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.createScrollBox(windowName, name, x, y, width, height); !success) {
        return warnArgumentValue(L, __func__, message, true);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Internal Function createLabel in an UserWindow
int TLuaInterpreter::createLabelUserWindow(lua_State* L, const QString& windowName, const QString& labelName)
{
    int n = lua_gettop(L);
    int x = getVerifiedInt(L, "createLabel", 3, "label x-coordinate");
    int y = getVerifiedInt(L, "createLabel", 4, "label y-coordinate");
    int width = getVerifiedInt(L, "createLabel", 5, "label width");
    int height = getVerifiedInt(L, "createLabel", 6, "label height");

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
    QString windowName = QLatin1String("main");
    int n = lua_gettop(L);
    int x = getVerifiedInt(L, "createLabel", 2, "label x-coordinate");
    int y = getVerifiedInt(L, "createLabel", 3, "label y-coordinate");
    int width = getVerifiedInt(L, "createLabel", 4, "label width");
    int height = getVerifiedInt(L, "createLabel", 5, "label height");

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

int TLuaInterpreter::deleteLabel(lua_State* L)
{
    QString labelName = getVerifiedString(L, __func__, 1, "label name");
    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.mpConsole->deleteLabel(labelName); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::setLabelToolTip(lua_State* L)
{
    QString labelName = getVerifiedString(L, __func__, 1, "label name");
    QString labelToolTip = getVerifiedString(L, __func__, 2, "text");
    double duration = 0;
    if (lua_gettop(L) > 2) {
        duration = getVerifiedDouble(L, __func__, 3, "duration");
    }

    Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setLabelToolTip(labelName, labelToolTip, duration); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::setLabelCursor(lua_State* L)
{
    QString labelName = getVerifiedString(L, __func__, 1, "label name");
    int labelCursor = getVerifiedInt(L, __func__, 2, "cursortype");
    Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setLabelCursor(labelName, labelCursor); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::setLabelCustomCursor(lua_State* L)
{
    int n = lua_gettop(L);
    int hotX = -1, hotY = -1;
    QString labelName = getVerifiedString(L, __func__, 1, "label name");
    QString pixmapLocation = getVerifiedString(L, __func__, 2, "custom cursor location");

    if (n > 2) {
        hotX = getVerifiedInt(L, __func__, 3, "hot spot x-coordinate");
        hotY = getVerifiedInt(L, __func__, 4, "hot spot y-coordinate");
    }

    Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setLabelCustomCursor(labelName, pixmapLocation, hotX, hotY); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createMapper
int TLuaInterpreter::createMapper(lua_State* L)
{
    int n = lua_gettop(L);
    QString windowName = "";
    int counter = 1;

    if (n > 4) {
        if (lua_type(L, 1) != LUA_TSTRING) {
            lua_pushfstring(L, "createMapper: bad argument #1 type (parent window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        }
        windowName = lua_tostring(L, 1);
        counter++;
        if (isMain(windowName)) {
            // createMapper only accepts the empty name as the main window
            windowName.clear();
        }
    }

    int x = getVerifiedInt(L, __func__, counter, "mapper x-coordinate");
    counter++;
    int y = getVerifiedInt(L, __func__, counter, "mapper y-coordinate");
    counter++;
    int width = getVerifiedInt(L, __func__, counter, "mapper width");
    counter++;
    int height = getVerifiedInt(L, __func__, counter, "mapper height");

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.mpConsole->createMapper(windowName, x, y, width, height); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createCommandLine
int TLuaInterpreter::createCommandLine(lua_State* L)
{
    QString windowName = QLatin1String("main");
    int n = lua_gettop(L);
    int counter = 1;

    if (n > 5) {
        if (lua_type(L, 1) != LUA_TSTRING) {
            lua_pushfstring(L, "createCommandLine: bad argument #1 type (parent window name as string expected, got %s!)", luaL_typename(L, 1));
            return lua_error(L);
        }
        windowName = lua_tostring(L, 1);
        counter++;
        if (isMain(windowName)) {
            // createCommandLine only accepts the empty name as the main window
            windowName.clear();
        }
    }

    if (lua_type(L, counter) != LUA_TSTRING) {
        lua_pushfstring(L, "createCommandLine: bad argument #%d type (commandLine name as string expected, got %s!)", counter, luaL_typename(L, counter));
        return lua_error(L);
    }
    QString commandLineName{lua_tostring(L, counter)};
    counter++;
    int x = getVerifiedInt(L, __func__, counter, "commandline x-coordinate");
    counter++;
    int y = getVerifiedInt(L, __func__, counter, "commandline y-coordinate");
    counter++;
    int width = getVerifiedInt(L, __func__, counter, "commandline width");
    counter++;
    int height = getVerifiedInt(L, __func__, counter, "commandline height");
    counter++;

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.mpConsole->createCommandLine(windowName, commandLineName, x, y, width, height); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createBuffer
int TLuaInterpreter::createBuffer(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    host.createBuffer(text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearUserWindow
int TLuaInterpreter::clearUserWindow(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        Host& host = getHostFromLua(L);
        host.mpConsole->mUpperPane->resetHScrollbar();
        host.mpConsole->buffer.clear();
        host.mpConsole->mUpperPane->forceUpdate();
        return 0;
    }
    QString text = lua_tostring(L, 1);

    Host& host = getHostFromLua(L);
    host.clearWindow(text);

    return 0;
}

// Documentation: ? - public function but should stay undocumented -- compare https://github.com/Mudlet/Mudlet/issues/1149
int TLuaInterpreter::closeUserWindow(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    host.closeWindow(text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hideWindow
int TLuaInterpreter::hideWindow(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "name");

    Host& host = getHostFromLua(L);
    host.hideWindow(text);

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
            sizeTop = getVerifiedInt(L, __func__, 1, "new size");
            sizeRight = sizeTop;
            sizeBottom = sizeTop;
            sizeLeft = sizeTop;
            break;
        }
        case 2: {
            sizeTop = getVerifiedInt(L, __func__, 1, "new height");
            sizeRight = getVerifiedInt(L, __func__, 2, "new width");
            sizeBottom = sizeTop;
            sizeLeft = sizeRight;
            break;
        }
        case 3: {
            sizeTop = getVerifiedInt(L, __func__, 1, "new top size");
            sizeRight = getVerifiedInt(L, __func__, 2, "new width");
            sizeBottom = getVerifiedInt(L, __func__, 3, "new bottom size");
            sizeLeft = sizeRight;
            break;
        }
        default: {
            sizeTop = getVerifiedInt(L, __func__, 1, "new top size");
            sizeRight = getVerifiedInt(L, __func__, 2, "new right size");
            sizeBottom = getVerifiedInt(L, __func__, 3, "new bottom size");
            sizeLeft = getVerifiedInt(L, __func__, 4, "new left size");
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
    int size = getVerifiedInt(L, __func__, 1, "new size");
    setBorderSize(L, size, Qt::TopSection);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderRight
int TLuaInterpreter::setBorderRight(lua_State* L)
{
    int size = getVerifiedInt(L, __func__, 1, "new size");
    setBorderSize(L, size, Qt::RightSection);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderBottom
int TLuaInterpreter::setBorderBottom(lua_State* L)
{
    int size = getVerifiedInt(L, __func__, 1, "new size");
    setBorderSize(L, size, Qt::BottomSection);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderLeft
int TLuaInterpreter::setBorderLeft(lua_State* L)
{
    int size = getVerifiedInt(L, __func__, 1, "new size");
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resizeWindow
int TLuaInterpreter::resizeWindow(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "windowName");
    double x1 = getVerifiedDouble(L, __func__, 2, "width");
    double y1 = getVerifiedDouble(L, __func__, 3, "height");
    Host& host = getHostFromLua(L);
    host.resizeWindow(text, static_cast<int>(x1), static_cast<int>(y1));
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#moveWindow
int TLuaInterpreter::moveWindow(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "name");
    double x1 = getVerifiedDouble(L, __func__, 2, "x");
    double y1 = getVerifiedDouble(L, __func__, 3, "y");
    Host& host = getHostFromLua(L);
    host.moveWindow(text, static_cast<int>(x1), static_cast<int>(y1));
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setWindow
int TLuaInterpreter::setWindow(lua_State* L)
{
    int n = lua_gettop(L);
    int x = 0, y = 0;
    bool show = true;

    QString windowname {WINDOW_NAME(L, 1)};

    if (lua_type(L, 2) != LUA_TSTRING) {
        lua_pushfstring(L, "setWindow: bad argument #2 type (element name as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    QString name{lua_tostring(L, 2)};

    if (n > 2) {
        x = getVerifiedInt(L, __func__, 3, "x-coordinate");
        y = getVerifiedInt(L, __func__, 4, "y-coordinate");
        show = getVerifiedBool(L, __func__, 5, "show element");
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.setWindow(windowname, name, x, y, show); !success) {
        return warnArgumentValue(L, __func__, message);
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
        }
        area = lua_tostring(L, 1);
    }

    if (n > 1) {
        area = qsl("f");
        x = getVerifiedInt(L, __func__, 1, "x-coordinate");
        y = getVerifiedInt(L, __func__, 2, "y-coordinate");
    }
    if (n > 2) {
        width = getVerifiedInt(L, __func__, 3, "width");
        height = getVerifiedInt(L, __func__, 4, "height");
    }

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.openMapWidget(area.toLower(), x, y, width, height); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::closeMapWidget(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.closeMapWidget(); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMainWindowSize
int TLuaInterpreter::setMainWindowSize(lua_State* L)
{
    int x1 = getVerifiedInt(L, __func__, 1, "mainWidth");
    int y1 = getVerifiedInt(L, __func__, 2, "mainHeight");
    mudlet::self()->resize(x1, y1);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBackgroundColor
int TLuaInterpreter::setBackgroundColor(lua_State* L)
{
    Host& host = getHostFromLua(L);
    QString windowName;
    int r, alpha;
    int s = 1;

    auto validRange = [](int number) {
        return number >= 0 && number <= 255;
    };

    if (lua_type(L, s) == LUA_TSTRING) {
        windowName = WINDOW_NAME(L, s++);
        r = getVerifiedInt(L, __func__, s, "red value 0-255");
        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, qsl("red value %1 needs to be between 0-255").arg(r));
        }
    } else if (lua_isnumber(L, s)) {
        r = static_cast<int>(lua_tonumber(L, s));
        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, qsl("red value %1 needs to be between 0-255").arg(r));
        }
    } else {
        lua_pushfstring(L, "setBackgroundColor: bad argument #%d type (window name as string, or red value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    int g = getVerifiedInt(L, __func__, ++s, "green value 0-255");
    if (!validRange(g)) {
        return warnArgumentValue(L, __func__, qsl("green value %1 needs to be between 0-255").arg(g));
    }

    int b = getVerifiedInt(L, __func__, ++s, "blue value 0-255");
    if (!validRange(b)) {
        return warnArgumentValue(L, __func__, qsl("blue value %1 needs to be between 0-255").arg(b));
    }

    // if we get nothing for the alpha value, assume it is 255. If we get a non-number value, complain.
    alpha = 255;
    if (lua_gettop(L) > s) {
        alpha = getVerifiedInt(L, __func__, ++s, "alpha value 0-255", true);
        if (!validRange(alpha)) {
            return warnArgumentValue(L, __func__, qsl("alpha value %1 needs to be between 0-255").arg(alpha));
        }
    }

    if (isMain(windowName)) {
        host.mBgColor.setRgb(r, g, b, alpha);
        host.mpConsole->setConsoleBgColor(r, g, b, alpha);
    } else if (!host.setBackgroundColor(windowName, r, g, b, alpha)) {
        return warnArgumentValue(L, __func__, qsl("window/label '%1' not found").arg(windowName));
    }
    lua_pushboolean(L, true);
    return 1;
}


// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getBackgroundColor
int TLuaInterpreter::getBackgroundColor(lua_State* L)
{
    Host& host = getHostFromLua(L);
    QColor color;

    QString windowName = qsl("main");
    int n = lua_gettop(L);
    if (n > 0) {
        windowName = getVerifiedString(L, __func__, 1, "window name");
    }

    if (isMain(windowName)) {
        color = host.mpConsole->getConsoleBgColor();
    } else if (auto optionalColor = host.getBackgroundColor(windowName)) {
        color = optionalColor.value();
    } else {
        return warnArgumentValue(L, __func__, qsl("window '%1' does not exist").arg(windowName));
    }

    lua_pushnumber(L, color.red());
    lua_pushnumber(L, color.green());
    lua_pushnumber(L, color.blue());
    lua_pushnumber(L, color.alpha());
    return 4;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#calcFontSize
int TLuaInterpreter::calcFontSize(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString windowName = qsl("main");
    QSize size;

    // font name and size are passed in as arguments
    if (lua_gettop(L) == 2) {
        auto font = QFont(getVerifiedString(L, __func__, 2, "font name"),
                          getVerifiedInt(L, __func__, 1, "font size"), QFont::Normal);
        auto fontMetrics = QFontMetrics(font);
        size = QSize(fontMetrics.averageCharWidth(), fontMetrics.height());

        lua_pushnumber(L, size.width());
        lua_pushnumber(L, size.height());
        return 2;
    }

    // otherwise either window name or font size is passed in
    if (lua_gettop(L) == 1 && lua_isnumber(L, 1)) {
        auto fontSize = lua_tonumber(L, 1);
        auto font = QFont(qsl("Bitstream Vera Sans Mono"), fontSize, QFont::Normal);

        auto fontMetrics = QFontMetrics(font);
        size = QSize(fontMetrics.averageCharWidth(), fontMetrics.height());
    } else {
        windowName = WINDOW_NAME(L, 1);
        size = host.calcFontSize(windowName);
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
    bool logOn = getVerifiedBool(L, __func__, 1, "turn logging on/off");

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
    QString windowName = qsl("main");
    QString imgPath;
    int mode = 1;
    int counter = 1;
    int n = lua_gettop(L);
    if (n > 1 && lua_type(L, 2) == LUA_TSTRING) {
        windowName = getVerifiedString(L, __func__, 1, "console or label name");
        counter++;
    }

    imgPath = getVerifiedString(L, __func__, counter, "image path");
    counter++;

    if (n > 2 || (counter == 2 && n > 1)) {
        mode = getVerifiedInt(L, __func__, counter, "mode");
    }

    if (mode < 1 || mode > 4) {
        return warnArgumentValue(L, __func__, qsl(
            "%1 is not a valid mode! Valid modes are 1 'border', 2 'center', 3 'tile', 4 'style'").arg(mode));
    }

    Host* host = &getHostFromLua(L);
    if (!host->setBackgroundImage(windowName, imgPath, mode)) {
        return warnArgumentValue(L, __func__, qsl("console or label '%1' not found").arg(windowName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetBackgroundImage
int TLuaInterpreter::resetBackgroundImage(lua_State* L)
{
    QString windowName = qsl("main");
    int n = lua_gettop(L);
    if (n > 0) {
        windowName = getVerifiedString(L, __func__, 1, "console name");
    }

    Host* host = &getHostFromLua(L);
    if (!host->resetBackgroundImage(windowName)) {
        return warnArgumentValue(L, __func__, qsl("console '%1' not found").arg(windowName));
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getImageSize
int TLuaInterpreter::getImageSize(lua_State* L)
{
    QString imageLocation = getVerifiedString(L, __func__, 1, "image location");
    if (imageLocation.isEmpty()) {
        return warnArgumentValue(L, __func__, "image location cannot be an empty string");
    }

    auto size = mudlet::self()->getImageSize(imageLocation);
    if (!size) {
        return warnArgumentValue(L, __func__, qsl("couldn't retrieve image size, is the location '%1' correct?").arg(imageLocation));
    }
    lua_pushnumber(L, size->width());
    lua_pushnumber(L, size->height());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLabelSizeHint
int TLuaInterpreter::getLabelSizeHint(lua_State* L)
{
    QString labelName = getVerifiedString(L, __func__, 1, "label name");
    Host& host = getHostFromLua(L);
    if (labelName.isEmpty()) {
        return warnArgumentValue(L, __func__, "label name cannot be an empty string");
    }

    auto size = host.mpConsole->getLabelSizeHint(labelName);
    if (!size) {
        return warnArgumentValue(L, __func__, qsl("label '%1' does not exist").arg(labelName));
    }
    lua_pushnumber(L, size->width());
    lua_pushnumber(L, size->height());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCmdLineAction
int TLuaInterpreter::setCmdLineAction(lua_State* L)
{
    Host& host = getHostFromLua(L);
    QString name = getVerifiedString(L, __func__, 1, "command line name");
    if (name.isEmpty()) {
        return warnArgumentValue(L, __func__, "command line name cannot be an empty string");
    }
    lua_remove(L, 1);

    if (!lua_isfunction(L, 1)) {
        lua_pushfstring(L, "setCmdLineAction: bad argument #2 type (function expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    int func = luaL_ref(L, LUA_REGISTRYINDEX);

    if (!host.setCmdLineAction(name, func)) {
        return warnArgumentValue(L, __func__, qsl("command line name '%1' not found").arg(name));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetCmdLineAction
int TLuaInterpreter::resetCmdLineAction(lua_State* L){
    Host& host = getHostFromLua(L);
    QString name = getVerifiedString(L, __func__, 1, "command line name");
    if (name.isEmpty()) {
        return warnArgumentValue(L, __func__, "command line name cannot be an empty string");
    }

    bool lua_result = false;
    lua_result = host.resetCmdLineAction(name);
    if (lua_result) {
        lua_pushboolean(L, true);
        return 1;
    } else {
        return warnArgumentValue(L, __func__, qsl("command line name '%1' not found").arg(name));
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCmdLineStyleSheet
int TLuaInterpreter::setCmdLineStyleSheet(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = "main";
    if (n > 1) {
        name = getVerifiedString(L, __func__, 1, "command line name", true);
    }
    QString styleSheet = getVerifiedString(L, __func__, n, "StyleSheet");
    Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setCmdLineStyleSheet(name, styleSheet); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::setLabelCallback(lua_State* L, const QString& funcName)
{
    Host& host = getHostFromLua(L);
    QString labelName = getVerifiedString(L, funcName.toUtf8().constData(), 1, "label name");
    if (labelName.isEmpty()) {
        return warnArgumentValue(L, __func__, "label name cannot be an empty string");
    }
    lua_remove(L, 1);

    if (!lua_isfunction(L, 1)) {
        lua_pushfstring(L, "%s: bad argument #2 type (function expected, got %s!)", funcName.toUtf8().constData(), luaL_typename(L, 1));
        return lua_error(L);
    }
    int func = luaL_ref(L, LUA_REGISTRYINDEX);

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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelClickCallback
int TLuaInterpreter::setLabelClickCallback(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelClickCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelDoubleClickCallback
int TLuaInterpreter::setLabelDoubleClickCallback(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelDoubleClickCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelReleaseCallback
int TLuaInterpreter::setLabelReleaseCallback(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelReleaseCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelMoveCallback
int TLuaInterpreter::setLabelMoveCallback(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelMoveCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelWheelCallback
int TLuaInterpreter::setLabelWheelCallback(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelWheelCallback"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelOnEnter
int TLuaInterpreter::setLabelOnEnter(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelOnEnter"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelOnLeave
int TLuaInterpreter::setLabelOnLeave(lua_State* L)
{
    return setLabelCallback(L, qsl("setLabelOnLeave"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMovie
int TLuaInterpreter::setMovie(lua_State* L)
{
    QString labelName = getVerifiedString(L, __func__, 1, "label name");
    if (labelName.isEmpty()) {
        return warnArgumentValue(L, __func__, "label name cannot be an empty string");
    }
    QString moviePath = getVerifiedString(L, __func__, 2, "movie (gif) path");

    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.setMovie(labelName, moviePath); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    lua_pushboolean(L, true);
    return 1;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::movieFunc(lua_State* L, const QString& funcName)
{
    QString labelName = getVerifiedString(L, funcName.toUtf8().constData(), 1, "label name");
    if (labelName.isEmpty()) {
        return warnArgumentValue(L, __func__, "label name cannot be an empty string");
    }
    auto pN = LABEL(L, labelName);
    auto movie = pN->movie();
    if (!movie) {
        return warnArgumentValue(L, __func__, qsl("no movie found at label '%1'").arg(labelName));
    }

    if (funcName == qsl("startMovie")) {
        movie->start();
    } else if (funcName == qsl("pauseMovie")) {
        movie->setPaused(true);
    } else if (funcName == qsl("setMovieFrame")) {
        int frame = getVerifiedInt(L, funcName.toUtf8().constData(), 2, "movie frame number");
        lua_pushboolean(L, movie->jumpToFrame(frame));
        return 1;
    } else if (funcName == qsl("setMovieSpeed")) {
        int speed = getVerifiedInt(L, funcName.toUtf8().constData(), 2, "movie playback speed in %");
        movie->setSpeed(speed);
    } else if (funcName == qsl("scaleMovie")) {
        bool autoScale{true};
        int n = lua_gettop(L);
        if (n > 1) {
            autoScale = getVerifiedBool(L, funcName.toUtf8().constData(), 2, "activate/deactivate scaling movie", true);
        }
        movie->setScaledSize(pN->size());
        if (autoScale) {
            connect(pN, &TLabel::resized, [=] {movie->setScaledSize(pN->size());} );
        } else {
            pN->disconnect(SIGNAL(resized()));
        }
    } else {
        return warnArgumentValue(L, __func__, qsl("'%1' is not a known function name - bug in Mudlet, please report it").arg(funcName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#startMovie
int TLuaInterpreter::startMovie(lua_State* L)
{
    return movieFunc(L, qsl("startMovie"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#pauseMovie
int TLuaInterpreter::pauseMovie(lua_State* L)
{
    return movieFunc(L, qsl("pauseMovie"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMovieFrame
int TLuaInterpreter::setMovieFrame(lua_State* L)
{
    return movieFunc(L, qsl("setMovieFrame"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMovieSpeed
int TLuaInterpreter::setMovieSpeed(lua_State* L)
{
    return movieFunc(L, qsl("setMovieSpeed"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMovieSpeed
int TLuaInterpreter::scaleMovie(lua_State* L)
{
    return movieFunc(L, qsl("scaleMovie"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setTextFormat
int TLuaInterpreter::setTextFormat(lua_State* L)
{
    Host& host = getHostFromLua(L);

    int n = lua_gettop(L);

    QString windowName {WINDOW_NAME(L, 1)};

    QVector<int> colorComponents(6); // 0-2 RGB background, 3-5 RGB foreground
    colorComponents[0] = qRound(qBound(0.0, getVerifiedDouble(L, __func__, 2, "red background color component"), 255.0));
    colorComponents[1] = qRound(qBound(0.0, getVerifiedDouble(L, __func__, 3, "green background color component"), 255.0));
    colorComponents[2] = qRound(qBound(0.0, getVerifiedDouble(L, __func__, 4, "blue background color component"), 255.0));
    colorComponents[3] = qRound(qBound(0.0, getVerifiedDouble(L, __func__, 5, "red foreground color component"), 255.0));
    colorComponents[4] = qRound(qBound(0.0, getVerifiedDouble(L, __func__, 6, "green foreground color component"), 255.0));
    colorComponents[5] = qRound(qBound(0.0, getVerifiedDouble(L, __func__, 7, "blue foreground color component"), 255.0));

    int s = 7;
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
        return warnArgumentValue(L, __func__, qsl("window '%1' does not exist").arg(windowName), true);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#raiseWindow
int TLuaInterpreter::raiseWindow(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpConsole->raiseWindow(windowName));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#lowerWindow
int TLuaInterpreter::lowerWindow(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpConsole->lowerWindow(windowName));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#showWindow
int TLuaInterpreter::showWindow(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "name");
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.showWindow(text));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomEnv
int TLuaInterpreter::setRoomEnv(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "room id");
    int env = getVerifiedInt(L, __func__, 2, "environment id");

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("room %1 doesn't exist").arg(id));
    }
    pR->environment = env;
    host.mpMap->mUnsavedMap = true;
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomName
int TLuaInterpreter::setRoomName(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int id = getVerifiedInt(L, __func__, 1, "room id");
    QString name = getVerifiedString(L, __func__, 2, "room name", true);

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(id));
    }
    pR->name = name;
    host.mpMap->mUnsavedMap = true;
    updateMap(L);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomName
int TLuaInterpreter::getRoomName(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int id = getVerifiedInt(L, __func__, 1, "room id");

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("room %1 doesn't exist").arg(id));
    }
    lua_pushstring(L, pR->name.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomWeight
int TLuaInterpreter::setRoomWeight(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "roomID");
    int w = getVerifiedInt(L, __func__, 2, "weight");

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->setWeight(w);
        host.mpMap->mUnsavedMap = true;
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
    bool isToSaveToProfile = false;

    Host& host = getHostFromLua(L);
    QString url = getVerifiedString(L, __func__, 1, "url");

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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomIDbyHash
int TLuaInterpreter::setRoomIDbyHash(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "room id");
    QString hash = getVerifiedString(L, __func__, 2, "hash");
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
    QString hash = getVerifiedString(L, __func__, 1, "hash");
    Host& host = getHostFromLua(L);
    int retID = host.mpMap->mpRoomDB->hashToRoomID.value(hash, -1);
    lua_pushnumber(L, retID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomHashByID
int TLuaInterpreter::getRoomHashByID(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "room id");
    Host& host = getHostFromLua(L);
    if (!host.mpMap->mpRoomDB->roomIDToHash.contains(id)) {
        return warnArgumentValue(L, __func__, qsl("no hash for room %1").arg(id));
    }
    QString retHash = host.mpMap->mpRoomDB->roomIDToHash[id];
    lua_pushstring(L, retHash.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#roomLocked
int TLuaInterpreter::roomLocked(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "roomID");
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
    int id = getVerifiedInt(L, __func__, 1, "roomID");
    bool b = getVerifiedBool(L, __func__, 2, "lockIfTrue");
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->isLocked = b;
        host.mpMap->mUnsavedMap = true;
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
    int id = getVerifiedInt(L, __func__, 1, "roomID");

    int dir = dirToNumber(L, 2);
    if (!dir) {
        lua_pushfstring(L, "lockExit: bad argument #2 type (direction as number or string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    bool b = getVerifiedBool(L, __func__, 3, "lockIfTrue");

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (pR) {
        pR->setExitLock(dir, b);
        host.mpMap->mUnsavedMap = true;
        host.mpMap->mMapGraphNeedsUpdate = true;
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#lockSpecialExit
int TLuaInterpreter::lockSpecialExit(lua_State* L)
{
    int fromRoomID = getVerifiedInt(L, __func__, 1, "exit room id");
    // The second argument (was the toRoomID) is now ignored as it is not required/considered in any way
    QString dir = getVerifiedString(L, __func__, 3, "special exit name/command");
    if (dir.isEmpty()) {
        return warnArgumentValue(L, __func__, "the special exit name/command cannot be empty");
    }
    bool b = getVerifiedBool(L, __func__, 4, "special exit lock state");

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(fromRoomID);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("exit room id %1 does not exist").arg(fromRoomID));
    }
    if (!pR->setSpecialExitLock(dir, b)) {
        return warnArgumentValue(L, __func__, qsl("the special exit name/command %1 does not exist in room id %2")
            .arg(dir, QString::number(fromRoomID)));
    }

    lua_pushboolean(L, true);
    host.mpMap->mUnsavedMap = true;
    host.mpMap->mMapGraphNeedsUpdate = true;
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hasSpecialExitLock
int TLuaInterpreter::hasSpecialExitLock(lua_State* L)
{
    int fromRoomID = getVerifiedInt(L, __func__, 1, "exit room id");
    // Second argument was the entrance room id but it is not needed any more and is ignored
    QString dir = getVerifiedString(L, __func__, 3, "special exit name/command");
    if (dir.isEmpty()) {
        return warnArgumentValue(L, __func__, "the special exit name/command cannot be empty");
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(fromRoomID);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("exit room id %1 does not exist").arg(fromRoomID));
    }
    if (!pR->getSpecialExits().contains(dir)) {
        return warnArgumentValue(L, __func__, qsl("the special exit name/command '%1' does not exist in room id %2")
            .arg(dir, QString::number(fromRoomID)));
    }

    lua_pushboolean(L, pR->hasSpecialExitLock(dir));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hasExitLock
int TLuaInterpreter::hasExitLock(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "roomID");

    int dir = dirToNumber(L, 2);
    if (!dir) {
        lua_pushfstring(L, "hasExitLock: bad argument #2 type (direction as number or string expected, got %s!)");
        return lua_error(L);
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
    int id = getVerifiedInt(L, __func__, 1, "roomID");

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
    int roomId = getVerifiedInt(L, __func__, 1, "room id");
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(roomId));
    }
    lua_newtable(L);
    QList<int> entrances = host.mpMap->mpRoomDB->getEntranceHash().values(roomId);
    // Could use a .toSet().toList() to remove duplicates values
    if (entrances.count() > 1) {
        std::sort(entrances.begin(), entrances.end());
    }
    for (int i = 0; i < entrances.size(); i++) {
        lua_pushnumber(L, i + 1);
        lua_pushnumber(L, entrances.at(i));
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#searchRoom
int TLuaInterpreter::searchRoom(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
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
                        return lua_error(L);
                    }
                }
            } else {
                lua_pushfstring(L, R"(searchRoom: bad argument #2 type ("case sensitive" as boolean is optional, got %s!))", luaL_typename(L, 2));
                return lua_error(L);
            }
        }
        room = lua_tostring(L, 1);
    } else {
        lua_pushfstring(L, R"(searchRoom: bad argument #1 ("room name" as string expected, got %s!))", luaL_typename(L, 1));
        return lua_error(L);
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
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QString key = QString();
    QString value = QString(); //both of these assigns a null value which is detectably different from the empty value

    if (lua_gettop(L)) {
        key = getVerifiedString(L, __func__, 1, "key", true);
        if (lua_gettop(L) > 1) {
            value = getVerifiedString(L, __func__, 2, "value", true);
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
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QString key = QString();
    QString value = QString(); //both of these assigns a null value which is detectably different from the empty value

    if (lua_gettop(L)) {
        key = getVerifiedString(L, __func__, 1, "key", true);
        if (lua_gettop(L) > 1) {
            value = getVerifiedString(L, __func__, 2, "value", true);
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
        QList<int> areaIds{areaIdsSet.begin(), areaIdsSet.end()};
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
        return warnArgumentValue(L, __func__, "no map present or loaded");
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
        return warnArgumentValue(L, __func__, "no map present or loaded");
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
    int area = getVerifiedInt(L, __func__, 1, "areaID");
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
        // with a zero index and we must maintain compatibility with code written
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
    int n = lua_gettop(L);
    bool isFullDataRequired = false;
    int area = getVerifiedInt(L, __func__, 1, "area id");

    if (n > 1) {
        isFullDataRequired = getVerifiedBool(L, __func__, 2, "full data wanted", true);
    }

    Host& host = getHostFromLua(L);
    TArea* pA = host.mpMap->mpRoomDB->getArea(area);
    if (!pA) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid area id").arg(area));
    }

    lua_newtable(L);
    if (n < 2 || (n > 1 && !isFullDataRequired)) {
        // Replicate original implementation
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#auditAreas
int TLuaInterpreter::auditAreas(lua_State* L)
{
    Host& host = getHostFromLua(L);
    host.mpMap->audit();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomWeight
int TLuaInterpreter::getRoomWeight(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int roomId;
    if (lua_gettop(L) > 0) {
        roomId = getVerifiedInt(L, __func__, 1, "roomID");
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
    int targetRoomId = getVerifiedInt(L, __func__, 1, "target room id");

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    } else if (!host.mpMap->mpRoomDB->getRoom(targetRoomId)) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid target room id").arg(targetRoomId));
    }

    if (!host.mpMap->gotoRoom(targetRoomId)) {
        int totalWeight = host.assemblePath(); // Needed if unsuccessful to clear lua speedwalk tables
        Q_UNUSED(totalWeight);
        return warnArgumentValue(L, __func__, qsl("no path found from current room to room with id %1").arg(targetRoomId), true);
    }
    host.startSpeedWalk();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getPath
int TLuaInterpreter::getPath(lua_State* L)
{
    int originRoomId = getVerifiedInt(L, __func__, 1, "starting room id");
    int targetRoomId = getVerifiedInt(L, __func__, 2, "target room id");

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    } else if (!host.mpMap->mpRoomDB->getRoom(originRoomId)) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid source room id").arg(originRoomId));
    } else if (!host.mpMap->mpRoomDB->getRoom(targetRoomId)) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid target room id").arg(targetRoomId));
    }

    bool ret = host.mpMap->gotoRoom(originRoomId, targetRoomId);
    int totalWeight = host.assemblePath(); // Needed even if unsuccessful, to clear lua tables then
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
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->deselect();
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resetFormat
int TLuaInterpreter::resetFormat(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->reset();
    lua_pushboolean(L, true);
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
    QString windowName {WINDOW_NAME(L, 1)};
    QString text = getVerifiedString(L, __func__, 2, "text");
    Host& host = getHostFromLua(L);
    host.echoWindow(windowName, text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setAppStyleSheet
int TLuaInterpreter::setAppStyleSheet(lua_State* L)
{
    QString styleSheet;
    QString tag;
    int n = lua_gettop(L);
    styleSheet = getVerifiedString(L, __func__, 1, "style sheet");
    if (n > 1) {
        tag = getVerifiedString(L, __func__, 2, "tag");
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
    QString styleSheet = getVerifiedString(L, __func__, 1, "style sheet");
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.setProfileStyleSheet(styleSheet));
    return 1;
}

// No documentation available in wiki - internal function
// this was an internal only function used by the package system, but it was
// inactive and has been removed
int TLuaInterpreter::showUnzipProgress(lua_State* L)
{
    return warnArgumentValue(L, __func__, "removed command, this function is now inactive and does nothing");
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#receiveMSP
int TLuaInterpreter::receiveMSP(lua_State* L)
{
    Host& host = getHostFromLua(L);
    std::string msg;

    if (!host.mTelnet.isMSPEnabled()) {
        return warnArgumentValue(L, __func__, "MSP is not currently enabled");
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

// Private
int TLuaInterpreter::loadMediaFileAsOrderedArguments(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TMediaData mediaData{};
    int numArgs = lua_gettop(L);
    QString stringValue;

    // name[,url])
    for (int i = 1; i <= numArgs; i++) {
        if (lua_isnil(L, i)) {
            continue;
        }

        switch (i) {
        case 1:
            stringValue = getVerifiedString(L, __func__, i, "name");

            if (QDir::homePath().contains('\\')) {
                stringValue.replace('/', R"(\)");
            } else {
                stringValue.replace('\\', "/");
            }

            mediaData.setMediaFileName(stringValue);
            break;
        case 2:
            stringValue = getVerifiedString(L, __func__, i, "url");
            mediaData.setMediaUrl(stringValue);
            break;
        }
    }

    if (mediaData.getMediaFileName().isEmpty()) {
        return warnArgumentValue(L, __func__, QLatin1String("missing argument 1 (file to play)"));
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaVolume(TMediaData::MediaVolumePreload);

    host.mpMedia->playMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::loadMediaFileAsTableArgument(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        QString key = getVerifiedString(L, __func__, -2, "table keys");
        key = key.toLower();

        if (key == QLatin1String("name") || key == QLatin1String("url")) {
            QString value = getVerifiedString(L, __func__, -1, key == QLatin1String("name") ? "value for name" : "value for url");

            if (key == QLatin1String("name") && !value.isEmpty()) {
                if (QDir::homePath().contains('\\')) {
                    value.replace('/', R"(\)");
                } else {
                    value.replace('\\', "/");
                }

                mediaData.setMediaFileName(value);
            } else if (key == QLatin1String("url") && !value.isEmpty()) {
                mediaData.setMediaUrl(value);
            }
        }

        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    if (mediaData.getMediaFileName().isEmpty()) {
        lua_pushstring(L, R"(loadMusicFile: missing name (add name = "file to play"))");
        return lua_error(L);
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaVolume(TMediaData::MediaVolumePreload);

    host.mpMedia->playMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadMusicFile
int TLuaInterpreter::loadMusicFile(lua_State* L)
{
    if (!lua_gettop(L)) {
        lua_pushfstring(L, "%s: need at least one argument", __func__);
        return lua_error(L);
    }

    if (lua_istable(L, 1)) {
        return loadMediaFileAsTableArgument(L);
    }

    return loadMediaFileAsOrderedArguments(L);
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadSoundFile
int TLuaInterpreter::loadSoundFile(lua_State* L)
{
    if (!lua_gettop(L)) {
        lua_pushfstring(L, "%s: need at least one argument", __func__);
        return lua_error(L);
    }

    if (lua_istable(L, 1)) {
        return loadMediaFileAsTableArgument(L);
    }

    return loadMediaFileAsOrderedArguments(L);
}

// Private
int TLuaInterpreter::playMusicFileAsOrderedArguments(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TMediaData mediaData{};
    int numArgs = lua_gettop(L);
    QString stringValue;
    int intValue = 0;
    bool boolValue = 0;

    // name[,volume][,fadein][,fadeout][,start][,loops][,key][,tag][,continue][,url])
    for (int i = 1; i <= numArgs; i++) {
        if (lua_isnil(L, i)) {
            continue;
        }

        switch (i) {
        case 1:
            stringValue = getVerifiedString(L, __func__, i, "name");

            if (QDir::homePath().contains('\\')) {
                stringValue.replace('/', R"(\)");
            } else {
                stringValue.replace('\\', "/");
            }

            mediaData.setMediaFileName(stringValue);
            break;
        case 2:
            intValue = getVerifiedInt(L, __func__, i, "volume");

            if (intValue == TMediaData::MediaVolumePreload) {
                {
                } // Volume of 0 supports preloading
            } else if (intValue > TMediaData::MediaVolumeMax) {
                intValue = TMediaData::MediaVolumeMax;
            } else if (intValue < TMediaData::MediaVolumeMin) {
                intValue = TMediaData::MediaVolumeMin;
            }

            mediaData.setMediaVolume(intValue);
            break;
        case 3:
            intValue = getVerifiedInt(L, __func__, i, "fadein");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadein", intValue);
                return lua_error(L);
            }

            mediaData.setMediaFadeIn(intValue);
            break;
        case 4:
            intValue = getVerifiedInt(L, __func__, i, "fadeout");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", intValue);
                return lua_error(L);
            }

            mediaData.setMediaFadeOut(intValue);
            break;
        case 5:
            intValue = getVerifiedInt(L, __func__, i, "start");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "start", intValue);
                return lua_error(L);
            }

            mediaData.setMediaStart(intValue);
            break;
        case 6:
            intValue = getVerifiedInt(L, __func__, i, "loops");

            if (intValue < TMediaData::MediaLoopsRepeat || intValue == 0) {
                intValue = TMediaData::MediaLoopsDefault;
            }

            mediaData.setMediaLoops(intValue);
            break;
        case 7:
            stringValue = getVerifiedString(L, __func__, i, "key");
            mediaData.setMediaKey(stringValue);
            break;
        case 8:
            stringValue = getVerifiedString(L, __func__, i, "tag");
            mediaData.setMediaTag(stringValue);
            break;
        case 9:
            boolValue = getVerifiedBool(L, __func__, i, "continue");
            mediaData.setMediaContinue(boolValue);
            break;
        case 10:
            stringValue = getVerifiedString(L, __func__, i, "url");
            mediaData.setMediaUrl(stringValue);
            break;
        }
    }

    if (mediaData.getMediaFileName().isEmpty()) {
        return warnArgumentValue(L, __func__, QLatin1String("missing argument 1 (file to play)"));
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);
    host.mpMedia->playMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::playMusicFileAsTableArgument(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        QString key = getVerifiedString(L, __func__, -2, "table keys");
        key = key.toLower();

        if (key == QLatin1String("name") || key == QLatin1String("url") || key == QLatin1String("key") || key == QLatin1String("tag")) {
            QString value = getVerifiedString(L,
                                              __func__,
                                              -1,
                                              key == QLatin1String("name")  ? "value for name"
                                              : key == QLatin1String("key") ? "value for key"
                                              : key == QLatin1String("tag") ? "value for tag"
                                                                            : "value for url");

            if (key == QLatin1String("name") && !value.isEmpty()) {
                if (QDir::homePath().contains('\\')) {
                    value.replace('/', R"(\)");
                } else {
                    value.replace('\\', "/");
                }

                mediaData.setMediaFileName(value);
            } else if (key == QLatin1String("url") && !value.isEmpty()) {
                mediaData.setMediaUrl(value);
            } else if (key == QLatin1String("key") && !value.isEmpty()) {
                mediaData.setMediaKey(value);
            } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                mediaData.setMediaTag(value);
            }
        } else if (key == QLatin1String("volume") || key == QLatin1String("fadein") || key == QLatin1String("fadeout") || key == QLatin1String("start") || key == QLatin1String("loops")) {
            int value = getVerifiedInt(L,
                                       __func__,
                                       -1,
                                       key == QLatin1String("volume")    ? "value for volume"
                                       : key == QLatin1String("fadein")  ? "value for fadein"
                                       : key == QLatin1String("fadeout") ? "value for fadeout"
                                       : key == QLatin1String("start")   ? "value for start"
                                                                         : "value for loops");

            if (key == QLatin1String("volume")) {
                if (value == TMediaData::MediaVolumePreload) {
                    {
                    } // Volume of 0 supports preloading
                } else if (value > TMediaData::MediaVolumeMax) {
                    value = TMediaData::MediaVolumeMax;
                } else if (value < TMediaData::MediaVolumeMin) {
                    value = TMediaData::MediaVolumeMin;
                }

                mediaData.setMediaVolume(value);
            } else if (key == QLatin1String("fadein")) {
                if (value < 0) {
                    lua_pushfstring(L, "playMusicFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadein", value);
                    return lua_error(L);
                }

                mediaData.setMediaFadeIn(value);
            } else if (key == QLatin1String("fadeout")) {
                if (value < 0) {
                    lua_pushfstring(L, "playMusicFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", value);
                    return lua_error(L);
                }

                mediaData.setMediaFadeOut(value);
            } else if (key == QLatin1String("start")) {
                if (value < 0) {
                    lua_pushfstring(L, "playMusicFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "start", value);
                    return lua_error(L);
                }

                mediaData.setMediaStart(value);
            } else if (key == QLatin1String("loops")) {
                if (value < TMediaData::MediaLoopsRepeat || value == 0) {
                    value = TMediaData::MediaLoopsDefault;
                }

                mediaData.setMediaLoops(value);
            } else if (key == QLatin1String("continue")) {
                bool value = getVerifiedBool(L, __func__, -1, "value for continue must be boolean");
                mediaData.setMediaContinue(value);
            }
        }

        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    if (mediaData.getMediaFileName().isEmpty()) {
        lua_pushstring(L, R"(playMusicFile: missing name (add name = "file to play"))");
        return lua_error(L);
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);
    host.mpMedia->playMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#playMusicFile
int TLuaInterpreter::playMusicFile(lua_State* L)
{
    if (!lua_gettop(L)) {
        lua_pushfstring(L, "%s: need at least one argument", __func__);
        return lua_error(L);
    }

    if (lua_istable(L, 1)) {
        return playMusicFileAsTableArgument(L);
    }

    return playMusicFileAsOrderedArguments(L);
}

// Private
int TLuaInterpreter::playSoundFileAsOrderedArguments(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TMediaData mediaData{};
    int numArgs = lua_gettop(L);
    QString stringValue;
    int intValue = 0;

    // name[,volume][,fadein][,fadeout][,start][,loops][,key][,tag][,priority][,url])
    for (int i = 1; i <= numArgs; i++) {
        if (lua_isnil(L, i)) {
            continue;
        }

        switch (i) {
        case 1:
            stringValue = getVerifiedString(L, __func__, i, "name");

            if (QDir::homePath().contains('\\')) {
                stringValue.replace('/', R"(\)");
            } else {
                stringValue.replace('\\', "/");
            }

            mediaData.setMediaFileName(stringValue);
            break;
        case 2:
            intValue = getVerifiedInt(L, __func__, i, "volume");

            if (intValue == TMediaData::MediaVolumePreload) {
                {
                } // Volume of 0 supports preloading
            } else if (intValue > TMediaData::MediaVolumeMax) {
                intValue = TMediaData::MediaVolumeMax;
            } else if (intValue < TMediaData::MediaVolumeMin) {
                intValue = TMediaData::MediaVolumeMin;
            }

            mediaData.setMediaVolume(intValue);
            break;
        case 3:
            intValue = getVerifiedInt(L, __func__, i, "fadein");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %s)", "fadein", intValue);
                return lua_error(L);
            }

            mediaData.setMediaFadeIn(intValue);
            break;
        case 4:
            intValue = getVerifiedInt(L, __func__, i, "fadeout");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %s)", "fadeout", intValue);
                return lua_error(L);
            }

            mediaData.setMediaFadeOut(intValue);
            break;
        case 5:
            intValue = getVerifiedInt(L, __func__, i, "start");

            if (intValue < 0) {
                lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %s)", "start", intValue);
                return lua_error(L);
            }

            mediaData.setMediaStart(intValue);
            break;
        case 6:
            intValue = getVerifiedInt(L, __func__, i, "loops");

            if (intValue < TMediaData::MediaLoopsRepeat || intValue == 0) {
                intValue = TMediaData::MediaLoopsDefault;
            }

            mediaData.setMediaLoops(intValue);
            break;
        case 7:
            stringValue = getVerifiedString(L, __func__, i, "key");
            mediaData.setMediaKey(stringValue);
            break;
        case 8:
            stringValue = getVerifiedString(L, __func__, i, "tag");
            mediaData.setMediaTag(stringValue);
            break;
        case 9:
            intValue = getVerifiedInt(L, __func__, i, "priority");

            if (intValue > TMediaData::MediaPriorityMax) {
                intValue = TMediaData::MediaPriorityMax;
            } else if (intValue < TMediaData::MediaPriorityMin) {
                intValue = TMediaData::MediaPriorityMin;
            }

            mediaData.setMediaPriority(intValue);
            break;
        case 10:
            stringValue = getVerifiedString(L, __func__, i, "url");
            mediaData.setMediaUrl(stringValue);
            break;
        }
    }

    if (mediaData.getMediaFileName().isEmpty()) {
        return warnArgumentValue(L, __func__, QLatin1String("missing argument 1 (file to play)"));
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    host.mpMedia->playMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::playSoundFileAsTableArgument(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        QString key = getVerifiedString(L, __func__, -2, "table keys");
        key = key.toLower();

        if (key == QLatin1String("name") || key == QLatin1String("url") || key == QLatin1String("key") || key == QLatin1String("tag")) {
            QString value = getVerifiedString(L,
                                              __func__,
                                              -1,
                                              key == QLatin1String("name")  ? "value for name"
                                              : key == QLatin1String("key") ? "value for key"
                                              : key == QLatin1String("tag") ? "value for tag"
                                                                            : "value for url");

            if (key == QLatin1String("name") && !value.isEmpty()) {
                if (QDir::homePath().contains('\\')) {
                    value.replace('/', R"(\)");
                } else {
                    value.replace('\\', "/");
                }

                mediaData.setMediaFileName(value);
            } else if (key == QLatin1String("url") && !value.isEmpty()) {
                mediaData.setMediaUrl(value);
            } else if (key == QLatin1String("key") && !value.isEmpty()) {
                mediaData.setMediaKey(value);
            } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                mediaData.setMediaTag(value);
            }
        } else if (key == QLatin1String("volume") || key == QLatin1String("fadein") || key == QLatin1String("fadeout") || key == QLatin1String("start") || key == QLatin1String("loops")
                   || key == QLatin1String("priority")) {
            int value = getVerifiedInt(L,
                                       __func__,
                                       -1,
                                       key == QLatin1String("volume")    ? "value for volume"
                                       : key == QLatin1String("fadein")  ? "value for fadein"
                                       : key == QLatin1String("fadeout") ? "value for fadeout"
                                       : key == QLatin1String("start")   ? "value for start"
                                       : key == QLatin1String("loops")   ? "value for loops"
                                                                         : "value for priority");

            if (key == QLatin1String("volume")) {
                if (value == TMediaData::MediaVolumePreload) {
                    {
                    } // Volume of 0 supports preloading
                } else if (value > TMediaData::MediaVolumeMax) {
                    value = TMediaData::MediaVolumeMax;
                } else if (value < TMediaData::MediaVolumeMin) {
                    value = TMediaData::MediaVolumeMin;
                }

                mediaData.setMediaVolume(value);
            } else if (key == QLatin1String("fadein")) {
                if (value < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadein", value);
                    return lua_error(L);
                }

                mediaData.setMediaFadeIn(value);
            } else if (key == QLatin1String("fadeout")) {
                if (value < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "fadeout", value);
                    return lua_error(L);
                }

                mediaData.setMediaFadeOut(value);
            } else if (key == QLatin1String("start")) {
                if (value < 0) {
                    lua_pushfstring(L, "playSoundFile: bad argument range for %s (values must be greater than or equal to 0, got value: %d)", "start", value);
                    return lua_error(L);
                }

                mediaData.setMediaStart(value);
            } else if (key == QLatin1String("loops")) {
                if (value < TMediaData::MediaLoopsRepeat || value == 0) {
                    value = TMediaData::MediaLoopsDefault;
                }

                mediaData.setMediaLoops(value);
            } else if (key == QLatin1String("priority")) {
                if (value > TMediaData::MediaPriorityMax) {
                    value = TMediaData::MediaPriorityMax;
                } else if (value < TMediaData::MediaPriorityMin) {
                    value = TMediaData::MediaPriorityMin;
                }

                mediaData.setMediaPriority(value);
            }
        }

        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    if (mediaData.getMediaFileName().isEmpty()) {
        lua_pushstring(L, R"(playSoundFile: missing name (add name = "file to play"))");
        return lua_error(L);
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    host.mpMedia->playMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#playSoundFile
int TLuaInterpreter::playSoundFile(lua_State* L)
{
    if (!lua_gettop(L)) {
        lua_pushfstring(L, "%s: need at least one argument", __func__);
        return lua_error(L);
    }

    if (lua_istable(L, 1)) {
        return playSoundFileAsTableArgument(L);
    }

    return playSoundFileAsOrderedArguments(L);
}

// Private
int TLuaInterpreter::stopMusicAsOrderedArguments(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TMediaData mediaData{};
    int numArgs = lua_gettop(L);
    QString stringValue;

    // values as ordered args: name[,key][,tag])
    for (int i = 1; i <= numArgs; i++) {
        if (lua_isnil(L, i)) {
            continue;
        }

        switch (i) {
        case 1:
            stringValue = getVerifiedString(L, __func__, i, "name");

            if (QDir::homePath().contains('\\')) {
                stringValue.replace('/', R"(\)");
            } else {
                stringValue.replace('\\', "/");
            }

            mediaData.setMediaFileName(stringValue);
            break;
        case 2:
            stringValue = getVerifiedString(L, __func__, i, "key");
            mediaData.setMediaKey(stringValue);
            break;
        case 3:
            stringValue = getVerifiedString(L, __func__, i, "tag");
            mediaData.setMediaTag(stringValue);
            break;
        }
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::stopMusicAsTableArgument(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        QString key = getVerifiedString(L, __func__, -2, "table keys");
        key = key.toLower();

        if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
            QString value = getVerifiedString(L, __func__, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag");

            if (key == QLatin1String("name") && !value.isEmpty()) {
                if (QDir::homePath().contains('\\')) {
                    value.replace('/', R"(\)");
                } else {
                    value.replace('\\', "/");
                }

                mediaData.setMediaFileName(value);
            } else if (key == QLatin1String("key") && !value.isEmpty()) {
                mediaData.setMediaKey(value);
            } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                mediaData.setMediaTag(value);
            }
        }

        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopMusic
int TLuaInterpreter::stopMusic(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    if (lua_gettop(L)) {
        if (lua_istable(L, 1)) {
            return stopMusicAsTableArgument(L);
        }

        return stopMusicAsOrderedArguments(L);
    }

    // no args
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeMusic);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::stopSoundsAsOrderedArguments(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TMediaData mediaData{};
    int numArgs = lua_gettop(L);
    QString stringValue;
    int intValue = 0;

    // values as ordered args: name[,key][,tag][,priority])
    for (int i = 1; i <= numArgs; i++) {
        if (lua_isnil(L, i)) {
            continue;
        }

        switch (i) {
        case 1:
            stringValue = getVerifiedString(L, __func__, i, "name");

            if (QDir::homePath().contains('\\')) {
                stringValue.replace('/', R"(\)");
            } else {
                stringValue.replace('\\', "/");
            }

            mediaData.setMediaFileName(stringValue);
            break;
        case 2:
            stringValue = getVerifiedString(L, __func__, i, "key");
            mediaData.setMediaKey(stringValue);
            break;
        case 3:
            stringValue = getVerifiedString(L, __func__, i, "tag");
            mediaData.setMediaTag(stringValue);
            break;
        case 4:
            intValue = getVerifiedInt(L, __func__, i, "priority");

            if (intValue > TMediaData::MediaPriorityMax) {
                intValue = TMediaData::MediaPriorityMax;
            } else if (intValue < TMediaData::MediaPriorityMin) {
                intValue = TMediaData::MediaPriorityMin;
            }

            mediaData.setMediaPriority(intValue);
            break;
        }
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Private
int TLuaInterpreter::stopSoundsAsTableArgument(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        // key at index -2 and value at index -1
        QString key = getVerifiedString(L, __func__, -2, "table keys");
        key = key.toLower();

        if (key == QLatin1String("name") || key == QLatin1String("key") || key == QLatin1String("tag")) {
            QString value = getVerifiedString(L, __func__, -1, key == QLatin1String("name") ? "value for name" : key == QLatin1String("key") ? "value for key" : "value for tag");

            if (key == QLatin1String("name") && !value.isEmpty()) {
                if (QDir::homePath().contains('\\')) {
                    value.replace('/', R"(\)");
                } else {
                    value.replace('\\', "/");
                }

                mediaData.setMediaFileName(value);
            } else if (key == QLatin1String("key") && !value.isEmpty()) {
                mediaData.setMediaKey(value);
            } else if (key == QLatin1String("tag") && !value.isEmpty()) {
                mediaData.setMediaTag(value);
            }
        } else if (key == QLatin1String("priority")) {
            int value = getVerifiedInt(L, __func__, -1, "value for priority must be integer");

            if (key == QLatin1String("priority")) {
                if (value > TMediaData::MediaPriorityMax) {
                    value = TMediaData::MediaPriorityMax;
                } else if (value < TMediaData::MediaPriorityMin) {
                    value = TMediaData::MediaPriorityMin;
                }

                mediaData.setMediaPriority(value);
            }
        }

        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    host.mpMedia->stopMedia(mediaData);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopSounds
int TLuaInterpreter::stopSounds(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TMediaData mediaData{};

    if (lua_gettop(L)) {
        if (lua_istable(L, 1)) {
            return stopSoundsAsTableArgument(L);
        }

        return stopSoundsAsOrderedArguments(L);
    }

    // no args
    mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
    mediaData.setMediaType(TMediaData::MediaTypeSound);

    host.mpMedia->stopMedia(mediaData);
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#moveCursorEnd
int TLuaInterpreter::moveCursorEnd(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->moveCursorEnd();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLastLineNumber
int TLuaInterpreter::getLastLineNumber(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE_NIL(L, windowName);
    int number = console ? console->getLastLineNumber() : -1;
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
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) {
        windowName = WINDOW_NAME(L, s++);
    }
    double b = getVerifiedDouble(L, __func__, s, "number of lines");
    Host& host = getHostFromLua(L);
    host.getTriggerUnit()->setTriggerStayOpen(windowName, static_cast<int>(b));
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLink
int TLuaInterpreter::setLink(lua_State* L)
{
    QString windowName, linkFunction{QString()};
    int funcRef{0};
    int s = 1;
    if (lua_gettop(L) > 2) {
        windowName = WINDOW_NAME(L, s++);
    }

    if (!(lua_isstring(L, s) || lua_isfunction(L, s))) {
        lua_pushfstring(L, "setLink: bad argument #%d type (command as string or function expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }
    if (lua_isfunction(L, s)) {
        lua_pushvalue(L, s++);
        funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        linkFunction = lua_tostring(L, s++);
    }

    QString linkHint = getVerifiedString(L, __func__, s++, "tooltip");

    Host& host = getHostFromLua(L);
    QStringList _linkFunction;
    _linkFunction << linkFunction;
    QStringList _linkHint;
    _linkHint << linkHint;
    QVector<int> _linkReference;
    _linkReference << funcRef;

    auto console = CONSOLE(L, windowName);
    console->setLink(_linkFunction, _linkHint, _linkReference);
    if (console != host.mpConsole) {
        console->mUpperPane->forceUpdate();
        console->mLowerPane->forceUpdate();
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setPopup
int TLuaInterpreter::setPopup(lua_State* L)
{
    QString windowName = "";
    QStringList _hintList;
    QStringList _commandList;
    QVector<int> luaReference;
    int s = 1;
    int n = lua_gettop(L);

    // console name is an optional first argument
    if (n > 2) {
        windowName = WINDOW_NAME(L, s++);
    }

    if (!lua_istable(L, s)) {
        lua_pushfstring(L, "setPopup: bad argument #%d type (command list as table expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }
    lua_pushnil(L);
    while (lua_next(L, s) != 0) {
        // key at index -2 and value at index -1
        if (lua_type(L, -1) == LUA_TSTRING) {
            QString cmd = lua_tostring(L, -1);
            _commandList << cmd;
            luaReference << 0;
        }

        if (lua_type(L, -1) == LUA_TFUNCTION) {
            lua_pushvalue(L, -1);
            _commandList << QString();
            luaReference << luaL_ref(L, LUA_REGISTRYINDEX);
        }
        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }
    if (!lua_istable(L, ++s)) {
        lua_pushfstring(L, "setPopup: bad argument #%d type (hint list as table expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }
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

    Host& host = getHostFromLua(L);
    if (_commandList.size() != _hintList.size()) {
        lua_pushstring(L, "setPopup: commands and hints list aren't the same size");
        return lua_error(L);
    }

    auto console = CONSOLE(L, windowName);
    console->setLink(_commandList, _hintList, luaReference);
    if (console != host.mpConsole) {
        console->mUpperPane->forceUpdate();
        console->mLowerPane->forceUpdate();
    }

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBold
int TLuaInterpreter::setBold(lua_State* L)
{
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable bold attribute");
    auto console = CONSOLE(L, windowName);
    console->setDisplayAttributes(TChar::Bold, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setItalics
int TLuaInterpreter::setItalics(lua_State* L)
{
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable italic attribute");
    auto console = CONSOLE(L, windowName);
    console->setDisplayAttributes(TChar::Italic, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setOverline
int TLuaInterpreter::setOverline(lua_State* L)
{
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable overline attribute");
    auto console = CONSOLE(L, windowName);
    console->setDisplayAttributes(TChar::Overline, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setReverse
int TLuaInterpreter::setReverse(lua_State* L)
{
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable reverse attribute");
    auto console = CONSOLE(L, windowName);
    console->setDisplayAttributes(TChar::Reverse, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setStrikeOut
int TLuaInterpreter::setStrikeOut(lua_State* L)
{
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable strikeout attribute");
    auto console = CONSOLE(L, windowName);
    console->setDisplayAttributes(TChar::StrikeOut, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setUnderline
int TLuaInterpreter::setUnderline(lua_State* L)
{
    QString windowName;
    int s = 1;
    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, s++);
    }
    bool isAttributeEnabled = getVerifiedBool(L, __func__, s, "enable underline attribute");
    auto console = CONSOLE(L, windowName);
    console->setDisplayAttributes(TChar::Underline, isAttributeEnabled);
    lua_pushboolean(L, true);
    return 1;
}

// No documentation available in wiki - internal function used by printError in DebugTools.lua
int TLuaInterpreter::errorc(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int n = lua_gettop(L);
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
        if (host.mpConsole->buffer.size() > 0 && !host.mpConsole->buffer.lineBuffer.at(host.mpConsole->buffer.lineBuffer.size() - 1).isEmpty()) {
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
    Host& host = getHostFromLua(L);
    int n = lua_gettop(L);
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
    QString event = getVerifiedString(L, __func__, 1, "event name");
    QString error = getVerifiedString(L, __func__, 2, "error message");
    host.mLuaInterpreter.logEventError(event, error);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#hideToolBar
int TLuaInterpreter::hideToolBar(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};

    Host& host = getHostFromLua(L);
    host.getActionUnit()->hideToolBar(windowName);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#showToolBar
int TLuaInterpreter::showToolBar(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};

    Host& host = getHostFromLua(L);
    host.getActionUnit()->showToolBar(windowName);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendATCP
int TLuaInterpreter::sendATCP(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "sendATCP: bad argument #1 type (message as string expected, got %1!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    std::string msg = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 1));

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
    std::string msg = host.mTelnet.encodeAndCookBytes(lua_tostring(L, 1));

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
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "sendTelnetChannel102: bad argument #1 type (message bytes {2 characters} as string expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }
    std::string msg = lua_tostring(L, 1);
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
        int id = qRound(lua_tonumber(L, index));
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
        QString name = lua_tostring(L, index);
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getButtonState
int TLuaInterpreter::getButtonState(lua_State* L)
{
    auto& host = getHostFromLua(L);
    if (!lua_gettop(L)) {
        // The original function only works in the script for a push-down button
        // and takes no arguments so provide the backwards compatible behaviour
        // if that is the case:
        lua_pushnumber(L, host.mpConsole->getButtonState());
        return 1;
    }

    auto [retCount, pItem] = getTActionFromIdOrName(L, 1, __func__);
    if (retCount) {
        // pItem will be a nullptr if retCount is non-zero:
        return retCount;
    }

    lua_pushboolean(L, pItem->mButtonState);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setButtonState
int TLuaInterpreter::setButtonState(lua_State* L)
{
    auto [retCount, pItem] = getTActionFromIdOrName(L, 1, __func__);
    if (retCount) {
        // pItem will be a nullptr if retCount is non-zero:
        return retCount;
    }

    auto checked = getVerifiedBool(L, __func__, 2, "checked");

    if (pItem->mButtonState != checked) {
        pItem->mButtonState = checked;
        if (pItem->mpEButton) {
            pItem->mpEButton->setChecked(checked);
        }
        if (pItem->mpFButton) {
            pItem->mpFButton->setChecked(checked);
        }
        lua_pushboolean(L, true);
        return 1;
    }

    // We only returned in the above (with a true value) if we changed the state:
    lua_pushboolean(L, false);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getNetworkLatency
int TLuaInterpreter::getNetworkLatency(lua_State* L)
{
    Host& host = getHostFromLua(L);
    lua_pushnumber(L, host.mTelnet.networkLatencyTime);
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getUserWindowSize
int TLuaInterpreter::getUserWindowSize(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};

    Host& host = getHostFromLua(L);
    QSize userWindowSize = host.mpConsole->getUserWindowSize(windowName);
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
    bool repeating{};
    double time = getVerifiedDouble(L, __func__, 1, "time in seconds {maybe decimal}");
    int n = lua_gettop(L);

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (lua_isfunction(L, 2)) {
        if (n > 2) {
            repeating = getVerifiedBool(L, __func__, 3, "repeating", true);
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

    QString luaCode = getVerifiedString(L, __func__, 2, "script or function name");
    if (n > 2) {
        repeating = getVerifiedBool(L, __func__, 3, "repeating", true);
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
    int expiryCount = -1;
    QString exactMatchPattern = getVerifiedString(L, __func__, 1, "exact match pattern");

    if (lua_isnumber(L, 3)) {
        expiryCount = lua_tonumber(L, 3);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 3)) {
        lua_pushfstring(L, "tempExactMatchTrigger: bad argument #3 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    if (lua_isstring(L, 2)) {
        triggerID = pLuaInterpreter->startTempExactMatchTrigger(exactMatchPattern, QString(lua_tostring(L, 2)), expiryCount);
    } else if (lua_isfunction(L, 2)) {
        triggerID = pLuaInterpreter->startTempExactMatchTrigger(exactMatchPattern, QString(), expiryCount);

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
    QString pattern = getVerifiedString(L, __func__, 1, "pattern");

    if (lua_isnumber(L, 3)) {
        expiryCount = lua_tonumber(L, 3);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 3)) {
        lua_pushfstring(L, "tempBeginOfLineTrigger: bad argument #3 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    if (lua_isstring(L, 2)) {
        triggerID = pLuaInterpreter->startTempBeginOfLineTrigger(pattern, QString(lua_tostring(L, 2)), expiryCount);
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
    int triggerID;
    int expiryCount = -1;
    QString substringPattern = getVerifiedString(L, __func__, 1, "substring pattern");

    if (lua_isnumber(L, 3)) {
        expiryCount = lua_tonumber(L, 3);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 3)) {
        lua_pushfstring(L, "tempTrigger: bad argument #3 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    if (lua_isstring(L, 2)) {
        triggerID = pLuaInterpreter->startTempTrigger(substringPattern, QString(lua_tostring(L, 2)), expiryCount);
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
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 2)) {
        lua_pushfstring(L, "tempPromptTrigger: bad argument #2 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }

    if (lua_isstring(L, 1)) {
        triggerID = pLuaInterpreter->startTempPromptTrigger(QString(lua_tostring(L, 1)), expiryCount);
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempColorTrigger
// This is documented as using a simple color table - but the numbers do not
// match ANSI numbering and fixing that would break existing scripts.
int TLuaInterpreter::tempColorTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    int value = getVerifiedInt(L, __func__, 1, "foreground color");

    // match ANSI numbering and fixing that would break existing scripts so it has
    // to be tweaked here (and in the Mudlet XML save file format!)
    int foregroundColor = TTrigger::scmIgnored;
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

    value = getVerifiedInt(L, __func__, 2, "background color");
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
        return warnArgumentValue(L, __func__, "only one of foreground and background colors can be -1 (ignored)");
    }

    int triggerID;
    int expiryCount = -1;

    if (lua_isnumber(L, 4)) {
        expiryCount = lua_tonumber(L, 4);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 4)) {
        lua_pushfstring(L, "tempColorTrigger: bad argument #4 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 4));
        return lua_error(L);
    }

    if (lua_isstring(L, 3)) {
        triggerID = pLuaInterpreter->startTempColorTrigger(foregroundColor, backgroundColor, QString(lua_tostring(L, 3)), expiryCount);
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

// Documentation: https://wiki.mudlet.org/w/Manual:Mudlet_Object_Functions#tempAnsiColorTrigger
// This is the replacement for tempColorTrigger() which uses the right numbers
// for ANSI colours in the range 0 to 255 or TTrigger::scmDefault for default
// colour or TTrigger::scmIgnored ignore; it is anticipated that additional
// special values less than zero may be added to detect other types of text (or
// for a 16M colour value where the components have to be given)
// Note that this function has four arguments, of which the *second* may be omitted. :-/
int TLuaInterpreter::tempAnsiColorTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    QString code;
    int ansiFgColor = TTrigger::scmIgnored;
    int ansiBgColor = TTrigger::scmIgnored;
    int s = 0;

    if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "tempAnsiColorTrigger: bad argument #%d type (foreground color as ANSI Color number {%d = ignore foreground color, %d = default color, 0 to 255 ANSI color} expected, got %s!)",
                        s, TTrigger::scmIgnored, TTrigger::scmDefault, luaL_typename(L, s));
        return lua_error(L);
    }
    {   // separate block so that "value" is not scoped to the whole function
        int value = lua_tointeger(L, s);
        if (value == TTrigger::scmIgnored && lua_gettop(L) < 2) {
            return warnArgumentValue(L, __func__, qsl(
                "invalid ANSI color number %1, it cannot be used (to ignore the foreground color) if the background color is omitted")
                .arg(value));
        }
        // At present we limit the range to (Trigger::scmIgnored),
        // (Trigger::scmDefault) and 0-255 ANSI colors - in the future we could
        // extend it to other "coded" values for locally generated textual
        // content
        if (!(value == TTrigger::scmIgnored || value == TTrigger::scmDefault || (value >= 0 && value <= 255))) {
            return warnArgumentValue(L, __func__, qsl(
                "invalid ANSI color number %1, only %2 (ignore foreground color), %3 (default foregroud color) or 0 to 255 recognised")
                .arg(QString::number(value), QString::number(TTrigger::scmIgnored), QString::number(TTrigger::scmDefault)));
        }
        if (value == TTrigger::scmIgnored && lua_gettop(L) < 4) {
            return warnArgumentValue(L, __func__, qsl(
                "invalid ANSI color number %1, you cannot ignore both foreground and background color (omitted)").arg(value));
        }
        ansiFgColor = value;
    }

    // s=1 at this point. If top=4 the next argument must be the BG color number,
    // otherwise it may have been omitted.
    if (lua_gettop(L) < s+3 && !lua_isnumber(L, s+1)) {
        // BG color omitted, skip this part
    } else if (!lua_isnumber(L, ++s)) {
        lua_pushfstring(L, "tempAnsiColorTrigger: bad argument #%d type (background color as ANSI Color number {%d = ignore foreground color, %d = default color, 0 to 255 ANSI color} expected, got %s!)",
                        s, TTrigger::scmIgnored, TTrigger::scmDefault, luaL_typename(L, s));
        return lua_error(L);
    } else {
        int value = lua_tointeger(L, s);
        if (!(value == TTrigger::scmIgnored || value == TTrigger::scmDefault || (value >= 0 && value <= 255))) {
            return warnArgumentValue(L, __func__, qsl(
                "invalid ANSI color number %1, only %2 (ignore background color), %3 (default background color) or 0 to 255 recognised")
                .arg(QString::number(value), QString::number(TTrigger::scmIgnored), QString::number(TTrigger::scmDefault)));
        } else if (value == TTrigger::scmIgnored && ansiFgColor == TTrigger::scmIgnored) {
                return warnArgumentValue(L, __func__, qsl(
                    "invalid ANSI color number %1, you cannot ignore both foreground and background color")
                    .arg(value));
        } else {
            ansiBgColor = value;
        }
    }

    if (lua_isstring(L, ++s)) {
        code = QString::fromUtf8(lua_tostring(L, s));
    } else if (lua_isfunction(L, s)) {
        // leave code as a null QString(), see below
    } else {
        lua_pushfstring(L, "tempAnsiColorTrigger: bad argument #%d type (code to run as a string or a function expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    int expiryCount = -1;
    if (lua_isnumber(L, ++s)) {
        expiryCount = lua_tonumber(L, s);
        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, ++s)) {
        lua_pushfstring(L, "tempAnsiColorTrigger: bad argument #%d value (trigger expiration count must be a number, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    int triggerID = pLuaInterpreter->startTempColorTrigger(ansiFgColor, ansiBgColor, code, expiryCount);
    if (code.isNull()) {
        auto trigger = host.getTriggerUnit()->getTrigger(triggerID);
        trigger->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, trigger);
        lua_pushvalue(L, s-1);
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
    int from = getVerifiedInt(L, __func__, 1, "line to start matching from");
    int howMany  = getVerifiedInt(L, __func__, 2, "how many lines to match for");
    int triggerID;
    // temp line triggers expire naturally on their own, thus don't need the expiry mechanism applicable to all other triggers
    int dontExpire = -1;

    if (lua_isstring(L, 3)) {
        triggerID = pLuaInterpreter->startTempLineTrigger(from, howMany, QString(lua_tostring(L, 3)), dontExpire);
    } else if (lua_isfunction(L, 3)) {
        triggerID = pLuaInterpreter->startTempLineTrigger(from, howMany, QString(), dontExpire);

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
    QString triggerName = getVerifiedString(L, __func__, 1, "trigger name create or add to");
    QString pattern = getVerifiedString(L, __func__, 2, "regex pattern to match");

    if (!lua_isstring(L, 3) && !lua_isfunction(L, 3)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #3 type (code to run as a string or a function expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    if (!lua_isnumber(L, 4)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #4 type (multiline flag as number expected, got %s!)", luaL_typename(L, 4));
        return lua_error(L);
    }
    bool multiLine = lua_tonumber(L, 4);

    if (!lua_isnumber(L, 7)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #7 type (filter flag as number expected, got %s!)", luaL_typename(L, 7));
        return lua_error(L);
    }
    bool filter = lua_tonumber(L, 7);

    if (!lua_isnumber(L, 8)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #8 type (match all flag as number expected, got %s!)", luaL_typename(L, 8));
        return lua_error(L);
    }
    bool matchAll = lua_tonumber(L, 8);

    int fireLength = getVerifiedInt(L, __func__, 12, "fire length");
    int lineDelta = getVerifiedInt(L, __func__, 13, "line delta");

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
    if (lua_type(L, 11) == LUA_TSTRING) {
        playSound = true;
        soundFile = lua_tostring(L, 11);
    } else {
        playSound = false;
    }

    int expiryCount = -1;

    if (lua_isnumber(L, 14)) {
        expiryCount = lua_tonumber(L, 14);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 14)) {
        lua_pushfstring(L, "tempComplexRegexTrigger: bad argument #14 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 14));
        return lua_error(L);
    }

    QStringList patterns;
    QList<int> propertyList;
    TTrigger* pP = host.getTriggerUnit()->findTrigger(triggerName);
    if (pP) {
        patterns = pP->getPatternsList();
        propertyList = pP->getRegexCodePropertyList();
    }
    patterns << pattern;
    if (colorTrigger) {
        propertyList << REGEX_COLOR_PATTERN;
    } else {
        propertyList << REGEX_PERL;
    }

    auto pT = new TTrigger("a", patterns, propertyList, multiLine, &host);
    pT->setIsFolder(false);
    pT->setIsActive(true);
    pT->setTemporary(true);
    pT->registerTrigger();
    pT->setName(triggerName);
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
        pT->setScript(lua_tostring(L, 3));
    } else if (lua_isfunction(L, 3)) {
        pT->setScript(QString());

        pT->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, pT);
        lua_pushvalue(L, 3);
        lua_settable(L, LUA_REGISTRYINDEX);
    }

    lua_pushnumber(L, pT->getID());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempButton
int TLuaInterpreter::tempButton(lua_State* L)
{
    //args: parent, name, orientation
    QString cmdButtonUp = "";
    QString cmdButtonDown = "";
    QString script = "";
    QString toolbar;
    QStringList nameL;
    nameL << toolbar;

    toolbar = getVerifiedString(L, __func__, 1, "toolbar name");
    QString name = getVerifiedString(L, __func__, 2, "button text");
    int orientation = getVerifiedInt(L, __func__, 3, "orientation");

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
    QString name = getVerifiedString(L, __func__, 1, "name");
    QString css = getVerifiedString(L, __func__, 2, "css");
    Host& host = getHostFromLua(L);
    auto actionsList = host.getActionUnit()->findActionsByName(name);
    if (actionsList.empty()) {
        return warnArgumentValue(L, __func__, qsl("no button named '%1' found").arg(name));
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

    name = getVerifiedString(L, __func__, 1, "name");
    int location = getVerifiedInt(L, __func__, 2, "location");
    int orientation = getVerifiedInt(L, __func__, 3, "orientation");

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
    QString regexPattern = getVerifiedString(L, __func__, 1, "regex pattern");

    if (lua_isnumber(L, 3)) {
        expiryCount = lua_tonumber(L, 3);

        if (expiryCount < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "trigger expiration count must be nil or greater than zero, got %1").arg(expiryCount));
        }
    } else if (!lua_isnoneornil(L, 3)) {
        lua_pushfstring(L, "tempRegexTrigger: bad argument #3 value (trigger expiration count must be nil or a number, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    if (lua_isstring(L, 2)) {
        triggerID = pLuaInterpreter->startTempRegexTrigger(regexPattern, lua_tostring(L, 2), expiryCount);
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
    QString regex = getVerifiedString(L, __func__, 1, "regex-type pattern");
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    if (lua_isfunction(L, 2)) {

        int result = pLuaInterpreter->startTempAlias(regex, QString());
        if (result == -1) {
            lua_pushnumber(L, -1);
            return 2;
        }

        TAlias* alias = host.getAliasUnit()->getAlias(result);
        Q_ASSERT_X(alias,
                   "TLuaInterpreter::tempAlias(...)",
                   "Got a positive result from LuaInterpreter::startTempAlias(...) but that failed to produce pointer to it from Host::mAliasUnit::getAlias(...)");
        alias->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, alias);
        lua_pushvalue(L, 2);
        lua_settable(L, LUA_REGISTRYINDEX);
        lua_pushnumber(L, result);
        return 1;
    }

    if (!lua_isstring(L, 2)) {
        lua_pushfstring(L, "tempAlias: bad argument #2 type (lua script as string or function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    QString script{lua_tostring(L, 2)};

    lua_pushnumber(L, pLuaInterpreter->startTempAlias(regex, script));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#exists
int TLuaInterpreter::exists(lua_State* L)
{
    QString name = getVerifiedString(L, __func__, 1, "name");
    QString type = getVerifiedString(L, __func__, 2, "type");
    Host& host = getHostFromLua(L);
    int count = 0;
    type = type.toLower();
    if (type == qsl("timer")) {
        count = host.getTimerUnit()->mLookupTable.count(name);
    } else if (type == qsl("trigger")) {
        count = host.getTriggerUnit()->mLookupTable.count(name);
    } else if (type == qsl("alias")) {
        count = host.getAliasUnit()->mLookupTable.count(name);
    } else if (type == qsl("keybind")) {
        count = host.getKeyUnit()->mLookupTable.count(name);
    } else if (type == qsl("button")) {
        count = host.getActionUnit()->findActionsByName(name).size();
    } else if (type == qsl("script")) {
        count = host.getScriptUnit()->findScriptId(name).size();
    } else {
        return warnArgumentValue(L, __func__, qsl(
            "invalid item type '%1' given, it should be one of: 'alias', 'button', 'script', 'keybind', 'timer' or 'trigger'").arg(type));
    }
    lua_pushnumber(L, count);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#isActive
int TLuaInterpreter::isActive(lua_State* L)
{
    QString name = getVerifiedString(L, __func__, 1, "item name");
    // Although we only use 4 ASCII strings the user may not enter a purely
    // ASCII value which we might have to report...
    QString type = getVerifiedString(L, __func__, 2, "item type");

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
    } else if (type.compare(QLatin1String("button"), Qt::CaseInsensitive) == 0) {
        QMap<int, TAction*> actions = host.getActionUnit()->getActionList();
        for (auto action : actions) {
            if (action->getName() == name && action->isActive()) {
                ++cnt;
            }
        }
    } else if (type.compare(QLatin1String("script"), Qt::CaseInsensitive) == 0) {
        QMap<int, TScript*> scripts = host.getScriptUnit()->getScriptList();
        for (auto script : scripts) {
            if (script->getName() == name && script->isActive()) {
                ++cnt;
            }
        }
    } else {
        return warnArgumentValue(L, __func__, qsl(
            "invalid item type '%1' given, it should be one (case insensitive) of: 'alias', 'button', 'script', 'keybind', 'timer' or 'trigger'").arg(type));
    }
    lua_pushnumber(L, cnt);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permAlias
int TLuaInterpreter::permAlias(lua_State* L)
{
    QString name = getVerifiedString(L, __func__, 1, "alias name");
    QString parent = getVerifiedString(L, __func__, 2, "alias group/parent");
    QString regex = getVerifiedString(L, __func__, 3, "regexp pattern");
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(4); !validationResult) {
        lua_pushfstring(L, "permAlias: bad argument #%d (%s)", 4, validationMessage.toUtf8().constData());
        return lua_error(L);
    }

    QString script{lua_tostring(L, 4)};
    auto [aliasId, message] = pLuaInterpreter->startPermAlias(name, parent, regex, script);
    if (aliasId == -1) {
        lua_pushfstring(L, "permAlias: cannot create alias (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, aliasId);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getScript
int TLuaInterpreter::getScript(lua_State* L)
{
    int n = lua_gettop(L);
    int pos = 1;
    QString name = getVerifiedString(L, __func__, 1, "script name");
    if (n > 1) {
        pos = getVerifiedInt(L, __func__, 2, "script position");
    }
    Host& host = getHostFromLua(L);

    auto ids = host.getScriptUnit()->findScriptId(name);
    auto pS = host.getScriptUnit()->getScript(ids.value(--pos, -1));
    if (!pS) {
        lua_pushnumber(L, -1);
        lua_pushstring(L, qsl("script \"%1\" at position \"%2\" not found").arg(name).arg(++pos).toUtf8().constData());
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
    QString name = getVerifiedString(L, __func__, 1, "script name");

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(2); !validationResult) {
        lua_pushfstring(L, "setScript: bad argument #%d (%s)", 2, validationMessage.toUtf8().constData());
        return lua_error(L);
    }
    QString luaCode{lua_tostring(L, 2)};

    if (n > 2) {
        pos = getVerifiedInt(L, __func__, 3, "script position");
    }

    auto [id, message] = pLuaInterpreter->setScriptCode(name, luaCode, --pos);
    lua_pushnumber(L, id);
    if (id == -1) {
        lua_pushfstring(L, "permScript: cannot set script (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, id);

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permScript
int TLuaInterpreter::permScript(lua_State* L)
{
    QString name = getVerifiedString(L, __func__, 1, "script name");
    QString parent = getVerifiedString(L, __func__, 2, "script parent name");
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(3); !validationResult) {
        lua_pushfstring(L, "permScript: bad argument #%d (%s)", 3, validationMessage.toUtf8().constData());
        return lua_error(L);
    }
    QString luaCode{lua_tostring(L, 3)};
    auto [id, message] = pLuaInterpreter->createPermScript(name, parent, luaCode);
    if (id == -1) {
        lua_pushfstring(L, "permScript: cannot create script (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, id);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permTimer
int TLuaInterpreter::permTimer(lua_State* L)
{
    QString name = getVerifiedString(L, __func__, 1, "timer name");
    QString parent = getVerifiedString(L, __func__, 2, "timer parent name");
    double time = getVerifiedDouble(L, __func__, 3, "time in seconds");
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(4); !validationResult) {
        lua_pushfstring(L, "permTimer: bad argument #%d (%s)", 4, validationMessage.toUtf8().constData());
        return lua_error(L);
    }
    QString luaCode{lua_tostring(L, 4)};
    auto [id, message] = pLuaInterpreter->startPermTimer(name, parent, time, luaCode);
    if (id == -1) {
        lua_pushfstring(L, "permTimer: cannot create timer (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, id);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permSubstringTrigger
int TLuaInterpreter::permSubstringTrigger(lua_State* L)
{
    QString name = getVerifiedString(L, __func__, 1, "trigger name");
    QString parent = getVerifiedString(L, __func__, 2, "trigger parent");
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
            regList << lua_tostring(L, -1);
        }
        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(4); !validationResult) {
        lua_pushfstring(L, "permSubstringTrigger: bad argument #%d (%s)", 4, validationMessage.toUtf8().constData());
        return lua_error(L);
    }

    QString script{lua_tostring(L, 4)};
    auto [triggerID, message] = pLuaInterpreter->startPermSubstringTrigger(name, parent, regList, script);
    if(triggerID == - 1) {
        lua_pushfstring(L, "permSubstringTrigger: cannot create trigger (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permPromptTrigger
int TLuaInterpreter::permPromptTrigger(lua_State* L)
{
    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    QString triggerName = getVerifiedString(L, __func__, 1, "trigger name");
    QString parentName = getVerifiedString(L, __func__, 2, "parent trigger name");
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(3); !validationResult) {
        lua_pushfstring(L, "permPromptTrigger: bad argument #%d (%s)", 3, validationMessage.toUtf8().constData());
        return lua_error(L);
    }
    QString luaFunction = lua_tostring(L, 3);

    auto [triggerID, message] = pLuaInterpreter->startPermPromptTrigger(triggerName, parentName, luaFunction);
    if(triggerID == - 1) {
        lua_pushfstring(L, "permPromptTrigger: cannot create trigger (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, triggerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permKey
int TLuaInterpreter::permKey(lua_State* L)
{
    QString keyName = getVerifiedString(L, __func__, 1, "key name");
    QString parentGroup = getVerifiedString(L, __func__, 2, "key parent group");

    uint_fast8_t argIndex = 3;
    int keyModifier = Qt::NoModifier;
    if (lua_gettop(L) > 4) {
        keyModifier = getVerifiedInt(L, __func__, 3, "key modifier", true);
        argIndex++;
    }
    int keyCode = getVerifiedInt(L, __func__, argIndex, "key code");

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(++argIndex); !validationResult) {
        lua_pushfstring(L, "permKey: bad argument #%d (%s)", argIndex, validationMessage.toUtf8().constData());
        return lua_error(L);
    }

    QString luaFunction{lua_tostring(L, argIndex)};
    auto [keyID, message] = pLuaInterpreter->startPermKey(keyName, parentGroup, keyCode, keyModifier, luaFunction);
    if(keyID == - 1) {
        lua_pushfstring(L, "permKey: cannot create key (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, keyID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#tempKey
int TLuaInterpreter::tempKey(lua_State* L)
{
    uint_fast8_t argIndex = 1;
    int keyModifier = Qt::NoModifier;
    if (lua_gettop(L) > 2) {
        keyModifier = getVerifiedInt(L, __func__, 1, "key modifier", true);
        argIndex++;
    }
    int keyCode = getVerifiedInt(L, __func__, argIndex, "key code");

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();

    if (lua_isfunction(L, ++argIndex)) {

        int result = pLuaInterpreter->startTempKey(keyModifier, keyCode, QString());
        if (result == -1) {
            lua_pushnumber(L, -1);
            return 2;
        }

        TKey* key = host.getKeyUnit()->getKey(result);
        Q_ASSERT_X(key,
                   "TLuaInterpreter::tempKey(...)",
                   "Got a positive result from LuaInterpreter::startTempKey(...) but that failed to produce pointer to it from Host::mKeyUnit::getKey(...)");
        key->mRegisteredAnonymousLuaFunction = true;
        lua_pushlightuserdata(L, key);
        lua_pushvalue(L, argIndex);
        lua_settable(L, LUA_REGISTRYINDEX);
        lua_pushnumber(L, result);
        return 1;
    }

    if (!lua_isstring(L, argIndex)) {
        lua_pushfstring(L, "tempKey: bad argument #%d type (lua script as string or function expected, got %s!)", argIndex, luaL_typename(L, argIndex));
        return lua_error(L);
    }
    QString luaFunction{lua_tostring(L, argIndex)};

    int timerID = pLuaInterpreter->startTempKey(keyModifier, keyCode, luaFunction);
    lua_pushnumber(L, timerID);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permBeginOfLineStringTrigger
int TLuaInterpreter::permBeginOfLineStringTrigger(lua_State* L)
{
    QString name = getVerifiedString(L, __func__, 1, "trigger name");
    QString parent = getVerifiedString(L, __func__, 2, "trigger parent");

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
            regList << lua_tostring(L, -1);
        }
        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(4); !validationResult) {
        lua_pushfstring(L, "permBeginOfLineStringTrigger: bad argument #%d (%s)", 4, validationMessage.toUtf8().constData());
        return lua_error(L);
    }

    QString script{lua_tostring(L, 4)};
    auto [triggerId, message] = pLuaInterpreter->startPermBeginOfLineStringTrigger(name, parent, regList, script);
    if (triggerId == -1) {
        lua_pushfstring(L, "permRegexTrigger: cannot create trigger (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, triggerId);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#permRegexTrigger
int TLuaInterpreter::permRegexTrigger(lua_State* L)
{
    QString name = getVerifiedString(L, __func__, 1, "trigger name");
    QString parent = getVerifiedString(L, __func__, 2, "trigger parent");

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
            regList << lua_tostring(L, -1);
        }
        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    Host& host = getHostFromLua(L);
    TLuaInterpreter* pLuaInterpreter = host.getLuaInterpreter();
    if (auto [validationResult, validationMessage] = pLuaInterpreter->validateLuaCodeParam(4); !validationResult) {
        lua_pushfstring(L, "permRegexTrigger: bad argument #%d (%s)", 4, validationMessage.toUtf8().constData());
        return lua_error(L);
    }

    QString script{lua_tostring(L, 4)};
    auto [triggerId, message] = pLuaInterpreter->startPermRegexTrigger(name, parent, regList, script);
    if (triggerId == -1) {
        lua_pushfstring(L, "permRegexTrigger: cannot create trigger (%s)", message.toUtf8().constData());
        return lua_error(L);
    }
    lua_pushnumber(L, triggerId);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#invokeFileDialog
int TLuaInterpreter::invokeFileDialog(lua_State* L)
{
    bool luaDir = getVerifiedBool(L, __func__, 1, "fileOrFolder");
    QString title = getVerifiedString(L, __func__, 2, "dialogTitle");

    if (!luaDir) {
        QString fileName = QFileDialog::getExistingDirectory(nullptr, title, QDir::currentPath());
        lua_pushstring(L, fileName.toUtf8().constData());
        return 1;
    } else {
        QString fileName = QFileDialog::getOpenFileName(nullptr, title, QDir::currentPath());
        lua_pushstring(L, fileName.toUtf8().constData());
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getTimestamp
int TLuaInterpreter::getTimestamp(lua_State* L)
{
    int n = lua_gettop(L);
    int s = 1;
    QString name;
    if (n > 1) {
        name = getVerifiedString(L, __func__, s++, "mini console, user window or buffer name {may be omitted for the \"main\" console}");
        if (name == QLatin1String("main")) {
            // clear it so it is treated as the main console below
            name.clear();
        }
    }

    qint64 luaLine = getVerifiedInt(L, __func__, s, "line number");
    if (luaLine < 1) {
        return warnArgumentValue(L, __func__, qsl(
            "line number %1 invalid, it should be greater than zero").arg(luaLine));
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
        if (!pC) {
            return warnArgumentValue(L, __func__, qsl("mini console, user window or buffer '%1' not found)").arg(name));
        }
        if (luaLine > 0 && luaLine < pC->buffer.timeBuffer.size()) {
            lua_pushstring(L, pC->buffer.timeBuffer.at(luaLine).toUtf8().constData());
        } else {
            lua_pushstring(L, "getTimestamp: invalid line number");
        }
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBorderColor
int TLuaInterpreter::setBorderColor(lua_State* L)
{
    int luaRed = getVerifiedInt(L, __func__, 1, "red");
    int luaGreen = getVerifiedInt(L, __func__, 2, "green");
    int luaBlue = getVerifiedInt(L, __func__, 3, "blue");
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
    int id = getVerifiedInt(L, __func__, 1, "roomID");
    int x = getVerifiedInt(L, __func__, 2, "x");
    int y = getVerifiedInt(L, __func__, 3, "y");
    int z = getVerifiedInt(L, __func__, 4, "z");
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpMap->setRoomCoordinates(id, x, y, z));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setCustomEnvColor
int TLuaInterpreter::setCustomEnvColor(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "environmentID");
    int r = getVerifiedInt(L, __func__, 2, "r");
    int g = getVerifiedInt(L, __func__, 3, "g");
    int b = getVerifiedInt(L, __func__, 4, "b");
    int alpha = getVerifiedInt(L, __func__, 5, "a");
    Host& host = getHostFromLua(L);
    host.mpMap->mCustomEnvColors[id] = QColor(r, g, b, alpha);
    host.mpMap->mUnsavedMap = true;
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setAreaName
int TLuaInterpreter::setAreaName(lua_State* L)
{
    int id = -1;
    QString existingName;
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    if (lua_isnumber(L, 1)) {
        id = lua_tonumber(L, 1);
        if (id < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "number %1 is not a valid area id as it is less than 1)").arg(id));
        }
        // Strangely, previous code allowed this command to create a NEW area's name
        // with this ID, but without a TArea instance to accompany it (the latter was/is
        // instantiated as needed when a room is moved to the relevant area...) and we
        // need to continue to allow this - Slysven
        //        else if (!host.mpMap->mpRoomDB->getAreaIDList().contains(id)) {
        //            return warnArgumentValue(L, __func__, qsl(
        //                "number %1 is not a valid area id").arg(id));
        //        }
    } else if (lua_isstring(L, 1)) {
        existingName = lua_tostring(L, 1);
        id = host.mpMap->mpRoomDB->getAreaNamesMap().key(existingName, 0);
        if (existingName.isEmpty()) {
            return warnArgumentValue(L, __func__, "area name cannot be empty");
        } else if (!host.mpMap->mpRoomDB->getAreaNamesMap().values().contains(existingName)) {
            return warnArgumentValue(L, __func__, qsl("area name '%1' does not exist").arg(existingName));
        } else if (host.mpMap->mpRoomDB->getAreaNamesMap().value(-1).contains(existingName)) {
            return warnArgumentValue(L, __func__, qsl(
                "area name '%1' is reserved and protected - it cannot be changed").arg(existingName));
        }
    } else {
        lua_pushfstring(L,
                        "setAreaName: bad argument #1 type (area id as number or area name as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
    }

    QString newName = getVerifiedString(L, __func__, 2, "area name").trimmed();
    // Now allow non-Ascii names but eliminate any leading or trailing spaces

    if (newName.isEmpty()) {
        // Empty name not allowed (any more)
        return warnArgumentValue(L, __func__, "area names may not be empty strings (and spaces are trimmed from the ends)");
    } else if (host.mpMap->mpRoomDB->getAreaNamesMap().values().count(newName) > 0) {
        // That name is already IN the areaNamesMap, and since we now enforce
        // uniqueness there can be only one of it - so we can check if this is a
        // problem or just pointless quite easily...!
        if (host.mpMap->mpRoomDB->getAreaNamesMap().value(id) != newName) {
            // And it isn't the trivial case, where the given areaID already IS that name
            return warnArgumentValue(L, __func__, qsl(
                "area names may not be duplicated and area id %1 already has the name '%2'")
                .arg(QString::number(host.mpMap->mpRoomDB->getAreaNamesMap().key(newName)), newName));
        }
        // Renaming an area to the same name is pointlessly successful!
        lua_pushboolean(L, true);
        return 1;
    }

    bool isCurrentAreaRenamed = false;
    if (host.mpMap->mpMapper) {
        if (id > 0 && host.mpMap->mpRoomDB->getAreaNamesMap().value(id) == host.mpMap->mpMapper->comboBox_showArea->currentText()) {
            isCurrentAreaRenamed = true;
        }
    }

    bool result = host.mpMap->mpRoomDB->setAreaName(id, newName);
    if (result) {
        host.mpMap->mUnsavedMap = true;
        if (host.mpMap->mpMapper) {
            host.mpMap->mpMapper->updateAreaComboBox();
            if (isCurrentAreaRenamed) {
                host.mpMap->mpMapper->comboBox_showArea->setCurrentText(newName);
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
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int id;
    QString name;
    if (!lua_isnumber(L, 1)) {
        if (!lua_isstring(L, 1)) {
            lua_pushfstring(L,
                            "getRoomAreaName: bad argument #1 type (area id as number or area name as string\n"
                            "expected, got %s!)",
                            luaL_typename(L, 1));
            return lua_error(L);
        }
        name = lua_tostring(L, 1);
    } else {
        id = lua_tonumber(L, 1);
    }

    if (!name.isNull()) {
        int result = host.mpMap->mpRoomDB->getAreaNamesMap().key(name, -1);
        lua_pushnumber(L, result);
        if (result != -1) {
            return 1;
        } else {
            lua_pushfstring(L, "getRoomAreaName: string '%s' is not a valid area name", name.toUtf8().constData());
            return 2;
        }
    } else {
        if (host.mpMap->mpRoomDB->getAreaNamesMap().contains(id)) {
            lua_pushstring(L, host.mpMap->mpRoomDB->getAreaNamesMap().value(id).toUtf8().constData());
            return 1;
        } else {
            lua_pushnumber(L, -1);
            lua_pushfstring(L, "getRoomAreaName: number %d is not a valid area id", id);
            return 2;
        }
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addAreaName
int TLuaInterpreter::addAreaName(lua_State* L)
{
    QString name = getVerifiedString(L, __func__, 1, "area name").trimmed();

    Host& host = getHostFromLua(L);
    if ((!host.mpMap) || (!host.mpMap->mpRoomDB)) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    } else if (name.isEmpty()) {
        // Empty names now not allowed
        return warnArgumentValue(L, __func__, "area names may not be empty strings (and spaces are trimmed from the ends)");
    } else if (host.mpMap->mpRoomDB->getAreaNamesMap().values().count(name) > 0) {
        // That name is already IN the areaNamesMap
        return warnArgumentValue(L, __func__, qsl("area names may not be duplicated and area id %1 already has the name '%2'")
            .arg(QString::number(host.mpMap->mpRoomDB->getAreaNamesMap().key(name)), name));
    }

    // Note that adding an area name implicitly creates an underlying TArea instance
    lua_pushnumber(L, host.mpMap->mpRoomDB->addArea(name));
    host.mpMap->mUnsavedMap = true;

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
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    if (lua_isnumber(L, 1)) {
        id = lua_tonumber(L, 1);
        if (id < 1) {
            return warnArgumentValue(L, __func__, qsl("number %1 is not a valid area id greater than zero").arg(id));
        }
        if (!host.mpMap->mpRoomDB->getAreaIDList().contains(id) && !host.mpMap->mpRoomDB->getAreaNamesMap().contains(id)) {
            return warnArgumentValue(L, __func__, qsl("number %1 is not a valid area id").arg(id));
        }
    } else if (lua_isstring(L, 1)) {
        name = lua_tostring(L, 1);
        if (name.isEmpty()) {
            return warnArgumentValue(L, __func__, "an empty string is not a valid area name");
        } else if (!host.mpMap->mpRoomDB->getAreaNamesMap().values().contains(name)) {
            return warnArgumentValue(L, __func__, qsl("string '%1' is not a valid area name").arg(name));
        } else if (name == host.mpMap->getDefaultAreaName()) {
            return warnArgumentValue(L, __func__, "you can't delete the default area");
        }
    } else {
        lua_pushfstring(L,
                        "deleteArea: bad argument #1 type (area Id as number or area name as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 1));
        return lua_error(L);
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
        host.mpMap->mUnsavedMap = true;
        host.mpMap->mMapGraphNeedsUpdate = true;
    }
    lua_pushboolean(L, result);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteRoom
int TLuaInterpreter::deleteRoom(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "roomID");
    if (id <= 0) {
        return 0;
    }
    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpMap->mpRoomDB->removeRoom(id));
    host.mpMap->mUnsavedMap = true;
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setExit
int TLuaInterpreter::setExit(lua_State* L)
{
    int from = getVerifiedInt(L, __func__, 1, "from roomID");
    int to = getVerifiedInt(L, __func__, 2, "to roomID");

    int dir = dirToNumber(L, 3);
    if (!dir) {
        lua_pushfstring(L, "setExit: bad argument #3 type (direction as number or string expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    lua_pushboolean(L, host.mpMap->setExit(from, to, dir));
    host.mpMap->mMapGraphNeedsUpdate = true;
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomCoordinates
int TLuaInterpreter::getRoomCoordinates(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "roomID");
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
    int id = getVerifiedInt(L, __func__, 1, "roomID");
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
    int id = getVerifiedInt(L, __func__, 1, "roomID");
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
    int id = getVerifiedInt(L, __func__, 1, "room id");
    Host& host = getHostFromLua(L);
    bool added = host.mpMap->addRoom(id);
    lua_pushboolean(L, added);
    if (added) {
        host.mpMap->setRoomArea(id, -1, false);
        host.mpMap->mUnsavedMap = true;
        host.mpMap->mMapGraphNeedsUpdate = true;
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#createRoomID
int TLuaInterpreter::createRoomID(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    if (lua_gettop(L) > 0) {
        int minId = getVerifiedInt(L, __func__, 1, "minimum room Id", true);
        if (minId < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "minimum room id %1 is an optional value but if provided it must be greater than zero").arg(minId));
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
    int id = getVerifiedInt(L, __func__, 1, "roomID");

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
    int id = getVerifiedInt(L, __func__, 1, "roomID");
    int fgr = getVerifiedInt(L, __func__, 2, "color1Red");
    int fgg = getVerifiedInt(L, __func__, 3, "color1Green");
    int fgb = getVerifiedInt(L, __func__, 4, "color1Blue");
    int bgr = getVerifiedInt(L, __func__, 5, "color2Red");
    int bgg = getVerifiedInt(L, __func__, 6, "color2Green");
    int bgb = getVerifiedInt(L, __func__, 7, "color2Blue");
    float radius = getVerifiedFloat(L, __func__, 8, "highlightRadius");
    int alpha1 = getVerifiedInt(L, __func__, 9, "color1Alpha");
    int alpha2 = getVerifiedInt(L, __func__, 10, "color2Alpha");

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
    int fontSize = 50;
    float zoom = 30.0;
    bool showOnTop = true;
    bool noScaling = true;
    QString fontName;
    int foregroundTransparency = 255;
    int backgroundTransparency = 50;

    int args = lua_gettop(L);
    int area = getVerifiedInt(L, __func__, 1, "areaID");
    QString text = getVerifiedString(L, __func__, 2, "text");
    float posx = getVerifiedFloat(L, __func__, 3, "posX");
    float posy = getVerifiedFloat(L, __func__, 4, "posY");
    float posz = getVerifiedFloat(L, __func__, 5, "posZ");
    int fgr = getVerifiedInt(L, __func__, 6, "fgRed");
    int fgg = getVerifiedInt(L, __func__, 7, "fgGreen");
    int fgb = getVerifiedInt(L, __func__, 8, "fgBlue");
    int bgr = getVerifiedInt(L, __func__, 9, "bgRed");
    int bgg = getVerifiedInt(L, __func__, 10, "bgGreen");
    int bgb = getVerifiedInt(L, __func__, 11, "bgBlue");
    if (args > 11) {
        zoom = getVerifiedFloat(L, __func__, 12, "zoom", true);
        fontSize = getVerifiedInt(L, __func__, 13, "fontSize", true);
        if (args > 13) {
            showOnTop = getVerifiedBool(L, __func__, 14, "showOnTop", true);
            if (args > 14) {
                noScaling = getVerifiedBool(L, __func__, 15, "noScaling", true);
            }
        }
    }
    if (args > 15) {
        fontName = getVerifiedString(L, __func__, 16, "fontName", true);
    }
    if (args > 16) {
        foregroundTransparency = getVerifiedInt(L, __func__, 17, "foregroundTransparency", true);
    }
    if (args > 17) {
        backgroundTransparency = getVerifiedInt(L, __func__, 18, "backgroundTransparency", true);
    }

    Host& host = getHostFromLua(L);
    lua_pushinteger(L, host.mpMap->createMapLabel(area, text, posx, posy, posz, QColor(fgr, fgg, fgb, foregroundTransparency), QColor(bgr, bgg, bgb, backgroundTransparency), showOnTop, noScaling, zoom, fontSize, fontName));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMapZoom
int TLuaInterpreter::setMapZoom(lua_State* L)
{
    qreal zoom = getVerifiedFloat(L, __func__, 1, "zoom");
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
    int area = getVerifiedInt(L, __func__, 1, "areaID");
    QString imagePathFileName = getVerifiedString(L, __func__, 2, "imagePathFileName");
    float posx = getVerifiedFloat(L, __func__, 3, "posX");
    float posy = getVerifiedFloat(L, __func__, 4, "posY");
    float posz = getVerifiedFloat(L, __func__, 5, "posZ");
    float width = getVerifiedFloat(L, __func__, 6, "width");
    float height = getVerifiedFloat(L, __func__, 7, "height");
    float zoom = getVerifiedFloat(L, __func__, 8, "zoom");
    bool showOnTop = getVerifiedBool(L, __func__, 9, "showOnTop");

    Host& host = getHostFromLua(L);
    lua_pushinteger(L, host.mpMap->createMapImageLabel(area, imagePathFileName, posx, posy, posz, width, height, zoom, showOnTop));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDoor
int TLuaInterpreter::setDoor(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int roomId = getVerifiedInt(L, __func__, 1, "room id");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(roomId));
    }
    QString exitCmd = getVerifiedString(L, __func__, 2, "door command");

    if (exitCmd.compare(qsl("n")) && exitCmd.compare(qsl("e")) && exitCmd.compare(qsl("s")) && exitCmd.compare(qsl("w"))
        && exitCmd.compare(qsl("ne"))
        && exitCmd.compare(qsl("se"))
        && exitCmd.compare(qsl("sw"))
        && exitCmd.compare(qsl("nw"))
        && exitCmd.compare(qsl("up"))
        && exitCmd.compare(qsl("down"))
        && exitCmd.compare(qsl("in"))
        && exitCmd.compare(qsl("out"))) {
        // One of the above WILL BE ZERO if the exitCmd is ONE of the above qsls
        // So the above will be TRUE if NONE of above strings match - which
        // means we must treat the exitCmd as a SPECIAL exit
        if (!(pR->getSpecialExits().contains(exitCmd))) {
            // And NOT a special one either
            return warnArgumentValue(L, __func__, qsl(
                "room with id %1 does not have a special exit in direction '%2'")
                .arg(QString::number(roomId), exitCmd));
        }
        // else IS a valid special exit - so fall out of if and continue
    } else {
        // Is a normal exit so see if it is valid
        if (!(((!exitCmd.compare(qsl("n"))) && (pR->getExit(DIR_NORTH) > 0 || pR->exitStubs.contains(DIR_NORTH)))
                || ((!exitCmd.compare(qsl("e"))) && (pR->getExit(DIR_EAST) > 0 || pR->exitStubs.contains(DIR_EAST)))
                || ((!exitCmd.compare(qsl("s"))) && (pR->getExit(DIR_SOUTH) > 0 || pR->exitStubs.contains(DIR_SOUTH)))
                || ((!exitCmd.compare(qsl("w"))) && (pR->getExit(DIR_WEST) > 0 || pR->exitStubs.contains(DIR_WEST)))
                || ((!exitCmd.compare(qsl("ne"))) && (pR->getExit(DIR_NORTHEAST) > 0 || pR->exitStubs.contains(DIR_NORTHEAST)))
                || ((!exitCmd.compare(qsl("se"))) && (pR->getExit(DIR_SOUTHEAST) > 0 || pR->exitStubs.contains(DIR_SOUTHEAST)))
                || ((!exitCmd.compare(qsl("sw"))) && (pR->getExit(DIR_SOUTHWEST) > 0 || pR->exitStubs.contains(DIR_SOUTHWEST)))
                || ((!exitCmd.compare(qsl("nw"))) && (pR->getExit(DIR_NORTHWEST) > 0 || pR->exitStubs.contains(DIR_NORTHWEST)))
                || ((!exitCmd.compare(qsl("up"))) && (pR->getExit(DIR_UP) > 0 || pR->exitStubs.contains(DIR_UP)))
                || ((!exitCmd.compare(qsl("down"))) && (pR->getExit(DIR_DOWN) > 0 || pR->exitStubs.contains(DIR_DOWN)))
                || ((!exitCmd.compare(qsl("in"))) && (pR->getExit(DIR_IN) > 0 || pR->exitStubs.contains(DIR_IN)))
                || ((!exitCmd.compare(qsl("out"))) && (pR->getExit(DIR_OUT) > 0 || pR->exitStubs.contains(DIR_OUT))))) {
            // No there IS NOT a stub or real exit in the exitCmd direction
            return warnArgumentValue(L, __func__, qsl(
                "room with id %1 does not have a normal exit or a stub exit in direction '%2'")
                .arg(QString::number(roomId), exitCmd));
        }
        // else IS a valid stub or real normal exit -fall through to continue
    }

    int doorStatus = getVerifiedInt(L, __func__, 3, "door type  {0='none', 1='open', 2='closed' or 3='locked'}");
    if (doorStatus < 0 || doorStatus > 3) {
        return warnArgumentValue(L, __func__, qsl(
            "door type %1 is not one of 0='none', 1='open', 2='closed' or 3='locked'").arg(doorStatus));
    }

    bool result = pR->setDoor(exitCmd, doorStatus);
    if (result) {
        host.mpMap->mUnsavedMap = true;
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
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int roomId = getVerifiedInt(L, __func__, 1, "room id");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(roomId));
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
    int roomID = getVerifiedInt(L, __func__, 1, "room id");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("room id %1 doesn't exist").arg(roomID));
    }

    QString direction(dirToString(L, 2));
    if (direction.isEmpty()) {
        lua_pushfstring(L, "setExitWeight: bad argument #2 type (direction as string or number {between 1 and 12 inclusive} expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    if (!pR->hasExitOrSpecialExit(direction)) {
        return warnArgumentValue(L, __func__, qsl("room id %1 does not have an exit that can be identified from '%2'")
            .arg(QString::number(roomID), lua_tostring(L, 2)));
    }

    qint64 weight = getVerifiedInt(L, __func__, 3, "exit weight");
    if (weight < 0 || weight > std::numeric_limits<int>::max()) {
        return warnArgumentValue(L, __func__, qsl(
            "weight %1 is outside of the usable range of 0 (which resets the weight back to that of the destination room) to %2")
            .arg(QString::number(weight), QString::number(std::numeric_limits<int>::max())));
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
    int id_to = 0;
    int r = 255;
    int g = 0;
    int b = 0;
    Qt::PenStyle line_style(Qt::SolidLine);
    QString direction;
    QList<qreal> x;
    QList<qreal> y;
    QList<int> z;
    int id_from = getVerifiedInt(L, __func__, 1, "room id");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id_from);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("room id %1 does not exist").arg(id_from));
    }

    if (!lua_isnumber(L, 2) && !lua_istable(L, 2)) {
        lua_pushfstring(L, "addCustomLine: bad argument #2 type (target room id as number or coordinate list as table expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    if (lua_isnumber(L, 2)) {
        id_to = static_cast<int>(lua_tointeger(L, 2));
        TRoom* pR_to = host.mpMap->mpRoomDB->getRoom(id_to);
        if (!pR_to) {
            return warnArgumentValue(L, __func__, qsl("target room id %1 does not exist").arg(id_to));
        }
        int area = pR->getArea();
        int area_to = pR_to->getArea();
        if (area != area_to) {
            return warnArgumentValue(L, __func__, qsl(
                "target room is in area '%1' (id: %2) which is not the one '%3' (id: %4) in which this custom line is to be drawn")
                .arg((host.mpMap->mpRoomDB->getAreaNamesMap()).value(area_to), QString::number(area_to),
                        (host.mpMap->mpRoomDB->getAreaNamesMap()).value(area), QString::number(area)));
        }

        x.append(static_cast<qreal>(pR_to->x));
        y.append(static_cast<qreal>(pR_to->y));
        z.append(pR->z);
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
                    }
                    switch (j) {
                    case 1:
                        x.append(lua_tonumber(L, -1));
                        break;
                    case 2:
                        y.append(lua_tonumber(L, -1));
                        break;
                    case 3:
                        z.append(static_cast<int>(lua_tonumber(L, -1)));
                        break;
                    default:; // No-op
                    }
                }
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
        if (!i || !x.count()) {
            // If there is only an empty sub-table inside the table then i is
            // one but there is nothing in any of the QLists and things will
            // still blow up as per Issue #5272 - so also check for at least one
            // x-coordinate value:
            return warnArgumentValue(L, __func__, "missing coordinates to create the line to");
        }
        if (x.count() != y.count() || x.count() != z.count()) {
            return warnArgumentValue(L, __func__, "mismatch in numbers of coordinates for the points for the custom line given in table as second argument; each must contain three coordinates, i.e. x, y AND z numeric values as a sub-table");
        }
    }

    direction = dirToString(L, 3);
    if (direction.isEmpty()) {
        lua_pushfstring(L, "addCustomLine: bad argument #3 type (direction as string or number (between 1 and 12 inclusive) expected, got %s!)", luaL_typename(L, 3));
        return lua_error(L);
    }
    if (!pR->hasExitOrSpecialExit(direction)) {
        return warnArgumentValue(L, __func__, qsl("room id %1 does not have an exit in a direction that can be identified from '%2'")
            .arg(QString::number(id_from), lua_tostring(L, 3)));
    }

    QString lineStyleString = getVerifiedString(L, __func__, 4, "line style");
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
        return warnArgumentValue(L, __func__, qsl(
            "invalid line style '%1', only use one of: 'solid line', 'dot line', 'dash line', 'dash dot line' or 'dash dot dot line'")
            .arg(lineStyleString));
    }

    if (!lua_istable(L, 5)) {
        lua_pushfstring(L, "addCustomLine: bad argument #5 type (RGB color components as a table expected, got %s!)", luaL_typename(L, 5));
        return lua_error(L);
    } else {
        lua_pushnil(L);
        int tind = 0;
        while (lua_next(L, 5) != 0) {
            if (++tind <= 3) {
                if (lua_type(L, -1) != LUA_TNUMBER) {
                    lua_pushfstring(L,
                                    "addCustomLine: bad argument #5 table item #%d type (%s color component as a number between 0 and 255 expected, got %s!)",
                                    tind,
                                    (tind == 1 ? "red" : (tind == 2 ? "green" : "blue")),
                                    luaL_typename(L, -1));
                    return lua_error(L);
                }

                qint64 component = lua_tointeger(L, -1);
                if (component < 0 || component > 255) {
                    return warnArgumentValue(L, __func__, qsl(
                        "%1 color component in the table of the fifth argument is %2 which is out of the valid range (0 to 255)")
                        .arg((tind == 1 ? "red" : (tind == 2 ? "green" : "blue")), QString::number(component)));
                }
                switch (tind) {
                case 1:
                    r = static_cast<int>(component);
                    break;
                case 2:
                    g = static_cast<int>(component);
                    break;
                case 3:
                    b = static_cast<int>(component);
                    break;
                default:
                    Q_UNREACHABLE();
                }
            }
            lua_pop(L, 1);
        }
    }

    bool arrow = getVerifiedBool(L, __func__, 6, "end with arrow");
    int lz = z.at(0);
    QList<QPointF> points;
    // TODO: make provision for 3D custom lines (and store the z coordinates and allow them to vary)
    points.append(QPointF(x.at(0), y.at(0)));
    for (int i = 1, total = z.size(); i < total; ++i) {
        if (lz != z.at(i)) {
            return warnArgumentValue(L, __func__, qsl(
                "the z values are not all on the same level (first wrong value is %1 at index %2)")
                .arg(QString::number(z.at(i)), QString::number(i + 1)));
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

    host.mpMap->mUnsavedMap = true;

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
    int roomId = getVerifiedInt(L, __func__, 1, "room id");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("room id %1 doesn't exist").arg(roomId));
    }

    QString direction = dirToString(L, 2);
    if (direction.isEmpty()) {
        lua_pushfstring(L, "removeCustomLine: bad argument #2 type (direction as string or number (between 1 and 12 inclusive) expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    if (!pR->hasExitOrSpecialExit(direction)) {
        return warnArgumentValue(L, __func__, qsl(
            "room id %1 does not have an exit that can be identified from '%2'").arg(QString::number(roomId), lua_tostring(L, 2)));
    }

    if (0 >= (pR->customLines.remove(direction) + pR->customLinesArrow.remove(direction)
        + pR->customLinesStyle.remove(direction) + pR->customLinesColor.remove(direction))) {
        return warnArgumentValue(L, __func__, qsl(
            "room id %1 does not appear to have a custom exit line for the exit indentifed from '%2'")
            .arg(QString::number(roomId), lua_tostring(L, 2)));
    }
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
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCustomLines
int TLuaInterpreter::getCustomLines(lua_State* L)
{
    int roomID = getVerifiedInt(L, __func__, 1, "room id");
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    if (!pR) { //if the room doesn't exist return nil
        return warnArgumentValue(L, __func__, qsl("room %1 doesn't exist").arg(roomID));
    }
    lua_newtable(L); //return table customLines[]
    QStringList exits = pR->customLines.keys();
    for (int i = 0, iTotal = exits.size(); i < iTotal; ++i) {
        lua_pushstring(L, exits.at(i).toUtf8().constData());
        lua_newtable(L); //customLines[direction]
        lua_pushstring(L, "attributes");
        lua_newtable(L); //customLines[direction]["attributes"]
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
        lua_settable(L, -3); //customLines[direction]["attributes"]["style"]
        lua_pushstring(L, "arrow");
        lua_pushboolean(L, pR->customLinesArrow.value(exits.at(i)));
        lua_settable(L, -3); //customLines[direction]["attributes"]["arrow"]
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
        lua_settable(L, -3); //customLines[direction]["attributes"]["color"]
        lua_settable(L, -3); //customLines[direction]["attributes"]
        lua_pushstring(L, "points");
        lua_newtable(L); //customLines[direction][points]
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
        lua_settable(L, -3); //customLines[direction]["points"]
        lua_settable(L, -3); //customLines[direction]
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCustomLines1
int TLuaInterpreter::getCustomLines1(lua_State* L)
{
    int roomID = getVerifiedInt(L, __func__, 1, "room id");
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    if (!pR) { //if the room doesn't exist return nil
        return warnArgumentValue(L, __func__, qsl("room %1 doesn't exist").arg(roomID));
    }
    lua_newtable(L); //return table customLines[]
    QStringList exits = pR->customLines.keys();
    for (int i = 0, iTotal = exits.size(); i < iTotal; ++i) {
        lua_pushstring(L, exits.at(i).toUtf8().constData());
        lua_newtable(L); //customLines[direction]
        lua_pushstring(L, "attributes");
        lua_newtable(L); //customLines[direction]["attributes"]
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
        lua_settable(L, -3); //customLines[direction]["attributes"]["style"]
        lua_pushstring(L, "arrow");
        lua_pushboolean(L, pR->customLinesArrow.value(exits.at(i)));
        lua_settable(L, -3); //customLines[direction]["attributes"]["arrow"]
        lua_pushstring(L, "color");
        lua_newtable(L);
        lua_pushinteger(L, 1);
        lua_pushinteger(L, pR->customLinesColor.value(exits.at(i)).red());
        lua_settable(L, -3);
        lua_pushinteger(L, 2);
        lua_pushinteger(L, pR->customLinesColor.value(exits.at(i)).green());
        lua_settable(L, -3);
        lua_pushinteger(L, 3);
        lua_pushinteger(L, pR->customLinesColor.value(exits.at(i)).blue());
        lua_settable(L, -3);
        lua_settable(L, -3); //customLines[direction]["attributes"]["color"]
        lua_settable(L, -3); //customLines[direction]["attributes"]
        lua_pushstring(L, "points");
        lua_newtable(L); //customLines[direction]["points"]
        QList<QPointF> pointL = pR->customLines.value(exits.at(i));
        for (int k = 0, kTotal = pointL.size(); k < kTotal; ++k) {
            // To allow the output from here to be fed back into addCustomLine
            // we need to start the numbering from the Lua standard of 1 and
            // NOT the C/C++ standard of 0 - otherwise the end-user has to
            // fiddle with the zero-th entry to keep the points in order:
            lua_pushinteger(L, k+1);
            lua_newtable(L); //customLines[direction]["points"][3 x coordinates]
            lua_pushinteger(L, 1);
            lua_pushnumber(L, pointL.at(k).x());
            lua_settable(L, -3);
            lua_pushinteger(L, 2);
            lua_pushnumber(L, pointL.at(k).y());
            lua_settable(L, -3);
            lua_pushinteger(L, 3);
            lua_pushnumber(L, pR->z);
            lua_settable(L, -3);
            lua_settable(L, -3); //customLines[direction]["points"][3 x coordinates]
        }
        lua_settable(L, -3); //customLines[direction]["points"]
        lua_settable(L, -3); //customLines[direction]
    }
    return 1;
}
// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getExitWeights
int TLuaInterpreter::getExitWeights(lua_State* L)
{
    int roomID = getVerifiedInt(L, __func__, 1, "roomID");
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomID);
    lua_newtable(L);
    if (pR) {
        QStringList keys = pR->getExitWeights().keys();
        for (int i = 0; i < keys.size(); i++) {
            lua_pushstring(L, keys.at(i).toUtf8().constData());
            lua_pushnumber(L, pR->getExitWeight(keys.at(i)));
            lua_settable(L, -3);
        }
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteMapLabel
int TLuaInterpreter::deleteMapLabel(lua_State* L)
{
    int area = getVerifiedInt(L, __func__, 1, "areaID");
    int labelID = getVerifiedInt(L, __func__, 2, "labelID");
    Host& host = getHostFromLua(L);
    host.mpMap->deleteMapLabel(area, labelID);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#windowType
int TLuaInterpreter::windowType(lua_State* L)
{
    Host& host = getHostFromLua(L);
    QString windowName = getVerifiedString(L, __func__, 1, "window name");

    if (auto kind = host.windowType(windowName)) {
        lua_pushstring(L, kind->toUtf8().constData());
        return 1;
    }

    lua_pushnil(L);
    lua_pushfstring(L, "'%s' is not a known label, any type of console, nor command line", windowName.toUtf8().constData());
    return 2;
}


// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapLabels
int TLuaInterpreter::getMapLabels(lua_State* L)
{
    int area = getVerifiedInt(L, __func__, 1, "areaID");
    Host& host = getHostFromLua(L);
    lua_newtable(L);
    auto pA = host.mpMap->mpRoomDB->getArea(area);
    if (pA && !pA->mMapLabels.isEmpty()) {
        QMapIterator<int, TMapLabel> it(pA->mMapLabels);
        while (it.hasNext()) {
            it.next();
            lua_pushnumber(L, it.key());
            lua_pushstring(L, it.value().text.toUtf8().constData());
            lua_settable(L, -3);
        }
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapLabel
int TLuaInterpreter::getMapLabel(lua_State* L)
{
    int areaId = getVerifiedInt(L, __func__, 1, "areaID");

    if (!lua_isstring(L, 2) && !lua_isnumber(L, 2)) {
        lua_pushfstring(L, "getMapLabel: bad argument #2 type (labelID as number or labelText as string expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    QString labelText;
    int labelId = -1;
    if (lua_type(L, 2) == LUA_TNUMBER) {
        labelId = lua_tointeger(L, 2);
        if (labelId < 0) {
            return warnArgumentValue(L, __func__, qsl("labelID %1 is invalid, it must be zero or greater").arg(labelId));
        }
    } else {
        labelText = lua_tostring(L, 2);
        // Can be an empty string as image labels have no text!
    }

    Host& host = getHostFromLua(L);
    auto pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        return warnArgumentValue(L, __func__, qsl("areaID %1 does not exist").arg(areaId));
    }
    if (pA->mMapLabels.isEmpty()) {
        // Return an empty table:
        lua_newtable(L);
        return 1;
    }

    if (labelId >= 0) {
        if (!pA->mMapLabels.contains(labelId)) {
            return warnArgumentValue(L, __func__, qsl("labelID %1 does not exist in area with areaID %2")
                .arg(QString::number(labelId), QString::number(areaId)));
        }
        lua_newtable(L);
        auto label = pA->mMapLabels.value(labelId);
        pushMapLabelPropertiesToLua(L, label);
        return 1;
    }

    lua_newtable(L);
    QMapIterator<int, TMapLabel> it(pA->mMapLabels);
    while (it.hasNext()) {
        it.next();
        if (it.value().text == labelText) {
            lua_newtable(L);
            pushMapLabelPropertiesToLua(L, it.value());
            lua_pushnumber(L, it.key());
            lua_insert(L, -2);
            lua_settable(L, -3);
        }
    }
    return 1;
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addSpecialExit
int TLuaInterpreter::addSpecialExit(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int fromRoomID = getVerifiedInt(L, __func__, 1, "exit room id");
    TRoom* pR_from = host.mpMap->mpRoomDB->getRoom(fromRoomID);
    if (!pR_from) {
        return warnArgumentValue(L, __func__, qsl("exit room id %1 does not exist").arg(fromRoomID));
    }

    int toRoomID = getVerifiedInt(L, __func__, 2, "entrance room id");
    TRoom* pR_to = host.mpMap->mpRoomDB->getRoom(toRoomID);
    if (!pR_to) {
        return warnArgumentValue(L, __func__, qsl("entrance room id %1 does not exist").arg(toRoomID));
    }

    QString dir = getVerifiedString(L, __func__, 3, "special exit name/command");
    if (dir.isEmpty()) {
        return warnArgumentValue(L, __func__, "the special exit name/command cannot be empty");
    }

    pR_from->setSpecialExit(toRoomID, dir);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeSpecialExit
int TLuaInterpreter::removeSpecialExit(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int fromRoomID = getVerifiedInt(L, __func__, 1, "exit room id");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(fromRoomID);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("exit room id %1 does not exist").arg(fromRoomID));
    }

    QString dir = getVerifiedString(L, __func__, 2, "special exit name/command");
    if (dir.isEmpty()) {
        return warnArgumentValue(L, __func__, "the exit command cannot be empty");
    }

    if (!pR->getSpecialExits().contains(dir)) {
        return warnArgumentValue(L, __func__, qsl(
            "the special exit name/command '%1' does not exist in room id %2").arg(dir, QString::number(fromRoomID)));
    }
    pR->setSpecialExit(-1, dir);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearRoomUserData
int TLuaInterpreter::clearRoomUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int roomId = getVerifiedInt(L, __func__, 1, "room id");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(roomId));
    }
    if (!pR->userData.isEmpty()) {
        pR->userData.clear();
        host.mpMap->mUnsavedMap = true;
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearRoomUserDataItem
int TLuaInterpreter::clearRoomUserDataItem(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int roomId = getVerifiedInt(L, __func__, 1, "room id");
    QString key = getVerifiedString(L, __func__, 2, "key");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(roomId));
    }
    // Turns out that an empty key IS possible, but if this changes this should be uncommented
    //        if (key.isEmpty()) {
    //            // If the user accidentally supplied an white-space only or empty key
    //            // string we don't do anything, but we, successfully, fail to do it... 8-)
    //            lua_pushboolean( L, false );
    //        }
    /*      else */ if (pR->userData.contains(key)) {
        pR->userData.remove(key);
        host.mpMap->mUnsavedMap = true;
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearAreaUserData
int TLuaInterpreter::clearAreaUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int areaId = getVerifiedInt(L, __func__, 1, "area id");
    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid area id").arg(areaId));
    }
    if (!pA->mUserData.isEmpty()) {
        pA->mUserData.clear();
        host.mpMap->mUnsavedMap = true;
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearAreaUserDataItem
int TLuaInterpreter::clearAreaUserDataItem(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int areaId = getVerifiedInt(L, __func__, 1, "area id");
    QString key = getVerifiedString(L, __func__, 2, "key");
    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid area id").arg(areaId));
    }
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "key can not be an empty string");
    }
    lua_pushboolean(L, (pA->mUserData.remove(key) > 0));
    host.mpMap->mUnsavedMap = true;
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearMapUserData
int TLuaInterpreter::clearMapUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    if (!host.mpMap->mUserData.isEmpty()) {
        host.mpMap->mUserData.clear();
        host.mpMap->mUnsavedMap = true;
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
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QString key = getVerifiedString(L, __func__, 1, "key");
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "key can not be an empty string");
    }
    lua_pushboolean(L, (host.mpMap->mUserData.remove(key) > 0));
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearSpecialExits
int TLuaInterpreter::clearSpecialExits(lua_State* L)
{
    int id_from = getVerifiedInt(L, __func__, 1, "roomID");
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id_from);
    if (pR) {
        pR->clearSpecialExits();
    }
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getSpecialExits
// This function was slightly borked - the version in place from 2011 to 2020
// did not handle the corner case of multiple special exits that go to the same
// room, it would only show one of them at random. Each special exit was listed
// in its own table (against the key of the exit room id) and it is a key to a
// "1" or "0" depending on whether the exit is locked or not. This was not
// documented in the wiki!
int TLuaInterpreter::getSpecialExits(lua_State* L)
{
    Host& host = getHostFromLua(L);
    int id_from = getVerifiedInt(L, __func__, 1, "exit room id");
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id_from);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("room with id %1 does not exist").arg(id_from));
    }

    bool showAllExits = false;
    if (lua_gettop(L) > 1) {
        showAllExits = getVerifiedBool(L, __func__, 2, "show every exit to same exit room id", true);
    }

    QMapIterator<QString, int> itSpecialExit(pR->getSpecialExits());
    QMultiMap<int, QString> specialExitsByExitId;
    while (itSpecialExit.hasNext()) {
        itSpecialExit.next();
        specialExitsByExitId.insert(itSpecialExit.value(), itSpecialExit.key());
    }

    QList<int> exitRoomIdList = specialExitsByExitId.keys();
    lua_newtable(L);
    for (int i = 0, exitRoomIdCount = exitRoomIdList.count(); i < exitRoomIdCount; ++i) {
        lua_pushnumber(L, exitRoomIdList.at(i));
        lua_newtable(L);
        {
            QStringList exitCommandsToThisRoomId = specialExitsByExitId.values(exitRoomIdList.at(i));
            int bestUnlockedExitIndex = -1;
            int bestUnlockedExitWeight = -1;
            int bestLockedExitIndex = -1;
            int bestLockedExitWeight = -1;
            int exitCommandsCount = exitCommandsToThisRoomId.count();
            for (int j = 0; j < exitCommandsCount; ++j) {
                if (showAllExits || exitCommandsCount == 1) {
                    // The simpler case - show all exits (or the only exit) to
                    // this room:
                    lua_pushstring(L, exitCommandsToThisRoomId.at(j).toUtf8().constData());
                    lua_pushstring(L, pR->hasSpecialExitLock(exitCommandsToThisRoomId.at(j)) ? "1" : "0");
                    lua_settable(L, -3);
                    // Go on to next exit to this room:
                    continue;
                }

                // The more complex (but highly unlikely in most MUDs) case
                // - find the best exit to this room when there are more than
                // one:
                int thisExitWeight = pR->getExitWeight(exitCommandsToThisRoomId.at(j));
                if (pR->hasSpecialExitLock(exitCommandsToThisRoomId.at(j))) {
                    if (bestLockedExitIndex == -1) {
                        bestLockedExitIndex = j;
                        bestLockedExitWeight = thisExitWeight;
                    } else if (thisExitWeight < bestLockedExitWeight) {
                        bestLockedExitIndex = j;
                        bestLockedExitWeight = thisExitWeight;
                    }

                } else {
                    if (bestUnlockedExitIndex == -1) {
                        bestUnlockedExitIndex = j;
                        bestUnlockedExitWeight = thisExitWeight;
                    } else if (thisExitWeight < bestUnlockedExitWeight) {
                        bestUnlockedExitIndex = j;
                        bestUnlockedExitWeight = thisExitWeight;
                    }

                }
            }

            if (!showAllExits && (exitCommandsCount > 1)) {
                // Produce the best exit to this room given that there IS more
                // than one and we haven't been asked to show them all:
                int bestExitIndex = (bestUnlockedExitIndex != -1) ? bestUnlockedExitIndex : bestLockedExitIndex;
                lua_pushstring(L, exitCommandsToThisRoomId.at(bestExitIndex).toUtf8().constData());
                lua_pushstring(L, pR->hasSpecialExitLock(exitCommandsToThisRoomId.at(bestExitIndex)) ? "1" : "0");
                lua_settable(L, -3);
            }
        }
        lua_settable(L, -3);
    }

    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getSpecialExitsSwap
int TLuaInterpreter::getSpecialExitsSwap(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!lua_isnumber(L, 1)) {
        lua_pushfstring(L, "getSpecialExitsSwap: bad argument #1 type (exit room id as number expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    int id_from = lua_tointeger(L, 1);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id_from);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("room with id %1 does not exist").arg(id_from));
    }

    QMapIterator<QString, int> it(pR->getSpecialExits());
    lua_newtable(L);
    while (it.hasNext()) {
        it.next();
        lua_pushstring(L, it.key().toUtf8().constData());
        lua_pushnumber(L, it.value());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomEnv
int TLuaInterpreter::getRoomEnv(lua_State* L)
{
    int roomID = getVerifiedInt(L, __func__, 1, "roomID");
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
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int roomId = getVerifiedInt(L, __func__, 1, "room id");
    QString key = getVerifiedString(L, __func__, 2, "key");
    bool isBackwardCompatibilityRequired = true;
    if (lua_gettop(L) > 2) {
        isBackwardCompatibilityRequired = !getVerifiedBool(L, __func__, 3, "enableFullErrorReporting {default = false}", true);
    }

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        if (!isBackwardCompatibilityRequired) {
            return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(roomId));
        }
        lua_pushstring(L, "");
        return 1;
    }
    if (!pR->userData.contains(key)) {
        if (!isBackwardCompatibilityRequired) {
            return warnArgumentValue(L, __func__, qsl(
                "no user data with key '%1' in room with id %2").arg(key, QString::number(roomId)));
        }
        lua_pushstring(L, "");
        return 1;
    }
    lua_pushstring(L, pR->userData.value(key).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAreaUserData
int TLuaInterpreter::getAreaUserData(lua_State* L)
{
    int areaId = getVerifiedInt(L, __func__, 1, "area id");
    QString key = getVerifiedString(L, __func__, 2, "key");
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "key is not allowed to be an empty string");
    }

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }
    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid area id").arg(areaId));
    }
    if (!pA->mUserData.contains(key)) {
        return warnArgumentValue(L, __func__, qsl(
            "no user data with key '%1' in area with id %2").arg(key, QString::number(areaId)));
    }
    lua_pushstring(L, pA->mUserData.value(key).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapUserData
int TLuaInterpreter::getMapUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QString key = getVerifiedString(L, __func__, 1, "key");
    if (!host.mpMap->mUserData.contains(key)) {
        return warnArgumentValue(L, __func__, qsl("no user data with key '%1' in map").arg(key));
    }
    lua_pushstring(L, host.mpMap->mUserData.value(key).toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomUserData
int TLuaInterpreter::setRoomUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int roomId = getVerifiedInt(L, __func__, 1, "room id");
    QString key = getVerifiedString(L, __func__, 2, "key");
    // Ideally should reject empty keys but this could break existing scripts so we can't
    QString value = getVerifiedString(L, __func__, 3, "value");

    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(roomId));
    }
    pR->userData[key] = value;
    host.mpMap->mUnsavedMap = true;
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setAreaUserData
int TLuaInterpreter::setAreaUserData(lua_State* L)
{
    int areaId = getVerifiedInt(L, __func__, 1, "area id");
    QString key = getVerifiedString(L, __func__, 2, "key");
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "key is not allowed to be an empty string");
    }
    QString value = getVerifiedString(L, __func__, 3, "value");

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid area id").arg(areaId));
    }
    pA->mUserData[key] = value;
    host.mpMap->mUnsavedMap = true;
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMapUserData
int TLuaInterpreter::setMapUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QString key = getVerifiedString(L, __func__, 1, "key");
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "key is not allowed to be an empty string");
    }
    QString value = getVerifiedString(L, __func__, 2, "value");

    host.mpMap->mUserData[key] = value;
    host.mpMap->mUnsavedMap = true;
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomUserDataKeys
int TLuaInterpreter::getRoomUserDataKeys(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int roomId = getVerifiedInt(L, __func__, 1, "room id");

    QStringList keys;
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(roomId));
    }
    keys = pR->userData.keys();
    lua_newtable(L);
    for (int i = 0; i < keys.size(); i++) {
        lua_pushnumber(L, i + 1);
        lua_pushstring(L, keys.at(i).toUtf8().constData());
        lua_settable(L, -3);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAllRoomUserData
int TLuaInterpreter::getAllRoomUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int roomId = getVerifiedInt(L, __func__, 1, "room id");

    QStringList keys;
    QStringList values;
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(roomId);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(roomId));
    }
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAllAreaUserData
int TLuaInterpreter::getAllAreaUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int areaId = getVerifiedInt(L, __func__, 1, "area id");

    QStringList keys;
    QStringList values;
    TArea* pA = host.mpMap->mpRoomDB->getArea(areaId);
    if (!pA) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid area id").arg(areaId));
    }
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getAllMapUserData
int TLuaInterpreter::getAllMapUserData(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
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
    QString localFile = getVerifiedString(L, __func__, 1, "local filename");
    QString urlString = getVerifiedString(L, __func__, 2, "remote url");
    QUrl url = QUrl::fromUserInput(urlString);
    if (!url.isValid()) {
        return warnArgumentValue(L, __func__, qsl("url is invalid, reason: %1").arg(url.errorString()));
    }

    QNetworkRequest request = QNetworkRequest(url);
    mudlet::self()->setNetworkRequestDefaults(url, request);

    host.updateProxySettings(host.mLuaInterpreter.mpFileDownloader);
    QNetworkReply* reply = host.mLuaInterpreter.mpFileDownloader->get(request);
    host.mLuaInterpreter.downloadMap.insert(reply, localFile);
    connect(reply, &QNetworkReply::downloadProgress, [=](qint64 bytesDownloaded, qint64 totalBytes) {
        raiseDownloadProgressEvent(L, urlString, bytesDownloaded, totalBytes);
        if (mudlet::debugMode) {
            auto& lHost = getHostFromLua(L);
            TDebug(Qt::white, Qt::blue) << "downloadFile: " << bytesDownloaded << "/" << totalBytes
                                        << " bytes ready for " << reply->url().toString() << "\n" >> &lHost;
        }
    });

    if (mudlet::debugMode) {
        TDebug(Qt::white, Qt::blue) << "downloadFile: start download from " << reply->url().toString() << "\n" >> &host;
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
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int id = getVerifiedInt(L, __func__, 1, "room id");
    if (!host.mpMap->mpRoomDB->getRoomIDList().contains(id)) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(id));
    }

    int areaId;
    QString areaName;
    if (lua_isnumber(L, 2)) {
        areaId = lua_tonumber(L, 2);
        if (areaId < 1) {
            return warnArgumentValue(L, __func__, qsl(
                "number %1 is not a valid area id greater than zero. "
                "To remove a room's area, use resetRoomArea( roomId )").arg(areaId));
        }
        if (!host.mpMap->mpRoomDB->getAreaNamesMap().contains(areaId)) {
            return warnArgumentValue(L, __func__, qsl("area id %1 does not exist").arg(areaId));
        }
    } else if (lua_isstring(L, 2)) {
        areaName = lua_tostring(L, 2);
        // areaId will be zero if not found!
        if (areaName.isEmpty()) {
            return warnArgumentValue(L, __func__, "area name cannot be empty");
        }
        areaId = host.mpMap->mpRoomDB->getAreaNamesMap().key(areaName, 0);
        if (!areaId) {
            return warnArgumentValue(L, __func__, qsl("area name '%1' does not exist").arg(areaName));
        }
    } else {
        lua_pushfstring(L,
                        "setRoomArea: bad argument #2 type (area Id as number or area name as string\n"
                        "expected, got %s!)",
                        luaL_typename(L, 2));
        return lua_error(L);
    }

    // Can set the room to an area which does not have a TArea instance but does
    // appear in the TRoomDB::areaNamesMap...
    bool result = host.mpMap->setRoomArea(id, areaId, false);
    if (result) {
        // As a successful result WILL change the area a room is in then the map
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
    int id = getVerifiedInt(L, __func__, 1, "room id");

    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    } else if (!host.mpMap->mpRoomDB->getRoomIDList().contains(id)) {
        return warnArgumentValue(L, __func__, qsl("number %1 is not a valid room id").arg(id));
    }
    bool result = host.mpMap->setRoomArea(id, -1, false);
    if (result) {
        // As a successful result WILL change the area a room is in then the map
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomChar
int TLuaInterpreter::setRoomChar(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "room id");
    QString symbol = getVerifiedString(L, __func__, 2, "room symbol");

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("room with id %1 does not exist").arg(id));
    }

    if (symbol.isEmpty()) {
        // Allow an empty string to be used to clear the symbol:
        pR->mSymbol.clear();
    } else {
        // 8.0 is the maximum supported by the Qt versions (5.6 to 5.10) we
        // handle/use/allow:
        pR->mSymbol = symbol.normalized(QString::NormalizationForm_C, QChar::Unicode_8_0);
    }
    host.mpMap->mUnsavedMap = true;
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomChar
int TLuaInterpreter::getRoomChar(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "room id");
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("room with id %1 does not exist").arg(id));
    }

    lua_pushstring(L, pR->mSymbol.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setRoomCharColor
int TLuaInterpreter::setRoomCharColor(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "room id");
    int r = getVerifiedInt(L, __func__, 2, "red component");
    if (r < 0 || r > 255) {
        lua_pushfstring(L, "setRoomCharColor: bad argument #2 type (red component value %d out of range (0 to 255)", r);
        return lua_error(L);
    }
    int g = getVerifiedInt(L, __func__, 3, "green component");
    if (g < 0 || g > 255) {
        lua_pushfstring(L, "setRoomCharColor: bad argument #3 type (red component value %d out of range (0 to 255)", r);
        return lua_error(L);
    }
    int b = getVerifiedInt(L, __func__, 4, "blue component");
    if (b < 0 || b > 255) {
        lua_pushfstring(L, "setRoomCharColor: bad argument #4 type (blue component value %d out of range (0 to 255)", r);
        return lua_error(L);
    }

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("room with id %1 does not exist").arg(id));
    }

    pR->mSymbolColor = QColor(r, g, b);
    if (host.mpMap->mpMapper && host.mpMap->mpMapper->mp2dMap) {
        host.mpMap->mpMapper->mp2dMap->update();
    }
    host.mpMap->mUnsavedMap = true;
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#unsetRoomCharColor
int TLuaInterpreter::unsetRoomCharColor(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "room id");

    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("room with id %1 does not exist").arg(id));
    }

    // Reset it to the default (and invalid) QColor:
    pR->mSymbolColor = {};
    if (host.mpMap->mpMapper && host.mpMap->mpMapper->mp2dMap) {
        host.mpMap->mpMapper->mp2dMap->update();
    }
    host.mpMap->mUnsavedMap = true;
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomCharColor
int TLuaInterpreter::getRoomCharColor(lua_State* L)
{
    int id = getVerifiedInt(L, __func__, 1, "room id");
    Host& host = getHostFromLua(L);
    TRoom* pR = host.mpMap->mpRoomDB->getRoom(id);
    if (!pR) {
        return warnArgumentValue(L, __func__, qsl("room with id %1 does not exist").arg(id));
    }

    lua_pushnumber(L, pR->mSymbolColor.red());
    lua_pushnumber(L, pR->mSymbolColor.green());
    lua_pushnumber(L, pR->mSymbolColor.blue());
    return 3;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRoomsByPosition
int TLuaInterpreter::getRoomsByPosition(lua_State* L)
{
    int area = getVerifiedInt(L, __func__, 1, "areaID");
    int x = getVerifiedInt(L, __func__, 2, "x");
    int y = getVerifiedInt(L, __func__, 3, "y");
    int z = getVerifiedInt(L, __func__, 4, "z");

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
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    int id = getVerifiedInt(L, __func__, 1, "area id");

    TArea* area = host.mpMap->mpRoomDB->getArea(id);
    if (!area) {
        return warnArgumentValue(L, __func__, qsl("area with id %1 does not exist").arg(id));
    } else {
        lua_pushboolean(L, area->gridMode);
        return 1;
    }
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setGridMode
int TLuaInterpreter::setGridMode(lua_State* L)
{
    int area = getVerifiedInt(L, __func__, 1, "areaID");
    bool gridMode = getVerifiedBool(L, __func__, 2, "true/false");
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
    host.mpMap->mUnsavedMap = true;
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setFgColor
int TLuaInterpreter::setFgColor(lua_State* L)
{
    int s = 0;
    int n = lua_gettop(L);
    auto validRange = [](int number) { return number >= 0 && number <= 255; };
    QString windowName;
    if (n > 3) {
        windowName = WINDOW_NAME(L, ++s);
    }
    int luaRed = getVerifiedInt(L, __func__, ++s, "red component value");
    if (!validRange(luaRed)) {
        return warnArgumentValue(L, __func__, qsl("red value %1 needs to be between 0-255").arg(luaRed));
    }
    int luaGreen = getVerifiedInt(L, __func__, ++s, "green component value");
    if (!validRange(luaGreen)) {
        return warnArgumentValue(L, __func__, qsl("green value %1 needs to be between 0-255").arg(luaGreen));
    }
    int luaBlue = getVerifiedInt(L, __func__, ++s, "blue component value");
    if (!validRange(luaBlue)) {
        return warnArgumentValue(L, __func__, qsl("blue value %1 needs to be between 0-255").arg(luaBlue));
    }

    auto console = CONSOLE(L, windowName);
    console->setFgColor(luaRed, luaGreen, luaBlue);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setBgColor
int TLuaInterpreter::setBgColor(lua_State* L)
{
    QString windowName;
    int r, g, b, alpha;

    auto validRange = [](int number) { return number >= 0 && number <= 255; };

    int s = 1;
    if (lua_isstring(L, s) && !lua_isnumber(L, s)) {
        windowName = WINDOW_NAME(L, s);

        if (!lua_isnumber(L, ++s)) {
            lua_pushfstring(L, "setBgColor: bad argument #%d type (red value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
            return lua_error(L);
        }
        r = static_cast<int>(lua_tonumber(L, s));

        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, qsl("red value %1 needs to be between 0-255").arg(r));
        }
    } else if (lua_isnumber(L, s)) {
        r = static_cast<int>(lua_tonumber(L, s));

        if (!validRange(r)) {
            return warnArgumentValue(L, __func__, qsl("red value %1 needs to be between 0-255").arg(r));
        }
    } else {
        lua_pushfstring(L, "setBgColor: bad argument #%d type (window name as string, or red value 0-255 as number expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }

    g = getVerifiedInt(L, __func__, ++s, "green value 0-255");
    if (!validRange(g)) {
        return warnArgumentValue(L, __func__, qsl("green value %1 needs to be between 0-255").arg(g));
    }

    b = getVerifiedInt(L, __func__, ++s, "blue value 0-255");
    if (!validRange(b)) {
        return warnArgumentValue(L, __func__, qsl("blue value %1 needs to be between 0-255").arg(b));
    }

    // if we get nothing for the alpha value, assume it is 255. If we get a non-number value, complain.
    alpha = 255;
    if (lua_gettop(L) > s) {
        alpha = getVerifiedInt(L, __func__, ++s, "alpha value 0-255", true);
        if (!validRange(alpha)) {
            return warnArgumentValue(L, __func__, qsl("alpha value %1 needs to be between 0-255").arg(alpha));
        }
    }

    auto console = CONSOLE(L, windowName);
    console->setBgColor(r, g, b, alpha);
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#insertLink
int TLuaInterpreter::insertLink(lua_State* L)
{
    int n = lua_gettop(L);
    int funcRef{0};
    bool useCurrentFormat{false};
    QString windowName{qsl("main")};
    QString singleHint, singleFunction{QString()}, text;
    if (n < 4) {
        text = getVerifiedString(L, __func__, 1, "text");
        if (!(lua_isstring(L, 2) || lua_isfunction(L, 2))) {
            lua_pushfstring(L, "insertLink: bad argument #2 type (command as string or function expected, got %s!)", luaL_typename(L, 2));
            return lua_error(L);
        }
        if (lua_isfunction(L, 2)) {
            lua_pushvalue(L, 2);
            funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
        } else {
            singleFunction = lua_tostring(L, 2);
        }
        singleHint = getVerifiedString(L, __func__, 3, "hint");
    }
    if (n == 4) {
        bool isBool{false};
        if (lua_isboolean(L, 4)) {
            isBool = true;
            useCurrentFormat = lua_toboolean(L, 4);
        }
        if (isBool) {
            text = getVerifiedString(L, __func__, 1, "text");
            singleHint = getVerifiedString(L, __func__, 3, "hint");
            if (!(lua_isstring(L, 2) || lua_isfunction(L, 2))) {
                lua_pushfstring(L, "insertLink: bad argument #2 type (command as string or function expected, got %s!)", luaL_typename(L, 2));
                return lua_error(L);
            }
            if (lua_isfunction(L, 2)) {
                lua_pushvalue(L, 2);
                funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
            } else {
                singleFunction = lua_tostring(L, 2);
            }
        } else {
            windowName = getVerifiedString(L, __func__, 1, "window name");
            text = getVerifiedString(L, __func__, 2, "text");
            singleHint = getVerifiedString(L, __func__, 4, "hint");
            if (!(lua_isstring(L, 3) || lua_isfunction(L, 3))) {
                lua_pushfstring(L, "insertLink: bad argument #3 type (command as string or function expected, got %s!)", luaL_typename(L, 3));
                return lua_error(L);
            }
            if (lua_isfunction(L, 3)) {
                lua_pushvalue(L, 3);
                funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
            } else {
                singleFunction = lua_tostring(L, 3);
            }
        }
    }

    if (n > 4) {
        windowName = getVerifiedString(L, __func__, 1, "window name");
        text = getVerifiedString(L, __func__, 2, "text");
        singleHint = getVerifiedString(L, __func__, 4, "hint");
        useCurrentFormat = getVerifiedBool(L, __func__, 5, "useCurrentFormat");
        if (!(lua_isstring(L, 3) || lua_isfunction(L, 3))) {
            lua_pushfstring(L, "insertLink: bad argument #3 type (command as string or function expected, got %s!)", luaL_typename(L, 3));
            return lua_error(L);
        }
        if (lua_isfunction(L, 3)) {
            lua_pushvalue(L, 3);
            funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
        } else {
            singleFunction = lua_tostring(L, 3);
        }
    }


    QStringList function;
    QStringList hint;
    QVector<int> luaReference;
    function << singleFunction;
    hint << singleHint;
    luaReference << funcRef;

    auto console = CONSOLE(L, windowName);
    console->insertLink(text, function, hint, useCurrentFormat, luaReference);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#insertPopup
int TLuaInterpreter::insertPopup(lua_State* L)
{
    QString windowName;
    QStringList _hintList;
    QStringList _commandList;
    QVector<int> luaReference;
    bool customFormat = false;
    int s = 1;
    int n = lua_gettop(L);

    // console name is an optional first argument
    if (n >= 4) {
        windowName = WINDOW_NAME(L, s++);
    }
    QString txt = getVerifiedString(L, __func__, s++, "text");

    if (!lua_istable(L, s)) {
        lua_pushfstring(L, "insertPopup: bad argument #%d type (commands as table expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }
    lua_pushnil(L);
    while (lua_next(L, s) != 0) {
        // key at index -2 and value at index -1
        if (lua_type(L, -1) == LUA_TSTRING) {
            QString cmd = lua_tostring(L, -1);
            _commandList << cmd;
            luaReference << 0;
        }

        if (lua_type(L, -1) == LUA_TFUNCTION){
            lua_pushvalue(L, -1);
            _commandList << QString();
            luaReference << luaL_ref(L, LUA_REGISTRYINDEX);
        }
        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    if (!lua_istable(L, ++s)) {
        lua_pushfstring(L, "insertPopup: bad argument #%d type (hints as table expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }
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

    if (n >= ++s) {
        customFormat = lua_toboolean(L, s);
    }

    if (_commandList.size() != _hintList.size()) {
        lua_pushstring(L, "Error: command list size and hint list size do not match cannot create popup");
        return lua_error(L);
    }

    auto console = CONSOLE(L, windowName);
    console->insertLink(txt, _commandList, _hintList, customFormat, luaReference);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#insertText
int TLuaInterpreter::insertText(lua_State* L)
{
    QString windowName;
    int s = 0;

    if (lua_gettop(L) > 1) { // Have more than one argument so first must be a console name
        windowName = WINDOW_NAME(L, ++s);
    }
    QString text = getVerifiedString(L, __func__, ++s, "text");

    auto console = CONSOLE(L, windowName);
    console->insertText(text);
    lua_pushboolean(L, true);
    return 1;
}

// No Documentation - public function but should stay undocumented -- compare https://github.com/Mudlet/Mudlet/issues/1149
int TLuaInterpreter::insertHTML(lua_State* L)
{
    QString sendText = getVerifiedString(L, __func__, 1, "sendText");
    Host& host = getHostFromLua(L);
    host.mpConsole->insertHTML(sendText);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addSupportedTelnetOption
int TLuaInterpreter::addSupportedTelnetOption(lua_State* L)
{
    int option = getVerifiedInt(L, __func__, 1, "option");
    Host& host = getHostFromLua(L);
    host.mTelnet.supportedTelnetOptions[option] = true;
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echo
int TLuaInterpreter::echo(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QString consoleName;
    int n = lua_gettop(L);
    int s = 1;

    if (n > 1) {
        consoleName = getVerifiedString(L, __func__, s++, "console name", true);
    }

    QString displayText = getVerifiedString(L, __func__, s, "text to display");

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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echoPopup
int TLuaInterpreter::echoPopup(lua_State* L)
{
    QString windowName;
    QStringList hintList;
    QStringList commandList;
    QVector<int> luaReference;
    bool customFormat = false;
    int s = 1;
    int n = lua_gettop(L);
    // console name is an optional first argument
    if (n >= 4) {
        windowName = WINDOW_NAME(L, s++);
    }
    QString text = getVerifiedString(L, __func__, s++, "text as string");

    if (!lua_istable(L, s)) {
        lua_pushfstring(L, "echoPopup: bad argument #%d type (command list as table expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }
    lua_pushnil(L);
    while (lua_next(L, s) != 0) {
        // key at index -2 and value at index -1
        if (lua_type(L, -1) == LUA_TSTRING) {
            QString cmd = lua_tostring(L, -1);
            commandList << cmd;
            luaReference << 0;
        }
        if (lua_type(L, -1) == LUA_TFUNCTION){
            lua_pushvalue(L, -1);
            commandList << QString();
            luaReference << luaL_ref(L, LUA_REGISTRYINDEX);
        }
        // removes value, but keeps key for next iteration
        lua_pop(L, 1);
    }

    if (!lua_istable(L, ++s)) {
        lua_pushfstring(L, "echoPopup: bad argument #%d type (hint list as table expected, got %s!)", s, luaL_typename(L, s));
        return lua_error(L);
    }
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

    if (n >= ++s) {
        customFormat = lua_toboolean(L, s);
    }

    if (commandList.size() != hintList.size()) {
        lua_pushfstring(L, "echoPopup: commands and hints list aren't the same size");
        return lua_error(L);
    }

    auto console = CONSOLE(L, windowName);
    console->echoLink(text, commandList, hintList, customFormat, luaReference);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#echoLink
int TLuaInterpreter::echoLink(lua_State* L)
{
    int n = lua_gettop(L);
    int funcRef{0};
    bool useCurrentFormat{false};
    QString windowName{qsl("main")};
    QString singleHint, singleFunction{QString()}, text;
    if (n < 4) {
        text = getVerifiedString(L, __func__, 1, "text");
        singleHint = getVerifiedString(L, __func__, 3, "hint");
        if (!(lua_isstring(L, 2) || lua_isfunction(L, 2))) {
            lua_pushfstring(L, "echoLink: bad argument #2 type (command as string or function expected, got %s!)", luaL_typename(L, 2));
            return lua_error(L);
        }
        if (lua_isfunction(L, 2)) {
            lua_pushvalue(L, 2);
            funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
        } else {
            singleFunction = lua_tostring(L, 2);
        }
    }
    if (n == 4) {
        bool isBool{false};
        if (lua_isboolean(L, 4)) {
            isBool = true;
            useCurrentFormat = lua_toboolean(L, 4);
        }
        if (isBool) {
            text = getVerifiedString(L, __func__, 1, "text");
            singleHint = getVerifiedString(L, __func__, 3, "hint");
            if (!(lua_isstring(L, 2) || lua_isfunction(L, 2))) {
                lua_pushfstring(L, "echoLink: bad argument #2 type (command as string or function expected, got %s!)", luaL_typename(L, 2));
                return lua_error(L);
            }
            if (lua_isfunction(L, 2)) {
                lua_pushvalue(L, 2);
                funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
            } else {
                singleFunction = lua_tostring(L, 2);
            }
        } else {
            windowName = getVerifiedString(L, __func__, 1, "window name");
            text = getVerifiedString(L, __func__, 2, "text");
            singleHint = getVerifiedString(L, __func__, 4, "hint");
            if (!(lua_isstring(L, 3) || lua_isfunction(L, 3))) {
                lua_pushfstring(L, "echoLink: bad argument #3 type (command as string or function expected, got %s!)", luaL_typename(L, 3));
                return lua_error(L);
            }
            if (lua_isfunction(L, 3)) {
                lua_pushvalue(L, 3);
                funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
            } else {
                singleFunction = lua_tostring(L, 3);
            }
        }
    }

    if (n > 4) {
        windowName = getVerifiedString(L, __func__, 1, "window name");
        text = getVerifiedString(L, __func__, 2, "text");
        singleHint = getVerifiedString(L, __func__, 4, "hint");
        useCurrentFormat = getVerifiedBool(L, __func__, 5, "useCurrentFormat");
        if (!(lua_isstring(L, 3) || lua_isfunction(L, 3))) {
            lua_pushfstring(L, "echoLink: bad argument #3 type (command as string or function expected, got %s!)", luaL_typename(L, 3));
            return lua_error(L);
        }
        if (lua_isfunction(L, 3)) {
            lua_pushvalue(L, 3);
            funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
        } else {
            singleFunction = lua_tostring(L, 3);
        }
    }

    QStringList function;
    QStringList hint;
    QVector<int> luaReference;
    function << singleFunction;
    hint << singleHint;
    luaReference << funcRef;

    auto console = CONSOLE(L, windowName);
    console->echoLink(text, function, hint, useCurrentFormat, luaReference);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMergeTables
int TLuaInterpreter::setMergeTables(lua_State* L)
{
    Host& host = getHostFromLua(L);

    QStringList modulesList;
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++) {
        modulesList << getVerifiedString(L, __func__, i, "module");
    }

    host.mGMCP_merge_table_keys = host.mGMCP_merge_table_keys + modulesList;
    host.mGMCP_merge_table_keys.removeDuplicates();

    return 0;
}

// No Documentation - public function but should stay undocumented -- compare https://github.com/Mudlet/Mudlet/issues/1149
int TLuaInterpreter::pasteWindow(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "pasteWindow: bad argument #1 type (window name as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QString windowName {WINDOW_NAME(L, 1)};
    Host& host = getHostFromLua(L);
    host.pasteWindow(windowName);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#openUrl
int TLuaInterpreter::openUrl(lua_State* L)
{
    QString url = getVerifiedString(L, __func__, 1, "url");
    QDesktopServices::openUrl(url);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setLabelStyleSheet
int TLuaInterpreter::setLabelStyleSheet(lua_State* L)
{
    std::string label = getVerifiedString(L, __func__, 1, "label").toStdString();
    std::string markup = getVerifiedString(L, __func__, 2, "markup").toStdString();
    Host& host = getHostFromLua(L);
    host.mpConsole->setLabelStyleSheet(label, markup);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getLabelStyleSheet
int TLuaInterpreter::getLabelStyleSheet(lua_State* L)
{
    QString label = getVerifiedString(L, __func__, 1, "label");
    Host& host = getHostFromLua(L);
    if (auto stylesheet = host.mpConsole->getLabelStyleSheet(label)) {
        lua_pushstring(L, stylesheet->toUtf8().constData());
        return 1;
    }

    lua_pushnil(L);
    lua_pushfstring(L, "label '%s' does not exist", label.toUtf8().constData());
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setUserWindowStyleSheet
int TLuaInterpreter::setUserWindowStyleSheet(lua_State* L)
{
    QString userWindowName = getVerifiedString(L, __func__, 1, "userwindow name");
    QString userWindowStyleSheet = getVerifiedString(L, __func__, 2, "StyleSheet");
    Host& host = getHostFromLua(L);

    if (auto [success, message] = host.mpConsole->setUserWindowStyleSheet(userWindowName, userWindowStyleSheet); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCustomEnvColorTable
int TLuaInterpreter::getCustomEnvColorTable(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap->mCustomEnvColors.empty()) {
        lua_newtable(L);
        QList<int> colorList = host.mpMap->mCustomEnvColors.keys();
        for (auto idx : colorList) {
            lua_pushnumber(L, idx);
            lua_newtable(L);
            // red component
            {
                lua_pushnumber(L, 1);
                lua_pushnumber(L, host.mpMap->mCustomEnvColors.value(idx).red());
                lua_settable(L, -3);
            }
            // green component
            {
                lua_pushnumber(L, 2);
                lua_pushnumber(L, host.mpMap->mCustomEnvColors.value(idx).green());
                lua_settable(L, -3);
            }
            // blue component
            {
                lua_pushnumber(L, 3);
                lua_pushnumber(L, host.mpMap->mCustomEnvColors.value(idx).blue());
                lua_settable(L, -3);
            }
            // alpha component
            {
                lua_pushnumber(L, 4);
                lua_pushnumber(L, host.mpMap->mCustomEnvColors.value(idx).alpha());
                lua_settable(L, -3);
            }
            lua_settable(L, -3);
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

    int n = lua_gettop(L);

    if (n == 1) {
        QString tidiedWhat = getVerifiedString(L, __func__, 1, "style", true).toLower().trimmed();
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
        lua_pushstring(L, QByteArray(APP_BUILD).trimmed().data());
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
    QString url = getVerifiedString(L, __func__, 1, "URL");
    lua_pushboolean(L, mudlet::self()->openWebPage(url));
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
    QString inputText = getVerifiedString(L, __func__, 1, "id").trimmed();
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
    quint64 numericEquivalent = inputText.toULongLong(&isOk);
    if (numericEquivalent && isOk) {
        QString appID = QString::number(numericEquivalent);
        if (pMudlet->mDiscord.setApplicationID(&host, appID)) {
            lua_pushboolean(L, true);
            return 1;
        }
        return warnArgumentValue(L, __func__, qsl("'%1' does not appear to be a valid Discord application id").arg(inputText));
    }
    return warnArgumentValue(L, __func__, qsl("'%1' can not be converted to the expected numeric Discord application id").arg(inputText));
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
    bool isActiveHost = (pMudlet->mpCurrentActiveHost == &host);
    int args = lua_gettop(L);

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

    QString gamename = getVerifiedString(L, __func__, 1, "game name");
    pMudlet->mDiscord.setDetailText(&host, tr("Playing %1").arg(gamename));
    pMudlet->mDiscord.setLargeImage(&host, gamename.toLower());
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

    int64_t timeStamp = getVerifiedInt(L, __func__, 1, "epoch time");
    if (timeStamp < 0) {
        return warnArgumentValue(L, __func__, "the timestamp must be zero to clear the 'elapsed:' time or an epoch time value from the recent past");
    }
    pMudlet->mDiscord.setStartTimeStamp(&host, timeStamp);
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

    int64_t timeStamp = getVerifiedInt(L, __func__, 1, "epoch time");

    if (timeStamp < 0) {
        return warnArgumentValue(L, __func__, "the timestamp must be zero to clear the 'remaining:' time or an epoch time value in the recent future");
    }
    pMudlet->mDiscord.setEndTimeStamp(&host, timeStamp);
    lua_pushboolean(L, true);
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
        return warnArgumentValue(L, __func__, result.second);
    } else if (!(host.mDiscordAccessFlags & Host::DiscordSetPartyInfo)) {
        return warnArgumentValue(L, __func__, "access to Discord party info is disabled in settings for privacy");
    }

    int64_t partySize = getVerifiedInt(L, __func__, 1, "current party size");
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

    QPair<int, int> partyValues = pMudlet->mDiscord.getParty(&host);
    lua_pushnumber(L, partyValues.first);
    lua_pushnumber(L, partyValues.second);
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getTime
int TLuaInterpreter::getTime(lua_State* L)
{
    int n = lua_gettop(L);
    bool return_string = false;
    QString format = qsl("yyyy.MM.dd hh:mm:ss.zzz");
    QString tm;
    if (n > 0) {
        return_string = getVerifiedBool(L, __func__, 1, "return as string", true);
        if (n > 1) {
            format = getVerifiedString(L, __func__, 2, "custom time format");
        }
    }
    QDateTime time = QDateTime::currentDateTime();
    if (return_string) {
        tm = time.toString(format);
        lua_pushstring(L, tm.toUtf8().constData());
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
    QString windowName {WINDOW_NAME(L, 1)};
    auto console = CONSOLE(L, windowName);
    console->appendBuffer();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#appendCmdLine
int TLuaInterpreter::appendCmdLine(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = "main";

    if (n > 1) {
        name = CMDLINE_NAME(L, 1);
    }
    QString text = getVerifiedString(L, __func__, n, "text to set on command line");
    auto pN = COMMANDLINE(L, name);

    QString curText = pN->toPlainText();
    pN->setPlainText(curText + text);
    QTextCursor cur = pN->textCursor();
    cur.clearSelection();
    cur.movePosition(QTextCursor::EndOfLine);
    pN->setTextCursor(cur);
    return 0;
}


// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#selectCmdLineText
int TLuaInterpreter::selectCmdLineText(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = "main";
    if (n >= 1) {
        name = CMDLINE_NAME(L, 1);
    }
    auto commandline = COMMANDLINE(L, name);
    commandline->selectAll();
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCmdLine
int TLuaInterpreter::getCmdLine(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = "main";
    if (n >= 1) {
        name = CMDLINE_NAME(L, 1);
    }
    auto commandline = COMMANDLINE(L, name);
    QString text = commandline->toPlainText();
    lua_pushstring(L, text.toUtf8().constData());
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addCmdLineSuggestion
int TLuaInterpreter::addCmdLineSuggestion(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = "main";
    if (n > 1) {
        name = CMDLINE_NAME(L, 1);
    }
    QString text = getVerifiedString(L, __func__, n, "suggestion text");
    auto pN = COMMANDLINE(L, name);
    pN->addSuggestion(text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeCmdLineSuggestion
int TLuaInterpreter::removeCmdLineSuggestion(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = "main";
    if (n > 1) {
        name = CMDLINE_NAME(L, 1);
    }
    QString text = getVerifiedString(L, __func__, n, "suggestion text");
    auto pN = COMMANDLINE(L, name);
    pN->removeSuggestion(text);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearCmdLineSuggestions
int TLuaInterpreter::clearCmdLineSuggestions(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = "main";
    if (n == 1) {
        name = CMDLINE_NAME(L, 1);
    }
    auto pN = COMMANDLINE(L, name);
    pN->clearSuggestions();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#installPackage
int TLuaInterpreter::installPackage(lua_State* L)
{
    QString location = getVerifiedString(L, __func__, 1, "package location path and file name");
    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.installPackage(location, 0); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#uninstallPackage
int TLuaInterpreter::uninstallPackage(lua_State* L)
{
    QString packageName = getVerifiedString(L, __func__, 1, "package name");
    Host& host = getHostFromLua(L);
    host.uninstallPackage(packageName, 0);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#installModule
int TLuaInterpreter::installModule(lua_State* L)
{
    QString modName = getVerifiedString(L, __func__, 1, "module location");
    Host& host = getHostFromLua(L);
    QString module = QDir::fromNativeSeparators(modName);

    if (auto [success, message] = host.installPackage(module, 3); !success) {
        return warnArgumentValue(L, __func__, message);
    }
    auto moduleManager = host.mpModuleManager;
    if (moduleManager && moduleManager->mModuleTable->isVisible()) {
        moduleManager->layoutModules();
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#uninstallModule
int TLuaInterpreter::uninstallModule(lua_State* L)
{
    QString module = getVerifiedString(L, __func__, 1, "module name");
    Host& host = getHostFromLua(L);
    if (!host.uninstallPackage(module, 3)) {
        lua_pushboolean(L, false);
        return 1;
    }
    auto moduleManager = host.mpModuleManager;
    if (moduleManager && moduleManager->mModuleTable->isVisible()) {
        moduleManager->layoutModules();
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#reloadModule
int TLuaInterpreter::reloadModule(lua_State* L)
{
    QString module = getVerifiedString(L, __func__, 1, "module name");
    Host& host = getHostFromLua(L);
    host.reloadModule(module);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableModuleSync
int TLuaInterpreter::enableModuleSync(lua_State* L)
{
    QString module = getVerifiedString(L, __func__, 1, "module name");
    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.changeModuleSync(module, QLatin1String("1")); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    auto moduleManager = host.mpModuleManager;
    if (moduleManager && !moduleManager->mModuleTable->findItems(module, Qt::MatchExactly).isEmpty()) {
        int row = moduleManager->mModuleTable->findItems(module, Qt::MatchExactly)[0]->row();
        auto checkItem = moduleManager->mModuleTable->item(row, 2);
        checkItem->setCheckState(Qt::Checked);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableModuleSync
int TLuaInterpreter::disableModuleSync(lua_State* L)
{
    QString module = getVerifiedString(L, __func__, 1, "module name");
    Host& host = getHostFromLua(L);
    if (auto [success, message] = host.changeModuleSync(module, QLatin1String("0")); !success) {
        return warnArgumentValue(L, __func__, message);
    }

    auto moduleManager = host.mpModuleManager;
    if (moduleManager && !moduleManager->mModuleTable->findItems(module, Qt::MatchExactly).isEmpty()) {
        int row = moduleManager->mModuleTable->findItems(module, Qt::MatchExactly)[0]->row();
        auto checkItem = moduleManager->mModuleTable->item(row, 2);
        checkItem->setCheckState(Qt::Unchecked);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getModuleSync
int TLuaInterpreter::getModuleSync(lua_State* L)
{
    QString module = getVerifiedString(L, __func__, 1, "module name");
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
    Host& host = getHostFromLua(L);
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
    Host& host = getHostFromLua(L);
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
    Host& host = getHostFromLua(L);
    auto infoMap = host.mModuleInfo;
    int n = lua_gettop(L);
    QString name = getVerifiedString(L, __func__, 1, "module name");
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
    Host& host = getHostFromLua(L);
    auto infoMap = host.mPackageInfo;
    int n = lua_gettop(L);
    QString name = getVerifiedString(L, __func__, 1, "package name");
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
  QString moduleName = getVerifiedString(L, __func__, 1, "module name");
  QString info = getVerifiedString(L, __func__, 2, "info");
  QString value = getVerifiedString(L, __func__, 3, "value");
  host.mModuleInfo[moduleName][info] = value;
  lua_pushboolean(L, true);
  return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setPackageInfo
int TLuaInterpreter::setPackageInfo(lua_State* L)
{
  Host& host = getHostFromLua(L);
  QString packageName = getVerifiedString(L, __func__, 1, "package name");
  QString info = getVerifiedString(L, __func__, 2, "info");
  QString value = getVerifiedString(L, __func__, 3, "value");
  host.mPackageInfo[packageName][info] = value;
  lua_pushboolean(L, true);
  return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setDefaultAreaVisible
int TLuaInterpreter::setDefaultAreaVisible(lua_State* L)
{
    Host& host = getHostFromLua(L);
    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    bool isToShowDefaultArea = getVerifiedBool(L, __func__, 1, "isToShowDefaultArea");
    if (host.mpMap->mpMapper) {
        // If we are re-enabling the display of the default area
        // AND the mapper was showing the default area
        // the area widget will NOT be showing the correct area name afterwards
        bool isAreaWidgetInNeedOfResetting = false;
        if ((!host.mpMap->mpMapper->getDefaultAreaShown()) && (isToShowDefaultArea) && (host.mpMap->mpMapper->mp2dMap->mAreaID == -1)) {
            isAreaWidgetInNeedOfResetting = true;
        }

        host.mpMap->mpMapper->setDefaultAreaShown(isToShowDefaultArea);
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
    QString event = getVerifiedString(L, __func__, 1, "event name");
    QString func = getVerifiedString(L, __func__, 2, "function name");
    Host& host = getHostFromLua(L);
    host.registerAnonymousEventHandler(event, func);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#expandAlias
int TLuaInterpreter::expandAlias(lua_State* L)
{
    QString payload = getVerifiedString(L, __func__, 1, "text to parse");
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#printCmdLine
int TLuaInterpreter::printCmdLine(lua_State* L)
{
    int n = lua_gettop(L);
    QString name = "main";
    if (n > 1) {
        name = CMDLINE_NAME(L, 1);
    }
    QString text = getVerifiedString(L, __func__, n, "text to set on command line");

    auto pN = COMMANDLINE(L, name);
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
    if (n > 1) {
        name = CMDLINE_NAME(L, 1);
    }
    auto pN = COMMANDLINE(L, name);
    pN->clear();
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#send
// Note this is registered as send NOT sendRaw - see initLuaGlobals()
// It converts the bytes in the command (the first argument) from Utf-8 to be
// encoded in the required Mud Server encoding.
int TLuaInterpreter::sendRaw(lua_State* L)
{
    QString text = getVerifiedString(L, __func__, 1, "command");
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
// The data can, theoretically, contain embedded ASCII NUL characters:
int TLuaInterpreter::sendSocket(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "sendSocket: bad argument #1 type (data as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    std::string data = lua_tostring(L, 1);

    Host& host = getHostFromLua(L);
    // msg is not in an encoded form here it is a literal set of bytes, which
    // is what this usage needs:
    host.mTelnet.socketOutRaw(data);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#sendIrc
int TLuaInterpreter::sendIrc(lua_State* L)
{
    QString target = getVerifiedString(L, __func__, 1, "target");
    QString msg = getVerifiedString(L, __func__, 2, "message");

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

    QPair<bool, QString> result = pHost->mpDlgIRC->sendMsg(target, msg);

    if (!result.first) {
        return warnArgumentValue(L, __func__, result.second.toUtf8().constData());
    }

    lua_pushboolean(L, true);
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setIrcNick
int TLuaInterpreter::setIrcNick(lua_State* L)
{
    QString nick = getVerifiedString(L, __func__, 1, "nick");
    if (nick.isEmpty()) {
        return warnArgumentValue(L, __func__, "nick must not be empty");
    }

    Host* pHost = &getHostFromLua(L);
    QPair<bool, QString> result = dlgIRC::writeIrcNickName(pHost, nick);
    if (!result.first) {
        return warnArgumentValue(L, __func__, qsl("unable to save nick name, reason: %1").arg(result.second));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setIrcServer
int TLuaInterpreter::setIrcServer(lua_State* L)
{
    int args = lua_gettop(L);
    int secure = false;
    int port = 6667;
    QString password;
    std::string addr = getVerifiedString(L, __func__, 1, "hostname").toStdString();
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
            QString c = lua_tostring(L, -1);
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
    QPair<bool, QString> result = dlgIRC::writeIrcChannels(pHost, newchannels);
    if (!result.first) {
        return warnArgumentValue(L, __func__, qsl("unable to save channels, reason: %1").arg(result.second));
    }

    lua_pushboolean(L, true);
    return 1;
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

#ifdef QT_TEXTTOSPEECH_LIB

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSpeak
int TLuaInterpreter::ttsSpeak(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    QString textToSay = getVerifiedString(L, __func__, 1, "text to say").trimmed();
    if (textToSay.isEmpty()) { // there's nothing more to say. discussion: https://github.com/Mudlet/Mudlet/issues/4688
        return warnArgumentValue(L, __func__, qsl("skipped empty text to speak (TTS)"));
    }

    std::vector<QString> dontSpeak = {"<", ">", "&lt;", "&gt;"}; // discussion: https://github.com/Mudlet/Mudlet/issues/4689
    for (const QString& dropThis : dontSpeak) {
        if (textToSay.contains(dropThis)) {
            textToSay.replace(dropThis, QString());
            if (mudlet::debugMode) {
                auto& host = getHostFromLua(L);
                TDebug(Qt::white, Qt::darkGreen) << "LUA: removed angle-shaped brackets (<>) from text to speak (TTS)\n" >> &host;
            }
        }
    }

    speechUnit->say(textToSay);
    speechCurrent = textToSay;
    return 0;
}

// No documentation available in wiki - internal function
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
    Q_UNUSED(L)
    TLuaInterpreter::ttsBuild();

    speechUnit->stop();

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsSetRate
int TLuaInterpreter::ttsSetRate(lua_State* L)
{
    TLuaInterpreter::ttsBuild();
    double rate = getVerifiedDouble(L, __func__, 1, "rate");

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
    double pitch = getVerifiedDouble(L, __func__, 1, "pitch");

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
    double volume = getVerifiedDouble(L, __func__, 1, "volume");

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
    QString nextVoice = getVerifiedString(L, __func__, 1, "voice");

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
    int index = getVerifiedInt(L, __func__, 1, "voice as index number");
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

// No documentation available in wiki - internal function
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
    QString inputText = getVerifiedString(L, __func__, 1, "input").trimmed();
    if (inputText.isEmpty()) { // there's nothing more to say. discussion: https://github.com/Mudlet/Mudlet/issues/4688
        return warnArgumentValue(L, __func__, qsl("skipped empty text to speak (TTS)"));
    }

    std::vector<QString> dontSpeak = {"<", ">", "&lt;", "&gt;"}; // discussion: https://github.com/Mudlet/Mudlet/issues/4689
    for (const QString& dropThis : dontSpeak) {
        if (inputText.contains(dropThis)) {
            inputText.replace(dropThis, QString());
            if (mudlet::debugMode) {
                auto& host = getHostFromLua(L);
                TDebug(Qt::white, Qt::darkGreen) << "LUA: removed angle-shaped brackets (<>) from text to speak (TTS)\n" >> &host;
            }
        }
    }

    int index;
    if (lua_gettop(L) > 1) {
        index = getVerifiedInt(L, __func__, 2, "index");
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
        int index = getVerifiedInt(L, __func__, 1, "index");
        index--;
        if (index < 0 || index > speechQueue.size()) {
            lua_pushboolean(L, false);
            return 1;
        }

        lua_pushstring(L, speechQueue.at(index).toUtf8().constData());
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
    Q_UNUSED(L)
    TLuaInterpreter::ttsBuild();

    speechUnit->pause();

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsResume
int TLuaInterpreter::ttsResume(lua_State* L)
{
    Q_UNUSED(L)
    TLuaInterpreter::ttsBuild();

    speechUnit->resume();

    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#ttsClearQueue
int TLuaInterpreter::ttsClearQueue(lua_State* L)
{
    TLuaInterpreter::ttsBuild();

    if (lua_gettop(L) > 0) {
        int index = getVerifiedInt(L, __func__, 1, "index");
        index--;
        if (index < 0 || index >= speechQueue.size()) {
            return warnArgumentValue(L, __func__, qsl("index %1 out of bounds for queue size %2").arg(index + 1, speechQueue.size()));
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
        return warnArgumentValue(L, __func__, "not speaking any text");
    } else if (speechUnit->state() == QTextToSpeech::BackendError) {
        return warnArgumentValue(L, __func__, "error with the computer's TTS engine");
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

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "setServerEncoding: bad argument #1 type (newEncoding as string expected, got %s!)", luaL_typename(L, 1));
        return lua_error(L);
    }
    QByteArray newEncoding = lua_tostring(L, 1);
    QPair<bool, QString> results = host.mTelnet.setEncoding(newEncoding);

    if (!results.first) {
        return warnArgumentValue(L, __func__, results.second);
    }

    lua_pushboolean(L, true);
    return 1;
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
    lua_pushstring(L, QSysInfo::productVersion().toUtf8().constData());
    return 2;
#elif defined(Q_OS_WIN32)
    lua_pushstring(L, "windows");
    lua_pushstring(L, QSysInfo::productVersion().toUtf8().constData());
    return 2;
#elif defined(Q_OS_MACOS)
    lua_pushstring(L, "mac");
    lua_pushstring(L, QSysInfo::productVersion().toUtf8().constData());
    return 2;
#elif defined(Q_OS_LINUX)
    lua_pushstring(L, "linux");
    lua_pushstring(L, QSysInfo::productVersion().toUtf8().constData());
    lua_pushstring(L, QSysInfo::productType().toUtf8().constData());
    return 3;
#elif defined(Q_OS_HURD)
    lua_pushstring(L, "hurd");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2;
#elif defined(Q_OS_FREEBSD)
    // Only defined on FreeBSD but NOT Debian kFreeBSD so we should check for
    // this first
    lua_pushstring(L, "freebsd");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2;
#elif defined(Q_OS_FREEBSD_KERNEL)
    // Defined for BOTH Debian kFreeBSD hybrid with a GNU userland and
    // main FreeBSD so it must be after Q_OS_FREEBSD check; included for Debian
    // packager who may want to have this!
    lua_pushstring(L, "kfreebsd");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2;
#elif defined(Q_OS_OPENBSD)
    lua_pushstring(L, "openbsd");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2;
#elif defined(Q_OS_NETBSD)
    lua_pushstring(L, "netbsd");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2;
#elif defined(Q_OS_BSD4)
    // Generic *nix - must be before unix and after other more specific results
    lua_pushstring(L, "bsd4");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2;
#elif defined(Q_OS_UNIX)
    // Most generic *nix - must be after bsd4 and other more specific results
    lua_pushstring(L, "unix");
    lua_pushstring(L, QSysInfo::kernelVersion().toUtf8().constData());
    return 2;
#else
    lua_pushstring(L, "unknown");
    lua_pushstring(L, "unknown");
    return 2;
#endif
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
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(getVerifiedString(L, __func__, 1, "text"));
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

    int error = luaL_dostring(L, code.toUtf8().constData());
    if (error) {
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

    QString thing = QString(R"(return get_formatted_code(get_ast("%1"), {indent_chunk = '  ', right_margin = 100, max_text_width = 160, keep_comments = true}))").arg(escapedCode);
    int error = luaL_dostring(L, thing.toUtf8().constData());
    if (error) {
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
bool TLuaInterpreter::compile(const QString& code, QString& errorMsg, const QString& name)
{
    lua_State* L = pGlobalLua;

    int error = (luaL_loadbuffer(L, code.toUtf8().constData(),
                                 strlen(code.toUtf8().constData()),
                                 name.toUtf8().constData()) || lua_pcall(L, 0, 0, 0));

    if (error) {
        std::string e = "Lua syntax error:";
        if (lua_isstring(L, 1)) {
            e.append(lua_tostring(L, 1));
        }
        errorMsg = "<b><font color='blue'>";
        errorMsg.append(e.c_str());
        errorMsg.append("</font></b>");
        if (mudlet::debugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::red) << "\n " << e.c_str() << "\n" >> &host;
        }
    } else {
        if (mudlet::debugMode) {
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
    QString script{lua_tostring(L, index)};
    return validLuaCode(script);
}

// No documentation available in wiki - internal function
// returns pair where first is bool stating true the given Lua code is valid, false otherwise
// second is empty if code is valid, error message if not valid
std::pair<bool, QString> TLuaInterpreter::validLuaCode(const QString &code)
{
    lua_State* L = pGlobalLua;
    int error = luaL_loadbuffer(L, code.toUtf8().constData(), strlen(code.toUtf8().constData()), code.toUtf8().data());
    int topElementIndex = lua_gettop(L);
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
        QString msg = qsl("\n%1 event <%2> display(%1) to see the full content\n").arg("mxp", token);
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
            QString msg = qsl("\n%1 event <%2> display(%1) to see the full content\n").arg(protocol, token);
            host.mpConsole->printSystemMessage(msg);
        }
        host.raiseEvent(event);
    }
    // auto-detect IRE composer
    if (tokenList.size() == 3 && tokenList.at(0).toLower() == "ire" && tokenList.at(1).toLower() == "composer" && tokenList.at(2).toLower() == "edit") {
        QRegularExpression rx(qsl(R"lit(\{ ?"title": ?"(.*)", ?"text": ?"(.*)" ?\})lit"));
        QRegularExpressionMatch match = rx.match(string_data);

        if (match.capturedStart() != -1) {
            QString title = match.captured(1);
            QString initialText = match.captured(2);
            QRegularExpression codeRegex(qsl(R"lit(\\n|\\t|\\"|\\\\|\\u[0-9a-cA-C][0-9a-fA-F]{3}|\\u[dD][0-7][0-9a-fA-F]{2}|\\u[efEF][0-9a-fA-F]{3}|\\u[dD][89abAB][0-9a-fA-F]{2}\\u[dD][c-fC-F][0-9a-fA-F]{2})lit"));
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

    // string_data is in the format of MSSP_VAR "PLAYERS" MSSP_VAL "52" MSSP_VAR "UPTIME" MSSP_VAL "1234567890"
    // The quote characters mean that the encased word is a string, the quotes themselves are not sent.
    QStringList packageList = string_data.split(MSSP_VAR);

    if (packageList.size() > 0) {
        Host& host = getHostFromLua(L);

        for (int i = 1; i < packageList.size(); i++) {
            // clear the stack to avoid it getting to big
            lua_settop(L, 0);

            QStringList payloadList = packageList[i].split(MSSP_VAL);

            if (payloadList.size() != 2) {
                continue;
            }

            QString msspVAR = payloadList[0];
            QString msspVAL = payloadList[1];

            lua_getglobal(L, "mssp");
            lua_pushstring(L, msspVAR.toUtf8().constData());
            lua_pushlstring(L, msspVAL.toUtf8().constData(), msspVAL.toUtf8().length());

            lua_rawset(L, -3);

            // Raise an event
            QString protocol = qsl("mssp");
            QString token = protocol;
            token.append(".");
            token.append(msspVAR);

            TEvent event{};
            event.mArgumentList.append(token);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            event.mArgumentList.append(token);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
            if (mudlet::debugMode) {
                QString msg = qsl("\n%1 event <%2> display(%1) to see the full content\n").arg(protocol, token);
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
        for (auto [name, capture] : mCapturedNameGroups) {
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
    lua_State* L = pGlobalLua;
    lua_pushlightuserdata(L, pT);
    lua_gettable(L, LUA_REGISTRYINDEX);
    if (lua_isfunction(L, -1)) {
        setMatches(L);
        int error = lua_pcall(L, 0, LUA_MULTRET, 0);
        if (error) {
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
                        auto& host = getHostFromLua(L);
                        TDebug(Qt::white, Qt::red) << "LUA: ERROR running anonymous Lua function ERROR:" << e.c_str() >> &host;
                    }
                }
            }
        } else {
            if (mudlet::debugMode) {
                auto& host = getHostFromLua(L);
                TDebug(Qt::white, Qt::darkGreen) << "LUA OK anonymous Lua function ran without errors\n" >> &host;
            }
        }
        lua_pop(L, lua_gettop(L));
        //lua_settop(L, 0);
        return !error;

    }

    QString _n = "error in anonymous Lua function";
    QString _n2 = "func reference not found by Lua, func cannot be called";
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
    } else if (mudlet::debugMode) {
        qWarning() << "LUA: ERROR deleting " << name << ", it is not a function as expected";
    }
}

// No documentation available in wiki - internal function
// returns true if function ran without errors
// as well as the boolean return value from the function
std::pair<bool, bool> TLuaInterpreter::callLuaFunctionReturnBool(void* pT)
{
    lua_State* L = pGlobalLua;

    lua_pushlightuserdata(L, pT);
    lua_gettable(L, LUA_REGISTRYINDEX);
    bool returnValue = false;

    if (lua_isfunction(L, -1)) {
        setMatches(L);
        int error = lua_pcall(L, 0, LUA_MULTRET, 0);
        if (error) {
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

            if (mudlet::debugMode) {
                auto& host = getHostFromLua(L);
                TDebug(Qt::white, Qt::darkGreen) << "LUA OK anonymous Lua function ran without errors\n" >> &host;
            }
        }
        lua_pop(L, lua_gettop(L));
        return {!error , returnValue};
    }

    QString _n = "error in anonymous Lua function";
    QString _n2 = "func reference not found by Lua, func cannot be called";
    std::string e = "Lua error:";
    logError(e, _n, _n2);

    return {false, false};
}

// No documentation available in wiki - internal function
// Third argument hides the "LUA OK" type message if it true which may be used
// to cut down on spammy output if things are okay.
bool TLuaInterpreter::call(const QString& function, const QString& mName, const bool muteDebugOutput)
{
    lua_State* L = pGlobalLua;
    setMatches(L);

    lua_getglobal(L, function.toUtf8().constData());
    int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error) {
        int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                logError(e, mName, function);
                if (mudlet::debugMode) {
                    auto& host = getHostFromLua(L);
                    TDebug(Qt::white, Qt::red) << "LUA ERROR: when running script " << mName << " (" << function << "),\nreason: " << e.c_str() << "\n" >> &host;
                }
            }
        }
    } else {
        if (mudlet::debugMode && !muteDebugOutput) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA OK: script " << mName << " (" << function << ") ran without errors\n" >> &host;
        }
    }
    lua_pop(L, lua_gettop(L));

    return (error);
}

// No documentation available in wiki - internal function
std::pair<bool, bool> TLuaInterpreter::callReturnBool(const QString& function, const QString& mName)
{
    lua_State* L = pGlobalLua;
    bool returnValue = false;

    setMatches(L);

    lua_getglobal(L, function.toUtf8().constData());
    int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error) {
        int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                logError(e, mName, function);
                if (mudlet::debugMode) {
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

        if (mudlet::debugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA OK script " << mName << " (" << function << ") ran without errors\n" >> &host;
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
        mpHost->mpEditorDialog->mpErrorConsole->print(qsl(" %1:<%2> %3:<%4>\n").arg(tr("object", "object is the Mudlet alias/trigger/script, used in this sample message: object:<Alias1> function:<cure_me>"), name, tr("function", "function is the Lua function, used in this sample message: object:<Alias1> function:<cure_me>"), function), QColor(Qt::green), QColor(Qt::black));
        mpHost->mpEditorDialog->mpErrorConsole->print(qsl("        <%1>\n").arg(e.c_str()), QColor(Qt::red), QColor(Qt::black));
    }

    // Log error to Profile's Main TConsole:
    if (mpHost->mEchoLuaErrors) {
        // ensure the Lua error is on a line of its own and is not prepended to
        // the previous line, however there is a nasty gotcha in that during
        // profile loading the (TMainConsole*) Host::mpConsole pointer is
        // null - but then the buffer must itself be empty:
        if (mpHost->mpConsole && mpHost->mpConsole->buffer.size() > 0 && !mpHost->mpConsole->buffer.lineBuffer.at(mpHost->mpConsole->buffer.lineBuffer.size() - 1).isEmpty()) {
            mpHost->postMessage(qsl("\n"));
        }

        mpHost->postMessage(qsl("[  LUA  ] - %1: <%2> %3:<%4>\n<%5>").arg(tr("object", "object is the Mudlet alias/trigger/script, used in this sample message: object:<Alias1> function:<cure_me>"), name, tr("function", "function is the Lua function, used in this sample message: object:<Alias1> function:<cure_me>"), function, e.c_str()));
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
        if (mpHost->mpConsole->buffer.size() > 0 && !mpHost->mpConsole->buffer.lineBuffer.at(mpHost->mpConsole->buffer.lineBuffer.size() - 1).isEmpty()) {
            mpHost->postMessage(qsl("\n"));
        }

        mpHost->postMessage(qsl("[  LUA  ] - error in event handler for %1:\n<%2>").arg(event, error));
    }
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callConditionFunction(std::string& function, const QString& mName)
{
    lua_State* L = pGlobalLua;

    lua_getfield(L, LUA_GLOBALSINDEX, function.c_str());
    int error = lua_pcall(L, 0, 1, 0);
    if (error) {
        int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                QString _f = function.c_str();
                logError(e, mName, _f);
                if (mudlet::debugMode) {
                    auto& host = getHostFromLua(L);
                    TDebug(Qt::white, Qt::red) << "LUA: ERROR running script " << mName << " (" << function.c_str() << ") ERROR:" << e.c_str() << "\n" >> &host;
                }
            }
        }
    } else {
        if (mudlet::debugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA OK script " << mName << " (" << function.c_str() << ") ran without errors\n" >> &host;
        }
    }

    bool ret = false;
    int returnValues = lua_gettop(L);
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
            for (auto [name, capture] : mMultiCaptureNameGroups.value(k - 1)) {
                lua_pushstring(L, name.toUtf8().constData());
                lua_pushstring(L, capture.toUtf8().constData());
                lua_settable(L, -3);
            }
            lua_settable(L, -3); //matches in regex
        }
        lua_setglobal(L, "multimatches");
    }

    lua_getglobal(L, function.toUtf8().constData());
    int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error) {
        int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                logError(e, mName, function);
                if (mudlet::debugMode) {
                    auto& host = getHostFromLua(L);
                    TDebug(Qt::white, Qt::red) << "LUA: ERROR running script " << mName << " (" << function << ") ERROR:" << e.c_str() << "\n" >> &host;
                }
            }
        }
    } else {
        if (mudlet::debugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA OK script " << mName << " (" << function << ") ran without errors\n" >> &host;
        }
    }
    lua_pop(L, lua_gettop(L));
    return !error;
}

// No documentation available in wiki - internal function
std::pair<bool, bool> TLuaInterpreter::callMultiReturnBool(const QString& function, const QString& mName)
{
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
    int error = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (error) {
        int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                logError(e, mName, function);
                if (mudlet::debugMode) {
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

        if (mudlet::debugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA OK script " << mName << " (" << function << ") ran without errors\n" >> &host;
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
        if (mudlet::debugMode) {
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
    lua_rawgeti(L, LUA_REGISTRYINDEX, func);
    lua_pushstring(L, text.toUtf8().constData());
    return callReference(L, qsl("cmdLineAction"), 1);
}

// No documentation available in wiki - internal function
bool TLuaInterpreter::callLabelCallbackEvent(const int func, const QEvent* qE)
{
    lua_State* L = pGlobalLua;
    lua_rawgeti(L, LUA_REGISTRYINDEX, func);
    QString name = qsl("label callback event");

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
            lua_pushstring(L, mMouseButtons.value(qME->button()).toUtf8().constData());
            lua_setfield(L, -2, qsl("button").toUtf8().constData());

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
            lua_setfield(L, -2, qsl("buttons").toUtf8().constData());

            // Push globalX()
            lua_pushnumber(L, qME->globalX());
            lua_setfield(L, -2, qsl("globalX").toUtf8().constData());

            // Push globalY()
            lua_pushnumber(L, qME->globalY());
            lua_setfield(L, -2, qsl("globalY").toUtf8().constData());

            // Push x()
            lua_pushnumber(L, qME->x());
            lua_setfield(L, -2, qsl("x").toUtf8().constData());

            // Push y()
            lua_pushnumber(L, qME->y());
            lua_setfield(L, -2, qsl("y").toUtf8().constData());
            return callReference(L, name, 1);
        }
        // These are QEvents
        case (QEvent::Enter): {
            auto qME = static_cast<const QEnterEvent*>(qE);
            lua_newtable(L);

            // Push globalX()
            lua_pushnumber(L, qME->globalX());
            lua_setfield(L, -2, qsl("globalX").toUtf8().constData());

            // Push globalY()
            lua_pushnumber(L, qME->globalY());
            lua_setfield(L, -2, qsl("globalY").toUtf8().constData());

            // Push x()
            lua_pushnumber(L, qME->x());
            lua_setfield(L, -2, qsl("x").toUtf8().constData());

            // Push y()
            lua_pushnumber(L, qME->y());
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
            lua_setfield(L, -2, qsl("buttons").toUtf8().constData());

// The Qt documentation is unclear when, precisely QWheelEvent::globalPos()
// became obsolete, 5.14.0 is a guess
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
            auto globalPosition = qME->globalPosition();
#else
            auto globalPosition = qME->globalPos();
#endif
            // Push globalX()
            lua_pushnumber(L, globalPosition.x());
            lua_setfield(L, -2, qsl("globalX").toUtf8().constData());

            // Push globalY()
            lua_pushnumber(L, globalPosition.y());
            lua_setfield(L, -2, qsl("globalY").toUtf8().constData());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
            auto position = qME->position();
#endif

            // Push x()
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
            lua_pushnumber(L, position.x());
#else
            lua_pushnumber(L, qME->x());
#endif
            lua_setfield(L, -2, qsl("x").toUtf8().constData());

            // Push y()
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
            lua_pushnumber(L, position.y());
#else
            lua_pushnumber(L, qME->y());
#endif
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
        QString name = "event handler function";
        logError(err, name, function);
        if (mudlet::debugMode) {
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
    QString luaFunction = qsl("condenseMapLoad");
    double loadTime = -1.0;

    lua_State* L = pGlobalLua;

    lua_getfield(L, LUA_GLOBALSINDEX, "condenseMapLoad");
    int error = lua_pcall(L, 0, 1, 0);
    if (error) {
        int nbpossible_errors = lua_gettop(L);
        for (int i = 1; i <= nbpossible_errors; i++) {
            std::string e = "";
            if (lua_isstring(L, i)) {
                e += lua_tostring(L, i);
                QString _f = luaFunction.toUtf8().constData();
                logError(e, luaFunction, _f);
                if (mudlet::debugMode) {
                    auto& host = getHostFromLua(L);
                    TDebug(Qt::white, Qt::red) << "LUA: ERROR running " << luaFunction << " ERROR:" << e.c_str() << "\n" >> &host;
                }
            }
        }
    } else {
        if (mudlet::debugMode) {
            auto& host = getHostFromLua(L);
            TDebug(Qt::white, Qt::darkGreen) << "LUA OK " << luaFunction << " ran without errors\n" >> &host;
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
    return performHttpRequest(L, __func__, 0, QNetworkAccessManager::PutOperation, qsl("put"));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#customHTTP
int TLuaInterpreter::customHTTP(lua_State* L)
{
    auto customMethod = getVerifiedString(L, __func__, 1, "http method");
    return performHttpRequest(L, __func__, 1, QNetworkAccessManager::CustomOperation, customMethod);
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

    QString urlString = getVerifiedString(L, functionName, pos + 2, "remote url");
    QUrl url = QUrl::fromUserInput(urlString);

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

    if (mudlet::debugMode) {
        TDebug(Qt::white, Qt::blue) << functionName << ": script is uploading data to " << reply->url().toString() << "\n" >> &host;
    }

    lua_pushboolean(L, true);
    lua_pushstring(L, reply->url().toString().toUtf8().constData()); // Returns the Url that was ACTUALLY used
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getHTTP
int TLuaInterpreter::getHTTP(lua_State* L)
{
    auto& host = getHostFromLua(L);
    QString urlString = getVerifiedString(L, __func__, 1, "remote url");
    QUrl url = QUrl::fromUserInput(urlString);
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

    if (mudlet::debugMode) {
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

// Documentation: https://wiki.mudlet.org/w/Manual:Networking_Functions#deleteHTTP
int TLuaInterpreter::deleteHTTP(lua_State *L)
{
    auto& host = getHostFromLua(L);
    QString urlString = getVerifiedString(L, __func__, 1, "remote url");
    QUrl url = QUrl::fromUserInput(urlString);
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

    if (mudlet::debugMode) {
        TDebug(Qt::white, Qt::blue) << qsl("deleteHTTP: script is sending delete request for %1\n").arg(reply->url().toString()) >> &host;
    }

    lua_pushboolean(L, true);
    lua_pushstring(L, reply->url().toString().toUtf8().constData()); // Returns the Url that was ACTUALLY used
    return 2;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getConnectionInfo
int TLuaInterpreter::getConnectionInfo(lua_State *L)
{
    Host& host = getHostFromLua(L);

    auto [hostName, hostPort, connected] = host.mTelnet.getConnectionInfo();
    lua_pushstring(L, hostName.toUtf8().constData());
    lua_pushnumber(L, hostPort);
    lua_pushboolean(L, connected);
    return 3;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Networking_Functions#unzipAsync
int TLuaInterpreter::unzipAsync(lua_State *L)
{
    QString zipLocation = getVerifiedString(L, __func__, 1, "zip location");
    QString extractLocation = getVerifiedString(L, __func__, 2, "extract location");

    QTemporaryDir temporaryDir;
    if (!temporaryDir.isValid()) {
        return warnArgumentValue(L, __func__, "couldn't create temporary directory to extract the zip into");
    }

    extractLocation = QDir::fromNativeSeparators(extractLocation);
    if (!extractLocation.endsWith(QLatin1String("/"))) {
        extractLocation.append(QLatin1String("/"));
    }

    QDir dir;
    if (!dir.mkpath(extractLocation)) {
        return warnArgumentValue(L, __func__, "couldn't create output directory to put the extracted files into");
    }

    auto future = QtConcurrent::run(mudlet::unzip, zipLocation, extractLocation, temporaryDir.path());
    auto watcher = new QFutureWatcher<bool>;
    QObject::connect(watcher, &QFutureWatcher<bool>::finished, [=]() {
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
    int top = lua_gettop(L);

    lua_pushnumber(L, varValue);
    lua_setglobal(L, varName.toUtf8().constData());
    lua_settop(L, top);
}

// No documentation available in wiki - internal function
QString TLuaInterpreter::getLuaString(const QString& stringName)
{
    lua_State* L = pGlobalLua;

    int error = luaL_dostring(L, qsl("return %1").arg(stringName).toUtf8().constData());
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

    int r = lua_toboolean(L, -1);
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

// No documentation available in wiki - internal helper function
// On success will swap out any messages in the queue and replace them
// with its success message, on failure will just append...
bool TLuaInterpreter::loadLuaModule(QQueue<QString>& resultMsgsQueue, const QString& requirement, const QString& failureConsequence, const QString& description, const QString& luaModuleId)
{
    int error = luaL_dostring(pGlobalLua, qsl("%1require \"%2\"")
                              .arg(luaModuleId.isEmpty() ? QString() : qsl("%1 =").arg(luaModuleId),
                                   requirement).toUtf8().constData());
    if (error) {
        QString luaErrorMsg = tr("No error message available from Lua");
        if (lua_isstring(pGlobalLua, -1)) {
            luaErrorMsg = tr("Lua error: %1").arg(lua_tostring(pGlobalLua, -1));
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
                                     (failureConsequence.isEmpty() ? QString() : qsl("\n%1").arg(failureConsequence))));
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
    lua_register(pGlobalLua, "resetFormat", TLuaInterpreter::resetFormat);
    lua_register(pGlobalLua, "moveCursorEnd", TLuaInterpreter::moveCursorEnd);
    lua_register(pGlobalLua, "getLastLineNumber", TLuaInterpreter::getLastLineNumber);
    lua_register(pGlobalLua, "getNetworkLatency", TLuaInterpreter::getNetworkLatency);
    lua_register(pGlobalLua, "createMiniConsole", TLuaInterpreter::createMiniConsole);
    lua_register(pGlobalLua, "createScrollBox", TLuaInterpreter::createScrollBox);
    lua_register(pGlobalLua, "createLabel", TLuaInterpreter::createLabel);
    lua_register(pGlobalLua, "deleteLabel", TLuaInterpreter::deleteLabel);
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
    lua_register(pGlobalLua, "playSoundFile", TLuaInterpreter::playSoundFile);
    lua_register(pGlobalLua, "playMusicFile", TLuaInterpreter::playMusicFile);
    lua_register(pGlobalLua, "stopMusic", TLuaInterpreter::stopMusic);
    lua_register(pGlobalLua, "stopSounds", TLuaInterpreter::stopSounds);
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
    lua_register(pGlobalLua, "setConsoleBufferSize", TLuaInterpreter::setConsoleBufferSize);
    lua_register(pGlobalLua, "enableScrollBar", TLuaInterpreter::enableScrollBar);
    lua_register(pGlobalLua, "disableScrollBar", TLuaInterpreter::disableScrollBar);
    lua_register(pGlobalLua, "enableHorizontalScrollBar", TLuaInterpreter::enableHorizontalScrollBar);
    lua_register(pGlobalLua, "disableHorizontalScrollBar", TLuaInterpreter::disableHorizontalScrollBar);
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
    // PLACEMARKER: End of main Lua interpreter functions registration

    QStringList additionalLuaPaths;
    QStringList additionalCPaths;
    const auto appPath{QCoreApplication::applicationDirPath()};
    const auto profilePath{mudlet::getMudletPath(mudlet::profileHomePath, hostName)};

    // Allow for modules or libraries placed in the profile root directory:
    additionalLuaPaths << qsl("%1/?.lua").arg(profilePath);
    additionalLuaPaths << qsl("%1/?/init.lua").arg(profilePath);
#if defined(Q_OS_WIN32)
    additionalCPaths << qsl("%1/?.dll").arg(profilePath);
#else
    additionalCPaths << qsl("%1/?.so").arg(profilePath);
#endif

#if defined(Q_OS_LINUX)
    // AppInstaller on Linux would like the C search path to also be set to
    // a ./lib sub-directory of the current binary directory:
    additionalCPaths << qsl("%1/lib/?.so").arg(appPath);
#elif defined(Q_OS_MAC)
    // macOS app bundle would like the search path to also be set to the current
    // binary directory for both modules and binary libraries:
    additionalCPaths << qsl("%1/?.so").arg(appPath);
    additionalLuaPaths << qsl("%1/?.lua").arg(appPath);
#elif defined(Q_OS_WIN32) && defined(INCLUDE_MAIN_BUILD_SYSTEM)
    // For CI builds or users/developers using the setup-windows-sdk.ps1 method:
    additionalCPaths << qsl("C:\\Qt\\Tools\\mingw730_32\\lib\\lua\\5.1\\?.dll");
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

lua_State* TLuaInterpreter::getLuaGlobalState() {
    return pGlobalLua;
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
#if ! defined (Q_OS_WIN32)
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
        QDir::toNativeSeparators(qsl("%1/../../mudlet/src/mudlet-lua/lua/LuaGlobal.lua").arg(executablePath))
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
        luaL_dostring(pGlobalLua, qsl("luaGlobalPath = \"%1\"").arg(QFileInfo(pathFileName).absolutePath()).toUtf8().constData());

        // load via Qt so UTF8 paths work on Windows - Lua can't handle it
        auto luaGlobal = readScriptFile(pathFileName);

        if (luaGlobal.isEmpty()) {
            failedMessages << tr("%1 (couldn't read file)", "This file could not be read for some reason (for example, no permission)").arg(pathFileName);
            continue;
        }

        error = luaL_dostring(pGlobalLua, luaGlobal.toUtf8().constData());
        if (!error) {
            mpHost->postMessage(tr("[  OK  ]  - Mudlet-lua API & Geyser Layout manager loaded."));
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
    in.setCodec(QTextCodec::codecForName("UTF-8"));

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

#if defined(Q_OS_WIN32)
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
        auto ids = mpHost->getScriptUnit()->findScriptId(parent);
        auto pParentScript = mpHost->getScriptUnit()->getScript(ids.value(0, -1));
        if (!pParentScript) {
            return {-1, qsl("parent '%1' not found").arg(parent)}; //parent not found
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
        return {-1, qsl("unable to compile \"%1\", reason: %2").arg(luaCode, errMsg)};
    }

    int id = pS->getID();
    pS->setIsActive(false);
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return {id, QString()};
}

// No documentation available in wiki - internal function
std::pair<int, QString> TLuaInterpreter::setScriptCode(QString& name, const QString& luaCode, int pos)
{
    if (name.isEmpty()) {
        return {-1, qsl("cannot have an empty string as name")};
    }

    auto ids = mpHost->getScriptUnit()->findScriptId(name);
    TScript* pS = mpHost->getScriptUnit()->getScript(ids.value(pos, -1));
    if (!pS) {
        return {-1, qsl("script \"%1\" at position \"%2\" not found").arg(name).arg(++pos)}; //script not found
    }
    auto oldCode = pS->getScript();
    if (!pS->setScript(luaCode)) {
        QString errMsg = pS->getError();
        pS->setScript(oldCode);
        return {-1, qsl("unable to compile \"%1\" at position \"%2\", reason: %3").arg(luaCode).arg(++pos).arg(errMsg)};
    }
    int id = pS->getID();
    mpHost->mpEditorDialog->writeScript(id);
    return {id, QString()};
}

// No documentation available in wiki - internal function
std::pair<int, QString> TLuaInterpreter::startPermTimer(const QString& name, const QString& parent, double timeout, const QString& function)
{
    QTime time = QTime(0, 0, 0, 0).addMSecs(qRound(timeout * 1000));
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
    // Also for perminent timers it is easier to debug if it is set before
    // registration:
    pT->setName(name);
    // This will lead to the generation of the id number:
    mpHost->getTimerUnit()->registerTimer(pT);
    if (!pT->setScript(function)) {
        QString errMsg = pT->getError();
        // Apparently this will call the TTimer::unregisterTimer(...) method:
        delete pT;
        return {-1, qsl("unable to compile \"%1\", reason: %2").arg(function, errMsg)};
    }

    pT->setIsActive(false);
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return {pT->getID(), QString()};
}

// No documentation available in wiki - internal function
QPair<int, QString> TLuaInterpreter::startTempTimer(double timeout, const QString& function, const bool repeating)
{
    QTime time = QTime(0,0,0,0).addMSecs(qRound(timeout * 1000));
    auto* pT = new TTimer(qsl("newTempTimerWithoutAnId"), time, mpHost, repeating);
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
        return qMakePair(-1, qsl("unable to compile \"%1\", reason: %2").arg(function, errMsg));
    }

    int id = pT->getID();
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
    mpHost->mpEditorDialog->mNeedUpdateData = true;
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
    int id = pT->getID();
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
    mpHost->mpEditorDialog->mNeedUpdateData = true;
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
    int id = pT->getID();
    pT->setName(QString::number(id));
    return id;
}

// No documentation available in wiki - internal function
int TLuaInterpreter::startTempExactMatchTrigger(const QString& regex, const QString& function, int expiryCount)
{
    TTrigger* pT = nullptr;
    QStringList sList {regex};
    QList<int> propertyList {REGEX_EXACT_MATCH};
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
    TTrigger* pT = nullptr;
    QStringList sList {regex};
    QList<int> propertyList {REGEX_BEGIN_OF_LINE_SUBSTRING};
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
    TTrigger* pT = nullptr;
    QStringList sList {regex};
    QList<int> propertyList {REGEX_SUBSTRING};
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
    TTrigger* pT = nullptr;
    QStringList sList {regex};
    QList<int> propertyList {REGEX_PERL};
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
    mpHost->mpEditorDialog->mNeedUpdateData = true;
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
    mpHost->mpEditorDialog->mNeedUpdateData = true;
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
    mpHost->mpEditorDialog->mNeedUpdateData = true;
    return {pT->getID(), QString()};
}

// No documentation available in wiki - internal function
std::pair<int, QString> TLuaInterpreter::startPermPromptTrigger(const QString& name, const QString& parent, const QString& function)
{
    TTrigger* pT;
    QList<int> propertyList = {REGEX_PROMPT};
    QStringList patterns = {QString()};

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
    mpHost->mpEditorDialog->mNeedUpdateData = true;
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getColumnCount
int TLuaInterpreter::getColumnCount(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};

    int columns;
    auto console = CONSOLE(L, windowName);
    columns = console->mUpperPane->getColumnCount();
    lua_pushnumber(L, columns);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getRowCount
int TLuaInterpreter::getRowCount(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};

    int rows;
    auto console = CONSOLE(L, windowName);
    rows = console->mUpperPane->getRowCount();
    lua_pushnumber(L, rows);
    return 1;
}

// No documentation available in wiki - internal function
// Used to unref lua objects in the registry to avoid memory leaks
// i.e. Unrefing tables passed into TLabel's event parameters.
void TLuaInterpreter::freeLuaRegistryIndex(int index) {
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapSelection
int TLuaInterpreter::getMapSelection(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    if (!pHost || !pHost->mpMap || !pHost->mpMap->mpMapper || !pHost->mpMap->mpMapper->mp2dMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
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
    QString windowName {WINDOW_NAME(L, 1)};

    Host& host = getHostFromLua(L);

    host.setClickthrough(windowName, true);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableClickthrough
int TLuaInterpreter::disableClickthrough(lua_State* L)
{
    QString windowName {WINDOW_NAME(L, 1)};

    Host& host = getHostFromLua(L);

    host.setClickthrough(windowName, false);
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
        return warnArgumentValue(L, __func__, "no user dictionary enabled in the preferences for this profile");
    }

    QString text = getVerifiedString(L, __func__, 1, "word");
    QPair<bool, QString> result = host.mpConsole->addWordToSet(text);
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

    QString text = getVerifiedString(L, __func__, 1, "word");
    QPair<bool, QString> result = host.mpConsole->removeWordFromSet(text);
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

    QString text = getVerifiedString(L, __func__, 1, "word");

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

    QString text = getVerifiedString(L, __func__, 1, "word");

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
        int r = i / 36;
        int g = (i - (r * 36)) / 6;
        int b = (i - (r * 36)) - (g * 6);

        lua_createtable(L, 3, 0);

        lua_pushnumber(L, r == 0 ? 0 : (r - 1) * 40 + 95);
        lua_rawseti(L, -2, 1);

        lua_pushnumber(L, g == 0 ? 0 : (g - 1) * 40 + 95);
        lua_rawseti(L, -2, 2);

        lua_pushnumber(L, b == 0 ? 0 : (b - 1) * 40 + 95);
        lua_rawseti(L, -2, 3);

        lua_getfield(L, LUA_GLOBALSINDEX, "color_table");
        lua_insert(L, -2);

        QString name = qsl("ansi_%1").arg(i + 16, 3, 10, QLatin1Char('0'));
        lua_pushstring(L, name.toUtf8().constData());
        lua_insert(L, -2);
        lua_settable(L, -3);
        lua_pop(L, 1);
    }

    // And insert the 24 Greyscale colours
    for (int i = 232; i < 256; ++i) {
        lua_createtable(L, 3, 0);

        int value = (i - 232) * 10 + 8;

        lua_pushnumber(L, value);
        lua_rawseti(L, -2, 1);

        lua_pushnumber(L, value);
        lua_rawseti(L, -2, 2);

        lua_pushnumber(L, value);
        lua_rawseti(L, -2, 3);

        lua_getfield(L, LUA_GLOBALSINDEX, "color_table");
        lua_insert(L, -2);

        QString name = qsl("ansi_%1").arg(i, 3, 10, QLatin1Char('0'));
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMapBackgroundColor
int TLuaInterpreter::getMapBackgroundColor(lua_State* L)
{
    auto& host = getHostFromLua(L);
    auto color = host.mBgColor_2;
    lua_pushnumber(L, color.red());
    lua_pushnumber(L, color.green());
    lua_pushnumber(L, color.blue());
    return 3;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#setMapBackgroundColor
int TLuaInterpreter::setMapBackgroundColor(lua_State* L)
{
    int r = getVerifiedInt(L, __func__, 1, "red component");
    if (r < 0 || r > 255) {
        return warnArgumentValue(L, __func__, qsl("red value %1 needs to be between 0-255").arg(r));
    }

    int g = getVerifiedInt(L, __func__, 2, "green component");
    if (g < 0 || g > 255) {
        return warnArgumentValue(L, __func__, qsl("green value %1 needs to be between 0-255").arg(g));
    }

    int b = getVerifiedInt(L, __func__, 3, "blue component");
    if (b < 0 || b > 255) {
        return warnArgumentValue(L, __func__, qsl("blue value %1 needs to be between 0-255").arg(b));
    }

    auto& host = getHostFromLua(L);
    host.mBgColor_2 = QColor(r, g, b);
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
    int r = getVerifiedInt(L, __func__, 1, "red component");
    if (r < 0 || r > 255) {
        return warnArgumentValue(L, __func__, qsl("red value %1 needs to be between 0-255").arg(r));
    }

    int g = getVerifiedInt(L, __func__, 2, "green component");
    if (g < 0 || g > 255) {
        return warnArgumentValue(L, __func__, qsl("green value %1 needs to be between 0-255").arg(g));
    }

    int b = getVerifiedInt(L, __func__, 3, "blue component");
    if (b < 0 || b > 255) {
        return warnArgumentValue(L, __func__, qsl("blue value %1 needs to be between 0-255").arg(b));
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
    int n = lua_gettop(L);
    QString title = getVerifiedString(L, __func__, 1, "title");
    QString text = title;
    if (n >= 2) {
        text = getVerifiedString(L, __func__, 2, "message");
    }
    int notificationExpirationTime = 1;
    if (n >= 3) {
        notificationExpirationTime = qMax(qRound(getVerifiedDouble(L, __func__, 3, "expiration time in seconds") / 1000), 1);
    }

    mudlet::self()->mTrayIcon.show();
    mudlet::self()->mTrayIcon.showMessage(title, text, mudlet::self()->mTrayIcon.icon(), notificationExpirationTime);
    return 0;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#saveJsonMap
int TLuaInterpreter::saveJsonMap(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    if (!pHost || !pHost->mpMap || !pHost->mpMap->mpMapper || !pHost->mpMap->mpMapper->mp2dMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    QString destination;

    if (lua_gettop(L) > 0) {
        destination = getVerifiedString(L, __func__, 1, "export pathFileName");
    }

    if (auto [result, message] = pHost->mpMap->writeJsonMapFile(destination); !result) {
        return warnArgumentValue(L, __func__, message);
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#loadJsonMap
int TLuaInterpreter::loadJsonMap(lua_State* L)
{
    Host* pHost = &getHostFromLua(L);
    if (!pHost || !pHost->mpMap || !pHost->mpMap->mpMapper || !pHost->mpMap->mpMapper->mp2dMap) {
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    auto source = getVerifiedString(L, __func__, 1, "import pathFileName");
    if (source.isEmpty()) {
        return warnArgumentValue(L, __func__, "a non-empty path and file name to read to must be provided");
    }

    if (auto [result, message] = pHost->mpMap->readJsonMapFile(source); !result) {
        return warnArgumentValue(L, __func__, message);
    }

    // Must run the audit() process now - as it is no longer done within
    // TMap::readJsonMapFile(...) as that can now be used elsewhere:
    pHost->mpMap->audit();
    pHost->mpMap->mpMapper->mp2dMap->init();
    pHost->mpMap->mpMapper->updateAreaComboBox();
    pHost->mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
    pHost->mpMap->mpMapper->show();

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#registerMapInfo
int TLuaInterpreter::registerMapInfo(lua_State* L)
{
    auto name = getVerifiedString(L, __func__, 1, "label");

    if (!lua_isfunction(L, 2)) {
        lua_pushfstring(L, "registerMapInfo: bad argument #2 type (callback as function expected, got %s!)", luaL_typename(L, 2));
        return lua_error(L);
    }
    int callback = luaL_ref(L, LUA_REGISTRYINDEX);

    auto& host = getHostFromLua(L);
    host.mpMap->mMapInfoContributorManager->registerContributor(name, [=](int roomID, int selectionSize, int areaId, int displayAreaId, QColor& infoColor) {
        Q_UNUSED(infoColor);
        lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
        if (roomID > 0) {
            lua_pushinteger(L, roomID);
        } else {
            lua_pushnil(L);
        }
        lua_pushinteger(L, selectionSize);
        lua_pushinteger(L, areaId);
        lua_pushinteger(L, displayAreaId);

        int error = lua_pcall(L, 4, 6, 0);
        if (error) {
            int errorCount = lua_gettop(L);
            if (mudlet::debugMode) {
                for (int i = 1; i <= errorCount; i++) {
                    if (lua_isstring(L, i)) {
                        auto errorMessage = lua_tostring(L, i);
                        TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA ERROR: when running map info callback for '" << name << "\nreason: " << errorMessage << "\n" >> 0;
                    }
                }
            }
            lua_pop(L, errorCount);
            return MapInfoProperties{};
        }

        auto nResult = lua_gettop(L);
        auto index = -nResult;
        QString text = lua_tostring(L, index);
        bool isBold = lua_toboolean(L, ++index);
        bool isItalic = lua_toboolean(L, ++index);
        int r = -1;
        int g = -1;
        int b = -1;
        if (!lua_isnil(L, ++index)) {
            r = lua_tonumber(L, index);
        }
        if (!lua_isnil(L, ++index)) {
            g = lua_tonumber(L, index);
        }
        if (!lua_isnil(L, ++index)) {
            b = lua_tonumber(L, index);
        }
        QColor color;
        if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
            color = QColor(r, g, b);
        }
        lua_pop(L, nResult);
        return MapInfoProperties{ isBold, isItalic, text, color };
    });

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#killMapInfo
int TLuaInterpreter::killMapInfo(lua_State* L)
{
    auto& host = getHostFromLua(L);
    auto name = getVerifiedString(L, __func__, 1, "label");
    if (!host.mpMap->mMapInfoContributorManager->removeContributor(name)) {
        return warnArgumentValue(L, __func__, qsl("map info '%1' does not exist").arg(name));
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#enableMapInfo
int TLuaInterpreter::enableMapInfo(lua_State* L)
{
    auto name = getVerifiedString(L, __func__, 1, "label");
    auto& host = getHostFromLua(L);
    if (!host.mpMap->mMapInfoContributorManager->enableContributor(name)) {
        return warnArgumentValue(L, __func__, qsl("map info '%1' does not exist").arg(name));
    }
    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#disableMapInfo
int TLuaInterpreter::disableMapInfo(lua_State* L)
{
    auto name = getVerifiedString(L, __func__, 1, "label");
    auto& host = getHostFromLua(L);
    if (!host.mpMap->mMapInfoContributorManager->disableContributor(name)) {
        return warnArgumentValue(L, __func__, qsl("map info '%1' does not exist").arg(name));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addFileWatch
int TLuaInterpreter::addFileWatch(lua_State * L)
{
    auto path = getVerifiedString(L, __func__, 1, "path");
    auto& host = getHostFromLua(L);

    QFileInfo fileInfo(path);
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

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addMouseEvent
int TLuaInterpreter::addMouseEvent(lua_State * L)
{
    Host& host = getHostFromLua(L);
    QStringList actionInfo;
    QString uniqueName = getVerifiedString(L, __func__, 1, "uniquename");
    if (host.mConsoleActions.contains(uniqueName)) {
        return warnArgumentValue(L, __func__, qsl("mouse event '%1' already exists").arg(uniqueName));
    }

    actionInfo << getVerifiedString(L, __func__, 2, "event name", false);

    // Display name
    if (!lua_isstring(L, 3)) {
        actionInfo << uniqueName;
    } else {
        actionInfo << lua_tostring(L, 3);
    }

    // tooltip text
    if (!lua_isstring(L, 4)) {
        actionInfo << QString();
    } else {
        actionInfo << lua_tostring(L, 4);
    }

    host.mConsoleActions.insert(uniqueName, actionInfo);

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeMouseEvent
int TLuaInterpreter::removeMouseEvent(lua_State * L)
{
    QString uniqueName = getVerifiedString(L, __func__, 1, "event name");
    Host& host = getHostFromLua(L);
    if (host.mConsoleActions.remove(uniqueName) == 0) {
        return warnArgumentValue(L, __func__, qsl("mouse event '%1' does not exist").arg(uniqueName));
    }

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getMouseEvents
int TLuaInterpreter::getMouseEvents(lua_State * L)
{
    Host& host = getHostFromLua(L);
    // create the result table
    lua_newtable(L);
    QMapIterator<QString, QStringList> it(host.mConsoleActions);
    while (it.hasNext()) {
        it.next();
        QStringList eventInfo = it.value();
        lua_createtable(L, 0, 3);
        lua_pushstring(L, eventInfo.at(0).toUtf8().constData());
        lua_setfield(L, -2, "event name");
        lua_pushstring(L, eventInfo.at(1).toUtf8().constData());
        lua_setfield(L, -2, "display name");
        lua_pushstring(L, eventInfo.at(2).toUtf8().constData());
        lua_setfield(L, -2, "tooltip text");

        // Add the mapEvent object to the result table
        lua_setfield(L, -2, it.key().toUtf8().constData());
    }
    return 1;
}

int TLuaInterpreter::setConfig(lua_State * L)
{
    auto& host = getHostFromLua(L);
    QString key = getVerifiedString(L, __func__, 1, "key");
    if (key.isEmpty()) {
        return warnArgumentValue(L, __func__, "you must provide key");
    }

    auto success = [&]()
    {
        if (mudlet::debugMode) {
            TDebug(Qt::white, Qt::blue) << qsl("setConfig: a script has changed %1\n").arg(key) >> &host;
        }
        lua_pushboolean(L, true);
        return 1;
    };

    if (host.mpMap && host.mpMap->mpMapper) {
        if (key == qsl("mapRoomSize")) {
            host.mpMap->mpMapper->slot_setRoomSize(getVerifiedInt(L, __func__, 2, "value"));
            return success();
        }
        if (key == qsl("mapExitSize")) {
            host.mpMap->mpMapper->slot_setExitSize(getVerifiedInt(L, __func__, 2, "value"));
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
    }

    if (key == qsl("enableGMCP")) {
        host.mEnableGMCP = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("enableMSDP")) {
        host.mEnableMSDP = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("enableMSSP")) {
        host.mEnableMSSP = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("enableMSP")) {
        host.mEnableMSP = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("inputLineStrictUnixEndings")) {
        host.mUSE_UNIX_EOL = getVerifiedBool(L, __func__, 2, "value");
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
        host.mFORCE_CHARSET_NEGOTIATION_OFF = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }
    if (key == qsl("specialForceMxpNegotiationOff")) {
        host.mFORCE_MXP_NEGOTIATION_OFF = getVerifiedBool(L, __func__, 2, "value");
        return success();
    }

    return warnArgumentValue(L, __func__, qsl("'%1' isn't a valid configuration option").arg(key));
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#addCommandLineMenuEvent
int TLuaInterpreter::addCommandLineMenuEvent(lua_State * L)
{
    int args = 1;
    int argsCount = lua_gettop(L);

    QString commandLineName;
    if (argsCount >= 3) {
        commandLineName = getVerifiedString(L, __func__, args++, "command line name");
    } else {
        commandLineName = qsl("main");
    }
    auto menuLabel = getVerifiedString(L, __func__, args++, "menu label");
    auto eventName = getVerifiedString(L, __func__, args++, "event name");

    const auto& commandline = COMMANDLINE(L, commandLineName);
    commandline->contextMenuItems.insert(menuLabel, eventName);

    lua_pushboolean(L, true);
    return 1;
}

// Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#removeCommandLineMenuEvent
int TLuaInterpreter::removeCommandLineMenuEvent(lua_State * L)
{
    int args = 1;
    int argsCount = lua_gettop(L);

    QString commandLineName;
    if (argsCount >= 2) {
        commandLineName = getVerifiedString(L, __func__, args++, "command line name");
    } else {
        commandLineName = qsl("main");
    }
    auto menuLabel = getVerifiedString(L, __func__, args++, "menu label");

    const auto& commandline = COMMANDLINE(L, commandLineName);

    if (commandline->contextMenuItems.remove(menuLabel) == 0) {
        lua_pushboolean(L, false);
        lua_pushfstring(L, "removeCommandLineMenuEvent: Cannot remove '%s', menu item does not exist.", menuLabel.toUtf8().constData());
        return 2;
    }
    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::deleteMap(lua_State* L)
{
    Host& host = getHostFromLua(L);

    if (!host.mpMap || !host.mpMap->mpRoomDB) {
        // These tests pass even if the map is empty, however there can still
        // be deleteable data present even in that case - and this test will
        // still succeed immediately after this function has been used!
        return warnArgumentValue(L, __func__, "no map present or loaded");
    }

    host.mpMap->mapClear();

    // Also cause any displayed map to reset:
    updateMap(L);

    lua_pushboolean(L, true);
    return 1;
}

int TLuaInterpreter::getProfileStats(lua_State* L)
{
    Host& host = getHostFromLua(L);

    auto [_1, triggersTotal, totalPatterns, tempTriggers, activeTriggers, activePatterns] = host.getTriggerUnit()->assembleReport();
    auto [_2, aliasesTotal, tempAliases, activeAliases] = host.getAliasUnit()->assembleReport();
    auto [_3, timersTotal, tempTimers, activeTimers] = host.getTimerUnit()->assembleReport();
    auto [_4, keysTotal, tempKeys, activeKeys] = host.getKeyUnit()->assembleReport();
    auto [_5, scriptsTotal, tempScripts, activeScripts] = host.getScriptUnit()->assembleReport();

    lua_newtable(L);

    // Triggers
    lua_pushstring(L, "triggers");
    lua_newtable(L);

    lua_pushstring(L, "total");
    lua_pushnumber(L, triggersTotal);
    lua_settable(L, -3);

    lua_pushstring(L, "temp");
    lua_pushnumber(L, tempTriggers);
    lua_settable(L, -3);

    lua_pushstring(L, "active");
    lua_pushnumber(L, activeTriggers);
    lua_settable(L, -3); // active

    lua_pushstring(L, "patterns");
    lua_newtable(L);

    lua_pushstring(L, "total");
    lua_pushnumber(L, totalPatterns);
    lua_settable(L, -3);

    lua_pushstring(L, "active");
    lua_pushnumber(L, activePatterns);
    lua_settable(L, -3);

    lua_settable(L, -3); // patterns
    lua_settable(L, -3); // triggers

    // Aliases
    lua_pushstring(L, "aliases");
    lua_newtable(L);

    lua_pushstring(L, "total");
    lua_pushnumber(L, aliasesTotal);
    lua_settable(L, -3);

    lua_pushstring(L, "temp");
    lua_pushnumber(L, tempAliases);
    lua_settable(L, -3);

    lua_pushstring(L, "active");
    lua_pushnumber(L, activeAliases);
    lua_settable(L, -3);
    lua_settable(L, -3);

    // Timers
    lua_pushstring(L, "timers");
    lua_newtable(L);

    lua_pushstring(L, "total");
    lua_pushnumber(L, timersTotal);
    lua_settable(L, -3);

    lua_pushstring(L, "temp");
    lua_pushnumber(L, tempTimers);
    lua_settable(L, -3);

    lua_pushstring(L, "active");
    lua_pushnumber(L, activeTimers);
    lua_settable(L, -3);
    lua_settable(L, -3);

    // Keys
    lua_pushstring(L, "keys");
    lua_newtable(L);

    lua_pushstring(L, "total");
    lua_pushnumber(L, keysTotal);
    lua_settable(L, -3);

    lua_pushstring(L, "temp");
    lua_pushnumber(L, tempKeys);
    lua_settable(L, -3);

    lua_pushstring(L, "active");
    lua_pushnumber(L, activeKeys);
    lua_settable(L, -3);
    lua_settable(L, -3);

    // Scripts
    lua_pushstring(L, "scripts");
    lua_newtable(L);

    lua_pushstring(L, "total");
    lua_pushnumber(L, scriptsTotal);
    lua_settable(L, -3);

    lua_pushstring(L, "temp");
    lua_pushnumber(L, tempScripts);
    lua_settable(L, -3);

    lua_pushstring(L, "active");
    lua_pushnumber(L, activeScripts);
    lua_settable(L, -3);
    lua_settable(L, -3);

    return 1;
}
