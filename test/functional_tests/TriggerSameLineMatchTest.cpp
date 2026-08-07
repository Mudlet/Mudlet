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

// A trigger created from another trigger's script (tempTrigger() & Co.) must
// still get to match the line being processed. That has been observable
// behaviour for as long as trigger processing iterated the live root-node
// std::list - a push_back lands in front of end(), so the new trigger was
// reached within the same pass - and room-capture scripts ("start capture on
// the room title line, grab it and the following lines") depend on it.
// Iterating a snapshot (introduced by #9267 to fix a use-after-free) silently
// deferred such triggers to the next line and broke those scripts.
class TriggerSameLineMatchTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-TriggerSameLineMatch";
    const QString mpPort = "4000";
    const QString mpLocalhost = "localhost";

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, mpPort.toUShort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    // The classic room-capture pattern: a trigger on the room title line
    // creates a catch-all temp trigger, which must capture the title line
    // itself, not start one line late.
    void test_tempTriggerCreatedInTriggerMatchesCurrentLine()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("captured = {}\n"
                                                               "tempRegexTrigger('^Room 74042', [=[\n"
                                                               "  tempRegexTrigger('^(.*)$', [[table.insert(captured, matches[2])]], 200)\n"
                                                               "]=])\n"
                                                               "feedTriggers('Room 74042: The Bitter Almond Grove\\n')\n"
                                                               "feedTriggers('Exits: North South West\\n')\n"
                                                               "echo('CAPTURED=' .. table.concat(captured, '|') .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("CAPTURED=Room 74042: The Bitter Almond Grove|Exits: North South West#")),
                 "Expected the temp trigger created on the room title line to capture that same line first, then the next line");
    }

    // The new trigger matches the current line after every pre-existing
    // trigger, mirroring where the live-list iteration used to reach it (the
    // end of the list), not right after its creator.
    void test_newTriggerMatchesAfterExistingTriggers()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("order = {}\n"
                                                               "tempRegexTrigger('^o$', [=[\n"
                                                               "  table.insert(order, 'first')\n"
                                                               "  tempRegexTrigger('^o$', [[table.insert(order, 'created')]])\n"
                                                               "]=])\n"
                                                               "tempRegexTrigger('^o$', [[table.insert(order, 'second')]])\n"
                                                               "feedTriggers('o\\n')\n"
                                                               "echo('ORDER=' .. table.concat(order, ',') .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("ORDER=first,second,created#")), "Expected the mid-pass trigger to fire on the current line after all pre-existing triggers");
    }

    // A trigger created by a trigger that was itself created this pass must
    // also match the current line - creation can chain within one line.
    void test_chainedCreationAllMatchCurrentLine()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("chain = {}\n"
                                                               "tempRegexTrigger('^c$', [==[\n"
                                                               "  table.insert(chain, 'creator')\n"
                                                               "  tempRegexTrigger('^c$', [=[\n"
                                                               "    table.insert(chain, 'A')\n"
                                                               "    tempRegexTrigger('^c$', [[table.insert(chain, 'B')]])\n"
                                                               "  ]=])\n"
                                                               "]==])\n"
                                                               "feedTriggers('c\\n')\n"
                                                               "echo('CHAIN=' .. table.concat(chain, ',') .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("CHAIN=creator,A,B#")), "Expected each generation of mid-pass triggers to still match the current line");
    }

    // A single-shot (expireAfter=1) temp trigger created mid-pass spends its
    // one shot on the creating line and must not linger to the next one.
    void test_singleShotFiresOnCreatingLine()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("expiryLine = ''\n"
                                                               "tempRegexTrigger('^e', [=[\n"
                                                               "  tempRegexTrigger('^(.*)$', [[expiryLine = expiryLine .. matches[2] .. ';']], 1)\n"
                                                               "]=], 1)\n"
                                                               "feedTriggers('e one\\n')\n"
                                                               "feedTriggers('e two\\n')\n"
                                                               "echo('EXPIRYLINE=' .. expiryLine .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("EXPIRYLINE=e one;#")), "Expected the single-shot temp trigger to fire once, on the line that created it");
    }

    // tempLineTrigger(0, n, ...) created mid-pass starts counting from the
    // current line, so its first capture is the creating line itself.
    void test_lineTriggerStartsOnCurrentLine()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("lineGrabs = {}\n"
                                                               "tempRegexTrigger('^lstart$', [=[\n"
                                                               "  tempLineTrigger(0, 2, [[table.insert(lineGrabs, getCurrentLine())]])\n"
                                                               "]=], 1)\n"
                                                               "feedTriggers('lstart\\n')\n"
                                                               "feedTriggers('second\\n')\n"
                                                               "feedTriggers('third\\n')\n"
                                                               "echo('LINEGRABS=' .. table.concat(lineGrabs, ',') .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("LINEGRABS=lstart,second#")), "Expected the mid-pass line trigger to grab the creating line and the one after it");
    }

    // With nested processing (the creator also calls feedTriggers()), the new
    // trigger matches the nested line while it is being processed and still
    // matches the outer line afterwards - in that order.
    void test_nestedFeedTriggersMatchesBothLines()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("nested = {}\n"
                                                               "tempRegexTrigger('^outer$', [=[\n"
                                                               "  tempRegexTrigger('^(.*)$', [[table.insert(nested, matches[2])]], 10)\n"
                                                               "  feedTriggers('inner\\n')\n"
                                                               "]=], 1)\n"
                                                               "feedTriggers('outer\\n')\n"
                                                               "echo('NESTED=' .. table.concat(nested, ',') .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("NESTED=inner,outer#")), "Expected the mid-pass trigger to match the nested line first, then the outer line it was created on");
    }

    // The naive "one-shot that re-arms itself at the end of its own handler" is
    // the shape users write. Without the budget this does not fail, it hangs.
    void test_selfRecreatingTriggerIsStopped()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("loopFires = 0\n"
                                                               "function arm()\n"
                                                               "  tempRegexTrigger('^hploop$', [[loopFires = loopFires + 1; arm()]], 1)\n"
                                                               "end\n"
                                                               "arm()\n"
                                                               "feedTriggers('hploop\\n')\n"
                                                               "echo('LOOPFIRES=' .. loopFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "Expected the same-line re-creation abort error in the console buffer");
        // one fire from the trigger already there, then one per budgeted creation;
        // the trailing # keeps the check from also passing on ten times the number
        const int expectedFires = 1 + static_cast<int>(TriggerUnit::scmMaxSameLineCreations);
        QVERIFY2(bufferContains(qsl("LOOPFIRES=%1#").arg(expectedFires)), qPrintable(qsl("Expected the re-arming trigger to fire exactly %1 times").arg(expectedFires)));
    }

    void test_selfRecreatingTriggerAbortNamesTheTrigger()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("function armNamed()\n"
                                                               "  tempComplexRegexTrigger('hpWatcher', '^hpnamed$', [[armNamed()]], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1)\n"
                                                               "end\n"
                                                               "armNamed()\n"
                                                               "feedTriggers('hpnamed\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("trigger 'hpWatcher'")), "Expected the abort message to name the trigger that keeps re-creating itself");
    }

    void test_finiteCreationChainIsUnaffected()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("chainFires = 0\n"
                                                               "function chainStep()\n"
                                                               "  chainFires = chainFires + 1\n"
                                                               "  if chainFires < 10 then\n"
                                                               "    tempRegexTrigger('^chain$', [[chainStep()]], 1)\n"
                                                               "  end\n"
                                                               "end\n"
                                                               "tempRegexTrigger('^chain$', [[chainStep()]], 1)\n"
                                                               "feedTriggers('chain\\n')\n"
                                                               "echo('CHAINFIRES=' .. chainFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("CHAINFIRES=10#")), "Expected all ten generations of the finite chain to match the current line");
        QVERIFY2(!bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "A chain that ends on its own must not trip the same-line generation budget");
    }

    // Without disowning what the loop created, each line costs a multiple of the
    // one before it, so the freeze is postponed rather than prevented.
    void test_selfRecreatingTriggerDoesNotAccumulateAcrossLines()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("keptFires = 0\n"
                                                               "function armKept()\n"
                                                               "  tempRegexTrigger('^kept$', [[keptFires = keptFires + 1; armKept()]])\n"
                                                               "end\n"
                                                               "armKept()\n"
                                                               "feedTriggers('kept\\n')\n"
                                                               "feedTriggers('kept\\n')\n"
                                                               "echo('KEPTFIRES=' .. keptFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        const int firesPerLine = 1 + static_cast<int>(TriggerUnit::scmMaxSameLineCreations);
        QVERIFY2(bufferContains(qsl("KEPTFIRES=%1#").arg(2 * firesPerLine)),
                 qPrintable(qsl("Expected the second line to cost the same %1 fires as the first, not a multiple of them").arg(firesPerLine)));
    }

    // Permanent triggers are saved with the profile, so they are stopped without
    // being deleted and with deactivate(), which leaves the user-active state
    // XMLexport writes alone.
    void test_selfRecreatingPermanentTriggerIsStoppedButNotDeleted()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("permFires = 0\n"
                                                               "function armPerm()\n"
                                                               "  permRegexTrigger('Perm Loop', '', {'^permloop$'}, [[permFires = permFires + 1; armPerm()]])\n"
                                                               "end\n"
                                                               "armPerm()\n"
                                                               "feedTriggers('permloop\\n')\n"
                                                               "echo('PERMFIRES=' .. permFires .. '#\\n')\n"
                                                               "echo('PERMACTIVE=' .. isActive('Perm Loop', 'trigger') .. '#\\n')\n"
                                                               "echo('PERMEXISTS=' .. exists('Perm Loop', 'trigger') .. '#\\n')\n"));

        const int expectedFires = 1 + static_cast<int>(TriggerUnit::scmMaxSameLineCreations);
        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "Expected a permanent trigger re-creating itself to be stopped too");
        QVERIFY2(bufferContains(qsl("PERMFIRES=%1#").arg(expectedFires)), qPrintable(qsl("Expected the re-arming permanent trigger to fire exactly %1 times").arg(expectedFires)));
        QVERIFY2(bufferContains(qsl("PERMACTIVE=1#")), "Expected only the trigger that predates the line to still be active");
        QVERIFY2(bufferContains(qsl("PERMEXISTS=%1#").arg(expectedFires + 1)), "Expected the stopped permanent triggers to still exist - stopping them is not deleting them");
    }

    // The outer line's own mid-pass triggers were registered before the nested
    // pass began, so its abort must not take them.
    void test_nestedPassAbortLeavesTheOuterLineAlone()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("seen = {}\n"
                                                               "function armInner()\n"
                                                               "  tempRegexTrigger('^inner$', [[armInner()]], 1)\n"
                                                               "end\n"
                                                               "armInner()\n"
                                                               "tempRegexTrigger('^outer$', [=[\n"
                                                               "  tempRegexTrigger('^(.*)$', [[table.insert(seen, matches[2])]], 10)\n"
                                                               "  feedTriggers('inner\\n')\n"
                                                               "]=], 1)\n"
                                                               "feedTriggers('outer\\n')\n"
                                                               "echo('SEEN=' .. table.concat(seen, ',') .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "Expected the runaway in the nested pass to be stopped");
        QVERIFY2(bufferContains(qsl("SEEN=inner,outer#")), "Expected the capture trigger created by the outer line to survive the nested pass's abort and still match the outer line");
    }

    // Not a feedTriggers() curiosity - real socket text takes the same path - and
    // driving it from the socket also proves the abort leaves the event loop running.
    void test_selfRecreatingTriggerFromServerTextIsStopped()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("function armFromServer()\n"
                                                               "  tempRegexTrigger('^HP: 100/100$', [[armFromServer()]], 1)\n"
                                                               "end\n"
                                                               "armFromServer()\n"));

        mpServer->sendRaw(QByteArray("HP: 100/100\r\n"));
        QTRY_VERIFY2_WITH_TIMEOUT(bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "Expected server text to reach the same-line generation budget and be stopped", 10000);
        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mpHostname);
        delete mudlet::self();
    }

    // Starts a profile the way a user would via the GUI (mirrors the helper in
    // TelnetTextDisplayedTest).
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

#include "TriggerSameLineMatchTest.moc"
QTEST_MAIN(TriggerSameLineMatchTest)
