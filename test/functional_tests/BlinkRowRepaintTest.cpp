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

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// The blink timer ticks up to 30 times a second while blinking text is on
// screen. A tick has to redraw the blinking glyphs at their new opacity, and
// what it redraws has to end up exactly as a whole-pane repaint would have it:
// the pane's cached screen seeds every later paint, so a blinking row the tick
// missed would freeze at a stale opacity.
class BlinkRowRepaintTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-BlinkRows";
    QString mpPort;
    const QString mpLocalhost = "localhost";

    // Paint events reaching the pane, recorded and optionally swallowed
    class PaintRecorder : public QObject
    {
    public:
        QList<QRegion> regions;
        bool swallow = false;

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override
        {
            if (event->type() == QEvent::Paint) {
                regions.append(static_cast<QPaintEvent*>(event)->region());
                return swallow;
            }
            return QObject::eventFilter(watched, event);
        }
    };

    static QString blinkingWelcome()
    {
        const QString filler = QString(100, QLatin1Char('X')) + qsl("\r\n");
        QString message;
        for (int i = 0; i < 80; ++i) {
            message.append(filler);
        }
        message.append(qsl("\x1b[5mThis one blinks\x1b[0m and this does not\r\n"));
        for (int i = 0; i < 4; ++i) {
            message.append(filler);
        }
        return message;
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    void test_blinkTickRedrawsOnlyTheBlinkingRowsAndMatchesAFullRepaint()
    {
        TTextEdit* pane = nullptr;
        int blinkingRow = -1;
        openPaneShowingBlinkingText(pane, blinkingRow);
        if (QTest::currentTestFailed()) {
            return;
        }
        const int rowHeight = pane->mFontHeight;
        const QRect blinkingRowRect(0, blinkingRow * rowHeight, pane->width(), rowHeight);
        const QRect topRowRect(0, 0, pane->width(), rowHeight);

        PaintRecorder recorder;
        pane->installEventFilter(&recorder);
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return recorder.regions.size() >= 3;
                         },
                         2000),
                 "the blink timer did not repaint the pane");
        for (const QRegion& region : std::as_const(recorder.regions)) {
            QVERIFY2(region.intersects(blinkingRowRect), "a blink tick repainted the pane without the blinking row");
            QVERIFY2(!region.intersects(topRowRect), "a blink tick repainted rows far from any blinking text");
        }
        const QRegion tickRegion = recorder.regions.constLast();

        // Let the blink phase move on without the pane drawing it, so that the
        // cached screen holds a stale opacity for the next tick to replace.
        recorder.swallow = true;
        QTest::qWait(300ms);
        recorder.swallow = false;
        pane->removeEventFilter(&recorder);

        // No event loop from here on, so the blink phase holds still.
        pane->slot_blinkStateChanged();
        pane->repaint(tickRegion);
        const QImage afterTick = pane->cachedScreen().copy();
        pane->forceUpdate();
        pane->repaint();
        const QImage afterFullRepaint = pane->cachedScreen().copy();
        QVERIFY2(afterTick == afterFullRepaint, "the rows a blink tick redrew left the cached screen different from a whole-pane repaint");
    }

    // Fewer new lines than a screenful scroll the pane through its cached-screen
    // shortcut, which keeps the timer running in case blinking text survived the
    // scroll. Once none is left on screen, the next tick has to stop it.
    void test_blinkTimerStopsOnceTheBlinkingLineScrollsAway()
    {
        TTextEdit* pane = nullptr;
        int blinkingRow = -1;
        openPaneShowingBlinkingText(pane, blinkingRow);
        if (QTest::currentTestFailed()) {
            return;
        }
        const int linesToScroll = blinkingRow + 2;
        QVERIFY2(linesToScroll < pane->getScreenHeight(), "scrolling the blinking line away takes a whole screenful, which repaints the pane in full anyway");

        TBuffer& buffer = *pane->mpBuffer;
        const int lastLineBefore = buffer.getLastLineNumber();
        QByteArray more;
        for (int i = 0; i < linesToScroll; ++i) {
            more.append(QByteArray(100, 'Y'));
            more.append("\r\n");
        }
        mpServer->sendRaw(more);
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return buffer.getLastLineNumber() >= lastLineBefore + linesToScroll;
                         },
                         5000),
                 "the extra lines never reached the buffer");

        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return !pane->mIsBlinkClientRegistered;
                         },
                         2000),
                 "the blink timer kept running after the blinking line left the screen");
    }

private:
    void openPaneShowingBlinkingText(TTextEdit*& pane, int& blinkingRow)
    {
        mpServer->setWelcomeMessage(blinkingWelcome());
        auto host = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(host, "No active host available for the test.");
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(2s), "Could not connect with the host.");

        QPointer<TMainConsole> console = host->mainConsoleView();
        QVERIFY(console);
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             for (int i = console->buffer.getLastLineNumber(); i >= 0; --i) {
                                 if (console->buffer.line(i).startsWith(qsl("This one blinks"))) {
                                     return true;
                                 }
                             }
                             return false;
                         },
                         5000),
                 "the blinking line never reached the buffer");

        host->setEnableBlinkText(true);
        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);

        pane = console->mUpperPane;
        QVERIFY(pane);
        const int screenRows = pane->getScreenHeight();
        QVERIFY2(screenRows > 12, qPrintable(qsl("the pane draws only %1 rows, too few to tell a blinking row from the rest").arg(screenRows)));
        QVERIFY2(pane->imageTopLine() > 0, "the screen is not full, so no paint can reuse the cached screen");

        pane->repaint();
        QVERIFY2(pane->mIsBlinkClientRegistered, "a paint showing blinking text did not start the blink timer");

        for (int row = 0; row < screenRows; ++row) {
            if (console->buffer.line(pane->imageTopLine() + row).startsWith(qsl("This one blinks"))) {
                blinkingRow = row;
            }
        }
        QVERIFY2(blinkingRow > 2, qPrintable(qsl("the blinking line is on row %1, not clear of the top of the screen").arg(blinkingRow)));
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

private slots:
    void cleanup()
    {
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mpHostname);
    }
};

#include "BlinkRowRepaintTest.moc"
MUDLET_GROUPED_TEST_MAIN(BlinkRowRepaintTest)
