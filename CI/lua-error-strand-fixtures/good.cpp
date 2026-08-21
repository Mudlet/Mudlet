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
