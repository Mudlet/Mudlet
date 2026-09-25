/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

/*
 * No profile is opened: the replay toolbar and its multiplier belong to the
 * main window, and the Host handed to replayStart() is only stored - every use
 * of it (pause, resume, stop and the paused marker on the time readout) is
 * null-guarded, and none of them is on the Faster path this test drives.
 *
 * Run with: ctest -R ReplaySpeedClampTest -V
 */

#include <QAction>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "mudlet.h"

#include "GroupedTest.h"

class ReplaySpeedClampTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

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

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
    }

    void cleanupTestCase()
    {
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            mudlet::self()->replayOver();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // PR #6468 (issue #6466): the Faster button doubles the multiplier on every
    // click, and clicking on took it past what an int holds.
    void test_speedingUpAReplayStopsAtTheCap()
    {
        QVERIFY2(mudlet::self()->replayStart(nullptr), "the replay toolbar would not start, so there is no Faster button to press");
        QCOMPARE(mudlet::self()->mReplaySpeed, 1);
        QAction* pFaster = mudlet::self()->findChild<QAction*>(qsl("replay_speed_up_action"));
        QVERIFY2(pFaster, "the replay toolbar carries no Faster button");

        pFaster->trigger();
        QCOMPARE(mudlet::self()->mReplaySpeed, 2);

        // far more clicks than the cap needs
        for (int click = 0; click < 20; ++click) {
            pFaster->trigger();
        }

        QCOMPARE(mudlet::self()->mReplaySpeed, 1024);
    }
};

#include "ReplaySpeedClampTest.moc"
MUDLET_GROUPED_TEST_MAIN(ReplaySpeedClampTest)
