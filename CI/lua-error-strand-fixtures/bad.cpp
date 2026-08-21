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
