/***************************************************************************
 *   Copyright (C) 2026 by the Mudlet project                              *
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

// Validates the guard that stops a self-feeding trigger (one whose action calls
// feedTriggers() with text that re-matches it) from recursing the C++ stack into
// an EXCEPTION_STACK_OVERFLOW crash - see Sentry event fbda193d. The guard must
// abort the loop with a catchable Lua error while leaving legitimate feedTriggers()
// use untouched.
class TFeedTriggersRecursionTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-FeedTriggersRecursion";
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

    // A trigger that feeds itself must be stopped at the depth limit with a
    // catchable Lua error, not crash the process via stack overflow.
    void test_selfFeedingTriggerIsStopped()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("loopCount = 0\n"
                                                               "loopTriggerId = tempRegexTrigger('^loopme$', [[loopCount = loopCount + 1; feedTriggers('loopme\\n')]])\n"
                                                               "feedTriggers('loopme\\n')\n"));

        // The whole recursion runs synchronously inside the call above; if the
        // guard works we are back here (no crash) with everything unwound.
        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(bufferContains(qsl("stuck in an endless loop")), "Expected the feedTriggers loop-abort error in the console buffer");

        // The abort message must name the offending trigger (a temp trigger's name
        // is its id), proving the name-tracking guard actually identifies the culprit
        // rather than silently falling back to the unnamed branch.
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "loopTriggerId");
        const int loopTriggerId = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        QVERIFY2(bufferContains(qsl("trigger '%1'").arg(loopTriggerId)), "Expected the abort message to name the offending trigger by its id");

        // The trigger should have fired exactly up to the limit and no further.
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("echo('LOOPCOUNT='..loopCount..'\\n')"));
        QVERIFY2(bufferContains(qsl("LOOPCOUNT=%1").arg(TriggerUnit::scmMaxProcessingDepth)), qPrintable(qsl("Expected the trigger to fire exactly %1 times").arg(TriggerUnit::scmMaxProcessingDepth)));
    }

    // A single, non-self-matching feedTriggers() must still work normally and not
    // be flagged as a loop.
    void test_normalFeedTriggersIsUnaffected()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        host->mEchoLuaErrors = true;

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("normalCount = 0\n"
                                                               "tempRegexTrigger('^hello$', [[normalCount = normalCount + 1]])\n"
                                                               "feedTriggers('hello\\n')\n"
                                                               "echo('NORMALCOUNT='..normalCount..'\\n')\n"));

        QCOMPARE(host->getTriggerUnit()->processingDepth(), 0);
        QVERIFY2(!bufferContains(qsl("stuck in an endless loop")), "A normal feedTriggers() call must not be treated as a loop");
        QVERIFY2(bufferContains(qsl("NORMALCOUNT=1")), "Expected the non-looping trigger to fire exactly once");
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

    // Joins every physical buffer line and normalises whitespace before matching,
    // so a long needle that the console word-wraps (with added indents) across
    // lines is still found - a per-line scan would miss it depending on where the
    // wrap lands, which shifts with the trigger id width and console geometry.
    bool bufferContains(const QString& needle)
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        QString allText;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            allText.append(console->buffer.line(i)).append(QChar::Space);
        }
        return allText.simplified().contains(needle);
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

#include "TFeedTriggersRecursionTest.moc"
QTEST_MAIN(TFeedTriggersRecursionTest)
