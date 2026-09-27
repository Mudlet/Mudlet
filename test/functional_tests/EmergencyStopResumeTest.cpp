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
#include "MudletApp.h"
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
    const QString mPackageName = "emergency stop package";

    // Long enough that nothing created here can fire during the run, so every
    // reading below is of what the emergency stop did and not of a tick
    static constexpr double scmTimerSeconds = 600.0;

    // The offset timer case needs two real intervals: a parent slow enough that
    // the case can watch the offset elapse before it fires, and a child offset
    // well inside it
    static constexpr double scmOffsetParentSeconds = 1.0;
    static constexpr double scmOffsetChildSeconds = 0.2;

    // The one timer here with a real interval, cleared once it has been killed.
    // A case that fails part way through would otherwise leave it ticking
    // through every case after it.
    int mTickerId = 0;

    // Timers left running by a case, switched off again however that case ended
    QStringList mRunningTimerNames;

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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);

        startProfile(mHostname, mLocalhost, mPort);
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanup()
    {
        if (mTickerId) {
            mpHost->getTimerUnit()->killTimer(QString::number(mTickerId));
            mpHost->getTimerUnit()->doCleanup();
            mTickerId = 0;
        }
        for (const QString& name : mRunningTimerNames) {
            mpHost->getTimerUnit()->disableTimer(name);
        }
        mRunningTimerNames.clear();
        // A case that ends with the bomb still pressed would take the next one's
        // events with it - Host::raiseEvent() is silent while mEmergencyStop is
        // set - so every case starts from the resumed state
        mpHost->reenableAllTriggers();
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
        mTickerId = id;
        QVERIFY(unit->getTimer(id));
        // readGlobalInt() cannot tell a missing global from a zero, and the
        // comparison after the stop would then hold as 0 == 0
        QVERIFY2(globalIsNumber(qsl("resumeTicks")), "the timer's callback should have left a number in resumeTicks");
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
        mTickerId = 0;
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
        // -1 is stopped, -2 is no such timer - worth telling apart in a failure
        QCOMPARE(unit->remainingTime(id), -1);

        unit->doCleanup();
        QVERIFY2(!unit->getTimer(id), "the killed timer should still have been freed");
    }

    // A spent one-shot is the corpse that is still isActive(): TTimer::execute()
    // queues it with mpQTimer->stop() + markCleanup() and no deactivate(). The
    // two calls below are what execute() does, rather than a real wait, because
    // Host::slot_timerFires() frees a fired one-shot as soon as it returns.
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
        QCOMPARE(unit->remainingTime(id), -1);

        unit->doCleanup();
        QVERIFY2(!unit->getTimer(id), "the spent one-shot should still have been freed");
    }

    // The other payload a timer can carry: one that only sends a command has no
    // script either, and setIsActive() arms it without asking for one - so it
    // ran happily until the first bomb press and never again after it
    void test_commandOnlyTimerRunsAgainAfterResume()
    {
        auto* unit = mpHost->getTimerUnit();
        const QString name = qsl("emergency stop command timer");
        auto [id, message] = mpHost->mLuaInterpreter.startPermTimer(name, QString(), scmTimerSeconds, QString());
        QVERIFY2(id > 0, qPrintable(message));
        unit->getTimer(id)->setCommand(qsl("look"));
        QVERIFY(unit->enableTimer(name));
        QVERIFY2(unit->remainingTime(id) > 0, "a timer that only sends a command is armed like any other");

        mpHost->stopAllTriggers();
        QCOMPARE(unit->remainingTime(id), -1);

        mpHost->reenableAllTriggers();
        QVERIFY2(unit->remainingTime(id) > 0, "the resume must restart the QTimer of a timer whose payload is a command");
    }

    // enableTimer(folderName) reaches a folder's children through the
    // no-argument overload rather than the by-id one the resume uses, so the
    // same guard has to be right in both. Here the bomb stops everything and the
    // user switches one folder back on from Lua.
    void test_enablingAFolderArmsItsCommandOnlyChild()
    {
        auto* unit = mpHost->getTimerUnit();
        const QString folderName = qsl("emergency stop timer folder");
        auto [folderId, folderMessage] = mpHost->mLuaInterpreter.startPermTimer(folderName, QString(), 0.0, QString());
        QVERIFY2(folderId > 0, qPrintable(folderMessage));
        QVERIFY(unit->getTimer(folderId)->isFolder());

        const QString childName = qsl("emergency stop foldered command timer");
        auto [childId, childMessage] = mpHost->mLuaInterpreter.startPermTimer(childName, folderName, scmTimerSeconds, QString());
        QVERIFY2(childId > 0, qPrintable(childMessage));
        unit->getTimer(childId)->setCommand(qsl("look"));
        QVERIFY(unit->enableTimer(folderName));
        QVERIFY(unit->enableTimer(childName));
        QVERIFY(unit->remainingTime(childId) > 0);

        mpHost->stopAllTriggers();
        QCOMPARE(unit->remainingTime(childId), -1);

        QVERIFY(unit->enableTimer(folderName));
        QVERIFY2(unit->remainingTime(childId) > 0, "enabling a folder must arm its command-only child again");
    }

    // The other timer the no-argument overload is handed: an offset timer, which
    // TimerUnit::enableTimer(name) passes to it directly. Its interval is an
    // offset from its parent firing rather than a schedule of its own, and the
    // parent arms it through enableTimer(int) - so widening the payload test
    // must not let switching one on by name start it early, while the parent
    // firing must still get a command-only one going (it never did before
    // #10751, which is the half of this the widening fixes).
    void test_commandOnlyOffsetTimerWaitsForItsParent()
    {
        auto* unit = mpHost->getTimerUnit();
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl("offsetParentTicks = 0\noffsetChildTicks = 0\n"
                                                                    "function onOffsetTimerSend(_, what)\n"
                                                                    "  if what == 'offsetParentCommand' then offsetParentTicks = offsetParentTicks + 1 end\n"
                                                                    "  if what == 'offsetChildCommand' then offsetChildTicks = offsetChildTicks + 1 end\n"
                                                                    "end\n"
                                                                    "registerAnonymousEventHandler('sysDataSendRequest', 'onOffsetTimerSend')")));
        QVERIFY2(globalIsNumber(qsl("offsetChildTicks")), "the send counters should be numbers before anything is counted into them");

        const QString parentName = qsl("emergency stop offset parent");
        auto [parentId, parentMessage] = mpHost->mLuaInterpreter.startPermTimer(parentName, QString(), scmOffsetParentSeconds, QString());
        QVERIFY2(parentId > 0, qPrintable(parentMessage));
        unit->getTimer(parentId)->setCommand(qsl("offsetParentCommand"));

        const QString childName = qsl("emergency stop offset command timer");
        auto [childId, childMessage] = mpHost->mLuaInterpreter.startPermTimer(childName, parentName, scmOffsetChildSeconds, QString());
        QVERIFY2(childId > 0, qPrintable(childMessage));
        auto* pChild = unit->getTimer(childId);
        pChild->setCommand(qsl("offsetChildCommand"));
        QVERIFY2(pChild->isOffsetTimer(), "a timer whose parent is not a folder is an offset timer");
        QVERIFY2(pChild->getScript().isEmpty(), "this one's only payload is its command");

        mRunningTimerNames << parentName << childName;
        QVERIFY(unit->enableTimer(parentName));
        QVERIFY(unit->enableTimer(childName));
        QVERIFY2(unit->remainingTime(childId) == -1, "switching an offset timer on by name must not give it a schedule of its own - its parent firing is what arms it");

        // long enough that an offset timer running on its own would have sent
        // by now, and short enough that the parent has not fired yet
        QTest::qWait(static_cast<int>(scmOffsetChildSeconds * 2500));
        QCOMPARE(readGlobalInt(qsl("offsetParentTicks")), 0);
        QVERIFY2(readGlobalInt(qsl("offsetChildTicks")) == 0, "a command-only offset timer must not fire before its parent has");

        // and the half the widened guard is there for: once the parent does
        // fire, the command-only child has to arm and send
        QTRY_VERIFY_WITH_TIMEOUT(readGlobalInt(qsl("offsetParentTicks")) > 0, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(readGlobalInt(qsl("offsetChildTicks")) > 0, 5000);
    }

    // The uninstallList half of the resume's skip: an uninstall with a timer
    // script on the call stack - a package auto-updater removing its own package
    // - cannot free that package's timers yet, and the resume walks the very list
    // they are still in. uninstall() switches them off on its way out, so the
    // skip is the second line of defence rather than the only one.
    void test_uninstalledTimerStaysDeadAcrossResume()
    {
        auto* unit = mpHost->getTimerUnit();
        const int id = createFunctionTempTimer(qsl("uninstalledResumeId"));
        auto* pTimer = unit->getTimer(id);
        QVERIFY(pTimer);
        pTimer->mPackageName = mPackageName;

        unit->beginProcessing();
        unit->uninstall(mPackageName);
        unit->endProcessing();
        QVERIFY2(unit->uninstallList.contains(pTimer), "an uninstall with a timer script on the stack has to defer the delete");
        QCOMPARE(unit->remainingTime(id), -1);

        mpHost->stopAllTriggers();
        mpHost->reenableAllTriggers();
        QCOMPARE(unit->remainingTime(id), -1);

        unit->doCleanup();
        QVERIFY2(!unit->getTimer(id), "the uninstalled timer should still have been freed");
    }

    // The resume puts back what the emergency stop took away, not what the
    // profile had switched off itself
    void test_userDisabledTimerStaysOffAcrossResume()
    {
        auto* unit = mpHost->getTimerUnit();
        const int id = createFunctionTempTimer(qsl("disabledResumeId"));
        const QString name = QString::number(id);
        QVERIFY(unit->remainingTime(id) > 0);

        QVERIFY(unit->disableTimer(name));
        QCOMPARE(unit->remainingTime(id), -1);

        mpHost->stopAllTriggers();
        mpHost->reenableAllTriggers();
        QCOMPARE(unit->remainingTime(id), -1);
        QVERIFY2(!unit->getTimer(id)->isActive(), "a timer the user disabled must stay off across a stop and resume");
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

    bool globalIsNumber(const QString& name)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, name.toUtf8().constData());
        const bool isNumber = lua_isnumber(L, -1);
        lua_pop(L, 1);
        return isNumber;
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
        if (host->mTelnet.getConnectionState() != QAbstractSocket::ConnectedState && !spy2.wait(8000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = MudletApp::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

#include "EmergencyStopResumeTest.moc"
MUDLET_GROUPED_TEST_MAIN(EmergencyStopResumeTest)
