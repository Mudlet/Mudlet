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
 * TriggerUnit, AliasUnit, KeyUnit and TimerUnit each defer the deletion of an
 * item until no script is on the call stack. Four properties of that machinery
 * are checked here for all four units:
 *
 * - freeing a temporary item must unlink only that item from the by-name lookup
 *   table, not every item filed under the same name (#9649). The lookup table is
 *   a QMultiMap and names are not unique
 * - killing by name must keep scanning past same-named items it cannot kill,
 *   rather than report failure over the first one (#9649)
 * - the two deferred-delete containers, mCleanupSet and uninstallList, must never
 *   free the same object twice, whichever order it lands in them (#9650)
 * - enabling by name must not reactivate an item that is only waiting to be
 *   freed, while still reaching every live item filed under that name (#9877)
 *
 * The timer half of #9649 cannot be reached from the busted Lua suite - that runs
 * inside a tempTimer, so TimerUnit's cleanup stays deferred for the whole run -
 * which is why it lives here. The trigger, alias and key halves are covered from
 * Lua as well, in Trigger_spec.lua, Alias_spec.lua and KeyBinds_spec.lua.
 *
 * Note on the ...ContainersStayDisjoint cases: a regression there is a double
 * free, which has no post-condition to read back - the assertions below hold
 * either way and the run aborts instead. That is a real signal because the
 * functional tests always build with the address sanitizer on non-Windows
 * (test/functional_tests/CMakeLists.txt includes EnableSanitizers.cmake, whose
 * USE_SANITIZER defaults to "address"), but it does mean these four cases carry
 * no weight in a build with sanitizers switched off. Only AliasUnit and KeyUnit
 * ever lacked those disjointness guards; the trigger and timer cases are there
 * to keep the four units checked alike.
 *
 * Run with: ctest -R UnitDeferredDeleteTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "ProfileTestHelper.h"
#include "AliasUnit.h"
#include "Host.h"
#include "KeyUnit.h"
#include "MudletInstanceCoordinator.h"
#include "TAlias.h"
#include "TKey.h"
#include "TLuaInterpreter.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class UnitDeferredDeleteTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "UnitDeferredDelete-Test";
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = "localhost";
    const QString mPackageName = "unit deferred delete package";

    // QMultiMap::count() is a qsizetype; narrow it so QCOMPARE reports a plain
    // number against the int literals below
    static int lookupCount(qsizetype count) { return static_cast<int>(count); }

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);

        startProfile(mHostname, mLocalhost, mPort);
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // #9649: temporary items are named after their id, so a permanent item called
    // after that number shares the name. tempComplexRegexTrigger() makes the
    // trigger case even easier - it takes a user-supplied name - and that variant
    // is covered from Lua in Trigger_spec.lua.
    void test_triggerLookupKeepsSameNamedPermanent()
    {
        auto* unit = mpHost->getTriggerUnit();
        const int tempId = mpHost->mLuaInterpreter.startTempTrigger(qsl("lookup_evict_trigger_temp"), QString());
        QVERIFY(tempId > 0);
        const QString sharedName = QString::number(tempId);

        const QStringList permPatterns{qsl("lookup_evict_trigger_perm")};
        auto [permId, message] = mpHost->mLuaInterpreter.startPermSubstringTrigger(sharedName, QString(), permPatterns, QString());
        QVERIFY2(permId > 0, qPrintable(message));
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killTrigger(sharedName), "the temporary trigger should be the one killed");
        unit->doCleanup();

        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 1);
        QCOMPARE(unit->mLookupTable.value(sharedName), unit->getTrigger(permId));
        QVERIFY2(unit->enableTrigger(sharedName), "the permanent trigger must still be reachable by name");
    }

    void test_aliasLookupKeepsSameNamedPermanent()
    {
        auto* unit = mpHost->getAliasUnit();
        const int tempId = mpHost->mLuaInterpreter.startTempAlias(qsl("^lookup_evict_alias$"), QString());
        QVERIFY(tempId > 0);
        const QString sharedName = QString::number(tempId);

        auto [permId, message] = mpHost->mLuaInterpreter.startPermAlias(sharedName, QString(), qsl("^lookup_evict_alias_perm$"), QString());
        QVERIFY2(permId > 0, qPrintable(message));
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killAlias(sharedName), "the temporary alias should be the one killed");
        unit->doCleanup();

        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 1);
        QCOMPARE(unit->mLookupTable.value(sharedName), unit->getAlias(permId));
        QVERIFY2(unit->enableAlias(sharedName), "the permanent alias must still be reachable by name");
    }

    void test_timerLookupKeepsSameNamedPermanent()
    {
        auto* unit = mpHost->getTimerUnit();
        auto [tempId, tempMessage] = mpHost->mLuaInterpreter.startTempTimer(60.0, QString(), false);
        QVERIFY2(tempId > 0, qPrintable(tempMessage));
        const QString sharedName = QString::number(tempId);

        auto [permId, message] = mpHost->mLuaInterpreter.startPermTimer(sharedName, QString(), 60.0, QString());
        QVERIFY2(permId > 0, qPrintable(message));
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killTimer(sharedName), "the temporary timer should be the one killed");
        unit->doCleanup();

        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 1);
        QCOMPARE(unit->mLookupTable.value(sharedName), unit->getTimer(permId));
        QVERIFY2(unit->enableTimer(sharedName), "the permanent timer must still be reachable by name");
    }

    void test_keyLookupKeepsSameNamedPermanent()
    {
        auto* unit = mpHost->getKeyUnit();
        QString emptyScript;
        int tempModifier = Qt::NoModifier;
        int tempKeyCode = Qt::Key_F5;
        const int tempId = mpHost->mLuaInterpreter.startTempKey(tempModifier, tempKeyCode, emptyScript);
        QVERIFY(tempId > 0);
        QString sharedName = QString::number(tempId);

        QString parent;
        int permModifier = Qt::NoModifier;
        int permKeyCode = Qt::Key_F6;
        auto [permId, message] = mpHost->mLuaInterpreter.startPermKey(sharedName, parent, permKeyCode, permModifier, emptyScript);
        QVERIFY2(permId > 0, qPrintable(message));
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killKey(sharedName), "the temporary key should be the one killed");
        unit->doCleanup();

        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 1);
        QCOMPARE(unit->mLookupTable.value(sharedName), unit->getKey(permId));
        QVERIFY2(unit->enableKey(sharedName), "the permanent key must still be reachable by name");
    }

    // #9649, the kill-by-name half: killX(name) walks the root node list, which
    // holds items in creation order, so a permanent item restored from the profile
    // at startup precedes this session's temporaries. Giving up on the first
    // same-named item that cannot be killed strands the killable one and reports a
    // bare false. Each case below renames a freshly created permanent item to the
    // id the next temporary will take, which is exactly the collision a saved
    // profile produces; the QCOMPARE on the temporary's id makes the test fail
    // loudly rather than silently stop testing anything if ids stop being handed
    // out in sequence.
    void test_triggerKillByNameScansPastPermanent()
    {
        auto* unit = mpHost->getTriggerUnit();
        const QStringList permPatterns{qsl("kill_order_trigger_perm")};
        auto [permId, message] = mpHost->mLuaInterpreter.startPermSubstringTrigger(qsl("kill order placeholder"), QString(), permPatterns, QString());
        QVERIFY2(permId > 0, qPrintable(message));
        const QString sharedName = QString::number(permId + 1);
        unit->getTrigger(permId)->setName(sharedName);

        const int tempId = mpHost->mLuaInterpreter.startTempTrigger(qsl("kill_order_trigger_temp"), QString());
        QCOMPARE(tempId, permId + 1);
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killTrigger(sharedName), "killTrigger must scan past the permanent trigger to the temporary one");
        unit->doCleanup();
        QVERIFY2(!unit->getTrigger(tempId), "the temporary trigger should have been freed");
        QVERIFY(unit->getTrigger(permId));
    }

    void test_aliasKillByNameScansPastPermanent()
    {
        auto* unit = mpHost->getAliasUnit();
        auto [permId, message] = mpHost->mLuaInterpreter.startPermAlias(qsl("kill order placeholder"), QString(), qsl("^kill_order_alias_perm$"), QString());
        QVERIFY2(permId > 0, qPrintable(message));
        const QString sharedName = QString::number(permId + 1);
        unit->getAlias(permId)->setName(sharedName);

        const int tempId = mpHost->mLuaInterpreter.startTempAlias(qsl("^kill_order_alias_temp$"), QString());
        QCOMPARE(tempId, permId + 1);
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killAlias(sharedName), "killAlias must scan past the permanent alias to the temporary one");
        unit->doCleanup();
        QVERIFY2(!unit->getAlias(tempId), "the temporary alias should have been freed");
        QVERIFY(unit->getAlias(permId));
    }

    void test_timerKillByNameScansPastPermanent()
    {
        auto* unit = mpHost->getTimerUnit();
        auto [permId, message] = mpHost->mLuaInterpreter.startPermTimer(qsl("kill order placeholder"), QString(), 60.0, QString());
        QVERIFY2(permId > 0, qPrintable(message));
        const QString sharedName = QString::number(permId + 1);
        unit->getTimer(permId)->setName(sharedName);

        auto [tempId, tempMessage] = mpHost->mLuaInterpreter.startTempTimer(60.0, QString(), false);
        QVERIFY2(tempId > 0, qPrintable(tempMessage));
        QCOMPARE(tempId, permId + 1);
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killTimer(sharedName), "killTimer must scan past the permanent timer to the temporary one");
        unit->doCleanup();
        QVERIFY2(!unit->getTimer(tempId), "the temporary timer should have been freed");
        QVERIFY(unit->getTimer(permId));
    }

    void test_keyKillByNameScansPastPermanent()
    {
        auto* unit = mpHost->getKeyUnit();
        QString emptyScript;
        QString parent;
        QString placeholder = qsl("kill order placeholder");
        int permModifier = Qt::NoModifier;
        int permKeyCode = Qt::Key_F7;
        auto [permId, message] = mpHost->mLuaInterpreter.startPermKey(placeholder, parent, permKeyCode, permModifier, emptyScript);
        QVERIFY2(permId > 0, qPrintable(message));
        QString sharedName = QString::number(permId + 1);
        unit->getKey(permId)->setName(sharedName);

        int tempModifier = Qt::NoModifier;
        int tempKeyCode = Qt::Key_F8;
        const int tempId = mpHost->mLuaInterpreter.startTempKey(tempModifier, tempKeyCode, emptyScript);
        QCOMPARE(tempId, permId + 1);
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killKey(sharedName), "killKey must scan past the permanent key to the temporary one");
        unit->doCleanup();
        QVERIFY2(!unit->getKey(tempId), "the temporary key should have been freed");
        QVERIFY(unit->getKey(permId));
    }

    // #9649 again, on the non-root removal path: a temporary child goes through
    // removeTrigger() rather than removeTriggerRootNode(), and both got the same
    // exact-match fix.
    void test_temporaryChildTriggerLeavesSameNamedSiblingAlone()
    {
        auto* unit = mpHost->getTriggerUnit();
        const QStringList parentPatterns{qsl("child_evict_parent")};
        auto [parentId, message] = mpHost->mLuaInterpreter.startPermSubstringTrigger(qsl("Child Eviction Parent"), QString(), parentPatterns, QString());
        QVERIFY2(parentId > 0, qPrintable(message));
        auto* pParent = unit->getTrigger(parentId);
        QVERIFY(pParent);

        const QString sharedName = qsl("Child Eviction Shared");
        const QStringList childPatterns{qsl("child_evict_perm")};
        auto [permChildId, childMessage] = mpHost->mLuaInterpreter.startPermSubstringTrigger(sharedName, qsl("Child Eviction Parent"), childPatterns, QString());
        QVERIFY2(permChildId > 0, qPrintable(childMessage));

        auto* pTempChild = new TTrigger(pParent, mpHost);
        pTempChild->setRegexCodeList(QStringList{qsl("child_evict_temp")}, QList<int>{REGEX_SUBSTRING});
        pTempChild->setIsFolder(false);
        pTempChild->setIsActive(true);
        pTempChild->setTemporary(true);
        pTempChild->registerTrigger();
        pTempChild->setName(sharedName);
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killTrigger(sharedName), "the temporary child should be the one killed");
        unit->doCleanup();

        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 1);
        QCOMPARE(unit->mLookupTable.value(sharedName), unit->getTrigger(permChildId));
    }

    // #9650: an item can be queued in mCleanupSet and in uninstallList at the same
    // time - uninstall() at a non-zero processing depth leaves its items in
    // uninstallList and drops them from mCleanupSet, and a script killing one of
    // them afterwards puts it back. doCleanup() has to free such an item exactly
    // once. Both containers are populated directly here because reaching the
    // overlap from Lua needs a package-owned temporary item, which no current
    // import path produces.
    void test_triggerDeferredDeleteContainersStayDisjoint()
    {
        auto* unit = mpHost->getTriggerUnit();
        const int id = mpHost->mLuaInterpreter.startTempTrigger(qsl("double_free_trigger"), QString(), -1);
        QVERIFY(id > 0);
        auto* pTrigger = unit->getTrigger(id);
        QVERIFY(pTrigger);

        unit->uninstallList.append(pTrigger);
        unit->markCleanup(pTrigger);
        unit->doCleanup();

        QVERIFY(unit->mCleanupSet.isEmpty());
        QVERIFY(unit->uninstallList.isEmpty());
        QVERIFY2(!unit->getTrigger(id), "the trigger should have been freed exactly once");
    }

    // #9650: uninstall() at depth 0 deletes straight away, so it also has to drop
    // the item from mCleanupSet - otherwise the next doCleanup() frees a dangling
    // pointer.
    void test_triggerUninstallAtDepthZeroClearsCleanupSet()
    {
        auto* unit = mpHost->getTriggerUnit();
        const int id = mpHost->mLuaInterpreter.startTempTrigger(qsl("uninstall_trigger"), QString(), -1);
        QVERIFY(id > 0);
        auto* pTrigger = unit->getTrigger(id);
        QVERIFY(pTrigger);
        pTrigger->mPackageName = mPackageName;

        unit->markCleanup(pTrigger);
        unit->uninstall(mPackageName);

        // pTrigger is freed by now, so read the set's size rather than look the
        // dangling pointer up in it
        QVERIFY2(unit->mCleanupSet.isEmpty(), "uninstall() must take the trigger it freed out of the cleanup set");
        unit->doCleanup();
        QVERIFY2(!unit->getTrigger(id), "the trigger should have been freed exactly once");
    }

    void test_aliasDeferredDeleteContainersStayDisjoint()
    {
        auto* unit = mpHost->getAliasUnit();
        const int id = mpHost->mLuaInterpreter.startTempAlias(qsl("^double_free_alias$"), QString());
        QVERIFY(id > 0);
        auto* pAlias = unit->getAlias(id);
        QVERIFY(pAlias);

        unit->uninstallList.append(pAlias);
        unit->markCleanup(pAlias);
        unit->doCleanup();

        QVERIFY(unit->mCleanupSet.isEmpty());
        QVERIFY(unit->uninstallList.isEmpty());
        QVERIFY2(!unit->getAlias(id), "the alias should have been freed exactly once");
    }

    void test_aliasUninstallAtDepthZeroClearsCleanupSet()
    {
        auto* unit = mpHost->getAliasUnit();
        const int id = mpHost->mLuaInterpreter.startTempAlias(qsl("^uninstall_alias$"), QString());
        QVERIFY(id > 0);
        auto* pAlias = unit->getAlias(id);
        QVERIFY(pAlias);
        pAlias->mPackageName = mPackageName;

        unit->markCleanup(pAlias);
        unit->uninstall(mPackageName);

        QVERIFY2(unit->mCleanupSet.isEmpty(), "uninstall() must take the alias it freed out of the cleanup set");
        unit->doCleanup();
        QVERIFY2(!unit->getAlias(id), "the alias should have been freed exactly once");
    }

    void test_timerDeferredDeleteContainersStayDisjoint()
    {
        auto* unit = mpHost->getTimerUnit();
        auto [id, message] = mpHost->mLuaInterpreter.startTempTimer(60.0, QString(), false);
        QVERIFY2(id > 0, qPrintable(message));
        auto* pTimer = unit->getTimer(id);
        QVERIFY(pTimer);

        unit->uninstallList.append(pTimer);
        unit->markCleanup(pTimer);
        unit->doCleanup();

        QVERIFY(unit->mCleanupSet.isEmpty());
        QVERIFY(unit->uninstallList.isEmpty());
        QVERIFY2(!unit->getTimer(id), "the timer should have been freed exactly once");
    }

    void test_timerUninstallAtDepthZeroClearsCleanupSet()
    {
        auto* unit = mpHost->getTimerUnit();
        auto [id, message] = mpHost->mLuaInterpreter.startTempTimer(60.0, QString(), false);
        QVERIFY2(id > 0, qPrintable(message));
        auto* pTimer = unit->getTimer(id);
        QVERIFY(pTimer);
        pTimer->mPackageName = mPackageName;

        unit->markCleanup(pTimer);
        unit->uninstall(mPackageName);

        QVERIFY2(unit->mCleanupSet.isEmpty(), "uninstall() must take the timer it freed out of the cleanup set");
        unit->doCleanup();
        QVERIFY2(!unit->getTimer(id), "the timer should have been freed exactly once");
    }

    void test_keyDeferredDeleteContainersStayDisjoint()
    {
        auto* unit = mpHost->getKeyUnit();
        QString emptyScript;
        int modifier = Qt::NoModifier;
        int keyCode = Qt::Key_F10;
        const int id = mpHost->mLuaInterpreter.startTempKey(modifier, keyCode, emptyScript);
        QVERIFY(id > 0);
        auto* pKey = unit->getKey(id);
        QVERIFY(pKey);

        unit->uninstallList.append(pKey);
        unit->markCleanup(pKey);
        unit->doCleanup();

        QVERIFY(unit->mCleanupSet.isEmpty());
        QVERIFY(unit->uninstallList.isEmpty());
        QVERIFY2(!unit->getKey(id), "the key should have been freed exactly once");
    }

    void test_keyUninstallAtDepthZeroClearsCleanupSet()
    {
        auto* unit = mpHost->getKeyUnit();
        QString emptyScript;
        int modifier = Qt::NoModifier;
        int keyCode = Qt::Key_F11;
        const int id = mpHost->mLuaInterpreter.startTempKey(modifier, keyCode, emptyScript);
        QVERIFY(id > 0);
        auto* pKey = unit->getKey(id);
        QVERIFY(pKey);
        pKey->mPackageName = mPackageName;

        unit->markCleanup(pKey);
        unit->uninstall(mPackageName);

        QVERIFY2(unit->mCleanupSet.isEmpty(), "uninstall() must take the key it freed out of the cleanup set");
        unit->doCleanup();
        QVERIFY2(!unit->getKey(id), "the key should have been freed exactly once");
    }

    // killTrigger() skips an item that is only waiting to be freed; enableTrigger()
    // has to as well, or it resurrects the corpse.
    void test_triggerEnableByNameCannotReviveKilled()
    {
        auto* unit = mpHost->getTriggerUnit();
        const int id = mpHost->mLuaInterpreter.startTempTrigger(qsl("resurrect_trigger"), QString(), -1);
        QVERIFY(id > 0);
        auto* pTrigger = unit->getTrigger(id);
        QVERIFY(pTrigger);
        const QString name = QString::number(id);
        QVERIFY(pTrigger->isActive());

        QVERIFY2(unit->killTrigger(name), "the temporary trigger should be killable by name");
        QVERIFY2(!pTrigger->isActive(), "killTrigger() must deactivate as well as queue the delete");
        QVERIFY2(unit->mCleanupSet.contains(pTrigger), "the killed trigger should be waiting to be freed");

        QVERIFY2(!unit->enableTrigger(name), "enableTrigger() must not report success for a trigger that is only waiting to be freed");
        QVERIFY2(!pTrigger->isActive(), "a killed trigger must stay dead until it is freed");

        unit->doCleanup();
        QVERIFY2(!unit->getTrigger(id), "the killed trigger should still have been freed");
    }

    // As a user meets it: a one-shot has spent its fire and a script later on the
    // same line enables it by name.
    void test_triggerEnableByNameCannotReviveExpiredOneShot()
    {
        mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("oneShotFires = 0\n"
                                                            "reviverRan = false\n"
                                                            "tempComplexRegexTrigger('watchOnce', '^ONESHOT$', [[oneShotFires = oneShotFires + 1]], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1)\n"
                                                            "tempRegexTrigger('^ONESHOT$', [[\n"
                                                            "  if not reviverRan then\n"
                                                            "    reviverRan = true\n"
                                                            "    enableTrigger('watchOnce')\n"
                                                            "    feedTriggers('ONESHOT\\n')\n"
                                                            "  end\n"
                                                            "]], 1)\n"
                                                            "feedTriggers('ONESHOT\\n')\n"));

        QCOMPARE(mpHost->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(readGlobalBool(qsl("reviverRan")), "the script that calls enableTrigger() has to have run for this to test anything");
        QCOMPARE(readGlobalInt(qsl("oneShotFires")), 1);
    }

    // uninstall() at a non-zero processing depth leaves its package's triggers in
    // uninstallList rather than mCleanupSet, still in the lookup table. Populated
    // directly: reaching that state from Lua needs a package-owned temporary item,
    // which no current import path produces.
    void test_triggerEnableByNameCannotReviveAnUninstalledTrigger()
    {
        auto* unit = mpHost->getTriggerUnit();
        const int id = mpHost->mLuaInterpreter.startTempTrigger(qsl("uninstall_revive_trigger"), QString(), -1);
        QVERIFY(id > 0);
        auto* pTrigger = unit->getTrigger(id);
        QVERIFY(pTrigger);
        const QString name = QString::number(id);

        pTrigger->setIsActive(false);
        unit->uninstallList.append(pTrigger);
        QVERIFY2(!unit->mCleanupSet.contains(pTrigger), "uninstall() keeps the two deferred-delete containers disjoint");

        QVERIFY2(!unit->enableTrigger(name), "enableTrigger() must not report success for a trigger an uninstall is waiting to free");
        QVERIFY2(!pTrigger->isActive(), "a trigger whose package has been uninstalled must stay inactive");

        unit->doCleanup();
        QVERIFY2(!unit->getTrigger(id), "the uninstalled trigger should still have been freed");
    }

    // The skip must not stop the walk: a corpse and a live trigger can share a
    // name, and enable-by-name still has to reach every live one.
    void test_triggerEnableByNameStillReachesALiveSameNamedTrigger()
    {
        auto* unit = mpHost->getTriggerUnit();
        const QStringList permPatterns{qsl("mixed_corpse_perm")};
        auto [permId, message] = mpHost->mLuaInterpreter.startPermSubstringTrigger(qsl("mixed corpse placeholder"), QString(), permPatterns, QString());
        QVERIFY2(permId > 0, qPrintable(message));
        const QString sharedName = QString::number(permId + 1);
        unit->getTrigger(permId)->setName(sharedName);

        const int tempId = mpHost->mLuaInterpreter.startTempTrigger(qsl("mixed_corpse_temp"), QString());
        QCOMPARE(tempId, permId + 1);
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killTrigger(sharedName), "the temporary trigger should be the one killed");
        unit->getTrigger(permId)->setIsActive(false);

        QVERIFY2(unit->enableTrigger(sharedName), "enableTrigger must walk past the corpse to the live trigger filed under the same name");
        QVERIFY2(unit->getTrigger(permId)->isActive(), "the live same-named trigger should have been enabled");
        QVERIFY2(!unit->getTrigger(tempId)->isActive(), "the killed trigger must stay dead");

        unit->doCleanup();
        QVERIFY(!unit->getTrigger(tempId));
    }

    // #9877, the alias half. doCleanup() cannot run while the script that did
    // the killing is on the call stack, so a script at depth 0 keeps the corpse
    // reachable by name for the rest of its own run.
    void test_aliasEnableByNameCannotReviveKilled()
    {
        auto* unit = mpHost->getAliasUnit();
        const int id = mpHost->mLuaInterpreter.startTempAlias(qsl("^resurrect_alias$"), QString());
        QVERIFY(id > 0);
        auto* pAlias = unit->getAlias(id);
        QVERIFY(pAlias);
        const QString name = QString::number(id);
        QVERIFY(pAlias->isActive());

        QVERIFY2(unit->killAlias(name), "the temporary alias should be killable by name");
        QVERIFY2(!pAlias->isActive(), "killAlias() must deactivate as well as queue the delete");
        QVERIFY2(unit->mCleanupSet.contains(pAlias), "the killed alias should be waiting to be freed");

        QVERIFY2(!unit->enableAlias(name), "enableAlias() must not report success for an alias that is only waiting to be freed");
        QVERIFY2(!pAlias->isActive(), "a killed alias must stay dead until it is freed");

        unit->doCleanup();
        QVERIFY2(!unit->getAlias(id), "the killed alias should still have been freed");
    }

    // As a user meets it: a script kills the alias, something else enables it by
    // name, and the next command goes out before doCleanup() has run.
    void test_aliasEnableByNameCannotReviveAKilledAliasOnTheNextCommand()
    {
        mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("aliasFires = 0\n"
                                                            "local id = tempAlias('^RESURRECTME$', [[aliasFires = aliasFires + 1]])\n"
                                                            "local name = tostring(id)\n"
                                                            "expandAlias('RESURRECTME', false)\n"
                                                            "firstFires = aliasFires\n"
                                                            "killAlias(name)\n"
                                                            "enableAliasReturned = enableAlias(name)\n"
                                                            "expandAlias('RESURRECTME', false)\n"));

        QCOMPARE(readGlobalInt(qsl("firstFires")), 1);
        QCOMPARE(readGlobalInt(qsl("aliasFires")), 1);
        QVERIFY2(!readGlobalBool(qsl("enableAliasReturned")), "enableAlias() must not report success for an alias that is only waiting to be freed");
    }

    void test_aliasEnableByNameCannotReviveAnUninstalledAlias()
    {
        auto* unit = mpHost->getAliasUnit();
        const int id = mpHost->mLuaInterpreter.startTempAlias(qsl("^uninstall_revive_alias$"), QString());
        QVERIFY(id > 0);
        auto* pAlias = unit->getAlias(id);
        QVERIFY(pAlias);
        const QString name = QString::number(id);

        pAlias->setIsActive(false);
        unit->uninstallList.append(pAlias);
        QVERIFY2(!unit->mCleanupSet.contains(pAlias), "uninstall() keeps the two deferred-delete containers disjoint");

        QVERIFY2(!unit->enableAlias(name), "enableAlias() must not report success for an alias an uninstall is waiting to free");
        QVERIFY2(!pAlias->isActive(), "an alias whose package has been uninstalled must stay inactive");

        unit->doCleanup();
        QVERIFY2(!unit->getAlias(id), "the uninstalled alias should still have been freed");
    }

    void test_aliasEnableByNameStillReachesALiveSameNamedAlias()
    {
        auto* unit = mpHost->getAliasUnit();
        auto [permId, message] = mpHost->mLuaInterpreter.startPermAlias(qsl("mixed corpse alias placeholder"), QString(), qsl("^mixed_corpse_alias_perm$"), QString());
        QVERIFY2(permId > 0, qPrintable(message));
        const QString sharedName = QString::number(permId + 1);
        unit->getAlias(permId)->setName(sharedName);

        const int tempId = mpHost->mLuaInterpreter.startTempAlias(qsl("^mixed_corpse_alias_temp$"), QString());
        QCOMPARE(tempId, permId + 1);
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killAlias(sharedName), "the temporary alias should be the one killed");
        unit->getAlias(permId)->setIsActive(false);

        QVERIFY2(unit->enableAlias(sharedName), "enableAlias must walk past the corpse to the live alias filed under the same name");
        QVERIFY2(unit->getAlias(permId)->isActive(), "the live same-named alias should have been enabled");
        QVERIFY2(!unit->getAlias(tempId)->isActive(), "the killed alias must stay dead");

        unit->doCleanup();
        QVERIFY(!unit->getAlias(tempId));
    }

    // #9877, the key half. There is no cleanup() between slots, so a permanent
    // key one case creates stays live and bound for every later one: F6 and F7
    // are held by the cases above and F4 by the last one here, so a case that
    // presses a key has to pick a code none of them uses.
    void test_keyEnableByNameCannotReviveKilled()
    {
        auto* unit = mpHost->getKeyUnit();
        QString emptyScript;
        int modifier = Qt::NoModifier;
        int keyCode = Qt::Key_F1;
        const int id = mpHost->mLuaInterpreter.startTempKey(modifier, keyCode, emptyScript);
        QVERIFY(id > 0);
        auto* pKey = unit->getKey(id);
        QVERIFY(pKey);
        QString name = QString::number(id);
        QVERIFY(pKey->isActive());

        QVERIFY2(unit->killKey(name), "the temporary key should be killable by name");
        QVERIFY2(!pKey->isActive(), "killKey() must deactivate as well as queue the delete");
        QVERIFY2(unit->mCleanupSet.contains(pKey), "the killed key should be waiting to be freed");

        QVERIFY2(!unit->enableKey(name), "enableKey() must not report success for a key that is only waiting to be freed");
        QVERIFY2(!pKey->isActive(), "a killed key must stay dead until it is freed");

        unit->doCleanup();
        QVERIFY2(!unit->getKey(id), "the killed key should still have been freed");
    }

    // As a user meets it: a script kills the key, something else enables it by
    // name, and the key is pressed before doCleanup() has run.
    void test_keyEnableByNameCannotReviveAKilledKeyOnTheNextPress()
    {
        auto* unit = mpHost->getKeyUnit();
        mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("keyFires = 0"));
        QString script = qsl("keyFires = keyFires + 1");
        int modifier = Qt::NoModifier;
        int keyCode = Qt::Key_F2;
        const int id = mpHost->mLuaInterpreter.startTempKey(modifier, keyCode, script);
        QVERIFY(id > 0);
        QString name = QString::number(id);

        QVERIFY(unit->processDataStream(Qt::Key_F2, Qt::NoModifier));
        QCOMPARE(readGlobalInt(qsl("keyFires")), 1);

        QVERIFY2(unit->killKey(name), "the temporary key should be killable by name");
        const bool enabled = unit->enableKey(name);

        QVERIFY2(!unit->processDataStream(Qt::Key_F2, Qt::NoModifier), "a killed key must not match a later key press");
        QCOMPARE(readGlobalInt(qsl("keyFires")), 1);
        QVERIFY2(!enabled, "enableKey() must not report success for a key that is only waiting to be freed");
        QVERIFY2(!unit->getKey(id), "the pass at depth 0 should have freed the killed key");
    }

    void test_keyEnableByNameCannotReviveAnUninstalledKey()
    {
        auto* unit = mpHost->getKeyUnit();
        QString emptyScript;
        int modifier = Qt::NoModifier;
        int keyCode = Qt::Key_F3;
        const int id = mpHost->mLuaInterpreter.startTempKey(modifier, keyCode, emptyScript);
        QVERIFY(id > 0);
        auto* pKey = unit->getKey(id);
        QVERIFY(pKey);
        QString name = QString::number(id);

        pKey->setIsActive(false);
        unit->uninstallList.append(pKey);
        QVERIFY2(!unit->mCleanupSet.contains(pKey), "uninstall() keeps the two deferred-delete containers disjoint");

        QVERIFY2(!unit->enableKey(name), "enableKey() must not report success for a key an uninstall is waiting to free");
        QVERIFY2(!pKey->isActive(), "a key whose package has been uninstalled must stay inactive");

        unit->doCleanup();
        QVERIFY2(!unit->getKey(id), "the uninstalled key should still have been freed");
    }

    void test_keyEnableByNameStillReachesALiveSameNamedKey()
    {
        auto* unit = mpHost->getKeyUnit();
        QString emptyScript;
        QString parent;
        QString placeholder = qsl("mixed corpse key placeholder");
        int permModifier = Qt::NoModifier;
        int permKeyCode = Qt::Key_F4;
        auto [permId, message] = mpHost->mLuaInterpreter.startPermKey(placeholder, parent, permKeyCode, permModifier, emptyScript);
        QVERIFY2(permId > 0, qPrintable(message));
        QString sharedName = QString::number(permId + 1);
        unit->getKey(permId)->setName(sharedName);

        int tempModifier = Qt::NoModifier;
        int tempKeyCode = Qt::Key_F9;
        const int tempId = mpHost->mLuaInterpreter.startTempKey(tempModifier, tempKeyCode, emptyScript);
        QCOMPARE(tempId, permId + 1);
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killKey(sharedName), "the temporary key should be the one killed");
        unit->getKey(permId)->setIsActive(false);

        QVERIFY2(unit->enableKey(sharedName), "enableKey must walk past the corpse to the live key filed under the same name");
        QVERIFY2(unit->getKey(permId)->isActive(), "the live same-named key should have been enabled");
        QVERIFY2(!unit->getKey(tempId)->isActive(), "the killed key must stay dead");

        unit->doCleanup();
        QVERIFY(!unit->getKey(tempId));
    }

    // #9877, the timer half. killTimer() stops the QTimer, so reviving one arms
    // it again - remainingTime() reads that back straight from the QTimer.
    void test_timerEnableByNameCannotReviveKilled()
    {
        auto* unit = mpHost->getTimerUnit();
        auto [id, message] = mpHost->mLuaInterpreter.startTempTimer(60.0, QString(), false);
        QVERIFY2(id > 0, qPrintable(message));
        auto* pTimer = unit->getTimer(id);
        QVERIFY(pTimer);
        const QString name = QString::number(id);
        QVERIFY(unit->remainingTime(id) > 0);

        QVERIFY2(unit->killTimer(name), "the temporary timer should be killable by name");
        QVERIFY2(unit->remainingTime(id) == -1, "killTimer() must stop the timer as well as queue the delete");
        QVERIFY2(unit->mCleanupSet.contains(pTimer), "the killed timer should be waiting to be freed");

        QVERIFY2(!unit->enableTimer(name), "enableTimer() must not report success for a timer that is only waiting to be freed");
        QVERIFY2(unit->remainingTime(id) == -1, "a killed timer must stay stopped until it is freed");

        unit->doCleanup();
        QVERIFY2(!unit->getTimer(id), "the killed timer should still have been freed");
    }

    // A one-shot that has fired is the one corpse that is still isActive():
    // TTimer::execute() queues a spent non-repeating temporary with
    // mpQTimer->stop() + markCleanup() and no deactivate(), unlike killTimer().
    // The guard therefore cannot be reduced to a state test - skipping merely
    // inactive timers would let this one fire again.
    //
    // The two calls below are exactly what TTimer::execute() does, rather than a
    // real wait: mudlet::slot_timerFires() runs doCleanup() as soon as execute()
    // returns, so a fired one-shot is freed before a test could look at it.
    void test_timerEnableByNameCannotReviveASpentOneShot()
    {
        auto* unit = mpHost->getTimerUnit();
        auto [id, message] = mpHost->mLuaInterpreter.startTempTimer(60.0, QString(), false);
        QVERIFY2(id > 0, qPrintable(message));
        auto* pTimer = unit->getTimer(id);
        QVERIFY(pTimer);
        const QString name = QString::number(id);

        pTimer->stop();
        unit->markCleanup(pTimer);
        QVERIFY2(pTimer->isActive(), "a spent one-shot is queued for cleanup without being deactivated");
        QCOMPARE(unit->remainingTime(id), -1);

        QVERIFY2(!unit->enableTimer(name), "enableTimer() must not report success for a spent one-shot waiting to be freed");
        QVERIFY2(unit->remainingTime(id) == -1, "a spent one-shot must not be re-armed");

        unit->doCleanup();
        QVERIFY2(!unit->getTimer(id), "the spent one-shot should still have been freed");
    }

    void test_timerEnableByNameCannotReviveAnUninstalledTimer()
    {
        auto* unit = mpHost->getTimerUnit();
        auto [id, message] = mpHost->mLuaInterpreter.startTempTimer(60.0, QString(), false);
        QVERIFY2(id > 0, qPrintable(message));
        auto* pTimer = unit->getTimer(id);
        QVERIFY(pTimer);
        const QString name = QString::number(id);

        pTimer->setIsActive(false);
        unit->uninstallList.append(pTimer);
        QVERIFY2(!unit->mCleanupSet.contains(pTimer), "uninstall() keeps the two deferred-delete containers disjoint");

        QVERIFY2(!unit->enableTimer(name), "enableTimer() must not report success for a timer an uninstall is waiting to free");
        QVERIFY2(unit->remainingTime(id) == -1, "a timer whose package has been uninstalled must stay stopped");

        unit->doCleanup();
        QVERIFY2(!unit->getTimer(id), "the uninstalled timer should still have been freed");
    }

    void test_timerEnableByNameStillReachesALiveSameNamedTimer()
    {
        auto* unit = mpHost->getTimerUnit();
        auto [permId, message] = mpHost->mLuaInterpreter.startPermTimer(qsl("mixed corpse timer placeholder"), QString(), 60.0, QString());
        QVERIFY2(permId > 0, qPrintable(message));
        const QString sharedName = QString::number(permId + 1);
        unit->getTimer(permId)->setName(sharedName);

        auto [tempId, tempMessage] = mpHost->mLuaInterpreter.startTempTimer(60.0, QString(), false);
        QVERIFY2(tempId > 0, qPrintable(tempMessage));
        QCOMPARE(tempId, permId + 1);
        QCOMPARE(lookupCount(unit->mLookupTable.count(sharedName)), 2);

        QVERIFY2(unit->killTimer(sharedName), "the temporary timer should be the one killed");
        unit->getTimer(permId)->setIsActive(false);

        QVERIFY2(unit->enableTimer(sharedName), "enableTimer must walk past the corpse to the live timer filed under the same name");
        QVERIFY2(unit->getTimer(permId)->isActive(), "the live same-named timer should have been enabled");
        QVERIFY2(unit->remainingTime(permId) > 0, "the live same-named timer should have been re-armed, not just flagged active");
        QVERIFY2(!unit->getTimer(tempId)->isActive(), "the killed timer must stay dead");

        unit->doCleanup();
        QVERIFY(!unit->getTimer(tempId));
    }

    // Helpers (reused from the ResetProfileTest pattern)

    int readGlobalInt(const QString& name)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, name.toUtf8().constData());
        const int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    bool readGlobalBool(const QString& name)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, name.toUtf8().constData());
        const bool value = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return value;
    }

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(500)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

#include "UnitDeferredDeleteTest.moc"
MUDLET_GROUPED_TEST_MAIN(UnitDeferredDeleteTest)
