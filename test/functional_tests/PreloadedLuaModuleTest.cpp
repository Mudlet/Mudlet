/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

/*
 * The Lua C modules an app that embeds Mudlet links in and names with THeadlessSession::preloadLuaModule(),
 * as it cannot load one from a file. Each test makes a session of its own, and with it the process's one
 * profile pool.
 */

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"
#include "TLuaInterpreter.h"

#include "GroupedTest.h"

namespace {

// As a module's luaopen_ function: called with the name required, returns the module
int openTestModule(lua_State* L)
{
    lua_newtable(L);
    lua_pushvalue(L, 1);
    lua_setfield(L, -2, "requiredAs");
    lua_pushliteral(L, "the app");
    lua_setfield(L, -2, "from");
    return 1;
}

int openLaterTestModule(lua_State* L)
{
    lua_newtable(L);
    lua_pushliteral(L, "the app, later");
    lua_setfield(L, -2, "from");
    return 1;
}

} // namespace

class PreloadedLuaModuleTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeProfile(const QString& name)
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host></Host></HostPackage>
</MudletPackage>
)");
        return true;
    }

    // A Lua module in the profile's folder, where require() looks first of all on disk
    static bool writeModuleOnDisk(const QString& profile, const QString& name)
    {
        QFile file(qsl("%1/%2.lua").arg(MudletApp::getMudletPath(enums::profileHomePath, profile), name));
        return file.open(QIODevice::WriteOnly) && file.write("return { from = 'disk' }\n") > 0;
    }

    // What tostring() makes of the Lua expression
    static QString luaValue(THeadlessSession& session, const QString& expression)
    {
        if (!session.runLua(qsl("preloadedModuleTestValue = tostring(%1)").arg(expression))) {
            return qsl("<%1 failed>").arg(expression);
        }
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "preloadedModuleTestValue");
        const QString value = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return value;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        QVERIFY2(!HostManager::self(), "A profile pool already exists, so a session cannot make its own.");
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void test_aProfileRequiresTheModulesTheAppNamed()
    {
        const QString name = qsl("Preloaded Lua Module");
        QVERIFY(writeProfile(name));
        QVERIFY(writeModuleOnDisk(name, qsl("preloadedtest_ondisk")));
        QVERIFY(writeModuleOnDisk(name, qsl("preloadedtest_both")));
        THeadlessSession::preloadLuaModule("preloadedtest.linked", openTestModule);
        THeadlessSession::preloadLuaModule("preloadedtest_both", openTestModule);
        THeadlessSession::preloadLuaModule("preloadedtest.twice", openTestModule);
        THeadlessSession::preloadLuaModule("preloadedtest.twice", openLaterTestModule);
        // One of the modules Mudlet itself requires, which nothing of Mudlet's uses as it loads
        THeadlessSession::preloadLuaModule("lpeg", openTestModule);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused the Lua module \"preloadedtest.nothing\"")));
        THeadlessSession::preloadLuaModule("preloadedtest.nothing", nullptr);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused the Lua module \"\"")));
        THeadlessSession::preloadLuaModule("", openTestModule);

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);

        QCOMPARE(luaValue(session, qsl("require('preloadedtest.linked').from")), qsl("the app"));
        QCOMPARE(luaValue(session, qsl("require('preloadedtest.linked').requiredAs")), qsl("preloadedtest.linked"));
        QCOMPARE(luaValue(session, qsl("package.loaded['preloadedtest.linked'] == require('preloadedtest.linked')")), qsl("true"));
        // Ahead of a module on disk, which is still found where the app named none
        QCOMPARE(luaValue(session, qsl("require('preloadedtest_both').from")), qsl("the app"));
        QCOMPARE(luaValue(session, qsl("require('preloadedtest_ondisk').from")), qsl("disk"));
        QCOMPARE(luaValue(session, qsl("require('preloadedtest.twice').from")), qsl("the app, later"));
        QCOMPARE(luaValue(session, qsl("lpeg.from")), qsl("the app"));
        QCOMPARE(luaValue(session, qsl("lpeg.requiredAs")), qsl("lpeg"));
        // Mudlet's other modules load as before
        QCOMPARE(luaValue(session, qsl("type(lfs.attributes)")), qsl("function"));
        QCOMPARE(luaValue(session, qsl("package.preload['preloadedtest.nothing']")), qsl("nil"));
        QCOMPARE(luaValue(session, qsl("package.preload['']")), qsl("nil"));

        // One named while the profile is open is for the Lua state its reset makes
        THeadlessSession::preloadLuaModule("preloadedtest.late", openTestModule);
        QCOMPARE(luaValue(session, qsl("package.preload['preloadedtest.late']")), qsl("nil"));
        QVERIFY(session.runLua(qsl("preloadedModuleTestBeforeReset = true")));
        QVERIFY(session.host()->resetProfile_phase1());
        QTRY_COMPARE(luaValue(session, qsl("preloadedModuleTestBeforeReset")), qsl("nil"));
        QCOMPARE(luaValue(session, qsl("require('preloadedtest.linked').from")), qsl("the app"));
        QCOMPARE(luaValue(session, qsl("require('preloadedtest.late').from")), qsl("the app"));
    }
};

#include "PreloadedLuaModuleTest.moc"
MUDLET_GROUPED_TEST_MAIN(PreloadedLuaModuleTest)
