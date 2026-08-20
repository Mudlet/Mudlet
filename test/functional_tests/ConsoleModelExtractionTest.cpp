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

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>
#include <memory>

#include "PortableModeTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TConsoleModel.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lua.h>
#else
#include <lua.h>
#endif
}

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForConsoleModelExtraction();

using namespace std::chrono_literals;

// The main console's text buffer, cursor/prompt state and fg/bg colours were
// lifted out of the TConsole widget into a core TConsoleModel that Host
// co-owns, and the per-line trigger orchestration moved from
// TMainConsole::runTriggers() to Host::runTriggers() (#8681). The widget keeps
// the former members as references aliasing the model, so these tests pin down
// that the aliasing really is one object, that the pipeline runs off the model,
// and that the model stays usable once its view has been destroyed - the
// co-ownership exists precisely so the two can outlive each other.
class ConsoleModelExtractionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-ConsoleModelExtraction";
    const QString mLocalhost = "localhost";
    QString mPort;

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForConsoleModelExtraction();

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
        // Bind an ephemeral OS-assigned port so parallel test runs (e.g. across
        // git worktrees) do not collide on a shared fixed port.
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    // The view's members must be the model's fields, not copies of them: same
    // addresses, and a write through either side seen by the other.
    void test_mainConsoleMembersAliasTheModel()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        auto console = host->mpConsole;
        TConsoleModel& model = host->mainConsoleModel();

        QCOMPARE(host->sharedMainConsoleModel().get(), &model);
        QCOMPARE(&console->model(), &model);
        QCOMPARE(&console->buffer, &model.buffer);
        QCOMPARE(&console->mFgColor, &model.mFgColor);
        QCOMPARE(&console->mBgColor, &model.mBgColor);
        QCOMPARE(&console->mCurrentLine, &model.mCurrentLine);
        QCOMPARE(&console->mEngineCursor, &model.mEngineCursor);
        QCOMPARE(&console->mUserCursor, &model.mUserCursor);
        QCOMPARE(&console->mIsPromptLine, &model.mIsPromptLine);

        model.mFgColor = QColorConstants::Svg::orange;
        QCOMPARE(console->mFgColor, QColorConstants::Svg::orange);
        console->mBgColor = QColorConstants::Svg::navy;
        QCOMPARE(model.mBgColor, QColorConstants::Svg::navy);

        model.mUserCursor = QPoint(7, 11);
        QCOMPARE(console->mUserCursor, QPoint(7, 11));

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("cecho('<white>AliasedBufferWrite\\n')\n"));
        QVERIFY2(joinedBuffer().contains(qsl("AliasedBufferWrite")), "Text echoed through the view must be visible in the model's buffer.");
    }

    // Only the main console shares Host's model; a user window gets its own, so
    // its buffer must be a different object.
    void test_subConsoleOwnsItsOwnModel()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("openUserWindow('modelWindow')\n"));
        auto* subConsole = host->mpConsole->mSubConsoleMap.value(qsl("modelWindow"));
        QVERIFY2(subConsole, "The user window was not created.");

        QVERIFY2(&subConsole->model() != &host->mainConsoleModel(), "A user window must not share the main console's model.");
        QVERIFY2(&subConsole->buffer != &host->mainConsoleModel().buffer, "A user window must not share the main console's buffer.");
    }

    // Host::runTriggers() is the relocated per-line orchestration. Feeding a
    // line has to reach it through the buffer, and calling it directly - with
    // no view involved - has to drive the same model state and fire triggers
    // again.
    void test_hostRunTriggersDrivesThePipeline()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");
        host->mEchoLuaErrors = true;

        // 'line' is the Lua global Host::runTriggers() sets for every line, so
        // recording it proves the orchestration ran, not just the trigger.
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("modelHits = {}\n"
                                                               "tempRegexTrigger('^ModelPipeline', [[table.insert(modelHits, line)]], 10)\n"
                                                               "feedTriggers('ModelPipeline alpha\\n')\n"));

        TConsoleModel& model = host->mainConsoleModel();
        const int fedLine = model.mEngineCursor;
        QVERIFY2(fedLine >= 0, "Feeding a line must have left the model's engine cursor on it.");
        QCOMPARE(model.mCurrentLine, qsl("ModelPipeline alpha"));
        QCOMPARE(model.buffer.line(fedLine), qsl("ModelPipeline alpha"));
        QVERIFY2(!model.mIsPromptLine, "runTriggers() must clear the prompt flag once the line is processed.");
        QCOMPARE(host->mpConsole->mCurrentLine, model.mCurrentLine);
        QCOMPARE(host->mpConsole->mEngineCursor, fedLine);

        host->runTriggers(fedLine);
        QCOMPARE(model.mEngineCursor, fedLine);
        QCOMPARE(model.mUserCursor, QPoint(0, fedLine));

        host->getLuaInterpreter()->compileAndExecuteScript(qsl("echo('MODELHITS=' .. table.concat(modelHits, '|') .. '#\\n')\n"));
        QVERIFY2(joinedBuffer().contains(qsl("MODELHITS=ModelPipeline alpha|ModelPipeline alpha#")),
                 "Driving Host::runTriggers() straight off the model must fire the trigger with the same line the fed one did.");
    }

    // Host co-owns the model, so closing the profile destroys the console
    // widget while the model - and its buffer - live on. Buffer work done in
    // that window used to reach through the buffer's back-pointer to the view
    // that has just gone (TBuffer::log() and TBuffer::shrinkBuffer()) and
    // crashed on the null pointer.
    void test_modelStaysUsableAfterItsViewIsDestroyed()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        // Co-own the model the way Host does, so the test still holds it even
        // if Host were to let go of its own reference.
        std::shared_ptr<TConsoleModel> model = host->sharedMainConsoleModel();

        // The floor for a buffer limit is 100 lines, so a couple of hundred
        // appends are needed to reach the shrink.
        model->buffer.setBufferSize(100, 10);

        destroyTheView(host);
        QCOMPARE(&host->mainConsoleModel(), model.get());

        QString lastAppended;
        for (int i = 0; i < 200; ++i) {
            lastAppended = qsl("view-less line %1").arg(i);
            // Only a line feed starts a new buffer line, and the shrink is what
            // has to be reached here:
            const QString text = lastAppended + QChar::LineFeed;
            model->buffer.append(text, 0, text.size(), QColorConstants::LightGray, QColorConstants::Black, TChar::None, 0);
        }

        QVERIFY2(model->buffer.size() <= 110, "The view-less buffer was never shrunk back to its limit.");

        bool firstAppendedFound = false;
        bool lastAppendedFound = false;
        for (int i = 0; i <= model->buffer.getLastLineNumber(); ++i) {
            const QString& bufferLine = model->buffer.line(i);
            firstAppendedFound = firstAppendedFound || bufferLine.contains(qsl("view-less line 0"));
            lastAppendedFound = lastAppendedFound || bufferLine.contains(lastAppended);
        }
        QVERIFY2(!firstAppendedFound, "shrinkBuffer() never ran on the view-less buffer - its earliest lines are still there.");
        QVERIFY2(lastAppendedFound, "The last line appended to the view-less buffer is not in it.");
    }

    // The whole point of the split: the telnet -> trigger pipeline must run off
    // the model with no widget anywhere in it. Feed a line into the view-less
    // buffer, drive Host::runTriggers() over it and the trigger has to fire.
    void test_triggersRunOffTheModelWithNoView()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        // The trigger's body only writes a Lua global: echoing would need the
        // very view this test destroys. 'line' is the global Host::runTriggers()
        // sets for every line, so recording it proves the orchestration ran.
        runLua(host,
               qsl("viewlessTriggerHit = 'none'\n"
                   "tempRegexTrigger('^ViewlessPipeline', [[viewlessTriggerHit = line]], 10)\n"));

        std::shared_ptr<TConsoleModel> model = host->sharedMainConsoleModel();
        destroyTheView(host);
        // Closing the profile emergency-stops the trigger engine
        // (Host::closeChildren()), which a profile that simply never had a view
        // would not do:
        host->reenableAllTriggers();

        const int fedLine = appendModelLine(model->buffer, qsl("ViewlessPipeline gamma"));
        QCOMPARE(model->buffer.line(fedLine), qsl("ViewlessPipeline gamma"));

        host->runTriggers(fedLine);

        QCOMPARE(luaGlobalString(host, "viewlessTriggerHit"), qsl("ViewlessPipeline gamma"));
        QCOMPARE(model->mCurrentLine, qsl("ViewlessPipeline gamma"));
        QCOMPARE(model->mEngineCursor, fedLine);
        QVERIFY2(!model->mIsPromptLine, "runTriggers() must clear the prompt flag once the line is processed.");
    }

    // sysBufferShrinkEvent tells scripts their stored line indexes just shifted.
    // With a view attached it has to carry that console's name and the batch
    // size that went away.
    void test_bufferShrinkEventWithAView()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        registerShrinkHandler(host);

        TConsoleModel& model = host->mainConsoleModel();
        model.buffer.setBufferSize(100, 10);
        runLua(host, qsl("for i = 1, 200 do echo('shrink line ' .. i .. '\\n') end\n"));

        QVERIFY2(luaGlobalNumber(host, "shrinkCount") >= 1, "Echoing past the buffer limit raised no sysBufferShrinkEvent.");
        QCOMPARE(luaGlobalString(host, "shrinkReport"), qsl("main:10"));
    }

    // The model outlives the view, and scripts keeping their own line-index
    // bookkeeping still need to hear that the indexes moved. The console name
    // the event carries lives on the view, but a view-less model can only be
    // the main console's, which is always named "main".
    void test_bufferShrinkEventWithNoView()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        registerShrinkHandler(host);

        std::shared_ptr<TConsoleModel> model = host->sharedMainConsoleModel();
        model->buffer.setBufferSize(100, 10);

        destroyTheView(host);
        // Event handlers are emergency-stopped by the close as well:
        host->reenableAllTriggers();

        for (int i = 0; i < 200; ++i) {
            appendModelLine(model->buffer, qsl("view-less shrink line %1").arg(i));
        }

        QVERIFY2(luaGlobalNumber(host, "shrinkCount") >= 1, "The view-less buffer shrank without raising sysBufferShrinkEvent.");
        QCOMPARE(luaGlobalString(host, "shrinkReport"), qsl("main:10"));
    }

    // The OSC 8 documentation examples are injected into the main console's
    // buffer by the trigger phrase, which is swallowed rather than displayed.
    // None of that needs a view, and it must not reach for one.
    void test_osc8DocsInjectionWithNoView()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        std::shared_ptr<TConsoleModel> model = host->sharedMainConsoleModel();
        destroyTheView(host);

        const QString phrase = qsl("!osc8-docs");
        model->buffer.appendLine(phrase, 0, phrase.size(), QColorConstants::LightGray, QColorConstants::Black, TChar::None, 0);

        const QString bufferText = joinedBuffer(model->buffer);
        QVERIFY2(!bufferText.contains(phrase), "The OSC 8 trigger phrase was displayed instead of being swallowed.");
        QVERIFY2(bufferText.contains(qsl("OSC 8 Hyperlink Examples")), "The OSC 8 documentation examples were not injected into the view-less buffer.");
    }

    // Both logging entry points write to a log file that belongs to the view,
    // so with none attached they have to do nothing rather than reach for it.
    // logRemainingOutput() still has to drop the deferred line it would have
    // written: that state is the buffer's, and leaving it behind would let a
    // later view replay a line from a finished session.
    void test_loggingCallsWithNoView()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        std::shared_ptr<TConsoleModel> model = host->sharedMainConsoleModel();
        destroyTheView(host);

        model->buffer.appendLog(qsl("view-less appendLog text\n"));

        model->buffer.lastTextToLog = qsl("still pending when the view went away\n");
        model->buffer.lastLoggedFromLine = 3;
        model->buffer.lastloggedToLine = 4;
        model->buffer.logRemainingOutput();

        QVERIFY2(model->buffer.lastTextToLog.isEmpty(), "logRemainingOutput() left its pending line behind for a later view to replay.");
        QCOMPARE(model->buffer.lastLoggedFromLine, -1);
        QCOMPARE(model->buffer.lastloggedToLine, -1);
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

private:
    // Utility function to manually start a profile like a user would do via the
    // GUI
    void startProfile()
    {
        const QString hostname = mHostname;
        const QString address = mLocalhost;
        const QString port = mPort;
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5s)) {
            QFAIL("Profile took too long to load.");
        }
        if (!mudlet::self()->getActiveHost()) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mudlet::self()->getActiveHost()->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2s)) {
            QFAIL("Could not connect with the host.");
        }
    }

    // Utility function joining every physical buffer line and normalising
    // whitespace, so a needle the console word-wraps across lines is still found.
    QString joinedBuffer(TBuffer& buffer)
    {
        QString allText;
        for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
            allText.append(buffer.line(i)).append(QChar::Space);
        }
        return allText.simplified();
    }

    QString joinedBuffer() { return joinedBuffer(mudlet::self()->getActiveHost()->mainConsoleModel().buffer); }

    // Utility function destroying the main console widget while Host - and the
    // model it co-owns - live on, which is the window every view-less test here
    // needs.
    void destroyTheView(Host* host)
    {
        QPointer<TMainConsole> console = host->mpConsole;
        // Forcing the close stops TMainConsole::closeEvent() asking whether the
        // profile should be saved, which would block on a modal dialog here.
        host->forceClose();
        QVERIFY2(host->requestClose(), "Closing the profile was refused.");
        QTest::qWait(500ms); // the console carries WA_DeleteOnClose

        QVERIFY2(console.isNull(), "The main console view was not destroyed by closing the profile.");
        QVERIFY2(host->mpConsole.isNull(), "The host still points at a main console.");
    }

    void runLua(Host* host, const QString& code) { QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(code), qPrintable(qsl("Lua snippet failed to run: %1").arg(code))); }

    // Utility function recording every sysBufferShrinkEvent the profile sees
    void registerShrinkHandler(Host* host)
    {
        runLua(host,
               qsl("shrinkReport = 'none'\n"
                   "shrinkCount = 0\n"
                   "function onModelBufferShrink(_, consoleName, batchSize)\n"
                   "  shrinkReport = tostring(consoleName) .. ':' .. tostring(batchSize)\n"
                   "  shrinkCount = shrinkCount + 1\n"
                   "end\n"
                   "registerAnonymousEventHandler('sysBufferShrinkEvent', 'onModelBufferShrink')\n"));
    }

    // Utility function appending one whole line - only a line feed starts a new
    // buffer line - and handing back the index it landed on.
    int appendModelLine(TBuffer& buffer, const QString& text)
    {
        const QString line = text + QChar::LineFeed;
        buffer.append(line, 0, line.size(), QColorConstants::LightGray, QColorConstants::Black, TChar::None, 0);
        return buffer.getLastLineNumber() - 1;
    }

    QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    int luaGlobalNumber(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const int value = lua_isnumber(L, -1) ? static_cast<int>(lua_tonumber(L, -1)) : -1;
        lua_pop(L, 1);
        return value;
    }

    // Utility function
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

void initializeQRCResourcesForConsoleModelExtraction()
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

#include "ConsoleModelExtractionTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConsoleModelExtractionTest)
