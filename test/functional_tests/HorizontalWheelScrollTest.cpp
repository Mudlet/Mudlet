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

#include <QScrollBar>
#include <QTemporaryDir>
#include <QWheelEvent>
#include <QtTest/QtTest>
#include <utility>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// With the horizontal scroll bar on, a sideways wheel tilt or touchpad swipe
// over the text must move the view along long lines the way dragging the bar does
class HorizontalWheelScrollTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = qsl("Test-HorizontalWheel");
    const QString mpLocalhost = qsl("localhost");
    QString mpPort;

    static void wheelSideways(TTextEdit* pane, const int angle, const Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        const QPointF position(pane->width() / 2.0, pane->height() / 2.0);
        QWheelEvent event(position, pane->mapToGlobal(position), QPoint(), QPoint(angle, 0), Qt::NoButton, modifiers, Qt::NoScrollPhase, false);
        QApplication::sendEvent(pane, &event);
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

    void test_sidewaysWheelMovesTheHorizontalScrollBar()
    {
        mpServer->setSendsWelcome(false);
        auto host = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY2(host, "the profile did not start");
        QSignalSpy spy(&(host->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(spy.wait(2s), "the profile did not connect");
        mudlet::self()->resize(1200, 800);
        mudlet::self()->show();
        QVERIFY(QTest::qWaitForWindowExposed(mudlet::self()));

        TMainConsole* console = host->mainConsoleView();
        QVERIFY(console);
        TTextEdit* upper = console->mUpperPane;
        console->setWrapAt(1000);
        console->setHorizontalScrollBar(true);
        console->print(QString(800, QChar('x')) + qsl("\n"));
        QScrollBar* bar = console->mpHScrollBar;
        QTRY_VERIFY2(!bar->isHidden(), "a line wider than the pane did not bring up the horizontal scroll bar");
        QVERIFY(bar->maximum() > 10);
        QCOMPARE(bar->value(), 0);

        wheelSideways(upper, -120);
        qApp->processEvents();
        const int afterRight = bar->value();
        QVERIFY2(afterRight > 0, "a negative x delta did not scroll along the long line");

        wheelSideways(upper, 120);
        qApp->processEvents();
        QVERIFY2(bar->value() < afterRight, "a positive x delta did not scroll back towards the start");

        // A fast swipe delivers several events before the next repaint
        bar->setValue(0);
        qApp->processEvents();
        wheelSideways(upper, -120);
        const int oneStep = bar->value();
        QVERIFY2(oneStep > 0, "the bar did not follow the swipe before the repaint");
        wheelSideways(upper, -120);
        QCOMPARE(bar->value(), 2 * oneStep);
        qApp->processEvents();
        QCOMPARE(bar->value(), 2 * oneStep);

        // One notch moves three columns, as a vertical one moves three lines; Shift slows it to one, Ctrl speeds it to ten
        const std::pair<Qt::KeyboardModifiers, int> steps[] = {{Qt::NoModifier, 3}, {Qt::ShiftModifier, 1}, {Qt::ControlModifier, 10}};
        for (const auto& [modifiers, columns] : steps) {
            bar->setValue(0);
            qApp->processEvents();
            wheelSideways(upper, -120, modifiers);
            QCOMPARE(bar->value(), columns);
        }

        // A touchpad sends many small deltas, which must add up to whole columns
        bar->setValue(0);
        qApp->processEvents();
        for (int i = 0; i < 120 / 2; ++i) {
            wheelSideways(upper, -2);
        }
        QCOMPARE(bar->value(), 3);
    }
};

#include "HorizontalWheelScrollTest.moc"
MUDLET_GROUPED_TEST_MAIN(HorizontalWheelScrollTest)
