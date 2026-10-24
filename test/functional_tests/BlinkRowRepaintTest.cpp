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

// A blink tick redraws only the rows around blinking text, and must leave the
// cached screen, which seeds later paints, exactly as a whole-pane repaint would.
class BlinkRowRepaintTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = qsl("Test-BlinkRows");
    QString mpPort;
    const QString mpLocalhost = qsl("localhost");

    class PaintSwallower : public QObject
    {
    protected:
        bool eventFilter(QObject* watched, QEvent* event) override { return event->type() == QEvent::Paint || QObject::eventFilter(watched, event); }
    };

    static QString fillerLines(const int count, const QChar letter)
    {
        QString lines;
        for (int i = 0; i < count; ++i) {
            lines.append(QString(100, letter));
            lines.append(qsl("\r\n"));
        }
        return lines;
    }

    // Descenders, underscores and accents ink the rows around the blinking one;
    // the next line blinks only on a combining accent, which is never drawn blinking.
    static QByteArray blinkingLines()
    {
        QString lines = qsl("\x1b[5mThis one blinks gjpqy___ O\u0308 g\u0303_ \u00c5\x1b[0m and this does not\r\n");
        lines.append(qsl("Only the accent of e\x1b[5m\u0301\x1b[0m blinks\r\n"));
        lines.append(fillerLines(1, QLatin1Char('Z')));
        lines.append(qsl("\u00c5\u00d6\u00c9\u00c4 \u1e7a \u00c2\u00ca \u01fa\u01fa\u01fa accents\r\n"));
        lines.append(fillerLines(2, QLatin1Char('Z')));
        return lines.toUtf8();
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

    void test_blinkTickRedrawsOnlyTheRowsAroundBlinkingTextAndMatchesAFullRepaint()
    {
        TTextEdit* pane = nullptr;
        int blinkingRow = -1;
        openPaneShowingBlinkingText(pane, blinkingRow);
        if (QTest::currentTestFailed()) {
            return;
        }
        const int blinkingLine = pane->imageTopLine() + blinkingRow;

        // Settles any pending paint, so all that follows is the tick's own doing
        pane->repaint();
        pane->slot_blinkStateChanged();
        QVERIFY2(!pane->mForceUpdate, "a blink tick asked for the whole pane");
        QCOMPARE(pane->mDirtyFirstLine, blinkingLine - 1);
        QCOMPARE(pane->mDirtyLastLine, blinkingLine + 1);

        // The blink phase moves on undrawn, so the cache holds a stale opacity
        PaintSwallower swallower;
        pane->installEventFilter(&swallower);
        QTest::qWait(300ms);
        pane->removeEventFilter(&swallower);

        // No event loop from here on, so the blink phase holds still
        const int rowHeight = pane->mFontHeight;
        pane->slot_blinkStateChanged();
        pane->repaint(QRect(0, (blinkingRow - 1) * rowHeight, pane->width(), 3 * rowHeight));
        const QImage afterTick = pane->cachedScreen().copy();
        pane->forceUpdate();
        pane->repaint();
        const QImage afterFullRepaint = pane->cachedScreen().copy();
        QVERIFY2(afterTick == afterFullRepaint, "the rows a blink tick redrew left the cached screen different from a whole-pane repaint");
    }

    // A scroll shorter than a screen keeps the timer running, so only a tick finding
    // nothing that is drawn blinking (the lone blinking accent is not) can stop it.
    void test_blinkTimerStopsOnceTheBlinkingLineScrollsAway()
    {
        TTextEdit* pane = nullptr;
        int blinkingRow = -1;
        openPaneShowingBlinkingText(pane, blinkingRow);
        if (QTest::currentTestFailed()) {
            return;
        }
        const int linesToScroll = blinkingRow + 1;
        QVERIFY2(linesToScroll < pane->getScreenHeight(), "scrolling the blinking line away takes a whole screenful, which repaints the pane in full anyway");

        TBuffer& buffer = *pane->mpBuffer;
        const int lastLineBefore = buffer.getLastLineNumber();
        mpServer->sendRaw(fillerLines(linesToScroll, QLatin1Char('Y')).toUtf8());
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

    // Stacked combining marks can ink rows past a neighbour of their own line
    void test_stackedMarksOnScreenMakeATickRepaintTheWholePane()
    {
        TTextEdit* pane = nullptr;
        int blinkingRow = -1;
        openPaneShowingBlinkingText(pane, blinkingRow);
        if (QTest::currentTestFailed()) {
            return;
        }
        TBuffer& buffer = *pane->mpBuffer;
        const int lastLineBefore = buffer.getLastLineNumber();
        mpServer->sendRaw(qsl("Stacked A\u0308\u0308\u0308 marks\r\n").toUtf8());
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             return buffer.getLastLineNumber() > lastLineBefore;
                         },
                         5000),
                 "the stacked marks never reached the buffer");

        pane->repaint();
        pane->slot_blinkStateChanged();
        QVERIFY2(pane->mForceUpdate, "a blink tick redrew only some rows while stacked marks could ink rows outside them");
    }

private:
    void openPaneShowingBlinkingText(TTextEdit*& pane, int& blinkingRow)
    {
        mpServer->setWelcomeMessage(fillerLines(80, QLatin1Char('X')));
        auto host = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(host, "No active host available for the test.");
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(2s), "Could not connect with the host.");

        QPointer<TMainConsole> console = host->mainConsoleView();
        QVERIFY(console);
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             for (int i = console->buffer.getLastLineNumber(); i >= 0; --i) {
                                 if (console->buffer.line(i) == QString(100, QLatin1Char('X'))) {
                                     return true;
                                 }
                             }
                             return false;
                         },
                         5000),
                 "the filler never reached the buffer");
        QVERIFY(host->mTelnet.setEncoding("UTF-8", false).first);
        mpServer->sendRaw(blinkingLines());
        QVERIFY2(QTest::qWaitFor(
                         [&]() {
                             const int last = console->buffer.getLastLineNumber();
                             return console->buffer.line(last) == QString(100, QLatin1Char('Z')) || console->buffer.line(last - 1) == QString(100, QLatin1Char('Z'));
                         },
                         5000),
                 "the blinking lines never reached the buffer");

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
