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

// Two engine-to-frontend wires the library split can sever. What has to fail is
// the *consumer* running: a QSignalSpy on the emitter still records the
// emission after the production connect() has been deleted, so it can only ever
// catch "stopped emitting".
//
//  - cTelnet::signal_connecting / signal_connected / signal_disconnected reach
//    mudlet::slot_telnetConnectionStateChanged, which is what refreshes the
//    per-tab connection indicator as the socket moves. Cut them and a dropped
//    profile goes on showing a connected tab. Only the main window's tab bar is
//    read here; the same slot's detached-window half is not gated.
//  - THyperlinkVisibilityManager::visibilityChanged reaches the lambda the
//    TConsole constructor wires for main consoles only, which forces both text
//    panes to redraw. Cut it and text the model has just concealed can stay on
//    screen out of the panes' cached pixmap. Only what is drawn is at stake:
//    hit-testing reads TChar::linkIndex() out of the buffer, so a stale pixmap
//    cannot leave a dead link clickable.
//
// Both are read through a second observer attached to the same signal after the
// production connect. Qt delivers to receivers in connection order, so what the
// observer sees is the state the production consumer has just left behind, with
// no event loop in between for anything else to interfere in.

#include <QPointer>
#include <QTemporaryDir>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TBuffer.h"
#include "THyperlinkVisibilityManager.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTabBar.h"
#include "TTextEdit.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// A game that accepts one connection and can drop it on demand, which is all
// either test needs of a server.
class LoopbackGameStub : public QObject
{
    Q_OBJECT

public:
    explicit LoopbackGameStub(QObject* parent = nullptr)
    : QObject(parent)
    {
        connect(&mServer, &QTcpServer::newConnection, this, &LoopbackGameStub::onNewConnection);
    }

    // Ephemeral port so parallel worktree runs never collide on a fixed one.
    bool start() { return mServer.listen(QHostAddress::LocalHost, 0); }
    quint16 serverPort() const { return mServer.serverPort(); }

    // The way a player really ends up disconnected: the game hangs up on them.
    // Answers whether there was anything to hang up on: the client's own
    // signal_connected can beat this side's newConnection into the dispatcher,
    // and a silent no-op here would surface as the profile "never noticing the
    // drop" - a severed-wire message for a harness problem.
    bool dropClient()
    {
        if (!mpClient) {
            return false;
        }
        mpClient->disconnectFromHost();
        return true;
    }

private slots:
    void onNewConnection()
    {
        mpClient = mServer.nextPendingConnection();
        if (mpClient) {
            connect(mpClient, &QTcpSocket::disconnected, mpClient, &QObject::deleteLater);
        }
    }

private:
    QTcpServer mServer;
    QPointer<QTcpSocket> mpClient;
};

// What the tab indicator read as when a given connection signal was delivered.
struct IndicatorReading
{
    QString signalName;
    TabConnectionIndicator state = TabConnectionIndicator::None;
};

class FrontendRefreshSeamTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    LoopbackGameStub* mpServer = nullptr;
    const QString mHostname = qsl("Test-FrontendRefreshSeam");
    // A literal address rather than "localhost", which can resolve to both
    // families and have Mudlet dial an IPv6 socket the stub is not listening on
    const QString mLocalhost = qsl("127.0.0.1");
    quint16 mPort = 0;

private slots:
    void initTestCase()
    {
        // Saved before the skip below, because cleanupTestCase() still runs
        // after a skipped initTestCase() and would otherwise clear a variable
        // this test never set.
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
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
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new LoopbackGameStub(qApp);
        QVERIFY2(mpServer->start(), "LoopbackGameStub failed to bind a loopback port");
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
        // QtTest does not run cleanup() when init() fails, so a bare QCOMPARE
        // here would strand the singleton - and this test is leak-checked.
        if (mudlet::getMudletPath(enums::mainPath) != qsl("%1/mudlet").arg(mConfigDir.path())) {
            delete mudlet::self();
            delete mpServer;
            mpServer = nullptr;
            QFAIL("the config root was not redirected, so this would run against the developer's own profile list");
        }
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        // A fresh config root reads the stored default, which is off, and with
        // it off every tab reads TabConnectionIndicator::None whatever the
        // socket does. Set before the profile so the tab is seeded as a user
        // with the feature on would see it.
        mudlet::self()->setShowTabConnectionIndicators(true);
        mudlet::self()->resize(1200, 800);
        deleteProfileDirectory();
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            // Mudlet writes the profile out as it shuts down, so removing the
            // directory first only has it recreated. The path has to be read
            // while the singleton is alive though - getMudletPath() reaches
            // through self() without checking it.
            const QString profileDir = mudlet::getMudletPath(enums::profileHomePath, mHostname);
            delete mudlet::self();
            QDir(profileDir).removeRecursively();
        }
    }

    // Four transitions across the three connection signals, all reaching one
    // consumer. Each reading is taken inside the emission that caused it, which
    // is what keeps the eight other refresh paths from repairing it first.
    void test_theTabIndicatorFollowsEveryConnectionStateChange()
    {
        Host* host = startProfile();
        QVERIFY(host);

        TTabBar* tabBar = mudlet::self()->mpTabBar;
        QVERIFY(tabBar);
        QVERIFY2(mudlet::self()->showTabConnectionIndicators(), "indicators are switched off, so every reading below would be None whatever the socket did");
        QVERIFY2(tabBar->tabIndex(mHostname) >= 0, "the profile got no tab of its own, so there is no indicator to assert on");

        // addConsoleForNewHost() arms a 3s single-shot refresh of every
        // indicator from the live socket state. Left pending it would land
        // between two of the emissions below and make a later reading
        // coincidentally right, so it is waited out rather than raced. Only
        // mudlet::slot_reconnect() re-arms it, and the drive below goes through
        // cTelnet directly.
        QTest::qWait(3200ms);
        QVERIFY2(tabBar->tabConnectionIndicator(mHostname) == TabConnectionIndicator::Connected,
                 qPrintable(qsl("the connected profile's tab reads %1, so the transitions below would start from the wrong state").arg(describe(tabBar->tabConnectionIndicator(mHostname)))));

        // Waiting the timer out is arithmetic; this is the proof. A refresh
        // still in flight would repair the indicator between two of the
        // emissions below and make a later reading coincidentally right, so a
        // value nothing but a refresh would overwrite is planted and checked.
        tabBar->setTabConnectionIndicator(mHostname, TabConnectionIndicator::Error);
        QTest::qWait(500ms);
        QVERIFY2(tabBar->tabConnectionIndicator(mHostname) == TabConnectionIndicator::Error, "an indicator refresh is still pending, so the readings below would not be this wire's doing");
        tabBar->setTabConnectionIndicator(mHostname, TabConnectionIndicator::Connected);

        QList<IndicatorReading> seen;
        const auto record = [this, &seen, tabBar](const QString& name) {
            seen.append(IndicatorReading{name, tabBar->tabConnectionIndicator(mHostname)});
        };
        // Declared after everything the lambdas below capture, so it is the
        // first of them destroyed and the connections are gone before their
        // referents are - including on an early return from a failed QVERIFY.
        QObject observerContext;
        // Attached after the production connects made in addConsoleForNewHost(),
        // so each of these runs with slot_telnetConnectionStateChanged() having
        // already had its turn at the same emission.
        connect(&host->mTelnet, &cTelnet::signal_connecting, &observerContext, [&record]() {
            record(qsl("connecting"));
        });
        connect(&host->mTelnet, &cTelnet::signal_connected, &observerContext, [&record]() {
            record(qsl("connected"));
        });
        connect(&host->mTelnet, &cTelnet::signal_disconnected, &observerContext, [&record]() {
            record(qsl("disconnected"));
        });

        host->mTelnet.disconnectIt();
        QVERIFY2(waitForReading(seen, qsl("disconnected"), 1), "the profile never noticed the disconnect it was asked for");

        host->mTelnet.reconnect();
        QVERIFY2(waitForReading(seen, qsl("connecting"), 1), "the profile never started connecting again");
        QVERIFY2(waitForReading(seen, qsl("connected"), 1), "the profile never reconnected to the stub");

        QVERIFY2(mpServer->dropClient(), "the stub had no client to hang up on, so the drop below was never made");
        QVERIFY2(waitForReading(seen, qsl("disconnected"), 2), "the profile never noticed the game dropping the connection");

        assertReading(seen, qsl("disconnected"), 1, TabConnectionIndicator::Disconnected);
        assertReading(seen, qsl("connecting"), 1, TabConnectionIndicator::Connecting);
        assertReading(seen, qsl("connected"), 1, TabConnectionIndicator::Connected);
        assertReading(seen, qsl("disconnected"), 2, TabConnectionIndicator::Disconnected);
    }

    // The repaint half of the OSC 8 visibility seam. TBufferOSC_spec.lua drives
    // the same delayed reveal but reads back only the line text, which
    // performReveal() restores before it emits, so the whole spec suite stays
    // green with this wire cut.
    void test_aHyperlinkVisibilityChangeForcesBothPanesToRedraw()
    {
        Host* host = startProfile();
        QVERIFY(host);
        host->mEnableOSC8Hyperlinks = true;

        TMainConsole* console = host->mpConsole;
        QVERIFY(console);
        QVERIFY(console->mUpperPane);
        QVERIFY(console->mLowerPane);

        // Of the three visibility actions, a delayed reveal is the only one that
        // completes without a click. The delay runs from the moment the link is
        // registered inside feedTriggers(), and the observer below goes on a
        // few statements later, so three seconds is budget for a loaded
        // sanitiser runner rather than a figure the reveal needs.
        //
        // Real escape bytes inside a Lua long-bracket string, rather than Lua's
        // own "\027" escapes inside a C++ raw string literal: moc stops parsing
        // a file at a raw string literal it cannot lex and silently emits no
        // meta-object for whatever follows, which links as an undefined vtable.
        const QString esc = QString(QChar(0x1B));
        const QString stringTerminator = esc + QLatin1Char('\\');
        const QString link = qsl("%1]8;;send:osc8seam?config={\"visibility\":{\"action\":\"reveal\",\"delay\":3000}}%2HIDDENWORD%1]8;;%2").arg(esc, stringTerminator);
        const QString feed = qsl("feedTriggers([==[OSCSEAM1(%1)OSCSEAM1\n]==])").arg(link);
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(feed), "feedTriggers() did not run, so no hyperlink was ever registered");

        int lineNumber = -1;
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             lineNumber = lineHolding(console, qsl("OSCSEAM1"));
                             return lineNumber >= 0;
                         },
                         8000),
                 "the line carrying the link never reached the buffer");
        // Concealment keeps the character count identical so buffer indices stay
        // valid, which is why the text is replaced space for space.
        QCOMPARE(console->buffer.lineBuffer.at(lineNumber), qsl("OSCSEAM1(          )OSCSEAM1"));

        // performReveal() calls update() on both panes itself, so counting
        // paints cannot tell the wire apart from its absence. forceUpdate()
        // additionally sets mForceUpdate, which is what stops the next paint
        // short-cutting to the cached screen pixmap instead of re-rendering the
        console->mUpperPane->mForceUpdate = false;
        console->mLowerPane->mForceUpdate = false;

        bool observed = false;
        bool upperForced = false;
        bool lowerForced = false;
        QObject observerContext;
        connect(&console->getHyperlinkVisibilityManager(), &THyperlinkVisibilityManager::visibilityChanged, &observerContext, [&]() {
            if (observed) {
                return;
            }
            observed = true;
            upperForced = console->mUpperPane->mForceUpdate;
            lowerForced = console->mLowerPane->mForceUpdate;
        });

        // Half a dozen other places force a pane to redraw - a command line
        // taking focus, a font change, the scroll-stopped timer - and any of
        // them landing in this window would leave the readings below holding
        // with the wire cut. Rather than enumerate them, the flags go back to
        // false on every turn of the loop, so what the observer reads can only
        // have been set inside the emission it is reading.
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             if (observed) {
                                 return true;
                             }
                             console->mUpperPane->mForceUpdate = false;
                             console->mLowerPane->mForceUpdate = false;
                             return false;
                         },
                         12000),
                 "the hyperlink never changed visibility, so nothing here was exercised");
        QCOMPARE(console->buffer.lineBuffer.at(lineNumber), qsl("OSCSEAM1(HIDDENWORD)OSCSEAM1"));
        QVERIFY2(upperForced, "the upper pane was not forced to redraw for a hyperlink that had just been revealed");
        QVERIFY2(lowerForced, "the lower pane was not forced to redraw for a hyperlink that had just been revealed");
    }

private:
    static QString describe(const TabConnectionIndicator state)
    {
        switch (state) {
        case TabConnectionIndicator::None:
            return qsl("None");
        case TabConnectionIndicator::Connected:
            return qsl("Connected");
        case TabConnectionIndicator::Connecting:
            return qsl("Connecting");
        case TabConnectionIndicator::Disconnected:
            return qsl("Disconnected");
        case TabConnectionIndicator::Error:
            return qsl("Error");
        }
        return qsl("unknown");
    }

    // The reading taken when the nth (1-based) emission of 'name' was delivered.
    static int readingIndex(const QList<IndicatorReading>& seen, const QString& name, const int occurrence)
    {
        int found = 0;
        for (int i = 0; i < seen.size(); ++i) {
            if (seen.at(i).signalName == name && ++found == occurrence) {
                return i;
            }
        }
        return -1;
    }

    static bool waitForReading(const QList<IndicatorReading>& seen, const QString& name, const int occurrence)
    {
        return QTest::qWaitFor(
                [&]() {
                    return readingIndex(seen, name, occurrence) >= 0;
                },
                8000);
    }

    // A QVERIFY2 here returns from this helper rather than from the test slot,
    // so the remaining readings are still checked and a run that broke more than
    // one of the three wires reports all of them rather than only the first.
    void assertReading(const QList<IndicatorReading>& seen, const QString& name, const int occurrence, const TabConnectionIndicator expected) const
    {
        const int index = readingIndex(seen, name, occurrence);
        QVERIFY2(index >= 0, qPrintable(qsl("emission %1 of %2 was never delivered").arg(QString::number(occurrence), name)));
        const TabConnectionIndicator actual = seen.at(index).state;
        QVERIFY2(actual == expected,
                 qPrintable(qsl("the tab read %1 rather than %2 when emission %3 of %4 reached the frontend").arg(describe(actual), describe(expected), QString::number(occurrence), name)));
    }

    static int lineHolding(TMainConsole* console, const QString& needle)
    {
        const int lastLine = console->buffer.getLastLineNumber();
        for (int i = lastLine; i >= qMax(0, lastLine - 20); --i) {
            if (i < console->buffer.lineBuffer.size() && console->buffer.lineBuffer.at(i).contains(needle)) {
                return i;
            }
        }
        return -1;
    }

    // Answers rather than asserting. assertReading() above wants a QVERIFY2 that
    // returns only from the helper, so one run reports every broken wire; here
    // the opposite is needed, because everything after this call dereferences
    // the host.
    Host* startProfile()
    {
        Host* host = TestProfile::create(mHostname, mLocalhost, QString::number(mPort));
        if (!host) {
            qWarning("no active host available for the test");
            return nullptr;
        }
        QSignalSpy connected(&host->mTelnet, &cTelnet::signal_connected);
        if (host->mTelnet.getConnectionState() != QAbstractSocket::ConnectedState && !connected.wait(8000)) {
            qWarning("could not connect to the stub");
            return nullptr;
        }
        return host;
    }

    void deleteProfileDirectory()
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, mHostname));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "FrontendRefreshSeamTest.moc"
MUDLET_GROUPED_TEST_MAIN(FrontendRefreshSeamTest)
