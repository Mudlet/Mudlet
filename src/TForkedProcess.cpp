/***************************************************************************
 *   Copyright (C) 2009 by Benjamin Lerman - mudlet@ambre.net              *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Christer Oscarsson                              *
 *                                          - christer.oscarsson@gmail.com *
 *   Copyright (C) 2020, 2022 by Stephen Lyons - slysven@virginmedia.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "TForkedProcess.h"

#include <QDir>


TForkedProcess::~TForkedProcess()
{
    if (callBackFunctionRef != -1) {
        luaL_unref(mpInterpreter->pGlobalLua, LUA_REGISTRYINDEX, callBackFunctionRef);
    }
}


TForkedProcess::TForkedProcess(TLuaInterpreter* pInterpreter, lua_State* L)
: QProcess()
, mpInterpreter(pInterpreter)
{
    int n = lua_gettop(L);
    if (n < 2) {
        lua_pushstring(L, "Need read function and process name as parameters.");
        lua_error(L);
    }

    if (!lua_isfunction(L, 1)) {
        lua_pushstring(L, "Need read function as first parameter.");
        lua_error(L);
    }

    lua_pushvalue(L, 1);
    callBackFunctionRef = luaL_ref(L, LUA_REGISTRYINDEX);


    QString prog{luaL_checkstring(L, 2)};
    QStringList args;
    for (int i = 3; i <= n; i++) {
        args << luaL_checkstring(L, i);
    }

    // QProcess::finished is overloaded so we have to say which form we are
    // connecting here
    connect(this, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), mpInterpreter, &TLuaInterpreter::slot_deleteSender);
    connect(this, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &TForkedProcess::slot_finished);
    connect(this, &QProcess::readyReadStandardOutput, this, &TForkedProcess::slot_receivedData);

    setProcessChannelMode(QProcess::MergedChannels);
    start(prog, args, QIODevice::ReadWrite);
    if (!waitForStarted()) {
        const QString errorMessage = qsl("Failed to start process '%1': %2. Working directory: '%3'. PATH: '%4'")
                                             .arg(prog, errorString(), QDir::currentPath(), qEnvironmentVariable("PATH"));
        lua_pushstring(L, errorMessage.toUtf8().constData());
        lua_error(L);
        return;
    }
    running = true;
}

void TForkedProcess::slot_finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
    running = false;
}

void TForkedProcess::slot_receivedData()
{
    while (canReadLine()) {
        QByteArray line = readLine();
        // Call lua function by stored Reference
        lua_rawgeti(mpInterpreter->pGlobalLua, LUA_REGISTRYINDEX, callBackFunctionRef);
        lua_pushstring(mpInterpreter->pGlobalLua, line.data());
        lua_pcall(mpInterpreter->pGlobalLua, 1, 0, 0);
    }
}

int TForkedProcess::sendMessage(lua_State* L)
{
    QPointer<TForkedProcess>* forkedProcess = *((QPointer<TForkedProcess>**)lua_topointer(L, lua_upvalueindex(1)));

    if (!forkedProcess || forkedProcess->isNull() || !(*forkedProcess)->running) {
        lua_pushstring(L, "Unable to send data to process. Process has ended.");
        return lua_error(L);
    }

    size_t stringLength = 0;
    const char* toWrite = lua_tolstring(L, 1, &stringLength);
    if (!toWrite) {
        lua_pushstring(L, "Unable to get data to send.");
        return lua_error(L);
    }
    size_t writedBytes = 0;
    while (stringLength > writedBytes) {
        int res = (*forkedProcess)->write(toWrite + writedBytes, stringLength - writedBytes);
        if (res == -1) {
            lua_pushstring(L, "Unable to send data to process.");
            return lua_error(L);
        }
        writedBytes += res;
    }
    return 0;
}

int TForkedProcess::isProcessRunning(lua_State* L)
{
    QPointer<TForkedProcess>* forkedProcess = *((QPointer<TForkedProcess>**)lua_topointer(L, lua_upvalueindex(1)));

    bool running = (forkedProcess != nullptr && !forkedProcess->isNull() && (*forkedProcess)->running);
    lua_pushboolean(L, running);
    return 1;
}

int TForkedProcess::closeInputOfProcess(lua_State* L)
{
    QPointer<TForkedProcess>* forkedProcess = *((QPointer<TForkedProcess>**)lua_topointer(L, lua_upvalueindex(1)));

    if (!forkedProcess || forkedProcess->isNull() || !(*forkedProcess)->running) {
        // Process is already finished. Nothing to do.
        return 0;
    }
    (*forkedProcess)->closeWriteChannel();
    return 0;
}

static int qPointerGC(lua_State* L)
{
    QPointer<TForkedProcess>* forkedProcessPointer = *((QPointer<TForkedProcess>**)lua_topointer(L, 1));
    delete forkedProcessPointer;
    lua_pushboolean(L, true);
    return 1;
}


int TForkedProcess::startProcess(TLuaInterpreter* pInterpreter, lua_State* L)
{
    auto process = new TForkedProcess(pInterpreter, L);

    // The userdata for the closures.
    auto ** luaMemory = (QPointer<TForkedProcess>**)lua_newuserdata(L, sizeof(QPointer<TForkedProcess>*));
    int userDataIndex = lua_gettop(L);
    if (lua_getmetatable(L, userDataIndex) != 0) {
        lua_pushstring(L, "Error: new user data should not have any metatable.");
        return lua_error(L);
    }
    if (luaL_newmetatable(L, "qPointerGCMetatable") == 1) {
        // First time one call this method. One must register the garbage collection method.
        int tableIndex = lua_gettop(L);
        lua_pushstring(L, "__gc");
        lua_pushcfunction(L, qPointerGC);
        lua_settable(L, tableIndex);
    }
    lua_setmetatable(L, userDataIndex);

    *luaMemory = new QPointer<TForkedProcess>(process);

    // One must return a table with the following function:
    // send( a ) -> () : to send a to the process
    // close() -> () : to close the input of the process
    // isRunning() -> bool : to know if the process is still running.

    lua_newtable(L);
    int tableIndex = lua_gettop(L);

    // The name of the send function
    lua_pushstring(L, "send");
    // The metadatcontaining the process
    lua_pushvalue(L, userDataIndex);
    // The send function
    lua_pushcclosure(L, TForkedProcess::sendMessage, 1);
    // Set the table for the send function.
    lua_settable(L, tableIndex);

    // The name of the close function
    lua_pushstring(L, "close");
    // The metadatcontaining the process
    lua_pushvalue(L, userDataIndex);
    // The close function
    lua_pushcclosure(L, TForkedProcess::closeInputOfProcess, 1);
    // Set the table for the close function.
    lua_settable(L, tableIndex);

    lua_pushstring(L, "isRunning");
    // The metadatcontaining the function
    lua_pushvalue(L, userDataIndex);
    // The isRunning function
    lua_pushcclosure(L, TForkedProcess::isProcessRunning, 1);
    // Set the table for the isRunning function.
    lua_settable(L, tableIndex);

    return 1;
}
