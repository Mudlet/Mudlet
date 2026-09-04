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

#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// A console pane used to repaint once per socket read. Those paints run inside
// the receive loop, so a flood of small packets - a GMCP speedwalk sends one
// room description per packet - spent a whole frame between every two packets
// it could have spent reading. TTextEdit::scheduleUpdate() caps the paints at
// one per csmPaintPaceMs instead, so several packets share a frame.
//
// What this covers is the seam that does the capping: output arriving inside the
// cooldown has to arm the pacer rather than paint at once, and the frame it held
// back has to arrive on its own afterwards. Batching without that trailing frame
// would leave the last packet of a burst unpainted until something else happened
// to repaint the pane, which is far worse than the cost it saves.
//
// Counting frames across a packet flood and dividing by the pacing window was
// tried here and removed, so it is worth saying why rather than leaving the next
// reader to reinvent it. A paced pane owes one frame per window it spends, so the
// bound can only be the window count - and that count grows with runner load
// until it reaches the packet count, past which a pane painting once per packet
// clears the bound too. The flood spanned 3 windows unloaded and was seen at 10
// on CI against the 17 that makes the bound vacuous, so it was well on its way to
// asserting nothing on precisely the machines it ran on. It is not a loss: print()
// reaches the pane through showNewLines() exactly as socket data does, so a pane
// that paints per packet never arms the pacer and fails the check below - which
// needs no wall clock to decide whether it holds.
class FramePacingTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-FramePacing";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";

    QObject* mpWatchedPane = nullptr;
    int mPaintCount = 0;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == mpWatchedPane && event->type() == QEvent::Paint) {
            ++mPaintCount;
        }
        return QObject::eventFilter(watched, event);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0);
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    void test_aPrintInsideTheCooldownHoldsAFrameBackAndStillGetsPainted()
    {
        mpServer->setWelcomeMessage(qsl("hello\r\n"));
        startProfile(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(waitForTextInBuffer(qsl("hello")), "welcome text never reached the buffer");

        mudlet::self()->resize(1200, 800);
        QTest::qWait(200ms);

        auto host = mudlet::self()->getActiveHost();
        QVERIFY(host);
        auto console = host->mpConsole;
        QVERIFY(console);
        TTextEdit* pane = console->mUpperPane;
        QVERIFY(pane);

        mpWatchedPane = pane;
        pane->installEventFilter(this);

        // Whatever arrives while a frame is still being held back has to be painted
        // when that frame lands. Reaching that case means printing inside the
        // cooldown a paint just started, so repaint() lands one synchronously and the
        // print follows without returning to the event loop - waiting for a paint
        // instead lets the window lapse, and the print then takes the immediate path
        // and never exercises deferral at all. The paint also leaves the pane clean
        // for what follows: it covers the whole widget, so paintEvent() empties any
        // pending region and stops a pacer that was already armed.
        mPaintCount = 0;
        pane->repaint();
        QVERIFY2(mPaintCount > 0, "repaint() delivered no paint event, so the pacer's cooldown never started and nothing below is inside a pacing window");

        QSignalSpy pacerFired(pane->mpPaintPacer, &QTimer::timeout);
        mPaintCount = 0;

        // The pane's clock starts at paintEvent() entry, so the paint's own duration
        // is already spent by the time repaint() returns - a full-pane paint of this
        // console on a loaded runner can eat the whole window on its own and leave the
        // print outside the very cooldown it is here to test. Restarting the clock
        // immediately before the print opens the window by construction, so deferral
        // is exercised every run rather than whenever the runner happened to be quick.
        pane->mSincePaint.restart();
        console->print(qsl("trailing line\n"));

        // The same clock scheduleUpdate() just consulted, so it cannot disagree with
        // what that call saw by more than the print's own tail. print() appends one
        // line and paints nothing synchronously - which is what the isActive() check
        // below already rests on - so the window closing inside it means the process
        // lost the CPU for a whole frame mid-print, and nothing below would be
        // measuring the pacer any more.
        const qint64 paneCooldownMs = pane->mSincePaint.elapsed();
        if (paneCooldownMs >= TTextEdit::csmPaintPaceMs) {
            QSKIP(qPrintable(qsl("the print itself took %1ms, past the %2ms cooldown it had to land inside - the runner stalled mid-print, so deferral was not exercised")
                                     .arg(paneCooldownMs)
                                     .arg(TTextEdit::csmPaintPaceMs)));
        }

        QVERIFY2(pane->mpPaintPacer->isActive(), "the print inside the cooldown did not hold a frame back, so frames are not being deferred at all");

        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mPaintCount > 0;
                         },
                         2000),
                 "the pane never repainted at all in the 2s after the print, so the harness is dead rather than the pacer being at fault");

        // The wait above cannot say which repaint satisfied it - an unrelated one
        // arrives about 130ms after the print regardless - so the timer having fired
        // is what ties a frame to the pacer, and it needs no deadline to do it. It
        // still cannot see a timeout handler that fires and paints nothing; telling
        // that apart would need a deadline between the two repaints, which is the
        // wall-clock trap this test was flaky for in the first place.
        QVERIFY2(pacerFired.count() == 1, "the pacer armed a frame and then never fired, so the held-back line waits on an unrelated repaint to appear");
    }

private:
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    bool waitForTextInBuffer(const QString& text, int timeoutMs = 5000)
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        return QTest::qWaitFor(
                [&]() {
                    for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
                        if (console->buffer.line(i) == text) {
                            return true;
                        }
                    }
                    return false;
                },
                timeoutMs);
    }

    void deleteProfileDirectory(const QString& profileName) { deleteDirectory(mudlet::getMudletPath(enums::profileHomePath, profileName)); }

    void deleteDirectory(const QString& path)
    {
        QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }

private slots:
    void cleanup()
    {
        const QString profilePath = mudlet::getMudletPath(enums::profileHomePath, mpHostname);
        mpWatchedPane = nullptr;
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }
};

#include "FramePacingTest.moc"
MUDLET_GROUPED_TEST_MAIN(FramePacingTest)
