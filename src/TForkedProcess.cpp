/***************************************************************************
 *   Copyright (C) 2009 by Benjamin Lerman                                 *
 *   mudlet@ambre.net                                                      *
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

#include <QPointer>

TForkedProcess::~TForkedProcess()
{
    if (callBackFunctionRef != -1 ) 
    {
        luaL_unref(interpreter->pGlobalLua, LUA_REGISTRYINDEX, callBackFunctionRef);
    }
}


TForkedProcess::TForkedProcess( TLuaInterpreter * interpreter, lua_State * L )
:QProcess()
{
    this->interpreter = interpreter;
    int n = lua_gettop(L);
    callBackFunctionRef = -1;
    if(n < 2) {
        lua_pushstring(L, "Need read function and process name as parameters.");
        lua_error(L);
    }

    if(!lua_isfunction(L, 1)) {
        lua_pushstring(L, "Need read function as first parameter.");
        lua_error(L);
    }

    lua_pushvalue( L, 1);
    callBackFunctionRef = luaL_ref(L, LUA_REGISTRYINDEX);


    QString prog = QString((char*)luaL_checkstring(L,  2));
    QStringList args;
    for(int i = 3; i <= n; i++) {
        args << ((char*)luaL_checkstring(L,  i));
    }

    connect(this, SIGNAL(finished(int)), interpreter, SLOT(slotDeleteSender()));
    connect(this, SIGNAL(finished(int)), this, SLOT(slotFinish()));
    connect(this, SIGNAL(readyReadStandardOutput ()), this, SLOT(slotReceivedData()));

    setReadChannelMode(QProcess::MergedChannels);
    start(prog, args, QIODevice::ReadWrite);
    waitForStarted();
    running = true;
}

void TForkedProcess::slotFinish() {
    running = false;
}

void TForkedProcess::slotReceivedData() {
    if(canReadLine ()) {
        QByteArray line = readLine();
        // Call lua function by stored Reference
        lua_rawgeti(interpreter->pGlobalLua, LUA_REGISTRYINDEX, callBackFunctionRef);
        lua_pushstring(interpreter->pGlobalLua, line.data());
        lua_pcall(interpreter->pGlobalLua, 1, 0, 0);
    }
}

int TForkedProcess::sendMessage( lua_State * L ) {
    QPointer<TForkedProcess> * forkedProcess = *((QPointer<TForkedProcess> **) lua_topointer(L, lua_upvalueindex(1)));

    if(forkedProcess == 0 || forkedProcess->isNull() || !(*forkedProcess)->running) {
        lua_pushstring(L, "Unable to send data to process. Process has ended.");
        lua_error(L);
    }

    size_t stringLength = 0;
    const char *toWrite = lua_tolstring (L, 1, &stringLength);
    if(toWrite == 0) {
        lua_pushstring(L, "Unable to get data to send.");
        lua_error(L);
    }
    size_t writedBytes = 0;
    while(stringLength > writedBytes) {
        int res = (*forkedProcess)->write(toWrite + writedBytes, stringLength - writedBytes);
        if(res == -1) {
            lua_pushstring(L, "Unable to send data to process.");
            lua_error(L);
        }
        writedBytes += res;
    }
    return 0;
}

int TForkedProcess::isProcessRunning ( lua_State * L ) {
    QPointer<TForkedProcess> * forkedProcess = *((QPointer<TForkedProcess> **) lua_topointer(L, lua_upvalueindex(1)));

    bool running = (forkedProcess != 0 && !forkedProcess->isNull() && (*forkedProcess)->running);
    lua_pushboolean(L, running);
    return 1;
}

int TForkedProcess::closeInputOfProcess ( lua_State * L ) {
    QPointer<TForkedProcess> * forkedProcess = *((QPointer<TForkedProcess> **) lua_topointer(L, lua_upvalueindex(1)));

    if(forkedProcess == 0 || forkedProcess->isNull() || !(*forkedProcess)->running) {
        // Process is already finished. Nothing to do.
        return 0;
    }
    (*forkedProcess)->closeWriteChannel ();
		return 0;
}

static int qPointerGC ( lua_State * L ) {
    QPointer<TForkedProcess> * forkedProcessPointer = *((QPointer<TForkedProcess> **) lua_topointer(L, 1));
    delete forkedProcessPointer;
    lua_pushboolean(L, true);
    return 1;
}


int TForkedProcess::startProcess( TLuaInterpreter * interpreter, lua_State * L ) {
    TForkedProcess * process = new TForkedProcess(interpreter, L);

    // The userdata for the closures.
    QPointer<TForkedProcess> ** luaMemory = (QPointer<TForkedProcess> **)lua_newuserdata (L, sizeof(QPointer<TForkedProcess> *));
    int userDataIndex = lua_gettop(L);
    if(lua_getmetatable (L, userDataIndex) != 0) {
        lua_pushstring(L, "Error: new user data should not have any metatable.");
        lua_error(L);
    } else {
        if(luaL_newmetatable(L, "qPointerGCMetatable") == 1) {
            // First time one call this method. One must register the garbage collection method.
            int tableIndex = lua_gettop(L);
            lua_pushstring(L, "__gc");
            lua_pushcfunction(L, qPointerGC); 
            lua_settable(L, tableIndex);
        }
        lua_setmetatable(L, userDataIndex);
    }
    *luaMemory = new QPointer<TForkedProcess>(process);

    // One must return a table with the following function:
    // send( a ) -> () : to send a to the process
    // close() -> () : to close the input of the process
    // isRunning() -> bool : to know if the process is still running.

    lua_newtable( L );
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

