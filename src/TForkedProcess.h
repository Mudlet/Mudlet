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

#ifndef FORKED_PROCESS_H
#define FORKED_PROCESS_H

#include "TLuaInterpreter.h"

#include<QProcess>

extern "C"
{
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}
class TLuaInterpreter;

class TForkedProcess : public QProcess {

Q_OBJECT

public:
    virtual ~TForkedProcess();

    static int startProcess( TLuaInterpreter *, lua_State *);

private:
    TForkedProcess( TLuaInterpreter *, lua_State * );

    int callBackFunctionRef;
    TLuaInterpreter *interpreter;
    bool running;

    static int closeInputOfProcess ( lua_State * L );
    static int isProcessRunning ( lua_State * L );
    static int sendMessage( lua_State * L );

private slots:
    void slotReceivedData();
    void slotFinish();
};

#endif

