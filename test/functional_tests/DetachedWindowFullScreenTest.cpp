/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include <QAction>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "TDetachedWindow.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// A detached window's full-screen toggle has to bring a maximized window back maximized, as the main window's does,
// and its menu item and toolbar button have to show the window's state whichever of them, or the window manager,
// changed it.
class DetachedWindowFullScreenTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstHostname = qsl("DetachedWindowFullScreen-First");
    const QString mSecondHostname = qsl("DetachedWindowFullScreen-Second");

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

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->show();

        deleteProfileDirectory(mFirstHostname);
        deleteProfileDirectory(mSecondHostname);
        // Two of them: detachTab() leaves the main window at least one tab
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
        if (mudlet::self()) {
            deleteProfileDirectory(mFirstHostname);
            deleteProfileDirectory(mSecondHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init() { QVERIFY2(mudlet::self()->getDetachedWindows().isEmpty(), "a detached window was left over from an earlier case"); }

    void cleanup()
    {
        const QStringList detachedProfiles = mudlet::self()->getDetachedWindows().keys();
        for (const QString& profileName : detachedProfiles) {
            mudlet::self()->slot_tabReattachRequested(profileName);
        }
    }

    void test_leavingFullScreenKeepsTheWindowMaximized_data() { addControlRows(); }

    void test_leavingFullScreenKeepsTheWindowMaximized()
    {
        QFETCH(QString, actionName);
        TDetachedWindow* pWindow = detachProfile(mSecondHostname);
        QVERIFY(pWindow);
        QAction* pToggle = action(pWindow, actionName);
        QVERIFY(pToggle);
        pWindow->setWindowState(Qt::WindowMaximized);
        QVERIFY(pWindow->windowState() & Qt::WindowMaximized);

        pToggle->trigger();
        QVERIFY(pWindow->windowState() & Qt::WindowFullScreen);

        pToggle->trigger();
        QVERIFY(!(pWindow->windowState() & Qt::WindowFullScreen));
        QVERIFY2(pWindow->windowState() & Qt::WindowMaximized, "leaving full screen un-maximized the window");
    }

    void test_bothControlsShowTheStateEitherSet_data() { addControlRows(); }

    void test_bothControlsShowTheStateEitherSet()
    {
        QFETCH(QString, actionName);
        TDetachedWindow* pWindow = detachProfile(mSecondHostname);
        QVERIFY(pWindow);
        QAction* pToggle = action(pWindow, actionName);
        QVERIFY(pToggle);
        QAction* pMenu = action(pWindow, qsl("menuFullScreen"));
        QAction* pButton = action(pWindow, qsl("fullscreen_action"));
        QVERIFY(pMenu);
        QVERIFY(pButton);

        pToggle->trigger();
        QVERIFY(pWindow->isFullScreen());
        QVERIFY(pMenu->isChecked());
        QVERIFY(pButton->isChecked());

        pToggle->trigger();
        QVERIFY(!pWindow->isFullScreen());
        QVERIFY(!pMenu->isChecked());
        QVERIFY(!pButton->isChecked());
    }

    void test_checkmarksFollowAStateChangeMadeOutsideMudlet()
    {
        TDetachedWindow* pWindow = detachProfile(mSecondHostname);
        QVERIFY(pWindow);
        QAction* pMenu = action(pWindow, qsl("menuFullScreen"));
        QAction* pButton = action(pWindow, qsl("fullscreen_action"));
        QVERIFY(pMenu);
        QVERIFY(pButton);

        pWindow->setWindowState(Qt::WindowFullScreen);
        QVERIFY(pMenu->isChecked());
        QVERIFY(pButton->isChecked());

        pWindow->setWindowState(Qt::WindowNoState);
        QVERIFY(!pMenu->isChecked());
        QVERIFY(!pButton->isChecked());
    }

private:
    static void addControlRows()
    {
        QTest::addColumn<QString>("actionName");
        QTest::newRow("Window menu") << qsl("menuFullScreen");
        QTest::newRow("toolbar") << qsl("fullscreen_action");
    }

    static QAction* action(TDetachedWindow* pWindow, const QString& name) { return pWindow ? pWindow->findChild<QAction*>(name) : nullptr; }

    TDetachedWindow* detachProfile(const QString& profileName)
    {
        const int tabIndex = mudlet::self()->mpTabBar->tabIndex(profileName);
        if (tabIndex < 0) {
            QTest::qFail(qPrintable(qsl("'%1' has no tab in the main window").arg(profileName)), __FILE__, __LINE__);
            return nullptr;
        }
        mudlet::self()->slot_tabDetachRequested(tabIndex, QPoint(200, 200));
        TDetachedWindow* pWindow = mudlet::self()->getDetachedWindows().value(profileName);
        if (!pWindow) {
            QTest::qFail(qPrintable(qsl("detaching tab %1 produced no window for '%2'").arg(tabIndex).arg(profileName)), __FILE__, __LINE__);
        }
        return pWindow;
    }

    void startProfile(const QString& hostname)
    {
        auto host = TestProfile::create(hostname, mLocalhost, mPort);
        if (!host) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy connectionSpy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connectionSpy.wait(2s)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "DetachedWindowFullScreenTest.moc"
MUDLET_GROUPED_TEST_MAIN(DetachedWindowFullScreenTest)
