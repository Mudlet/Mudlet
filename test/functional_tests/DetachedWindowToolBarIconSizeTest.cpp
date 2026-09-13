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
 * The toolbar icon size setting has to reach a detached profile window's toolbar
 * as well as the main window's. There are two ways for it not to: the setting
 * changing while the window is already open, and a window detached after the
 * change starting out at whatever its toolbar was built with.
 *
 * The dropdown buttons need checking separately from the toolbar itself. They go
 * in with addWidget() rather than as actions, and QToolBar only propagates its
 * style to the buttons it creates for actions - a widget it merely holds keeps
 * whatever style it was given.
 *
 * Run with: ctest -R DetachedWindowToolBarIconSizeTest -V
 */

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QToolBar>
#include <QToolButton>
#include <QtTest/QtTest>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TDetachedWindow.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

class DetachedWindowToolBarIconSizeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstHostname = qsl("DetachedWindowToolBarIconSize-First");
    const QString mSecondHostname = qsl("DetachedWindowToolBarIconSize-Second");
    // Both sides of the setting's only branch: 4 draws text under the icons, 1 is
    // small enough to drop the text and leave 8x8 icons. 4 rather than the
    // default 3 because 3 works out to the 24x24 icons a toolbar that was never
    // told a size ends up with anyway, which would leave nothing to measure
    static constexpr int mLargeIconSize = 4;
    static constexpr int mSmallIconSize = 1;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so that a second copy of this test
        // running at the same time is not told the profile names are in use. The
        // opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        deleteProfileDirectory(mFirstHostname);
        deleteProfileDirectory(mSecondHostname);

        // Two of them, because slot_tabDetachRequested() refuses index 0
        startProfile(mFirstHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        startProfile(mSecondHostname);
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mFirstHostname);
            deleteProfileDirectory(mSecondHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Every test starts from the large size and detaches for itself, so that the
    // setting can change before or after the detach as the case needs
    void init() { mudlet::self()->setToolBarIconSize(mLargeIconSize); }

    void cleanup()
    {
        if (mudlet::self()->getDetachedWindows().contains(mSecondHostname)) {
            mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        }
    }

    void test_theIconSizeChangeReachesAnOpenDetachedWindow()
    {
        TDetachedWindow* pDetachedWindow = detachSecondProfile();
        QVERIFY(pDetachedWindow);
        QToolBar* pDetachedToolBar = detachedToolBar(pDetachedWindow);
        QVERIFY(pDetachedToolBar);
        QCOMPARE(pDetachedToolBar->toolButtonStyle(), Qt::ToolButtonTextUnderIcon);

        mudlet::self()->setToolBarIconSize(mSmallIconSize);

        QCOMPARE(pDetachedToolBar->toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(pDetachedToolBar->iconSize(), QSize(mSmallIconSize * 8, mSmallIconSize * 8));
        // Against the main window as well as the numbers, since matching it is the point
        QCOMPARE(pDetachedToolBar->toolButtonStyle(), mudlet::self()->mpMainToolBar->toolButtonStyle());
        QCOMPARE(pDetachedToolBar->iconSize(), mudlet::self()->mpMainToolBar->iconSize());
    }

    // A window detached after the setting changed is a separate failure from a
    // missed live update: its toolbar is built from scratch at that point
    void test_aFreshlyDetachedWindowStartsAtTheCurrentIconSize()
    {
        mudlet::self()->setToolBarIconSize(mSmallIconSize);

        TDetachedWindow* pDetachedWindow = detachSecondProfile();
        QVERIFY(pDetachedWindow);
        QToolBar* pDetachedToolBar = detachedToolBar(pDetachedWindow);
        QVERIFY(pDetachedToolBar);

        QCOMPARE(pDetachedToolBar->toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(pDetachedToolBar->iconSize(), QSize(mSmallIconSize * 8, mSmallIconSize * 8));
    }

    void test_theDropdownButtonsFollowTheIconSize()
    {
        TDetachedWindow* pDetachedWindow = detachSecondProfile();
        QVERIFY(pDetachedWindow);

        mudlet::self()->setToolBarIconSize(mSmallIconSize);

        for (const QString& buttonName : {qsl("connect"), qsl("mute"), qsl("discord"), qsl("package_manager")}) {
            QToolButton* pButton = pDetachedWindow->findChild<QToolButton*>(buttonName);
            QVERIFY2(pButton, qPrintable(qsl("the detached window has no '%1' toolbar button").arg(buttonName)));
            QVERIFY2(pButton->toolButtonStyle() == Qt::ToolButtonIconOnly, qPrintable(qsl("the '%1' button still draws its text under its icon at the smallest icon size").arg(buttonName)));
        }
    }

    // Going back up has to work as well as going down - the branch that picks the
    // style is only exercised one way by the tests above
    void test_theIconSizeChangeBackToLargeReachesADetachedWindow()
    {
        mudlet::self()->setToolBarIconSize(mSmallIconSize);
        TDetachedWindow* pDetachedWindow = detachSecondProfile();
        QVERIFY(pDetachedWindow);
        QToolBar* pDetachedToolBar = detachedToolBar(pDetachedWindow);
        QVERIFY(pDetachedToolBar);

        mudlet::self()->setToolBarIconSize(mLargeIconSize);

        QCOMPARE(pDetachedToolBar->toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QCOMPARE(pDetachedToolBar->iconSize(), QSize(mLargeIconSize * 8, mLargeIconSize * 8));
        QCOMPARE(pDetachedToolBar->iconSize(), mudlet::self()->mpMainToolBar->iconSize());
    }

private:
    QToolBar* detachedToolBar(TDetachedWindow* pDetachedWindow) const { return pDetachedWindow ? pDetachedWindow->findChild<QToolBar*>(qsl("detachedMainToolBar")) : nullptr; }

    TDetachedWindow* detachSecondProfile()
    {
        if (!mudlet::self()->getDetachedWindows().isEmpty()) {
            QTest::qFail("a detached window was left over from an earlier test", __FILE__, __LINE__);
            return nullptr;
        }

        mudlet::self()->slot_tabDetachRequested(1, QPoint(200, 200));

        TDetachedWindow* pDetachedWindow = mudlet::self()->getDetachedWindows().value(mSecondHostname);
        if (!pDetachedWindow) {
            // Whichever profile sits at tab 1 is the one that detaches, so say
            // which one was expected rather than only that nothing appeared
            QTest::qFail(qPrintable(qsl("detaching tab 1 produced no window for '%1' - the tab order is not what this test assumes").arg(mSecondHostname)), __FILE__, __LINE__);
        }
        return pDetachedWindow;
    }

    void startProfile(const QString& hostname)
    {
        auto host = TestProfile::create(hostname, mLocalhost, mPort);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectionSpy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connectionSpy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "DetachedWindowToolBarIconSizeTest.moc"
MUDLET_GROUPED_TEST_MAIN(DetachedWindowToolBarIconSizeTest)
