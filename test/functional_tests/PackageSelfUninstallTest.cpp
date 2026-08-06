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
 * The second half of the file covers the other side of that deferral: an item
 * whose delete is outstanding is still registered, and a profile save taken
 * before the unit goes idle again used to write it back out - resurrecting the
 * uninstalled package's items as orphans the Package Manager cannot remove.
 *
 * Run with: ctest -R PackageSelfUninstallTest -V
 */

#include <QtTest/QtTest>

#include <QTemporaryDir>

#include "ActionUnit.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "ScriptUnit.h"
#include "TAction.h"
#include "TEvent.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "XMLexport.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lua.h>
#else
#include <lua.h>
#endif
}

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForPackageSelfUninstallTest();

// TriggerUnit raises its processing depth inside processDataStream() and hands
// it back before returning, so the only way to take a save at depth from a
// trigger is from a trigger's own script - which is exactly the reported case
// (a script-driven uninstall followed by a save). This stands in for the Lua
// saveProfile() such a script would call; a bare test Host has no console for
// the real one to serialize.
static Host* gpMidPassExportHost = nullptr;
static QString gMidPassExportPath;
static QString gMidPassExportedXml;

static int exportProfileMidPass(lua_State*)
{
    gMidPassExportedXml.clear();
    if (!gpMidPassExportHost) {
        return 0;
    }
    auto writer = std::make_shared<XMLexport>(gpMidPassExportHost);
    if (!writer->exportPackage(gMidPassExportPath, true, true)) {
        return 0;
    }
    QFile file(gMidPassExportPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        gMidPassExportedXml = QString::fromUtf8(file.readAll());
    }
    return 0;
}

class PackageSelfUninstallTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfileName = qsl("PackageSelfUninstall-Test");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForPackageSelfUninstallTest();

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

    // The deferral above keeps the uninstalled items registered until the unit
    // is idle again, and the XML writers used to serialize whatever was
    // registered. A save taken in that window - which is what the mpkg /
    // auto-updater shape does, uninstalling from a script and saving straight
    // after - therefore wrote the just-uninstalled package's items back into
    // the profile. They return on the next load as items of a package that is
    // no longer installed, so the Package Manager does not list them and
    // uninstallPackage() will not remove them: the only way out is deleting
    // them by hand in the editor.
    //
    // This is the trigger route, driven the whole way: the package's own
    // trigger fires, uninstalls its package and saves, all inside the pass.
    void test_saveFromTriggerScriptDoesNotResurrectItsPackage()
    {
        const QString packageName = qsl("resurrect-trigger");
        mpHost->mInstalledPackages << packageName;

        gpMidPassExportHost = mpHost;
        gMidPassExportPath = qsl("%1/mid-pass-export.xml").arg(mConfigDir.path());
        lua_register(mpHost->mLuaInterpreter.getLuaGlobalState(), "qaExportProfileMidPass", exportProfileMidPass);

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

        // a sibling that never runs, to show the whole group goes, not just the
        // one trigger that happened to be executing
        auto pBystander = new TTrigger(pGroup, mpHost);
        pBystander->setRegexCodeList({qsl("^never matched$")}, {REGEX_PERL});
        pBystander->registerTrigger();
        pBystander->setName(qsl("resurrectTriggerBystander"));
        pBystander->setIsActive(true);

        mpHost->getTriggerUnit()->processDataStream(qsl("resurrect me"), -1);

        QVERIFY2(!mpHost->mInstalledPackages.contains(packageName), "package was not uninstalled");
        QVERIFY2(!gMidPassExportedXml.isEmpty(), "the mid-pass export produced nothing to check");
        QVERIFY2(!gMidPassExportedXml.contains(qsl("resurrectTriggerGroup")), "a save taken mid-pass wrote the uninstalled package's trigger group back into the profile");
        QVERIFY2(!gMidPassExportedXml.contains(qsl("resurrectTriggerKicker")), "a save taken mid-pass wrote the uninstalled package's trigger back into the profile");
        QVERIFY2(!gMidPassExportedXml.contains(qsl("resurrectTriggerBystander")), "a save taken mid-pass wrote the uninstalled package's trigger back into the profile");

        // the pass ended, so the deferred deletes must have been flushed
        QVERIFY2(mpHost->getTriggerUnit()->findItems(qsl("resurrectTriggerKicker")).empty(), "uninstalled trigger is still registered");
        gpMidPassExportHost = nullptr;
    }

    // The timer route is the one that makes the corruption permanent: TTimer's
    // processing depth is held for the whole of execute(), so even the save
    // uninstallPackage() defers to the next event loop pass runs inside it, and
    // when that is the last save of the session there is nothing left to undo
    // it. beginProcessing()/endProcessing() here are the very calls
    // TTimer::execute() wraps its callback in.
    void test_saveDuringTimerCallbackDoesNotResurrectItsPackage()
    {
        const QString packageName = qsl("resurrect-timer");
        mpHost->mInstalledPackages << packageName;

        auto pTimer = new TTimer(qsl("resurrectTimer"), QTime(0, 0, 30), mpHost);
        mpHost->getTimerUnit()->registerTimer(pTimer);
        pTimer->mPackageName = packageName;
        QVERIFY2(pTimer->setScript(qsl("local noop = true\n")), "timer script failed to compile");

        QVERIFY2(exportedProfileXml().contains(qsl("resurrectTimer")), "the timer should be in the profile before its package is uninstalled");

        mpHost->getTimerUnit()->beginProcessing();
        QVERIFY(mpHost->uninstallPackage(packageName, enums::PackageModuleType::Package));
        const QString xml = exportedProfileXml();
        mpHost->getTimerUnit()->endProcessing();
        mpHost->getTimerUnit()->doCleanup();

        QVERIFY2(!xml.isEmpty(), "the export produced nothing to check");
        QVERIFY2(!xml.contains(qsl("resurrectTimer")), "a save taken during a timer callback wrote the uninstalled package's timer back into the profile");
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

        mpHost->getActionUnit()->beginProcessing();
        QVERIFY(mpHost->uninstallPackage(packageName, enums::PackageModuleType::Package));
        const QString xml = exportedProfileXml();
        mpHost->getActionUnit()->endProcessing();
        mpHost->getActionUnit()->doCleanup();

        QVERIFY2(!xml.isEmpty(), "the export produced nothing to check");
        QVERIFY2(!xml.contains(qsl("resurrectAction")), "a save taken during a button script wrote the uninstalled package's button back into the profile");
        QVERIFY2(!mpHost->getActionUnit()->findAction(qsl("resurrectAction")), "uninstalled button is still registered");
    }

    // The event-handler route: Host::raiseEvent() holds ScriptUnit's depth for
    // the whole dispatch.
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

        mpHost->getScriptUnit()->beginProcessing();
        QVERIFY(mpHost->uninstallPackage(packageName, enums::PackageModuleType::Package));
        const QString xml = exportedProfileXml();
        mpHost->getScriptUnit()->endProcessing();
        mpHost->getScriptUnit()->doCleanup();

        QVERIFY2(!xml.isEmpty(), "the export produced nothing to check");
        QVERIFY2(!xml.contains(qsl("resurrectScript")), "a save taken during an event dispatch wrote the uninstalled package's script back into the profile");
        QVERIFY2(mpHost->getScriptUnit()->findItems(qsl("resurrectScript")).empty(), "uninstalled script is still registered");
    }

private:
    // Writes the profile's items out through the same writers a profile save
    // uses, without needing the console a full Host::saveProfile() would.
    QString exportedProfileXml()
    {
        const QString path = qsl("%1/profile-export.xml").arg(mConfigDir.path());
        auto writer = std::make_shared<XMLexport>(mpHost);
        if (!writer->exportPackage(path, true, true)) {
            return {};
        }
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

void initializeQRCResourcesForPackageSelfUninstallTest()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "PackageSelfUninstallTest.moc"
QTEST_MAIN(PackageSelfUninstallTest)
