//#include <QList>
//#include <QMap>

#include <LuaInterface.h>
//#include <TVar.h>
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

private slots:

    void initTestCase()
    {

    }

    void testRetrieveStrings()
    {
        lua_State* L = luaL_newstate();
        LuaInterface* interface = new LuaInterface(L);
        luaL_loadstring(L, QString("test = 1").toUtf8().constData());
        lua_pcall(L, 0, 0, 0);
        interface->getVars(false);
        VarUnit* vu = interface->getVarUnit();
        TVar* base = vu->getBase();
        QList<TVar*> children = base->getChildren();
        qDebug() << "children are" << children;

    }

    void cleanupTestCase()
    {
    }
};

#include "TLuaInterfaceTest.moc"
QTEST_MAIN(TVarTest)
