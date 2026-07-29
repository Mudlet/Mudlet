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
