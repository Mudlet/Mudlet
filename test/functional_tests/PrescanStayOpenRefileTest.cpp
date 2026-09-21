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
        auto* triggerUnit = mpHost->getTriggerUnit();
        QVERIFY(triggerUnit);

        // The index only switches on past a floor of indexed roots, so there
        // have to be enough of them for a rebuild to cost anything at all.
        constexpr int fillerTriggers = 80;
        constexpr int lines = 400;

        QString setup;
        for (int i = 1; i <= fillerTriggers; ++i) {
            setup += qsl("permSubstringTrigger('prescanFiller%1', '', {'qqfiller%1zz'}, '')\n").arg(i);
        }
        // Its own pattern never appears, so the call below is the only thing
        // that ever refiles it - which keeps this measuring the script-driven
        // path rather than the one match() takes when a window runs out.
        setup += qsl("permSubstringTrigger('prescanStayOpen', '', {'qqnevermatchesqq'}, '')\n");
        setup += qsl("prescanProbeFires = 0\n");
        setup += qsl("permSubstringTrigger('prescanProbe', '', {'prescanprobeline'}, "
                     "'prescanProbeFires = prescanProbeFires + 1 setTriggerStayOpen(\"prescanStayOpen\", 0)')\n");
        QVERIFY2(runLua(setup), "the probe triggers did not compile");

        // One line first, so the index is built and switched on before the
        // count is taken - otherwise the profile's first rebuild would be
        // charged to the loop below.
        QVERIFY(runLua(qsl("feedTriggers('prescanprobeline\\n')")));
        const quint64 rebuildsBefore = triggerUnit->prescanRebuildCount();

        for (int line = 0; line < lines; ++line) {
            QVERIFY(runLua(qsl("feedTriggers('prescanprobeline %1\\n')").arg(line)));
        }
        const quint64 rebuilds = triggerUnit->prescanRebuildCount() - rebuildsBefore;

        // Without this, the assertion below holds vacuously: a probe that never
        // matched would call setTriggerStayOpen() no times and rebuild nothing.
        QCOMPARE(luaInteger("prescanProbeFires"), lines + 1);

        // Maintenance is not free of rebuilds entirely - refiling abandons the
        // slot's old entries, and the index reclaims them in one go once they
        // have built up. That is amortised across roughly as many lines as
        // there are triggers, so the budget is generous and still nowhere near
        // the one-per-line a rebuilding path produces.
        const quint64 budget = lines / 10;
        QVERIFY2(rebuilds <= budget,
                 qPrintable(qsl("%1 lines of setTriggerStayOpen() rebuilt the prescan index %2 times, budget %3 - "
                                "the index is being discarded per line rather than maintained")
                                    .arg(lines)
                                    .arg(rebuilds)
                                    .arg(budget)));
    }
};

#include "PrescanStayOpenRefileTest.moc"
MUDLET_GROUPED_TEST_MAIN(PrescanStayOpenRefileTest)
