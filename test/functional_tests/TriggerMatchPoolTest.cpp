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
 * TriggerMatchPool::shutdown() stops the helper threads; main() calls it before
 * deleting the application. A helper that has parked is asleep on the cursor
 * word, so stopping it means waking it, by the same publish that a batch uses.
 * If that wake were lost, shutdown() would wait forever and this case would
 * time out rather than fail. The matching itself is covered from Lua, in
 * TriggerFlood_spec.lua.
 *
 * Run with: ctest -R TriggerMatchPoolTest -V
 */

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QThread>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TriggerMatchPool.h"
#include "mudlet.h"

#include "GroupedTest.h"

class TriggerMatchPoolTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfileName = qsl("TriggerMatchPool-Test");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static int luaInteger(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        // Read once, when the pool is first used, which nothing in this
        // process has done yet: a grouped case is its own process. One helper,
        // every line a flood, one trigger enough to share out, and no spin
        // budget so the helper parks as soon as a batch is over.
        qputenv("MUDLET_MATCH_THREADS", "2");
        qputenv("MUDLET_MATCH_THRESHOLD", "1");
        qputenv("MUDLET_MATCH_FLOOD_LINES", "1");
        qputenv("MUDLET_MATCH_SPIN_US", "0");

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void shutdownWakesParkedHelper()
    {
        TriggerMatchPool& pool = TriggerMatchPool::instance();
        if (pool.workerCount() == 0) {
            QSKIP("the pool needs a second core before it starts a helper");
        }
        QCOMPARE(pool.workerCount(), 2);

        // An empty save, so no default packages install; the dialogue slot is
        // what gives the profile the console feedTriggers() prints through.
        const QString folder = mudlet::getMudletPath(enums::profileXmlFilesPath, mProfileName);
        QVERIFY(QDir().mkpath(folder));
        QFile save(qsl("%1/2020-01-01#00-00-00.xml").arg(folder));
        QVERIFY(save.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(save.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE MudletPackage>\n"
                           "<MudletPackage version=\"1.001\">\n"
                           "<HostPackage><Host></Host></HostPackage>\n"
                           "</MudletPackage>\n")
                > 0);
        save.close();

        Host* host = mudlet::self()->loadProfile(mProfileName, false);
        QVERIFY(host);
        QVERIFY(host->mLoadedOk);
        mudlet::self()->slot_connectionDialogueFinished(mProfileName, false);
        QVERIFY(host->mpConsole);
        const quint64 prescansBefore = pool.prescanCount();
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("needleCount = 0\n"
                                                               "tempTrigger('needle', [[needleCount = needleCount + 1]])\n"
                                                               "feedTriggers('needle\\n')\n"));
        // The line went through the pool and the trigger still fired, so the
        // helper has consumed a batch and is parked on its epoch.
        QCOMPARE(pool.prescanCount(), prescansBefore + 1);
        QCOMPARE(luaInteger(host, "needleCount"), 1);
        // Gives the helper time to finish its share and park. Nothing here can
        // observe that it has, so the case is only as strong as this wait.
        QThread::msleep(50);

        TriggerMatchPool::shutdown();
        QCOMPARE(pool.workerCount(), 0);

        // Off for good: the same line is declined and matched sequentially.
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("feedTriggers('needle\\n')"));
        QCOMPARE(pool.prescanCount(), prescansBefore + 1);
        QCOMPARE(luaInteger(host, "needleCount"), 2);

        TriggerMatchPool::shutdown();
        QCOMPARE(pool.workerCount(), 0);
    }
};

MUDLET_GROUPED_TEST_MAIN(TriggerMatchPoolTest)
#include "TriggerMatchPoolTest.moc"
