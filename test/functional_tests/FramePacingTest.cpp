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
    // text, and spaced far enough apart that an unpaced pane paints per packet.
    static constexpr int csmBatchCount = 20;
    static constexpr int csmBatchGapMs = 2;

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
        for (int i = 0; i < csmBatchCount; ++i) {
            mpServer->sendRaw(qsl("line %1\r\n").arg(i).toUtf8());
            QTest::qWait(csmBatchGapMs);
        }
        const int paintsDuringFlood = mPaintCount;

        QTest::qWait(200ms);
        QVERIFY2(waitForTextInBuffer(qsl("line %1").arg(csmBatchCount - 1)), "the flood never reached the buffer, so nothing was being paced");
        QVERIFY2(paintsDuringFlood > 0, "the pane painted nothing at all during the flood, so this test cannot tell pacing from a dead harness");
        QVERIFY2(paintsDuringFlood < csmBatchCount / 2,
                 qPrintable(qsl("%1 packets drew %2 frames - the pane is still painting per packet rather than per pacing window").arg(csmBatchCount).arg(paintsDuringFlood)));

        // Whatever arrives while a frame is still being held back has to be
        // painted when that frame lands. Landing a paint first is what puts the
        // second print inside the pacing window, which is the case that defers.
        mPaintCount = 0;
        console->print(qsl("paced probe\n"));
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mPaintCount > 0;
                         },
                         2000),
                 "output never repainted the pane, so a deferred frame cannot be told apart from a lost one");
        const int paintsBeforeTheDeferredLine = mPaintCount;

        console->print(qsl("trailing line\n"));
        QTest::qWait(200ms);
        QVERIFY2(mPaintCount > paintsBeforeTheDeferredLine, "the line printed just after a paint was never drawn - pacing dropped the trailing frame instead of holding it for one window");
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
