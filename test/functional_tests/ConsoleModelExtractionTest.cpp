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

#include <QFile>
#include <QRegularExpression>
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
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "XMLimport.h"
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

// The main console's text buffer, cursor/prompt state, fg/bg colours and log
// lifecycle were lifted out of the TConsole widget into a core TConsoleModel
// that Host co-owns, and the per-line trigger orchestration moved from
// TMainConsole::runTriggers() to Host::runTriggers() (#8681). The widget keeps
// the former members as references aliasing the model, so these tests pin down
// that the aliasing really is one object, that the pipeline runs off the model,
// and that the model stays usable once its view has been destroyed - the
// co-ownership exists precisely so the two can outlive each other, and the
// chosen system spell dictionary lives on Host for that same reason.
class ConsoleModelExtractionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-ConsoleModelExtraction";
    const QString mColourHostname = "Test-ConsoleModelColours";
    const QString mSpellHostname = "Test-ConsoleModelSpellDic";
    const QString mLocalhost = "localhost";
    QString mPort;
    const QColor mProfileFgColor{0xFF, 0x00, 0xFF};
    const QColor mProfileBgColor{0x00, 0x00, 0x80};
    // Deliberately not a locale code, so it can never be the starting dictionary
    // getSpellDic() falls back to: the seeded save is then the only place a
    // profile could have got this name from.
    const QString mProfileSpellDic = "mudlet_test_dictionary";

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
        deleteProfileDirectory(mColourHostname);
        deleteProfileDirectory(mSpellHostname);
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
        QCOMPARE(&console->mLogFile, &model.mLogFile);
        QCOMPARE(&console->mLogFileName, &model.mLogFileName);
        QCOMPARE(&console->mLogStream, &model.mLogStream);
        QCOMPARE(&console->mLogToLogFile, &model.mLogToLogFile);

        model.mFgColor = QColorConstants::Svg::orange;
        QCOMPARE(console->mFgColor, QColorConstants::Svg::orange);
        console->mBgColor = QColorConstants::Svg::navy;
        QCOMPARE(model.mBgColor, QColorConstants::Svg::navy);

        model.mUserCursor = QPoint(7, 11);
        QCOMPARE(console->mUserCursor, QPoint(7, 11));

        console->mLogFileName = qsl("aliased-log-name");
        QCOMPARE(model.mLogFileName, qsl("aliased-log-name"));
        model.mLogFileName.clear();

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

    // The log file, its stream and the on/off flag are core model state, so the
    // announcement and the log button are all a logging change still needs this
    // view for. TConsoleModel raises both through Host and TMainConsole acts on
    // them; the announcement has to carry the file that was actually opened.
    void test_loggingChangeIsAnnouncedByTheView()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        auto console = host->mpConsole;
        // Through TMainConsole::tr() rather than the English literal, so the
        // assertions hold whatever interface language the run picks up.
        const QString offerToStart = TMainConsole::tr("Start logging game output to log file.");
        const QString offerToStop = TMainConsole::tr("Stop logging game output to log file.");
        const QString startAnnouncement = TMainConsole::tr("Logging has started. Log file is %1");
        const QString stopAnnouncement = TMainConsole::tr("Logging has been stopped. Log file is %1");
        // The sentinel is what makes logging resume at the next launch
        // (Host::mLogStatus), so it has to appear and disappear with the log.
        const QString sentinel = mudlet::getMudletPath(enums::profileDataItemPath, host->getName(), qsl("autolog"));
        QVERIFY2(console->logButton->toolTip().contains(offerToStart), "The log button does not offer to start logging before one has been started.");

        // Through the toolbar button rather than toggleLogging() directly: that
        // is the path asking for the announcement, TConsole::slot_toggleLogging.
        console->logButton->click();
        QVERIFY2(host->mainConsoleModel().mLogToLogFile, "Clicking the log button did not start a log.");
        const QString logFileName = host->mainConsoleModel().mLogFileName;
        QVERIFY2(QFile::exists(sentinel), "Starting a log left no autolog sentinel, so logging would not resume next launch.");
        QVERIFY2(consoleTextContains(startAnnouncement.arg(logFileName)), "Starting a log was not announced on the console.");
        QVERIFY2(console->logButton->toolTip().contains(offerToStop), "The log button still offers to start logging while a log is running.");

        // A user window's own text must not be interleaved into the game log -
        // TBuffer::log() runs for every buffer and only the main one may write.
        runLua(host, qsl("openUserWindow('logSpy')\n"));
        auto* subConsole = console->mSubConsoleMap.value(qsl("logSpy"));
        QVERIFY2(subConsole, "The user window was not created.");
        // Two lines, because log() holds each one back until the next commits:
        // with only one the leak would still be sitting in the sub-console
        // buffer's deferred slot when logging stopped.
        appendModelLine(subConsole->buffer, qsl("user-window-only-text"));
        appendModelLine(subConsole->buffer, qsl("user-window-second-line"));
        appendModelLine(host->mainConsoleModel().buffer, qsl("main-console-logged-text"));

        console->logButton->click();
        QVERIFY2(!host->mainConsoleModel().mLogToLogFile, "Clicking the log button again did not stop the log.");
        QVERIFY2(!QFile::exists(sentinel), "Stopping a log left the autolog sentinel behind, so logging would resume unasked.");
        QVERIFY2(consoleTextContains(stopAnnouncement.arg(logFileName)), "Stopping a log was not announced on the console.");
        QVERIFY2(console->logButton->toolTip().contains(offerToStart), "The log button does not offer to start logging again once the log has stopped.");

        const QString contents = readFile(logFileName);
        QVERIFY2(!contents.isEmpty(), "The log file that was closed is not readable.");
        QVERIFY2(contents.contains(qsl("main-console-logged-text")), "The main console's line never reached the log file.");
        QVERIFY2(!contents.contains(qsl("user-window-only-text")), "A user window's own text was written into the game log.");
        QVERIFY2(!contents.contains(qsl("user-window-second-line")), "A user window's own text was written into the game log.");
        // The announcements are printed on the console on purpose either side
        // of the logging flag, so neither may end up in the file itself.
        // Whitespace-insensitive, because the console wraps a line this long
        // and the log records it wrapped - a plain contains() would miss it and
        // pass for the wrong reason.
        QVERIFY2(!logTextContains(contents, startAnnouncement.arg(logFileName)), "The start announcement was logged into the file it announced.");
        QVERIFY2(!logTextContains(contents, stopAnnouncement.arg(logFileName)), "The stop announcement was logged into the file it closed.");

        QFile::remove(logFileName);
    }

    // The point of moving the whole lifecycle rather than the stream alone: a
    // profile with no view has to be able to *start* a log, write to it and
    // stop it. Every step here runs against the model with the widget gone.
    void test_loggingRunsWithNoView()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        std::shared_ptr<TConsoleModel> model = host->sharedMainConsoleModel();
        destroyTheView(host);

        // false = no announcement, the way startLogging() calls it from Lua
        model->toggleLogging(false);
        QVERIFY2(model->mLogToLogFile, "A view-less profile could not start a log.");
        const QString logFileName = model->mLogFileName;
        QVERIFY2(!logFileName.isEmpty(), "Starting a view-less log named no file.");
        QVERIFY2(QFile::exists(logFileName), "Starting a view-less log opened no file.");

        // append() logs the line it completes, so this is the ordinary per-line
        // path rather than a direct poke at TBuffer::log().
        appendModelLine(model->buffer, qsl("view-less-logged-line"));
        model->buffer.appendLog(qsl("view-less-appended-text\n"));

        model->toggleLogging(false);
        QVERIFY2(!model->mLogToLogFile, "A view-less profile could not stop its log.");
        QVERIFY2(!model->mLogFile.isOpen(), "Stopping a view-less log left its file open.");

        const QString contents = readFile(logFileName);
        QVERIFY2(!contents.isEmpty(), "The view-less log file is not readable.");
        // The literal half of the format string, so the assertion survives a
        // translated run: "'Log session starting at 'hh:mm..." -> the quoted run.
        QVERIFY2(contents.contains(QCoreApplication::translate("TMainConsole", "'Log session starting at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'.").section(QChar('\''), 1, 1)),
                 "The view-less log has no session-start banner.");
        QVERIFY2(contents.contains(QCoreApplication::translate("TMainConsole", "'Log session ending at 'hh:mm:ss' on 'dddd', 'd' 'MMMM' 'yyyy'.").section(QChar('\''), 1, 1)),
                 "The view-less log has no session-end banner, so the session was never closed off.");
        QVERIFY2(contents.contains(qsl("view-less-appended-text")), "appendLog() never reached the view-less log file.");
        // The most recent line is held back for duplicate detection and only
        // written out as logging stops, so finding it proves the view-less stop
        // flushed as well as the view-less writes landing.
        QVERIFY2(contents.contains(qsl("view-less-logged-line")), "The line the view-less buffer logged never reached the log file.");

        // The deferred-logging state is the buffer's own, so it has to be
        // cleared whatever the log did - otherwise a later session replays the
        // last line of this one.
        model->buffer.lastTextToLog = qsl("still pending once the session ended\n");
        model->buffer.lastLoggedFromLine = 3;
        model->buffer.lastloggedToLine = 4;
        model->buffer.logRemainingOutput();
        QVERIFY2(model->buffer.lastTextToLog.isEmpty(), "logRemainingOutput() left its pending line behind for a later session to replay.");
        QCOMPARE(model->buffer.lastLoggedFromLine, -1);
        QCOMPARE(model->buffer.lastloggedToLine, -1);

        QFile::remove(logFileName);
    }

    void test_profileLoadFillsTheModelColoursWithNoView()
    {
        pinTheFixtureColoursAreNotTheDefaults();
        const QString saveFolder = mudlet::getMudletPath(enums::profileXmlFilesPath, mColourHostname);
        QVERIFY2(QDir().mkpath(saveFolder), "Could not create the seeded profile's save directory.");
        const QString savePath = qsl("%1profileColours.xml").arg(saveFolder);
        writeProfileColourSave(savePath);
        // loadProfile() reports a profile with no save at all as loaded fine, so
        // a save that never landed would read as a bare colour mismatch below:
        QVERIFY2(QFileInfo(savePath).size() > 0, "The seeded profile save is missing or empty.");

        Host* host = mudlet::self()->loadProfile(mColourHostname, false);
        QVERIFY2(host, "The seeded profile was not loaded.");
        QVERIFY2(host->mProfileLoadError.isEmpty(), qPrintable(qsl("Reading the seeded profile save failed: %1").arg(host->mProfileLoadError)));
        QVERIFY2(host->mpConsole.isNull(), "loadProfile() built a view, so this no longer tests the view-less path.");
        QCOMPARE(host->mFgColor, mProfileFgColor);
        QCOMPARE(host->mBgColor, mProfileBgColor);

        TConsoleModel& model = host->mainConsoleModel();
        QCOMPARE(model.mFgColor, mProfileFgColor);
        QCOMPARE(model.mBgColor, mProfileBgColor);

        // The buffer's own copy of the pair is what unstyled text is stamped
        // with, so the load has to have landed that too:
        std::string plainText = "ProfileColour plain\n";
        model.buffer.translateToPlainText(plainText, true);
        const int plainLine = model.buffer.getLastLineNumber() - 1;
        QVERIFY2(plainLine >= 0, "The plain text never reached the view-less buffer.");
        QCOMPARE(model.buffer.line(plainLine), qsl("ProfileColour plain"));
        QCOMPARE(model.buffer.buffer.at(plainLine).at(0).foreground(), mProfileFgColor);
    }

    // The trigger is made before the colours change on purpose - a colour
    // pattern also snapshots the colour it was built with, and one made
    // afterwards would match on that snapshot whatever the model held.
    void test_colourTriggerMatchesTheProfileColoursWithNoView()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        // matches[1] rather than line, so a match elsewhere on the line cannot
        // satisfy the assertions. The long bracket has to be levelled: plain
        // [[ ]] would end at the first ]] inside matches[1].
        runLua(host,
               qsl("colourTriggerHit = 'none'\n"
                   "tempAnsiColorTrigger(%1, %2, [==[colourTriggerHit = matches[1]]==])\n")
                       .arg(QString::number(TTrigger::scmDefault), QString::number(TTrigger::scmIgnored)));

        std::shared_ptr<TConsoleModel> model = host->sharedMainConsoleModel();
        destroyTheView(host);
        // Closing the profile emergency-stops the trigger engine, which a
        // profile that simply never had a view would not do:
        host->reenableAllTriggers();

        // Magenta is not the default yet, so this line must not match - which is
        // what proves the match below turns on the model rather than on the
        // pattern's snapshot:
        QCOMPARE(model->mFgColor, QColorConstants::LightGray);
        const int staleLine = appendModelLine(model->buffer, qsl("ProfileColour before"), mProfileFgColor, mProfileBgColor);
        host->runTriggers(staleLine);
        QCOMPARE(luaGlobalString(host, "colourTriggerHit"), qsl("none"));

        importProfileColours(host);
        QCOMPARE(model->mFgColor, mProfileFgColor);

        const int fedLine = appendModelLine(model->buffer, qsl("ProfileColour delta"), mProfileFgColor, mProfileBgColor);
        host->runTriggers(fedLine);

        QCOMPARE(luaGlobalString(host, "colourTriggerHit"), qsl("ProfileColour delta"));
    }

    // Driven both ways, so the model is shown to track the restyle rather than
    // to have been set once.
    void test_restylingTheViewKeepsTheModelColoursInStep()
    {
        pinTheFixtureColoursAreNotTheDefaults();
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");
        QCOMPARE(host->mainConsoleModel().mFgColor, QColorConstants::LightGray);
        QCOMPARE(host->mainConsoleModel().mBgColor, QColorConstants::Black);

        host->mFgColor = mProfileFgColor;
        host->mBgColor = mProfileBgColor;
        host->mpConsole->changeColors();

        QCOMPARE(host->mainConsoleModel().mFgColor, mProfileFgColor);
        QCOMPARE(host->mainConsoleModel().mBgColor, mProfileBgColor);

        host->mFgColor = QColorConstants::LightGray;
        host->mBgColor = QColorConstants::Black;
        host->mpConsole->changeColors();

        QCOMPARE(host->mainConsoleModel().mFgColor, QColorConstants::LightGray);
        QCOMPARE(host->mainConsoleModel().mBgColor, QColorConstants::Black);
    }

    // The same XML arrives as a package import into a live profile, and there
    // the model on its own is not enough - the view has to be restyled with it,
    // or text in the new foreground lands on the old background.
    void test_importingColoursIntoALiveProfileRestylesTheView()
    {
        pinTheFixtureColoursAreNotTheDefaults();
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");
        QVERIFY2(!host->mpConsole->mBgImageMode, "A background image is set, so the console is not styled by colour here.");

        const QString expectedBackground = qsl("rgba(%1,%2,%3,%4)").arg(mProfileBgColor.red()).arg(mProfileBgColor.green()).arg(mProfileBgColor.blue()).arg(mProfileBgColor.alpha());
        QVERIFY2(!host->mpConsole->mpMainDisplay->styleSheet().contains(expectedBackground), "The console already carries the imported background, so the assertion below cannot fail.");

        importProfileColours(host);

        QCOMPARE(host->mainConsoleModel().mBgColor, mProfileBgColor);
        QVERIFY2(host->mpConsole->mpMainDisplay->styleSheet().contains(expectedBackground),
                 qPrintable(qsl("The console was not restyled by the import: %1").arg(host->mpConsole->mpMainDisplay->styleSheet())));
    }

    // A profile is loaded and saved before it has a view, so the name of the
    // system spell dictionary it chose has to make the whole round trip through
    // Host - a save that reaches into the main console widget for it dereferences
    // a null pointer here.
    void test_spellDictionaryRoundTripsWithNoView()
    {
        const QString saveFolder = mudlet::getMudletPath(enums::profileXmlFilesPath, mSpellHostname);
        QVERIFY2(QDir().mkpath(saveFolder), "Could not create the seeded profile's save directory.");
        const QString savePath = qsl("%1profileSpellDic.xml").arg(saveFolder);
        writeProfileSave(savePath, qsl("      <mSpellDic>%1</mSpellDic>\n").arg(mProfileSpellDic));
        // loadProfile() reports a profile with no save at all as loaded fine, so
        // a save that never landed would read as a bare dictionary mismatch below:
        QVERIFY2(QFileInfo(savePath).size() > 0, "The seeded profile save is missing or empty.");

        Host* host = mudlet::self()->loadProfile(mSpellHostname, false);
        QVERIFY2(host, "The seeded profile was not loaded.");
        QVERIFY2(host->mProfileLoadError.isEmpty(), qPrintable(qsl("Reading the seeded profile save failed: %1").arg(host->mProfileLoadError)));
        QVERIFY2(host->mpConsole.isNull(), "loadProfile() built a view, so this no longer tests the view-less path.");
        QCOMPARE(host->getSpellDic(), mProfileSpellDic);

        const auto [xml, saveError] = savedProfileXml(host);
        QVERIFY2(!xml.isEmpty(), qPrintable(qsl("Saving the view-less profile produced nothing: %1").arg(saveError)));
        QVERIFY2(xml.contains(qsl("<mSpellDic>%1</mSpellDic>").arg(mProfileSpellDic)),
                 qPrintable(qsl("The view-less profile save did not carry the spell dictionary \"%1\" back out.").arg(mProfileSpellDic)));
    }

    // The same read the other way round: with nothing chosen the member is empty,
    // so the save has to go through getSpellDic() to keep naming a dictionary at
    // all.
    void test_spellDictionarySaveKeepsTheStartingDictionary()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");
        // Spelled out rather than read back from getSpellDic(), which is the
        // expression under test: a fallback quietly changed to some other
        // non-empty value has to redden this, not follow it. A profile made
        // moments ago in this test's own config directory has picked nothing.
#if defined(Q_OS_OPENBSD)
        const QString startingDictionary = qsl("en-GB");
#else
        const QString startingDictionary = qsl("en_US");
#endif
        QCOMPARE(host->getSpellDic(), startingDictionary);

        const auto [xml, saveError] = savedProfileXml(host);
        QVERIFY2(!xml.isEmpty(), qPrintable(qsl("Saving the profile produced nothing: %1").arg(saveError)));
        QVERIFY2(xml.contains(qsl("<mSpellDic>%1</mSpellDic>").arg(startingDictionary)),
                 qPrintable(qsl("The profile save did not carry the starting spell dictionary \"%1\".").arg(startingDictionary)));
    }

    // Saving the name is only half the wire: Host owning it is worth nothing
    // unless something carries it into the console's Hunspell handle. Both
    // directions are driven here - the handle a new view builds for itself, and
    // the reload Host::setSpellDic() has to push into a live one.
    void test_choosingADictionaryReachesTheConsole()
    {
        startProfile();
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        Hunhandle* handle = host->mpConsole->getHunspellHandle_system();
        QVERIFY2(handle, "The view built no system dictionary handle for the profile's dictionary.");

        // Hunspell_create() hands back a usable handle even when neither file
        // exists, so a non-null handle only proves the load ran. Telling a
        // reload apart from a failed load needs a dictionary that knows a word,
        // which a machine with no en_US installed cannot supply.
        if (!Hunspell_spell(handle, "the")) {
            QSKIP("no en_US dictionary is installed here, so a reload cannot be told apart from a failed load");
        }

        host->setSpellDic(mProfileSpellDic);
        Hunhandle* reloaded = host->mpConsole->getHunspellHandle_system();
        QVERIFY2(reloaded, "The reload left the profile with no system dictionary handle at all.");
        QVERIFY2(!Hunspell_spell(reloaded, "the"), "Choosing a dictionary that does not exist left the previous one loaded, so Host::setSpellDic() never reached the console.");
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        deleteProfileDirectory(mColourHostname);
        deleteProfileDirectory(mSpellHostname);
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
    int appendModelLine(TBuffer& buffer, const QString& text, const QColor& fgColor = QColorConstants::LightGray, const QColor& bgColor = QColorConstants::Black)
    {
        const QString line = text + QChar::LineFeed;
        buffer.append(line, 0, line.size(), fgColor, bgColor, TChar::None, 0);
        return buffer.getLastLineNumber() - 1;
    }

    // Utility function: a model that was never given the profile's colours holds
    // the built-in pair, so an assertion written against either of those would
    // pass whether the refresh ran or not.
    void pinTheFixtureColoursAreNotTheDefaults()
    {
        QVERIFY2(mProfileFgColor != QColorConstants::LightGray, "The foreground colour under test is the built-in default, so the assertions on it cannot fail.");
        QVERIFY2(mProfileBgColor != QColorConstants::Black, "The background colour under test is the built-in default, so the assertions on it cannot fail.");
    }

    // Utility function spelling the colour elements the way
    // XMLexport::exportHost() writes them.
    void writeProfileColourSave(const QString& filePath)
    {
        writeProfileSave(filePath,
                         qsl("      <mFgColor>%1</mFgColor>\n"
                             "      <mBgColor alpha=\"%2\">%3</mBgColor>\n")
                                 .arg(mProfileFgColor.name(), QString::number(mProfileBgColor.alpha()), mProfileBgColor.name()));
    }

    // Utility function writing the smallest profile save readHost() accepts,
    // holding nothing but the given <Host> children. Not surgical: readHost()
    // reads a missing boolean attribute as "off", so importing one into a live
    // profile also turns some three dozen of its settings off and zeroes its
    // borders.
    void writeProfileSave(const QString& filePath, const QString& hostChildren)
    {
        QFile file(filePath);
        QVERIFY2(file.open(QFile::WriteOnly | QFile::Text), qPrintable(qsl("Could not write the profile save %1.").arg(filePath)));
        QTextStream out(&file);
        out << qsl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<MudletPackage version=\"1.001\">\n"
                   "  <HostPackage>\n"
                   "    <Host>\n"
                   "%1"
                   "    </Host>\n"
                   "  </HostPackage>\n"
                   "</MudletPackage>\n")
                        .arg(hostChildren);
        out.flush();
        QVERIFY2(out.status() == QTextStream::Ok && file.error() == QFile::NoError, qPrintable(qsl("Writing the profile save %1 failed: %2").arg(filePath, file.errorString())));
    }

    // Utility function handing back either the XML the production save wrote or
    // the reason there is none. Both waits matter: saveProfile() refuses to
    // start while another save is in flight, and the one it does start only
    // finishes on a background thread.
    std::pair<QString, QString> savedProfileXml(Host* host)
    {
        QTemporaryDir saveDir;
        if (!saveDir.isValid()) {
            return {{}, qsl("could not create a directory to save the profile into")};
        }
        host->waitForProfileSave();
        auto [saved, xmlPath, saveError] = host->saveProfile(saveDir.path(), qsl("spellDictionary"));
        if (!saved) {
            return {{}, saveError};
        }
        host->waitForProfileSave();
        QFile file(xmlPath);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            return {{}, qsl("could not read %1 back: %2").arg(xmlPath, file.errorString())};
        }
        const QString xml = QString::fromUtf8(file.readAll());
        if (xml.isEmpty()) {
            return {{}, qsl("%1 was written empty").arg(xmlPath)};
        }
        return {xml, {}};
    }

    // Utility function reading a profile's Host settings into a live profile, as
    // a profile load does. importPackage() only reports XML well-formedness, so
    // the colour it was asked for is checked here rather than by each caller.
    void importProfileColours(Host* host)
    {
        QTemporaryDir importDir;
        QVERIFY2(importDir.isValid(), "Could not create a directory to write the profile save into.");
        const QString filePath = qsl("%1/profileColours.xml").arg(importDir.path());
        writeProfileColourSave(filePath);

        QFile file(filePath);
        QVERIFY2(file.open(QFile::ReadOnly | QFile::Text), "Could not read back the profile save just written.");
        XMLimport importer(host);
        const auto [success, message] = importer.importPackage(&file);
        QVERIFY2(success, qPrintable(qsl("Reading the profile save failed: %1").arg(message)));
        QCOMPARE(host->mFgColor, mProfileFgColor);
        QCOMPARE(host->mBgColor, mProfileBgColor);
    }

    // Utility function: the console word-wraps, and joinedBuffer() glues the
    // pieces back together with a space, so compare with every space removed
    // rather than betting on where a long line was broken.
    bool consoleTextContains(const QString& needle)
    {
        QString haystack = joinedBuffer();
        QString wanted = needle;
        return haystack.remove(QChar::Space).contains(wanted.remove(QChar::Space));
    }

    // Utility function: the console wraps long lines and the log records them
    // wrapped, so an assertion about a long message has to ignore where the
    // break landed.
    static bool logTextContains(const QString& contents, const QString& needle)
    {
        QString haystack = contents;
        QString wanted = needle;
        return haystack.remove(QRegularExpression(qsl("\\s"))).contains(wanted.remove(QRegularExpression(qsl("\\s"))));
    }

    QString readFile(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        return QString::fromUtf8(file.readAll());
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
