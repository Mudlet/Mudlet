/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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

#include <atomic>
#include <functional>
#include <thread>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
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

// A wrap width that cannot hold a single glyph - because it is zero, because
// the glyph is wider than the width, or because the indentation uses the width
// up - made TBuffer::getWrapInfo() break the line at the character it was
// already sitting on, so the scan never advanced and Mudlet hung (#9622).
// Everything here therefore runs under a watchdog: a regression is an endless
// loop on the main thread, so no assertion after it would ever be reached.
class NarrowWindowWrapTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-NarrowWrap";
    QString mPort; // assigned the stub's actual loopback port in init()
    const QString mLocalhost = "localhost";
    const QString mMiniConsole = "wrapTest";
    // U+6F22 U+5B57 - East Asian Wide, so two columns are needed per glyph
    const QString mWideText = QString(QChar(0x6F22)) + QChar(0x5B57);

private slots:
    void initTestCase() { initializeQRCResources(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        // Port 0 asks the OS for an ephemeral port so parallel test runs do
        // not collide on a hardcoded one
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), "TelnetServerStub failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    // The report from #9622, at the level the Lua API no longer allows:
    // TConsole::setWrapAt() is still reachable from C++, so the wrapping itself
    // has to cope with a width of zero instead of spinning forever.
    void test_zeroWrapWidthDoesNotHang()
    {
        startProfile();
        auto* console = createTestMiniConsole();
        console->setWrapAt(0);

        runWithWatchdog("echo at a wrap width of zero", [this]() {
            runLua(qsl("echo('%1', 'abc def\\n')").arg(mMiniConsole));
        });

        // no width can hold a character, so every character ends up on a line of
        // its own - but not one of them may be dropped or duplicated
        QCOMPARE(joinedText(console), qsl("abcdef"));
    }

    // A width of one column with two-column glyphs is the same dead end, and
    // unlike a width of zero it is a perfectly ordinary thing to ask for.
    void test_wrapWidthNarrowerThanTheGlyphDoesNotHang()
    {
        startProfile();
        auto* console = createTestMiniConsole();
        runLua(qsl("setWindowWrap('%1', 1)").arg(mMiniConsole));

        runWithWatchdog("echo of a wide glyph at a wrap width of one", [this]() {
            runLua(qsl("echo('%1', '%2\\n')").arg(mMiniConsole, mWideText));
        });

        QCOMPARE(joinedText(console), mWideText);
    }

    // Indentation is subtracted from the wrap width, so a legal width and a
    // legal indent together can still leave less room than one glyph needs.
    void test_indentEatingTheWrapWidthDoesNotHang()
    {
        startProfile();
        auto* console = createTestMiniConsole();
        runLua(qsl("setWindowWrap('%1', 5)").arg(mMiniConsole));
        // both, so that whichever of the two a line uses leaves a single column
        runLua(qsl("setWindowWrapIndent('%1', 4)").arg(mMiniConsole));
        runLua(qsl("setWindowWrapHangingIndent('%1', 4)").arg(mMiniConsole));

        runWithWatchdog("echo of a wide glyph with the indent using up the wrap width", [this]() {
            runLua(qsl("echo('%1', '%2\\n')").arg(mMiniConsole, mWideText));
        });

        QCOMPARE(joinedText(console), mWideText);
    }

    // Nothing can be shown in a window that is zero columns wide, so the Lua
    // API turns such a width away rather than let it reach the wrapping.
    void test_setWindowWrapRejectsWidthsBelowOne()
    {
        startProfile();
        auto* console = createTestMiniConsole();
        runLua(qsl("setWindowWrap('%1', 40)").arg(mMiniConsole));

        // under the watchdog as well: were the width to be accepted, the echo
        // reporting the result would be the thing that hangs
        runWithWatchdog("setWindowWrap() with a width of zero", [this]() {
            runLua(qsl("local ok, err = setWindowWrap('%1', 0) echo('%1', 'RESULT:'..tostring(ok)..':'..tostring(err))").arg(mMiniConsole));
        });

        QCOMPARE(joinedText(console), qsl("RESULT:nil:wrapAtmustbegreaterthanzero,got0"));
        // the rejected call must not have changed the width either
        QCOMPARE(console->getWrapAt(), 40);
    }

    void cleanup()
    {
        const QString profilePath = mudlet::getMudletPath(enums::profileHomePath, mHostname);

        // Tear down Mudlet (and with it the live cTelnet connection) before the
        // stub server it is talking to, so the socket is closed from the client
        // side rather than being yanked out from under an active connection.
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }

private:
    // Runs work on the main thread with a hard deadline: should the wrapping
    // regress into an endless loop, kill the test process with a useful message
    // rather than leave the whole ctest run to sit until its own timeout.
    void runWithWatchdog(const char* what, const std::function<void()>& work, int timeoutSeconds = 10)
    {
        std::atomic_bool finished{false};
        std::thread watchdog([&finished, what, timeoutSeconds]() {
            for (int i = 0; i < timeoutSeconds * 10 && !finished.load(); ++i) {
                QThread::msleep(100);
            }
            if (!finished.load()) {
                qFatal("%s did not finish within %d seconds - the wrapping is stuck in a loop", what, timeoutSeconds);
            }
        });
        work();
        finished.store(true);
        watchdog.join();
    }

    void runLua(const QString& script)
    {
        auto host = mudlet::self()->getActiveHost();
        host->getLuaInterpreter()->compileAndExecuteScript(script);
    }

    // a miniconsole keeps the assertions free of the main console's connection
    // messages, and wraps its text through exactly the same code
    TConsole* createTestMiniConsole()
    {
        runLua(qsl("createMiniConsole('%1', 0, 0, 300, 300)").arg(mMiniConsole));
        auto* console = mudlet::self()->getActiveHost()->mpConsole->mSubConsoleMap.value(mMiniConsole);
        Q_ASSERT_X(console, "createTestMiniConsole", "the miniconsole the tests write into was not created");
        return console;
    }

    void startProfile()
    {
        QTimer::singleShot(0, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mPort);
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

        QSignalSpy connectedSpy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connectedSpy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    // everything in the console, with the whitespace that wrapping introduces
    // at the break points removed, so wrapped and unwrapped text compare equal
    static QString joinedText(TConsole* console)
    {
        QString text;
        for (int i = 0, total = console->buffer.getLastLineNumber(); i <= total; ++i) {
            text.append(console->buffer.line(i));
        }
        return text.remove(QChar::Space).remove(QChar::LineFeed);
    }

    void deleteProfileDirectory(const QString& profileName) { deleteDirectory(mudlet::getMudletPath(enums::profileHomePath, profileName)); }

    void deleteDirectory(const QString& path)
    {
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
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

#include "NarrowWindowWrapTest.moc"
QTEST_MAIN(NarrowWindowWrapTest)
