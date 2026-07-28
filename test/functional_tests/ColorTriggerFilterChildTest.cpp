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
 * A color-pattern trigger that is the child of a filter parent ("only pass
 * matches") must fire when the filtered capture carries the wanted colors.
 * Filter parents re-run their children on just the capture text, so the
 * child's color scan has to be aimed at the right buffer line and bounded to
 * the capture's window within it. Covers: a top-level color trigger control,
 * the filter-child case, and that the child only scans the parent's capture
 * rather than the whole line.
 *
 * Run with: ctest -R ColorTriggerFilterChildTest -V
 */

#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TTrigger.h"
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
void initializeQRCResourcesForColorTriggerFilterChildTest();

class ColorTriggerFilterChildTest : public QObject
{
    Q_OBJECT

private:
    // ANSI 3 is yellow, i.e. the SGR 33 foreground:
    static constexpr int ansiYellow = 3;

    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "ColorTriggerFilterChild-Test";
    const QString mPort = "4006";
    const QString mLocalhost = "localhost";

    void runLua(const QString& code)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        luaL_dostring(L, code.toUtf8().constData());
    }

    bool luaGlobalTrue(const char* name)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, name);
        const bool value = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return value;
    }

    TTrigger* makeFilterParent(const QString& name, const QString& pattern)
    {
        auto* pT = new TTrigger(nullptr, mpHost);
        pT->setIsFolder(false);
        pT->setTemporary(true);
        pT->setRegexCodeList({pattern}, {REGEX_PERL});
        pT->mFilterTrigger = true;
        pT->registerTrigger();
        pT->setScript(QString());
        pT->setName(name);
        pT->setIsActive(true);
        return pT;
    }

    TTrigger* makeYellowColorTrigger(TTrigger* pParent, const QString& name, const QString& script)
    {
        auto* pT = new TTrigger(pParent, mpHost);
        pT->setIsFolder(false);
        pT->setTemporary(true);
        pT->setupTmpColorTrigger(ansiYellow, TTrigger::scmIgnored);
        pT->registerTrigger();
        pT->setScript(script);
        pT->setName(name);
        pT->setIsActive(true);
        return pT;
    }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForColorTriggerFilterChildTest();

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

    void test_topLevelColorTriggerFires()
    {
        auto* pTop = makeYellowColorTrigger(nullptr, qsl("top level yellow"), qsl("topLevelFired = true"));

        runLua(qsl("topLevelFired = false"));
        runLua(qsl("feedTriggers('\\27[33mtop level control\\27[0m\\n')"));
        QVERIFY2(luaGlobalTrue("topLevelFired"), "top-level color trigger should fire on yellow text");

        pTop->setIsActive(false);
    }

    void test_colorChildUnderFilterParentFires()
    {
        auto* pParent = makeFilterParent(qsl("filter parent"), qsl("^(hello world)$"));
        auto* pChild = makeYellowColorTrigger(pParent, qsl("yellow filter child"), qsl("filterChildFired = true"));

        runLua(qsl("filterChildFired = false"));
        runLua(qsl("feedTriggers('\\27[33mhello world\\27[0m\\n')"));
        QVERIFY2(luaGlobalTrue("filterChildFired"), "color trigger child of a filter parent should fire on a yellow capture");

        pParent->setIsActive(false);
        pChild->setIsActive(false);
    }

    void test_colorChildScansOnlyParentCapture()
    {
        auto* pParent = makeFilterParent(qsl("filter capture parent"), qsl("(world)$"));
        auto* pChild = makeYellowColorTrigger(pParent, qsl("yellow capture child"), qsl("captureChildFired = true"));

        // only "hello" is yellow while the filter passes just "world" through,
        // so the child must not see any yellow text
        runLua(qsl("captureChildFired = false"));
        runLua(qsl("feedTriggers('\\27[33mhello\\27[0m world\\n')"));
        QVERIFY2(!luaGlobalTrue("captureChildFired"), "color child must not fire when the yellow text lies outside the parent's capture");

        runLua(qsl("feedTriggers('hello \\27[33mworld\\27[0m\\n')"));
        QVERIFY2(luaGlobalTrue("captureChildFired"), "color child should fire when the parent's capture is yellow");

        pParent->setIsActive(false);
        pChild->setIsActive(false);
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

void initializeQRCResourcesForColorTriggerFilterChildTest()
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

#include "ColorTriggerFilterChildTest.moc"
QTEST_MAIN(ColorTriggerFilterChildTest)
