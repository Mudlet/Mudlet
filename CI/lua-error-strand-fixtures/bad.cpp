// Fixture: every shape the scanner must catch. Not compiled.
int TLuaInterpreter::strandsALocal(lua_State* L)
{
    const QString name = getVerifiedString(L, __func__, 1, "name");
    const int id = getVerifiedInt(L, __func__, 2, "id");
    return 0;
}

int TLuaInterpreter::strandsATemporary(lua_State* L)
{
    const QString funcName = qsl("stt.init");
    const QString path = getVerifiedString(L, funcName.toUtf8().constData(), 1, "path");
    return 0;
}

int TLuaInterpreter::strandsAcrossARawRaise(lua_State* L)
{
    const QByteArray payload{lua_tostring(L, 1)};
    if (somethingIsWrong()) {
        return lua_error(L);
    }
    return 0;
}

int TLuaInterpreter::strandsUnderAWrappedSignature(
        lua_State* L)
{
    const QString name{lua_tostring(L, 1)};
    return lua_error(L);
}

// A pre-gate on a different argument must not suppress: argument 1 is checked,
// argument 2 is not, so getVerifiedString can still raise while key is live.
int TLuaInterpreter::preGateOnAnotherArgument(lua_State* L)
{
    if (!checkStringArg(L, __func__, 1, "key")) {
        return lua_error(L);
    }
    const QString key{lua_tostring(L, 1)};
    const QString value = getVerifiedString(L, __func__, 2, "value");
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
    const int id = getVerifiedInt(L, __func__, 2, "id");
    return 0;
}

int TLuaInterpreter::strandsAnAutoLocal(lua_State* L)
{
    auto name = getVerifiedString(L, __func__, 1, "name");
    const int id = getVerifiedInt(L, __func__, 2, "id");
    return 0;
}

int TLuaInterpreter::strandsAContainerFilledAfterDeclaration(lua_State* L)
{
    QStringList results;
    results << QString::fromUtf8(lua_tostring(L, 1));
    const int id = getVerifiedInt(L, __func__, 2, "id");
    return 0;
}

int TLuaInterpreter::strandsAcrossTheThirdTableParser(lua_State* L)
{
    const QString windowName{lua_tostring(L, 1)};
    QStringList commandList;
    QVector<int> luaReferences;
    parseCommandsOrFunctionsTable(L, __func__, 2, commandList, luaReferences);
    return 0;
}
