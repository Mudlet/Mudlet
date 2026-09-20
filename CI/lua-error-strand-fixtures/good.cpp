/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

// Fixture: shapes that are safe and must NOT be reported. Not compiled.
int TLuaInterpreter::literalNameIsFree(lua_State* L)
{
    const char* funcName = "stt.init";
    const QString path = getVerifiedString(L, funcName, 1, "path");
    return 0;
}

int TLuaInterpreter::qslIsStaticStorage(lua_State* L)
{
    const QString label = qsl("main");
    const int id = getVerifiedInt(L, __func__, 1, "id");
    return 0;
}

int TLuaInterpreter::temporaryOnTheResultIsFine(lua_State* L)
{
    const QString text = getVerifiedString(L, __func__, 1, "text").trimmed();
    return 0;
}

int TLuaInterpreter::emptinessIsTheRaiseCondition(lua_State* L)
{
    const QString direction = dirToString(L, 2);
    if (direction.isEmpty()) {
        return lua_error(L);
    }
    return 0;
}

int TLuaInterpreter::declaredInABranchThatDoesNotRaise(lua_State* L)
{
    if (lua_isstring(L, 1)) {
        const QString existingName{lua_tostring(L, 1)};
        return warnArgumentValue(L, __func__, "no");
    } else {
        return lua_error(L);
    }
}

int TLuaInterpreter::preGatedRaiseIsDead(lua_State* L)
{
    if (!checkCommandOrFunctionArg(L, __func__, commandPos)) {
        return lua_error(L);
    }
    const QString windowName{lua_tostring(L, 1)};
    QString command;
    int luaReference = 0;
    parseCommandOrFunction(L, __func__, commandPos, command, luaReference);
    return 0;
}
