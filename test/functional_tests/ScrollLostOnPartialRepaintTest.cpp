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
        const QImage afterIncremental = pane->mScreenMap.toImage();

        pane->forceUpdate();
        pane->repaint();
        const QImage authoritative = pane->mScreenMap.toImage();

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

        const QImage afterIncremental = pane->mScreenMap.toImage();

        // What the same buffer looks like when every row is re-rendered.
        pane->forceUpdate();
        pane->repaint();
        const QImage authoritative = pane->mScreenMap.toImage();

        QVERIFY2(!afterIncremental.isNull() && !authoritative.isNull(), "no cached screen to compare");
        QCOMPARE(afterIncremental.size(), authoritative.size());
        QVERIFY2(afterIncremental == authoritative,
                 "a partial repaint that met a pending scroll drew the pre-scroll screen and then discarded the scroll, so the line that arrived "
                 "is missing from the pane until something unrelated forces a full repaint");
    }

    // Abutting cells of one background are filled as one rectangle, so each
    // cell still has to come out in its own color - both on a full redraw and
    // on a scroll that redraws only the rows it brought into view.
    void test_backgroundRunsKeepTheirColors()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host && host->mpConsole, "no main console");
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY(pane);
        auto* lua = host->getLuaInterpreter();

        lua->compileAndExecuteScript(qsl("for i = 1, 200 do echo('FILLER ' .. i .. '\\n') end\n"));
        qApp->processEvents();
        pane->forceUpdate();
        pane->repaint();
        qApp->processEvents();
        QVERIFY2(pane->mScreenHeight > 4, "the pane is too short for this test to mean anything");

        // Runs of one, two and more cells, with colors recurring after others so
        // that a fill reaching past the end of its run lands on a different one.
        const QList<std::pair<QColor, int>> runs{{QColor(255, 0, 0), 3},
                                                 {QColor(0, 0, 255), 1},
                                                 {QColor(255, 0, 0), 2},
                                                 {QColor(0, 160, 0), 4},
                                                 {QColor(0, 0, 255), 2},
                                                 {QColor(255, 0, 0), 1},
                                                 {QColor(0, 160, 0), 1},
                                                 {QColor(255, 255, 0), 5}};
        QString decho;
        QList<QColor> cellColors;
        for (const auto& [color, cells] : runs) {
            // Spaces, so that no glyph ink lands on the pixels sampled below
            decho += qsl("<255,255,255:%1,%2,%3>%4").arg(color.red()).arg(color.green()).arg(color.blue()).arg(QString(cells, QChar::Space));
            for (int i = 0; i < cells; ++i) {
                cellColors.append(color);
            }
        }
        const QString script = qsl("decho('%1<r>\\n')\n").arg(decho);
        lua->compileAndExecuteScript(script);
        lua->compileAndExecuteScript(qsl("echo('BETWEEN\\n')\n"));
        qApp->processEvents();
        pane->repaint();

        // Scrolled by a line during a partial repaint, the reused rows and the
        // freshly drawn one have to agree with a full redraw.
        lua->compileAndExecuteScript(script);
        pane->repaint(QRect(0, 0, pane->width(), pane->height() / 2));
        const QImage afterIncremental = pane->mScreenMap.toImage();
        pane->forceUpdate();
        pane->repaint();
        const QImage authoritative = pane->mScreenMap.toImage();
        QVERIFY2(!afterIncremental.isNull() && !authoritative.isNull(), "no cached screen to compare");
        QVERIFY2(afterIncremental == authoritative, "the scroll left the background runs drawn differently from a full redraw");

        const QString pattern(cellColors.size(), QChar::Space);
        QList<int> patternRows;
        for (int line = pane->imageTopLine(); line < pane->imageTopLine() + pane->mScreenHeight; ++line) {
            if (line < host->mpConsole->buffer.lineBuffer.size() && host->mpConsole->buffer.lineBuffer.at(line) == pattern) {
                patternRows.append(line - pane->imageTopLine());
            }
        }
        QCOMPARE(patternRows.size(), 2);
        const qreal dpr = authoritative.devicePixelRatio();
        for (const int row : patternRows) {
            const int y = qFloor((row + 0.5) * pane->mFontHeight * dpr);
            for (int cell = 0; cell < cellColors.size(); ++cell) {
                const int x = qFloor((cell + 0.5) * pane->mFontWidth * dpr);
                QVERIFY2(authoritative.pixelColor(x, y) == cellColors.at(cell),
                         qPrintable(qsl("row %1, cell %2 is %3 rather than %4").arg(row).arg(cell).arg(authoritative.pixelColor(x, y).name(), cellColors.at(cell).name())));
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
