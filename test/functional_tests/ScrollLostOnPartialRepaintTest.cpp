/***************************************************************************
 *   Copyright (C) 2026 by Jay Howard - jay.patrick.howard@gmail.com       *
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

#include <QPainter>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

/*
 * A screen cache owes one property: an incremental paint draws what a forced
 * full repaint of the same buffer would. Both cases below assert exactly that,
 * because a partial-region repaint - which is what one mouse move of a widget
 * dragged over the pane delivers - has to honour a scroll that arrived since
 * the cache was built rather than skip it.
 */
class ScrollLostOnPartialRepaintTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-ScrollLost";
    QString mpPort;
    const QString mpLocalhost = "localhost";

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

    // The fallback at the top of drawForeground() - mLastRenderedOffset still 0
    // while lineOffset has moved on - is reached once each time the view leaves
    // the top of the buffer, and it derives its scroll from y_bottom.
    void test_leavingTheTopOfTheBufferDuringAPartialRepaint()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host && host->mpConsole, "no main console");
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY(pane);
        auto* lua = host->getLuaInterpreter();

        // sized here rather than taken as found: the guard below needs the rows,
        // and a smaller default window would skip the case while ctest still
        // reported a pass
        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);
        const int screenHeight = pane->mScreenHeight;
        QVERIFY2(screenHeight >= 20, "the pane is too short to leave the top of the buffer by more than the ten-line shortcut");

        // Measured against what the buffer already holds - a profile arrives with
        // a few lines of its own, and connect-time output would otherwise push the
        // view off the top and leave nothing for this case to exercise.
        const int room = screenHeight - 2 - static_cast<int>(host->mpConsole->buffer.lineBuffer.size());
        QVERIFY2(room > 0, "the profile filled the pane before the case could");
        lua->compileAndExecuteScript(qsl("for i = 1, %1 do echo('FILLER ' .. i .. '\\n') end\n").arg(room));
        qApp->processEvents();
        pane->forceUpdate();
        pane->repaint();
        qApp->processEvents();
        QVERIFY2(pane->imageTopLine() == 0, "the view already left the top, so the fallback under test is not the one that runs");
        QCOMPARE(pane->mLastRenderedOffset, 0);

        // Enough at once to clear the ten-line shortcut. Unpatched, the fallback
        // derives its row count from the repaint region, so a burst this size came
        // in under mScreenHeight and fed the shifted blit a wrong one; the fix pins
        // y_bottom before that fallback reads it, so the count can only trip the
        // full redraw instead.
        const int burst = screenHeight / 2 + 2;
        lua->compileAndExecuteScript(qsl("for i = 1, %1 do echo('BURST ' .. i .. '\\n') end\n").arg(burst));
        QVERIFY2(pane->imageTopLine() >= 10, "the burst did not clear the ten-line shortcut");

        // A shallow region, which is what the fallback measured unpatched.
        pane->repaint(QRect(0, 0, pane->width(), 3 * pane->mFontHeight));
        const QImage afterIncremental = pane->cachedScreen().copy();

        pane->forceUpdate();
        pane->repaint();
        const QImage authoritative = pane->cachedScreen().copy();

        QVERIFY2(!afterIncremental.isNull() && !authoritative.isNull(), "no cached screen to compare");
        QCOMPARE(afterIncremental.size(), authoritative.size());
        QVERIFY2(afterIncremental == authoritative,
                 "leaving the top of the buffer on a partial repaint shifted the cached screen by a row count derived from the repaint region rather "
                 "than from the scroll, so the pane kept rows of misplaced text");
    }

    void test_aLineArrivingDuringAPartialRepaintIsNotLost()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host && host->mpConsole, "no main console");
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY(pane);
        auto* lua = host->getLuaInterpreter();

        // Enough lines that lineOffset is past the < 10 shortcut, so the cache
        // paths under test are the ones that run.
        lua->compileAndExecuteScript(qsl("for i = 1, 200 do echo('FILLER ' .. i .. '\\n') end\n"));
        qApp->processEvents();
        pane->forceUpdate();
        pane->repaint();
        qApp->processEvents();
        QVERIFY2(pane->mScreenHeight > 4, "the pane is too short for this test to mean anything");

        // A line arrives, and then a partial-region repaint reaches the pane
        // before any full one does - which is what one mouse move of a drag
        // over the pane looks like. No processEvents() in between, so the
        // scroll is still pending when that repaint runs.
        lua->compileAndExecuteScript(qsl("echo('MIDDRAG_LINE\\n')\n"));
        const QRect partial(0, 0, pane->width(), pane->height() / 2);
        QVERIFY2(partial.height() < pane->rect().height(), "the repaint has to be partial to exercise the path");
        pane->repaint(partial);

        const QImage afterIncremental = pane->cachedScreen().copy();

        // What the same buffer looks like when every row is re-rendered.
        pane->forceUpdate();
        pane->repaint();
        const QImage authoritative = pane->cachedScreen().copy();

        QVERIFY2(!afterIncremental.isNull() && !authoritative.isNull(), "no cached screen to compare");
        QCOMPARE(afterIncremental.size(), authoritative.size());
        QVERIFY2(afterIncremental == authoritative,
                 "a partial repaint that met a pending scroll drew the pre-scroll screen and then discarded the scroll, so the line that arrived "
                 "is missing from the pane until something unrelated forces a full repaint");
    }

    // The cached screen slides over a buffer twice its height and moves the rows
    // it keeps back to the far end whenever it reaches an end, so each direction
    // scrolls more than a screen's worth to cross both ends at least once.
    void test_scrollingBothWaysAcrossTheCachedScreensEnds_data()
    {
        QTest::addColumn<QString>("background");
        QTest::addColumn<bool>("opaque");
        // The cached screen is cleared to the background when the pane can paint
        // it itself, so a color other than the default black shows a row cleared
        // to the wrong one; translucent keeps the cleared-to-transparent kind
        QTest::newRow("default") << QString() << true;
        QTest::newRow("opaque") << qsl("20, 40, 90") << true;
        QTest::newRow("translucent") << qsl("20, 40, 90, 128") << false;
    }

    void test_scrollingBothWaysAcrossTheCachedScreensEnds()
    {
        QFETCH(QString, background);
        QFETCH(bool, opaque);
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host && host->mpConsole, "no main console");
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY(pane);
        auto* lua = host->getLuaInterpreter();
        if (!background.isEmpty()) {
            lua->compileAndExecuteScript(qsl("setBackgroundColor(%1)\n").arg(background));
        }

        // Each line a different length, so that a row kept from the wrong place
        // cannot pass for the right one
        lua->compileAndExecuteScript(qsl("for i = 1, 400 do echo(string.format('LINE %03d %s\\n', i, string.rep('#', i % 37))) end\n"));
        qApp->processEvents();
        // Leaving the bottom opens the split screen and resizes the pane, so do
        // that before the steps under test
        pane->scrollUp(100);
        qApp->processEvents();
        // Twice, as a change of background takes effect from the next frame
        for (int frame = 0; frame < 2; ++frame) {
            pane->forceUpdate();
            pane->repaint();
        }
        QCOMPARE(pane->testAttribute(Qt::WA_OpaquePaintEvent), opaque);
        const int screenHeight = pane->mScreenHeight;
        QVERIFY2(screenHeight > 4, "the pane is too short for this test to mean anything");
        QVERIFY2(pane->imageTopLine() > 2 * screenHeight + 10, "too little buffer above the view to scroll up through both ends");

        int step = 0;
        for (const bool up : {true, false}) {
            for (int scrolled = 0; scrolled <= 2 * screenHeight;) {
                const int lines = 1 + step++ % 3;
                up ? pane->scrollUp(lines) : pane->scrollDown(lines);
                scrolled += lines;
                pane->repaint();
                const QImage afterIncremental = pane->cachedScreen().copy();
                pane->forceUpdate();
                pane->repaint();
                const QImage authoritative = pane->cachedScreen().copy();
                QVERIFY2(!afterIncremental.isNull() && afterIncremental == authoritative,
                         qPrintable(qsl("scrolling %1 by %2 line(s), %3 lines in, left the cached screen different from a full redraw").arg(up ? qsl("up") : qsl("down")).arg(lines).arg(scrolled)));
            }
        }
    }

    // Output that arrives soon after a paint leaves the scrollbar to the paint
    // pacer, and a full repaint landing first must not take that with it
    void test_aRepaintBeforeThePacerFiresLeavesTheScrollBarCurrent()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host && host->mpConsole, "no main console");
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY(pane);
        QScrollBar* scrollBar = host->mpConsole->mpScrollBar;
        QVERIFY(scrollBar);
        auto* lua = host->getLuaInterpreter();

        lua->compileAndExecuteScript(qsl("for i = 1, 200 do echo('FILLER ' .. i .. '\\n') end\n"));
        qApp->processEvents();
        pane->repaint();
        lua->compileAndExecuteScript(qsl("echo('PACED_LINE\\n')\n"));
        QVERIFY2(pane->mpPaintPacer->isActive(), "the line arrived after the paint window closed, so the pacer this case is about never started");
        pane->forceUpdate();
        pane->repaint();

        // not QTRY: later output from the connection refreshes the scrollbar
        // within its retry window and would hide the loss
        QTest::qWait(100ms);
        QCOMPARE(scrollBar->maximum(), host->mpConsole->buffer.getLastLineNumber() + 1);
    }

    // A hover or selection repaint draws into a scratch buffer seeded only with
    // the rows it repaints, so what it shows has to match a full repaint while
    // the rest of the scratch is left as it was.
    void test_aHoverRepaintCopiesAndShowsOnlyItsOwnRows()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host && host->mpConsole, "no main console");
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY(pane);
        auto* lua = host->getLuaInterpreter();
        mudlet::self()->resize(1200, 800);
        lua->compileAndExecuteScript(qsl("for i = 1, 200 do echo(string.format('FILLER %03d %s\\n', i, string.rep('_', i % 23))) end\n"));
        QTest::qWait(100ms);
        const int rows = pane->mScreenHeight;
        const int fontHeight = pane->mFontHeight;
        QVERIFY2(rows > 12, "the pane is too short to hold a band away from both of its edges");

        // Cell-aligned at both edges of the pane, as hover and selection repaints
        // are, and once off the cell grid so the rows it names round outwards
        const QList<QRect> bands{QRect(0, 0, pane->width(), fontHeight),
                                 QRect(0, 3 * fontHeight + fontHeight / 2, pane->width(), 2 * fontHeight),
                                 QRect(0, (rows - 1) * fontHeight, pane->width(), pane->height() - (rows - 1) * fontHeight)};
        for (const QRect& band : bands) {
            pane->forceUpdate();
            QPixmap reference(pane->size());
            pane->render(&reference);
            QVERIFY2(pane->imageTopLine() > 0, "the pane must be scrolled for a band repaint to reuse the cached screen");

            const QColor stale(Qt::magenta);
            const QImage cached = pane->cachedScreen();
            const qreal dpr = cached.devicePixelRatio();
            pane->mRenderBuffer = QImage(cached.size(), cached.format());
            pane->mRenderBuffer.setDevicePixelRatio(dpr);
            pane->mRenderBuffer.fill(stale);

            QPixmap shown = reference.copy();
            {
                QPainter eraser(&shown);
                eraser.fillRect(band, stale);
            }
            QVERIFY2(!pane->mMouseTracking && pane->mDirtyFirstLine < 0, "a drag or a pending dirty line would repaint the cache itself rather than the scratch buffer");
            pane->render(&shown, band.topLeft(), QRegion(band));
            QVERIFY2(pane->mRenderBuffer.pixelColor(0, qRound((band.top() + band.height() / 2) * dpr)) != stale,
                     qPrintable(qsl("repainting rows %1 to %2 did not draw into the scratch buffer, so nothing here tests it").arg(band.top()).arg(band.bottom())));

            // Mid-cell on a text row the band cannot reach, which also keeps it
            // clear of the spare row below the last line that every paint redraws
            const int farRow = qRound(((band.top() > pane->height() / 2 ? 1 : rows - 3) + 0.5) * fontHeight * dpr);
            QVERIFY2(pane->mRenderBuffer.pixelColor(0, farRow) == stale,
                     qPrintable(qsl("repainting rows %1 to %2 copied device row %3 of the cached screen, which it cannot show").arg(band.top()).arg(band.bottom()).arg(farRow)));
            QVERIFY2(shown.toImage() == reference.toImage(),
                     qPrintable(qsl("repainting rows %1 to %2 over a stale scratch buffer showed something other than a full repaint").arg(band.top()).arg(band.bottom())));
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
        if (!spy.wait(2s)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName) { deleteDirectory(MudletApp::getMudletPath(enums::profileHomePath, profileName)); }

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
        const QString profilePath = MudletApp::getMudletPath(enums::profileHomePath, mpHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }
};

#include "ScrollLostOnPartialRepaintTest.moc"
MUDLET_GROUPED_TEST_MAIN(ScrollLostOnPartialRepaintTest)
