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
 * enableX()/disableX() look an item up by name and must toggle EVERY item
 * sharing that name, not just the first one found (e.g. two groups both named
 * "Druid Aliases"). This covers all scriptable item types - aliases, triggers,
 * timers, keys and scripts - by creating two same-named items plus a
 * differently-named control, toggling them through the real Lua functions, and
 * checking each item's active state directly.
 *
 * Run with: ctest -R EnableDisableByNameTest -V
 */

#include <QtTest/QtTest>

#include <functional>

#include "AliasUnit.h"
#include "Host.h"
#include "KeyUnit.h"
#include "MudletInstanceCoordinator.h"
#include "ScriptUnit.h"
#include "TAlias.h"
#include "TKey.h"
#include "TLuaInterpreter.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#else
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#endif
}

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForEnableDisableByNameTest();

class EnableDisableByNameTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "EnableDisableByName-Test";
    const QString mPort = "4005";
    const QString mLocalhost = "localhost";

    void runLua(const QString& code)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) != 0) {
            const QString error = QString::fromUtf8(lua_tostring(L, -1));
            lua_pop(L, 1);
            QFAIL(qPrintable(qsl("Lua error running test script: %1").arg(error)));
        }
    }

    // Drives one item type through the shared expectation: enableX(dupName)
    // activates BOTH same-named items, disableX(dupName) deactivates BOTH while
    // leaving the differently-named control untouched, and enableX(dupName)
    // brings both back. `active` reads an item's live state by id.
    void checkToggleAffectsAllMatches(const QString& typeName,
                                      const QString& dupName,
                                      const QString& soloName,
                                      int id1,
                                      int id2,
                                      int idControl,
                                      const QString& enableFn,
                                      const QString& disableFn,
                                      std::function<bool(int)> active)
    {
        runLua(qsl("%1('%2')").arg(enableFn, dupName));
        runLua(qsl("%1('%2')").arg(enableFn, soloName));
        QVERIFY2(active(id1) && active(id2), qPrintable(qsl("both same-named %1s should start enabled").arg(typeName)));
        QVERIFY2(active(idControl), qPrintable(qsl("control %1 should start enabled").arg(typeName)));

        runLua(qsl("%1('%2')").arg(disableFn, dupName));
        QVERIFY2(!active(id1), qPrintable(qsl("first %1 should be disabled").arg(typeName)));
        QVERIFY2(!active(id2), qPrintable(qsl("second same-named %1 should ALSO be disabled").arg(typeName)));
        QVERIFY2(active(idControl), qPrintable(qsl("differently-named %1 must stay enabled").arg(typeName)));

        runLua(qsl("%1('%2')").arg(enableFn, dupName));
        QVERIFY2(active(id1), qPrintable(qsl("first %1 should be re-enabled").arg(typeName)));
        QVERIFY2(active(id2), qPrintable(qsl("second same-named %1 should ALSO be re-enabled").arg(typeName)));
    }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForEnableDisableByNameTest();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, mPort.toUShort());
        mudlet::start();
        mudlet::self()->setupConfig();
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
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    void test_aliasEnableDisableAffectsAllMatches()
    {
        auto [id1, m1] = mpHost->mLuaInterpreter.startPermAlias(qsl("Dup Aliases"), qsl(""), qsl("^dup_alias_1$"), qsl(""));
        auto [id2, m2] = mpHost->mLuaInterpreter.startPermAlias(qsl("Dup Aliases"), qsl(""), qsl("^dup_alias_2$"), qsl(""));
        auto [id3, m3] = mpHost->mLuaInterpreter.startPermAlias(qsl("Solo Alias"), qsl(""), qsl("^dup_alias_3$"), qsl(""));
        QVERIFY2(id1 > 0, qPrintable(m1));
        QVERIFY2(id2 > 0, qPrintable(m2));
        QVERIFY2(id3 > 0, qPrintable(m3));

        auto* unit = mpHost->getAliasUnit();
        checkToggleAffectsAllMatches(qsl("alias"), qsl("Dup Aliases"), qsl("Solo Alias"), id1, id2, id3, qsl("enableAlias"), qsl("disableAlias"), [unit](int id) {
            auto* p = unit->getAlias(id);
            return p && p->isActive();
        });
    }

    void test_triggerEnableDisableAffectsAllMatches()
    {
        QStringList p1{qsl("dup_trig_1")};
        QStringList p2{qsl("dup_trig_2")};
        QStringList p3{qsl("dup_trig_3")};
        auto [id1, m1] = mpHost->mLuaInterpreter.startPermSubstringTrigger(qsl("Dup Triggers"), qsl(""), p1, qsl(""));
        auto [id2, m2] = mpHost->mLuaInterpreter.startPermSubstringTrigger(qsl("Dup Triggers"), qsl(""), p2, qsl(""));
        auto [id3, m3] = mpHost->mLuaInterpreter.startPermSubstringTrigger(qsl("Solo Trigger"), qsl(""), p3, qsl(""));
        QVERIFY2(id1 > 0, qPrintable(m1));
        QVERIFY2(id2 > 0, qPrintable(m2));
        QVERIFY2(id3 > 0, qPrintable(m3));

        auto* unit = mpHost->getTriggerUnit();
        checkToggleAffectsAllMatches(qsl("trigger"), qsl("Dup Triggers"), qsl("Solo Trigger"), id1, id2, id3, qsl("enableTrigger"), qsl("disableTrigger"), [unit](int id) {
            auto* p = unit->getTrigger(id);
            return p && p->isActive();
        });
    }

    // setTriggerStayOpen() looks triggers up by name through the same path as the
    // enable/disable functions, so it must update EVERY same-named trigger too.
    void test_setTriggerStayOpenAffectsAllMatches()
    {
        QStringList p1{qsl("stayopen_trig_1")};
        QStringList p2{qsl("stayopen_trig_2")};
        QStringList p3{qsl("stayopen_trig_3")};
        auto [id1, m1] = mpHost->mLuaInterpreter.startPermSubstringTrigger(qsl("StayOpen Triggers"), qsl(""), p1, qsl(""));
        auto [id2, m2] = mpHost->mLuaInterpreter.startPermSubstringTrigger(qsl("StayOpen Triggers"), qsl(""), p2, qsl(""));
        auto [id3, m3] = mpHost->mLuaInterpreter.startPermSubstringTrigger(qsl("Solo StayOpen"), qsl(""), p3, qsl(""));
        QVERIFY2(id1 > 0, qPrintable(m1));
        QVERIFY2(id2 > 0, qPrintable(m2));
        QVERIFY2(id3 > 0, qPrintable(m3));

        auto* unit = mpHost->getTriggerUnit();
        auto* t1 = unit->getTrigger(id1);
        auto* t2 = unit->getTrigger(id2);
        auto* t3 = unit->getTrigger(id3);
        QVERIFY(t1 && t2 && t3);

        runLua(qsl("setTriggerStayOpen('StayOpen Triggers', 5)"));
        QVERIFY2(t1->mKeepFiring == 5, "first trigger should be set to stay open");
        QVERIFY2(t2->mKeepFiring == 5, "second same-named trigger should ALSO be set to stay open");
        QVERIFY2(t3->mKeepFiring == 0, "differently-named trigger must stay untouched");
    }

    void test_timerEnableDisableAffectsAllMatches()
    {
        auto [id1, m1] = mpHost->mLuaInterpreter.startPermTimer(qsl("Dup Timers"), qsl(""), 60.0, qsl(""));
        auto [id2, m2] = mpHost->mLuaInterpreter.startPermTimer(qsl("Dup Timers"), qsl(""), 60.0, qsl(""));
        auto [id3, m3] = mpHost->mLuaInterpreter.startPermTimer(qsl("Solo Timer"), qsl(""), 60.0, qsl(""));
        QVERIFY2(id1 > 0, qPrintable(m1));
        QVERIFY2(id2 > 0, qPrintable(m2));
        QVERIFY2(id3 > 0, qPrintable(m3));

        auto* unit = mpHost->getTimerUnit();
        checkToggleAffectsAllMatches(qsl("timer"), qsl("Dup Timers"), qsl("Solo Timer"), id1, id2, id3, qsl("enableTimer"), qsl("disableTimer"), [unit](int id) {
            auto* p = unit->getTimer(id);
            return p && p->isActive();
        });
    }

    void test_keyEnableDisableAffectsAllMatches()
    {
        QString n1 = qsl("Dup Keys");
        QString n2 = qsl("Dup Keys");
        QString n3 = qsl("Solo Key");
        QString parent;
        QString func;
        int mod = Qt::NoModifier;
        int kc1 = Qt::Key_F7;
        int kc2 = Qt::Key_F8;
        int kc3 = Qt::Key_F9;
        auto [id1, m1] = mpHost->mLuaInterpreter.startPermKey(n1, parent, kc1, mod, func);
        auto [id2, m2] = mpHost->mLuaInterpreter.startPermKey(n2, parent, kc2, mod, func);
        auto [id3, m3] = mpHost->mLuaInterpreter.startPermKey(n3, parent, kc3, mod, func);
        QVERIFY2(id1 > 0, qPrintable(m1));
        QVERIFY2(id2 > 0, qPrintable(m2));
        QVERIFY2(id3 > 0, qPrintable(m3));

        auto* unit = mpHost->getKeyUnit();
        checkToggleAffectsAllMatches(qsl("key"), qsl("Dup Keys"), qsl("Solo Key"), id1, id2, id3, qsl("enableKey"), qsl("disableKey"), [unit](int id) {
            auto* p = unit->getKey(id);
            return p && p->isActive();
        });
    }

    void test_scriptEnableDisableAffectsAllMatches()
    {
        auto makeScript = [this](const QString& name) {
            auto* pScript = new TScript(name, mpHost);
            pScript->setScript(qsl(""));
            mpHost->getScriptUnit()->registerScript(pScript);
            pScript->setIsActive(true);
            pScript->compile();
            return pScript->getID();
        };
        int id1 = makeScript(qsl("Dup Scripts"));
        int id2 = makeScript(qsl("Dup Scripts"));
        int id3 = makeScript(qsl("Solo Script"));
        QVERIFY(id1 > 0);
        QVERIFY(id2 > 0);
        QVERIFY(id3 > 0);

        auto* unit = mpHost->getScriptUnit();
        checkToggleAffectsAllMatches(qsl("script"), qsl("Dup Scripts"), qsl("Solo Script"), id1, id2, id3, qsl("enableScript"), qsl("disableScript"), [unit](int id) {
            auto* p = unit->getScript(id);
            return p && p->isActive();
        });
    }

    // Helpers (reused from ResetProfileTest pattern)

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(1000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
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

void initializeQRCResourcesForEnableDisableByNameTest()
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

#include "EnableDisableByNameTest.moc"
QTEST_MAIN(EnableDisableByNameTest)
