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
 * AliasUnit and KeyUnit count how deeply their processDataStream() is nested so
 * that an item deleted mid-pass (the #9337 self-uninstall pattern) is only freed
 * once the outermost pass has finished - see the deferral added in #9383.
 *
 * The count is a member, so a pass that returns without taking its level back
 * off leaves the unit permanently "busy": every later doCleanup() declines to
 * run and the deferred deletes are never flushed. Nothing crashes and nothing
 * warns, which is why these paths are asserted directly. KeyUnit is the one that
 * matters most: it returns from inside its match loop as soon as a key fires,
 * which is exactly the shape of exit a hand-written decrement gets forgotten on.
 *
 * Run with: ctest -R UnitProcessingDepthTest -V
 */

#include <QtTest/QtTest>

#include <QScopeGuard>
#include <QTemporaryDir>

#include "PortableModeTestHelper.h"
#include "AliasUnit.h"
#include "Host.h"
#include "HostManager.h"
#include "KeyUnit.h"
#include "MudletInstanceCoordinator.h"
#include "TKey.h"
#include "TLuaInterpreter.h"
#include "mudlet.h"

#include "GroupedTest.h"

class UnitProcessingDepthTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfileName = qsl("UnitProcessingDepth-Test");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;

    // Reads back Lua state through the return value rather than getLuaString(),
    // which reports an absolute stack slot and so only answers correctly for the
    // first call in a process.
    bool luaHolds(const QString& condition) { return mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("assert(%1)").arg(condition)); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // Keep the test hermetic: resolve the config dir to a temporary
        // directory rather than the user's real profiles.
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
        // would normally clear this; the items below need to compile:
        mpHost->mBlockScriptCompile = false;
    }

    // Applies to every slot, including any added later: a level left on after a
    // pass is what silently wedges the unit, so no slot gets to end holding one.
    void cleanup()
    {
        QCOMPARE(mpHost->getKeyUnit()->processingDepth(), 0);
        QCOMPARE(mpHost->getAliasUnit()->processingDepth(), 0);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // A key that fires returns from the middle of the match loop; one that does
    // not runs the loop out. Both exits owe the unit its level back, and the
    // repeat is what makes a leak visible - one leaked level looks like nothing,
    // an accumulating count is what actually wedges cleanup.
    void keyProcessingDepthIsHandedBackOnEveryExit()
    {
        auto* keyUnit = mpHost->getKeyUnit();
        QCOMPARE(keyUnit->processingDepth(), 0);
        // The exit under test is only taken when this is false, and a match
        // reports true either way - so without pinning it, a changed default
        // would quietly move this slot onto the fall-through path instead.
        QCOMPARE(keyUnit->mRunAllKeyMatches, false);

        QString name = qsl("depthProbeKey");
        QString parent;
        QString script = qsl("keyFireCount = (keyFireCount or 0) + 1");
        int keycode = Qt::Key_F7;
        int modifier = Qt::NoModifier;
        auto [id, message] = mpHost->mLuaInterpreter.startPermKey(name, parent, keycode, modifier, script);
        QVERIFY2(id > 0, qPrintable(message));

        constexpr int passes = 5;
        for (int pass = 1; pass <= passes; ++pass) {
            QVERIFY2(keyUnit->processDataStream(Qt::Key_F7, Qt::NoModifier), "the probe key did not match, so the return-from-the-loop exit went untested");
            QCOMPARE(keyUnit->processingDepth(), 0);

            QVERIFY2(!keyUnit->processDataStream(Qt::Key_F8, Qt::NoModifier), "an unbound key reported a match");
            QCOMPARE(keyUnit->processingDepth(), 0);
        }

        // Proves the matching calls really did run the key's script, so the
        // depth assertions above are not passing on a loop that never matched.
        QVERIFY2(luaHolds(qsl("keyFireCount == %1").arg(passes)), "the probe key matched but its script did not run once per pass");
    }

    // With mRunAllKeyMatches set, a match no longer returns early and every key
    // gets a turn - the other way through the same function.
    void keyProcessingDepthIsHandedBackWhenRunningAllMatches()
    {
        auto* keyUnit = mpHost->getKeyUnit();
        QCOMPARE(keyUnit->processingDepth(), 0);

        QList<int> probeIds;
        const bool savedRunAllKeyMatches = keyUnit->mRunAllKeyMatches;
        keyUnit->mRunAllKeyMatches = true;
        // Hands the unit back exactly as it was found - the flag is global to
        // the profile and the F9 probes would otherwise fire in later slots.
        const auto restoreGuard = qScopeGuard([keyUnit, savedRunAllKeyMatches, &probeIds] {
            keyUnit->mRunAllKeyMatches = savedRunAllKeyMatches;
            for (const int probeId : probeIds) {
                if (auto* pKey = keyUnit->getKey(probeId)) {
                    pKey->setIsActive(false);
                }
            }
        });

        QString parent;
        QString script = qsl("allMatchCount = (allMatchCount or 0) + 1");
        int keycode = Qt::Key_F9;
        int modifier = Qt::NoModifier;
        for (const auto& keyName : {qsl("allMatchProbeA"), qsl("allMatchProbeB")}) {
            QString name = keyName;
            auto [id, message] = mpHost->mLuaInterpreter.startPermKey(name, parent, keycode, modifier, script);
            QVERIFY2(id > 0, qPrintable(message));
            probeIds.append(id);
        }

        QVERIFY(keyUnit->processDataStream(Qt::Key_F9, Qt::NoModifier));
        QCOMPARE(keyUnit->processingDepth(), 0);
        QVERIFY2(luaHolds(qsl("allMatchCount == 2")), "only one of the two keys bound to F9 ran, so the loop did not carry on past the first match");
    }

    // AliasUnit has the single exit, but the same permanence applies: the level
    // has to be back off before the unit is asked to process anything else.
    void aliasProcessingDepthIsHandedBackOnEveryExit()
    {
        auto* aliasUnit = mpHost->getAliasUnit();
        QCOMPARE(aliasUnit->processingDepth(), 0);

        auto [id, message] = mpHost->mLuaInterpreter.startPermAlias(qsl("depthProbeAlias"), QString(), qsl("^probe$"), qsl("aliasFireCount = (aliasFireCount or 0) + 1"));
        QVERIFY2(id > 0, qPrintable(message));

        constexpr int passes = 5;
        for (int pass = 1; pass <= passes; ++pass) {
            QVERIFY2(aliasUnit->processDataStream(qsl("probe")), "the probe alias did not match");
            QCOMPARE(aliasUnit->processingDepth(), 0);

            QVERIFY2(!aliasUnit->processDataStream(qsl("nothing matches this")), "an unmatched command reported a match");
            QCOMPARE(aliasUnit->processingDepth(), 0);
        }

        QVERIFY2(luaHolds(qsl("aliasFireCount == %1").arg(passes)), "the probe alias matched but its script did not run once per pass");
    }

    // What the count is actually for. Both items delete themselves from their
    // own script, which the unit has to defer while the pass is on the stack and
    // then flush - and the flush is the drain step the guard runs at depth 0.
    // Nothing else pumps cleanup here: Host::slot_purgeTemps() needs an event
    // loop this test never spins, so if the drain does not run the item survives.
    void anItemThatKillsItselfMidPassIsFreedByTheDrain()
    {
        auto* aliasUnit = mpHost->getAliasUnit();
        const int aliasId = mpHost->mLuaInterpreter.startTempAlias(qsl("^selfkill$"), qsl("killAlias(tostring(selfKillAliasId))"));
        QVERIFY(aliasId > 0);
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("selfKillAliasId = %1").arg(aliasId)));
        QVERIFY2(aliasUnit->getAlias(aliasId), "the temp alias was not registered");

        QVERIFY2(aliasUnit->processDataStream(qsl("selfkill")), "the self-killing alias did not match");
        QCOMPARE(aliasUnit->processingDepth(), 0);
        QVERIFY2(!aliasUnit->getAlias(aliasId), "the alias killed itself mid-pass but was never freed - the drain did not run");

        auto* keyUnit = mpHost->getKeyUnit();
        int keycode = Qt::Key_F10;
        int modifier = Qt::NoModifier;
        const int keyId = mpHost->mLuaInterpreter.startTempKey(modifier, keycode, qsl("killKey(tostring(selfKillKeyId))"));
        QVERIFY(keyId > 0);
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("selfKillKeyId = %1").arg(keyId)));
        QVERIFY2(keyUnit->getKey(keyId), "the temp key was not registered");

        QVERIFY2(keyUnit->processDataStream(Qt::Key_F10, Qt::NoModifier), "the self-killing key did not match");
        QCOMPARE(keyUnit->processingDepth(), 0);
        QVERIFY2(!keyUnit->getKey(keyId), "the key killed itself mid-pass but was never freed - the drain did not run");
    }
};

#include "UnitProcessingDepthTest.moc"
MUDLET_GROUPED_TEST_MAIN(UnitProcessingDepthTest)
