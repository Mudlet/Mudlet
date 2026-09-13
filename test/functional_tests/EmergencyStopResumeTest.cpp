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
 * The emergency stop - the console's bomb button and Ctrl+Alt+S, both of which
 * land in TConsole::slot_stopAllItems() - stops every item of the profile and
 * puts them all back when it is toggled off again. What the toggle does to
 * timers is checked here, because a timer is the one item with a QTimer behind
 * it: being active again is not the same as running again.
 *
 * - a tempTimer() created from a Lua function has no script string, since its
 *   callback lives in the Lua registry, and the resume used to leave the QTimer
 *   of one stopped for the rest of the session (#10751)
 * - the resume must not restart a timer that is only waiting to be freed,
 *   whether it was killed or is a spent one-shot (#9887). A killed timer keeps
 *   shouldBeActive() true, so nothing in the activation path alone stops the
 *   corpse coming back
 *
 * Lua drives the creation of the function timer rather than a direct
 * startTempTimer() call, because it is tempTimer()'s own binding that registers
 * the callback and sets mRegisteredAnonymousLuaFunction.
 *
 * Run with: ctest -R EmergencyStopResumeTest -V
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TTimer.h"
#include "TelnetServerStub.h"
#include "TimerUnit.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

class EmergencyStopResumeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "EmergencyStopResume-Test";
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = "localhost";

    // Long enough that nothing created here can fire during the run, so every
    // reading below is of what the emergency stop did and not of a tick
    static constexpr double scmTimerSeconds = 600.0;

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so a concurrent copy of this test
        // does not share a profile list with it - see UnitDeferredDeleteTest
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

    // #10751: the resume only restarted the QTimer of a timer that had a script
    // string, which a function timer never has
    void test_functionTempTimerRunsAgainAfterResume()
    {
        auto* unit = mpHost->getTimerUnit();
        const int id = createFunctionTempTimer(qsl("functionResumeId"));
        auto* pTimer = unit->getTimer(id);
        QVERIFY(pTimer);
        QVERIFY2(pTimer->mRegisteredAnonymousLuaFunction, "tempTimer() with a function argument should have registered that function");
        QVERIFY2(pTimer->getScript().isEmpty(), "a function timer's callback lives in the Lua registry, not in a script string");
        QVERIFY(unit->remainingTime(id) > 0);

        mpHost->stopAllTriggers();
        QCOMPARE(unit->remainingTime(id), -1);

        mpHost->reenableAllTriggers();
        QVERIFY2(pTimer->isActive(), "the resume should have re-activated the function timer");
        QVERIFY2(unit->remainingTime(id) > 0, "the resume must restart the QTimer of a temporary timer created from a Lua function, not just flag it active");
    }

    // The same resume seen from the player's side: an armed QTimer and a callback
    // that runs again are not the same claim, and it is the tick count that a
    // package notices. A real interval rather than a probe of the timer's state,
    // so the whole path - QTimer, TTimer::execute() and the registry lookup the
    // callback needs - is on trial.
    void test_functionTempTimerFiresAgainAfterResume()
    {
        auto* unit = mpHost->getTimerUnit();
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("resumeTicks = 0")));
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("resumeTickerId = tempTimer(0.1, function() resumeTicks = resumeTicks + 1 end, true)")));
        const int id = readGlobalInt(qsl("resumeTickerId"));
        QVERIFY(unit->getTimer(id));
        QTRY_VERIFY_WITH_TIMEOUT(readGlobalInt(qsl("resumeTicks")) > 0, 5000);

        mpHost->stopAllTriggers();
        const int ticksAtStop = readGlobalInt(qsl("resumeTicks"));
        QTest::qWait(500);
        QCOMPARE(readGlobalInt(qsl("resumeTicks")), ticksAtStop);

        mpHost->reenableAllTriggers();
        QTRY_VERIFY_WITH_TIMEOUT(readGlobalInt(qsl("resumeTicks")) > ticksAtStop, 5000);

        // it would otherwise go on ticking through the cases below
        QVERIFY(unit->killTimer(QString::number(id)));
        unit->doCleanup();
    }

    // The same timer written the other way round, which always resumed - so a
    // failure here is the emergency stop itself, not the function case
    void test_scriptTempTimerRunsAgainAfterResume()
    {
        auto* unit = mpHost->getTimerUnit();
        auto [id, message] = mpHost->mLuaInterpreter.startTempTimer(scmTimerSeconds, qsl("scriptResumeTicks = scriptResumeTicks + 1"), true);
        QVERIFY2(id > 0, qPrintable(message));
        QVERIFY(unit->remainingTime(id) > 0);

        mpHost->stopAllTriggers();
        QCOMPARE(unit->remainingTime(id), -1);

        mpHost->reenableAllTriggers();
        QVERIFY2(unit->remainingTime(id) > 0, "the resume must restart the QTimer of a temporary timer created from a script string");
    }

    void test_permanentTimerRunsAgainAfterResume()
    {
        auto* unit = mpHost->getTimerUnit();
        const QString name = qsl("emergency stop permanent timer");
        auto [id, message] = mpHost->mLuaInterpreter.startPermTimer(name, QString(), scmTimerSeconds, qsl("permanentResumeTicks = permanentResumeTicks + 1"));
        QVERIFY2(id > 0, qPrintable(message));
        // permTimer() hands back a timer that is switched off, as the Lua API does
        QVERIFY(unit->enableTimer(name));
        QVERIFY(unit->remainingTime(id) > 0);

        mpHost->stopAllTriggers();
        QCOMPARE(unit->remainingTime(id), -1);

        mpHost->reenableAllTriggers();
        QVERIFY2(unit->remainingTime(id) > 0, "the resume must restart the QTimer of a permanent timer");
    }

    // #9887: killTimer() deactivates without clearing mUserActiveState, so the
    // corpse still answers shouldBeActive() and passes canBeUnlocked() - only
    // being queued for deletion marks it out
    void test_killedFunctionTempTimerStaysDeadAcrossResume()
    {
        auto* unit = mpHost->getTimerUnit();
        const int id = createFunctionTempTimer(qsl("killedResumeId"));
        auto* pTimer = unit->getTimer(id);
        QVERIFY(pTimer);

        QVERIFY2(unit->killTimer(QString::number(id)), "the temporary timer should be killable by name");
        QVERIFY(unit->mCleanupSet.contains(pTimer));
        QCOMPARE(unit->remainingTime(id), -1);

        mpHost->stopAllTriggers();
        mpHost->reenableAllTriggers();
        QVERIFY2(unit->remainingTime(id) == -1, "the resume must not re-arm a killed timer that doCleanup() has not got to yet");

        unit->doCleanup();
        QVERIFY2(!unit->getTimer(id), "the killed timer should still have been freed");
    }

    // A spent one-shot is the corpse that is still isActive(): TTimer::execute()
    // queues it with mpQTimer->stop() + markCleanup() and no deactivate(). The
    // two calls below are what execute() does, rather than a real wait, because
    // mudlet::slot_timerFires() frees a fired one-shot as soon as it returns.
    void test_spentOneShotFunctionTimerStaysDeadAcrossResume()
    {
        auto* unit = mpHost->getTimerUnit();
        const int id = createFunctionTempTimer(qsl("spentResumeId"), false);
        auto* pTimer = unit->getTimer(id);
        QVERIFY(pTimer);

        pTimer->stop();
        unit->markCleanup(pTimer);
        QVERIFY2(pTimer->isActive(), "a spent one-shot is queued for cleanup without being deactivated");
        QCOMPARE(unit->remainingTime(id), -1);

        mpHost->stopAllTriggers();
        mpHost->reenableAllTriggers();
        QVERIFY2(unit->remainingTime(id) == -1, "the resume must not re-arm a one-shot that has already fired");

        unit->doCleanup();
        QVERIFY2(!unit->getTimer(id), "the spent one-shot should still have been freed");
    }

private:
    // tempTimer(seconds, function() ... end, repeating), run as a player would
    // run it, with the id left in a global for the caller to read back
    int createFunctionTempTimer(const QString& globalName, bool repeating = true)
    {
        const QString code = qsl("%1 = tempTimer(%2, function() end, %3)").arg(globalName, QString::number(scmTimerSeconds), repeating ? qsl("true") : qsl("false"));
        if (!mpHost->mLuaInterpreter.compileAndExecuteScript(code)) {
            return -1;
        }
        return readGlobalInt(globalName);
    }

    int readGlobalInt(const QString& name)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, name.toUtf8().constData());
        const int value = static_cast<int>(lua_tointeger(L, -1));
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

#include "EmergencyStopResumeTest.moc"
MUDLET_GROUPED_TEST_MAIN(EmergencyStopResumeTest)
