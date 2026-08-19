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

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <atomic>
#include <functional>
#include <thread>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

// A wrap width that cannot hold a single glyph - because it is zero, because
// the glyph is wider than the width, or because the indentation uses the width
// up - made TBuffer::getWrapInfo() break the line at the character it was
// already sitting on, so the scan never advanced and Mudlet hung (#9622).
// Every step that can reach the wrapping therefore runs under a watchdog: a
// regression is an endless loop on the main thread, so no assertion after it
// would ever be reached.
class NarrowWindowWrapTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-NarrowWrap";
    QString mPort; // assigned the stub's actual loopback port in init()
    const QString mLocalhost = "localhost";
    const QString mMiniConsole = "wrapTest";
    // U+6F22 U+5B57 - East Asian Wide, so two columns are needed per glyph
    const QString mWideText = QString(QChar(0x6F22)) + QChar(0x5B57);

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

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
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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
        QVERIFY(console);
        console->setWrapAt(0);

        runWithWatchdog("echo at a wrap width of zero", [this]() {
            runLua(qsl("echo('%1', 'abcdef\\n')").arg(mMiniConsole));
        });

        // no width can hold a character, so every character ends up on a line
        // of its own - and not one of them may be dropped or duplicated
        QCOMPARE(nonEmptyLineCount(console), 6);
        QCOMPARE(joinedText(console), qsl("abcdef"));
    }

    // Newlines inside the echoed text take their own path through the wrapping
    // scan, which has to keep its bookkeeping straight alongside the forced
    // per-glyph breaks.
    void test_embeddedNewlinesAtZeroWrapWidthDoNotHang()
    {
        startProfile();
        auto* console = createTestMiniConsole();
        QVERIFY(console);
        console->setWrapAt(0);

        runWithWatchdog("echo of embedded newlines at a wrap width of zero", [this]() {
            runLua(qsl("echo('%1', 'ab\\ncd\\n')").arg(mMiniConsole));
        });

        QCOMPARE(joinedText(console), qsl("abcd"));
    }

    // A width of one column with two-column glyphs is the same dead end, and
    // unlike a width of zero it is a perfectly ordinary thing to ask for.
    void test_wrapWidthNarrowerThanTheGlyphDoesNotHang()
    {
        startProfile();
        auto* console = createTestMiniConsole();
        QVERIFY(console);
        runLua(qsl("setWindowWrap('%1', 1)").arg(mMiniConsole));

        runWithWatchdog("echo of a wide glyph at a wrap width of one", [this]() {
            runLua(qsl("echo('%1', '%2\\n')").arg(mMiniConsole, mWideText));
        });

        QCOMPARE(nonEmptyLineCount(console), 2);
        QCOMPARE(joinedText(console), mWideText);
    }

    // Indentation is subtracted from the wrap width, so a legal width and a
    // legal indent together can still leave less room than one glyph needs.
    void test_indentEatingTheWrapWidthDoesNotHang()
    {
        startProfile();
        auto* console = createTestMiniConsole();
        QVERIFY(console);
        runLua(qsl("setWindowWrap('%1', 5)").arg(mMiniConsole));
        // both, so that whichever of the two a line uses leaves a single column
        runLua(qsl("setWindowWrapIndent('%1', 4)").arg(mMiniConsole));
        runLua(qsl("setWindowWrapHangingIndent('%1', 4)").arg(mMiniConsole));

        runWithWatchdog("echo of a wide glyph with the indent using up the wrap width", [this]() {
            runLua(qsl("echo('%1', '%2\\n')").arg(mMiniConsole, mWideText));
        });

        QCOMPARE(textIgnoringIndentation(console), mWideText);
    }

    // An indent at or beyond the wrap width is not range-checked anywhere.
    // wrapLine() drops such an indent instead of leaving no room at all, and
    // that is what keeps this case out of the trap the one above falls into.
    void test_indentWiderThanTheWrapWidthDoesNotHang()
    {
        startProfile();
        auto* console = createTestMiniConsole();
        QVERIFY(console);
        runLua(qsl("setWindowWrap('%1', 5)").arg(mMiniConsole));
        runLua(qsl("setWindowWrapIndent('%1', 10)").arg(mMiniConsole));
        runLua(qsl("setWindowWrapHangingIndent('%1', 10)").arg(mMiniConsole));

        runWithWatchdog("echo with an indent wider than the wrap width", [this]() {
            runLua(qsl("echo('%1', '%2%2\\n')").arg(mMiniConsole, mWideText));
        });

        QCOMPARE(textIgnoringIndentation(console), mWideText + mWideText);
    }

    // insertText() wraps against the screen width and the profile's own indent
    // rather than the console's, so it reaches the wrapping by a different
    // route than echo() does.
    void test_insertTextIntoTheMainConsoleDoesNotHang()
    {
        mpServer->setWelcomeMessage(qsl("HELLO\r\n"));
        startProfile();
        auto* host = mudlet::self()->getActiveHost();
        QVERIFY2(waitForMainConsoleText(qsl("HELLO")), "Welcome text never reached the buffer");

        // leave a single column free of the screen width the insert wraps at -
        // both indents, since only the first segment of a line uses the plain
        // one and every segment after it uses the hanging one
        const int indent = host->mScreenWidth - 1;
        QVERIFY2(indent > 1, "the main console reported no usable screen width");
        runLua(qsl("setWindowWrapIndent('main', %1)").arg(indent));
        runLua(qsl("setWindowWrapHangingIndent('main', %1)").arg(indent));

        // mid-line, so the insert goes through insertInLine() rather than the
        // append path the cursor at the end of the buffer would take
        const int welcomeLine = mainConsoleLineOf(qsl("HELLO"));
        QVERIFY2(welcomeLine >= 0, "the welcome line went missing from the buffer");
        QVERIFY2(host->mpConsole->moveCursor(2, welcomeLine), "could not position the user cursor mid-line");

        runWithWatchdog("insertText of a wide glyph with the indent using up the screen width", [this, host]() {
            // the newline is what makes the insert re-wrap the line it landed in
            host->mpConsole->insertText(mWideText + QChar::LineFeed + mWideText);
        });

        QVERIFY2(mainConsoleContains(mWideText), "the inserted text did not survive wrapping");
    }

    // Nothing can be shown in a window that is zero columns wide, so the Lua
    // API turns such a width away rather than let it reach the wrapping.
    void test_setWindowWrapRejectsWidthsBelowOne()
    {
        startProfile();
        auto* console = createTestMiniConsole();
        QVERIFY(console);
        // wide enough that the reported result is not itself wrapped
        runLua(qsl("setWindowWrap('%1', 200)").arg(mMiniConsole));

        // under the watchdog as well: were the width to be accepted, the echo
        // reporting the result would be the thing that hangs
        runWithWatchdog("setWindowWrap() with a width of zero", [this]() {
            runLua(qsl("local ok, err = setWindowWrap('%1', 0) echo('%1', 'RESULT:'..tostring(ok)..':'..tostring(err))").arg(mMiniConsole));
        });

        const QString result = joinedText(console);
        QVERIFY2(result.startsWith(qsl("RESULT:nil:")), qPrintable(qsl("setWindowWrap() did not refuse a wrap width of zero, it returned: %1").arg(result)));
        QVERIFY2(result.contains(qsl("greater than zero")), qPrintable(qsl("the refusal did not say why: %1").arg(result)));
        // the rejected call must not have changed the width either
        QCOMPARE(console->getWrapAt(), 200);
    }

    // An accepted width answers true, so that the usual `if not ok then` check
    // does not read every successful call as a failure.
    void test_setWindowWrapReportsSuccess()
    {
        startProfile();
        auto* console = createTestMiniConsole();
        QVERIFY(console);

        runLua(qsl("local ok = setWindowWrap('%1', 200) echo('%1', 'RESULT:'..tostring(ok))").arg(mMiniConsole));

        QCOMPARE(joinedText(console), qsl("RESULT:true"));
        QCOMPARE(console->getWrapAt(), 200);
    }

    // The main console's width is mirrored into the profile and reported to the
    // game, so a refused width must not reach either.
    void test_rejectedMainConsoleWidthLeavesTheProfileUntouched()
    {
        startProfile();
        auto* host = mudlet::self()->getActiveHost();
        runLua(qsl("setWindowWrap(80)"));
        QCOMPARE(host->mWrapAt, 80);

        runWithWatchdog("setWindowWrap() with a width of zero on the main console", [this]() {
            runLua(qsl("setWindowWrap(0)"));
        });

        QCOMPARE(host->mWrapAt, 80);
        QCOMPARE(host->mpConsole->getWrapAt(), 80);
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
        return mudlet::self()->getActiveHost()->mpConsole->mSubConsoleMap.value(mMiniConsole);
    }

    void startProfile()
    {
        auto host = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectedSpy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connectedSpy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    // the buffer carries empty lines of its own (one is always kept ready for
    // the next text), so only the lines with something in them are counted
    static int nonEmptyLineCount(TConsole* console)
    {
        int count = 0;
        for (int i = 0, total = console->buffer.getLastLineNumber(); i <= total; ++i) {
            if (!console->buffer.line(i).isEmpty()) {
                ++count;
            }
        }
        return count;
    }

    // every line of the console joined back together - the wrapping only breaks
    // lines, so this has to come back out exactly as it went in
    static QString joinedText(TConsole* console)
    {
        QString text;
        for (int i = 0, total = console->buffer.getLastLineNumber(); i <= total; ++i) {
            text.append(console->buffer.line(i));
        }
        return text;
    }

    // as joinedText(), but with every space dropped, for the cases where the
    // wrapping pads lines out with indentation. Spaces in the text itself are
    // lost along with it, so these cases echo text that has none.
    static QString textIgnoringIndentation(TConsole* console) { return joinedText(console).remove(QChar::Space); }

    static int mainConsoleLineOf(const QString& text)
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        for (int i = 0, total = console->buffer.getLastLineNumber(); i <= total; ++i) {
            if (console->buffer.line(i).contains(text)) {
                return i;
            }
        }
        return -1;
    }

    static bool mainConsoleContains(const QString& text) { return mainConsoleLineOf(text) >= 0; }

    bool waitForMainConsoleText(const QString& text, int timeoutMs = 5000)
    {
        return QTest::qWaitFor(
                [this, &text]() {
                    return mainConsoleContains(text);
                },
                timeoutMs);
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

#include "NarrowWindowWrapTest.moc"
MUDLET_GROUPED_TEST_MAIN(NarrowWindowWrapTest)
