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
#include "TConsole.h"
#include "TDebug.h"
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

// The Central Debug Console filters at the point a message is written rather
// than when it is drawn, so that switching a filter leaves what is already on
// screen alone and a filtered-out message costs nothing at all. These tests pin
// down that contract: which messages get through, that continuation fragments
// follow the message they belong to, and that pausing holds messages back
// rather than losing them.
class DebugConsoleFilterTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-DebugConsoleFilter";
    QString mPort; // assigned the stub's actual loopback port in init()
    const QString mLocalhost = "localhost";

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

    void cleanup()
    {
        TDebug::setPaused(false);
        TDebug::discardPausedMessages();
        TDebug::setTextFilter(QString(), Qt::CaseInsensitive);
        TDebug::setEnabledCategories(TDebug::csmAllCategories);
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

    // Starts a profile the way a user would via the GUI (mirrors the helper in
    // ClearWindowLogTest).
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

#include "DebugConsoleFilterTest.moc"
QTEST_MAIN(DebugConsoleFilterTest)
