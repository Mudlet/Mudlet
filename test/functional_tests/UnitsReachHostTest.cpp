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
 * The scriptable units reach the main console only through their Host: a
 * timer's QTimer fires into Host::slot_timerFires(), a colorizer trigger
 * recolors the current line through the Host's selection forwarders and a
 * push-down action records its state on the main console's model. Each case
 * drives one unit and reads the result back off the model or off Lua.
 *
 * Run with: ctest -R UnitsReachHostTest -V
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "ActionUnit.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TAction.h"
#include "TBuffer.h"
#include "TConsoleModel.h"
#include "TLuaInterpreter.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
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

#include "GroupedTest.h"

class UnitsReachHostTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "UnitsReachHost-Test";
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
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

    int luaInt(const QString& global)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, global.toUtf8().constData());
        const int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    TBuffer& buffer() { return mpHost->mainConsoleModel().buffer; }

    int lineContaining(const QString& text)
    {
        for (int i = buffer().getLastLineNumber(); i >= 0; --i) {
            if (buffer().line(i).contains(text)) {
                return i;
            }
        }
        return -1;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so a concurrent copy of this test
        // does not find the profile name already in use. Since #9712 the opt-in
        // that makes setupConfig() adopt a directory is
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

    void test_timerFiresThroughItsHost()
    {
        auto* pTimer = new TTimer(qsl("urh-timer"), QTime(0, 0, 0, 50), mpHost);
        QVERIFY(mpHost->getTimerUnit()->registerTimer(pTimer));
        QVERIFY2(pTimer->setScript(qsl("urhFired = (urhFired or 0) + 1")), "timer script failed to compile");
        pTimer->setIsActive(true);
        pTimer->enableTimer();

        QTRY_VERIFY2_WITH_TIMEOUT(luaInt(qsl("urhFired")) >= 1, "the timer never ran its script", 3000);

        // The destructor unregisters the timer and stops its QTimer
        delete pTimer;
    }

    void test_colorizerTriggerRecolorsTheMainConsoleLine()
    {
        const QColor fg(12, 34, 56);
        const QColor bg(65, 43, 21);
        auto* pTrigger = new TTrigger(qsl("urh-color"), QStringList{qsl("urh paint me")}, QList<int>{REGEX_SUBSTRING}, false, mpHost);
        pTrigger->setIsColorizerTrigger(true);
        pTrigger->setColorizerFgColor(fg);
        pTrigger->setColorizerBgColor(bg);
        pTrigger->setIsActive(true);
        QVERIFY(mpHost->getTriggerUnit()->registerTrigger(pTrigger));

        runLua(qsl("feedTriggers('before urh paint me after\\n')"));

        const int line = lineContaining(qsl("urh paint me"));
        QVERIFY2(line >= 0, "the fed line never reached the main console buffer");
        const int start = buffer().line(line).indexOf(qsl("urh paint me"));
        const auto& chars = buffer().buffer.at(line);
        QCOMPARE(chars.at(start).foreground(), fg);
        QCOMPARE(chars.at(start).background(), bg);
        QCOMPARE(chars.at(start + 11).foreground(), fg);
        QVERIFY2(chars.at(start - 1).foreground() != fg, "the color spilled outside the match");
        QVERIFY2(chars.at(start + 12).foreground() != fg, "the color spilled outside the match");

        delete pTrigger;
    }

    void test_pushDownActionStateLandsOnTheMainConsoleModel()
    {
        auto* pAction = new TAction(qsl("urh-button"), mpHost);
        pAction->setIsPushDownButton(true);
        pAction->setIsActive(true);
        QVERIFY(mpHost->getActionUnit()->registerAction(pAction));

        pAction->mButtonState = true;
        pAction->execute();
        QCOMPARE(mpHost->mainConsoleModel().mButtonState, 2);
        runLua(qsl("urhButton = getButtonState()"));
        QCOMPARE(luaInt(qsl("urhButton")), 2);

        pAction->mButtonState = false;
        pAction->execute();
        QCOMPARE(mpHost->mainConsoleModel().mButtonState, 1);
        runLua(qsl("urhButton = getButtonState()"));
        QCOMPARE(luaInt(qsl("urhButton")), 1);

        delete pAction;
    }

private:
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
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

#include "UnitsReachHostTest.moc"
MUDLET_GROUPED_TEST_MAIN(UnitsReachHostTest)
