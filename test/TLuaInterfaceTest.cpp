#include <LuaInterface.h>
#include <TVar.h>
#include <VarUnit.h>
#include <QtTest/QtTest>

extern "C" {
    #include <lauxlib.h>
    #include <lua.h>
    #include <lualib.h>
}

class TVarTest : public QObject {
Q_OBJECT

private:
    lua_State* L;
    LuaInterface* interface;

private slots:

    void initTestCase()
    {
        L = luaL_newstate();
        interface = new LuaInterface(L);
    }

    void execLua(QString string) {
        luaL_loadstring(L, string.toUtf8().constData());
        lua_pcall(L, 0, 0, 0);
    }

    void testRetrieveStrings()
    {
        execLua("test = '1'");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar* base = vu->getBase();
        QList<TVar*> children = base->getChildren();
        TVar* testVar = children.first();
        QCOMPARE(testVar->getName(), "test");
        QCOMPARE(testVar->getValue(), "1");
        QCOMPARE(testVar->getValueType(), LUA_TSTRING);
    }

    void testRetrieveNumber()
    {
        execLua("test = 1");
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar* base = vu->getBase();
        QList<TVar*> children = base->getChildren();
        TVar* testVar = children.first();
        QCOMPARE(testVar->getName(), "test");
        QCOMPARE(testVar->getValue(), "1");
        QCOMPARE(testVar->getValueType(), LUA_TNUMBER);
    }

    void cleanupTestCase()
    {
    }
};

#include "TLuaInterfaceTest.moc"
QTEST_MAIN(TVarTest)
