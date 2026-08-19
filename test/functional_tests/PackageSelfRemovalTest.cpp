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
 * Regression test for use-after-free when a package uninstalls itself from its
 * own timer script or event-handler script (#9337 class, the remaining timer
 * and script cases).
 *
 * Package auto-updaters commonly call uninstallPackage()+installPackage() on
 * their own package from one of the package's own items. Since the
 * *Unit::uninstall() methods started deleting items immediately (instead of
 * just unregistering them), doing that from a timer deleted the very TTimer
 * whose execute() was still on the call stack; doing it from an event handler
 * deleted TScript objects that Host::raiseEvent() was still iterating over; and
 * doing it from a script's top-level body deleted the very TScript that
 * compileScript() was in the middle of compiling - heap corruption every way.
 * TriggerUnit/AliasUnit/KeyUnit gained a processing-depth deferral in #9383;
 * this covers TimerUnit and ScriptUnit (both the event-dispatch and the
 * compile-time body paths).
 *
 * Under AddressSanitizer the pre-fix code aborts with heap-use-after-free
 * inside TTimer::execute() / Host::raiseEvent() / TScript::compileScript(); with
 * the deferral in place all scenarios complete cleanly.
 *
 * The second half covers the other side of that deferral: an item whose delete
 * is outstanding is still registered, and must not be written back into the
 * profile by a save taken before the unit goes idle.
 *
 * Run with: ctest -R PackageSelfRemovalTest -V
 */

#include <QtTest/QtTest>

#include <QScopeGuard>
#include <QTemporaryDir>

#include "PortableModeTestHelper.h"
#include "ActionUnit.h"
#include "AliasUnit.h"
#include "Host.h"
#include "HostManager.h"
#include "LuaInterface.h"
#include "MudletInstanceCoordinator.h"
#include "ScriptUnit.h"
#include "TAction.h"
#include "TAlias.h"
#include "TEvent.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "VarUnit.h"
#include "XMLexport.h"
#include "XMLimport.h"
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

// TriggerUnit only holds its depth inside processDataStream(), so a save at
// depth has to come from a trigger's own script. Stands in for the Lua
// saveProfile() one would call - a bare test Host has no console for that.
static Host* gpMidPassExportHost = nullptr;
static QString gMidPassExportPath;
static QString gMidPassExportedXml;

static int exportProfileMidPass(lua_State* L)
{
    Q_UNUSED(L)
    gMidPassExportedXml.clear();
    if (!gpMidPassExportHost) {
        return 0;
    }
    auto writer = std::make_shared<XMLexport>(gpMidPassExportHost);
    // variables included: the only export here that builds the variable tree,
    // and it does so with a Lua call frame live
    if (!writer->exportPackage(gMidPassExportPath, true, false)) {
        qWarning() << "exportProfileMidPass() - the export itself failed";
        return 0;
    }
    QFile file(gMidPassExportPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "exportProfileMidPass() - could not read back" << gMidPassExportPath;
        return 0;
    }
    gMidPassExportedXml = QString::fromUtf8(file.readAll());
    file.close();
    QFile::remove(gMidPassExportPath);
    return 0;
}

class PackageSelfRemovalTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfileName = qsl("PackageSelfRemoval-Test");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // Keep the test hermetic: point the config dir resolution at a
        // temporary directory instead of the user's real profiles.
        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QVERIFY2(mudlet::self()->getHostManager().addHost(mProfileName, QString(), QString(), QString()), "failed to create the Host");
        mpHost = mudlet::self()->getHostManager().getHost(mProfileName);
        QVERIFY(mpHost);
        // A bare Host blocks script compilation until the full profile boot
        // would normally clear this; the test's scripts need to compile:
        mpHost->mBlockScriptCompile = false;
        // NB: mLoadedOk is left false on purpose - the deferred saveProfile()
        // that uninstallPackage() schedules then declines to run, which this
        // console-less test Host could not service anyway.
        createKeeperItems();
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // A package's event-handler script uninstalls its own package while
    // Host::raiseEvent() is mid-dispatch. The second handler in the same
    // package is what the pre-fix code would have called through a freed
    // TScript pointer.
    void test_scriptEventHandlerSelfUninstall()
    {
        const QString packageName = qsl("selfuninstall-script");
        mpHost->mInstalledPackages << packageName;

        auto pUninstaller = new TScript(nullptr, mpHost);
        mpHost->getScriptUnit()->registerScript(pUninstaller);
        pUninstaller->mPackageName = packageName;
        pUninstaller->setName(qsl("selfUninstallHandler"));
        QVERIFY2(pUninstaller->setScript(qsl("function selfUninstallHandler(event)\n    uninstallPackage(\"%1\")\nend\n").arg(packageName)), "uninstaller handler script failed to compile");
        pUninstaller->setEventHandlerList(QStringList{qsl("testSelfUninstallEvent")});
        pUninstaller->setIsActive(true);

        auto pBystander = new TScript(nullptr, mpHost);
        mpHost->getScriptUnit()->registerScript(pBystander);
        pBystander->mPackageName = packageName;
        pBystander->setName(qsl("selfUninstallBystander"));
        QVERIFY2(pBystander->setScript(qsl("function selfUninstallBystander(event)\nend\n")), "bystander handler script failed to compile");
        pBystander->setEventHandlerList(QStringList{qsl("testSelfUninstallEvent")});
        pBystander->setIsActive(true);

        TEvent event{};
        event.mArgumentList.append(qsl("testSelfUninstallEvent"));
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        // Pre-fix this dispatch freed both TScripts under raiseEvent()'s feet:
        mpHost->raiseEvent(event);

        QVERIFY2(!mpHost->mInstalledPackages.contains(packageName), "package was not uninstalled");
        // The deferred deletes must have been flushed once the dispatch ended:
        QVERIFY2(mpHost->getScriptUnit()->findItems(qsl("selfUninstallHandler")).empty(), "uninstalled script is still registered");
        QVERIFY2(mpHost->getScriptUnit()->findItems(qsl("selfUninstallBystander")).empty(), "uninstalled script is still registered");
    }

    // A package script's TOP-LEVEL body (not an event handler) uninstalls its own
    // package while it is being compiled - the path profile boot and reset take
    // when they run "all the Lua code outside of functions" via ScriptUnit::compileAll().
    // Pre-fix, ScriptUnit::uninstall() deleted the script immediately: compileScript()
    // then wrote to the freed TScript (heap-use-after-free WRITE at TScript::compileScript),
    // and compileAll()'s range-for walked onto the freed std::list node it had unlinked.
    void test_scriptBodySelfUninstallOnCompile()
    {
        const QString packageName = qsl("selfuninstall-script-compile");
        mpHost->mInstalledPackages << packageName;

        // Defer compilation so the bodies run through compileAll() below rather than
        // from setScript() here, mirroring how a freshly loaded package's scripts are
        // compiled at profile boot (mudlet::loadProfile) and reset (Host::resetProfile_phase2):
        mpHost->mBlockScriptCompile = true;

        auto pUninstaller = new TScript(nullptr, mpHost);
        mpHost->getScriptUnit()->registerScript(pUninstaller);
        pUninstaller->mPackageName = packageName;
        pUninstaller->setName(qsl("selfUninstallOnCompile"));
        // A bare uninstallPackage() at the top level runs the moment the script is compiled:
        pUninstaller->setScript(qsl("uninstallPackage(\"%1\")").arg(packageName));
        pUninstaller->setIsActive(true);

        // A second script in the same package, registered AFTER the uninstaller, is
        // the node the pre-fix immediate delete unlinks from under compileAll()'s loop:
        auto pBystander = new TScript(nullptr, mpHost);
        mpHost->getScriptUnit()->registerScript(pBystander);
        pBystander->mPackageName = packageName;
        pBystander->setName(qsl("selfUninstallCompileBystander"));
        pBystander->setScript(qsl("local noop = true\n"));
        pBystander->setIsActive(true);

        mpHost->mBlockScriptCompile = false;
        // Pre-fix this trips heap-use-after-free; post-fix the delete is deferred until
        // after the loop and then flushed by compileAll():
        mpHost->getScriptUnit()->compileAll();

        QVERIFY2(!mpHost->mInstalledPackages.contains(packageName), "package was not uninstalled");
        // The deferred deletes must have been flushed at the end of compileAll():
        QVERIFY2(mpHost->getScriptUnit()->findItems(qsl("selfUninstallOnCompile")).empty(), "uninstalled script is still registered");
        QVERIFY2(mpHost->getScriptUnit()->findItems(qsl("selfUninstallCompileBystander")).empty(), "uninstalled bystander script is still registered");
    }

    // A package script's top-level body uninstalls its own package from a plain
    // setScript() compile - the path permScript()/setScript() take when invoked from
    // an alias, key or the command line, none of which sit inside compileAll(), the
    // editor's saveScript(), or raiseEvent(). Pre-fix this hit the same
    // heap-use-after-free in compileScript() as the compileAll() case. The compile
    // guard defers the delete here too; because this entry point has no synchronous
    // flush of its own, the deferred script lingers (deactivated) until a catch-all
    // flush - ScriptUnit::doCleanup() via Host::incomingStreamProcessor()/
    // slot_purgeTemps(), or the doCleanup() Host::uninstallPackage()'s queued save runs
    // - collects it, which must happen without leaking or double-freeing.
    void test_scriptBodySelfUninstallFromSetScript()
    {
        const QString packageName = qsl("selfuninstall-script-setscript");
        mpHost->mInstalledPackages << packageName;

        auto pUninstaller = new TScript(nullptr, mpHost);
        mpHost->getScriptUnit()->registerScript(pUninstaller);
        pUninstaller->mPackageName = packageName;
        pUninstaller->setName(qsl("selfUninstallFromSetScript"));
        // mBlockScriptCompile is false, so setScript() compiles and runs the top-level
        // body immediately - uninstallPackage() fires from inside compileScript():
        pUninstaller->setScript(qsl("uninstallPackage(\"%1\")").arg(packageName));

        QVERIFY2(!mpHost->mInstalledPackages.contains(packageName), "package was not uninstalled");

        // No synchronous flush point covers this entry, so the script is still
        // registered (deferred, deactivated) right after setScript() returns:
        QVERIFY2(!mpHost->getScriptUnit()->findItems(qsl("selfUninstallFromSetScript")).empty(), "self-uninstalling script should still be deferred, not yet deleted");

        // The catch-all flush (as wired into incomingStreamProcessor()/slot_purgeTemps())
        // must then collect it cleanly - no leak, no double-free:
        mpHost->getScriptUnit()->doCleanup();
        QVERIFY2(mpHost->getScriptUnit()->findItems(qsl("selfUninstallFromSetScript")).empty(), "deferred uninstalled script was not flushed");
    }

    // A package's timer script uninstalls its own package while
    // TTimer::execute() is still on the call stack for that timer. Pre-fix,
    // execute() would resume on a freed `this`.
    void test_timerScriptSelfUninstall()
    {
        const QString packageName = qsl("selfuninstall-timer");
        mpHost->mInstalledPackages << packageName;

        auto pTimer = new TTimer(qsl("selfUninstallTimer"), QTime(0, 0, 0, 250), mpHost);
        mpHost->getTimerUnit()->registerTimer(pTimer);
        pTimer->mPackageName = packageName;
        // The trailing error() is what makes this a genuine regression test: after
        // uninstallPackage() has (pre-fix) freed this timer, the error aborts the
        // Lua call so it returns false, and TTimer::execute() then resumes past the
        // call and reads the freed `this` at `mpQTimer->stop()` - the heap-use-after-free
        // the deferral prevents. Without the error the call returns cleanly and
        // execute() never touches `this` again, so the bug would go undetected.
        QVERIFY2(pTimer->setScript(qsl("uninstallPackage(\"%1\")\nerror(\"boom\")").arg(packageName)), "timer script failed to compile");
        pTimer->setIsActive(true);
        pTimer->enableTimer();

        // Let the timer fire and take its package (and itself) down:
        QTRY_VERIFY_WITH_TIMEOUT(!mpHost->mInstalledPackages.contains(packageName), 5000);

        // Allow any queued activity (the declined deferred save, further timer
        // ticks) to surface problems:
        QTest::qWait(500);
        // mudlet::slot_timerFires() flushes the deferred delete as soon as the
        // uninstalling timer's execute() has finished, so by now the timer must
        // be properly gone - not lingering deactivated where the next profile
        // save would serialize it back in:
        QVERIFY2(!mpHost->getTimerUnit()->findFirstTimer(qsl("selfUninstallTimer")), "uninstalled package timer is still registered");
    }

    // The trigger route, driven the whole way: the package's own trigger fires,
    // uninstalls its package and saves, all inside the pass. Its export is the
    // only one here that includes the variables, so it doubles as the check that
    // the variable tree can be built from inside a live Lua call frame.
    void test_saveFromTriggerScriptDoesNotResurrectItsPackage()
    {
        const QString packageName = qsl("resurrect-trigger");
        mpHost->mInstalledPackages << packageName;

        gpMidPassExportHost = mpHost;
        gMidPassExportPath = qsl("%1/mid-pass-export.xml").arg(mConfigDir.path());
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_register(L, "qaExportProfileMidPass", exportProfileMidPass);
        QCOMPARE(luaL_dostring(L, "midPassSavedVar = 'saved from inside the pass'"), 0);
        mpHost->getLuaInterface()->getVarUnit()->savedVars.insert(qsl("midPassSavedVar"));

        auto pGroup = new TTrigger(nullptr, mpHost);
        pGroup->setIsFolder(true);
        pGroup->registerTrigger();
        pGroup->setName(qsl("resurrectTriggerGroup"));
        pGroup->mPackageName = packageName;
        pGroup->setIsActive(true);

        auto pKicker = new TTrigger(pGroup, mpHost);
        pKicker->setRegexCodeList({qsl("^resurrect me$")}, {REGEX_PERL});
        pKicker->registerTrigger();
        QVERIFY2(pKicker->setScript(qsl("uninstallPackage(\"%1\")\nqaExportProfileMidPass()").arg(packageName)), "trigger script failed to compile");
        pKicker->setName(qsl("resurrectTriggerKicker"));
        pKicker->setIsActive(true);

        // a sibling that never runs: the whole group must go, not just the one
        // that fired
        auto pBystander = new TTrigger(pGroup, mpHost);
        pBystander->setRegexCodeList({qsl("^never matched$")}, {REGEX_PERL});
        pBystander->registerTrigger();
        pBystander->setName(qsl("resurrectTriggerBystander"));
        pBystander->setIsActive(true);

        QVERIFY2(exportedProfileXml().contains(qsl("resurrectTriggerGroup")), "the trigger group should be in the profile before its package is uninstalled");

        mpHost->getTriggerUnit()->processDataStream(qsl("resurrect me"), -1);

        QVERIFY2(!mpHost->mInstalledPackages.contains(packageName), "package was not uninstalled");
        QVERIFY2(!gMidPassExportedXml.isEmpty(), "the mid-pass export produced nothing to check");
        QVERIFY2(!gMidPassExportedXml.contains(qsl("resurrectTriggerGroup")), "a save taken mid-pass wrote the uninstalled package's trigger group back into the profile");
        QVERIFY2(!gMidPassExportedXml.contains(qsl("resurrectTriggerKicker")), "a save taken mid-pass wrote the uninstalled package's trigger back into the profile");
        QVERIFY2(!gMidPassExportedXml.contains(qsl("resurrectTriggerBystander")), "a save taken mid-pass wrote the uninstalled package's trigger back into the profile");
        const QString keeperError = keepersMissingFrom(gMidPassExportedXml);
        QVERIFY2(keeperError.isEmpty(), qPrintable(keeperError));
        QVERIFY2(gMidPassExportedXml.contains(qsl("saved from inside the pass")), "the variables were not read out of Lua by a save taken from inside a script");

        QVERIFY2(mpHost->getTriggerUnit()->findItems(qsl("resurrectTriggerKicker")).empty(), "uninstalled trigger is still registered");

        mpHost->getLuaInterface()->getVarUnit()->savedVars.remove(qsl("midPassSavedVar"));
        gpMidPassExportHost = nullptr;
    }

    // The alias route: a package shipping its own "uninstall" alias.
    void test_saveFromAliasScriptDoesNotResurrectItsPackage()
    {
        const QString packageName = qsl("resurrect-alias");
        mpHost->mInstalledPackages << packageName;

        gpMidPassExportHost = mpHost;
        gMidPassExportPath = qsl("%1/mid-pass-alias-export.xml").arg(mConfigDir.path());
        lua_register(mpHost->mLuaInterpreter.getLuaGlobalState(), "qaExportProfileMidPass", exportProfileMidPass);

        auto pAlias = new TAlias(qsl("resurrectAlias"), mpHost);
        pAlias->setRegexCode(qsl("^resurrect me$"));
        mpHost->getAliasUnit()->registerAlias(pAlias);
        pAlias->mPackageName = packageName;
        QVERIFY2(pAlias->setScript(qsl("uninstallPackage(\"%1\")\nqaExportProfileMidPass()").arg(packageName)), "alias script failed to compile");
        pAlias->setIsActive(true);

        QVERIFY2(exportedProfileXml().contains(qsl("resurrectAlias")), "the alias should be in the profile before its package is uninstalled");

        mpHost->getAliasUnit()->processDataStream(qsl("resurrect me"));

        QVERIFY2(!mpHost->mInstalledPackages.contains(packageName), "package was not uninstalled");
        QVERIFY2(!gMidPassExportedXml.isEmpty(), "the mid-pass export produced nothing to check");
        QVERIFY2(!gMidPassExportedXml.contains(qsl("resurrectAlias")), "a save taken mid-pass wrote the uninstalled package's alias back into the profile");
        const QString keeperError = keepersMissingFrom(gMidPassExportedXml);
        QVERIFY2(keeperError.isEmpty(), qPrintable(keeperError));

        QVERIFY2(!mpHost->getAliasUnit()->findFirstAlias(qsl("resurrectAlias")), "uninstalled alias is still registered");
        gpMidPassExportHost = nullptr;
    }

    // The timer route. beginProcessing()/endProcessing() below are the calls
    // TTimer::execute() wraps its whole callback in.
    void test_saveDuringTimerCallbackDoesNotResurrectItsPackage()
    {
        const QString packageName = qsl("resurrect-timer");
        mpHost->mInstalledPackages << packageName;

        auto pTimer = new TTimer(qsl("resurrectTimer"), QTime(0, 0, 30), mpHost);
        mpHost->getTimerUnit()->registerTimer(pTimer);
        pTimer->mPackageName = packageName;
        QVERIFY2(pTimer->setScript(qsl("local noop = true\n")), "timer script failed to compile");

        QVERIFY2(exportedProfileXml().contains(qsl("resurrectTimer")), "the timer should be in the profile before its package is uninstalled");

        QString xml;
        {
            mpHost->getTimerUnit()->beginProcessing();
            // a failed QVERIFY returns from the slot; a level left on would
            // wedge every later test's doCleanup()
            const auto depthGuard = qScopeGuard([this]() {
                mpHost->getTimerUnit()->endProcessing();
                mpHost->getTimerUnit()->doCleanup();
            });
            QVERIFY(mpHost->uninstallPackage(packageName, enums::PackageModuleType::Package));
            xml = exportedProfileXml();
        }

        QVERIFY2(!xml.isEmpty(), "the export produced nothing to check");
        QVERIFY2(!xml.contains(qsl("resurrectTimer")), "a save taken during a timer callback wrote the uninstalled package's timer back into the profile");
        const QString keeperError = keepersMissingFrom(xml);
        QVERIFY2(keeperError.isEmpty(), qPrintable(keeperError));
        QVERIFY2(!mpHost->getTimerUnit()->findFirstTimer(qsl("resurrectTimer")), "uninstalled timer is still registered");
    }

    // The button route: TAction::execute() holds ActionUnit's depth the same way.
    void test_saveDuringButtonScriptDoesNotResurrectItsPackage()
    {
        const QString packageName = qsl("resurrect-action");
        mpHost->mInstalledPackages << packageName;

        auto pAction = new TAction(qsl("resurrectAction"), mpHost);
        mpHost->getActionUnit()->registerAction(pAction);
        pAction->mPackageName = packageName;
        QVERIFY2(pAction->setScript(qsl("local noop = true\n")), "button script failed to compile");

        QVERIFY2(exportedProfileXml().contains(qsl("resurrectAction")), "the button should be in the profile before its package is uninstalled");

        QString xml;
        {
            mpHost->getActionUnit()->beginProcessing();
            const auto depthGuard = qScopeGuard([this]() {
                mpHost->getActionUnit()->endProcessing();
                mpHost->getActionUnit()->doCleanup();
            });
            QVERIFY(mpHost->uninstallPackage(packageName, enums::PackageModuleType::Package));
            xml = exportedProfileXml();
        }

        QVERIFY2(!xml.isEmpty(), "the export produced nothing to check");
        QVERIFY2(!xml.contains(qsl("resurrectAction")), "a save taken during a button script wrote the uninstalled package's button back into the profile");
        const QString keeperError = keepersMissingFrom(xml);
        QVERIFY2(keeperError.isEmpty(), qPrintable(keeperError));
        QVERIFY2(!mpHost->getActionUnit()->findAction(qsl("resurrectAction")), "uninstalled button is still registered");
    }

    // The event-handler route: Host::raiseEvent() holds ScriptUnit's depth.
    void test_saveDuringEventDispatchDoesNotResurrectItsPackage()
    {
        const QString packageName = qsl("resurrect-script");
        mpHost->mInstalledPackages << packageName;

        auto pScript = new TScript(nullptr, mpHost);
        mpHost->getScriptUnit()->registerScript(pScript);
        pScript->mPackageName = packageName;
        pScript->setName(qsl("resurrectScript"));
        QVERIFY2(pScript->setScript(qsl("local noop = true\n")), "script failed to compile");

        QVERIFY2(exportedProfileXml().contains(qsl("resurrectScript")), "the script should be in the profile before its package is uninstalled");

        QString xml;
        {
            mpHost->getScriptUnit()->beginProcessing();
            const auto depthGuard = qScopeGuard([this]() {
                mpHost->getScriptUnit()->endProcessing();
                mpHost->getScriptUnit()->doCleanup();
            });
            QVERIFY(mpHost->uninstallPackage(packageName, enums::PackageModuleType::Package));
            xml = exportedProfileXml();
        }

        QVERIFY2(!xml.isEmpty(), "the export produced nothing to check");
        QVERIFY2(!xml.contains(qsl("resurrectScript")), "a save taken during an event dispatch wrote the uninstalled package's script back into the profile");
        const QString keeperError = keepersMissingFrom(xml);
        QVERIFY2(keeperError.isEmpty(), qPrintable(keeperError));
        QVERIFY2(mpHost->getScriptUnit()->findItems(qsl("resurrectScript")).empty(), "uninstalled script is still registered");
    }

    // Host::reloadModule() - reachable from Lua - uninstalls and reinstalls a
    // module back to back, so from a script the old items are still registered
    // when the new ones arrive.
    void test_moduleSaveDuringReloadDoesNotDuplicateItsItems()
    {
        const QString moduleName = qsl("resurrect-module");
        registerModuleAs(moduleName);
        QVERIFY2(importModuleTimerNamed(moduleName, qsl("moduleTimerBeforeReload")), "could not import the module's timer");

        QVERIFY2(exportedModuleXml(moduleName).contains(qsl("moduleTimerBeforeReload")), "the module's timer should be in its file before the reload");

        QString xml;
        {
            mpHost->getTimerUnit()->beginProcessing();
            const auto depthGuard = qScopeGuard([this]() {
                mpHost->getTimerUnit()->endProcessing();
                mpHost->getTimerUnit()->doCleanup();
            });
            // the uninstall half: at depth the old timer only gets deactivated
            QVERIFY(mpHost->uninstallPackage(moduleName, enums::PackageModuleType::ModuleSync));
            // ... and the reinstall half brings the module back with fresh items
            registerModuleAs(moduleName);
            QVERIFY2(importModuleTimerNamed(moduleName, qsl("moduleTimerAfterReload")), "could not re-import the module's timer");

            xml = exportedModuleXml(moduleName);
        }

        QVERIFY2(!xml.isEmpty(), "the module export produced nothing to check");
        QVERIFY2(xml.contains(qsl("moduleTimerAfterReload")), "the reloaded module's timer must be written to its file");
        QVERIFY2(!xml.contains(qsl("moduleTimerBeforeReload")), "a module save taken mid-reload wrote the pre-reload copy of the timer back into the module file");
    }

private:
    // Items of a package that is never uninstalled. Every other assertion here
    // is an absence, so without these an over-broad filter passes the whole file
    // while emptying the user's profile.
    void createKeeperItems()
    {
        const QString keeperPackage = qsl("keeper-package");
        mpHost->mInstalledPackages << keeperPackage;

        auto pTrigger = new TTrigger(nullptr, mpHost);
        pTrigger->setRegexCodeList({qsl("^never matched$")}, {REGEX_PERL});
        pTrigger->registerTrigger();
        pTrigger->setName(qsl("keeperTrigger"));
        pTrigger->mPackageName = keeperPackage;

        auto pAlias = new TAlias(qsl("keeperAlias"), mpHost);
        pAlias->setRegexCode(qsl("^never matched$"));
        mpHost->getAliasUnit()->registerAlias(pAlias);
        pAlias->mPackageName = keeperPackage;

        auto pTimer = new TTimer(qsl("keeperTimer"), QTime(0, 0, 30), mpHost);
        mpHost->getTimerUnit()->registerTimer(pTimer);
        pTimer->mPackageName = keeperPackage;

        auto pAction = new TAction(qsl("keeperAction"), mpHost);
        mpHost->getActionUnit()->registerAction(pAction);
        pAction->mPackageName = keeperPackage;

        auto pScript = new TScript(nullptr, mpHost);
        mpHost->getScriptUnit()->registerScript(pScript);
        pScript->setName(qsl("keeperScript"));
        pScript->mPackageName = keeperPackage;
    }

    QString keepersMissingFrom(const QString& xml) const
    {
        for (const auto& name : {qsl("keeperTrigger"), qsl("keeperAlias"), qsl("keeperTimer"), qsl("keeperAction"), qsl("keeperScript")}) {
            if (!xml.contains(name)) {
                return qsl("a save taken while a delete was outstanding dropped \"%1\", which belongs to a package that is still installed").arg(name);
            }
        }
        return {};
    }

    void registerModuleAs(const QString& moduleName)
    {
        mpHost->mInstalledModules[moduleName] = QStringList{qsl("%1/%2.xml").arg(mConfigDir.path(), moduleName), qsl("0")};
        mpHost->mModulesLoadedOk << moduleName;
    }

    // The module-member flag is private to XMLimport, so a genuine module item
    // can only be made by importing one: that creates the module's master folder
    // per unit, and renaming the timer's tells the two copies apart.
    bool importModuleTimerNamed(const QString& moduleName, const QString& itemName)
    {
        const QString path = qsl("%1/%2-import.xml").arg(mConfigDir.path(), itemName);
        auto* pSeed = new TTimer(itemName, QTime(0, 0, 30), mpHost);
        mpHost->getTimerUnit()->registerTimer(pSeed);
        pSeed->setScript(qsl("local noop = true\n"));
        const bool exported = XMLexport(pSeed).exportTimer(path);
        mpHost->getTimerUnit()->unregisterTimer(pSeed);
        delete pSeed;
        if (!exported) {
            return false;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }
        XMLimport importer(mpHost);
        const bool imported = importer.importPackage(&file, moduleName, 1).first;
        file.close();
        QFile::remove(path);
        if (!imported) {
            return false;
        }
        TTimer* pTimer = mpHost->getTimerUnit()->findFirstTimer(moduleName);
        if (!pTimer) {
            return false;
        }
        pTimer->setName(itemName);
        return true;
    }

    // Builds the document writeModuleXML() produces for a save and reads it back.
    QString exportedModuleXml(const QString& moduleName)
    {
        const QString path = qsl("%1/module-export.xml").arg(mConfigDir.path());
        XMLexport writer(mpHost);
        writer.writeModuleXML(moduleName);
        if (!XMLexport::saveXmlDocToFile(path, *writer.takeExportDocument())) {
            return {};
        }
        return readBack(path);
    }

    // The writers a profile save uses, without the console Host::saveProfile()
    // would need.
    QString exportedProfileXml()
    {
        const QString path = qsl("%1/profile-export.xml").arg(mConfigDir.path());
        auto writer = std::make_shared<XMLexport>(mpHost);
        if (!writer->exportPackage(path, true, true)) {
            return {};
        }
        return readBack(path);
    }

    static QString readBack(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {};
        }
        const QString xml = QString::fromUtf8(file.readAll());
        file.close();
        QFile::remove(path);
        return xml;
    }
};

#include "PackageSelfRemovalTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageSelfRemovalTest)
