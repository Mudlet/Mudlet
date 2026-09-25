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
 * The prescan index answers "which of these triggers could match this line?",
 * so that a profile pays for a line's length rather than for its trigger count.
 * That only holds while the index is maintained; a rebuild files every root
 * again, so one rebuild per line puts the trigger count straight back into the
 * per-line cost and gives away the whole point of having an index.
 *
 * setTriggerStayOpen() changes one trigger's filing and so needs one trigger's
 * worth of maintenance. Nothing observable separates that from a rebuild - both
 * leave the same index behind and match identically - so the count of rebuilds
 * is the only thing that can tell them apart, and it is what this watches.
 *
 * Run with: ctest -R PrescanStayOpenRefileTest -V
 */

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "TLuaInterpreter.h"
#include "TriggerUnit.h"
#include "mudlet.h"

#include "GroupedTest.h"

class PrescanStayOpenRefileTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfileName = qsl("PrescanStayOpenRefile-Test");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;

    bool runLua(const QString& script) { return mpHost->mLuaInterpreter.compileAndExecuteScript(script); }

    int luaInteger(const char* name)
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    // 80 is comfortably past the floor of indexed roots the filter needs before
    // it switches on, so that a rebuild costs something to begin with.
    static constexpr int scFillerTriggers = 80;

    bool installProbe(const QString& tag, const QString& probeBody)
    {
        QString setup;
        for (int i = 1; i <= scFillerTriggers; ++i) {
            setup += qsl("permSubstringTrigger('%1Filler%2', '', {'qq%1filler%2zz'}, '')\n").arg(tag).arg(i);
        }
        // The target's own pattern never appears on any line fed below, so every
        // fire it records can only have come from the stay-open window. Without
        // that count, a setTriggerStayOpen() which did nothing at all would
        // satisfy every rebuild budget here perfectly.
        setup += qsl("%1Fires = 0\n%1Open = 0\n").arg(tag);
        setup += qsl("permSubstringTrigger('%1Target', '', {'qq%1nevermatchesqq'}, '%1Fires = %1Fires + 1')\n").arg(tag);
        setup += qsl("permSubstringTrigger('%1Probe', '', {'%1probeline'}, '%2')\n").arg(tag, probeBody);
        return runLua(setup);
    }

    // Each slot leaves the profile as it found it. The index rebuilds once its
    // mutations reach max(32, live roots), so triggers an earlier slot left
    // behind raise the bar the next one is measured against and its budget
    // stops meaning anything.
    void removeProbe(const QString& tag)
    {
        QString teardown;
        for (int i = 1; i <= scFillerTriggers; ++i) {
            teardown += qsl("killTrigger('%1Filler%2')\n").arg(tag).arg(i);
        }
        teardown += qsl("killTrigger('%1Target')\nkillTrigger('%1Probe')\n").arg(tag);
        QVERIFY2(runLua(teardown), "the teardown did not compile, so the next slot would measure against this one's triggers");
        // one line, so the killings are reclaimed and the live-root count the
        // next slot measures against is its own
        runLua(qsl("feedTriggers('teardownline\\n')"));
    }

    quint64 feedAndCountRebuilds(const QString& tag, int lines)
    {
        auto* triggerUnit = mpHost->getTriggerUnit();
        // One line first, so the index is built and switched on before the
        // count is taken, rather than charging the loop below for the
        // profile's first rebuild.
        runLua(qsl("feedTriggers('%1probeline\\n')").arg(tag));
        runLua(qsl("%1Fires = 0").arg(tag));
        const quint64 before = triggerUnit->prescanRebuildCount();
        for (int line = 0; line < lines; ++line) {
            runLua(qsl("feedTriggers('%1probeline %2\\n')").arg(tag).arg(line));
        }
        return triggerUnit->prescanRebuildCount() - before;
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

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        // An empty save, so no default packages install and the only triggers
        // in the profile are this test's; the dialogue slot is what gives the
        // profile the console feedTriggers() prints through.
        const QString folder = MudletPaths::getMudletPath(enums::profileXmlFilesPath, mProfileName);
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

        mpHost = mudlet::self()->loadProfile(mProfileName, false);
        QVERIFY(mpHost);
        QVERIFY(mpHost->mLoadedOk);
        mudlet::self()->slot_connectionDialogueFinished(mProfileName, false);
        QVERIFY(mpHost->mpConsole);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void stayOpenFromAScriptCostsOneTriggerRatherThanAnIndex()
    {
        constexpr int lines = 400;
        // The count alternates so that the trigger really is moved in and out of
        // what the index can filter once a line. Only the call that opens the
        // window does that: by the time the probe sets 0 the target has usually
        // closed itself already, and a call that changes nothing is skipped.
        // Setting the same count every line instead would measure an empty loop.
        QVERIFY2(installProbe(qsl("alternating"),
                              qsl("alternatingOpen = 1 - alternatingOpen "
                                  "setTriggerStayOpen(\"alternatingTarget\", alternatingOpen)")),
                 "the alternating probe triggers did not compile");

        const quint64 rebuilds = feedAndCountRebuilds(qsl("alternating"), lines);
        const int fires = luaInteger("alternatingFires");
        removeProbe(qsl("alternating"));

        QVERIFY2(fires > 0, "the stay-open trigger never fired, so this measured a setTriggerStayOpen() that did nothing");
        // Maintenance is not free of rebuilds entirely - refiling abandons the
        // slot's old entries, and the index reclaims them in one go once they
        // have built up. That is amortised across roughly as many lines as
        // there are triggers, so the budget is generous and still nowhere near
        // the one-per-line a rebuilding path produces.
        const quint64 budget = lines / 50;
        QVERIFY2(rebuilds <= budget,
                 qPrintable(qsl("%1 lines of setTriggerStayOpen() rebuilt the prescan index %2 times, budget %3 - "
                                "the index is being discarded per line rather than maintained")
                                    .arg(lines)
                                    .arg(rebuilds)
                                    .arg(budget)));
    }

    // A script that sets the same stay-open count every line changes nothing the
    // index files on. The refile it would otherwise cost is not merely wasted
    // work: each one counts towards the mutations that buy a rebuild.
    void repeatedStayOpenCallsThatChangeNothingCostNothing()
    {
        constexpr int lines = 400;
        constexpr int callsPerLine = 100;
        QVERIFY2(installProbe(qsl("steady"), qsl("for i = 1, %1 do setTriggerStayOpen(\"steadyTarget\", 3) end").arg(callsPerLine)), "the repeating probe triggers did not compile");

        const quint64 rebuilds = feedAndCountRebuilds(qsl("steady"), lines);
        const int fires = luaInteger("steadyFires");
        removeProbe(qsl("steady"));

        // The window is reopened at 3 before it can ever count down to 0, so
        // the trigger is open on every line. That is what says the count is
        // still being assigned on the path which skips the refile, rather than
        // the call being dropped.
        QCOMPARE(fires, lines);
        QVERIFY2(rebuilds == 0,
                 qPrintable(qsl("%1 calls a line over %2 lines, none of them changing the trigger's filing, "
                                "still rebuilt the prescan index %3 times")
                                    .arg(callsPerLine)
                                    .arg(lines)
                                    .arg(rebuilds)));
    }

    // A script that opens and closes one trigger repeatedly inside a single line
    // queues that one slot to be filed again as many times as it was called,
    // because the index a line is reading from is only patched once the line is
    // over. Filing it once is enough; filing it again for every call spends
    // mutations that buy a rebuild.
    void manyFlipsInsideOneLineCostOneRefile()
    {
        constexpr int lines = 60;
        constexpr int flipsPerLine = 40;
        // The last flip leaves the window open, so the line after it records a
        // fire and an inert loop cannot pass.
        QVERIFY2(installProbe(qsl("flip"), qsl("for i = 1, %1 do setTriggerStayOpen(\"flipTarget\", 1 - (i % 2)) end").arg(flipsPerLine)), "the flipping probe triggers did not compile");

        const quint64 rebuilds = feedAndCountRebuilds(qsl("flip"), lines);
        const int fires = luaInteger("flipFires");
        removeProbe(qsl("flip"));

        QVERIFY2(fires > 0, "the stay-open trigger never fired, so this measured a setTriggerStayOpen() that did nothing");
        const quint64 budget = lines / 20;
        QVERIFY2(rebuilds <= budget,
                 qPrintable(qsl("%1 flips a line over %2 lines rebuilt the prescan index %3 times, budget %4 - "
                                "a slot filed again and again inside one line is costing a rebuild each time")
                                    .arg(flipsPerLine)
                                    .arg(lines)
                                    .arg(rebuilds)
                                    .arg(budget)));
    }
};

#include "PrescanStayOpenRefileTest.moc"
MUDLET_GROUPED_TEST_MAIN(PrescanStayOpenRefileTest)
