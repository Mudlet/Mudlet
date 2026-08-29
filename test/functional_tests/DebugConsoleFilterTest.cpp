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

#include "GroupedTest.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

// The Central Debug Console filters at the point a message is written rather
// than when it is drawn, so that switching a filter leaves what is already on
// screen alone and a filtered-out message costs nothing at all. These tests pin
// down that contract: which messages get through, that continuation fragments
// follow the message they belong to, and that pausing holds messages back
// rather than losing them. The console's own controls are covered here too,
// since bringing one up needs the same running profile.
class DebugConsoleFilterTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-DebugConsoleFilter";
    QString mPort; // assigned the stub's actual loopback port in init()
    const QString mLocalhost = "localhost";

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        // pre-create $XDG_CONFIG_HOME/mudlet/profiles so setupConfig() adopts it
        // and the test never touches the real profiles or settings
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

    // Only the enabled categories reach the console; the rest are dropped
    // before they are ever appended.
    void test_disabledCategoryNeverReachesTheConsole()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories({TDebug::Category::TriggerMatch});

        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch) << "a trigger matched\n" >> host;
        TDebug(Qt::darkGreen, Qt::black, TDebug::Category::GameLine) << "a line from the game\n" >> host;

        QVERIFY2(debugBufferContains(qsl("a trigger matched")), "Enabled category did not reach the debug console");
        QVERIFY2(!debugBufferContains(qsl("a line from the game")), "Disabled category leaked into the debug console");
    }

    // Turning a category back on must not retroactively reveal what was
    // filtered out, nor disturb what is already on screen.
    void test_reenablingACategoryDoesNotRewriteHistory()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories({TDebug::Category::TriggerMatch});
        TDebug(Qt::darkGreen, Qt::black, TDebug::Category::GameLine) << "hidden while off\n" >> host;

        TDebug::setEnabledCategories({TDebug::Category::TriggerMatch, TDebug::Category::GameLine});
        TDebug(Qt::darkGreen, Qt::black, TDebug::Category::GameLine) << "shown once on\n" >> host;

        QVERIFY2(!debugBufferContains(qsl("hidden while off")), "Enabling a category brought back a message that was filtered out when it arrived");
        QVERIFY2(debugBufferContains(qsl("shown once on")), "Message did not appear after its category was enabled");
    }

    // The text filter matches on the whole composed message.
    void test_textFilterKeepsOnlyMatchingMessages()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setTextFilter(qsl("goblin"), Qt::CaseInsensitive);

        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch) << "a Goblin appears\n" >> host;
        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch) << "a merchant appears\n" >> host;

        QVERIFY2(debugBufferContains(qsl("a Goblin appears")), "Message matching the text filter was dropped");
        QVERIFY2(!debugBufferContains(qsl("a merchant appears")), "Message not matching the text filter got through");
    }

    void test_textFilterHonoursCaseSensitivity()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setTextFilter(qsl("Goblin"), Qt::CaseSensitive);

        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch) << "a Goblin appears\n" >> host;
        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch) << "a goblin appears\n" >> host;

        QVERIFY2(debugBufferContains(qsl("a Goblin appears")), "Case-sensitive text filter dropped an exact match");
        QVERIFY2(!debugBufferContains(qsl("a goblin appears")), "Case-sensitive text filter let a differently-cased message through");
    }

    // With a category of their own to hide behind, protocol events print the
    // data they arrived with instead of asking for a display() call to see it.
    void test_protocolEventPrintsItsPayload()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);

        QString key = qsl("Char.Vitals");
        host->getLuaInterpreter()->setGMCPTable(key, qsl("{\"hp\":\"100\",\"maxhp\":\"100\"}"));

        const QString buffer = joinedDebugBuffer();
        QVERIFY2(buffer.contains(qsl("gmcp event <gmcp.Char.Vitals> {\"hp\":\"100\",\"maxhp\":\"100\"}")), "The GMCP frame was not printed with the event it arrived on");
        QVERIFY2(!buffer.contains(qsl("display(gmcp)")), "A payload that fitted was elided anyway");
    }

    // One event is raised per level of the key, all carrying the same frame -
    // printing it on each would triple every message for a two-level key.
    void test_protocolPayloadIsNotRepeatedForEveryKeyLevel()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);

        QString key = qsl("Char.Vitals");
        host->getLuaInterpreter()->setGMCPTable(key, qsl("{\"hp\":\"100\"}"));

        const QString buffer = joinedDebugBuffer();
        QVERIFY2(buffer.contains(qsl("gmcp event <gmcp.Char>")), "The event raised for the parent key was not printed");
        QCOMPARE(buffer.count(qsl("\"hp\":\"100\"")), 1);
    }

    // A single frame can run to tens of kilobytes, which would bury everything
    // around it - so past the cap the rest is left to display().
    void test_longProtocolPayloadIsElided()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        // Wide enough that what is printed lands on one buffer line, so this
        // tests the eliding rather than the console's wrapping:
        mudlet::smpDebugConsole->setWrapAt(4000);

        // 1211 characters in total, of which the first 1000 are inlined:
        const QString filler = QString(1200, QLatin1Char('x'));
        QString key = qsl("Room.Info");
        host->getLuaInterpreter()->setGMCPTable(key, qsl("{\"desc\":\"%1\"}").arg(filler));

        const QString buffer = joinedDebugBuffer();
        QVERIFY2(buffer.contains(qsl("gmcp event <gmcp.Room.Info> {\"desc\":\"xxx")), "The start of an over-long payload was not printed");
        QVERIFY2(buffer.contains(qsl("... (211 more characters, display(gmcp) to see all)")), "An elided payload did not say how much of it was left out");
        QVERIFY2(!buffer.contains(QString(1000, QLatin1Char('x'))), "The whole of an over-long payload reached the console");
    }

    // Several call sites emit a header and then a csmContinue fragment as two
    // separate TDebug objects. The fragment has to share the header's fate, or
    // the console is left with a bare "<some captured text>" hanging on its own.
    void test_continuationFollowsItsHeader()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setTextFilter(qsl("capture group"), Qt::CaseInsensitive);

        TDebug(Qt::darkCyan, Qt::black, TDebug::Category::TriggerDetail) << "capture group #1 = " >> host;
        TDebug(Qt::darkMagenta, Qt::black, TDebug::Category::TriggerDetail) << TDebug::csmContinue << "<the captured text>\n" >> host;

        QVERIFY2(debugBufferContains(qsl("capture group #1")), "Header matching the text filter was dropped");
        QVERIFY2(debugBufferContains(qsl("<the captured text>")), "Continuation fragment was orphaned by the text filter");
    }

    void test_continuationIsDroppedWithItsHeader()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setTextFilter(qsl("something else entirely"), Qt::CaseInsensitive);

        TDebug(Qt::darkCyan, Qt::black, TDebug::Category::TriggerDetail) << "capture group #1 = " >> host;
        TDebug(Qt::darkMagenta, Qt::black, TDebug::Category::TriggerDetail) << TDebug::csmContinue << "<the captured text>\n" >> host;

        QVERIFY2(!debugBufferContains(qsl("<the captured text>")), "Continuation fragment survived although its header was filtered out");
    }

    // The text worth searching for usually sits in the fragment, not the head -
    // the game line in "new line arrived:", the trigger name in "ERROR:". A
    // fragment that matches has to bring its head back with it, or filtering on
    // the obvious thing hides the message it belongs to.
    void test_matchingFragmentBringsBackItsHeader()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setTextFilter(qsl("wandering merchant"), Qt::CaseInsensitive);

        TDebug(Qt::darkGreen, Qt::black, TDebug::Category::GameLine) << "new line arrived:" >> host;
        TDebug(Qt::lightGray, Qt::black, TDebug::Category::GameLine) << TDebug::csmContinue << "You see a wandering merchant here.\n" >> host;

        const QString buffer = joinedDebugBuffer();
        QVERIFY2(buffer.contains(qsl("You see a wandering merchant here.")), "Fragment matching the text filter was dropped");
        QVERIFY2(buffer.contains(qsl("new line arrived:")), "Fragment matched the text filter but its header was left behind");
        QVERIFY2(buffer.indexOf(qsl("new line arrived:")) < buffer.indexOf(qsl("wandering merchant")), "Header was printed after the fragment it introduces");
    }

    // A category the head fails is a real "no" - unlike the text filter, no
    // fragment can talk its way past it.
    void test_matchingFragmentCannotDefeatACategoryFilter()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories({TDebug::Category::TriggerMatch});
        TDebug::setTextFilter(qsl("wandering merchant"), Qt::CaseInsensitive);

        TDebug(Qt::darkGreen, Qt::black, TDebug::Category::GameLine) << "new line arrived:" >> host;
        TDebug(Qt::lightGray, Qt::black, TDebug::Category::GameLine) << TDebug::csmContinue << "You see a wandering merchant here.\n" >> host;

        QVERIFY2(!debugBufferContains(qsl("wandering merchant")), "A fragment matching the text filter got past a disabled category");
    }

    // Past the cap the OLDEST held messages are discarded and counted, so that
    // resuming can say how much of the history is missing. Asserted on the
    // queue rather than the console: replaying a full queue is by definition
    // enough to overflow the console's own line limit, so what survives in the
    // buffer afterwards says more about TBuffer::shrinkBuffer() than about the
    // cap being tested here.
    void test_pausedQueueDropsTheOldestPastItsCap()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setPaused(true);

        // Two more than the queue can hold, so exactly the first two go:
        const int limit = TDebug::pausedMessageLimit();
        for (int i = 0; i < limit + 2; ++i) {
            TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch) << qsl("held message %1\n").arg(i) >> host;
        }

        QCOMPARE(TDebug::pausedMessageCount(), limit);
        QCOMPARE(TDebug::pausedDroppedCount(), 2);

        TDebug::setPaused(false);

        QCOMPARE(TDebug::pausedMessageCount(), 0);
        QVERIFY2(!TDebug::pausedDroppedCount(), "The dropped count survived the replay that was supposed to report it");
        // The newest is at the tail of the replay, so it outlives any trimming:
        QVERIFY2(debugBufferContains(qsl("held message %1").arg(limit + 1)), "The newest held message did not survive the replay");
    }

    // A message held back keeps the time it arrived, so a replayed burst does
    // not look as though it all happened the moment the user hit Resume.
    void test_replayedMessageKeepsItsArrivalTime()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setPaused(true);
        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch) << "timed message\n" >> host;
        const QString arrivalTime = QTime::currentTime().toString(mudlet::smTimeStampFormat);

        QTest::qWait(1200);
        TDebug::setPaused(false);

        auto* console = mudlet::smpDebugConsole.data();
        int lineOfMessage = -1;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            if (console->buffer.line(i).contains(qsl("timed message"))) {
                lineOfMessage = i;
                break;
            }
        }
        QVERIFY2(lineOfMessage >= 0, "Held-back message never arrived on resume");
        // Same second, rather than the same millisecond, to stay clear of the
        // wait's own jitter:
        QCOMPARE(console->buffer.timeBuffer.at(lineOfMessage).left(8), arrivalTime.left(8));
    }

    // Narrowing to one trigger is the point of the item filter, so anything
    // belonging to a different one has to go.
    void test_itemFilterKeepsOnlyThatItem()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setItemFilter(qsl("Combat trigger"));

        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch, qsl("Combat trigger")) << "the one being watched\n" >> host;
        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch, qsl("Healing trigger")) << "a different trigger\n" >> host;
        TDebug(Qt::darkGreen, Qt::black, TDebug::Category::GameLine) << "a message about no item at all\n" >> host;

        QVERIFY2(debugBufferContains(qsl("the one being watched")), "The item being filtered for was dropped");
        QVERIFY2(!debugBufferContains(qsl("a different trigger")), "Another item's message got past the item filter");
        QVERIFY2(!debugBufferContains(qsl("a message about no item at all")), "A message belonging to no item got past the item filter");
    }

    // The completer offers names case-insensitively, so the filter has to match
    // that way too or picking what was offered silences the console.
    void test_itemFilterIgnoresCase()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setItemFilter(qsl("combat trigger"));

        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch, qsl("Combat Trigger")) << "differently cased\n" >> host;

        QVERIFY2(debugBufferContains(qsl("differently cased")), "The item filter matched case-sensitively");
    }

    // ...but a console that shows literally nothing while a profile starts and
    // stops is indistinguishable from a broken one.
    void test_itemFilterStillShowsSystemMessages()
    {
        startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setItemFilter(qsl("Combat trigger"));

        TDebug(Qt::blue, Qt::white, TDebug::Category::System) << "a profile came or went\n" >> nullptr;

        QVERIFY2(debugBufferContains(qsl("a profile came or went")), "The item filter silenced system messages too");
    }

    void test_clearingTheItemFilterRestoresEverything()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setItemFilter(qsl("Combat trigger"));
        TDebug::setItemFilter(QString());

        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch, qsl("Healing trigger")) << "back again\n" >> host;

        QVERIFY2(debugBufferContains(qsl("back again")), "Clearing the item filter did not restore other items' messages");
    }

    // Closing a profile must not leave its pointer behind in the filter state -
    // a later profile allocated at the same address would start life silenced.
    void test_closingAProfileForgetsItsFilterSetting()
    {
        auto* host = startDebuggingProfile();

        TDebug::setHostEnabled(host, false);
        QVERIFY2(!TDebug::hostEnabled(host), "Profile was not muted to begin with");

        // Mirrors what Host's destructor does - it has no pointer to pass:
        TDebug::removeHost(nullptr, host->getName());

        QVERIFY2(TDebug::hostEnabled(host), "A closed profile's muted setting outlived it");
    }

    // Filtering by profile keeps another profile's chatter out without
    // silencing messages that belong to no profile at all.
    void test_disabledProfileIsSilencedButSystemMessagesAreNot()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setHostEnabled(host, false);

        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch) << "from the muted profile\n" >> host;
        TDebug(Qt::blue, Qt::white, TDebug::Category::System) << "not from any profile\n" >> nullptr;

        QVERIFY2(!debugBufferContains(qsl("from the muted profile")), "Message from a disabled profile reached the console");
        QVERIFY2(debugBufferContains(qsl("not from any profile")), "System message was silenced by a profile filter");

        TDebug::setHostEnabled(host, true);
        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch) << "from the profile again\n" >> host;
        QVERIFY2(debugBufferContains(qsl("from the profile again")), "Re-enabling a profile did not restore its messages");
    }

    // Pausing holds messages back rather than throwing them away, and resuming
    // replays them in the order they arrived.
    void test_pauseHoldsMessagesAndResumeReplaysThemInOrder()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setPaused(true);

        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch) << "first while paused\n" >> host;
        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch) << "second while paused\n" >> host;

        QCOMPARE(TDebug::pausedMessageCount(), 2);
        QVERIFY2(!debugBufferContains(qsl("first while paused")), "A message arriving while paused was shown anyway");

        TDebug::setPaused(false);

        const QString buffer = joinedDebugBuffer();
        QVERIFY2(buffer.contains(qsl("first while paused")), "A message held back while paused was lost on resume");
        QVERIFY2(buffer.contains(qsl("second while paused")), "A message held back while paused was lost on resume");
        QVERIFY2(buffer.indexOf(qsl("first while paused")) < buffer.indexOf(qsl("second while paused")), "Held-back messages were replayed out of order");
        QCOMPARE(TDebug::pausedMessageCount(), 0);
    }

    // A filtered-out message must not even be held back, so that pausing with
    // the noisy categories off stays cheap.
    void test_pauseDoesNotHoldFilteredOutMessages()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories({TDebug::Category::TriggerMatch});
        TDebug::setPaused(true);

        TDebug(Qt::darkGreen, Qt::black, TDebug::Category::GameLine) << "filtered out while paused\n" >> host;

        QCOMPARE(TDebug::pausedMessageCount(), 0);
        TDebug::setPaused(false);
        QVERIFY2(!debugBufferContains(qsl("filtered out while paused")), "A filtered-out message appeared after resuming");
    }

    // Clearing the console while paused must also drop what is being held, or
    // resuming would immediately refill the console the user just emptied.
    void test_discardingPausedMessagesEmptiesTheQueue()
    {
        auto* host = startDebuggingProfile();

        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        TDebug::setPaused(true);
        TDebug(Qt::blue, Qt::black, TDebug::Category::TriggerMatch) << "about to be discarded\n" >> host;
        QCOMPARE(TDebug::pausedMessageCount(), 1);

        TDebug::discardPausedMessages();
        QCOMPARE(TDebug::pausedMessageCount(), 0);

        TDebug::setPaused(false);
        QVERIFY2(!debugBufferContains(qsl("about to be discarded")), "A discarded message was replayed on resume");
    }

    // The find bar floats over the console rather than sitting in a layout, so
    // nothing but the console itself keeps it in the corner and inside the
    // window.
    void test_findBarFloatsInTheBottomRightCorner()
    {
        startDebuggingProfile();

        auto* console = mudlet::smpDebugConsole.data();
        console->resize(800, 600);
        console->showSearchBar();

        auto* findBar = console->mpFindBar.data();
        QVERIFY2(findBar, "The Central Debug Console has no find bar");
        QVERIFY2(!findBar->isHidden(), "Asking for the find bar left it hidden");
        QVERIFY2(findBar->width() < console->width() / 2, "The find bar takes up the width of the console instead of only the room it needs");
        QVERIFY2(console->rect().contains(findBar->geometry()), "The find bar hangs outside the console");
        const QPoint middle = findBar->geometry().center();
        QVERIFY2(middle.x() > console->width() / 2 && middle.y() > console->height() / 2, "The find bar is not in the bottom right corner");
    }

    void test_findBarKeepsItsCornerOnResize()
    {
        startDebuggingProfile();

        auto* console = mudlet::smpDebugConsole.data();
        console->resize(800, 600);
        console->showSearchBar();

        auto* findBar = console->mpFindBar.data();
        const QRect wasAt = findBar->geometry();
        const QSize wasSized = console->size();
        console->resize(1000, 700);
        // Nothing delivers a resize event to a console that is not on screen,
        // so hand it the one the window system would have:
        QResizeEvent resizeEvent(console->size(), wasSized);
        QCoreApplication::sendEvent(console, &resizeEvent);

        QVERIFY2(findBar->geometry() != wasAt, "The find bar stayed where it was when the console was resized around it");
        QVERIFY2(console->rect().contains(findBar->geometry()), "Resizing the console left the find bar outside it");
        const QPoint middle = findBar->geometry().center();
        QVERIFY2(middle.x() > console->width() / 2 && middle.y() > console->height() / 2, "The find bar did not follow the corner it hangs off");
    }

    void cleanup()
    {
        TDebug::setPaused(false);
        TDebug::discardPausedMessages();
        TDebug::setTextFilter(QString(), Qt::CaseInsensitive);
        TDebug::setItemFilter(QString());
        TDebug::setEnabledCategories(TDebug::csmAllCategories);
        // A profile muted by a test that failed part way through would
        // otherwise silence whatever runs next:
        TDebug::enableAllHosts();
        mudlet::smDebugMode = false;

        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

private:
    // Starts a profile and brings the Central Debug Console into existence, the
    // way toggling Debug in the editor does.
    Host* startDebuggingProfile()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto* host = mudlet::self()->getActiveHost();

        mudlet::self()->attachDebugArea(host->getName());
        mudlet::smDebugMode = true;
        // Drain anything the profile emitted while starting up, so each test
        // only sees what it wrote itself:
        TDebug::flushMessageQueue();
        mudlet::smpDebugConsole->clear();
        return host;
    }

    QString joinedDebugBuffer()
    {
        auto* console = mudlet::smpDebugConsole.data();
        QString allText;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            allText.append(console->buffer.line(i)).append(QChar::Space);
        }
        return allText.simplified();
    }

    bool debugBufferContains(const QString& needle) { return joinedDebugBuffer().contains(needle); }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }

    // Starts a profile by driving the connection dialog, as a user would.
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            const auto dialog = []() {
                return mudlet::self()->mpConnectionDialog.data();
            };

            mudlet::self()->startAutoLogin({});

            // slot_showConnectionDialog() defers the dialog's show() to a zero
            // timer, and a field cannot take focus before its window is up, so
            // this wait has to spin the event loop to get there. A predicate
            // already true on entry - the dialog merely existing, say - would
            // not: qWaitFor returns without processing anything in that case.
            if (!QTest::qWaitFor(
                        [&dialog]() {
                            return dialog() && dialog()->isVisible();
                        },
                        5000)) {
                qWarning() << "the connection dialog never appeared";
                return;
            }

            // Focus is what each step hands to the next, so the field about to
            // be typed into is the real precondition. Naming it also keeps a
            // missed handoff a legible warning, rather than the null-widget
            // assert QTest::keyClicks() aborts the whole process with.
            const auto waitForFocus = [](QWidget* field, const char* name) {
                if (QTest::qWaitFor(
                            [field]() {
                                return QApplication::focusWidget() == field;
                            },
                            5000)) {
                    return true;
                }
                qWarning() << "focus never reached the" << name << "field";
                return false;
            };

            QTest::mouseClick(dialog()->new_profile_button, Qt::LeftButton);
            if (!waitForFocus(dialog()->profile_name_entry, "profile name")) {
                return;
            }
            QTest::keyClicks(dialog()->profile_name_entry, hostname);
            QTest::keyClick(dialog()->profile_name_entry, Qt::Key_Tab);

            if (!waitForFocus(dialog()->host_name_entry, "server address")) {
                return;
            }
            QTest::keyClicks(dialog()->host_name_entry, address);
            QTest::keyClick(dialog()->host_name_entry, Qt::Key_Tab);

            if (!waitForFocus(dialog()->port_entry, "port")) {
                return;
            }
            QTest::keyClicks(dialog()->port_entry, port);
            QTest::keyClick(dialog()->port_entry, Qt::Key_Return);
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
};

#include "DebugConsoleFilterTest.moc"
MUDLET_GROUPED_TEST_MAIN(DebugConsoleFilterTest)
