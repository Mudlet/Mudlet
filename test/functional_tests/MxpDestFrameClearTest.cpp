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

#include <QClipboard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TConsole.h"
#include "TLuaInterpreter.h"
#include "TMxpFrameManager.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// <DEST frame EOF> empties the frame's buffer, but that must go through
// TConsole::clear() rather than a bare TBuffer::clear(): a selection made on
// the frame beforehand would otherwise keep pointing at the deleted lines,
// and getSelectedText()'s batch-delete adjustment then drives its line index
// negative, sending Copy far out of the line buffer. The scroll state has to
// go with the lines too, or a frame the user had scrolled up in stays frozen
// over the emptied buffer and never shows new output again.
class MxpDestFrameClearTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("MxpDestFrameClear-Test-Host");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFrameName = qsl("destFrame");

    void runLua(const QString& script) { QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(script), qPrintable(script)); }

    // through feedTriggers so the tag travels the real parse path; the
    // payloads carry double quotes, hence the Lua long bracket
    void feed(const QString& data) { runLua(qsl("feedTriggers([[%1]] .. \"\\n\")").arg(data)); }

    TConsole* frameConsole() const
    {
        const TMxpFrame* frame = mpHost->mMxpFrameManager.getFrame(mFrameName);
        return frame ? frame->console : nullptr;
    }

    // The frame, created and filled. Returns its console, or nullptr with the
    // failure already registered.
    TConsole* filledFrame()
    {
        feed(qsl("<FRAME Name=\"%1\" Align=\"right\" Width=\"30%\" Height=\"50%\">").arg(mFrameName));
        TConsole* console = frameConsole();
        if (!console || !console->mUpperPane) {
            QTest::qFail("the FRAME tag did not produce a console", __FILE__, __LINE__);
            return nullptr;
        }
        for (int i = 1; i <= 5; ++i) {
            feed(qsl("<DEST %1>frame line %2</DEST>").arg(mFrameName, QString::number(i)));
        }
        // a selection over one line only would sidestep the batch-delete
        // adjustment, so the multi-line fill is load-bearing
        if (console->buffer.lineBuffer.size() < 2 || !console->buffer.lineBuffer.at(0).contains(qsl("frame line"))) {
            QTest::qFail("the DEST redirect did not reach the frame", __FILE__, __LINE__);
            return nullptr;
        }
        return console;
    }

    // The filled frame with every line selected on the upper pane.
    TTextEdit* paneWithSelection()
    {
        TConsole* console = filledFrame();
        if (!console) {
            return nullptr;
        }
        console->mUpperPane->slot_selectAll();
        // a non-empty region is also the proof that the pane's font metrics
        // are live - highlightSelection() builds it from them - so the copy
        // path's geometry bail-outs cannot green these tests for free
        if (console->mUpperPane->mSelectedRegion.isEmpty()) {
            QTest::qFail("select-all put no selection on the frame pane", __FILE__, __LINE__);
            return nullptr;
        }
        return console->mUpperPane;
    }

    // The filled frame scrolled up into split-screen: lower pane showing,
    // upper pane frozen off tail mode.
    TConsole* scrolledUpFrame()
    {
        TConsole* console = filledFrame();
        if (!console) {
            return nullptr;
        }
        runLua(qsl("scrollTo(\"%1\", 0)").arg(mFrameName));
        // the upper pane's half of the scroll runs on a 0ms timer
        settle();
        if (console->mUpperPane->mIsTailMode || console->mLowerPane->isHidden()) {
            QTest::qFail("the scroll-up did not open split-screen on the frame", __FILE__, __LINE__);
            return nullptr;
        }
        return console;
    }

    // Proof the EOF was parsed and acted on: without it, a green assertion
    // after the feed could just as well mean the tag never made it through
    bool frameEmptied()
    {
        TConsole* console = frameConsole();
        if (!console) {
            QTest::qFail("no frame console after the EOF", __FILE__, __LINE__);
            return false;
        }
        if (console->buffer.lineBuffer.size() != 1 || !console->buffer.lineBuffer.at(0).isEmpty()) {
            QTest::qFail(qPrintable(qsl("the EOF did not empty the frame: %1 line(s), starting \"%2\"").arg(QString::number(console->buffer.lineBuffer.size()), console->buffer.lineBuffer.value(0))),
                         __FILE__,
                         __LINE__);
            return false;
        }
        return true;
    }

    void settle() { QTest::qWait(50ms); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own - sharing the developer's
        // ~/.config/mudlet means sharing a profile list. Since #9712 the
        // opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        // Sized before the profile: getMainWindowSize() ignores a shrink of
        // more than half, and under the offscreen plugin the never-sized
        // default is wide enough that coming down to 1200 is such a shrink
        mudlet::self()->resize(1200, 800);

        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
            delete mudlet::self();
            QDir(path).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        // the tags come in through feedTriggers, so the processor is forced
        // on: that is what locks secure mode, which negotiating MXP with a
        // server would not do
        runLua(qsl("setConfig('specialForceMXPProcessorOn', true)"));
        settle();
    }

    void cleanup()
    {
        runLua(qsl("setConfig('specialForceMXPProcessorOn', false)"));
        mpHost->mMxpFrameManager.resetAllFrames();
        settle();
    }

    void test_eofClearTakesTheSelectionWithTheLines()
    {
        TTextEdit* pane = paneWithSelection();
        if (!pane) {
            return;
        }

        feed(qsl("<DEST %1 EOF></DEST>").arg(mFrameName));

        if (!frameEmptied()) {
            return;
        }
        QVERIFY2(pane->mSelectedRegion.isEmpty(), "the EOF clear deleted every line but left the selection standing over them");
    }

    void test_copyAfterAnEofClearStaysInRange()
    {
        TTextEdit* pane = paneWithSelection();
        if (!pane) {
            return;
        }

        // positive control: Copy has to demonstrably work on this pane first,
        // or the post-clear check would also pass on a copy that bailed out
        // for an unrelated reason
        QApplication::clipboard()->setText(QString());
        pane->slot_copySelectionToClipboard();
        QVERIFY2(QApplication::clipboard()->text().contains(qsl("frame line 1")), "Copy did not reach the clipboard on the filled frame");

        feed(qsl("<DEST %1 EOF></DEST>").arg(mFrameName));
        if (!frameEmptied()) {
            return;
        }

        // with the selection gone Copy has nothing to do, so the sentinel has
        // to survive; before the fix this read lineBuffer far out of range
        const QString sentinel = qsl("MXPDESTCLEAR sentinel");
        QApplication::clipboard()->setText(sentinel);
        pane->slot_copySelectionToClipboard();
        QCOMPARE(QApplication::clipboard()->text(), sentinel);
    }

    void test_eofOnAScrolledUpFrameResumesFollowingOutput()
    {
        TConsole* console = scrolledUpFrame();
        if (!console) {
            return;
        }

        feed(qsl("<DEST %1 EOF></DEST>").arg(mFrameName));
        if (!frameEmptied()) {
            return;
        }
        QVERIFY2(console->mUpperPane->mIsTailMode, "the EOF clear left the frame frozen at the old scroll position");
        QVERIFY2(console->mLowerPane->isHidden(), "the EOF clear left the split-screen scrollback open");

        feed(qsl("<DEST %1>fresh line</DEST>").arg(mFrameName));
        QCOMPARE(console->mUpperPane->mCursorY, console->buffer.size());
    }

    void test_eofClearTakesALowerPaneSelectionToo()
    {
        TConsole* console = scrolledUpFrame();
        if (!console) {
            return;
        }
        console->mLowerPane->slot_selectAll();
        QVERIFY2(!console->mLowerPane->mSelectedRegion.isEmpty(), "select-all put no selection on the lower pane");

        feed(qsl("<DEST %1 EOF></DEST>").arg(mFrameName));
        if (!frameEmptied()) {
            return;
        }
        // clearSplit() only hides the lower pane - a stale selection there
        // would come back the next time the frame is scrolled
        QVERIFY2(console->mLowerPane->mSelectedRegion.isEmpty(), "the EOF clear left the lower pane's selection standing over deleted lines");
    }

    // Closing the split scrollback hides the lower pane, which resizes the
    // upper one and raises sysConsoleSizeChanged. That event must reach Lua on
    // a later event-loop turn, never synchronously inside the telnet parse -
    // a handler could feed output or clear windows while TBuffer's line state
    // is mid-flight.
    void test_eofClearDoesNotRunLuaInsideTheParse()
    {
        TConsole* console = scrolledUpFrame();
        if (!console) {
            return;
        }
        runLua(qsl("_G.probeFired = false; function probeOnSize(_, name) if name == '%1' then _G.probeFired = true end end; _G.probeId = registerAnonymousEventHandler('sysConsoleSizeChanged', "
                   "'probeOnSize')")
                       .arg(mFrameName));
        feed(qsl("<DEST %1 EOF></DEST>").arg(mFrameName));
        QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(not _G.probeFired)")), "the resize event handler ran synchronously inside the telnet parse");
        settle();
        // positive control: the handler is wired up and does fire once the
        // event loop turns, so the assert above cannot pass vacuously
        QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(_G.probeFired)")), "the resize event never reached the Lua handler at all");
        runLua(qsl("killAnonymousEventHandler(_G.probeId)"));
    }
};

#include "MxpDestFrameClearTest.moc"
MUDLET_GROUPED_TEST_MAIN(MxpDestFrameClearTest)
