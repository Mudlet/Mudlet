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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*
 * A save from the editor syncs modules: the end of the save reloads them in the
 * other profiles that have them, and so runs those profiles' module scripts while
 * this profile's save still reads as running. One that raises a global event
 * reaches this profile's handlers there, and a script install from one of those
 * must not wait for that save, which cannot finish until the install returns.
 * No spec reaches this: only the editor's Save and Import ask for a save that
 * syncs modules.
 *
 * Run with: ctest -R ModuleSyncSaveReentryTest -V
 */

#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#else
#include <lauxlib.h>
#include <lua.h>
#endif
}

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ModuleSyncSaveReentryTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpSavingHost = nullptr;
    Host* mpOtherHost = nullptr;
    const QString mSavingProfile = qsl("ModuleSyncReentry-Saving");
    const QString mOtherProfile = qsl("ModuleSyncReentry-Other");
    const QString mModuleName = qsl("module-sync-reentry");
    const QString mLocalhost = qsl("localhost");
    QString mPort;
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // A synced module has its file rewritten by every save, so it lives out of the repository's sight
    QTemporaryDir mModuleDir;
    QString mModulePath;

    QString runLua(Host* pHost, const QString& code) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) == 0) {
            return QString();
        }
        const char* message = lua_tostring(L, -1);
        const QString error = message ? QString::fromUtf8(message) : qsl("(a Lua error that is not a string)");
        lua_pop(L, 1);
        return error;
    }

    QString luaGlobalString(Host* pHost, const QString& globalName) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const char* value = lua_tostring(L, -1);
        const QString result = value ? QString::fromUtf8(value) : QString();
        lua_pop(L, 1);
        return result;
    }

    bool writeModule()
    {
        mModulePath = mModuleDir.filePath(qsl("%1.xml").arg(mModuleName));
        QFile file(mModulePath);
        if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
            return false;
        }
        const QByteArray xml("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                             "<!DOCTYPE MudletPackage>\n"
                             "<MudletPackage version=\"1.001\">\n"
                             "<ScriptPackage>\n"
                             "<Script isActive=\"yes\" isFolder=\"no\">\n"
                             "<name>module-sync-reentry script</name>\n"
                             "<packageName></packageName>\n"
                             "<script>raiseGlobalEvent(\"mudletTestModuleReloaded\")</script>\n"
                             "<eventHandlerList/>\n"
                             "</Script>\n"
                             "</ScriptPackage>\n"
                             "</MudletPackage>\n");
        return file.write(xml) == xml.size();
    }

    bool installModule(Host* pHost)
    {
        pHost->waitForProfileSave();
        auto [installed, message] = pHost->installPackage(mModulePath, enums::PackageModuleType::ModuleFromScript, true);
        if (!installed) {
            qWarning().noquote() << "installing the module in" << pHost->getName() << "failed:" << message;
            return false;
        }
        return pHost->mModulesLoadedOk.contains(mModuleName);
    }

    static bool settle(Host* pHost)
    {
        // installing a module owes the profile a save; let it come and go
        if (!QTest::qWaitFor(
                    [pHost]() {
                        return !pHost->hasPendingProfileSave();
                    },
                    5s)) {
            return false;
        }
        pHost->waitForProfileSave();
        return !pHost->currentlySavingProfile();
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mModuleDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        MudletApp::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mpSavingHost = TestProfile::create(mSavingProfile, mLocalhost, mPort);
        QVERIFY2(mpSavingHost, "the saving profile did not open");

        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, mOtherProfile)));
        QVERIFY(MudletApp::writeProfileData(mOtherProfile, qsl("url"), mLocalhost).first);
        QVERIFY(MudletApp::writeProfileData(mOtherProfile, qsl("port"), mPort).first);
        QVERIFY2(runLua(mpSavingHost, qsl("loadProfile('%1', true)").arg(mOtherProfile)).isNull(), "the other profile could not be loaded");
        QVERIFY(QTest::qWaitFor(
                [this]() {
                    return HostManager::self()->getHost(mOtherProfile) != nullptr;
                },
                5s));
        mpOtherHost = HostManager::self()->getHost(mOtherProfile);
        QVERIFY2(mpOtherHost->mpConsole, "the other profile has no console, so a module sync passes it by");

        QVERIFY2(writeModule(), "could not write the module");
        QVERIFY2(installModule(mpSavingHost), "the module could not be installed in the saving profile");
        QVERIFY2(installModule(mpOtherHost), "the module could not be installed in the other profile");
        // A sync only reaches the modules a profile has a priority for, which installing alone does not give
        QVERIFY(runLua(mpOtherHost, qsl("setModulePriority('%1', 1)").arg(mModuleName)).isNull());
        auto [synced, syncMessage] = mpSavingHost->changeModuleSync(mModuleName, QLatin1String("1"));
        QVERIFY2(synced, qPrintable(syncMessage));
        QVERIFY(settle(mpSavingHost));
        QVERIFY(settle(mpOtherHost));
    }

    void cleanupTestCase()
    {
        for (Host* pHost : {mpSavingHost, mpOtherHost}) {
            if (pHost) {
                pHost->waitForProfileSave();
            }
        }
        mpSavingHost = nullptr;
        mpOtherHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_aScriptInstallReachedFromTheModuleSyncDoesNotWaitForTheSave()
    {
        QVERIFY(runLua(mpSavingHost, qsl(R"(
mudletTestNestedInstall = nil
registerAnonymousEventHandler("mudletTestModuleReloaded", function()
  local ok, err = installPackage("")
  mudletTestNestedInstall = tostring(ok) .. "|" .. tostring(err)
end, true)
)"))
                        .isNull());

        QElapsedTimer elapsed;
        elapsed.start();
        auto [ok, filename, error] = mpSavingHost->saveProfile(QString(), QString(), true);
        QVERIFY2(ok, qPrintable(error));
        mpSavingHost->waitForProfileSave();
        const auto took = elapsed.elapsed();

        const QString answer = luaGlobalString(mpSavingHost, qsl("mudletTestNestedInstall"));
        QVERIFY2(!answer.isEmpty(), "the other profile's module was not reloaded, so no install was reached from the save");
        QVERIFY2(took < 10000, qPrintable(qsl("the save took %1 ms to finish: the install inside it waited for it").arg(took)));
        QVERIFY2(answer.contains(qsl("profile save is still in progress")), qPrintable(answer));
        QVERIFY(!mpSavingHost->currentlySavingProfile());
    }
};

#include "ModuleSyncSaveReentryTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModuleSyncSaveReentryTest)
