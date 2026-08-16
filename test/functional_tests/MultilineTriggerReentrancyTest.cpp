/***************************************************************************
 *   Copyright (C) 2026 by Jay Howard - jay.patrick.howard@gmail.com       *
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

#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TriggerUnit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResources();

// A multiline trigger's completed TMatchState used to stay in mConditionMap
// while its script ran. Feeding text from that script re-enters trigger
// processing on the same trigger, which found the state still complete, fired it
// a second time, and erased it - leaving the outer frame reading a destroyed
// state. Against the unfixed code the first case here does not merely fail, it
// takes the process down with SIGSEGV.
//
// tempComplexRegexTrigger() is the only Lua function that can set the condition
// line delta, which conditions arriving on separate lines need. The perm*Trigger
// variants do make a multiline trigger - they pass (patterns.size() > 1) as the
// flag - but leave the delta at 0, so their conditions must all match one line.
// Called twice under one name it also accumulates patterns into a single trigger.
class MultilineTriggerReentrancyTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-MultilineTriggerReentrancy";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    // The completed state must fire exactly once even though its own script
    // feeds text back through the pipeline.
    void test_completedStateFiresOnceWhenItsScriptFeedsText()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("fireCount = 0\n"
                                                               "fed = false\n"
                                                               "local code = [=[\n"
                                                               "  fireCount = fireCount + 1\n"
                                                               "  if not fed then\n"
                                                               "    fed = true\n"
                                                               "    feedTriggers('zzz filler\\n')\n"
                                                               "  end\n"
                                                               "]=]\n"
                                                               "tempComplexRegexTrigger('MLR', [[^alpha$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)\n"
                                                               "tempComplexRegexTrigger('MLR', [[^beta$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)\n"
                                                               "feedTriggers('alpha\\n')\n"
                                                               "feedTriggers('beta\\n')\n"
                                                               "echo('FIRECOUNT=' .. fireCount .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("FIRECOUNT=1#")), "the completed multiline state fired more than once because the nested pass could still see it in mConditionMap");
    }

    // The filter branch re-reads the state's captures after execute() has
    // returned, so it is the second place a destroyed state was dereferenced.
    void test_filterTriggerSurvivesANestedFeed()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("filterFires = 0\n"
                                                               "filterFed = false\n"
                                                               "local code = [=[\n"
                                                               "  filterFires = filterFires + 1\n"
                                                               "  if not filterFed then\n"
                                                               "    filterFed = true\n"
                                                               "    feedTriggers('zzz filler\\n')\n"
                                                               "  end\n"
                                                               "]=]\n"
                                                               "tempComplexRegexTrigger('MLF', [[^gamma (\\w+)$]], code, 1, 0, 0, 1, 0, 0, 0, 0, 0, 3)\n"
                                                               "tempComplexRegexTrigger('MLF', [[^delta (\\w+)$]], code, 1, 0, 0, 1, 0, 0, 0, 0, 0, 3)\n"
                                                               "feedTriggers('gamma one\\n')\n"
                                                               "feedTriggers('delta two\\n')\n"
                                                               "echo('FILTERFIRES=' .. filterFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("FILTERFIRES=1#")), "a filtering multiline trigger did not fire exactly once across a nested feed");
    }

    // Guards the other direction: moving the state out of mConditionMap before
    // the script runs must not disturb ordinary multiline matching, and the
    // captures gathered across the earlier lines must still reach the script.
    void test_ordinaryMultilineTriggerStillFiresWithItsCaptures()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("plainFires = 0\n"
                                                               "caps = ''\n"
                                                               "local code = [=[\n"
                                                               "  plainFires = plainFires + 1\n"
                                                               "  caps = multimatches[1][2] .. ',' .. multimatches[2][2]\n"
                                                               "]=]\n"
                                                               "tempComplexRegexTrigger('MLP', [[^one (\\w+)$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)\n"
                                                               "tempComplexRegexTrigger('MLP', [[^two (\\w+)$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)\n"
                                                               "feedTriggers('one aaa\\n')\n"
                                                               "feedTriggers('two bbb\\n')\n"
                                                               "echo('PLAIN=' .. plainFires .. '/' .. caps .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("PLAIN=1/aaa,bbb#")), "an ordinary multiline trigger no longer fires once with the captures from both of its lines");
    }

    // A state whose window closes without completing must still be dropped, so
    // the same lines arriving later start a fresh state rather than completing
    // the stale one. Both sides are asserted: a count of zero alone would also be
    // what a silently broken setup produces.
    void test_expiredStateIsStillDiscarded()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        // A delta of 1 lets the state survive exactly one further line, so the
        // consecutive pair completes it and the padded pair must not:
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("expiredFires = 0\n"
                                                               "local code = [=[expiredFires = expiredFires + 1]=]\n"
                                                               "tempComplexRegexTrigger('MLE', [[^first$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1)\n"
                                                               "tempComplexRegexTrigger('MLE', [[^second$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1)\n"
                                                               "feedTriggers('first\\n')\n"
                                                               "feedTriggers('second\\n')\n"
                                                               "echo('CONSECUTIVE=' .. expiredFires .. '#\\n')\n"
                                                               "feedTriggers('first\\n')\n"
                                                               "feedTriggers('padding\\n')\n"
                                                               "feedTriggers('second\\n')\n"
                                                               "echo('PADDED=' .. expiredFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("CONSECUTIVE=1#")), "a multiline state within its line delta did not complete, so the case proves nothing about expiry");
        QVERIFY2(bufferContains(qsl("PADDED=1#")), "a match state outlived its line delta, so a padded second condition completed it");
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mpHostname);
        delete mudlet::self();
    }

private:
    // Starts a profile the way a user would via the GUI (mirrors the helper in
    // TriggerSameLineMatchTest).
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
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    // Joins every physical buffer line and normalises whitespace before
    // matching, so a needle the console word-wraps across lines is still found.
    QString joinedBuffer()
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        QString allText;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            allText.append(console->buffer.line(i)).append(QChar::Space);
        }
        return allText.simplified();
    }

    bool bufferContains(const QString& needle) { return joinedBuffer().contains(needle); }

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

void initializeQRCResources()
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

#include "MultilineTriggerReentrancyTest.moc"
QTEST_MAIN(MultilineTriggerReentrancyTest)
