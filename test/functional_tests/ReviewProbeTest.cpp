/***************************************************************************
 *   Review probe for PR #9897 - NOT part of the PR under review.           *
 *                                                                         *
 *   Empirically exercises the scenarios the review agents flagged as      *
 *   untested, to distinguish "works but untested" from "broken":          *
 *     P1: filter trigger delivers filtered captures to a child (G1)       *
 *     P2: fireLength/mStayOpen keep-firing path (G2)                      *
 *     P3: two states completing in the same pass both fire (G3)           *
 *     P4: nested feed creates AND completes a fresh state (G4)            *
 *     P5: delta-1 expiry boundary, positive and negative control (G5)     *
 *     P6: permRegexTrigger multi-pattern route reaches the multiline      *
 *         machinery (test-header "only Lua route" comment claim)          *
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

class ReviewProbeTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-ReviewProbe";
    QString mpPort;
    const QString mpLocalhost = "localhost";

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0);
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    // P1 (G1): a filtering multiline trigger must hand the *filtered* capture
    // text (the capture group, not the whole line) to its children.
    void probe_filterDeliversCapturesToChild()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("parentFires = 0\n"
                                                               "childCaught = {}\n"
                                                               "local pcode = [=[parentFires = parentFires + 1]=]\n"
                                                               "tempComplexRegexTrigger('P1', [[^gamma (\\w+)$]], pcode, 1, 0, 0, 1, 0, 0, 0, 0, 0, 3)\n"
                                                               "tempComplexRegexTrigger('P1', [[^delta (\\w+)$]], pcode, 1, 0, 0, 1, 0, 0, 0, 0, 0, 3)\n"
                                                               "permRegexTrigger('P1child', 'P1', {[[^(\\w+)$]]}, [=[table.insert(childCaught, matches[2])]=])\n"
                                                               "feedTriggers('gamma one\\n')\n"
                                                               "feedTriggers('delta two\\n')\n"
                                                               "echo('P1=' .. parentFires .. '/' .. table.concat(childCaught, ',') .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("P1=1/one,two#")), qPrintable(qsl("got: %1").arg(grabProbe(qsl("P1=")))));
    }

    // P2 (G2): fireLength (mStayOpen -> mKeepFiring) keeps executing the
    // script on the N lines after the fire, then stops.
    void probe_fireLengthKeepFiring()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("keepFires = 0\n"
                                                               "local code = [=[keepFires = keepFires + 1]=]\n"
                                                               "tempComplexRegexTrigger('P2', [[^alpha$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 2, 3)\n"
                                                               "tempComplexRegexTrigger('P2', [[^beta$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 2, 3)\n"
                                                               "feedTriggers('alpha\\n')\n"
                                                               "feedTriggers('beta\\n')\n"
                                                               "feedTriggers('filler1\\n')\n"
                                                               "feedTriggers('filler2\\n')\n"
                                                               "feedTriggers('filler3\\n')\n"
                                                               "echo('P2=' .. keepFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("P2=3#")), qPrintable(qsl("got: %1").arg(grabProbe(qsl("P2=")))));
    }

    // P3 (G3): two states of the same trigger completing on the same line
    // must both be drained and both fire.
    void probe_multipleCompletedStatesInOnePass()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("multiFires = 0\n"
                                                               "local code = [=[multiFires = multiFires + 1]=]\n"
                                                               "tempComplexRegexTrigger('P3', [[^alpha$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)\n"
                                                               "tempComplexRegexTrigger('P3', [[^beta$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)\n"
                                                               "feedTriggers('alpha\\n')\n"
                                                               "feedTriggers('alpha\\n')\n"
                                                               "feedTriggers('beta\\n')\n"
                                                               "echo('P3=' .. multiFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("P3=2#")), qPrintable(qsl("got: %1").arg(grabProbe(qsl("P3=")))));
    }

    // P4 (G4): the firing script feeds lines that create AND complete a fresh
    // state of the same trigger; the nested pass must fire it exactly once.
    void probe_nestedFeedCompletesNewState()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("nestFires = 0\n"
                                                               "nestFed = false\n"
                                                               "local code = [=[\n"
                                                               "  nestFires = nestFires + 1\n"
                                                               "  if not nestFed then\n"
                                                               "    nestFed = true\n"
                                                               "    feedTriggers('alpha\\n')\n"
                                                               "    feedTriggers('beta\\n')\n"
                                                               "  end\n"
                                                               "]=]\n"
                                                               "tempComplexRegexTrigger('P4', [[^alpha$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)\n"
                                                               "tempComplexRegexTrigger('P4', [[^beta$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)\n"
                                                               "feedTriggers('alpha\\n')\n"
                                                               "feedTriggers('beta\\n')\n"
                                                               "echo('P4=' .. nestFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("P4=2#")), qPrintable(qsl("got: %1").arg(grabProbe(qsl("P4=")))));
    }

    // P5 (G5): delta-1 boundary in both directions - consecutive lines fire,
    // an intervening line kills the state.
    void probe_deltaOneExpiryControl()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("d1Fires = 0\n"
                                                               "local code = [=[d1Fires = d1Fires + 1]=]\n"
                                                               "tempComplexRegexTrigger('P5', [[^first$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1)\n"
                                                               "tempComplexRegexTrigger('P5', [[^second$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1)\n"
                                                               "feedTriggers('first\\n')\n"
                                                               "feedTriggers('second\\n')\n"
                                                               "echo('P5a=' .. d1Fires .. '#\\n')\n"
                                                               "feedTriggers('first\\n')\n"
                                                               "feedTriggers('padding\\n')\n"
                                                               "feedTriggers('second\\n')\n"
                                                               "echo('P5b=' .. d1Fires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("P5a=1#")), qPrintable(qsl("got: %1").arg(grabProbe(qsl("P5a=")))));
        QVERIFY2(bufferContains(qsl("P5b=1#")), qPrintable(qsl("got: %1").arg(grabProbe(qsl("P5b=")))));
    }

    // P6 (comment claim): permRegexTrigger with two patterns creates a
    // multiline trigger (delta 0), which fires when both patterns match the
    // same line - so tempComplexRegexTrigger is NOT the only Lua route into
    // the multiline machinery.
    void probe_permRegexTriggerMultilineRoute()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("permFires = 0\n"
                                                               "permRegexTrigger('P6', '', {[[^x ]], [[alpha]]}, [=[permFires = permFires + 1]=])\n"
                                                               "feedTriggers('x alpha\\n')\n"
                                                               "echo('P6=' .. permFires .. '#\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("P6=1#")), qPrintable(qsl("got: %1").arg(grabProbe(qsl("P6=")))));
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mpHostname);
        delete mudlet::self();
    }

private:
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

    // On failure, show what the probe actually printed instead of just "not found".
    QString grabProbe(const QString& prefix)
    {
        const QString all = joinedBuffer();
        const int at = all.indexOf(prefix);
        return at < 0 ? qsl("<marker absent>") : all.mid(at, 40);
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

#include "ReviewProbeTest.moc"
QTEST_MAIN(ReviewProbeTest)
