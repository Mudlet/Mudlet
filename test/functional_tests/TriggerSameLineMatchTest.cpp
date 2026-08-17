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

#include "ProfileTestHelper.h"
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
        const int expectedFires = 1 + TriggerUnit::scmMaxSameLineGenerations;
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
        const int firesPerLine = 1 + TriggerUnit::scmMaxSameLineGenerations;
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

        const int expectedFires = 1 + TriggerUnit::scmMaxSameLineGenerations;
        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "Expected a permanent trigger re-creating itself to be stopped too");
        QVERIFY2(bufferContains(qsl("PERMFIRES=%1#").arg(expectedFires)), qPrintable(qsl("Expected the re-arming permanent trigger to fire exactly %1 times").arg(expectedFires)));
        QVERIFY2(bufferContains(qsl("PERMACTIVE=1#")), "Expected only the trigger that predates the line to still be active");
        QVERIFY2(bufferContains(qsl("PERMEXISTS=%1#").arg(expectedFires + 1)), "Expected the stopped permanent triggers to still exist - stopping them is not deleting them");
    }

    // A script arming a batch of unrelated triggers is not a runaway, however
    // big the batch: each of them starts a creation chain of its own, and none
    // of those chains ever gets a second link.
    void test_bulkUnrelatedCreationsAreNotStopped()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        const int bulkCount = TriggerUnit::scmMaxSameLineGenerations + 1;
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("bulkFires = 0\n"
                                                               "tempRegexTrigger('^bulkgate$', [=[\n"
                                                               "  for i = 1, %1 do\n"
                                                               "    tempRegexTrigger('^bulkpay$', [[bulkFires = bulkFires + 1]])\n"
                                                               "  end\n"
                                                               "]=], 1)\n"
                                                               "feedTriggers('bulkgate\\n')\n"
                                                               "feedTriggers('bulkpay\\n')\n"
                                                               "echo('BULKFIRES=' .. bulkFires .. '#\\n')\n")
                                                                   .arg(bulkCount));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(!bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "A batch of unrelated triggers must not be mistaken for a trigger re-creating itself");
        QVERIFY2(bufferContains(qsl("BULKFIRES=%1#").arg(bulkCount)), qPrintable(qsl("Expected all %1 triggers armed on the previous line to survive and fire").arg(bulkCount)));
    }

    // Two scripts arming triggers on one line get a budget each, so neither can
    // exhaust the other's - together they come to more than one budget's worth.
    void test_twoScriptsArmingOnOneLineKeepBothSetsOfTriggers()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        const int eachCount = (TriggerUnit::scmMaxSameLineGenerations / 2) + 1;
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("firesA, firesB = 0, 0\n"
                                                               "tempRegexTrigger('^sharedgate$', [=[\n"
                                                               "  for i = 1, %1 do\n"
                                                               "    tempRegexTrigger('^payA$', [[firesA = firesA + 1]])\n"
                                                               "  end\n"
                                                               "]=], 1)\n"
                                                               "tempRegexTrigger('^sharedgate$', [=[\n"
                                                               "  for i = 1, %1 do\n"
                                                               "    tempRegexTrigger('^payB$', [[firesB = firesB + 1]])\n"
                                                               "  end\n"
                                                               "]=], 1)\n"
                                                               "feedTriggers('sharedgate\\n')\n"
                                                               "feedTriggers('payA\\n')\n"
                                                               "feedTriggers('payB\\n')\n"
                                                               "echo('SHARED=' .. firesA .. ',' .. firesB .. '#\\n')\n")
                                                                   .arg(eachCount));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("SHARED=%1,%1#").arg(eachCount)), qPrintable(qsl("Expected both scripts to keep all %1 of the triggers they armed").arg(eachCount)));
    }

    // The batch is armed by a trigger that was itself created on this line, so
    // creator and batch share a lineage. Counting a lineage's members rather than
    // its generations condemns the whole batch here, which is the room-capture
    // shape: the room-title trigger creates the capture trigger, and the capture
    // trigger is what arms the batch.
    void test_bulkCreationsFromAMidLineTriggerAreNotStopped()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        const int bulkCount = TriggerUnit::scmMaxSameLineGenerations + 1;
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("deepFires = 0\n"
                                                               "tempRegexTrigger('^deepgate$', [===[\n"
                                                               "  tempRegexTrigger('^deepgate$', [==[\n"
                                                               "    for i = 1, %1 do\n"
                                                               "      tempRegexTrigger('^deeppay$', [[deepFires = deepFires + 1]])\n"
                                                               "    end\n"
                                                               "  ]==], 1)\n"
                                                               "]===], 1)\n"
                                                               "feedTriggers('deepgate\\n')\n"
                                                               "feedTriggers('deeppay\\n')\n"
                                                               "echo('DEEPFIRES=' .. deepFires .. '#\\n')\n")
                                                                   .arg(bulkCount));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(!bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "A batch is one generation wherever it is armed from, and must not be mistaken for a runaway");
        QVERIFY2(bufferContains(qsl("DEEPFIRES=%1#").arg(bulkCount)), qPrintable(qsl("Expected all %1 triggers armed by a trigger created on the same line to survive and fire").arg(bulkCount)));
    }

    // Permanent triggers take the same path, and are the more painful loss - a
    // "rebuild my triggers when the game says X" routine arms them in bulk.
    void test_bulkPermanentCreationsAreNotStopped()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        const int bulkCount = TriggerUnit::scmMaxSameLineGenerations + 1;
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("permBulkFires = 0\n"
                                                               "function permBulkStep() permBulkFires = permBulkFires + 1 end\n"
                                                               "tempRegexTrigger('^permgate$', [=[\n"
                                                               "  for i = 1, %1 do\n"
                                                               "    permRegexTrigger('PermBulk' .. i, '', {'^permpay$'}, [[permBulkStep()]])\n"
                                                               "  end\n"
                                                               "]=], 1)\n"
                                                               "feedTriggers('permgate\\n')\n"
                                                               "feedTriggers('permpay\\n')\n"
                                                               "echo('PERMBULK=' .. permBulkFires .. '#\\n')\n"
                                                               "echo('PERMBULKACTIVE=' .. isActive('PermBulk%1', 'trigger') .. '#\\n')\n")
                                                                   .arg(bulkCount));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("PERMBULK=%1#").arg(bulkCount)), qPrintable(qsl("Expected all %1 permanent triggers armed on the previous line to survive and fire").arg(bulkCount)));
        QVERIFY2(bufferContains(qsl("PERMBULKACTIVE=1#")), "Expected the permanent triggers to be left switched on");
    }

    // The whole point of the budget being per chain: the runaway loses its
    // triggers, the script that happened to arm a trigger on the same line does not.
    void test_runawayChainSparesTriggersFromOtherScripts()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("innocentFires = 0\n"
                                                               "function armRunaway()\n"
                                                               "  tempRegexTrigger('^runline$', [[armRunaway()]], 1)\n"
                                                               "end\n"
                                                               "armRunaway()\n"
                                                               "tempRegexTrigger('^runline$', [=[\n"
                                                               "  tempRegexTrigger('^innocent$', [[innocentFires = innocentFires + 1]])\n"
                                                               "]=], 1)\n"
                                                               "feedTriggers('runline\\n')\n"
                                                               "feedTriggers('innocent\\n')\n"
                                                               "echo('INNOCENT=' .. innocentFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "Expected the self-recreating chain to still be stopped");
        QVERIFY2(bufferContains(qsl("INNOCENT=1#")), "Expected the trigger armed by an unrelated script on the same line to survive the runaway's abort and fire");
    }

    // A lineage of exactly the budget's depth ends on its own; the trip is on the
    // generation after it, which test_selfRecreatingTriggerIsStopped() pins from
    // the other side. Both land on the same fire count, so the presence or
    // absence of the abort message is what tells the two apart.
    void test_chainExactlyAtTheLimitIsNotStopped()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        const int limit = TriggerUnit::scmMaxSameLineGenerations;
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("boundFires = 0\n"
                                                               "function boundStep()\n"
                                                               "  boundFires = boundFires + 1\n"
                                                               "  if boundFires <= %1 then\n"
                                                               "    tempRegexTrigger('^boundline$', [[boundStep()]], 1)\n"
                                                               "  end\n"
                                                               "end\n"
                                                               "tempRegexTrigger('^boundline$', [[boundStep()]], 1)\n"
                                                               "feedTriggers('boundline\\n')\n"
                                                               "echo('BOUNDFIRES=' .. boundFires .. '#\\n')\n")
                                                                   .arg(limit));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(!bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "A chain of exactly the budget's length ends on its own and must not be stopped");
        QVERIFY2(bufferContains(qsl("BOUNDFIRES=%1#").arg(limit + 1)), qPrintable(qsl("Expected the chain to run to its own end, %1 fires").arg(limit + 1)));
    }

    // Once the line that created a trigger is done with, that trigger is as
    // ordinary as any other and what it creates starts fresh chains - otherwise
    // it would carry its creator's chain around for the rest of the session.
    void test_aTriggerOutlivingItsLineStartsFreshChains()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        const int bulkCount = TriggerUnit::scmMaxSameLineGenerations + 1;
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("laterFires = 0\n"
                                                               "tempRegexTrigger('^egate$', [==[\n"
                                                               "  tempRegexTrigger('^esecond$', [=[\n"
                                                               "    for i = 1, %1 do\n"
                                                               "      tempRegexTrigger('^epay$', [[laterFires = laterFires + 1]])\n"
                                                               "    end\n"
                                                               "  ]=], 1)\n"
                                                               "]==], 1)\n"
                                                               "feedTriggers('egate\\n')\n"
                                                               "feedTriggers('esecond\\n')\n"
                                                               "feedTriggers('epay\\n')\n"
                                                               "echo('LATERFIRES=' .. laterFires .. '#\\n')\n")
                                                                   .arg(bulkCount));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(!bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "A trigger created on an earlier line is not part of a chain any more and must arm freely");
        QVERIFY2(bufferContains(qsl("LATERFIRES=%1#").arg(bulkCount)), qPrintable(qsl("Expected all %1 triggers armed on the later line to survive and fire").arg(bulkCount)));
    }

    // A lineage that starts in the outer pass and runs away inside a nested
    // feedTriggers() has members either side of the nested pass's first-node
    // index, which is why stopping one scans the whole list rather than the
    // tail of the tripping pass. Scanning only the tail leaves the first link
    // alive, and the outer pass then has to trip on the same lineage all over
    // again - the fire count is what shows that, at twice this number.
    void test_runawayCrossingIntoANestedPassIsStoppedWhole()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("nestFires, nestSafeFires = 0, 0\n"
                                                               "function armNested()\n"
                                                               "  tempRegexTrigger('^nestin$', [[nestFires = nestFires + 1; armNested()]])\n"
                                                               "end\n"
                                                               "tempRegexTrigger('^nestout$', [=[\n"
                                                               "  armNested()\n"
                                                               "  tempRegexTrigger('^nestsafe$', [[nestSafeFires = nestSafeFires + 1]])\n"
                                                               "  feedTriggers('nestin\\n')\n"
                                                               "]=], 1)\n"
                                                               "feedTriggers('nestout\\n')\n"
                                                               "echo('NESTFIRES=' .. nestFires .. '#\\n')\n"
                                                               "nestFires = 0\n"
                                                               "feedTriggers('nestin\\n')\n"
                                                               "feedTriggers('nestsafe\\n')\n"
                                                               "echo('NESTAFTER=' .. nestFires .. '#\\n')\n"
                                                               "echo('NESTSAFE=' .. nestSafeFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "Expected a runaway that crosses into a nested pass to be stopped");
        QVERIFY2(bufferContains(qsl("NESTFIRES=%1#").arg(TriggerUnit::scmMaxSameLineGenerations)), "Expected the runaway to cost one budget, not one per pass the lineage is spread across");
        QVERIFY2(bufferContains(qsl("NESTAFTER=0#")), "Expected no member of the stopped lineage to be left armed, wherever in the list it sat");
        QVERIFY2(bufferContains(qsl("NESTSAFE=1#")), "Expected a trigger armed by an unrelated script on the outer line to survive the nested pass's abort");
    }

    // Creations made inside a nested pass are appended to the same list the outer
    // pass is walking, so a batch armed there has to be read as one generation
    // just the same.
    void test_bulkCreationsInsideANestedPassAreNotStopped()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        const int bulkCount = TriggerUnit::scmMaxSameLineGenerations + 1;
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("crossFires = 0\n"
                                                               "tempRegexTrigger('^crossout$', [==[\n"
                                                               "  tempRegexTrigger('^crossin$', [=[\n"
                                                               "    for i = 1, %1 do\n"
                                                               "      tempRegexTrigger('^crosspay$', [[crossFires = crossFires + 1]])\n"
                                                               "    end\n"
                                                               "  ]=], 1)\n"
                                                               "  feedTriggers('crossin\\n')\n"
                                                               "]==], 1)\n"
                                                               "feedTriggers('crossout\\n')\n"
                                                               "feedTriggers('crosspay\\n')\n"
                                                               "echo('CROSSFIRES=' .. crossFires .. '#\\n')\n")
                                                                   .arg(bulkCount));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(!bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "A batch armed inside a nested pass is still one generation and must not be stopped");
        QVERIFY2(bufferContains(qsl("CROSSFIRES=%1#").arg(bulkCount)), qPrintable(qsl("Expected all %1 triggers armed inside the nested pass to survive and fire").arg(bulkCount)));
    }

    // Only root triggers carry a lineage, so a trigger sitting in a folder creates
    // on the folder's behalf. Read the child's own (always empty) lineage instead
    // and every round would start a fresh one, which never deepens and so never
    // trips - the run would only end at the per-line creation ceiling.
    void test_folderChildCreatesOnItsRootsBehalf()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("folderFires, folderCount = 0, 0\n"
                                                               "function makeFolderGen()\n"
                                                               "  folderCount = folderCount + 1\n"
                                                               "  local name = 'FGen' .. folderCount\n"
                                                               "  permGroup(name, 'trigger')\n"
                                                               "  permRegexTrigger('FChild' .. folderCount, name, {'^folderloop$'}, [[folderFires = folderFires + 1; makeFolderGen()]])\n"
                                                               "end\n"
                                                               "makeFolderGen()\n"
                                                               "feedTriggers('folderloop\\n')\n"
                                                               "echo('FOLDERFIRES=' .. folderFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "Expected a runaway driven from inside a folder to be stopped");
        QVERIFY2(bufferContains(qsl("FOLDERFIRES=%1#").arg(1 + TriggerUnit::scmMaxSameLineGenerations)),
                 "Expected the folder's lineage to deepen by one per round, so the generation budget is what ends it");
    }

    // The same for a filter chain, where the child is reached through the parent's
    // capture rather than by the root list passing data down.
    void test_filterChainChildCreatesOnItsRootsBehalf()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("filterFires, filterCount = 0, 0\n"
                                                               "function makeFilterGen()\n"
                                                               "  filterCount = filterCount + 1\n"
                                                               "  local name = 'FiltP' .. filterCount\n"
                                                               "  tempComplexRegexTrigger(name, '^(filterloop)$', '', 0, 0, 0, 1, 0, 0, 0, 0, 0, 0)\n"
                                                               "  permRegexTrigger('FiltC' .. filterCount, name, {'filterloop'}, [[filterFires = filterFires + 1; makeFilterGen()]])\n"
                                                               "end\n"
                                                               "makeFilterGen()\n"
                                                               "feedTriggers('filterloop\\n')\n"
                                                               "echo('FILTERFIRES=' .. filterFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("Trigger processing stopped to prevent a freeze")), "Expected a runaway driven from inside a filter chain to be stopped");
        QVERIFY2(bufferContains(qsl("FILTERFIRES=%1#").arg(1 + TriggerUnit::scmMaxSameLineGenerations)),
                 "Expected the filter parent's lineage to deepen by one per round, so the generation budget is what ends it");
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
        auto host = TestProfile::create(hostname, address, port);
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
