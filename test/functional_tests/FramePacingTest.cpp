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

#include <QElapsedTimer>
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
// one per 16ms instead, so several packets share a frame.
//
// Both halves of that have to hold. Batching without a trailing frame would
// leave the last packet of a burst unpainted until something else happened to
// repaint the pane, which is far worse than the cost it saves.
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

    // One line per packet, small enough that the cost is the paint and not the
    // text. The gap has to stay well under csmPaintPaceMs: at a whole window per
    // packet a perfect pacer and a broken one both draw one frame per packet, and
    // the assertion below degrades to the warning it prints for that case.
    static constexpr int csmBatchCount = 20;
    static constexpr int csmBatchGapMs = 2;

    // A window is measured from the last paint rather than off a fixed grid -
    // paintEvent() restarts the pacer's clock - so paced paints are always at least
    // csmPaintPaceMs apart and the +1 on the window count is exact whatever the
    // phase. This covers what that count cannot see: an expose or resize repaint
    // lands without the pacer gating it. Two rather than one is plain margin.
    static constexpr int csmPaintSlack = 2;

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

    void test_aFloodOfPacketsSharesFramesAndStillPaintsTheLastOne()
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

        mPaintCount = 0;
        QElapsedTimer floodTimer;
        floodTimer.start();
        for (int i = 0; i < csmBatchCount; ++i) {
            mpServer->sendRaw(qsl("line %1\r\n").arg(i).toUtf8());
            QTest::qWait(csmBatchGapMs);
        }
        const int paintsDuringFlood = mPaintCount;
        const qint64 floodMs = floodTimer.elapsed();

        QTest::qWait(200ms);
        QVERIFY2(waitForTextInBuffer(qsl("line %1").arg(csmBatchCount - 1)), "the flood never reached the buffer, so nothing was being paced");
        QVERIFY2(paintsDuringFlood > 0, "the pane painted nothing at all during the flood, so this test cannot tell pacing from a dead harness");

        // What a paced pane owes is one frame per window it spent, so that is what the
        // count has to be measured against. Measuring it against a share of the packet
        // count instead holds only while qWait() sleeps for about the gap it is asked
        // for: on a loaded runner it sleeps several times that, and the flood that spans
        // 3 windows here stretched to 10 on CI, where a perfectly paced pane drew the 10
        // frames it owed and was failed for it. An unpaced pane is still caught while the
        // flood stays inside far fewer windows than it sent packets, which is what the
        // warning below checks.
        const int windowsSpanned = static_cast<int>(floodMs / TTextEdit::csmPaintPaceMs) + 1;

        // The bound is worth asserting even in that case, so it stays outside the
        // branch - it just stops telling a paced pane from an unpaced one once the
        // runner is slow enough that windowsSpanned + csmPaintSlack reaches the
        // packet count.
        if (windowsSpanned + csmPaintSlack >= csmBatchCount) {
            qWarning() << "flood of" << csmBatchCount << "packets took" << floodMs << "ms, about a pacing window each - pacing is not being told apart from scheduler jitter this run";
        }
        QVERIFY2(paintsDuringFlood <= windowsSpanned + csmPaintSlack,
                 qPrintable(qsl("%1 packets over %2ms spanned %3 pacing windows but drew %4 frames - the pane is painting per packet rather than per pacing window")
                                    .arg(csmBatchCount)
                                    .arg(floodMs)
                                    .arg(windowsSpanned)
                                    .arg(paintsDuringFlood)));

        // Whatever arrives while a frame is still being held back has to be painted
        // when that frame lands. Reaching that case means printing inside the 16ms
        // cooldown a paint just started, so repaint() lands one synchronously and the print
        // follows without returning to the event loop - waiting for a paint instead
        // lets the window lapse, and the print then takes the immediate path and
        // never exercises deferral at all.
        mPaintCount = 0;
        pane->repaint();
        QVERIFY2(mPaintCount > 0, "repaint() delivered no paint event, so the pacer's cooldown never started and nothing below is inside a pacing window");

        QSignalSpy pacerFired(pane->mpPaintPacer, &QTimer::timeout);
        mPaintCount = 0;
        console->print(qsl("trailing line\n"));

        // The clock scheduleUpdate() itself consults, read just after the print: it
        // overshoots what that call saw by the print's tail alone, well under a
        // millisecond, and leaves the paint's own duration out of the reckoning
        // entirely. Under the bound the print certainly landed inside the cooldown
        // and the frame owed deferral; over it the pane may already have been
        // entitled to paint at once, and asserting deferral anyway would be the
        // wall-clock trap the flood half above guards against. Sound only while
        // nothing inside print() paints synchronously, which is what the isActive()
        // check below already rests on.
        const qint64 paneCooldownMs = pane->mSincePaint.elapsed();
        const bool printLandedInsideTheCooldown = paneCooldownMs < TTextEdit::csmPaintPaceMs;
        if (printLandedInsideTheCooldown) {
            QVERIFY2(pane->mpPaintPacer->isActive(), "the print inside the cooldown did not hold a frame back, so frames are not being deferred at all");
        } else {
            qWarning() << "the print landed" << paneCooldownMs << "ms after the paint began, past the" << TTextEdit::csmPaintPaceMs << "ms cooldown - deferral not exercised this run";
        }

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
        if (printLandedInsideTheCooldown) {
            QVERIFY2(pacerFired.count() == 1, "the pacer armed a frame and then never fired, so the held-back line waits on an unrelated repaint to appear");
        }
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
