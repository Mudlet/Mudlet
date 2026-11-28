/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TBuffer.h"
#include "TPaintPacer.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

#include "GroupedTest.h"

class PaintPacerTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = qsl("Test-PaintPacer");
    QString mpPort;
    const QString mpLocalhost = qsl("localhost");
    const QRect mView{0, 0, 100, 100};

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
        TestProfile::removeProfileDirectory(mpHostname);
    }

    void cleanup()
    {
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        TestProfile::removeProfileDirectory(mpHostname);
    }

    // Nothing has been painted yet, so the frame's window is open
    void test_aScheduleBeforeAnyPaintGoesOutAtOnce()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        TBuffer buffer(pHost);
        TPaintPacer pacer(buffer, nullptr);
        QSignalSpy painted(&pacer, &TPaintPacer::paintWanted);
        pacer.schedule(QRect(0, 0, 10, 10), mView);
        QCOMPARE(painted.count(), 1);
        QCOMPARE(painted.at(0).at(0).value<QRegion>(), QRegion(QRect(0, 0, 10, 10)));
        QVERIFY(!pacer.mpPaintPacer->isActive());
    }

    void test_aScheduleInsideTheFrameArmsTheTimerForTheRest()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        TBuffer buffer(pHost);
        TPaintPacer pacer(buffer, nullptr);
        QSignalSpy painted(&pacer, &TPaintPacer::paintWanted);
        pacer.paintStarted(QRegion());
        pacer.schedule(QRect(0, 0, 10, 10), mView);
        QCOMPARE(painted.count(), 0);
        QVERIFY(pacer.mpPaintPacer->isActive());
        QVERIFY(pacer.mpPaintPacer->remainingTime() <= TPaintPacer::csmPaintPaceMs);
        QTRY_COMPARE_WITH_TIMEOUT(painted.count(), 1, 1000);
        QCOMPARE(painted.at(0).at(0).value<QRegion>(), QRegion(QRect(0, 0, 10, 10)));
    }

    void test_aScreenOfLinesBetweenFramesStretchesThePace()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        TBuffer buffer(pHost);
        TPaintPacer pacer(buffer, nullptr);
        pacer.setScreenRows([]() {
            return 5;
        });
        pacer.paintStarted(QRegion());
        QCOMPARE(pacer.paceMs(), TPaintPacer::csmPaintPaceMs);
        for (int line = 0; line < 5; ++line) {
            buffer.appendEmptyLine();
        }
        QCOMPARE(pacer.paceMs(), TPaintPacer::csmPaintPaceMs);
        buffer.appendEmptyLine();
        QCOMPARE(pacer.paceMs(), TPaintPacer::csmFloodPaintPaceMs);
    }

    // The view moves its scroll bar before it paints, so the two arrive together
    void test_aPendingScrollBarUpdateIsHandedOverWithTheFrame()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        TBuffer buffer(pHost);
        TPaintPacer pacer(buffer, nullptr);
        QStringList order;
        connect(&pacer, &TPaintPacer::scrollBarWanted, this, [&order]() {
            order << qsl("scroll bar");
        });
        connect(&pacer, &TPaintPacer::paintWanted, this, [&order]() {
            order << qsl("paint");
        });
        pacer.setScrollBarUpdatePending(true);
        pacer.schedule(QRect(0, 0, 10, 10), mView);
        QCOMPARE(order, (QStringList{qsl("scroll bar"), qsl("paint")}));
    }
};

#include "PaintPacerTest.moc"
MUDLET_GROUPED_TEST_MAIN(PaintPacerTest)
