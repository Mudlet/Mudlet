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

#include <chrono>
#include <memory>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TConsoleModel.h"
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
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-ConsoleModelExtraction";
    const QString mLocalhost = "localhost";
    QString mPort;

private slots:
    void initTestCase() { initializeQRCResourcesForConsoleModelExtraction(); }

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
        QCOMPARE(model.mCurrentLine, qsl("ModelPipeline alpha\n"));
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
        QPointer<TMainConsole> console = host->mpConsole;

        // The floor for a buffer limit is 100 lines, so a couple of hundred
        // appends are needed to reach the shrink.
        model->buffer.setBufferSize(100, 10);

        // Forcing the close stops TMainConsole::closeEvent() asking whether the
        // profile should be saved, which would block on a modal dialog here.
        host->forceClose();
        QVERIFY2(host->requestClose(), "Closing the profile was refused.");
        QTest::qWait(500ms); // the console carries WA_DeleteOnClose

        QVERIFY2(console.isNull(), "The main console view was not destroyed by closing the profile.");
        QVERIFY2(host->mpConsole.isNull(), "The host still points at a main console.");
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
    QString joinedBuffer()
    {
        TBuffer& buffer = mudlet::self()->getActiveHost()->mainConsoleModel().buffer;
        QString allText;
        for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
            allText.append(buffer.line(i)).append(QChar::Space);
        }
        return allText.simplified();
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
QTEST_MAIN(ConsoleModelExtractionTest)
