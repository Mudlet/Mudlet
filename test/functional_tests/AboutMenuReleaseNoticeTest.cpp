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
 * When an update is found the toolbar's About button says so, but the
 * "Check for updates..." item in the menubar's About menu kept saying
 * "Check for updates..." (#5110).
 *
 * Run with: ctest -R AboutMenuReleaseNoticeTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "mudlet.h"
#if defined(INCLUDE_UPDATER)
#include "updater.h"
#endif

#include "GroupedTest.h"

class AboutMenuReleaseNoticeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;

private slots:
    void initTestCase()
    {
#if !defined(INCLUDE_UPDATER)
        QSKIP("built without the updater");
#else
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        QVERIFY(mudlet::self()->pUpdater);
#endif
    }

#if defined(INCLUDE_UPDATER)
    void test_menuItemAnnouncesAvailableUpdates()
    {
        auto* pMudlet = mudlet::self();
        QCOMPARE(pMudlet->dactionUpdate->text(), qsl("Check for updates..."));

        emit pMudlet->pUpdater->signal_updateAvailable(2);

        QCOMPARE(pMudlet->dactionUpdate->text(), mudlet::tr("Review %n update(s)...", nullptr, 2));
    }

#if !defined(Q_OS_MACOS)
    // The feed keeps listing an installed update until Mudlet restarts into it
    void test_laterCheckKeepsRestartPrompt()
    {
        auto* pMudlet = mudlet::self();
        emit pMudlet->pUpdater->signal_updateInstalled();
        QCOMPARE(pMudlet->dactionUpdate->text(), qsl("Update installed - restart to apply"));

        emit pMudlet->pUpdater->signal_updateAvailable(1);

        QCOMPARE(pMudlet->dactionUpdate->text(), qsl("Update installed - restart to apply"));
    }
#endif
#endif
};

#include "AboutMenuReleaseNoticeTest.moc"
MUDLET_GROUPED_TEST_MAIN(AboutMenuReleaseNoticeTest)
