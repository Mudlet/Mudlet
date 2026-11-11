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

// Fixture: every shape the scanner must catch. Not compiled.
int TLuaInterpreter::strandsALocal(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "name");
    const int id = getVerifiedInt(L, __func__, 2, "id");  // STRAND
    return 0;
}

int TLuaInterpreter::strandsATemporary(lua_State* L)
{
    const QString funcName = qsl("stt.init");
    const QString path = getVerifiedString(L, funcName.toUtf8().constData(), 1, "path");  // STRAND
    return 0;
}

int TLuaInterpreter::strandsAcrossARawRaise(lua_State* L)
{
    const QByteArray payload{lua_tostring(L, 1)};
    if (somethingIsWrong()) {
        return lua_error(L);  // STRAND
    }
    return 0;
}

int TLuaInterpreter::strandsUnderAWrappedSignature(
        lua_State* L)
{
    const QString name{lua_tostring(L, 1)};
    return lua_error(L);  // STRAND
}

// A pre-gate on a different argument must not suppress: argument 1 is checked,
// argument 2 is not, so getVerifiedString can still raise while key is live.
int TLuaInterpreter::preGateOnAnotherArgument(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "key")) {
        return lua_error(L);
    }
    const QString key{lua_tostring(L, 1)};
    const QString value = getVerifiedString(L, __func__, 2, "value");  // STRAND
    return 0;
}

// isEmpty() as an early-return guard leaves the variable proven NON-empty, so
// the raise below it still strands.
int TLuaInterpreter::emptyGuardThenRaise(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "name");
    if (name.isEmpty()) {
        return warnArgumentValue(L, __func__, "empty");
    }
    const int id = getVerifiedInt(L, __func__, 2, "id");  // STRAND
    return 0;
}

int TLuaInterpreter::strandsAnAutoLocal(lua_State* L)
{
    auto name = getVerifiedString(L, __func__, 1, "name");
    const int id = getVerifiedInt(L, __func__, 2, "id");  // STRAND
    return 0;
}

int TLuaInterpreter::strandsAContainerFilledAfterDeclaration(lua_State* L)
{
    QStringList results;
    results << QString::fromUtf8(lua_tostring(L, 1));
    const int id = getVerifiedInt(L, __func__, 2, "id");  // STRAND
    return 0;
}

int TLuaInterpreter::strandsAcrossTheThirdTableParser(lua_State* L)
{
    const QString windowName{lua_tostring(L, 1)};
    QStringList commandList;
    QVector<int> luaReferences;
    parseCommandsOrFunctionsTable(L, __func__, 2, commandList, luaReferences);  // STRAND
    return 0;
}

// A checker inside a branch that has closed again does not dominate the raise,
// so it cannot vouch for it.
int TLuaInterpreter::gateInsideABranch(lua_State* L)
{
    if (lua_gettop(L) > 2) {
        if (!checkStringArg(L, __func__, 2, "value")) {
            return lua_error(L);
        }
    }
    const QString key{lua_tostring(L, 1)};
    const QString value = getVerifiedString(L, __func__, 2, "value");  // STRAND
    return 0;
}

class Indented
{
    static int wrappedAndIndented(
            lua_State* L)
    {
        const QString name{lua_tostring(L, 1)};
        return lua_error(L);  // STRAND
    }
};

int TLuaInterpreter::braceInsideABlockComment(lua_State* L)
{
    const QString name{lua_tostring(L, 1)};
    /* a block comment with a stray } in it, which would close this function's
       scope early and drop every variable tracked in it */
    return lua_error(L);  // STRAND
}
