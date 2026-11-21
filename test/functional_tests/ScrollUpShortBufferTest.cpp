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
#include <QWheelEvent>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "TCommandLine.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Scrolling back while all the text fits on screen has nothing to show, so it
// must not split the console into two panes of the same lines
class ScrollUpShortBufferTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = qsl("Test-ScrollUpShort");
    const QString mpLocalhost = qsl("localhost");
    QString mpPort;

    TMainConsole* startProfile()
    {
        mpServer->setSendsWelcome(false);
        auto host = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        if (!host) {
            return nullptr;
        }
        QSignalSpy spy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy.wait(2s)) {
            return nullptr;
        }
        // The panes are laid out, and so know how many rows they hold, only once
        // the window they are in is up
        mudlet::self()->resize(1200, 800);
        mudlet::self()->show();
        if (!QTest::qWaitForWindowExposed(mudlet::self())) {
            return nullptr;
        }
        qApp->processEvents();
        return host->mainConsoleView();
    }

    static void wheelUp(TTextEdit* pane)
    {
        const QPointF position(pane->width() / 2.0, pane->height() / 2.0);
        QWheelEvent event(position, pane->mapToGlobal(position), QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(pane, &event);
    }

    static void echoLine(TMainConsole* console, const int number) { console->print(qsl("LINE %1\n").arg(number)); }

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
        TestSettings::deleteProfileDirectory(mpHostname);
    }

    void cleanup()
    {
        const QString profilePath = MudletApp::getMudletPath(enums::profileHomePath, mpHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        QDir(profilePath).removeRecursively();
    }

    void test_scrollingUpOverAFewLinesDoesNotSplit_data()
    {
        QTest::addColumn<QString>("how");
        QTest::newRow("mouse wheel") << qsl("wheel");
        QTest::newRow("Page Up") << qsl("pageUp");
        QTest::newRow("one line") << qsl("line");
    }

    void test_scrollingUpOverAFewLinesDoesNotSplit()
    {
        QFETCH(QString, how);
        TMainConsole* console = startProfile();
        QVERIFY2(console, "the profile did not start");
        TTextEdit* upper = console->mUpperPane;
        for (int line = 1; line <= 5; ++line) {
            echoLine(console, line);
        }
        qApp->processEvents();
        QVERIFY2(upper->getScreenHeight() > static_cast<int>(console->buffer.size()) + 2, "the pane is too short to hold the lines with room to spare");
        QCOMPARE(upper->imageTopLine(), 0);

        if (how == qsl("wheel")) {
            wheelUp(upper);
        } else if (how == qsl("pageUp")) {
            QVERIFY(console->mpCommandLine);
            QTest::keyClick(console->mpCommandLine, Qt::Key_PageUp);
        } else {
            console->scrollUp(1);
        }
        // the split's scroll and Page Up's page both run on 0ms timers
        QTest::qWait(50ms);

        QVERIFY2(console->mLowerPane->isHidden(), "scrolling up with nothing above the screen split the console");
        QVERIFY2(upper->mIsTailMode, "scrolling up with nothing above the screen took the console out of following new lines");
        QCOMPARE(upper->imageTopLine(), 0);
    }

    // Up to a screenful there is nothing to scroll back to; the first line more is
    // something, and scrolling up has to reach it
    void test_theFirstLineOffTheTopIsWhatLetsItSplit()
    {
        TMainConsole* console = startProfile();
        QVERIFY2(console, "the profile did not start");
        TTextEdit* upper = console->mUpperPane;
        const int screenHeight = upper->getScreenHeight();
        QVERIFY2(screenHeight > 4, "the pane is too short for this test to mean anything");
        QCOMPARE(upper->imageTopLine(), 0);

        int line = 0;
        while (upper->imageTopLine() == 0) {
            QVERIFY2(line <= 2 * screenHeight, "the text never went off the top of the pane");
            wheelUp(upper);
            qApp->processEvents();
            QVERIFY2(console->mLowerPane->isHidden(), qPrintable(qsl("with %1 lines all on screen, scrolling up split the console").arg(console->buffer.size())));
            echoLine(console, ++line);
        }

        wheelUp(upper);
        QTRY_VERIFY2(!upper->mIsTailMode, "scrolling up with a line above the screen did not scroll back");
        QVERIFY2(!console->mLowerPane->isHidden(), "scrolling back did not open the split-screen scrollback");
    }
};

#include "ScrollUpShortBufferTest.moc"
MUDLET_GROUPED_TEST_MAIN(ScrollUpShortBufferTest)
