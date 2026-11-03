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
 * cannot wait for that save, which cannot finish until the install returns: it is
 * put off until the save is done instead.
 * No spec reaches this: only the editor's Save and Import ask for a save that
 * syncs modules. Nor can one hold a save that never reports itself done, which
 * the last test here needs.
 *
 * Run with: ctest -R ModuleSyncSaveReentryTest -V
 */

#include <QElapsedTimer>
#include <QScopeGuard>
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

    QString writeArchive(const QString& name, const QByteArray& content)
    {
        const QString path = mModuleDir.filePath(name);
        QFile file(path);
        if (!file.open(QFile::WriteOnly | QFile::Truncate) || file.write(content) != content.size()) {
            return QString();
        }
        return path;
    }

    QString writePackage(const QString& name)
    {
        return writeArchive(qsl("%1.xml").arg(name),
                            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE MudletPackage>\n<MudletPackage version=\"1.001\">\n<ScriptPackage>\n"
                            "<Script isActive=\"yes\" isFolder=\"no\">\n<name>reentry package script</name>\n<packageName></packageName>\n<script></script>\n"
                            "<eventHandlerList/>\n</Script>\n</ScriptPackage>\n</MudletPackage>\n");
    }

    // Saves with module sync, so the other profile's module raises the event the given Lua answers, and
    // hands back how long the save took to finish
    qint64 saveReachingBackWith(const QString& install, const QString& path)
    {
        // The reload the previous test gave the other profile owes it a save of its own. While that is still
        // being written the reload is put off until it is done, so it would run after this save, not within it
        if (!settle(mpOtherHost) || !settle(mpSavingHost)) {
            qWarning().noquote() << "a profile was still saving before the save that syncs modules";
            return -1;
        }
        const QString registered = runLua(mpSavingHost,
                                          qsl(R"(
mudletTestNestedInstall = nil
registerAnonymousEventHandler("mudletTestModuleReloaded", function()
  local ok, err = %1([[%2]])
  mudletTestNestedInstall = tostring(ok) .. "|" .. tostring(err)
end, true)
)")
                                                  .arg(install, path));
        if (!registered.isNull()) {
            qWarning().noquote() << "registering the handler failed:" << registered;
            return -1;
        }
        QElapsedTimer elapsed;
        elapsed.start();
        auto [ok, filename, error] = mpSavingHost->saveProfile(QString(), QString(), true);
        if (!ok) {
            qWarning().noquote() << "the save was refused:" << error;
            return -1;
        }
        mpSavingHost->waitForProfileSave();
        return elapsed.elapsed();
    }

    QString mainConsoleText()
    {
        runLua(mpSavingHost, qsl("mudletTestConsoleText = table.concat(getLines('main', 0, getLastLineNumber('main') + 1), '\\n')"));
        return luaGlobalString(mpSavingHost, qsl("mudletTestConsoleText"));
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
        QVERIFY2(mpOtherHost->mainConsoleView(), "the other profile has no console, so a module sync passes it by");

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

    void test_aScriptInstallReachedFromTheModuleSyncIsPutOffUntilTheSaveIsDone()
    {
        const QString packageName = qsl("module-sync-reentry-package");
        const QString path = writePackage(packageName);
        QVERIFY(!path.isEmpty());

        const auto took = saveReachingBackWith(qsl("installPackage"), path);
        QVERIFY(took >= 0);

        const QString answer = luaGlobalString(mpSavingHost, qsl("mudletTestNestedInstall"));
        QVERIFY2(!answer.isEmpty(), "the other profile's module was not reloaded, so no install was reached from the save");
        QCOMPARE(answer, qsl("true|nil"));
        QVERIFY2(took < 10000, qPrintable(qsl("the save took %1 ms to finish: the install inside it waited for it").arg(took)));
        QVERIFY(QTest::qWaitFor(
                [this, &packageName]() {
                    return mpSavingHost->mInstalledPackages.contains(packageName);
                },
                5s));
        QVERIFY(settle(mpSavingHost));
        QVERIFY(mpSavingHost->uninstallPackage(packageName, enums::PackageModuleType::Package));
        QVERIFY(settle(mpSavingHost));
    }

    void test_aScriptModuleInstallReachedFromTheModuleSyncIsPutOffUntilTheSaveIsDone()
    {
        const QString nestedModule = qsl("module-sync-reentry-nested");
        const QString path = writePackage(nestedModule);
        QVERIFY(!path.isEmpty());

        // The Module Manager refreshes on this, so it has to come once the module is there to list
        QList<bool> listedOnChange;
        const auto connection = connect(mpSavingHost, &Host::signal_moduleListChangedByScript, this, [this, &listedOnChange, &nestedModule]() {
            listedOnChange << mpSavingHost->mInstalledModules.contains(nestedModule);
        });
        const auto disconnectGuard = qScopeGuard([connection]() {
            disconnect(connection);
        });

        const auto took = saveReachingBackWith(qsl("installModule"), path);
        QVERIFY(took >= 0);

        const QString answer = luaGlobalString(mpSavingHost, qsl("mudletTestNestedInstall"));
        QVERIFY2(!answer.isEmpty(), "the other profile's module was not reloaded, so no install was reached from the save");
        QCOMPARE(answer, qsl("true|nil"));
        QVERIFY2(took < 10000, qPrintable(qsl("the save took %1 ms to finish: the install inside it waited for it").arg(took)));
        QVERIFY(QTest::qWaitFor(
                [this, &nestedModule]() {
                    return mpSavingHost->mInstalledModules.contains(nestedModule);
                },
                5s));
        QVERIFY2(QTest::qWaitFor(
                         [&listedOnChange]() {
                             return !listedOnChange.isEmpty() && listedOnChange.constLast();
                         },
                         2s),
                 "the module list was never announced as changed once the put-off install had added the module");
        QVERIFY(settle(mpSavingHost));
        QVERIFY(mpSavingHost->uninstallPackage(nestedModule, enums::PackageModuleType::ModuleFromScript));
        QVERIFY(settle(mpSavingHost));
    }

    void test_aPutOffScriptInstallThatFailsSaysSoOnTheConsole()
    {
        // The script was answered true before the install was tried, so the console is all that can tell it
        const QString path = writeArchive(qsl("module-sync-reentry-notazip.mpackage"), "not a zip archive");
        QVERIFY(!path.isEmpty());

        const auto took = saveReachingBackWith(qsl("installPackage"), path);
        QVERIFY(took >= 0);

        QCOMPARE(luaGlobalString(mpSavingHost, qsl("mudletTestNestedInstall")), qsl("true|nil"));
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mainConsoleText().contains(qsl("Package install failed"));
                         },
                         5s),
                 qPrintable(mainConsoleText()));
        QVERIFY(mainConsoleText().contains(qsl("could not unzip package")));
        QVERIFY(settle(mpSavingHost));
    }

    void test_aScriptInstallSoonGivesUpOnASaveThatNeverReportsItselfFinished()
    {
        QVERIFY(settle(mpSavingHost));
        const QString packageName = qsl("module-sync-reentry-stuck-save");
        const QString path = writePackage(packageName);
        QVERIFY(!path.isEmpty());
        // Every write is done, but the save still reads as running
        mpSavingHost->mWritingHostAndModules = true;
        const auto unstick = qScopeGuard([this]() {
            mpSavingHost->mWritingHostAndModules = false;
        });

        QElapsedTimer elapsed;
        elapsed.start();
        QVERIFY(runLua(mpSavingHost, qsl("local ok, err = installPackage([[%1]]); mudletTestNestedInstall = tostring(ok) .. '|' .. tostring(err)").arg(path)).isNull());
        const auto took = elapsed.elapsed();

        QCOMPARE(luaGlobalString(mpSavingHost, qsl("mudletTestNestedInstall")), qsl("nil|a profile save is still in progress, try again later"));
        // Nothing else is delivered while a script waits, so the whole window stays unpainted until it gives up
        QVERIFY2(took < 5000, qPrintable(qsl("the install took %1 ms to give up on the save").arg(took)));
        QVERIFY(!mpSavingHost->mInstalledPackages.contains(packageName));
    }
};

#include "ModuleSyncSaveReentryTest.moc"
MUDLET_GROUPED_TEST_MAIN(ModuleSyncSaveReentryTest)
